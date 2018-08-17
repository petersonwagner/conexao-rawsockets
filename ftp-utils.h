#ifndef __COMMLINE__
#define __COMMLINE__
#include <stdint.h>
#include <unistd.h>


#define ACK 			0
#define SIZE 			2
#define OK 				3
#define CD 				6
#define LS 				7
#define GET 			8
#define PUT 			9
#define END				10 //A
#define SHOW_ON_SCREEN	12 //C
#define DATA 			13 //D
#define ERROR 			14 //E
#define NACK 			15 //F

#define TIMEOUT 0

#define DIR_DOESNT_EXIST   7
#define PERMISSION_ERR     8
#define INSUFFICIENT_SPACE 9

#define CONNECTION_LOST    5
#define SUCCESS			   6

#define DONT_CARE		   -1

#define MESSAGE_SIZE 64
#define DATA_SIZE 31

//void send_file (int socket, char *file_name);
int listen_socket(int socket, int seconds);
void print_message (uint8_t *message);
uint8_t get_size (uint8_t *message);
uint8_t get_sequence (uint8_t *message);
uint8_t get_type (uint8_t *message);
int check_parity (uint8_t *message);
uint8_t* get_data (uint8_t *message);
int check_start_delim (uint8_t *message);
int enough_space(char *path, int file_size);
long long int file_size(char *nome);
void receive_file (int socket, FILE *file);
void send_file (int socket, FILE *file);
void pack_message (uint8_t* message, uint8_t* data, uint8_t size, uint8_t sequence, uint8_t type);
void put_server (int socket, char *file_name);
void put_client (int socket, char *file_name);
void get_server (int socket, char *file_name);
void get_client (int socket, char *file_name);
int ls_local (char *command);
void rls_client (int socket, char *command);
void rls_server (int socket, char *command);
int cd_command (char *dir);
void rcd_client (int socket, char *directory);
void rcd_server (int socket, char *directory);
uint8_t send_message (int socket, uint8_t *message_to_send);
int receive_message (int socket, uint8_t *message_received);

#endif