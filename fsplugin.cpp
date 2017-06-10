// fsplugin.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "fsplugin.h"
#include "cunicode.h"

#define pluginrootlen 1
BOOL EnableDebugPrivilege(BOOL bEnable);
HANDLE hinst;

char* strlcpy(char* p,char*p2,int maxlen)
{
	if ((int)strlen(p2)>=maxlen) {
		strncpy(p,p2,maxlen);
		p[maxlen]=0;
	} else
		strcpy(p,p2);
	return p;
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
   	if (ul_reason_for_call==DLL_PROCESS_ATTACH)
		hinst=hModule;
	return TRUE;
}

int PluginNumber;
tProgressProc ProgressProc=NULL;
tLogProc LogProc=NULL;
tRequestProc RequestProc=NULL;
tProgressProcW ProgressProcW=NULL;
tLogProcW LogProcW=NULL;
tRequestProcW RequestProcW=NULL;



int __stdcall FsInitW(int PluginNr,tProgressProcW pProgressProcW,tLogProcW pLogProcW,tRequestProcW pRequestProcW)
{
	
	ProgressProcW=pProgressProcW;
    LogProcW=pLogProcW;
    RequestProcW=pRequestProcW;
	PluginNumber=PluginNr;
	return 0;
}

typedef struct {
	WCHAR PathW[wdirtypemax];
	WCHAR LastFoundNameW[wdirtypemax];
	HANDLE searchhandle;
} tLastFindStuct, *pLastFindStuct;


BOOL EnableDebugPrivilege(BOOL bEnable)
{
	HANDLE hToken = nullptr;
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) return FALSE;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) return FALSE;

	TOKEN_PRIVILEGES tokenPriv;
	tokenPriv.PrivilegeCount = 1;
	tokenPriv.Privileges[0].Luid = luid;
	tokenPriv.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) return FALSE;

	return TRUE;
}

HANDLE __stdcall FsFindFirstW(WCHAR* Path, WIN32_FIND_DATAW *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAW));
	FindData->dwFileAttributes = FILE_ATTRIBUTE_SYSTEM;
	EnableDebugPrivilege(true);
	char sbuf[wdirtypemax];
	HANDLE Proc;
	PROCESSENTRY32 peProcessEntry;
	HANDLE CONST hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot)
	{
		return INVALID_HANDLE_VALUE;
	}
	peProcessEntry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hSnapshot, &peProcessEntry);
	while (!(Proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, peProcessEntry.th32ProcessID)))
	{
		Process32Next(hSnapshot, &peProcessEntry);
	}

	GetModuleFileNameEx(Proc, 0, sbuf, MAX_PATH);
	CloseHandle(Proc);
	WCHAR buf[wdirtypemax];
	Slesh(sbuf, Path);

	HANDLE hdnl = FindFirstFileT(Path, FindData);
	if (hdnl == INVALID_HANDLE_VALUE)
	{
		return INVALID_HANDLE_VALUE;
	}
	else {
		return hSnapshot;
	}
	return INVALID_HANDLE_VALUE;
}

HANDLE snap = NULL;
HANDLE lastProc;



BOOL __stdcall FsFindNextW(HANDLE Hdl, WIN32_FIND_DATAW *FindData)
{
	EnableDebugPrivilege(true);
	memset(FindData, 0, sizeof(WIN32_FIND_DATAW));
	FindData->dwFileAttributes = FILE_ATTRIBUTE_SYSTEM;
	if (snap == NULL) snap = Hdl;
	HANDLE Proc;
	PROCESSENTRY32 peProcessEntry;
	peProcessEntry.dwSize = sizeof(PROCESSENTRY32);
	Process32Next(snap, &peProcessEntry);
	while (!(Proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, peProcessEntry.th32ProcessID)))
	{
		if(!(Process32Next(snap, &peProcessEntry))) return false;
	}
	lastProc = Proc;
	char sbuf[wdirtypemax];
	GetModuleFileNameEx(Proc, 0, sbuf, MAX_PATH);
	CloseHandle(Proc);
	WCHAR buf[wdirtypemax];
	Slesh(sbuf, buf);
	FindFirstFileT(buf, FindData);
	return true;
}


int __stdcall FsFindClose(HANDLE Hdl)
{
	FindClose(lastProc);
	snap = 0;
	return 0;
}


int __stdcall FsExecuteFileW(HWND MainWin,WCHAR* RemoteName,WCHAR* Verb)
{
    SHELLEXECUTEINFOW shex;
	if (wcslen(RemoteName)<pluginrootlen+2)
		return FS_EXEC_ERROR;

	if (_wcsicmp(Verb,L"open")==0) {
		return FS_EXEC_OK;

	} else if (_wcsicmp(Verb,L"properties")==0)
	{
		char buf[wdirtypemax];
		walcopy(buf, RemoteName + pluginrootlen, 100);
		DWORD PID = GetProcessByExeName(buf);
		showSV(PID, buf);
	}
	else
		return FS_EXEC_ERROR;
}


DWORD GetProcessByExeName(char *ExeName)
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, "Error = " + GetLastError(), "Error (GetProcessByExeName)", MB_OK | MB_ICONERROR);
		return 0;
	}

	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			if (strcmpi(pe32.szExeFile, ExeName) == 0)
			{
				CloseHandle(hProcessSnap);
				return pe32.th32ProcessID;
			}
		} while (Process32Next(hProcessSnap, &pe32));
	}
	CloseHandle(hProcessSnap);
	return 0;
}

