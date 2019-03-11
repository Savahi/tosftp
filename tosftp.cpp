#define LIBSSH_STATIC
#include <windows.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "kernel32.lib")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ftp.h"
#include "sftp.h"

#define CONNECTION_NAMES_BUFFER 2000
wchar_t _connectionNames[CONNECTION_NAMES_BUFFER + 1];

#define MAX_CONNECTIONS_NUMBER 100
wchar_t *_connections[MAX_CONNECTIONS_NUMBER];
int _connectionsNumber = 0;
int readConnections(wchar_t *fileName);

#define PROFILE_STRING_BUFFER 1000

wchar_t _server[PROFILE_STRING_BUFFER + 1];
wchar_t _directory[PROFILE_STRING_BUFFER + 2]; // +2 to append slash if required
wchar_t _user[PROFILE_STRING_BUFFER + 1];
wchar_t _password[PROFILE_STRING_BUFFER + 1];
wchar_t _mode[PROFILE_STRING_BUFFER + 1];
int _port=-1;

int readConnection(wchar_t *fileName, wchar_t *connectionName);

#define MAX_FILES_NUMBER 100
wchar_t *_fileNames[MAX_FILES_NUMBER];
int _filesNumber = 0;
int readFileNames(wchar_t *fileNamesBuffer);

wchar_t _results[MAX_FILES_NUMBER+1];

static void delete_spaces_from_string(wchar_t* str);
static bool is_empty_string(wchar_t* str, bool comma_is_empty_char);
static void delete_char_from_string(wchar_t* str, int pos);
static void substitute_char_in_string(wchar_t*str, wchar_t charToFind, wchar_t charToReplaceWith);

static void writeResultIntoIniFile(wchar_t *sectionName, const wchar_t *result = nullptr);

