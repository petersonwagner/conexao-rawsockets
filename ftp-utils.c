#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <errno.h>
#include "ftp-utils.h"



//O operador '%' do C NÃO É a operação de módulo
//ref:https://stackoverflow.com/questions/11720656/modulo-operation-with-negative-numbers
int mod (int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

uint8_t send_sequenced_msg (int socket, uint8_t *message_to_send, int sequence)
{
	int trials_left = 16;
	uint8_t *message_received = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));

	write(socket, message_to_send, MESSAGE_SIZE);

	do
	{
		if (listen_socket(socket, 1) == TIMEOUT)
		{
			write(socket, message_to_send, MESSAGE_SIZE);
			trials_left--;
		}
		else
		{
			read (socket, message_received, MESSAGE_SIZE);

			if (check_start_delim(message_received) && check_parity(message_received))
			{
				if (get_type(message_received) == NACK)
					trials_left--;
				else if (get_sequence(message_received) == sequence)
				{
					return SUCCESS;
				}
			}
		}
		
	} while (trials_left > 0);

	return CONNECTION_LOST;
}

int receive_sequenced_message (int socket, uint8_t *message_received, int sequence)
{
	int trials_left = 16;

    uint8_t *message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));

	do
	{
		if (listen_socket(socket, 1) != TIMEOUT)
		{
			read (socket, message_received, MESSAGE_SIZE);

			if (check_start_delim(message_received) && check_parity(message_received))
			{
				if (get_sequence(message_received) == sequence)
				{
					return SUCCESS;
				}
				else if (get_sequence(message_received) == mod(sequence-1, 64))
				{
					printf("resending ack, was expecting %d and received %d\n", sequence, get_sequence(message_received));
					pack_message (message, NULL, 0, (sequence - 1) % 64, ACK);
					write (socket, message, MESSAGE_SIZE);
				}
			}
		}
		else
			trials_left--;
	} while (trials_left > 0);

	printf("connection lost\n");
	return CONNECTION_LOST;
}

void send_file (int socket, FILE *file)
{
	int bytes_read, receiver_response, sequence = 0;
	uint8_t *message, *data;
    struct timeval timeout = {1,0};

    message    = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 	   = (uint8_t*) malloc (DATA_SIZE    * sizeof(uint8_t));

	do
	{
		bytes_read = fread (data, sizeof(uint8_t), DATA_SIZE, file);
		pack_message (message, data, bytes_read, sequence, DATA);
		receiver_response = send_sequenced_msg (socket, message, sequence);

		if (receiver_response == CONNECTION_LOST)
		{
			printf("connection lost\n");
			printf("my sequence was %d\n", sequence);
			return;
		}

		sequence = (sequence + 1) % 64;

	} while (!feof(file));

	pack_message (message, data, 0, sequence, END);
	receiver_response = send_sequenced_msg (socket, message, sequence);
	
	free(message);
	free(data);
}

void receive_file (int socket, FILE *file)
{
	int sender_response;
	uint8_t *message, *data;
	uint8_t sequence = 0, completed = 0;
    struct timeval timeout = {1,0};

	message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 	= (uint8_t*) malloc (DATA_SIZE 	  * sizeof(uint8_t));

    do
	{
		sender_response = receive_sequenced_message (socket, message, sequence);
		if (sender_response == CONNECTION_LOST)
		{
			printf("connection lost\n");
			printf("my sequence was %d\n", sequence);
			return;
		}


		if (get_type(message) == DATA)
			fwrite (get_data(message), 1, get_size (message), file);
		else if(get_type(message) == END)
			completed = 1;
		

		pack_message (message, data, 0, sequence, ACK);
		write(socket, message, MESSAGE_SIZE);
		sequence = (sequence + 1) % 64;
	} while (!completed);

	free(message);
	free(data);

	return;
}



uint8_t send_message (int socket, uint8_t *message_to_send)
{
	int trials_left = 16;
	uint8_t *message_received = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));

	write(socket, message_to_send, MESSAGE_SIZE);

	do
	{
		if (listen_socket(socket, 2) == TIMEOUT)
		{
			write(socket, message_to_send, MESSAGE_SIZE);
			trials_left--;
		}
		else
		{
			read (socket, message_received, MESSAGE_SIZE);

			if (check_start_delim(message_received) && check_parity(message_received))
			{
				if (get_type(message_received) == NACK)
					trials_left--;
				else if (get_type(message_received) == ERROR)
					return *get_data(message_received);
				else
					return SUCCESS;
			}
		}
		
	} while (trials_left > 0);

	return CONNECTION_LOST;
}

