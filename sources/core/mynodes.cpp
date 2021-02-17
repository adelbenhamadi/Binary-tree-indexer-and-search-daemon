
#include "mynodes.h"

#include <queue>


constexpr auto PATH_MAX = 1024;

using namespace mynodes::globals;

namespace mynodes
{

	

	std::ofstream openOutputFile(const char* cFile,char *sError) {
		char filename[PATH_MAX];
		sprintf(filename, "%s/%s", g_sDataInitDir, cFile);
		auto ret = realpath(filename, filename);
		std::ofstream os(filename, std::ios::out | std::ios::binary);
		if (!os.is_open())
			snprintf(sError,255,"failed to open %s: %s", cFile, strerror(errno));
		return os;

	}
	std::ifstream openInputFile(const char* cFile,char *sError) {
		char filename[PATH_MAX];
		sprintf(filename, "%s/%s", g_sDataInitDir, cFile);
		auto ret = realpath(filename, filename);
		//open file
		std::ifstream os(filename, std::ios::in | std::ios::binary);
		if (!os.is_open())
			snprintf(sError, 256, "failed to open %s: %s", cFile, strerror(errno));
		return os;

	}

	MyIndex::MyIndex(const string_t& sName, const string_t& sPath)
		:m_sIndexName(sName)
		, m_sIndexPath(sPath)
		, m_tPrefixNodes()
		, m_tSuffixNodes()
		, m_tSettings()
		, m_tLeaves()
		, m_iResultCount(0)
		, m_iCurrentMode(1)
		, m_tIndexStats()
		, m_tResultItems()
		, m_tIoStats()
	
	{
		m_sError[255] = '\0';
#if ENABLE_INFIX
		m_tInfixNodes = NodeCollection();
#endif
	}
	void MyIndex::Setup(MySourceSettings& tSettings)
	{
	}

