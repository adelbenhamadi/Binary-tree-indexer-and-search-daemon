//***************************************************************************************/
//*    snippet of this code are udapted from Sphinxsearch source code
//*    Author: Copyright (c) 2001-2016, Andrew Aksyonoff / Copyright (c) 2008-2016, Sphinx Technologies Inc
//*    Date: ?
//*    Code version: 2.95
//*    Availability: http://www.sphinxsearch.com
//*
//***************************************************************************************///

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