#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../3d_accel.h"

#define DRIVER "vmwsmini.vxd"
#define CMD_SIZE 128

BOOL run_command(HANDLE vxd, const char *cmdline)
{
	if(strncmp(cmdline, "show", sizeof("show")-1) == 0)
	{
		DeviceIoControl(vxd, OP_MOUSE_SHOW,
			NULL, 0,
			NULL, 0,
			NULL, NULL);
	}
	else if(strncmp(cmdline, "hide", sizeof("hide")-1) == 0)
	{
		DeviceIoControl(vxd, OP_MOUSE_HIDE,
			NULL, 0,
			NULL, 0,
			NULL, NULL);
	}
	else if(strncmp(cmdline, "move", sizeof("move")-1) == 0)
	{
		DWORD pos[2] = {0, 0};
		char *ptr = (char*)cmdline + sizeof("move") - 1;
		
		pos[0] = strtoul(ptr, &ptr, 0);
		pos[1] = strtoul(ptr, NULL, 0);
		
		DeviceIoControl(vxd, OP_MOUSE_MOVE,
			&pos[0], sizeof(pos),
			NULL, 0,
			NULL, NULL);
	}
	else if(strncmp(cmdline, "exit", sizeof("exit")-1) == 0)
	{
		return FALSE;
	}
	else
	{
		printf("Unknown command: %s\n", cmdline);
	}
	
	return TRUE;
}

int main()
{
	char cmdbuf[CMD_SIZE];
	int cmdbuf_len = 0;
	
	HANDLE vxd = CreateFileA("\\\\.\\" DRIVER, 0, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, 0);
	if(vxd == INVALID_HANDLE_VALUE)
	{
		printf("cannot load VXD driver\n");
		return EXIT_FAILURE;
	}
	
	for(;;)
	{
		int c = getchar();
		switch(c)
		{
			case EOF:
				if(cmdbuf_len == 0)
				{
					goto dingo;
				}
				/* fall */
			case '\n':
				cmdbuf[cmdbuf_len] = '\0';
				cmdbuf_len = 0;
				if(!run_command(vxd, cmdbuf))
				{
					goto dingo;
				}
				break;
			case '\r':
				break;
			default:
				if(cmdbuf_len < CMD_SIZE - 1)
				{
					cmdbuf[cmdbuf_len] = c;
					cmdbuf_len++;
				}
				break;
		}
	}
	
	dingo:
	CloseHandle(vxd);
	
	return EXIT_SUCCESS;
}