	bool MyIndex::build()
	{
		assert(vSources.size());
		// setup sources
		for (auto pSource : vSources)
		{
			assert(pSource);
			pSource->ResetStats();
		}

		bool result = false;
		for (auto pSource : vSources)
		{
			debugPrintf(1, "\nbuild from source: %s", pSource->m_sName);
#if ASYNC_JOB
			debugPrintf(1, "\n..start async job");
			std::future<bool> asyncJob = std::async(std::launch::async, &MyIndex::buildFromSource, this, pSource, m_tSettings., true);
			//wait for async job
			result = result || asyncJob.get();
#else
			result = buildFromSource(pSource, m_tSettings.m_iMemLimit, m_tSettings.m_iWriteBufferSize);
#endif
		}
		return result;
	}
	inline int isValidChar(const char* c)
	{
		return (*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c == '-') || (*c == '_');
	}
	bool MyIndex::buildFromSource(MySource* pSource, size_t iMemLimit, size_t iWriteBufferSz)
	{
		constexpr int _max_word_len = 1024;
		char sToken[_max_word_len];
		MySourceStats* sourceStats = pSource->getStats();
		bool& bShowProgress = m_tSettings.m_bShowProgress;
		string_t slvsIndexTmpFilename = getIndexFilename("lvs.tmp");
		string_t slvsIndexFilename = getIndexFilename("lvs");

		FileWriter os(slvsIndexTmpFilename);
		if (!os.opened()) {
			printError("\nError: cannot open file %s for writing , aborting..", slvsIndexTmpFilename.c_str());
			return false;
		}
		std::vector<BinBlock> dBlocks;
		std::vector<LeafItem> buffer;
		iWriteBufferSz = std::max(iMemLimit / sizeof(LeafItem) , 1024ULL * 1024ULL);
		//buffer.reserve(bufferSize);	
		unsigned int iToken = 0;
		size_t sz0, curr = 0, iBlock = 0, ilastBlockOffset = 0;
		//column field to index
		m_iCurrentCol = 0;
		int fieldSz = 0;
		
		string_t word;
		const char* sNext;
		char* sError = const_cast <char*>("");
		int pw = 0, wordPos;
		const size_t iDocBufSz = 10;
		did_t docId = 0;
		///////////////////////////////////////////////////////
		//Indexing phase
		///////////////////////////////////////////////////////

		sourceStats->Reset(MySourceStats::INDEX);
		while (pSource->load(iDocBufSz, sError)) {
			for (auto& doc : pSource->m_tDocuments)
			{
				sourceStats->m_iDocumentsCount++;
				wordPos = -1;
				string_t& document = doc.second[m_iCurrentCol];
				for (auto it = document.begin(); it != document.end(); ++it) {
					*it |= 0x20; //tolower
					//*it &= ~0x20;//toupper

				}
				//std::transform(document.begin(), document.end(), document.begin(), ::tolower);
				if (document.empty())
				{
					continue;
				}
				docId = doc.first;
				const char* p = document.c_str();
				sourceStats->m_iBytes += (int)strlen(p);

				while (*p)
				{
					//strip html
					while (*p && !isValidChar(p)) {
						if (*p == '&') {
							while (*p) {
								p++;
								if (*p == ';') break;
							}
						}
						else if (*p == '<') {

							if ((p[1] == 'p' || p[1] == 'i' || p[1] == 'b') && p[2] == '>') p += 3;
							else if (((p[1] == 'u' && p[2] == 'l') || (p[1] == 'o' && p[2] == 'l') || (p[1] == 'l' && p[2] == 'i')) && p[3] == '>') p += 4;
							else if (
								((p[1] == 'p' && p[2] == 'r' && p[3] == 'e'))
								&& p[4] == '>') p += 5;
							else if (
								(p[1] == 's' && p[2] == 'c' && p[3] == 'r' && p[4] == 'i' && p[5] == 'p' && p[6] == 't')||
								(p[1] == 's' && p[2] == 't' && p[3] == 'y' && p[4] == 'l' && p[5] == 'e')||
								(p[1] == 'c' && p[2] == 'o' && p[3] == 'd' && p[4] == 'e')||
								(p[1] == 't' && p[2] == 'e' && p[3] == 'm' && p[4] == 'p' && p[5] == 'l' && p[6] == 'a' && p[7] == 't' && p[8] == 'e')

								) {
								while (*p) {
									p++;
									if (*p == '<' && p[1] == '/') break;
								}
							}
							else{
								while (*p) {
									p++;
									if (*p == '>') break;
								}
							}

						}//end if (*p == '<')
						else  p++;

					}

					//tokenize
					sNext = p;
					while (isValidChar(p))
						p++;
					if (sNext != p)
					{
						sz0 = p - sNext;
						if (sz0 < g_iMinKeywordSize) continue;
						if (sz0 > 255)	sz0 = 255;
						strncpy(sToken, sNext, sz0);
						sToken[sz0] = '\0';
					}
					else
					{
						continue;
					}
					word = sToken;
					
					//update position
					wordPos++;
					//stemming & hashing
					iToken = m_hasher.hash(word, m_tSettings.m_bStemming);
					//find leaf if not create one
					Leaf& leaf = m_tLeaves.m_vDict[iToken];
					buffer.emplace_back(LeafItem(docId));
					buffer.back().tokenId = iToken;
					buffer.back().position = wordPos;
					debugPrintf(10, "\n%s did: %llu, tokenId: %llu", word, leaf.m_iLastAddedId, buffer.back().tokenId);


#if ENABLE_PREFIX
					// leaf.m_vLeafItems.back().hits++;
					 //if (!findPrefix(word))
					if (!m_tPrefixNodes.find(word))
					{
						m_iCurrentMode = 1;
						create_node(word, csKEY_ZERO);
					}
#endif
#if ENABLE_SUFFIX
					//if (!findSuffix(word))
					if (!m_tSuffixNodes.find(word))
					{
						m_iCurrentMode = 0;
						create_node(word, csKEY_ZERO);
					}
#endif
				}//end while

				//done with this doc
				if (buffer.size() >= iWriteBufferSz) {
					std::sort(buffer.begin(), buffer.end(), LeafItemCompare());
					const size_t& bs = buffer.size();
					os.write(&(buffer)[0], bs * sizeof(LeafItem));
					sourceStats->m_iTotalHits += bs;
					dBlocks.emplace_back(BinBlock(bs, ilastBlockOffset));
					ilastBlockOffset += bs;
					iBlock++;
					buffer.clear();
					if (bShowProgress)
					{
						sourceStats->printProgress();
					}
				}

			}//for
			if (bShowProgress)
			{
				sourceStats->printProgress();
			}
		}//while
		pSource->close();
		//finally sort, write out what's left and close		
		if (buffer.size()) {
			std::sort(buffer.begin(), buffer.end(), LeafItemCompare());
			os.write(&(buffer)[0], buffer.size() * sizeof(LeafItem));
			sourceStats->m_iTotalHits += buffer.size();
			dBlocks.emplace_back(BinBlock(buffer.size(), ilastBlockOffset));
			ilastBlockOffset = 0;
			iBlock++;
			if (bShowProgress)
			{
				sourceStats->printProgress(true);
			}
		}
		os.close();
		buffer.clear();

		/////////////////////////////////////////////////////
		//sorting phase (external if needed)
		/////////////////////////////////////////////////////
		sourceStats->Reset(MySourceStats::SORT);
		if (dBlocks.size() > 1) {
			std::priority_queue<std::pair<LeafItem, int>, std::vector<std::pair<LeafItem, int> >, MinHeapCompare> minHeap;
			FileReader is(slvsIndexTmpFilename);
			if (!is.opened()) {
				diePrintf("\nError: Cannot open tmp file [%s] for reading, aborting", slvsIndexTmpFilename.c_str());
				return false;
			}
			size_t blockIdx = 0;

			//read first value from every binBlock and push it in minHeap
			for (size_t i = 0; i < dBlocks.size(); i++) {
				//update BinBlock chunckSize
				dBlocks[i].updateChuncks(dBlocks.size());
				if (!dBlocks[i].read(&is)) {
					diePrintf("I/O error: reading from BinBlock %llu failed, aborting!", i);
				}
				const LeafItem& firstValue = *(dBlocks[i].more(1));
				minHeap.emplace(std::pair<LeafItem, int>(firstValue, i));
			}

			sourceStats->m_iSortedHits = dBlocks.size();
			FileWriter outputFile(slvsIndexFilename);
			if (outputFile.opened()) {
				while (minHeap.size() > 0) {
					auto minPair = minHeap.top();
					auto& currentBlock = dBlocks[minPair.second];
					buffer.push_back(minPair.first);
					sourceStats->m_iSortedHits++;
					minHeap.pop();
					if (currentBlock.read(&is)) {
						const LeafItem& nextValue = *currentBlock.more();
						minHeap.emplace(std::pair<LeafItem, int>(nextValue, minPair.second));
					}
					else {
						debugPrintf(10, "\nError: buf/minheap: %llu/%llu   bytes from block: %d",
							sourceStats->m_iSortedHits, minHeap.size(), minPair.second);
					}

					if (buffer.size() >= iWriteBufferSz) {
						outputFile.write(&(buffer)[0], iWriteBufferSz * sizeof(LeafItem));
						buffer.clear();
					}

					if (bShowProgress) {
						sourceStats->printProgress();
					}
				}
				//finally write last buffer
				if (buffer.size()) {
					outputFile.write(&(buffer)[0], buffer.size() * sizeof(LeafItem));
					if (bShowProgress) {
						sourceStats->printProgress(true);
					}
				}
				outputFile.close();
			}
			is.close();
			std::remove(slvsIndexTmpFilename.c_str());
		}//end external sorting
		 //no external sorting needed just rename tmp file
		else
		{
			if (bShowProgress) {
				sourceStats->printProgress(true);
			}
						
			if (std::remove(slvsIndexFilename.c_str())) {
				printError("I/O Error: Cannot remove file %s", slvsIndexFilename.c_str());
			}
			else {
				if (std::rename(slvsIndexTmpFilename.c_str(), slvsIndexFilename.c_str())) {
					printError("I/O Error: Cannot rename tmp file %s", slvsIndexTmpFilename.c_str());
				}
			}
		}
		//clean up
		dBlocks.clear();
		buffer.clear();
		return true;
	}

