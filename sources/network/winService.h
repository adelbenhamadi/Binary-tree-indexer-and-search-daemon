#pragma once
#include "network.h"

class WinService_t : public Service_t {
	void UpdateStatusCtrl(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
	void WINAPI ServiceControl(DWORD dwControlCode);
	SC_HANDLE OpenManager();
	void Install(int argc, char** argv);
	void Uninstall();
};

