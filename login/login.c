#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
#ifndef LOGIN_H
#define LOGIN_H
*/

int logar (char *user_name)
{
    if(user_name == NULL) return -1;

    if(strlen(user_name) < 2 || strlen(user_name) > 32) return -2;

    FILE *users = fopen("users.txt","r");
    
    if(users == NULL) return -3;

    char login[32];

    while(fscanf(users, "%s\n", login) != EOF)
    {
        // printf("login [%s]\n",login);

        if(strcmp(user_name, login) == 0)
        {
            fclose(users);
            return 1; // true
        }
    }

    fclose(users);
    return 0; // false;
}


int cadastra_user (char *user_name)
{
    if(user_name == NULL) return -1;

    if(strlen(user_name) < 2 || strlen(user_name) > 32) return 0;

    if(logar(user_name) == 1) return 1;

    FILE *users = fopen("./users/users.txt", "a");

    if(users == NULL) return -1;

    fprintf(users, "%s\n", user_name);

    fclose(users);
    return 1;
}

int set_mensagens (char *msn)
{
    if(msn == NULL) return -1;

    FILE *usersOff = fopen("usersOff.txt", "r");

    if( usersOff == NULL) return -2;

    char name[32], path[60];

    int erros = 0;

    while (fscanf(usersOff, "%s\n", name) != EOF)
    {
        bzero(path, 59);
        strcpy(path, "users/\0");
        strcat(path, name);
        strcat(path, ".txt\0");

        FILE *arg = fopen(path, "a");

        if(arg == NULL)
        {
            erros++;
            continue;
        }

        fprintf(arg, "%s\n", msn);
        fclose(arg);
    }
    
    fclose(usersOff);
    return erros;
}

int existe (char *userName)
{
    if(userName == NULL) return 0;

    char login[32];

    FILE *users = fopen("./users/users.txt", "r");

    if(users == NULL) return -1;

    while (fscanf(users, "%s %s\n", login) != EOF)
    {
        if(strcmp(userName, login) == 0) {
            
            fclose(users);
            return 1;
        }
    }
    
    fclose(users);
    return 0;
}

int send_msgs_not_read (char *userName)
{
    if(userName == NULL) return 0;


    FILE *users = fopen("./users/users.txt", "r");

    if(users == NULL) return -1;

    char path[60], msg[101];
    strcat(path, "./login/users/\0");
    strcat(path, userName);
    strcat(path, ".txt");

    while (fgets(msg, 100, users) != EOF)
    {
        // send()
    }
    
    fclose(users);
    return 0;
}

void remove_name(char *name)
{
    FILE *fptr1, *fptr2;
    char filename[] = "usersOff.txt";
    char temp_filename[] = "temp.txt";
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