int receive_message (int socket, uint8_t *message_received)
{
	int trials_left = 16;

	do
	{
		if (listen_socket(socket, 2) != TIMEOUT)
		{
			read (socket, message_received, MESSAGE_SIZE);

			if (check_start_delim(message_received) && check_parity(message_received))
				return SUCCESS;
		}
		else
			trials_left--;
	} while (trials_left > 0);

	printf("connection lost\n");
	return CONNECTION_LOST;
}


/*DEBUG*/
void print_message (uint8_t *message)
{
	printf("\nmessage:\n");
	printf("inicio: %d | size: %d | sequence: %d | type: %d\n", message[0], get_size(message), get_sequence(message), get_type(message));

	printf("\ndata: ");

	for (int i = 3; i < get_size(message) + 3; ++i)
		printf("%d|", message[i]);

	printf("\nparity: %d\n", message[get_size(message)+3]);
	printf("========================\n");

	fflush (stdout);
}

int ls_local (char *command)
{
	FILE *fp;
	char path[PATH_MAX];
	fp = popen(command, "r");

	while (fgets(path, PATH_MAX, fp) != NULL)
		printf("%s", path);

	pclose(fp);

	return 0;
}

int cd_command (char *dir)
{
	if(chdir(dir) < 0)
	{
		if (errno == EACCES)
		{
			printf("Permissão negada.\n");
			return PERMISSION_ERR;
		}
		else if (errno == ENOTDIR)
		{
			printf("%s não é um diretorio.\n", dir);
			return DIR_DOESNT_EXIST;
		}
		else if (errno == ENOENT)
		{
			printf("%s não existe.\n", dir);
			return DIR_DOESNT_EXIST;
		}

		return PERMISSION_ERR;
	}
	else
		return 0;
}


int listen_socket(int socket, int seconds)
{
	/**/int ret;
	fd_set set;
	struct timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(socket, &set);

	if ((ret = select (FD_SETSIZE, &set, NULL, NULL, &timeout)) == 0)
		printf("\nTIMEOUT\n");
	
	return ret;

	//return select (FD_SETSIZE, &set, NULL, NULL, &timeout);
}

uint8_t get_size (uint8_t *message)
{
	return message[1] >> 3;
}

uint8_t get_sequence (uint8_t *message)
{
	return (((message[1] << 3) & 0x38) | (message[2] >> 5));
}

uint8_t get_type (uint8_t *message)
{
	return message[2] & 0x1F;
}

uint8_t* get_data (uint8_t *message)
{
	return &message[3];
}

int check_start_delim (uint8_t *message)
{
	if (message[0] == 0x7E)
		return 1;
	else
		return 0;
}

int check_parity (uint8_t *message)
{
	uint8_t xor_vert = 0, size;
	size = get_size(message);

	for (int i = 0; i < size+3; ++i)
		xor_vert = xor_vert ^ message[i];

	if (message[size+3] == xor_vert)
		return 1;
	else
	{
		printf("paridade deu ruim: msg.crc:%d != crc:%d\n", message[size+3], xor_vert);
		return 0;
	}
}

void rls_client (int socket, char *command)
{
	uint8_t *message, *data;
	int server_response;
	
	message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 	= (uint8_t*) malloc (DATA_SIZE 	  * sizeof(uint8_t));


	//envia o comando LS pro server
	memcpy (data, command, 31);
	pack_message (message, data, 31, 0, LS);
	server_response = send_message (socket, message);

	if (server_response == CONNECTION_LOST)
	{
		printf("conexao perdida\n");
		return;
	}
	else if (server_response == PERMISSION_ERR)
	{
		printf("erro de permissao\n");
		return;
	}

	receive_file (socket, stdout);


	free(message);
	free(data);

}

void rls_server (int socket, char *command)
{
	uint8_t size;
	uint8_t *message, *data;
	
	message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 	= (uint8_t*) malloc (DATA_SIZE 	  * sizeof(uint8_t));
	FILE *process_fp = popen (command, "r");


	//envia resposta para o comando ls
	if (process_fp == NULL)
	{
		data[0] = PERMISSION_ERR;
		pack_message (message, data, 1, 0, ERROR);
		write(socket, message, MESSAGE_SIZE);
		return;
	}
	else
	{
		pack_message (message, data, 0, 0, OK);
		write(socket, message, MESSAGE_SIZE);
	}

	send_file (socket, process_fp);

	free(message);
	free(data);
	pclose (process_fp);
}


