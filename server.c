#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "ftp-utils.h"


#define DEVICE "eno1"

int ConexaoRawSocket(char *device);



int main(int argc, char **argv)
{
	char cur_dir[100];
	uint8_t *message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
	int socket = ConexaoRawSocket (DEVICE);
	printf("Executando: server\n");

	getcwd(cur_dir, 100);
	printf("SERVER: %s@ ", cur_dir);
	fflush(stdout);

	while(1)
	{
		read(socket, message, MESSAGE_SIZE);
		if (check_start_delim(message))
		{
			if (check_parity(message))
			{
				if (get_type(message) == PUT)
				{
					printf("receiving put\n");
					put_server (socket, get_data(message));
				}
				else if (get_type(message) == GET)
				{
					printf("received get\n");
					get_server (socket, get_data(message));
				}
				else if (get_type(message) == LS)
				{
					printf("received rls\n");
					rls_server (socket, get_data(message));
				}
				else if (get_type(message) == CD)
				{
					printf("received rcd\n");
					rcd_server (socket, get_data(message));
				}

				getcwd(cur_dir, 100);
				printf("\nSERVER: %s@ ", cur_dir);
				fflush(stdout);
			}
			else
			{
				pack_message (message, NULL, 0, 0, NACK);
				write (socket, message, MESSAGE_SIZE);
			}
		}
	}


	return 0;
}