BOOL __stdcall FsDeleteFileW(WCHAR* RemoteName)
{
	EnableDebugPrivilege(true);
	char buf[wdirtypemax];
	walcopy(buf, RemoteName + pluginrootlen, 150);
	DWORD PID = GetProcessByExeName(buf);
	if (!PID)  return false;
	else
	{
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, PID);
		if (TerminateProcess(hProcess, 0)) MessageBox(NULL, "Процесс удален!", "Информация", MB_OK | MB_ICONEXCLAMATION);
		else { MessageBox(NULL, "Не могу удалить этот процесс!", "Информация", MB_OK | MB_ICONEXCLAMATION); return false; }
	}
	return true;

}

void __stdcall FsGetDefRootName(char* DefRootName,int maxlen)
{
	strlcpy(DefRootName,"My Task Manager",maxlen);
}

int __stdcall FsExtractCustomIconW(WCHAR* RemoteName,int ExtractFlags,HICON* TheIcon)
{
	WCHAR* p;
	BOOL success,isdirectory;
	if (ExtractFlags & FS_ICONFLAG_SMALL)
		success=ExtractIconExT(RemoteName+pluginrootlen,0,NULL,TheIcon,1)==1;
	else
		success=ExtractIconExT(RemoteName+pluginrootlen,0,TheIcon,NULL,1)==1;
	if (success)
			return FS_ICON_EXTRACTED_DESTROY;  // must be destroyed with DestroyIcon!!!
			
	return FS_ICON_USEDEFAULT;
}


/**************************************************************************************/
/*********************** content plugin = custom columns part! ************************/
/**************************************************************************************/

#define fieldcount 3
char* fieldnames[fieldcount] = { "id","size", "creationdate" };

int fieldtypes[fieldcount] = { ft_numeric_64, ft_numeric_64, ft_datetime };

int fieldflags[fieldcount] = { contflags_substsize, contflags_substsize, contflags_substdatetime };

int sortorders[fieldcount]={-1,-1,-1};


int __stdcall FsContentGetSupportedField(int FieldIndex,char* FieldName,char* Units,int maxlen)
{
	if (FieldIndex<0 || FieldIndex>=fieldcount)
		return ft_nomorefields;
	strlcpy(FieldName,fieldnames[FieldIndex],maxlen-1);
	return fieldtypes[FieldIndex];
}

int __stdcall FsContentGetValueT(BOOL unicode, WCHAR* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	FILETIME ft[4];
	SYSTEMTIME st[4];
	if (wcslen(FileName+pluginrootlen)<=3)
	return ft_fileerror;

	PROCESS_MEMORY_COUNTERS pmc;
	char buf[wdirtypemax];
	walcopy(buf, FileName + pluginrootlen, 100);
	DWORD PID = GetProcessByExeName(buf);
	EnableDebugPrivilege(true);
	HANDLE Proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

	if (Proc != INVALID_HANDLE_VALUE)
	{
		GetProcessMemoryInfo(Proc, &pmc, sizeof(pmc));
		GetProcessTimes(Proc, &ft[0], &ft[1], &ft[2], &ft[3]);
		FileTimeToSystemTime(&ft[0], &st[0]);

		switch (FieldIndex) {
		case 0: 
			*(int*)FieldValue = PID;
			break;
		case 1:  	
			*(size_t*)FieldValue = pmc.WorkingSetSize/1024;
			break;
		case 2: 
			*(FILETIME*)FieldValue = ft[0];
			break;
		default:
		return ft_nosuchfield;
		}
	} 
	else
	return ft_fileerror;
	return fieldtypes[FieldIndex];

}

int __stdcall FsContentGetValueW(WCHAR* FileName,int FieldIndex,int UnitIndex,void* FieldValue,int maxlen,int flags)
{
	return FsContentGetValueT(true,FileName,FieldIndex,UnitIndex,FieldValue,maxlen,flags);
}

int __stdcall FsContentGetSupportedFieldFlags(int FieldIndex)
{
	if (FieldIndex==-1)
		return contflags_substmask | contflags_edit;
	else if (FieldIndex<0 || FieldIndex>=fieldcount)
		return 0;
	else
		return fieldflags[FieldIndex];
}

int __stdcall FsContentGetDefaultSortOrder(int FieldIndex)
{
	if (FieldIndex<0 || FieldIndex>=fieldcount)
		return 1;
	else 
		return sortorders[FieldIndex];
}

BOOL __stdcall FsContentGetDefaultView(char* ViewContents,char* ViewHeaders,char* ViewWidths,char* ViewOptions,int maxlen)
{
	strlcpy(ViewContents,"[=<fs>.id]\\n[=<fs>.size]\\n[=<fs>.creationdate]",maxlen);  // separated by backslash and n, not new lines!
	strlcpy(ViewHeaders,"ID\\nПамять(КБ)\\nВремя и дата",maxlen);  // titles in ENGLISH also separated by backslash and n, not new lines!
	strlcpy(ViewWidths,"110,25,35,40,50",maxlen);
	strlcpy(ViewOptions,"-1|0",maxlen);  // auto-adjust-width, or -1 for no adjust | horizonal scrollbar flag
	return true;
}


