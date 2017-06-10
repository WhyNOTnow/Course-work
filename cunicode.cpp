#include "stdafx.h"
#include "fsplugin.h"
#include "cunicode.h"

extern tProgressProc ProgressProc;
extern tLogProc LogProc;
extern tRequestProc RequestProc;
extern tProgressProcW ProgressProcW;
extern tLogProcW LogProcW;
extern tRequestProcW RequestProcW;

char usysychecked=0;

WCHAR* Slesh(CHAR* str,WCHAR* res)
{
	std::string str2 = str;
	for (std::string::size_type n = str2.find('\\'); n < 2 ; str2.find('\\', n))
	{
		str2.insert(n, "\\");
		while (str2.at(n) == '\\')
			n++;
	}
	char *cstr = new char[str2.length() + 1];
	strcpy(cstr, str2.c_str());
	awlcopy(res, cstr, str2.length());
	return res;
}

char* walcopy(char* outname,WCHAR* inname,int maxlen)
{
	if (inname) {
		WideCharToMultiByte(CP_ACP,0,inname,-1,outname,maxlen,NULL,NULL);
		outname[maxlen]=0;
		return outname;
	} else
		return NULL;
}

WCHAR* awlcopy(WCHAR* outname,char* inname,int maxlen)
{
	if (inname) {
		MultiByteToWideChar(CP_ACP,0,inname,-1,outname,maxlen);
		outname[maxlen]=0;
		return outname;
	} else
		return NULL;
}

WCHAR* wcslcpy(WCHAR *str1,const WCHAR *str2,int imaxlen)
{
	if ((int)wcslen(str2)>=imaxlen-1) {
		wcsncpy(str1,str2,imaxlen-1);
		str1[imaxlen-1]=0;
	} else
		wcscpy(str1,str2);
	return str1;
}

WCHAR* wcslcat(wchar_t *str1,const WCHAR *str2,int imaxlen)
{
	int l1=(int)wcslen(str1);
	if ((int)wcslen(str2)+l1>=imaxlen-1) {
		wcsncpy(str1+l1,str2,imaxlen-1-l1);
		str1[imaxlen-1]=0;
	} else
		wcscat(str1,str2);
	return str1;
}

// return true if name wasn't cut
BOOL MakeExtraLongNameW(WCHAR* outbuf,const WCHAR* inbuf,int maxlen)
{
	if (wcslen(inbuf)>259) {
		if (inbuf[0]=='\\' && inbuf[1]=='\\') {   // UNC-Path! Use \\?\UNC\server\share\subdir\name.ext
			wcslcpy(outbuf,L"\\\\?\\UNC",maxlen);
			wcslcat(outbuf,inbuf+1,maxlen);
		} else {
			wcslcpy(outbuf,L"\\\\?\\",maxlen);
			wcslcat(outbuf,inbuf,maxlen);
		}
	} else
		wcslcpy(outbuf,inbuf,maxlen);
	return (int)wcslen(inbuf)+4<=maxlen;
}

/***********************************************************************************************/

void copyfinddatawa(WIN32_FIND_DATA *lpFindFileDataA,WIN32_FIND_DATAW *lpFindFileDataW)
{
	walcopy(lpFindFileDataA->cAlternateFileName,lpFindFileDataW->cAlternateFileName,sizeof(lpFindFileDataW->cAlternateFileName)-1);
	walcopy(lpFindFileDataA->cFileName,lpFindFileDataW->cFileName,sizeof(lpFindFileDataW->cFileName)-1);
	lpFindFileDataA->dwFileAttributes=lpFindFileDataW->dwFileAttributes;
	lpFindFileDataA->dwReserved0=lpFindFileDataW->dwReserved0;
	lpFindFileDataA->dwReserved1=lpFindFileDataW->dwReserved1;
	lpFindFileDataA->ftCreationTime=lpFindFileDataW->ftCreationTime;
	lpFindFileDataA->ftLastAccessTime=lpFindFileDataW->ftLastAccessTime;
	lpFindFileDataA->ftLastWriteTime=lpFindFileDataW->ftLastWriteTime;
	lpFindFileDataA->nFileSizeHigh=lpFindFileDataW->nFileSizeHigh;
	lpFindFileDataA->nFileSizeLow=lpFindFileDataW->nFileSizeLow;
}

void copyfinddataaw(WIN32_FIND_DATAW *lpFindFileDataW,WIN32_FIND_DATA *lpFindFileDataA)
{
	awlcopy(lpFindFileDataW->cAlternateFileName,lpFindFileDataA->cAlternateFileName,countof(lpFindFileDataW->cAlternateFileName)-1);
	awlcopy(lpFindFileDataW->cFileName,lpFindFileDataA->cFileName,countof(lpFindFileDataW->cFileName)-1);
	lpFindFileDataW->dwFileAttributes=lpFindFileDataA->dwFileAttributes;
	lpFindFileDataW->dwReserved0=lpFindFileDataA->dwReserved0;
	lpFindFileDataW->dwReserved1=lpFindFileDataA->dwReserved1;
	lpFindFileDataW->ftCreationTime=lpFindFileDataA->ftCreationTime;
	lpFindFileDataW->ftLastAccessTime=lpFindFileDataA->ftLastAccessTime;
	lpFindFileDataW->ftLastWriteTime=lpFindFileDataA->ftLastWriteTime;
	lpFindFileDataW->nFileSizeHigh=lpFindFileDataA->nFileSizeHigh;
	lpFindFileDataW->nFileSizeLow=lpFindFileDataA->nFileSizeLow;
}

