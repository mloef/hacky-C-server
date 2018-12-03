#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

/*
    CONCURRENT SERVER: THREAD EXAMPLE
    Must be linked with the "pthread" library also, e.g.:
       cc -o example example.c -lnsl -lsocket -lpthread 

    This program creates a connection socket, binds a name to it, then
    listens for connections to the sockect.  When a connection is made,
    it accepts messages from the socket until eof, and then waits for
    another connection...

    This is an example of a CONCURRENT server -- by creating threads several
    clients can be served at the same time...

    This program has to be killed to terminate, or alternately it will abort in
    120 seconds on an alarm...
*/
pthread_mutex_t lock;

struct serverParm {
           int connectionDesc;
           char* log;
        };

void *serverThread(void *parmPtr) {

#define PARMPTR ((struct serverParm *) parmPtr)
    int recievedMsgLen;
    int sentMsgLen;
    char messageBuf[4096];
    char retBuf[4096];
    char hostname[32];
    char sig[32];
    char date[128];
    FILE *fp;
    FILE *tempfp;
    FILE *log;
    
    gethostname(hostname, 32);
    
    log = fopen(PARMPTR->log, "a");

    /* Server thread code to deal with message processing */
    printf("DEBUG: connection made, connectionDesc=%d\n",
            PARMPTR->connectionDesc);
    if (PARMPTR->connectionDesc < 0) {
        printf("Accept failed\n");
        return(0);    /* Exit thread */
    }
    
    read(PARMPTR->connectionDesc,sig,32); 
    
    /* Receive messages from sender... */
    while ((recievedMsgLen=
            read(PARMPTR->connectionDesc,messageBuf,sizeof(messageBuf)-1)) > 0) 
    {
        messageBuf[recievedMsgLen] = '\0';
        
        tempfp = popen("date", "r");
        sentMsgLen = fread(date, sizeof(char), 128, tempfp);
        date[sentMsgLen * sizeof(char)] = '\0';
        fclose(tempfp);
        
        pthread_mutex_lock(&lock);
        printf("ThreadID: %d\nServer Hostname: %s\nClient Hostname: %s\nTimestamp: %s\nMessage: %s\n", syscall(SYS_gettid), hostname, sig, date, messageBuf);
        fprintf(log, "ProcessID: %d\nThreadID: %d\nTimestamp: %s\nMessage: %s\n", getpid(), syscall(SYS_gettid), date, messageBuf);


        if(strcmp(messageBuf, "exit\n") == 0) {
            printf("DEBUG: connection ended with %d\n", PARMPTR->connectionDesc);
            pthread_mutex_unlock(&lock);
            close(PARMPTR->connectionDesc);  /* Avoid descriptor leaks */
            free(PARMPTR);                   /* And memory leaks */
            fclose(log);
            return(0);
        }

        fp = popen(messageBuf, "r");
        if (fp == NULL)
            printf("popen failed\n");

        sentMsgLen = fread(retBuf, sizeof(char), 2048, fp);
        fclose(fp);
        retBuf[sentMsgLen * sizeof(char)] = '\0';

        printf("Result: %s\n", retBuf);
        fprintf(log, "Result: %s\n", retBuf);
        fflush(log);
        pthread_mutex_unlock(&lock);
 
        if (write(PARMPTR->connectionDesc,retBuf,sizeof(retBuf)-1) < 0) {
               perror("Server: write error");
               return(0);
           }
    }
    fclose(log);
    close(PARMPTR->connectionDesc);  /* Avoid descriptor leaks */
    free(PARMPTR);                   /* And memory leaks */
    return(0);                       /* Exit thread */
}

main (int argc, char **argv) {
    int listenDesc;
    struct sockaddr_in myAddr;
    struct serverParm *parmPtr;
    int connectionDesc;
    pthread_t threadID;

    /* For testing purposes, make sure process will terminate eventually */
    alarm(120);  /* Terminate in 120 seconds */

    int port_num = atoi(argv[1]);
    pthread_mutex_init(&lock, NULL);

    /* Create socket from which to read */
    if ((listenDesc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("open error on socket");
        exit(1);
    }

    /* Create "name" of socket */
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    myAddr.sin_port = htons(port_num);
        
    if (bind(listenDesc, (struct sockaddr *) &myAddr, sizeof(myAddr)) < 0) {
        perror("bind error");
        exit(1);
    }

    /* Start accepting connections.... */
    /* Up to 5 requests for connections can be queued... */
    listen(listenDesc,5);

    while (1) /* Do forever */ {
        /* Wait for a client connection */
        connectionDesc = accept(listenDesc, NULL, NULL);

        /* Create a thread to actually handle this client */
        parmPtr = (struct serverParm *)malloc(sizeof(struct serverParm));
        parmPtr->connectionDesc = connectionDesc;
        parmPtr->log = argv[2];
        if (pthread_create(&threadID, NULL, serverThread, (void *)parmPtr) 
              != 0) {
            perror("Thread create error");
            close(connectionDesc);
            close(listenDesc);
            exit(1);
        } else {
            
        }

        printf("Parent ready for another connection\n");
    }
    
    pthread_mutex_destroy(&lock);
}
