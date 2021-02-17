
void ConnectToRemoteAgents(CSphVector<Agent_t>& dAgents, bool bRetryOnly)
{
	ARRAY_FOREACH(iAgent, dAgents)
	{
		Agent_t& tAgent = dAgents[iAgent];
		if (bRetryOnly && tAgent.m_eState != AGENT_RETRY)
			continue;

		tAgent.m_eState = AGENT_UNUSED;
		tAgent.m_bSuccess = false;

		struct sockaddr_in sa;
		memset(&sa, 0, sizeof(sa));
		memcpy(&sa.sin_addr, tAgent.GetAddr(), tAgent.GetAddrLen());
		sa.sin_family = (short)tAgent.GetAddrType();
		sa.sin_port = htons((unsigned short)tAgent.m_iPort);

		tAgent.m_iSock = socket(tAgent.GetAddrType(), SOCK_STREAM, 0);
		if (tAgent.m_iSock < 0)
		{
			tAgent.m_sFailure.SetSprintf("socket() failed: %s", sphSockError());
			return;
		}

		if (sphSetSockNB(tAgent.m_iSock) < 0)
		{
			tAgent.m_sFailure.SetSprintf("sphSetSockNB() failed: %s", sphSockError());
			return;
		}

		if (connect(tAgent.m_iSock, (struct sockaddr*) & sa, sizeof(sa)) < 0)
		{
			int iErr = sphSockGetErrno();
			if (iErr != EINPROGRESS && iErr != EINTR && iErr != EWOULDBLOCK) // check for EWOULDBLOCK is for winsock only
			{
				tAgent.Close();
				tAgent.m_sFailure.SetSprintf("connect() failed: %s", sphSockError(iErr));
				tAgent.m_eState = AGENT_RETRY; // do retry on connect() failures
				return;

			}
			else
			{
				// connection in progress
				tAgent.m_eState = AGENT_CONNECT;
			}
		}
		else
		{
			// socket connected, ready to read hello message
			tAgent.m_eState = AGENT_HELLO;
		}
	}
}


int QueryRemoteAgents(CSphVector<Agent_t>& dAgents, int iTimeout, const IRequestBuilder_t& tBuilder)
{
	int iAgents = 0;
	assert(iTimeout >= 0);

	int iPassed = 0;
	float tmStart = sphLongTimer();
	while (iPassed <= iTimeout)
	{
		fd_set fdsRead, fdsWrite;
		FD_ZERO(&fdsRead);
		FD_ZERO(&fdsWrite);

		int iMax = 0;
		bool bDone = true;
		ARRAY_FOREACH(i, dAgents)
		{
			const Agent_t& tAgent = dAgents[i];
			if (tAgent.m_eState == AGENT_CONNECT || tAgent.m_eState == AGENT_HELLO)
			{
				assert(tAgent.m_iPort > 0);
				assert(tAgent.m_iSock > 0);

				sphFDSet(tAgent.m_iSock, (tAgent.m_eState == AGENT_CONNECT) ? &fdsWrite : &fdsRead);
				iMax = Max(iMax, tAgent.m_iSock);
				bDone = false;
			}
		}
		if (bDone)
			break;

		iPassed = int(1000.0f * (sphLongTimer() - tmStart));
		int iToWait = Max(iTimeout - iPassed, 0);

		struct timeval tvTimeout;
		tvTimeout.tv_sec = iToWait / 1000; // full seconds
		tvTimeout.tv_usec = (iToWait % 1000) * 1000; // remainder is msec, so *1000 for usec

		// FIXME! check exceptfds for connect() failure as well, so that actively refused
		// connections would not stall for a full timeout
		if (select(1 + iMax, &fdsRead, &fdsWrite, NULL, &tvTimeout) <= 0)
			continue;

		ARRAY_FOREACH(i, dAgents)
		{
			Agent_t& tAgent = dAgents[i];

			// check if connection completed
			if (tAgent.m_eState == AGENT_CONNECT && FD_ISSET(tAgent.m_iSock, &fdsWrite))
			{
				int iErr = 0;
				socklen_t iErrLen = sizeof(iErr);
				getsockopt(tAgent.m_iSock, SOL_SOCKET, SO_ERROR, (char*)&iErr, &iErrLen);
				if (iErr)
				{
					// connect() failure
					tAgent.m_sFailure.SetSprintf("connect() failed: %s", sphSockError(iErr));
					tAgent.Close();
				}
				else
				{
					// connect() success
					tAgent.m_eState = AGENT_HELLO;
				}
				continue;
			}

			// check if hello was received
			if (tAgent.m_eState == AGENT_HELLO && FD_ISSET(tAgent.m_iSock, &fdsRead))
			{
				// read reply
				int iRemoteVer;
				int iRes = sphSockRecv(tAgent.m_iSock, (char*)&iRemoteVer, sizeof(iRemoteVer));
				iRemoteVer = ntohl(iRemoteVer);
				if (iRes != sizeof(iRemoteVer) || iRemoteVer <= 0)
				{
					tAgent.m_sFailure.SetSprintf("expected protocol v.%d, got v.%d", SPHINX_SEARCHD_PROTO, iRemoteVer);
					tAgent.Close();
					continue;
				}

				// send request
				NetOutputBuffer_c tOut(tAgent.m_iSock);
				tBuilder.BuildRequest(tAgent.m_sIndexes.cstr(), tOut);
				tOut.Flush(); // FIXME! handle flush failure?

				tAgent.m_eState = AGENT_QUERY;
				iAgents++;
			}
		}
	}

	ARRAY_FOREACH(i, dAgents)
	{
		// check if connection timed out
		Agent_t& tAgent = dAgents[i];
		if (tAgent.m_eState != AGENT_QUERY && tAgent.m_eState != AGENT_UNUSED)
		{
			tAgent.Close();
			tAgent.m_sFailure.SetSprintf("%s() timed out", tAgent.m_eState == AGENT_HELLO ? "read" : "connect");
			tAgent.m_eState = AGENT_RETRY; // do retry on connect() failures
		}
	}

	return iAgents;
}


