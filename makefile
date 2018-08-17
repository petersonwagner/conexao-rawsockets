all: server client

server: server.c ConexaoRawSocket.c ftp-utils.c
	gcc server.c ConexaoRawSocket.c ftp-utils.c -g -o server

client: client.c ConexaoRawSocket.c ftp-utils.c
	gcc client.c ConexaoRawSocket.c ftp-utils.c -g -o client

clean:
	rm -rf client server saida.pgm log* imagens/log* randomdir/*