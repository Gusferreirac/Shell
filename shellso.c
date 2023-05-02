#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

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
void comandoBasico(char** argv, int numArgs){
    int async = 0;

    //Verifica se eh um comando assincrono
    if(strcmp(argv[numArgs-1],"&") == 0){
        async = 1;
        argv[numArgs-1]= '\0';
        signal(SIGCHLD, SIG_IGN); 
    }

	if(fork() == 0){ //Filho
		execvp(argv[0],argv);
        perror(argv[0]); //Caso o comando nao exista emite uma mensagem de erro
        exit(1);
	}else{
        if(async != 1){
            wait(NULL);
        }
    }
}

/*Funcao para redirecionar entrada para arquivo*/
void redirecionaEntrada(char **buffer, int numArgs, int redSaida){
    int aux,aux2,i,saidaFd, async = 0;
    char *argv[100], *buffer2[100];

    quebraArgumentos(argv,&aux,buffer[0]," ");
    quebraArgumentos(buffer2,&aux2,buffer[numArgs-1]," >");

    buffer[1]++;

    if(redSaida == 1){
        int tamanhoString = 0;

        tamanhoString = strlen(buffer[1]);
        buffer[1][tamanhoString-1]='\0';
    }

    if(strcmp(buffer2[aux2-1],"&") == 0){
        async = 1;
        argv[numArgs-1]= '\0';
        signal(SIGCHLD, SIG_IGN); 
    }

    if(fork() == 0){ //Filho
        if(redSaida == 1){
            saidaFd = open(buffer2[0],O_WRONLY , S_IRUSR | S_IWUSR); //Tags para escrita e acesso ao arquivo

            //Caso o arquivo nao exista eh incluida a tag de criacao de arquivo
            if(saidaFd < 0){
                saidaFd = open(buffer2[0],O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR);
            }

            //Verifica novamente se o arquivo foi criado com sucesso
            if(saidaFd < 0){
                perror("Erro ao criar arquivo!");
            }

            dup2(saidaFd,1);  
        }

        int redirecionaFd = open(buffer[1],O_RDONLY, S_IRUSR | S_IWUSR); //Tags para leitura e acesso ao arquivo

        //Caso o arquivo nao exista eh exibida uma mensagem de erro
        if(redirecionaFd < 0){
            perror("Erro ao abrir o arquivo!");
            exit(1);
        }

        dup2(redirecionaFd,0);

        execvp(argv[0],argv);
        perror(argv[0]);
        exit(1);
    }else{
        if(async != 1){
            wait(NULL);
        }
    }
}

/*Redireciona a saida do comando para um arquivo
se o arquivo nao existe ele eh criado*/
void redirecionaSaida(char** buffer, int numArgs){
    int aux,aux2, async = 0;
    char *argv[100], *buffer2[100];

    quebraArgumentos(argv,&aux,buffer[0]," ");
    quebraArgumentos(buffer2,&aux2,buffer[1]," ");

    if(strcmp(buffer2[aux2-1],"&") == 0){
        async = 1;
        signal(SIGCHLD, SIG_IGN); 
    }

    if(fork() == 0){ //Filho
        int redirecionaFd = open(buffer2[0],O_WRONLY , S_IRUSR | S_IWUSR); //Tags para escrita e acesso ao arquivo

        //Caso o arquivo nao exista eh incluida a tag de criacao de arquivo
        if(redirecionaFd < 0){
            redirecionaFd = open(buffer2[0],O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR);
        }

        //Verifica novamente se o arquivo foi criado com sucesso
        if(redirecionaFd < 0){
            perror("Erro ao criar arquivo!");
        }

        dup2(redirecionaFd,1);

        execvp(argv[0],argv);
        perror(argv[0]);
        exit(1);
    }else{
        if(async != 1){
            wait(NULL);
        }
    }
}


/*Funcao que le e define qual tipo
de comando vai ser executado pelo shell*/
void leituraComando(){
    char buffer[1024], *argumentos[100];
    int redSaida = 0, numArgs=0;
    fgets(buffer,1024,stdin);

    buffer[strcspn(buffer, "\n")] = 0; //Remove o \n no final da string

    if(strcmp(buffer,"fim") == 0){
        printf("\n");
        exit(1);
    }else if (strstr(buffer,"<=")){ //Redirecionar entrada para arquivo
        if(strstr(buffer,"=>")){
            redSaida = 1;
        }
        quebraArgumentos(argumentos,&numArgs,buffer,"<=");
        redirecionaEntrada(argumentos, numArgs, redSaida);
    }else if (strstr(buffer,"=>")){ //Redirecionar saida para arquivo
        quebraArgumentos(argumentos,&numArgs,buffer,"=>");
        redirecionaSaida(argumentos, numArgs);
    }else{ //Comando simples
        quebraArgumentos(argumentos,&numArgs,buffer," ");
        comandoBasico(argumentos, numArgs);
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