	//optimized version of create_node()
	bool MyIndex::create_node(string_t& word, const string_t& left)
	{
		string_t parent;
		switch (m_iCurrentMode)
		{
		case 0:
			m_tSuffixNodes.add(word, left, csKEY_ZERO);
			parent = word.substr(1);
			break;
		case 1:
			m_tPrefixNodes.add(word, left, csKEY_ZERO);
			parent = word.substr(0, word.length() - 1);
			break;
		}

		if (!parent.empty())
		{
			create_node(parent, word);
		}
		return true;
	}

	bool MyIndex::create_node(string_t& word, const string_t& left,
		const string_t& right, const int iMode, const did_t iDoc)
	{
		string_t parent;
		switch (iMode)
		{

		case 1:
			m_tPrefixNodes.add(word, left, right);
			parent = word.substr(0, word.length() - 1);
			break;
#if ENABLE_INFIX
		case 2:
			assert(iDoc >= 0);
			m_tInfixNodes.add(word, left, right);
			if (!m_tLeaves.find(word))
				m_tLeaves.add(word);

			m_tLeaves.m_vCollection[word].addItem(iDoc);
			parent = word.substr(0, word.length() - 1);
			break;
#endif
		case 0:
			m_tSuffixNodes.add(word, left, right);
#if ENABLE_PRE_SUFFIX
			//create prefix node for suffix word
			create_node(word, csKEY_ZERO, csKEY_ZERO, 2, iDoc);
			debugPrintf("\ncreate_node prefix for suffix %s", word.c_str());
#endif
			parent = word.substr(1);
			break;
		}

		//
		if (!parent.empty())
		{
			create_node(parent, word, csKEY_ZERO, iMode, iDoc);
		}
		return true;
		}

