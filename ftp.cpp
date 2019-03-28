#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "Wininet")
#include "ftp.h"

#define FTP_MAX_SERVER 100
static char _server[FTP_MAX_SERVER + 1];

#define FTP_MAX_USER 100
static char _user[FTP_MAX_USER + 1];

#define FTP_MAX_PASSWORD 100
static char _password[FTP_MAX_PASSWORD + 1];

#define FTP_MAX_REMOTE_ADDR 500
static char _remoteAddr[FTP_MAX_REMOTE_ADDR + 1];

static unsigned long _port;

static long int _ftpErrorCode = 0;
static DWORD _winInetErrorCode = 0;
static char *_winInetErrorText = NULL;

static long int _timeOut = -1L;

static HINTERNET _hInternet = NULL;
static HINTERNET _hFtpSession = NULL;

static int validateDirectories(char *);
static bool dstDirIsValidated=false;

static bool findCharsInString(char* str, char* chars);

static int createRemoteAddr(char *fileName, char *directory, char *server, char *user, char *password)
{
	if( server != NULL && user != NULL && password != NULL ) {	
		if (strlen(server) + strlen(user) + strlen(password) + strlen(directory) + strlen(fileName) + 7 >= FTP_MAX_REMOTE_ADDR) {
			return -1;
		}
		strcpy(_remoteAddr, "ftp://");
		strcat(_remoteAddr, user);
		strcat(_remoteAddr, ":");
		strcat(_remoteAddr, password);
		strcat(_remoteAddr, "@");
		strcat(_remoteAddr, server);
		strcat(_remoteAddr, directory);
		strcat(_remoteAddr, "/");
		strcat(_remoteAddr, fileName);
	} else {
		int directoryLength = strlen(directory);
		int fileNameLength = strlen(fileName);
		if (directoryLength + fileNameLength + 1 >= FTP_MAX_REMOTE_ADDR) {
			return -1;
		}
		strcpy(_remoteAddr, directory);
		if (directoryLength > 0) {
			if (_remoteAddr[directoryLength - 1] != '/') {
				strcat(_remoteAddr, "/");
			}
		}
		strcat(_remoteAddr, fileName);					
	}
	return 0;
}



int ftpTest(char *fileName, char *directory, unsigned long int *size)
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	return 1;
}


int ftpDelete(char *dstFileName, char *dstDirectory ) 
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if (createRemoteAddr(dstFileName, dstDirectory, NULL, NULL, NULL) == -1) {
		_ftpErrorCode = -1;
	} else {
		int lastCharIndex = strlen(_remoteAddr) - 1;
		if( _remoteAddr[lastCharIndex] != '*' ) {
			//MessageBoxA( NULL, _remoteAddr, "FILE TO DELETE", MB_OK );
			//if( !FtpDeleteFileA(_hFtpSession, _remoteAddr) ) {
			//	_ftpErrorCode = -1;				
			//}
		} else {
			WIN32_FIND_DATA fd;
			HINTERNET hFtpSession = InternetConnectA(_hInternet, _server, 
				INTERNET_DEFAULT_FTP_PORT, _user, _password, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
			if (hFtpSession) {
				HINTERNET hFind = FtpFindFirstFileA(_hFtpSession,_remoteAddr,&fd,0,0);
				if( hFind ) {
					bool findNext;
					do {
						if( fd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY ) {
							char fileToDelete[FTP_MAX_REMOTE_ADDR + 1 + MAX_PATH];
							strcpy(fileToDelete, _remoteAddr);
							fileToDelete[lastCharIndex] = '\x0';
							strcat( fileToDelete, fd.cFileName );
							//MessageBoxA( NULL, fileToDelete, "FILE TO DELETE", MB_OK );
							//if( !FtpDeleteFileA(_hFtpSession, fileToDelete) ) {
							//	_ftpErrorCode = -1;
							//}
						}
						findNext = InternetFindNextFileA(hFind, &fd);
					} while( findNext );
					InternetCloseHandle(hFind);
				}
				InternetCloseHandle(hFtpSession);
			} else {
				_ftpErrorCode = -1;
			}
		}
	}
	return _ftpErrorCode;
}


