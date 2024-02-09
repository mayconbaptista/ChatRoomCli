#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 100

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

// strutura para salvar os dados do client
typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client_t;

client_t *clients[MAX_CLIENTS]; // numero maximo de clientes

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void remove_name (char *name);
int send_msgs_not_read (char *userName, client_t *cli);
int client_login (char *userName);
int set_mensagens (char *msn);
int cadastra_user (char *user_name);

void str_overwrite_stdout()
{
    printf("\r%s", "> ");
    fflush(stdout);
}

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

// adiciona clients na lista estatica
void client_add (client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
	{
		if(!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Remove clients da lista estatica
void client_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
	{
		if(clients[i]){
			if(clients[i]->uid == uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Enviando mensagem para todos os clientes exeto ele mesmo.
void send_message(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(clients[i])
		{
			if(clients[i]->uid != uid)
			{
				if(write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: ao gravar a mensagem.");
					break;
				}
			}
		}
	}

	set_mensagens(s);

	pthread_mutex_unlock(&clients_mutex);
}


void print_client_addr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// Cuidando de toda a comunicação com os clients
void *handle_client(void *arg)
{
	char buff_out[BUFFER_SZ];

	char name[32];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) > 32)
	{
		printf("usuário sem nome.\n");
		leave_flag = 1;
	}
	else
	{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s entrou na sala.\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid);

		// verifica se o osuário já logou antes
		if(client_login(name) == 1)
		{
			send_msgs_not_read(name, cli);
			remove_name(cli->name);
		}
	}

	bzero(buff_out, BUFFER_SZ);

	while(true)
	{
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		
		if (receive > 0)
		{
			if(strlen(buff_out) > 0)
			{
				remove_endl(buff_out, strlen(buff_out));
				send_message(buff_out, cli->uid);
				printf("%s -> %s\n", buff_out, cli->name);
			}
		}
		else if (receive == 0 || strcmp(buff_out, "exit") == 0)
		{
			sprintf(buff_out, "%s saiu.\n", cli->name);
			printf("%s", buff_out);
			cadastra_user(cli->name); // cadastra user na pasta de offlines
			send_message(buff_out, cli->uid); // informa a todos que o user saiu
			leave_flag = 1;
		}
		else
		{
			printf("ERROR: cod -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

	// deleta o client da fila e yield thread 
	close(cli->sockfd);
	client_remove(cli->uid);
	free(cli);
	cli_count--;

	pthread_detach(pthread_self());
	return NULL;
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
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	//Criando um socket TCP (SOCK_STREAM) e apenas endereços IPv4 (AF_INET)
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

  	// Ignore pipe signals
	signal(SIGPIPE, SIG_IGN);

	//configurando
	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
	{
		perror("ERROR: ao criar o socket.");
    	return EXIT_FAILURE;
	}

	// Associando servidor a uma porta e apartir de agora nenhum outro processo terá acesso a essa porta
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR: falha ao vincular o socketa uma porta.");
		return EXIT_FAILURE;
	}

	// Entrar em escuta para receber conexões de clientes
	if (listen(listenfd, 10) < 0)
	{
		perror("ERROR: Falha ao colocar o socked na escuta.");
		return EXIT_FAILURE;
	}

	printf("SERVIDOR RUNNIG ...\n");

	while(true)
	{
		socklen_t clilen = sizeof(cli_addr);

		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		if((cli_count + 1) == MAX_CLIENTS)
		{
			printf("Numero maxímo de clients atingido.\n");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		// Instanciando novo client
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		// Addiciona novo client a fila e fork thread
		client_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		sleep(1);
	}

	printf("SERVER OFF\n");
	return EXIT_SUCCESS;
}

// ++++++++++++++++++++++++++++++++ func axiliares login

// coloca o nome do usuário no arquivo de deslogados
int cadastra_user (char *user_name)
{
    if(user_name == NULL) return -1;

    if(strlen(user_name) < 2 || strlen(user_name) > 32) return 0;

    FILE *users = fopen("./login/usersOff.txt", "a");

    if(users == NULL) return -1;

    fprintf(users, "%s\n", user_name);

    fclose(users);
    return 1;
}// ok

// quarda todas as mensagens dos clients que estão offline
int set_mensagens (char *msn)
{
    if(msn == NULL) return -1;

    FILE *usersOff = fopen("./login/usersOff.txt", "r");

    if( usersOff == NULL) return -2;

    char name[32], path[60];

    unsigned int erros = 0;
	memset(name, '\0', sizeof(name));

    while (!feof(usersOff))
    {
		fscanf(usersOff, "%s\n", name);
        bzero(path, 59);
		sprintf(path, "./login/users/%s.txt", name);

		if(strlen(name) >= 2)
		{
			FILE *arg = fopen(path, "a");

			if(arg == NULL)
			{
				erros++;
			}
			
			remove_endl(msn, strlen(msn));
			fprintf(arg, "%s\n", msn);
        	fclose(arg);
		}
    }
    
    fclose(usersOff);
    return erros;
}//ok

// verifica se o crient e novo
int client_login (char *userName)
{
    if(userName == NULL) return 0;

    char login[32];

    FILE *users = fopen("./login/usersOff.txt", "r");

    if(users == NULL) return -1;

    while (fscanf(users, "%s\n", login) != EOF)
    {
        if(strcmp(userName, login) == 0) {
            
            fclose(users);
            return 1;
        }
    }
    
    fclose(users);
    return 0;
}

int send_msgs_not_read (char *userName, client_t *cli)
{
    if(userName == NULL) return -1;

    char path[60], msg[100];

	bzero(path, 59);
	sprintf(path,"./login/users/%s.txt",userName);

    FILE *users = fopen(path, "r");

    if(users == NULL) return -2;

	unsigned int countt_erros = 0;

    while (!feof(users))
    {
		fgets(msg, 100, users);

		if(write(cli->sockfd, msg, strlen(msg)) < 0)
		{
			countt_erros++;
		}
    }
    
    fclose(users);
	remove(path);
    return countt_erros;
}

void remove_name (char *name)
{
    FILE *fptr1, *fptr2;
    char filename[] = "./login/usersOff.txt";
    char temp_filename[] = "./login/temp.txt";
    char line[100];
    int line_len;

    // Abre o arquivo original para leitura
    fptr1 = fopen(filename, "r");
    if (fptr1 == NULL)
    {
        printf("Erro ao abrir o arquivo %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Cria um novo arquivo temporário para escrita
    fptr2 = fopen(temp_filename, "w");
    if (fptr2 == NULL)
    {
        printf("Erro ao criar o arquivo %s\n", temp_filename);
        exit(EXIT_FAILURE);
    }

    // Lê cada linha do arquivo original
    while (fgets(line, 100, fptr1) != NULL)
    {
        line_len = strlen(line);

        // Verifica se a linha contém o nome
        if (strstr(line, name) == NULL) {
            // Se não, escreve a linha no arquivo temporário
            fwrite(line, line_len, 1, fptr2);
        }
    }

    // Fecha ambos os arquivos
    fclose(fptr1);
    fclose(fptr2);

    // Remove o arquivo original
    remove(filename);

    // Renomeia o arquivo temporário com o nome do arquivo original
    rename(temp_filename, filename);
}