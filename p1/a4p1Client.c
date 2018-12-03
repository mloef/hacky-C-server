#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 4096 /*max text line length*/

int
main(int argc, char **argv) 
{
    int sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE], sig[32], stamp[128];
    FILE* fp0;
    FILE* fp1;

    if (argc != 4) {
        perror("Usage: TCPClient <Server IP> <Server Port> <logfile name>"); 
        exit(1);
    }

    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(2);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2])); 

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        perror("Problem in connecting to the server");
        exit(3);
    }
    gethostname(sig, 32); 
    send(sockfd, sig, 32, 0);    
    
    fp0 = fopen(argv[3], "a");

    while (fgets(sendline, MAXLINE, stdin) != NULL) {
        
        fp1 = popen("hostname; date", "r");
        fread(stamp, sizeof(' '), 128, fp1);
        printf("%s\n", stamp);
        fprintf(fp0,"%s\n", stamp);

        send(sockfd, sendline, strlen(sendline), 0);
        fprintf(fp0, "String sent to server: %s\n", sendline);

        if (strcmp(sendline, "exit\n") == 0)
            exit(0);
            
        if (recv(sockfd, recvline, MAXLINE,0) == 0){
            perror("The server terminated prematurely"); 
            exit(4);
        }

        printf("String received from the server:\n%s\n", recvline);
        fprintf(fp0, "String received from the server:\n%s\n\n", recvline);
        printf("\n");
        fclose(fp1);
    }
    
    fclose(fp0);
    exit(0);
}