	bool MyIndex::follow(const string_t& sFirst, const string_t& sSecond,
		const int& iFollowMode, const bool& right, LeafItemsVector_t* pMergedLeafItems)
	{
#if (ENABLE_PREFIX || ENABLE_SUFFIX)
		assert(m_pSearchNodeCol && "m_pSearchNodeCol is NULL!");

		if (!m_pSearchNodeCol->find(sFirst))
			return false;
#endif
		debugPrintf(3, "\n follow %s-%s", sFirst.c_str(), sSecond.c_str());
		//check before merging
		size_t iKey = m_hasher.hash(sFirst, m_tSettings.m_bStemming);
		bool result = m_tLeaves.find(iKey);
		if (iFollowMode == 2)
		{
			result &= utility::hasSuffix(sFirst, sSecond);
		}
		if (result)
		{

			Leaf& res = m_tLeaves.m_vDict[iKey];
			m_iResultCount += res.m_iCount;
			std::move(res.m_vLeafItems.begin(), res.m_vLeafItems.end(), std::back_inserter(*pMergedLeafItems));
			//LeafItemsVector_t::iterator it = pMergedLeafItems->begin();
			//pMergedLeafItems->insert(it, res.begin(), res.end());

		}
		string_t sTmp;
		//no need to follow for exact searching
		if (iFollowMode == 3) {
			/*

			  #if (ENABLE_PREFIX || ENABLE_SUFFIX)
				sTmp = sFirst + "-";
				follow(sTmp, sSecond, 1, false, pMergedLeafItems);
			  #endif
			  */
			return true;
		}
		//continue following left and right
		sTmp = m_pSearchNodeCol->m_vNodes[sFirst].left;

		if (!sTmp.empty())
			follow(sTmp, sSecond, iFollowMode, true, pMergedLeafItems);
		if (right)
		{
			sTmp = m_pSearchNodeCol->m_vNodes[sFirst].right;

			if (!sTmp.empty())
				follow(sTmp, sSecond, iFollowMode, true, pMergedLeafItems);
		}
		return true;
	}
	void MyIndex::doSearch(const string_t& l, const string_t& r, const int& iFollowMode)
	{

		clearResults();

		switch (iFollowMode)
		{
		case 0: //suffix searching like *ty

			m_pSearchNodeCol = &m_tSuffixNodes;
			break;
		case 1: //prefix searching like az*

			m_pSearchNodeCol = &m_tPrefixNodes;
			break;
		case 2: //infix searching like az*ty
			m_pSearchNodeCol = &m_tPrefixNodes;
			//nodcol = &preSuffixNodes;
			break;
		case 3: //exact search like azerty
			m_pSearchNodeCol = &m_tPrefixNodes;
			break;
		case 4: //infix search like *ert* TODO

			break;
		}
		follow(l, r, iFollowMode, false, &m_tResultItems);
	}
	void MyIndex::doSearch(const Expression_t& hExpression)
	{
		string_t sKey = hExpression.value;
		std::transform(sKey.begin(), sKey.end(), sKey.begin(), ::tolower);
		const string_t sStar = "*";
		string_t sTmp;
		std::size_t pos = sKey.find(sStar);
		if (pos == STD_NPOS)
		{
			doSearch(sKey, sTmp, 3);
			return;
		}
		if (pos == 0)
		{
			sKey = sKey.substr(pos + 1, STD_NPOS);
			doSearch(sKey, sTmp, 0);
			return;
		}
		if (pos > 0 && pos < sKey.size() - 1)
		{
			sTmp = sKey.substr(pos + 1, STD_NPOS);
			sKey = sKey.substr(0, pos);
			doSearch(sKey, sTmp, 2);
			return;
		}
		if (pos == sKey.size() - 1)
		{
			sKey = sKey.substr(0, sKey.length() - 1);
			doSearch(sKey, sTmp, 1);
			return;
		}
	}