/***********************************************************************************************/

int ProgressProcT(int PluginNr,WCHAR* SourceName,WCHAR* TargetName,int PercentDone)
{
	if (ProgressProcW) {
		return ProgressProcW(PluginNr,SourceName,TargetName,PercentDone);
	} else if (ProgressProc) {
		char buf1[MAX_PATH],buf2[MAX_PATH];
		return ProgressProc(PluginNr,wafilenamecopy(buf1,SourceName),wafilenamecopy(buf2,TargetName),PercentDone);
	} else
		return 0;
}

void LogProcT(int PluginNr,int MsgType,WCHAR* LogString)
{
	if (LogProcW) {
		LogProcW(PluginNr,MsgType,LogString);
	} else if (LogProc) {
		char buf[1024];
		LogProc(PluginNr,MsgType,walcopy(buf,LogString,sizeof(buf)-1));
	}
}

BOOL RequestProcT(int PluginNr,int RequestType,WCHAR* CustomTitle,
              WCHAR* CustomText,WCHAR* ReturnedText,int maxlen)
{
	if (RequestProcW) {
		return RequestProcW(PluginNr,RequestType,CustomTitle,
          CustomText,ReturnedText,maxlen);
	} else if (RequestProc) {
		char buf1[MAX_PATH],buf2[MAX_PATH],buf3[MAX_PATH];
		char* preturn=wafilenamecopy(buf3,ReturnedText);
		BOOL retval=RequestProc(PluginNr,RequestType,wafilenamecopy(buf1,CustomTitle),
          wafilenamecopy(buf2,CustomText),preturn,maxlen);
		if (retval && preturn)
			awlcopy(ReturnedText,preturn,maxlen);
		return retval;
	} else
		return false;
}

BOOL SetFileAttributesT(WCHAR* lpFileName,DWORD dwFileAttributes)
{
	char buf[MAX_PATH];
	return SetFileAttributes(wafilenamecopy(buf,lpFileName),dwFileAttributes);
}

UINT ExtractIconExT(WCHAR* lpszFile,int nIconIndex,HICON *phiconLarge,HICON *phiconSmall,UINT nIcons)
{
	char buf[MAX_PATH];
	return ExtractIconEx(wafilenamecopy(buf,lpszFile),nIconIndex,phiconLarge,phiconSmall,nIcons);
}

HANDLE FindFirstFileT(WCHAR* lpFileName,LPWIN32_FIND_DATAW lpFindFileData)
{
		char buf[MAX_PATH];
		WIN32_FIND_DATA FindFileDataA;
		HANDLE retval=FindFirstFile(wafilenamecopy(buf,lpFileName),&FindFileDataA);
		if (retval!=INVALID_HANDLE_VALUE) {
			awlcopy(lpFindFileData->cAlternateFileName,FindFileDataA.cAlternateFileName,countof(lpFindFileData->cAlternateFileName)-1);
			awlcopy(lpFindFileData->cFileName,FindFileDataA.cFileName,countof(lpFindFileData->cFileName)-1);
			lpFindFileData->dwFileAttributes=FindFileDataA.dwFileAttributes;
			lpFindFileData->dwReserved0=FindFileDataA.dwReserved0;
			lpFindFileData->dwReserved1=FindFileDataA.dwReserved1;
			lpFindFileData->ftCreationTime=FindFileDataA.ftCreationTime;
			lpFindFileData->ftLastAccessTime=FindFileDataA.ftLastAccessTime;
			lpFindFileData->ftLastWriteTime=FindFileDataA.ftLastWriteTime;
			lpFindFileData->nFileSizeHigh=FindFileDataA.nFileSizeHigh;
			lpFindFileData->nFileSizeLow=FindFileDataA.nFileSizeLow;
		}
		return retval;
}

BOOL FindNextFileT(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
		WIN32_FIND_DATA FindFileDataA;
		memset(&FindFileDataA, 0, sizeof(FindFileDataA));
		BOOL retval = FindNextFile(hFindFile, &FindFileDataA);
		if (retval) {
			awlcopy(lpFindFileData->cAlternateFileName, FindFileDataA.cAlternateFileName, countof(lpFindFileData->cAlternateFileName) - 1);
			awlcopy(lpFindFileData->cFileName, FindFileDataA.cFileName, countof(lpFindFileData->cFileName) - 1);
			lpFindFileData->dwFileAttributes = FindFileDataA.dwFileAttributes;
			lpFindFileData->dwReserved0 = FindFileDataA.dwReserved0;
			lpFindFileData->dwReserved1 = FindFileDataA.dwReserved1;
			lpFindFileData->ftCreationTime = FindFileDataA.ftCreationTime;
			lpFindFileData->ftLastAccessTime = FindFileDataA.ftLastAccessTime;
			lpFindFileData->ftLastWriteTime = FindFileDataA.ftLastWriteTime;
			lpFindFileData->nFileSizeHigh = FindFileDataA.nFileSizeHigh;
			lpFindFileData->nFileSizeLow = FindFileDataA.nFileSizeLow;
		}
		return retval;
	
}