wchar_t **_argList;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* cmdLine, int nCmdShow)
{
	int exitStatus = -1;
	int status;

	int argCount;
	_argList = CommandLineToArgvW(GetCommandLineW(), &argCount);
	if (_argList == nullptr || argCount < 3 ) {
		MessageBoxW(NULL, L"Unable to parse command line. Use tosftp.exe actions.ini servers.ini", L"Error", MB_OK);
		goto lab_exit;
	}

	if (readConnections(_argList[1]) <= 0) { // Each action (transfer) section is entitled with a name of a connection
		MessageBoxW(NULL, L"Unable to read connections", L"Error", MB_OK);
		goto lab_exit;
	} 
	// The *_connections[] array is initialized now with connection names, the _connectionsNumber variable stores the number of connections read.

	for (int iconn = 0; iconn < _connectionsNumber; iconn++) { // Iterating through transfer sections...
		wchar_t action[PROFILE_STRING_BUFFER + 1];
		status = GetPrivateProfileStringW(_connections[iconn], L"Action", NULL, action, PROFILE_STRING_BUFFER, _argList[1]);
		if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
			writeResultIntoIniFile(_connections[iconn]);
			continue;
		}

		if (readConnection(_argList[2], _connections[iconn]) == -1) { // Reading details of the connection
			writeResultIntoIniFile(_connections[iconn]);
			continue;
		}

		int transferMode;
		if( wcscmp( _mode, L"FTP" ) == 0 ) {
			transferMode = 1;
		} else if ( (wcscmp(_mode, L"SSH") == 0) || (wcscmp(_mode, L"SFTP") == 0) ) {
			transferMode = 2;
		} else {
			writeResultIntoIniFile(_connections[iconn]);
			continue;
		}

		wchar_t filesDir[PROFILE_STRING_BUFFER + 2]; // A local directory to read file from / write files to
		status = GetPrivateProfileStringW(_connections[iconn], L"FilesDir", NULL, filesDir, PROFILE_STRING_BUFFER, _argList[1]);
		if ( status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
			writeResultIntoIniFile(_connections[iconn]);
			continue;
		}
		if (status <= 0) {
			filesDir[0] = '\x0';
		} else {
			int len = wcslen(filesDir);
			if( filesDir[len-1] != L'\\' ) {
				filesDir[len] = L'\\';
				filesDir[len + 1] = L'\x0';
			}
		}

		wchar_t fileNamesBuffer[PROFILE_STRING_BUFFER + 1]; // A buffer to read the list of files into
		status = GetPrivateProfileStringW(_connections[iconn], L"FileNames", NULL, fileNamesBuffer, PROFILE_STRING_BUFFER, _argList[1]);
		if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
			writeResultIntoIniFile(_connections[iconn]);			
			continue;
		}

		int actionCode;
		if( wcscmp(action, L"PUT") == 0 ) {
			actionCode = 2; // Upload
		} else if (wcscmp(action, L"GET") == 0) {
				actionCode = 1; // Download
		} else {
			continue;
		}

		char default_char = '_';
		char serverMultiByte[PROFILE_STRING_BUFFER + 1];
		WideCharToMultiByte(CP_ACP, 0, _server, -1, serverMultiByte, PROFILE_STRING_BUFFER, &default_char, NULL);
		char userMultiByte[PROFILE_STRING_BUFFER + 1];
		WideCharToMultiByte(CP_ACP, 0, _user, -1, userMultiByte, PROFILE_STRING_BUFFER, &default_char, NULL);
		char passwordMultiByte[PROFILE_STRING_BUFFER + 1];
		WideCharToMultiByte(CP_ACP, 0, _password, -1, passwordMultiByte, PROFILE_STRING_BUFFER, &default_char, NULL);
		char directoryMultiByte[PROFILE_STRING_BUFFER + 1];
		WideCharToMultiByte(CP_ACP, 0, _directory, -1, directoryMultiByte, PROFILE_STRING_BUFFER, &default_char, NULL);

		int initStatus=-1;		
		if( transferMode == 1 ) { // FTP
			int credentialsStatus = ftpSetCredentials(serverMultiByte, userMultiByte, passwordMultiByte, _port);
			if( credentialsStatus >= 0 ) {
				initStatus = ftpInit();
			}
		} else { // SSH
			int credentialsStatus = sftpSetCredentials(serverMultiByte, userMultiByte, passwordMultiByte);
			if( credentialsStatus >= 0 ) {
				initStatus = sftpInit();
			}
		}
		if( initStatus < 0 ) {
			writeResultIntoIniFile(_connections[iconn]);
			continue;
		}

		if (readFileNames(fileNamesBuffer) <= 0) {
			writeResultIntoIniFile(_connections[iconn]);
			continue;
		}

		for( int ifile = 0 ; ifile < _filesNumber ; ifile++ ) {
			_results[ifile] = L'-';
		}
		_results[_filesNumber] = L'\x0';
		
		for( int ifile = 0 ; ifile < _filesNumber ; ifile++ ) {
			if( actionCode == 2 ) { // Uploading...
				wchar_t srcPath[PROFILE_STRING_BUFFER * 2 + 1];
				wcscpy(srcPath, filesDir);
				wcscat(srcPath, _fileNames[ifile]);
				substitute_char_in_string(srcPath, '/', '\\');

				char srcPathMultiByte[PROFILE_STRING_BUFFER + 1];
				WideCharToMultiByte(CP_ACP, 0, srcPath, -1, srcPathMultiByte, PROFILE_STRING_BUFFER * 2, &default_char, NULL);

				substitute_char_in_string(_fileNames[ifile], '\\', '/');
				char fileNameMultiByte[PROFILE_STRING_BUFFER + 1];
				WideCharToMultiByte(CP_ACP, 0, _fileNames[ifile], -1, fileNameMultiByte, PROFILE_STRING_BUFFER, &default_char, NULL);

				MessageBoxA(NULL, srcPathMultiByte, "src Path Multi byte", MB_OK);
				MessageBoxA(NULL, directoryMultiByte, "Directory Multi byte", MB_OK);				
				
				if (transferMode == 1) {				// FTP
					status = ftpUpload(srcPathMultiByte, fileNameMultiByte, directoryMultiByte );
					DWORD error;					
					char errorText[1000];
					char errorMessage[1000];
					ftpGetLastError(NULL, &error, errorText);
					sprintf( errorMessage, "%s::::(%d)", errorText, error );
					MessageBoxA(NULL, errorMessage, "Error", MB_OK);				
				} else {								// SSH FTP
					status = sftpUpload(srcPathMultiByte, fileNameMultiByte, directoryMultiByte);
					DWORD error;					
					char errorText[1000];
					char errorMessage[1000];
					ftpGetLastError(NULL, &error, errorText);
					sprintf( errorMessage, "%s::::(%d)", errorText, error );
					MessageBoxA(NULL, errorMessage, "Error", MB_OK);				
				}
				_results[ifile] = (status == 0) ? L'+' : L'-';				
				} 
			else if ( actionCode == 1 ) { // Downloading...
				wchar_t destPath[PROFILE_STRING_BUFFER*2 + 1];
				wcscpy(destPath, filesDir);
				wcscat(destPath, _fileNames[ifile]);
				substitute_char_in_string(destPath, '/', '\\');

				char destPathMultiByte[PROFILE_STRING_BUFFER + 1];
				WideCharToMultiByte(CP_ACP, 0, destPath, -1, destPathMultiByte, PROFILE_STRING_BUFFER*2, &default_char, NULL);
				substitute_char_in_string(_fileNames[ifile], '\\', '/');
				char fileNameMultiByte[PROFILE_STRING_BUFFER + 1];
				WideCharToMultiByte(CP_ACP, 0, _fileNames[ifile], -1, fileNameMultiByte, PROFILE_STRING_BUFFER, &default_char, NULL);

				//strcpy(directoryMultiByte,"");				
				char b[2000];
				sprintf( b, "destpath=%s :: filename=%s :: directory=%s", destPathMultiByte, fileNameMultiByte, directoryMultiByte);
				MessageBoxA(NULL, b, "destPathMultiByte", MB_OK);				

				if (transferMode == 1) {				//	FTP
					status = ftpDownload(destPathMultiByte, fileNameMultiByte, directoryMultiByte);
					DWORD error;
					char errorText[1000];
					char errorMessage[1000];
					ftpGetLastError(NULL, &error, errorText);
					sprintf( errorMessage, "%s::::(%d/%d)", errorText, status, error );
					MessageBoxA(NULL, errorMessage, "Error", MB_OK);				
				} else {												// SSH FTP
					status = sftpDownload(destPathMultiByte, fileNameMultiByte, directoryMultiByte);
				}
				_results[ifile] = (status == 0) ? L'+' : L'-';
			}
		}
		MessageBoxW(NULL, _results, L"Error", MB_OK);								
		writeResultIntoIniFile(_connections[iconn], _results);
		
		if( transferMode == 1 ) { 			// FTP
			ftpClose();
		} else {							// SFTP
			sftpClose();
		}
	}