	bool MyIndex::save_nodes()
	{
		FileWriter os(getIndexFilename("nds"));

		if (os.opened()) {
			//write prefix collection, count first
			u64_t co = m_tPrefixNodes.m_vNodes.size();
			os.write((char*)&co, sizeof(u64_t));
			for (auto& it : m_tPrefixNodes.m_vNodes)
			{

				debugPrintf(6, "\n\n%s <-- %s --> %s", it.second.left.c_str(), it.first.c_str(), it.second.right.c_str());
				os.writeString(it.first);
				os.writeString(it.second.left);
				os.writeString(it.second.right);

			}
			co = m_tSuffixNodes.m_vNodes.size();
			os.write((char*)&co, sizeof(u64_t));
			for (auto& it : m_tSuffixNodes.m_vNodes)
			{

				debugPrintf(6, "\n\n%s <-- %s --> %s", it.second.left.c_str(), it.first.c_str(), it.second.right.c_str());
				os.writeString(it.first);
				os.writeString(it.second.left);
				os.writeString(it.second.right);

			}
			os.close();
		}
		else
		{
			perror("Error opening file for writing");
			return false;
		}

		return true;
	}
	bool MyIndex::save_dict()
	{
		FileWriter os(getIndexFilename("dic"));
		if (os.opened()) {

			size_t co = m_tLeaves.m_vDict.size();
			os.write((char*)&co, sizeof(size_t));
			for (auto& it : m_tLeaves.m_vDict)
			{
				co = it.first;
				os.write((char*)&co, sizeof(size_t));
				//os.writeString(it.first);
			}

			os.close();
		}
		else
		{
			perror("Error opening file for writing");
			return false;
		}

		return true;
	}
	bool MyIndex::load_dict()
	{
		m_tLeaves.reset();
		FileReader os(getIndexFilename("dic"));
		if (os.opened()) {
			size_t sz, co = 0;
			os.read((char*)&co, sizeof(size_t));
			for (u64_t i = 0; i < co; i++)
			{
				os.read((char*)&sz, sizeof(size_t));
				m_tLeaves.add(sz);
				//const string_t& word = readStringFromBinFile(os);
				//m_tIoStats.m_iBytesIn += word.size();
				//m_tLeaves.add(word);
			}
			os.close();
		}
		else
		{
			perror("Error opening file for reading");
			return false;
		}

		return true;
	}
	bool MyIndex::load_nodes()
	{
		FileReader os(getIndexFilename("nds"));
		if (os.opened()) {
			//read count
			size_t co;
			//char word[128], left[128], right[128];
			//read prefixes
			os.read((char*)&co, sizeof(u64_t));
			for (u64_t i = 0; i < co; i++)
			{

				const string_t& word = os.readString();
				const string_t& left = os.readString();
				const string_t& right = os.readString();
				debugPrintf(10, "\n%lld.read Prefix: %s <-- %s --> %s", i, left.c_str(), word.c_str(), right.c_str());
				m_tPrefixNodes.add0(word, left, right);

				m_tIoStats.m_iBytesIn += strlen(word.c_str()) + strlen(left.c_str()) + strlen(right.c_str());
				/*
				readStringFromBinFile(os, word);
				readStringFromBinFile(os, left);
				readStringFromBinFile(os, right);
				debugPrintf(1, "\n%lld.read Prefix: %s <-- %s --> %s", i, left,	word, right);
				m_tPrefixNodes.add0((string_t) word, (string_t)left, (string_t) right);

				m_tIoStats.m_iBytesIn += strlen(word) + strlen(left) + strlen(right);
				*/
			}
			//read suffixes
			/*os.read((char*)&co, sizeof(u64_t));
			for (u64_t i = 0; i < co; i++)
			{

				const string_t  word = readStringFromBinFile(os);
				const string_t  left = readStringFromBinFile(os);
				const string_t  right = readStringFromBinFile(os);
				debugPrintf(6, "\n%lld.read suffix: %s <-- %s --> %s", i, left.c_str(),	word.c_str(), right.c_str());
				//m_tSuffixNodes.add(word, left, right);
				m_tIoStats.m_iBytesIn += strlen(word.c_str()) + strlen(left.c_str()) + strlen(right.c_str());

			}*/
			os.close();
		}
		else
		{
			perror("Error opening file for reading");
			return false;
		}

		return true;
	}
	/*
	bool MyIndex::save_leaves(const char* dir)
	{
		char filename[PATH_MAX];
		sprintf(filename, "%s/%s", dir, m_sIndexPath.c_str());
		realpath(filename, filename);
		strcat(filename, ".lvs");
		std::ofstream os(filename, std::ios::out | std::ios::binary);
		if (os)
		{

			u64_t co = m_tLeaves.m_vCollection.size();
			os.write((char*)&co, sizeof(u64_t));
			//write collection
			size_t sz = 0;
			for (auto& it : m_tLeaves.m_vCollection)
			{
				//write word
				writeStringToBinFile(it.first, os);
				co = it.second.m_vLeafItems.size();
				co = co > g_iMaxLeafSize ? g_iMaxLeafSize : co;

				debugPrintf(5, "\nwriting %s => %llu items", it.first.c_str(), co);
				debugPrintf(5, "\t hits: %d", it.second.m_vLeafItems[0].hits);

				os.write((char*)&co, sizeof(u64_t));
				if (co > 0)
				{
					os.write((char*)&(it.second.m_vLeafItems)[0], co * sizeof(LeafItem));

				}
			}
			os.close();
		}
		else
		{
			perror("Error opening file for writing");
			return false;
		}

		return true;
	}
	*/
	bool MyIndex::load_leaves()
	{
		m_tLeaves.m_iLeavesCount = 0;
		FileReader os(getIndexFilename("lvs"));
		if (os.opened()) {

			size_t sz = sizeof(LeafItem);
			tid_t lastTokenId = 0;
			did_t lastDocId = 0;
			int ibufSize = sz * 1024 * 1024;
			std::vector<LeafItem> vTmp;
			vTmp.insert(vTmp.begin(), ibufSize, LeafItem());
			while (!os.eof())
			{
				int co = os.read((char*)&(vTmp)[0], ibufSize);
				co = co / sizeof(LeafItem);
				Leaf* leaf = &m_tLeaves.m_vDict[vTmp[0].tokenId];
				for (int i = 0; i < co; i++) {
					LeafItem& key = vTmp[i];
					debugPrintf(10, "\nkey(%lu,%lu,%d,%d)", key.did, key.tokenId, key.position, key.hits);
					if (key.tokenId == 0 && key.did == 0) continue;
					if (key.tokenId != lastTokenId) {
						leaf = &m_tLeaves.m_vDict[key.tokenId];
					}
					if (key.did == lastDocId && key.tokenId == lastTokenId) //same tokenId and did
					{
						leaf->m_vLeafItems.back().hits++;
					}
					else {
						//read only a segment of 1000 items
							//if (leaf->m_iCount < 1000) 
						leaf->m_vLeafItems.emplace_back(key);
						leaf->m_vLeafItems.back().hits = 1;
						m_tLeaves.m_iLeavesCount++;
					}
					leaf->m_iCount++;

					lastDocId = key.did;
					lastTokenId = key.tokenId;
				}

			}
			m_tIoStats.m_iBytesIn += sz * m_tLeaves.m_iLeavesCount;
			os.close();
		}
		else
		{
			perror("Error opening file for reading");
			return false;
		}
		return true;
	}
	/*
	bool MyIndex::load_leaves(const char* dir)
	{
		char filename[PATH_MAX];
		sprintf(filename, "%s/%s", dir, m_sIndexPath.c_str());
		realpath(filename, filename);
		strcat(filename, ".lvs");

		//open file
		std::ifstream os(filename, std::ios::in | std::ios::binary);
		if (os) {
			size_t sz, icount, leavesCount = 0;
			os.read((char*)&leavesCount, sizeof(u64_t));
			for (u64_t i = 0; i < leavesCount; i++)
			{
				const string_t& word = readStringFromBinFile(os);
				m_tIoStats.m_iBytesIn += word.size();
				m_tLeaves.add(word);
				//read items count
				os.read((char*)&icount, sizeof(u64_t));

				debugPrintf(5, "\n%lld .nreading %s => %llu items", i, word.c_str(), icount);
				Leaf& leaf = m_tLeaves.m_vCollection[word];
				leaf.m_iCount = icount;
				//read only a segment of 100 items
				if (icount > 100) icount = 100;
				leaf.m_vLeafItems.resize(icount);
				os.read((char*)&(leaf.m_vLeafItems)[0], icount * sizeof(LeafItem));
				m_tIoStats.m_iBytesIn += (icount * sizeof(LeafItem));
				if (leaf.m_iCount > icount) os.seekg((leaf.m_iCount - icount) * sizeof(LeafItem), os.cur);

				debugPrintf(5, "\t hits: %lld", leaf.m_vLeafItems[0].hits);

			}
			os.close();
		}
		else
		{
			perror("Error opening file for reading");
			return false;
		}
		return true;
	}
	*/