int ftpUpload(char *srcFileName, char *dstFileName, char *dstDirectory, bool createDstDirIfNotExists ) 
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if (createRemoteAddr(dstFileName, dstDirectory, NULL, NULL, NULL) == -1) {
		_ftpErrorCode = -1;
	} else {
		int directoryValidated = false;
		if (!createDstDirIfNotExists && !findCharsInString(dstFileName, "/\\")) {
			directoryValidated = true;
		}
		else if( dstDirIsValidated && !findCharsInString(dstFileName, "/\\") ) {
			directoryValidated = true;
		}
		else {
			if (validateDirectories(_remoteAddr) >= 0) {
				directoryValidated = true;
			}
		}
		if (directoryValidated) {
			DWORD status = FtpPutFileA(_hFtpSession, srcFileName, _remoteAddr, FTP_TRANSFER_TYPE_BINARY, 0);
			if (!status) {
				_ftpErrorCode = -1;
			}
		}
	}
	return _ftpErrorCode;
}


int ftpDownload(char *dstFileName, char *srcFileName, char *srcDirectory ) 
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if (createRemoteAddr(srcFileName, srcDirectory, NULL, NULL, NULL) == -1) {
		_ftpErrorCode = -1;
	} else {
		int status = FtpGetFileA(_hFtpSession, _remoteAddr, dstFileName, false, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0); 
		if( !status ) {
			_ftpErrorCode = -1;
		}
	}
	return _ftpErrorCode;
}


void ftpSetTimeOut(unsigned long int timeOut) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	_timeOut = timeOut;
}


int ftpSetCredentials(char *server, char *user, char *password, int port) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if( strlen(server) > FTP_MAX_SERVER || strlen(user) > FTP_MAX_USER || strlen(password) > FTP_MAX_PASSWORD ) {
		_ftpErrorCode = -1;
	} else {
		strcpy( _server, server );
		strcpy( _user, user );
		strcpy( _password, password );
		_port = port;		
		if( _port < 0 ) {
			_port = INTERNET_DEFAULT_FTP_PORT;
		}
	}
	return _ftpErrorCode;
}


int ftpInit(void) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	dstDirIsValidated = false; 

	_hInternet = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!_hInternet) {
		_ftpErrorCode = -1;
	} else {
		_hFtpSession = InternetConnectA(_hInternet, _server, INTERNET_DEFAULT_FTP_PORT, _user, _password, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
		if (!_hFtpSession) {
			_ftpErrorCode = -1;
		} 
	}

	if( _ftpErrorCode != 0 ) {
		ftpClose();
	}

	return _ftpErrorCode;
}


void ftpClose(void) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if( _hFtpSession != NULL ) {
	    InternetCloseHandle(_hFtpSession);
	}
	_hFtpSession = NULL;
	if( _hInternet != NULL ) {
	    InternetCloseHandle(_hInternet);
	}
	_hInternet = NULL;
}


int ftpGetLastError(int *ftpErrorCode, DWORD *winInetErrorCode, char *winInetErrorText) {
	if (ftpErrorCode != NULL) {
		*ftpErrorCode = _ftpErrorCode;
	}
	if (winInetErrorCode != NULL) {
		*winInetErrorCode = GetLastError();
	}
	if (winInetErrorText != NULL) {
		winInetErrorText = NULL;
	}
	return 0;
}


static int validateDirectories( char *remoteAddr ) {
	int returnValue = 0;
	char remoteDir[FTP_MAX_REMOTE_ADDR + 1];
	WIN32_FIND_DATAA findFileData;

	int remoteAddrLen = strlen(remoteAddr);

	for (int i = 1; i < remoteAddrLen; i++) {
		if ( (remoteAddr[i] == '\\' || remoteAddr[i] == '/') && (remoteAddr[i-1] != '\\' && remoteAddr[i-1] != '/') ) {
			strncpy(remoteDir, remoteAddr, i);
			remoteDir[i] = '\x0';

			HINTERNET hFtpSession = InternetConnectA(_hInternet, _server, 
				INTERNET_DEFAULT_FTP_PORT, _user, _password, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
			if (hFtpSession) {
				bool status = true;
				if (FtpFindFirstFileA(hFtpSession, remoteDir, &findFileData, 0, NULL) == NULL) { // The directory wasn't found...
					status = FtpCreateDirectoryA(hFtpSession, remoteDir);
				}
				InternetCloseHandle(hFtpSession);
				if (!status) {
					returnValue = -1;
					break;
				}
			}
			else {
				returnValue = -1;
				break;
			}
		}
	}

	if( returnValue == 0 ) {
		dstDirIsValidated = true;
	}
	return returnValue;
}


static bool findCharsInString(char* str, char* chars)
{
	int strLen = strlen(str);
	int charsLen = strlen(chars);
	for (unsigned int s = 0; s < strLen; s++) {
		for (unsigned int c = 0; c < charsLen; c++) {
			if (str[s] == chars[c]) {
				return true;
			}
		}
	}
	return false;
}