#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>

#define COR_AZUL   "\x1b[34m"
#define COR_VERDE  "\x1b[32m"
#define NEGRITO  "\e[1m"
#define RESET_COR  "\x1b[0m"

/*Mensagem inicial que aparece no topo do shell*/
void msgInicial(){
    printf("\n===== BEM VINDO AO SHELL =====\n\n");
}

/*Funcao para mostrar o diretorio em 
que o shellesta sendo executado*/
void mostrarDiretorio(){
    char cwd[1024], username[1024];

    getlogin_r(username, 1024); //Pega o nome do usuario

    if (getcwd(cwd, sizeof(cwd)) != NULL){
        printf(COR_VERDE NEGRITO "%s@SO" RESET_COR, username);
        printf(NEGRITO ":" RESET_COR);
        printf(COR_AZUL NEGRITO "%s" RESET_COR, cwd);
    }else 	perror("falha ao carregar diretorio\n");
    printf("$ ");
}

/*Funcao que quebra a string de entrada em diversos 
argumentos com base em um delimitador*/
void quebraArgumentos(char** argumentos,int *numArgs,char *buffer, char *delimitador){
	char *token;
	token=strtok(buffer,delimitador);

	int i = 0;

	while(token){
		argumentos[i]=malloc(sizeof(token)+1);
		strcpy(argumentos[i],token);
        argumentos[i][strcspn(argumentos[i], "\n")] = 0;
		token=strtok(NULL,delimitador);
        i++;
	}
	argumentos[i]=NULL;
	*numArgs=i;
}

/*Funcao para executar um comando basico
sem redirecionador ou pipes*/
void comandoBasico(char** argv){
	if(fork()>0){
		wait(NULL);
	}
	else{
		execvp(argv[0],argv);
        perror(argv[0]); //Caso o comando nao exista emite uma mensagem de erro
        exit(1);
	}
}

/*Redireciona a saida do comando para um arquivo
se o arquivo nao existe ele eh criado*/
void redirecionaSaida(char** buffer){
    int aux;
    char *argv[100];

    quebraArgumentos(argv,&aux,buffer[0]," ");

    if(fork() == 0){
        int redirecionaFd = open(buffer[1],O_WRONLY , S_IRUSR | S_IWUSR); //Tags para escrita e acesso ao arquivo

        //Caso o arquivo nao exista eh incluida a tag de criacao de arquivo
        if(redirecionaFd < 0){
            redirecionaFd = open(buffer[1],O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR);
        }

        //Verifica novamente se o arquivo foi criado com sucesso
        if(redirecionaFd < 0){
            perror("Erro ao criar arquivo!");
        }

        dup2(redirecionaFd,1);

        execvp(argv[0],argv);
        perror(argv[0]);
        exit(1);
    }

    wait(NULL);
}


/*Funcao que le e define qual tipo
de comando vai ser executado pelo shell*/
void leituraComando(){
    char buffer[1024], *argumentos[100];
    int numArgs=0;
    fgets(buffer,1024,stdin);

    buffer[strcspn(buffer, "\n")] = 0; //Remove o \n no final da string

    if(strcmp(buffer,"fim") == 0){
        printf("\n");
        exit(1);
    }else if (strstr(buffer,"=>")){ //Redirecionar saida para arquivo
        quebraArgumentos(argumentos,&numArgs,buffer,"=>");
        redirecionaSaida(argumentos);
    }else if (strstr(buffer,"<=")){ //Redirecionar entrada para arquivo
        
    }else if (strchr(buffer,'&')){ //Execucao em background
        
    }else{ //Comando simples
        quebraArgumentos(argumentos,&numArgs,buffer," ");
        comandoBasico(argumentos);
    }
}

int main(int argc, char const *argv[])
{
    int aux;

    msgInicial();

    while (1)
    {
        mostrarDiretorio();

        /*Leitura dos comandos*/
        leituraComando();
    }
    
    return 0;
}
