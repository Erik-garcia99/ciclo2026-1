#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(){

    char **tokens = malloc(5 * sizeof(char*));
    char *token;
    char input[50];
    int position = 0;

    printf("ingrese el comando: ");

    int i;
    for(i = 0; i < 50; i++){
        scanf("%c", &input[i]);      // bug 1 fix
        if(input[i] == '\n'){
            break;
        }
    }

    input[i] = '\0';                 // bug 4 fix

    token = strtok(input, " ");

    while(token != NULL){

        if(position >= 5) break;     // bug 3 fix: checa ANTES

        tokens[position] = strdup(token);
        position++;                  // bug 2 fix

        token = strtok(NULL, " ");
    }

    tokens[position] = NULL;

    for(int j = 0; j < position; j++){   // bug 5 fix
        printf("%s\n", tokens[j]);
    }

    // liberar memoria
    for(int j = 0; j < position; j++){
        free(tokens[j]);
    }
    free(tokens);

    return 0;
}