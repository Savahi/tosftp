// A demo for sftp file transfer
#include <windows.h>
#pragma comment(lib, "user32.lib")
#include <winuser.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ftp.h"
#include "sftp.h"

static char *_server = "u38989.ssh.masterhost.ru";
static char *_user = "u38989";
static char *_password = "amitin9sti";
static char *_dstDirectory = "/home/u38989";

static char *_ftpServer = "u38989.ftp.masterhost.ru";
static char *_ftpUser = "u38989_2";
static char *_ftpPassword = "al6u7ssion";
static char *_ftpDstDirectory = "";


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* cmdLine, int nCmdShow) {
//int main(void) {

	int status;
	long unsigned int size;
	char buffer[2000];

	strcpy(buffer, "Starting SFTP test...\n");

	if (sftpSetCredentials(_server, _user, _password) == -1) {
		goto lab_close;
	}

	if( sftpInit() == -1 ) { // A must...
		goto lab_close;
	}

	// Uploading...
	strcat(buffer, "Uploading...\n");
	status = sftpUpload("file1.txt", "file1.txt", _dstDirectory);
	if (status == 0) {
		strcat(buffer, "Uploaded Ok\n");
	} else {
		int error;
		char errorBuffer[1000];
		sftpGetLastError(NULL, &error, NULL);
		sprintf(errorBuffer, "Error uploading file (%d, %d)!\n", status, error);
		strcat(buffer, errorBuffer);
	}

	// Downloading
	strcat(buffer, "Downloading...\n");
	status = sftpDownload("file1-downloaded.txt", "file1.txt", _dstDirectory);
	if (status == 0) {
		strcat(buffer, "Downloaded Ok\n");
	} else {
		int error;
		char errorBuffer[1000];
		sftpGetLastError(NULL, &error, NULL);
		sprintf(errorBuffer, "Error downloading file (%d, %d)!\n", status, error);
		strcat(buffer, errorBuffer);
	}

	strcat(buffer, "Starting FTP test...\n");

	if (ftpSetCredentials(_ftpServer, _ftpUser, _ftpPassword, 21) == -1) {
		goto lab_close;
	}

	if (ftpInit() == -1) { // A must...
		goto lab_close;
	}

	// Uploading...
	strcat(buffer, "Uploading...\n");
	status = ftpUpload("file2.txt", "file2.txt", _ftpDstDirectory);
	if (status == 0) {
		strcat(buffer, "Uploaded Ok\n");
	}
	else {
		DWORD error;
		char errorBuffer[1000];
		ftpGetLastError(NULL, &error, NULL);
		sprintf(errorBuffer, "Error uploading file (%d, %d)!\n", status, (unsigned int)error);
		strcat(buffer, errorBuffer);

		char b[2001];
		DWORD blen=2000;
		InternetGetLastResponseInfoA(&error, b, &blen);
		MessageBox(NULL, b, "TEST LOG", MB_OK);

	}

	// Downloading
	strcat(buffer, "Downloading...\n");
	status = ftpDownload("file2-downloaded.txt", "file2.txt", _ftpDstDirectory);
	if (status == 0) {
		strcat(buffer, "Downloaded Ok\n");
	}
	else {
		DWORD error;
		char errorBuffer[1000];
		ftpGetLastError(NULL, &error, NULL);
		sprintf(errorBuffer, "Error downloading file (%d, %d)!\n", status, (unsigned int)error);
		strcat(buffer, errorBuffer);
	}


lab_close:
	sftpClose();
	ftpClose();

	MessageBox(NULL, buffer, "TEST LOG", MB_OK);

lab_exit:
	exit(0);
}
