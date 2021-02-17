#pragma once
#include <string>

#if _WIN32
#include <winsock2.h>

typedef SOCKET Socket_t;
#else
#include <sys/types.h>
#include <unistd.h>

typedef  int Socket_t;
#endif


class NetReadBuffer {
	
	virtual uint32_t ReadDocId() {
		return Read<uint32_t>();
	}
	
	virtual bool ReadBytes(const void* pBuf, int iLen);
	template<class T> T Read();

	virtual void Flush();
};
class NetWriteBuffer  {
	
	virtual void WriteDocId(uint32_t& t) {
		Write<uint32_t>(t);
	}
	virtual bool WriteBytes(const void* pBuf, int iLen);
	template<class T>
	void Write(T& t);

	virtual void Flush();
};
enum ServiceState { ssClosed, ssPending, ssWaiting, ssStarted, ssStoped };

struct NetWork_t {
	NetWork_t() {}
	void init() {}


private:
	Socket_t _socket;
};
class Service_t {
	Service_t() : _state(ServiceState::ssClosed) {}
	~Service_t() {}
	virtual void shutdown() {}
	virtual void start() {}
	
private:
	ServiceState _state;
};
