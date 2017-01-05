//file PPWRUN.CPP

//Descr: Ётот кусок кода запускает через рандомные промежутки
//  времени ppw.exe, который находитс€ в C:\ppy\bin\ppw.exe
//  ¬ переменную ret пишетс€ сгенерированное число.
//  ќно генерируетс€ функцией GetRandom в промежутке между min и max 

#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <Winuser.h>
#include <string.h>
#include <process.h>

#define NUMTIMES 30

int GetRandom(int min, int max) {return (int) (((double)rand() / (RAND_MAX + 1)) * (max - min) + min);}

// argc - number of parametrs comming from command line
// argv[] - array of parametrs comming from command line
int main(int argc, char * argv[])
{
	/*char cmdl[256];
	memset(cmdl, 0, sizeof(cmdl));
	for(int i = 1; i < argc; i++) {
		strcat(cmdl, argv[i]);
		if(i != argc - 1)
			strcat(cmdl, " ");
	}*/
	srand((unsigned)time(NULL));
    STARTUPINFO sti;                
    ZeroMemory(&sti,sizeof(STARTUPINFO));
    sti.cb = sizeof(STARTUPINFO);        
    sti.wShowWindow = TRUE;
    PROCESS_INFORMATION pi;
    for(int i = 0; i < NUMTIMES; i++) {
		int ret = GetRandom(1, 3);
		printf("%d\n", ret);
		Sleep(ret);
		CreateProcess("C:\\ppy\\bin\\ppw.exe",GetCommandLine(),NULL,NULL,FALSE,NULL,NULL,NULL,&sti,&pi);
		//CreateProcess("C:\\ppy\\bin\\ppw.exe",(LPSTR)cmdl,NULL,NULL,FALSE,NULL,NULL,NULL,&sti,&pi);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		//Sleep(1000);
		//TerminateProcess();
		//SendMessage(GetConsoleHwnd(), WM_CLOSE, 0, 0);
		//execl("C:\\ppy\\bin\\ppw.exe", cmdl, NULL);
		//spawnl(_P_NOWAIT, "C:\\ppy\\bin\\ppw.exe", cmdl, NULL);
    }
    return 0;
}