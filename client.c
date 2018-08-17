#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ftp-utils.h"


#define DEVICE "enp8s0"

int ConexaoRawSocket(char *device);





int main(int argc, char **argv)
{
	int exit = 0;
	unsigned char command[35];
	char cur_dir[100];
	int socket = ConexaoRawSocket (DEVICE);
	printf("Executando: client\n");

	printf("=======================\n");
	printf("COMANDOS:\n");
	printf("ls\n");
	printf("rls\n");
	printf("cd directory\n");
	printf("rcd directory\n");
	printf("put file_name\n");
	printf("get file_name\n");
	printf("exit\n");
	printf("=======================\n\n");

	
	while(!exit)
	{
		getcwd(cur_dir, 100);
		printf("CLIENT: %s$ ", cur_dir);

		fgets(command, 35, stdin);
		command[strlen(command)-1] = '\0';

		if (!strncmp (command, "put", 3))
			put_client (socket, &command[4]);
		else if (!strncmp (command, "get", 3))
			get_client (socket, &command[4]);
		else if (!strncmp (command, "ls", 2))
			ls_local (command);
		else if (!strncmp (command, "rls", 3))
			rls_client (socket, &command[1]);  //gambiarra pra tirar o 'r'
		else if (!strncmp (command, "cd", 2))
			cd_command (&command[3]);
		else if (!strncmp (command, "rcd", 3))
			rcd_client (socket, &command[4]);
		else if (!strncmp (command, "exit", 4))
			exit = 1;
		else
			printf("comando invalido\n");
	}


	return 0;
}