void pack_message (uint8_t *message, uint8_t* data, uint8_t size, uint8_t sequence, uint8_t type)
{
	uint8_t xor_vert = 0;
	message[0] = 0x7E; //MARCADOR DE INICIO
	message[1] = ((size     << 3) & 0xF8) | ((sequence >> 3) & 0x07);
	message[2] = ((sequence << 5) & 0xF8) | ((type)          & 0x1F);
	memcpy (&message[3], data, size);


	for (int i = 0; i < size+3; ++i)
		xor_vert = xor_vert ^ message[i];
	message[size+3] = xor_vert;
	
	return;
}

int enough_space(char *path, int file_size)
{
	struct statvfs stat;
	if(statvfs(path,&stat) != 0)
		return -1;

	//tamanho do bloco * numero de blocos disponiveis = total de bytes disponiveis
	return ((stat.f_bsize * stat.f_bavail) > file_size);
}

long long int file_size(char *nome)
{
	struct stat st;
	stat(nome, &st);
	return st.st_size;
}


void get_client (int socket, char *file_name)
{
	int server_response;
	long int file_size;

	uint8_t *message, *data;
  	message	 = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data     = (uint8_t*) malloc (DATA_SIZE    * sizeof(uint8_t));


    //envia o comando get com o nome do arquivo
	strncpy (data, file_name, 31);
	pack_message (message, data, strlen(file_name)+1, 0, GET);
	server_response = send_message (socket, message);

	if (server_response == CONNECTION_LOST)
	{
		printf("connection lost\n");
		return;
	}
	
	if (server_response == DIR_DOESNT_EXIST)
	{
		printf("arquivo nao existe\n");
		return;
	}
	else if (server_response == PERMISSION_ERR)
	{
		printf("erro de acesso/permissao\n");
		return;
	}

	FILE *file = fopen (file_name, "w");
  	if (file == NULL)
  	{
  		printf("ARQUIVO NAO ENCONTRADO\n");
  		return;
  	}

	file_size = atol (get_data(message));

	//envia resposta sobre o tamanho do arquivo
	if (enough_space (".", file_size))
	{
		pack_message (message, data, 0, 1, OK);
		write(socket, message, MESSAGE_SIZE);
	}
	else
	{
		printf("espaço insuficiente\n");
		data[0] = INSUFFICIENT_SPACE;
		pack_message (message, data, 1, 1, ERROR);
		write(socket, message, MESSAGE_SIZE);
	}

	printf("RECEBENDO ARQUIVO\n");
	receive_file (socket, file);
	printf("ARQUIVO RECEBIDO COM SUCESSO\n");

	free (message);
	free (data);
	fclose (file);

	return;
}

void get_server (int socket, char *file_name)
{
	int client_response;
	uint8_t *message, *data;
    message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data    = (uint8_t*) malloc (DATA_SIZE    * sizeof(uint8_t));

    FILE *file = fopen (file_name, "r");
  	if (file == NULL)
  	{
		if(access(file_name, F_OK) != 0)
  		{
  			printf("sem permissao de acesso\n");
  			data[0] = DIR_DOESNT_EXIST;
  			pack_message (message, data, 1, 0, ERROR);
			write(socket, message, MESSAGE_SIZE);
  		}

  		else if(access(file_name, R_OK) != 0)
  		{
  			printf("sem permissao de leitura\n");
  			data[0] = PERMISSION_ERR;
  			pack_message (message, data, 1, 0, ERROR);
			write(socket, message, MESSAGE_SIZE);
  		}
  		return;
  	}


	//envia o tamanho do arquivo
	snprintf(data, 31, "%llu", file_size(file_name));
	data[30] = '\0';
	pack_message (message, data, 31, 0, SIZE);
	client_response = send_message (socket, message);


	if (client_response == CONNECTION_LOST)
	{
		printf("connection lost\n");
		return;
	}
	else if (client_response == INSUFFICIENT_SPACE)
	{
		printf("ESPAÇO INSUFICIENTE\n");
		return;
	}

	printf("ENVIANDO ARQUIVO\n");
    send_file (socket, file);
	printf("ARQUIVO ENVIADO COM SUCESSO\n");

	free (message);
	free (data);
	fclose (file);

	return;
}