lab_exit:
	if (_argList != nullptr) {
		LocalFree(_argList);
	}

	return exitStatus;
}


int readConnections(wchar_t *fileName)
{
	DWORD charsRead = GetPrivateProfileSectionNamesW(_connectionNames, CONNECTION_NAMES_BUFFER, fileName);
	if (charsRead <= 0 || charsRead == CONNECTION_NAMES_BUFFER - 2) {
		return 0;
	}
	
	_connections[0] = &_connectionNames[0];
	_connectionsNumber = 1;
	for (unsigned int i = 0; i <= charsRead ; i++) {
		if (_connectionNames[i] == L'\x0' && _connectionNames[i + 1] != L'\x0') {
			_connections[_connectionsNumber] = &_connectionNames[i + 1];
			_connectionsNumber++;
			i++;
		}
	}
	return _connectionsNumber;
}


int readConnection(wchar_t *fileName, wchar_t *connectionName)
{
	DWORD status;

	status = GetPrivateProfileStringW(connectionName, L"Host", NULL, _server, PROFILE_STRING_BUFFER, fileName);			
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}

	int serverNameLength = wcslen(_server);
	int directoryFoundAt = -1; // A starting directory '/' symbol position - to separate directory from server address
	for (int i = 0; i < serverNameLength; i++) {
		if (_server[i] == L'/') { // A host name contains as well a directory...
			directoryFoundAt = i;
			break;
		}
	}
	if (directoryFoundAt > 0) { // A directory found...
		_server[directoryFoundAt] = L'\x0';
		_directory[0] = L'/';
		int directoryNameLength = 1;
		for (int i = directoryFoundAt + 1; i < serverNameLength; i++) {
			_directory[directoryNameLength] = _server[i];
			directoryNameLength++;
		}
		if (_directory[directoryNameLength - 1] != L'/') {
			_directory[directoryNameLength] = L'/';
			directoryNameLength++;
		}
		_directory[directoryNameLength] = L'\x0';
	}
	else {
		_directory[0] = L'\x0';
	}

	status = GetPrivateProfileStringW(connectionName, L"User", NULL, _user, PROFILE_STRING_BUFFER, fileName);
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}
	status = GetPrivateProfileStringW(connectionName, L"Password", NULL, _password, PROFILE_STRING_BUFFER, fileName);
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}
	status = GetPrivateProfileStringW(connectionName, L"Mode", NULL, _mode, PROFILE_STRING_BUFFER, fileName);
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}
	//_port = GetPrivateProfileIntW(connectionName, L"Port", -1, fileName);
	return 0;
}


