#pragma comment(lib, "ssh.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include "sftp.h"

#define TRANSFER_BUFFER_SIZE 16384

#define SFTP_MAX_SERVER 100
static char _server[SFTP_MAX_SERVER + 1];

#define SFTP_MAX_USER 100
static char _user[SFTP_MAX_USER + 1];

#define SFTP_MAX_PASSWORD 100
static char _password[SFTP_MAX_PASSWORD + 1];

#define SFTP_MAX_REMOTE_ADDR 500
static char _remoteAddr[SFTP_MAX_REMOTE_ADDR + 1];

static int _port = -1;

static ssh_session _sshSession=NULL;
static sftp_session _sftpSession=NULL;

static long int _sftpErrorCode = 0;
static int _sshErrorCode = SSH_NO_ERROR;
static char *_sshErrorText = NULL;

static long int _timeOut = -1L;

static int validateDirectories(char *remoteAddr);
static bool dstDirIsValidated=false;

static bool findCharsInString(char* str, char* chars);

static int createRemoteAddr(char *fileName, char *directory,
	char *server, char *user, char *password)
{
	if( server != NULL && user != NULL && password != NULL ) {
		if (strlen(server) + strlen(user) + strlen(password) + strlen(directory) + strlen(fileName) + 7 >= SFTP_MAX_REMOTE_ADDR) {
			return -1;
		}
		strcpy(_remoteAddr, "sftp://");
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
		if (directoryLength + fileNameLength + 1 >= SFTP_MAX_REMOTE_ADDR) {
			return -1;
		}
		strcpy(_remoteAddr, directory);
		if( _remoteAddr[ directoryLength-1 ] != '/' ) {
			strcat(_remoteAddr, "/");
		}
		strcat(_remoteAddr, fileName);			
	}
	return 0;
}


static long int getRemoteFileSize(char *remoteFile)
{
	return 0;
}


int sftpTest(char *fileName, char *directory, unsigned long int *size) 
{
	_sftpErrorCode = 0;
	_sshErrorCode = SSH_NO_ERROR;

	if (createRemoteAddr(fileName, directory, NULL, NULL, NULL) == -1) {
		return -1;
	}

	return _sftpErrorCode;
}


int sftpDelete(char *dstFileName, char *dstDirectory) 
{
	_sftpErrorCode = 0;
	_sshErrorCode = SSH_NO_ERROR;

	return _sftpErrorCode;
}


int sftpUpload( char *srcFileName, char *dstFileName, char *dstDirectory, bool createDstDirIfNotExists ) 
{
	sftp_file dstFile;
	char buffer[TRANSFER_BUFFER_SIZE];
	int srcFile;
	int status, bytesRead, bytesWritten;

	_sftpErrorCode = 0;
	_sshErrorCode = SSH_NO_ERROR;

	if (createRemoteAddr(dstFileName, dstDirectory, NULL, NULL, NULL) == -1) {
		_sftpErrorCode = -1;
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
		if (!directoryValidated) {
			_sftpErrorCode = -1;
		}
		else {
			dstFile = sftp_open(_sftpSession, _remoteAddr, O_WRONLY | O_CREAT | O_TRUNC, 0000700);
			if (dstFile == NULL) {
				_sftpErrorCode = SFTP_ERROR_FAILED_TO_WRITE_REMOTE;
			}
			else {
				srcFile = open(srcFileName, O_RDONLY);
				if (srcFile < 0) {
					_sftpErrorCode = SFTP_ERROR_FAILED_TO_READ_LOCAL;
				}
				else {
					for (;;) {
						bytesRead = read(srcFile, buffer, sizeof(buffer));
						if (bytesRead < 0) {
							_sftpErrorCode = SFTP_ERROR_FAILED_TO_READ_LOCAL;
							break; // Error
						}
						if (bytesRead == 0) {
							break; // EOF
						}
						bytesWritten = sftp_write(dstFile, buffer, bytesRead);
						if (bytesWritten != bytesRead) {
							_sftpErrorCode = SFTP_ERROR_FAILED_TO_WRITE_REMOTE;
							break;
						}
					}
					close(srcFile);
				}
				status = sftp_close(dstFile);
				if (status != SSH_OK) {
					_sftpErrorCode = SFTP_ERROR_FAILED_TO_WRITE_REMOTE;
				}
			}
		}
  	}
	return _sftpErrorCode;
}


int sftpDownload(char *dstFileName, char *srcFileName, char *srcDirectory ) 
{
	_sftpErrorCode = 0;
	_sshErrorCode = SSH_NO_ERROR;
	char buffer[TRANSFER_BUFFER_SIZE];
	sftp_file srcFile;
	int dstFile;
	int status, bytesRead, bytesWritten;

	if (createRemoteAddr(srcFileName, srcDirectory, NULL, NULL, NULL) == -1) {
		_sftpErrorCode = -1;
	} else {
		srcFile = sftp_open(_sftpSession, _remoteAddr, O_RDONLY, 0);
		if (srcFile == NULL) {
			_sftpErrorCode = SFTP_ERROR_FAILED_TO_READ_REMOTE;		
		} else {
			dstFile = open(dstFileName, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);
			if (dstFile < 0) {
				_sftpErrorCode = SFTP_ERROR_FAILED_TO_WRITE_LOCAL;		
			} else {
				for (;;) {
					bytesRead = sftp_read(srcFile, buffer, sizeof(buffer));
					if (bytesRead < 0) {
						_sftpErrorCode = SFTP_ERROR_FAILED_TO_READ_REMOTE;		
						break; // Error
					}
					if (bytesRead == 0) {
						break; // EOF
					} 
					bytesWritten = write(dstFile, buffer, bytesRead);
					if (bytesWritten != bytesRead) {
						_sftpErrorCode = SFTP_ERROR_FAILED_TO_WRITE_LOCAL;		
						break;
					}
				}
				close(dstFile);
			}
			status = sftp_close(srcFile);
			if (status != SSH_OK) {
				_sftpErrorCode = SFTP_ERROR_FAILED_TO_READ_REMOTE;		
			}
		}
	}
	return _sftpErrorCode;
}


