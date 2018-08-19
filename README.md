# Conexao Rawsockets - Trabalho de Redes de Computadores I (Prof. Albini)

### Arquivos
- ```client.c```: Código fonte do cliente, envia os comandos.
- ```server.c```: Código fonte do servidor, recebe os comandos e os executa.
- ```ftp-utils.c```: Arquivo com a implementação das funções utilizadas nos dois arquivos anteriores.
- ```ftp-utils.h```: Cabeçalho das funções.
- ```ConexaoRawSocket.c```: Código pronto para a conexão do RawSocket.
- ```makefile```: Arquivo para compilação.


### Comandos de execução e compilação
- Compilar o cliente e o servidor: ```make```
- Executar o servidor: ```sudo ./server```
- Executar o cliente: ```sudo ./client```


### Comandos do programa
- ```ls```: Mostra os arquivos e os diretórios do diretório atual do cliente.
- ```rls```: Mostra os arquivos e os diretórios do diretório atual do servidor. Esse comando é transferido como se fosse um arquivo por ser aberto por popen(“ls [...]”, "r") pelo servidor, portanto ele usa as mesmas funções do get e put.
- ```cd directory```: Abre um diretório directory do cliente. A implementação é feita utilizando a função chdir.
- ```rcd directory```: Abre um diretório directory do servidor. Funciona da mesma maneira do cd local e envia a resposta do comando.
- ```get file_name```: Recebe o arquivo file_name do servidor. Verificação de espaço livre é feita utilizando a função statvfs.
- ```put file_name```: Envia o arquivo file_name para o servidor. Verificação de espaço livre também é feita utilizando a função statvfs.
- ```exit```: Encerra o programa.