int readFileNames( wchar_t *fileNamesBuffer ) {
	
	delete_spaces_from_string(fileNamesBuffer);
	if (is_empty_string(fileNamesBuffer, true)) {
		return 0;
	}

	int fileNamesBufferLength = wcslen(fileNamesBuffer);

	_fileNames[0] = fileNamesBuffer;
	_filesNumber = 1;

	for ( int ibuff = 0 ; ibuff < fileNamesBufferLength ; ibuff++) {
		if (fileNamesBuffer[ibuff] == L',') { // A separation comma found
			fileNamesBuffer[ibuff] = L'\x0';
			ibuff++;
			_fileNames[_filesNumber] = &fileNamesBuffer[ibuff];
			_filesNumber += 1;
		}
	}
	return _filesNumber;
}


static void substitute_char_in_string( wchar_t*str, wchar_t charToFind, wchar_t charToReplaceWith )
{
	for( unsigned int i = 0 ; i < wcslen(str) ; i++ ) {
		if( str[i] == charToFind ) {
			str[i] = charToReplaceWith;
		}
	}
}


static bool is_empty_string(wchar_t* str, bool comma_is_empty_char)
{
	for (unsigned int i = 0; i < wcslen(str); i++) {
		if (str[i] != L' ' && str[i] != L'\r' && str[i] != L'\n' && (str[i] != L',' && !comma_is_empty_char) ) {
			return true;
		}
	}
	return false;
}

static void delete_char_from_string(wchar_t* str, int pos)
{
	size_t len = wcslen(str);

	for (unsigned int i = pos + 1; i < len; i++) {
		str[i - 1] = str[i];
	}
	str[len - 1] = L'\x0';
}

static void delete_spaces_from_string(wchar_t* str)
{
	size_t len = wcslen(str);
	for (unsigned int i = 0; i < len; i++) { // Deleting from the beginning
		if( str[i] != L' ') {
			break;
		}
		delete_char_from_string(str, 0);
		len--;
	}

	for (int i = len - 1; i >= 0; i--) { // Deleting from the end
		if (str[i] != L' ') {
			break;
		}
		delete_char_from_string(str, i);
		len--;
	}

	for (unsigned int i = len-1 ; i > 0; i--) { // Deleting before ","
		if (str[i-1] == L' ' && str[i] == L',') {
			delete_char_from_string(str, i-1);
			len--;
		}
	}

	for (unsigned int i = 1; i < len ; ) { // Deleting after ","
		if (str[i - 1] == L',' && str[i] == L' ') {
			delete_char_from_string(str, i);
			len--;
		} else {
			i++;
		}
	}
}

static void writeResultIntoIniFile( wchar_t *sectionName, const wchar_t *result )
{
	const wchar_t *error = L"-";

	if( result == nullptr ) {
		result = error;
	}
	WritePrivateProfileStringW( sectionName, L"Result", result, _argList[1] );
}
