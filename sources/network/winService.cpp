#include "winService.h"


SERVICE_STATUS			g_ss;
SERVICE_STATUS_HANDLE	g_ssHandle;

void WinService_t::UpdateStatusCtrl(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	if (dwCurrentState == SERVICE_START_PENDING)
		g_ss.dwControlsAccepted = 0;
	else
		g_ss.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	g_ss.dwCurrentState = dwCurrentState;
	g_ss.dwWin32ExitCode = dwWin32ExitCode;
	g_ss.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
		g_ss.dwCheckPoint = 0;
	else
		g_ss.dwCheckPoint = dwCheckPoint++;

	SetServiceStatus(g_ssHandle, &g_ss);
}

void WINAPI WinService_t::ServiceControl(DWORD dwControlCode)
{
	switch (dwControlCode)
	{
	case SERVICE_CONTROL_STOP:
		UpdateStatusCtrl(SERVICE_STOP_PENDING, NO_ERROR, 0);
		g_bServiceStop = true;
		break;

	default:
		UpdateStatusCtrl(g_ss.dwCurrentState, NO_ERROR, 0);
		break;
	}
}


const char* WinErrorInfo()
{
	static char sBuf[1024];

	DWORD uErr = ::GetLastError();
	sprintf(sBuf, "code=%d, error=", uErr);

	int iLen = strlen(sBuf);
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, uErr, 0, sBuf + iLen, sizeof(sBuf) - iLen, NULL)) // FIXME? force US-english langid?
		strcpy(sBuf + iLen, "(no message)");

	return sBuf;
}


SC_HANDLE WinService_t::OpenManager()
{
	SC_HANDLE hSCM = OpenSCManager(
		NULL,					// local computer
		NULL,					// ServicesActive database 
		SC_MANAGER_ALL_ACCESS);// full access rights

	if (hSCM == NULL)
		perror("OpenSCManager() failed: %s", WinErrorInfo());

	return hSCM;
}


void strappend(char* sBuf, const int iBufLimit, char* sAppend)
{
	int iLen = strlen(sBuf);
	int iAppend = strlen(sAppend);

	int iToCopy = std::min(iBufLimit - iLen - 1, iAppend);
	memcpy(sBuf + iLen, sAppend, iToCopy);
	sBuf[iLen + iToCopy] = '\0';
}


void WinService_t::Install(int argc, char** argv)
{
	if (g_bService)
		return;

	sphInfo("Installing service...");

	char szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
		sphFatal("GetModuleFileName() failed: %s", WinErrorInfo());

	strappend(szPath, sizeof(szPath), " --ntservice");
	for (int i = 1; i < argc; i++)
		if (strcmp(argv[i], "--install"))
		{
			strappend(szPath, sizeof(szPath), " ");
			strappend(szPath, sizeof(szPath), argv[i]);
		}

	SC_HANDLE hSCM = OpenManager();
	SC_HANDLE hService = CreateService(
		hSCM,							// SCM database 
		g_sServiceName,					// name of service 
		g_sServiceName,					// service name to display 
		SERVICE_ALL_ACCESS,				// desired access 
		SERVICE_WIN32_OWN_PROCESS,		// service type 
		SERVICE_AUTO_START,				// start type 
		SERVICE_ERROR_NORMAL,			// error control type 
		szPath,							// path to service's binary 
		NULL,							// no load ordering group 
		NULL,							// no tag identifier 
		NULL,							// no dependencies 
		NULL,							// LocalSystem account 
		NULL);							// no password 

	if (!hService)
	{
		CloseServiceHandle(hSCM);
		sphFatal("CreateService() failed: %s", WinErrorInfo());

	}
	else
	{
		sphInfo("Service '%s' installed succesfully.", g_sServiceName);
	}

	CSphString sDesc;
	sDesc.SetSprintf("%s-%s", g_sServiceName, SPHINX_VERSION);

	SERVICE_DESCRIPTION tDesc;
	tDesc.lpDescription = (LPSTR)sDesc.cstr();
	if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &tDesc))
		sphWarning("failed to set service description");

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
}


void WinService_t::Uninstall()
{
	if (g_bService)
		return;

	sphInfo("Deleting service...");

	// open manager
	SC_HANDLE hSCM = OpenManager();

	// open service
	SC_HANDLE hService = OpenService(hSCM, g_sServiceName, DELETE);
	if (!hService)
	{
		CloseServiceHandle(hSCM);
		sphFatal("OpenService() failed: %s", WinErrorInfo());
	}

	// do delete
	bool bRes = !!DeleteService(hService);
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);

	if (!bRes)
		sphFatal("DeleteService() failed: %s", WinErrorInfo());
	else
		sphInfo("Service '%s' deleted succesfully.", g_sServiceName);

}