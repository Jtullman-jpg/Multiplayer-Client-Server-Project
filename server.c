/*Jackson Tullman
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
#define QUESTIONSIZE 50
#define PLAYERAMOUNT 2

//Player Struct
struct Player {
    int fd;
    int score;
    char name[128]; };

//Entry Struct
struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

//reading questions
int read_questions(struct Entry* arr, char* filename) {
    //opening file
    FILE* file = fopen(filename, "r");
    //if file open fails
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    //counter and buffer
    char buff[BIGNUMBER];
    int i = 0;

    //begin reading by 4 lines each question
    while (fgets(buff, BIGNUMBER, file)) {
        //Prompt reading into current entry Line 1
        strcpy(arr[i].prompt, buff);
        //Options line reading Line 2
        fgets(buff, BIGNUMBER, file);
        //index for buffer
        int pos = 0;
        //splitting buffer by spaces
        for (int j = 0; j < 3; j++) {
            //indexing
            int k = 0;
            //copying characters into options until hit space, null terminator, or new line
            while (buff[pos] != ' ' && buff[pos] != '\0' && buff[pos] != '\n') {
                arr[i].options[j][k] = buff[pos];
                k++;
                pos++;
            }
            //applying null terminator
            arr[i].options[j][k] = '\0';
            //move past space between opts
            if (buff[pos] == ' ') {
                pos++;
            }

        }
        //Correct answer reading Line 3
        fgets(buff, BIGNUMBER, file);

        //stripping new line to get full correct answer, wrong answer if not
        if (buff[strlen(buff) - 1] == '\n') {
            buff[strlen(buff) - 1] = '\0';
        }

        //comparing to find correct answer's index
        for (int j = 0; j<3; j++) {
            if (strcmp(arr[i].options[j], buff) == 0) {
                //store answer indexes
                arr[i].answer_idx = j;
                break;
            }
        }

        //Skipping blank space Line 4
        fgets(buff, BIGNUMBER, file);
        //next Question
        i++;
    }
    //closing file and returning question amount
    fclose(file);
    //return # of questions
    return i;
}

//Accepting players
int acceptPlayers(int serverFd, struct Player players[]) {
    //initalize client fds to -1 or not in use
    int clientFd[PLAYERAMOUNT];
    for (int i = 0; i < PLAYERAMOUNT; i++) {
        clientFd[i] = -1;
    }

    //variables needed
    //count connections of those who have entered a name
    int nconn = 0;
    //set fd for select()
    fd_set myfdSet;
    //address incoming
    struct sockaddr_in inAddr;
    socklen_t addrLen = sizeof(inAddr);
    //buffer for reading
    char buff[BIGNUMBER];

    //main loop
    while (1) {

        //clear the set
        FD_ZERO(&myfdSet);
        //watch the socket for new connections
        FD_SET(serverFd, &myfdSet);
        //initialize largest fd, to be found later
        int largestFd = serverFd;

        //add client sockets to list
        for (int i = 0; i < PLAYERAMOUNT; i++) {
            //check if file descriptor in use
            if (clientFd[i] > -1) {
                //add client fd to working set
                FD_SET(clientFd[i], &myfdSet);
                //update largest fd for select()
                if (clientFd[i] > largestFd) {
                    largestFd = clientFd[i];
                }
            }
        }

        //wait for socket to be ready
        select(largestFd + 1, &myfdSet, NULL, NULL, NULL);

        //check if client is ready
        if (FD_ISSET(serverFd, &myfdSet)) {
            //accept incoming client connection, and store info
            int inFD = accept(serverFd, (struct sockaddr*)&inAddr, &addrLen);

            //Handle max connection
            //If max connections reached
            if(nconn == PLAYERAMOUNT) {
                close(inFD);
                fprintf(stderr, "Max connection reached!\n");
            }
            //otherwise store new socket
            //and ask for name
            else {
                for (int i = 0; i < PLAYERAMOUNT; i++) {
                    //is position is unused add socket to fds
                    if (clientFd[i] == -1) {
                        //add fd
                        clientFd[i] = inFD;
                        //increment connections
                        nconn++;
                        break;
                    }
                }

                //new connection messages
                printf("New connection accepted\n");
                //ask for name
                send(inFD, "Please type your name:\n", 24, 0);
            }
        }
        //handling input of name
        //looping through players
        for (int i = 0; i < PLAYERAMOUNT; i++) {
            //only read if client is ready and the spot is open
            if (clientFd[i] > -1 && FD_ISSET(clientFd[i], &myfdSet)){
                int readBytes = recv(clientFd[i], buff, BIGNUMBER-1, 0);
                //if connection lost by client
                if (readBytes <= 0) {
                    //close client
                    close(clientFd[i]);
                    //clear
                    FD_CLR(clientFd[i], &myfdSet);
                    //set space to unused
                    clientFd[i] = -1;
                    //decrement counter
                    nconn--;
                    //print lost connection
                    printf("Lost connection!\n");
                    //continue with reading other connections
                    continue;
                }
                //read normal
                //adding null terminator at end
                buff[readBytes] = '\0';
                //checking if character prior is new line, if so make null terminator
                if (buff[readBytes - 1] == '\n') {
                    buff[readBytes - 1] = '\0';
                }

                //adding player information to player struct
                players[i].fd = clientFd[i];
                players[i].score = 0;
                strcpy(players[i].name, buff);
                printf("Hi %s!\n", players[i].name);

                // check if all players are ready
                int names = 0;
                for (int v = 0; v < PLAYERAMOUNT; v++) {
                    if (players[v].fd > 0) {
                        names++;
                    }
                }
                if (names == PLAYERAMOUNT) {
                    printf("The game starts now!\n");
                    return PLAYERAMOUNT;
                }

                // only clear after checking
                clientFd[i] = -1;
            }
        }
    }
}

//playing game
void play(struct Entry* questions, struct Player* players, int questCount, int playerCount) {
    //buffer to hold messages sent
    char buff[BIGNUMBER];

    //main loop
    for (int i = 0; i < questCount; i++) {
        //print questions and options to server
        printf("Question %d: %s", i+1, questions[i].prompt);
        printf("1: %s\n", questions[i].options[0]);
        printf("2: %s\n", questions[i].options[1]);
        printf("3: %s\n", questions[i].options[2]);

        //printing questions to clients
        for (int j = 0; j < playerCount; j++) {
            //sending question
            sprintf(buff, "Question %d: %s", i+1, questions[i].prompt);
            send(players[j].fd, buff, strlen(buff), 0);
            //sending options
            sprintf(buff, "Press 1: %s\nPress 2: %s\nPress 3: %s\n",questions[i].options[0],questions[i].options[1],questions[i].options[2]);
            send(players[j].fd, buff, strlen(buff), 0);

        }

        //waiting for player
        //has answered check variable
        int hasAnswered = 0;
        //wait for player to answer
        while(!hasAnswered) {
            //fixing player 0 priority bug
            fd_set myfdset;
            FD_ZERO(&myfdset);
            //tracking larger file descriptor for select()
            int largestFD = -1;

            //add sockets to set while also obtaining largest fd
            for (int g = 0; g < playerCount; g++) {
                FD_SET(players[g].fd, &myfdset);
                if (players[g].fd > largestFD) {
                    //update largest
                    largestFD = players[g].fd;
                }
            }

            //wait for socket to be ready
            select(largestFD + 1, &myfdset, NULL, NULL, NULL);

            //check for ready sockets
            for(int p = 0; p < playerCount; p++) {
                //check if ready
                if (FD_ISSET(players[p].fd, &myfdset)) {
                    //read data from socket
                    int readBytes = recv(players[p].fd, buff, BIGNUMBER-1,0);
                    //if data read
                    if (readBytes > 0) {
                        //null terminate string
                        buff[readBytes] = '\0';
                        //convert the input to an int
                        int pick = atoi(buff);
                        //compare pick with answer
                        if(pick == questions[i].answer_idx + 1) {
                            //if right increment score
                            players[p].score++;
                        }
                        else {
                            //if wrong decrement score
                            players[p].score--;
                        }
                        //set the player having answered to true
                        hasAnswered = 1;
                        //break out of loop
                        break;
                }
            }
        }
    }
        //print correct answer to server
        printf("Correct answer: %s\n", questions[i].options[questions[i].answer_idx]);
        //send correct answer to clients
        for (int w = 0; w < playerCount; w++) {
            //get correct asnwer
            sprintf(buff, "Correct answer: %s\n", questions[i].options[questions[i].answer_idx]);
            //send correct answer
            send(players[w].fd, buff, strlen(buff),0);
        }
    }
    //find winner score
    //starting with first player
    int winner = players[0].score;
    for(int i = 1; i<playerCount; i++) {
        //find the greatest score
        if (players[i].score > winner) {
            winner = players[i].score;
        }
    }

    //print winner or winners if same score, could be a tie
    for (int j = 0; j < playerCount; j++) {
        //if players score matches winning score
        if (players[j].score == winner) {
            //display congrats
            printf("Congrats, %s!\n", players[j].name);
        }
        //close socket for all players
        close(players[j].fd);
    }

}

int main(int argc, char *argv[]) {
    //getopts

    //defaults for flags
    char *qfile = "questions.txt";
    char *IP = "127.0.0.1";
    char *port = "25555";

    //opts loop
    int opts;
    while((opts = getopt(argc, argv, "f:i:p:h")) != -1) {
        //-f flag
        if (opts == 'f') {
            qfile = optarg;
        }
        //-i flag
        else if (opts == 'i') {
            IP = optarg;
        }
        //-p flag
        else if (opts == 'p') {
            port = optarg;
        }
        //-h flag
        else if (opts == 'h') {
            printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n", argv[0]);
            printf("  -f question_file   Default to \"qshort.txt\";\n");
            printf("  -i IP_address      Default to \"127.0.0.1\";\n");
            printf("  -p port_number     Default to 25555;\n");
            printf("  -h                 Display this help info.");
            exit(0);
        }
        //unknown flag
        else {
            char *unknown = argv[optind - 1];
            fprintf(stderr, "Error: Unknown option '%s' received.\n", unknown);
            exit(1);
        }
    }

    //beginning to set up socket
    //vars
    int server_fd;
    int client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);
    //convert port
    int portVal = atoi(port);
    //set up
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    //port setup
    server_addr.sin_port = htons(portVal);
    //ip setup
    server_addr.sin_addr.s_addr = inet_addr(IP);
    //binding
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));
    bind(server_fd, (struct sockaddr *) &server_addr,sizeof(server_addr));
    //listening
    listen(server_fd,2);
    //welcome message
    printf("Welcome to 392 Trivia!\n");

    //begin main sequence
    //get questions
    struct Entry questions[QUESTIONSIZE];
    int qAmount = read_questions(questions, qfile);
    //players
    struct Player players[PLAYERAMOUNT];
    int pAmount = acceptPlayers(server_fd, players);
    //start game
    play(questions, players, qAmount, pAmount);

    return 0;
}