	string_t MyIndex::getIndexFilename(const char* cExt) {
		string_t t = m_sIndexPath + "." + cExt;
		return t;
	}
	bool MyIndex::save_documents(MySource* pSource)
	{

		FileWriter os(getIndexFilename("dcs"));
		if (os.opened())
		{

			size_t docid, co, co2;
			//write count
			co = pSource->count();
			os.write((char*)&co, sizeof(size_t));
			for (auto& doc : pSource->m_tDocuments)
			{
				//write docId as from database
				docid = doc.first;
				os.write((char*)&docid, sizeof(did_t));
				//write columns count
				co2 = doc.second.size();
				os.write((char*)&co2, sizeof(size_t));
				//write only first column
				os.writeString(doc.second[0]);
				//write all columns
				/*for (auto &v : doc.second)
					writeStringToBinFile(v, os);
					*/
			}
			os.close();
		}
		else
		{
			perror("Error opening file for writing");
			return false;
		}

		return true;
	}
	bool MyIndex::load_documents(MySource* pSource)
	{
		FileReader os(getIndexFilename("dcs"));

		if (os.opened())
		{

			//read count
			string_t content;
			u64_t co, docid;
			size_t co2;
			SourceCollection_t& source = pSource->m_tDocuments;
			source.clear();
			os.read((char*)&co, sizeof(u64_t));
			for (u64_t i = 0; i < co; i++)
			{
				//get docID
				os.read((char*)&docid, sizeof(u64_t));
				//sVector_t& doc = source[docid];
				//get columns count
				os.read((char*)&co2, sizeof(size_t));
				//get only fisrt column
				content = os.readString();
				source[docid].emplace_back(content);
				//read all columns
				/*for (unsigned int j = 0; j < co2; j++)
				{
					content = readStringFromBinFile(os);
					source[docid].emplace_back(content);
					debugPrintf(6,"\nreading doc %llu => %s ", docid, content.c_str());
				}*/
			}

			os.close();
		}
		else
		{
			perror("Error opening file for reading");
			return false;
		}
		return true;
	}
	bool MyIndex::save_data()
	{
		timePoint_t t_s = STD_NOW;
		save_dict();
		utility::print_time("\tDict saved in", t_s);

		t_s = STD_NOW;
		save_nodes();
		utility::print_time("\tNodes saved in", t_s);

		t_s = STD_NOW;
		for (auto pSource : vSources)
			save_documents(pSource);
		utility::print_time("\tDocuments saved in", t_s);

		return true;
	}
	bool MyIndex::load_data(bool b_loadDocs)
	{

		char mess[64] = "";
		m_tLeaves.reset();
		m_tIoStats.reset();

		timePoint_t t_s = STD_NOW;
		load_dict();
		utility::print_time(" Dict loaded in", t_s);

		t_s = STD_NOW;
		load_leaves();
		sprintf(mess, " Leaves loaded: %llu  %8.2f Mb in", m_tLeaves.m_iLeavesCount, (float)m_tIoStats.m_iBytesIn / (1024 * 1024));
		utility::print_time(mess, t_s);

		m_tIoStats.reset();
		t_s = STD_NOW;
		load_nodes();
		sprintf(mess, " Nodes loaded: %llu  %8.2f Mb in", m_tPrefixNodes.count(), (float)m_tIoStats.m_iBytesIn / (1024 * 1024));
		utility::print_time(mess, t_s);
		if (!b_loadDocs)  return true;

		m_tIoStats.reset();
		t_s = STD_NOW;
		load_documents(vSources[0]);
		sprintf(mess, " Documents loaded: %llu  %8.2f Mb in", vSources[0]->count(), (float)m_tIoStats.m_iBytesIn / (1024 * 1024));
		utility::print_time(mess, t_s);
		return true;
	}
	string_t MyIndex::get_document(MySource* pSource, const LeafItem& wh)
	{
		string_t doc;
		//auto source = *(pSource->m_pDocuments);
		SourceCollection_t::const_iterator it = pSource->m_tDocuments.find(wh.did);
		if (it != pSource->m_tDocuments.end()) {
			//only load first column
			doc = it->second[0];
			/*
			for (auto &v : source[wh.did])
				doc = doc + '\t' + v;
				*/
		}

		return doc;
	}
	void MyIndex::clearResults()
	{
		m_iResultCount = 0;
		m_tResultItems.clear();
	}

	} // namespace mynodes