void sftpSetTimeOut(unsigned long int timeOut) {
	_sftpErrorCode = 0;
	_timeOut = timeOut;
}


int sftpSetCredentials(char *server, char *user, char *password, int port) {
	int status;
	_sftpErrorCode = 0;
	_sshErrorCode = SSH_NO_ERROR;

	if( strlen(server) > SFTP_MAX_SERVER || strlen(user) > SFTP_MAX_USER || strlen(password) > SFTP_MAX_PASSWORD ) {
		_sftpErrorCode = -1;
	} else {
		strcpy( _server, server );
		strcpy( _user, user );
		strcpy( _password, password );
		if( port < 0 ) {
			_port = 22;
		} else {
			_port = port;
		}
	}
	return _sftpErrorCode;
}


int sftpInit(void) {
	int status;
	_sftpErrorCode = 0;
	_sshErrorCode = SSH_NO_ERROR;

	dstDirIsValidated = false; 

	_sshSession = ssh_new();
	if (_sshSession == NULL) {
		_sftpErrorCode = SFTP_ERROR_FAILED_TO_CREATE_SSH_SESSION;
	} else {
		ssh_options_set(_sshSession, SSH_OPTIONS_HOST, _server);
		if( !( _port < 0 ) ) {
			ssh_options_set(_sshSession, SSH_OPTIONS_PORT, &_port); 			
		}
		// Connect to server 
		status = ssh_connect(_sshSession);
	 	if (status != SSH_OK) { 
			_sftpErrorCode = SFTP_ERROR_FAILED_TO_CONNECT;
		} else {
			ssh_options_set(_sshSession, SSH_OPTIONS_USER, _user);
			status = ssh_userauth_password(_sshSession, NULL, _password);
			//status = ssh_userauth_password(_sshSession, _user, _password);
			if (status != SSH_AUTH_SUCCESS) {
				_sftpErrorCode = SFTP_ERROR_FAILED_TO_AUTHORIZE;
			} else { // INITIALIZING SFTP SESSION...
				_sftpSession = sftp_new(_sshSession);
  				if (_sftpSession == NULL) { 
  					_sftpErrorCode = SFTP_ERROR_FAILED_TO_CREATE_SFTP_SESSION;
  				} else {
					status = sftp_init(_sftpSession);
					if( status != SSH_OK ) {
	  					_sftpErrorCode = SFTP_ERROR_FAILED_TO_CREATE_SFTP_SESSION;
					}
				}
			}
		}
	}

	if( _sftpErrorCode == -1 ) {
		sftpClose(false); // An error arised, that's why the line is commented.
	} 
	return _sftpErrorCode;
}


void sftpClose(bool resetErrors) {
	if( resetErrors ) {
		_sftpErrorCode = 0;
		_sshErrorCode = SSH_NO_ERROR;
	}

	if( _sftpSession != NULL ) {
		sftp_free(_sftpSession);
	}    
	_sftpSession = NULL;

	if( _sshSession != NULL ) {
	    ssh_disconnect(_sshSession);
    	ssh_free(_sshSession);
	}
	_sshSession = NULL;
}


int sftpGetLastError(int *sftpErrorCode, int *sshErrorCode, char *sshErrorText) {
	if (sftpErrorCode != NULL) {
		*sftpErrorCode = _sftpErrorCode;
	}
	if (sshErrorCode != NULL) {
		*sshErrorCode = ssh_get_error_code(_sshSession);
	}
	if (sshErrorText != NULL) {
		sshErrorText = (char *)ssh_get_error(_sshSession);
	}
	return 0;
}

static int validateDirectories(char *remoteAddr) {
	int returnValue = 0;
	char remoteDir[SFTP_MAX_REMOTE_ADDR + 1];

	int remoteAddrLen = strlen(remoteAddr);

	for (int i = 1; i < remoteAddrLen; i++) {
		if ((remoteAddr[i] == '\\' || remoteAddr[i] == '/') && (remoteAddr[i - 1] != '\\' && remoteAddr[i - 1] != '/')) {
			strncpy(remoteDir, remoteAddr, i);
			remoteDir[i] = '\x0';

			sftp_attributes attr = sftp_stat( _sftpSession, remoteDir );
			if (attr == NULL) {
				// MessageBoxA( NULL, remoteDir, "HERE", MB_OK );
				int status = sftp_mkdir(_sftpSession, remoteDir, 0000700);
				if (status != SSH_OK) {
					returnValue = -1;
					break;
				}
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