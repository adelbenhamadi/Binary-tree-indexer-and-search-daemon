#pragma once

/// remote agent state
enum AgentState_e
{
	AGENT_UNUSED,				///< agent is unused for this request
	AGENT_CONNECT,				///< connecting to agent
	AGENT_HELLO,				///< waiting for "VER x" hello
	AGENT_QUERY,				///< query sent, wating for reply
	AGENT_REPLY,				///< reading reply
	AGENT_RETRY					///< should retry
};