int WaitForRemoteAgents(CSphVector<Agent_t>& dAgents, int iTimeout, IReplyParser_t& tParser)
{
	assert(iTimeout >= 0);

	int iAgents = 0;
	int iPassed = 0;
	float tmStart = sphLongTimer();
	while (iPassed <= iTimeout)
	{
		fd_set fdsRead;
		FD_ZERO(&fdsRead);

		int iMax = 0;
		bool bDone = true;
		ARRAY_FOREACH(iAgent, dAgents)
		{
			Agent_t& tAgent = dAgents[iAgent];
			if (tAgent.m_eState == AGENT_QUERY || tAgent.m_eState == AGENT_REPLY)
			{
				assert(tAgent.m_iPort > 0);
				assert(tAgent.m_iSock > 0);

				sphFDSet(tAgent.m_iSock, &fdsRead);
				iMax = Max(iMax, tAgent.m_iSock);
				bDone = false;
			}
		}
		if (bDone)
			break;

		iPassed = int(1000.0f * (sphLongTimer() - tmStart));
		int iToWait = Max(iTimeout - iPassed, 0);

		struct timeval tvTimeout;
		tvTimeout.tv_sec = iToWait / 1000; // full seconds
		tvTimeout.tv_usec = (iToWait % 1000) * 1000; // remainder is msec, so *1000 for usec

		if (select(1 + iMax, &fdsRead, NULL, NULL, &tvTimeout) <= 0)
			continue;

		ARRAY_FOREACH(iAgent, dAgents)
		{
			Agent_t& tAgent = dAgents[iAgent];
			if (!(tAgent.m_eState == AGENT_QUERY || tAgent.m_eState == AGENT_REPLY))
				continue;
			if (!FD_ISSET(tAgent.m_iSock, &fdsRead))
				continue;

			// if there was no reply yet, read reply header
			bool bFailure = true;
			for (;; )
			{
				if (tAgent.m_eState == AGENT_QUERY)
				{
					// try to read
					struct
					{
						WORD	m_iStatus;
						WORD	m_iVer;
						int		m_iLength;
					} tReplyHeader;
					STATIC_SIZE_ASSERT(tReplyHeader, 8);

					if (sphSockRecv(tAgent.m_iSock, (char*)&tReplyHeader, sizeof(tReplyHeader)) != sizeof(tReplyHeader))
					{
						// bail out if failed
						tAgent.m_sFailure.SetSprintf("failed to receive reply header");
						break;
					}

					tReplyHeader.m_iStatus = ntohs(tReplyHeader.m_iStatus);
					tReplyHeader.m_iVer = ntohs(tReplyHeader.m_iVer);
					tReplyHeader.m_iLength = ntohl(tReplyHeader.m_iLength);

					// check the packet
					if (tReplyHeader.m_iLength<0 || tReplyHeader.m_iLength>MAX_PACKET_SIZE) // FIXME! add reasonable max packet len too
					{
						tAgent.m_sFailure.SetSprintf("invalid packet size (status=%d, len=%d, max_packet_size=%d)", tReplyHeader.m_iStatus, tReplyHeader.m_iLength, MAX_PACKET_SIZE);
						break;
					}

					// header received, switch the status
					assert(tAgent.m_pReplyBuf == NULL);
					tAgent.m_eState = AGENT_REPLY;
					tAgent.m_pReplyBuf = new BYTE[tReplyHeader.m_iLength];
					tAgent.m_iReplySize = tReplyHeader.m_iLength;
					tAgent.m_iReplyRead = 0;
					tAgent.m_iReplyStatus = tReplyHeader.m_iStatus;

					if (!tAgent.m_pReplyBuf)
					{
						// bail out if failed
						tAgent.m_sFailure.SetSprintf("failed to alloc %d bytes for reply buffer", tAgent.m_iReplySize);
						break;
					}
				}

				// if we are reading reply, read another chunk
				if (tAgent.m_eState == AGENT_REPLY)
				{
					// do read
					assert(tAgent.m_iReplyRead < tAgent.m_iReplySize);
					int iRes = sphSockRecv(tAgent.m_iSock, (char*)tAgent.m_pReplyBuf + tAgent.m_iReplyRead,
						tAgent.m_iReplySize - tAgent.m_iReplyRead);

					// bail out if read failed
					if (iRes < 0)
					{
						tAgent.m_sFailure.SetSprintf("failed to receive reply body: %s", sphSockError());
						break;
					}

					assert(iRes > 0);
					assert(tAgent.m_iReplyRead + iRes <= tAgent.m_iReplySize);
					tAgent.m_iReplyRead += iRes;
				}

				// if reply was fully received, parse it
				if (tAgent.m_eState == AGENT_REPLY && tAgent.m_iReplyRead == tAgent.m_iReplySize)
				{
					MemInputBuffer_c tReq(tAgent.m_pReplyBuf, tAgent.m_iReplySize);

					// absolve thy former sins
					tAgent.m_sFailure = "";

					// check for general errors/warnings first
					if (tAgent.m_iReplyStatus == SEARCHD_WARNING)
					{
						CSphString sAgentWarning = tReq.GetString();
						tAgent.m_sFailure.SetSprintf("remote warning: %s", sAgentWarning.cstr());

					}
					else if (tAgent.m_iReplyStatus == SEARCHD_RETRY)
					{
						tAgent.m_eState = AGENT_RETRY;
						break;

					}
					else if (tAgent.m_iReplyStatus != SEARCHD_OK)
					{
						CSphString sAgentError = tReq.GetString();
						tAgent.m_sFailure.SetSprintf("remote error: %s", sAgentError.cstr());
						break;
					}

					// call parser
					if (!tParser.ParseReply(tReq, tAgent))
						break;

					// check if there was enough data
					if (tReq.GetError())
					{
						tAgent.m_sFailure.SetSprintf("incomplete reply");
						break;
					}

					// all is well
					iAgents++;
					tAgent.Close();

					tAgent.m_bSuccess = true;
					assert(tAgent.m_dResults.GetLength());
				}

				bFailure = false;
				break;
			}

			if (bFailure)
			{
				tAgent.Close();
				tAgent.m_dResults.Reset();
			}
		}
	}

	// close timed-out agents
	ARRAY_FOREACH(iAgent, dAgents)
	{
		Agent_t& tAgent = dAgents[iAgent];
		if (tAgent.m_eState == AGENT_QUERY)
		{
			assert(!tAgent.m_dResults.GetLength());
			assert(!tAgent.m_bSuccess);
			tAgent.Close();
			tAgent.m_sFailure.SetSprintf("query timed out");
		}
	}

	return iAgents;
}

