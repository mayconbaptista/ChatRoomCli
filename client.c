#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define LENGTH 100

// Global variables
volatile sig_atomic_t flag = 0;

int sockfd = 0;
char name[32];

void str_overwrite_stdout()
{
	printf("%s", "> ");
	fflush(stdout);
}

// simula o methodo trim que remove o \n da string.
void remove_endl (char* arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{ // trim \n
		if (arr[i] == '\n')
		{
			arr[i] = '\0';
			break;
		}
	}
}

void catch_ctrl_c_and_exit(int sig)
{
    flag = 1;
}


void send_mensagem ()
{
  	char mensagem[LENGTH] = {};

	char buffer[LENGTH + 32] = {};

	while(true)
	{
		str_overwrite_stdout();
		fgets(mensagem, LENGTH, stdin);
		remove_endl(mensagem, LENGTH);

		if (strcmp(mensagem, "exit") == 0)
		{
			break;
		}
		else
		{
			sprintf(buffer, "%s: %s\n", name, mensagem);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(mensagem, LENGTH);
    	bzero(buffer, LENGTH + 32);
  	}
	
	catch_ctrl_c_and_exit(2);
}

void recv_mensagem ()
{
	char mensagem[LENGTH] = {};

  	while (true)
	{
		int receive = recv(sockfd, mensagem, LENGTH, 0);

		if (receive > 0)
		{
			printf("%s", mensagem);
			str_overwrite_stdout();
		}
		else if (receive == 0)
		{
			break;
		}
		else
		{
			// -1
		}
		memset(mensagem, 0, sizeof(mensagem));
  	}
}

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("Uso: %s <porta>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("User name: ");
	fgets(name, 32, stdin);
	remove_endl(name, strlen(name));


	if (strlen(name) > 32 || strlen(name) < 2)
	{
		printf("Intervalo de nome inválido [2-32).\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;

	// setando o socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("ERROR: socket.");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

  	// Connetando com o servidor
	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("ERROR: conexão.\n");
		return EXIT_FAILURE;
	}

	// Enviando o nome
	if(send(sockfd, name, 32, 0) < 0)
	{
		perror("Error ao enviar o nome.\n");
	}

	printf("BEM VINDO %s.\n",name);

	// criando a thread de para envio da menssagem
	pthread_t send_msg_thread;
  	if(pthread_create(&send_msg_thread, NULL, (void *) send_mensagem, NULL) != 0)
	{
		printf("ERROR: pthread coleta.\n");
    	return EXIT_FAILURE;
	}

	// criando a thread de recebimento das mesagens
	pthread_t recv_msg_thread;
  	if(pthread_create(&recv_msg_thread, NULL, (void*) recv_mensagem, NULL) != 0)
	{
		printf("ERROR: pthread ouvindo.\n");
		return EXIT_FAILURE;
	}

	while (true)
	{
		if(flag)
		{
			printf("\nBye .o/\n");
			break;
		}
	}

	close(sockfd);
	return EXIT_SUCCESS;
}