void put_client (int socket, char *file_name)
{
	int server_response;
    FILE *file = fopen (file_name, "r");
  	if (file == NULL)
  	{
  		printf("ARQUIVO NAO ENCONTRADO\n");
  		return;
  	}

	uint8_t *message, *data;
    message 	 = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 		 = (uint8_t*) malloc (DATA_SIZE    * sizeof(uint8_t));

	
	//envia o comando put com o nome do arquivo
	strncpy (data, file_name, 31);
	pack_message (message, data, strlen(file_name)+1, 0, PUT);
	server_response = send_message (socket, message);
	

	//le a resposta para o put
	if (server_response == CONNECTION_LOST)
	{
		printf("connection lost\n");
		return;
	}
	else if (server_response == PERMISSION_ERR)
	{
		printf("Erro de permissão.\n");
		return;
	}


	//envia o tamanho do arquivo
	snprintf(data, 31, "%llu", file_size(file_name));
	data[30] = '\0';
	pack_message (message, data, 31, 0, SIZE);
	server_response = send_message (socket, message);


	if (server_response == CONNECTION_LOST)
	{
		printf("connection lost\n");
		return;
	}
	else if (server_response == INSUFFICIENT_SPACE)
	{
		printf("espaço insuficiente\n");
		return;
	}

	printf("ENVIANDO ARQUIVO\n");
	send_file (socket, file);
	printf("ARQUIVO ENVIADO COM SUCESSO\n");


	free(message);
	free(data);
	fclose (file);
}


void put_server (int socket, char *file_name)
{
	int client_response;
	uint8_t size;
	uint8_t *message, *data;
	
	message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 	= (uint8_t*) malloc (DATA_SIZE 	  * sizeof(uint8_t));
	FILE *file = fopen (file_name, "w");

	if (file == NULL)
  	{
		if(access(file_name, F_OK) != 0)
  		{
  			printf("arquivo ou diretorio nao existe\n");
  			data[0] = DIR_DOESNT_EXIST;
  			pack_message (message, data, 1, 0, ERROR);
			write(socket, message, MESSAGE_SIZE);
  		}

  		else if(access(file_name, W_OK) != 0)
  		{
  			printf("sem permissao de escrita\n");
  			data[0] = PERMISSION_ERR;
  			pack_message (message, data, 1, 0, ERROR);
			write(socket, message, MESSAGE_SIZE);
  		}
  		return;
	}
	else
	{
  		pack_message (message, NULL, 0, 0, OK);
		write(socket, message, MESSAGE_SIZE);
	}


	client_response = receive_message (socket, message);
	if (client_response == CONNECTION_LOST)
	{
		printf("conexao perdida\n");
		return;
	}

	if (get_type(message) == SIZE)
	{
		size = atoi(get_data(message));
		if (enough_space(".", size))
		{
  			pack_message (message, NULL, 0, 0, OK);
			write(socket, message, MESSAGE_SIZE);
		}
		else
		{
			data[0] = PERMISSION_ERR;
			pack_message (message, data, 1, 0, ERROR);
			write(socket, message, MESSAGE_SIZE);
			printf("sem espaco\n");
			return;
		}
	}

	pack_message (message, data, 0, 0, OK);
	write(socket, message, MESSAGE_SIZE);

	printf("RECEBENDO ARQUIVO\n");
	receive_file (socket, file);
	printf("ARQUIVO RECEBIDO COM SUCESSO\n");

	fclose (file);

	free(message);
	free(data);

	return;
}


void rcd_client (int socket, char *directory)
{
	uint8_t *message, *data;
	int server_response;
	
	message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 	= (uint8_t*) malloc (DATA_SIZE 	  * sizeof(uint8_t));


	//envia o comando CD pro server
	strcpy (data, directory);
	pack_message (message, data, 31, 0, CD);
	
	server_response = send_message (socket, message);

	//le a resposta do server
	if (server_response == CONNECTION_LOST)
	{
		printf("connection lost\n");
		return;
	}
	else if (server_response == DIR_DOESNT_EXIST)
	{
		printf("Diretorio não existe\n");
		return;
	}
	else if (server_response == PERMISSION_ERR)
	{
		printf("Erro de permissão.\n");
		return;
	}


	free (message);
	free (data);

	return;
}

void rcd_server (int socket, char *directory)
{
	uint8_t error;
	uint8_t *message, *data;
	
	message = (uint8_t*) malloc (MESSAGE_SIZE * sizeof(uint8_t));
    data 	= (uint8_t*) malloc (DATA_SIZE 	  * sizeof(uint8_t));

	error = cd_command(directory);

	if (error)
	{
		data[0] = error;
		pack_message (message, data, 1, 0, ERROR);
		write(socket, message, MESSAGE_SIZE);
	}
	else
	{
		pack_message (message, data, 0, 0, OK);
		write(socket, message, MESSAGE_SIZE);
	}

	free (message);
	free (data);
	
	return;
}

