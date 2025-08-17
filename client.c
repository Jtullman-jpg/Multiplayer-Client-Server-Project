/*
Jackson Tullman
I pledge my honor that I have abided by the Stevens Honor System
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define BIGNUMBER 1024
#define WRITE_END 1
#define READ_END 0

//parse connect
void parse_connect(int argc, char** argv, int* server_fd) {
    //ip/port default
    char *port = "25555";
    char *IP = "127.0.0.1";
    //parsing with get opt
    int opts;

    //parse options
    while ((opts = getopt(argc, argv, "i:p:h")) != -1) {
        //i flag
        if (opts == 'i') {
            IP = optarg;
        }
        //p flag
        else if (opts == 'p') {
            port = optarg;
        //h flag
        } else if (opts == 'h') {
            printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n", argv[0]);
            printf("  -i IP_address      Default to \"127.0.0.1\";\n");
            printf("  -p port_number     Default to 25555;\n");
            printf("  -h                 Display this help info.\n");
            exit(0);
        }
        //unknown flag
        else {
            //if unknown, using optind
            char *unknown = argv[optind - 1];
            //error message
            fprintf(stderr, "Error: Unknown option '%s' received.\n", unknown);
            exit(1);
        }
    }
    //converting port to int
    int portVal = atoi(port);
    //set up server address struct
    struct sockaddr_in server_addr;
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //clear memory of struct
    memset(&server_addr, 0, sizeof(server_addr));
    //set address family
    server_addr.sin_family = AF_INET;
    //port setup
    server_addr.sin_port = htons(portVal);
    //ip setup
    server_addr.sin_addr.s_addr = inet_addr(IP);
    //connect to server
    connect(*server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

}

//gameTime
void gameTime(int server_fd) {
    //buff to send/rec
    char buff[BIGNUMBER];
    //file descriptor for select
    fd_set myfdset;
    //largest fd tracker
    int larger;

    //getting larger fd
    //select needs to know from input and server
    if (server_fd > STDIN_FILENO) {
        larger = server_fd;
    } else {
        larger = STDIN_FILENO;
    }

    //main loop, multiplexed client
    while (1) {
        //using FD functions to clear
        //Clearing
        FD_ZERO(&myfdset);
        //Check for messages from server
        FD_SET(server_fd, &myfdset);
        //Check for messages from std input
        FD_SET(STDIN_FILENO, &myfdset);

        //wait for signal
        select(larger + 1, &myfdset, NULL, NULL, NULL);

        //if receive signal from server
        if(FD_ISSET(server_fd, &myfdset)) {
            //read from server
            int readBytes = recv(server_fd, buff, BIGNUMBER-1,0);
            //if server closed
            if (readBytes == 0) {
                //exit loop and end
                break;
            }
            //adding null terminator
            buff[readBytes] = '\0';
            //write to server
            printf("%s", buff);
        }

        //if user types something
        if (FD_ISSET(STDIN_FILENO, &myfdset)) {
            //gets user input, if end of file, break
            char* userInput = fgets(buff, sizeof(buff), stdin);
            if (userInput == NULL) {
                break;
            }
            //getting # of bytes to send
            int numBytes = strlen(buff);
            //sending if there's input
            if (numBytes>0) {
                //sending bytes
               send(server_fd, buff, numBytes, 0);
            }
        }
    }
    //close server once finished
    close(server_fd);
}
int main(int argc, char *argv[]) {
    //declare server file descriptor
    int server_fd;
    //call parse connect
    parse_connect(argc, argv, &server_fd);
    //game time activation
    gameTime(server_fd);
    //close server
    close(server_fd);
    return 0;
}