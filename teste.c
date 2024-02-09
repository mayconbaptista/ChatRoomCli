#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
