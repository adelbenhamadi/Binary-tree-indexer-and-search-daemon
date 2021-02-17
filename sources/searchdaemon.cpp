
#include "network/network.h"

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>




void ShowHelp() {}


#if USE_WINDOWS
BOOL WINAPI CtrlHandler(DWORD)
{
	if (!g_bService)
		g_bGotSigterm = true;
	return TRUE;
}
#endif


int WINAPI ServiceMain(int argc, char** argv)
{

	exit(0);

}
int main(int argc, char** argv)
{
	return ServiceMain(argc, argv);

}