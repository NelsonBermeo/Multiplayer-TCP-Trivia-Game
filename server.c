#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_CONN 3

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player {
    int fd;
    int score;
    char name[128];
};

void server_to_players_message(struct Player players[], char* msg) { //This message loops over the player arr and sends out a message to their fds
    for (int i = 0; i < MAX_CONN; i++) {
        if (players[i].fd != -1) {
            write(players[i].fd, msg, strlen(msg));
        }
    }
}

void server_to_player_question(struct Player players[], struct Entry* question, int question_num) {
    char q[1024]; //This message loops over the player arr and sends out a message to their fds with the question format. 
    snprintf(q, sizeof(q),
        "Question %d: %s\n"
        "1: %s\n"
        "2: %s\n"
        "3: %s\n",
        question_num,
        question->prompt,
        question->options[0],
        question->options[1],
        question->options[2]
    );

    server_to_players_message(players, q); //sends out the message using the previous function
}

void remove_newline(char* str) {
    str[strcspn(str, "\n")] = '\0';
    //strcspn finds the \n in the string and we turn that into a null terminator bascially removing it nice and sexily
}

int read_questions(struct Entry* arr, char* filename) {
    //arr is the array of question entries
    //filename is the path to question database 
    //This function returns the number of question entries 
    FILE* questions = fopen(filename, "r");

    if (questions == NULL) {
        fprintf(stderr, "Error: File could not be opened");
        //Should I exit or return? 
        exit(EXIT_FAILURE);
    }

    int count = 0; //Count serves as our entry index and the number of entries
    char line[1024];

    while (count < 50) {
        // Read prompt
        if (fgets(line, sizeof(line), questions) == NULL) {
            break; //That means we are done reading and we can close the file outside and end
        }

        remove_newline(line);
        // Skip blank lines between questions and stop reading 
        if (strlen(line) == 0) {
            continue;
        }
        strcpy(arr[count].prompt, line); //We can't just assign strings we have to strcpy stuff 

        // Read 3 options
        if (fgets(line, sizeof(line), questions) == NULL){
            //fclose(questions);
            break;
        }
        remove_newline(line);
        char *token;
        int idx = 0;
        token = strtok(line, " "); //We split the line based off of space 
        while (token != NULL){ //I could have done a forloop maybe but this was just the syntax I saw on google so why not just make it work
            strcpy(arr[count].options[idx], token);
            idx++;
            token = strtok(NULL, " ");
        }

        // Read correct answer
        if (fgets(line, sizeof(line), questions) == NULL) {
            //fclose(questions);
            break;
        }
        remove_newline(line);
        arr[count].answer_idx = 0;
        for (int j = 0; j < 3; j++) { //Loop through the options and find which one is the same as the answer with strcmp 
            if (strcmp(line, arr[count].options[j]) == 0) {
                arr[count].answer_idx = j + 1; //+1 so the player can choose 1 2 3 
                break;
            }
        }

        count++;
    }

    fclose(questions);
    return count;
}

int main(int argc, char *argv[]){

    int question_file_bool = 0;
    int help_me_bool = 0;
    int ip_address_bool = 0;
    int port_number_bool = 0;
    char* question_file;
    char* ip_address;
    int port_number;
    int opt; 
    while ((opt = getopt(argc, argv, ":f:i:p:h")) != -1){
        switch (opt) {
            case 'f':
                question_file_bool++;
                if (question_file_bool > 1){
                    fprintf(stderr, "Error: Too many flags\n");
                    exit(EXIT_FAILURE);
                }
                question_file = optarg; //optarg holds our argument
                break;
            case 'i': 
                ip_address_bool++;
                if (ip_address_bool > 1){
                    fprintf(stderr, "Error: Too many flags\n");
                    exit(EXIT_FAILURE);
                }
                ip_address = optarg;
                break;
            case 'p':
                port_number_bool++;
                if (port_number_bool > 1){
                    fprintf(stderr, "Error: Too many flags\n");
                    exit(EXIT_FAILURE);
                }
                port_number = atoi(optarg); //atoi turns out string argument into a int
                break;
            case 'h':
                help_me_bool++;
                if (help_me_bool > 1){
                    fprintf(stderr, "Error: Too many flags\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case ':':
                fprintf(stderr, "Error: Option '-%c' requires an argument.\n", optopt);
                exit(EXIT_FAILURE);
            case '?': 
                fprintf(stderr, "Error: Unknown option '-<%c>' received.\n", optopt);
                //exit or what? 
                exit(EXIT_FAILURE);
        }

    }
    if (question_file_bool == 0){ //These are going to be the default
        question_file = "qshort.txt";
    }
    if (ip_address_bool == 0){
        ip_address = "127.0.0.1";
    }
    if (port_number_bool == 0){
        port_number = 25555;
    }

    if (help_me_bool == 1){
        printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n-f question_file Default to 'qshort.txt';\n-i IP_address Default to '127.0.0.1';\n-p port_number Default to 25555;\n-h Display this help info.\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    //Let's read from the database 

    struct Entry questions[50];
    memset(&questions, 0, sizeof(questions));
    int num_questions = read_questions(questions, question_file); //Add that question file when it's time to test 
    if (num_questions <= 0) {
        fprintf(stderr, "Error: Could not read questions\n");
        exit(EXIT_FAILURE);
    }


    //Now let's initialize the server
    int server_fd; // This is for our socket()
    int client_fd = -1;
    char buffer[1024]; 
    struct sockaddr_in server_addr; 


    //Step 1: Create the socket! 

    server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    //Socket takes in a domain, type and protocol. 
    //Domain is an integer specifying the address family and protocol in our case we use AF_INET which is specifies our protocl family is IPV4 set which I think is TCP so like a connection orented model.
    //type is the socket type like sock_stream, sock_dgram, etc. We use SOCK_STREAM to provide "sequenced, reliable, two-way, connection-based byte streams." 
    //protocol is 0 to "let the system choose the proper one"
    memset(&server_addr, 0, sizeof(server_addr));
    //We have to "make sure all the data in the memory space the object occupies are zeros"
    server_addr.sin_family = AF_INET;
    //We initialize objects in our server_addr. Our sin_family is the domain and we use the same one we used for our socket 
    server_addr.sin_port = htons(port_number);
    //sin_port is the prt number we use 
    //We use htons to convert from big endian to little endian 
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    //Next we se up the IP address which is a attribute in the sockaddr_in struct but itself is a struct with one attribute called s_addr ; inet_addr turns our IP address into an integer value 

    //Step 2: Bind the socket 

    //The bind function takes the socket fd and address structure and binds them together to form the end of the socket 
    int r = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    //Bind takes our fd, our addr, and the size of it 
    if (r == -1) { perror("bind"); exit(-1); }
    //We error check it 

    //Step 3: Listen for clients 

    //So far we. have an active socket. One that can connect to others. So far we have the server which has a job: to listen for cleints. The server will call listen to tell the kernel it wants to listen for clients. 

    if (listen(server_fd, 3) == 0) printf("Welcome to 392 Trivia!\n");
    else { printf("Error\n"); return 1; }
    // We give listen the socket fd, and the max size of the queue which is 2 for now 

    //Alright, let's read the question database
    //The format is 
    //Question
    //3 possible answers (seperated by spaces)
    //correct answer
    //We can use a struct to store everything
    //Let's put the function above main

    //So that's done. 

    // int client_fds[MAX_CONN]; //Since select will clear out all our stuff, we make client_fds so we don't lose all our stuff and we initialize them all to be -1
    // for (int i = 0; i < MAX_CONN; i++) client_fds[i] = -1;

    //Instead of the above we will have players
    struct Player players[MAX_CONN];
    for (int i = 0; i < MAX_CONN; i++) {
        players[i].fd = -1; //No fds
        players[i].score = 0; //No score 
        memset(players[i].name, 0, sizeof(players[i].name)); //Clear up name space
    }

    fd_set active_fds; // Okay this is going to be our active fds which store our file descriptors from the client 
    
    int curr_client; //This is the accept return for the new client
    struct sockaddr_in client_addr; 
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int num_of_connetions = 0; 
    int game_start_bool = 0; //waiting for players 0, or game is running 1
    int current_question = 0;
    int accepting_answers_bool = 0; //Can the players type or not? 


    while (1) {
        // Build the fd_set from scratch each iteration
        // Part 1 : Preparation of the file descriptor set 
        // fd_set read_fds; //Maybe the fd_set is the active_fds from textbook? This monitors things
        FD_ZERO(&active_fds); //Let's clear out active fds so we have a fresh start each iteration
        FD_SET(server_fd, &active_fds); // Add our server to the active fds 
        int max_fd = server_fd; //I think we need this for select uhmm 

        //Here we have to loop through client_fds which are all our clients and if it isn't -1 we add it back to the set 
        // for (int i = 0; i < MAX_CONN; i++) {
        //     // if (client_fds[i] != -1) { now we are working with players
        //     if (players[i].fd != -1)
        //         FD_SET(client_fds[i], &active_fds);
        //         if (client_fds[i] > max_fd)
        //             max_fd = client_fds[i]; // Why do we need to know the largest fd? Does that like show order of which client goes next? 
        //     }
        // }

        for (int i = 0; i < MAX_CONN; i++) {
            if (players[i].fd != -1) {
                FD_SET(players[i].fd, &active_fds);
                if (players[i].fd > max_fd) {
                    max_fd = players[i].fd;
                }
            }
        }

        //Now we can use select? Part 2: Monitor FDSET 
        int activity = select(max_fd + 1, &active_fds, NULL, NULL, NULL);
        if (activity < 0) { perror("select"); break; }
        // The program will halt at this select 

        // Part 3: Accept New Connections 
        // We need to call accept() to get the new client's file descriptor 
        // FD_ISSET returns true if the fd arg 1 is a member of the set arg 2 
        if (FD_ISSET(server_fd, &active_fds)) { //If server is in active fds 
            curr_client = accept(server_fd, (struct sockaddr *) &client_addr, &addr_size);

            //So I think this is our client 
            if (num_of_connetions == MAX_CONN) {
                close(curr_client);
                fprintf(stderr, "Max connection reached!\n");
            } else {
                for (int i = 0; i < MAX_CONN; i++){
                    // if (client_fds[i] == -1){
                    //     client_fds[i] = curr_client;
                    //     break; //So we look through client fds and we find our first -1 and we throw our client in there? okay bet 
                    // } Instead of the above normal code from the textbook we use the following for players
                    if (players[i].fd == -1) {
                        players[i].fd = curr_client; //client fd
                        players[i].score = 0; //Just a reset
                        memset(players[i].name, 0, sizeof(players[i].name));//Reset name
                        write(curr_client, "Please type your name:\n", 23);
                        break;
                    }
                }
                printf("New Connection Detected!\n");
                num_of_connetions++;
            }
        }

        // Check each client for incoming data
        // Part 4: Close Connection
        //When a client process terminates, it will send an EOF to the server and the clients fd in the server becomes ready to read. ok ok ok . 
        //With this occuring, we need to figure out which process is terminating and if read() returns 0 we close that fd, remove it from active_fs and set it to -1 in client_fds

        // for (int i = 0; i < MAX_CONN; i++) {
        //     if (client_fds[i] > -1 && FD_ISSET(client_fds[i], &active_fds)) { 
        //         char buffer[1024]; 
        //         if (read(client_fds[i], buffer, 1024) == 0){
        //             close(client_fds[i]);
        //             FD_CLR(client_fds[i], &active_fds);
        //             client_fds[i] = -1;
        //             printf("LOST  CONNECTION\n");
        //             num_of_connetions--;
        //         }
        //     }
        // }  the textbook players time: 

        for (int i = 0; i < MAX_CONN; i++) { //Here is where we can read that the player is sending us 
            if (players[i].fd > -1 && FD_ISSET(players[i].fd, &active_fds)) { 
                char buffer[1024]; 
                int bytes_read = read(players[i].fd, buffer, 1024);
                
                if (bytes_read == 0){ //If the player sent nothing then bye bye
                    close(players[i].fd);
                    FD_CLR(players[i].fd, &active_fds); //Removes from set 
                    players[i].fd = -1;
                    players[i].score = 0;
                    memset(players[i].name, 0, sizeof(players[i].name));
                    printf("Lost connection!\n");
                    num_of_connetions--;
                    //Should I end the game and disconnet when a player disconnects? 
                } else { //I think here is where I can write to the players the questions 
                    // remove_newline(buffer); 
                    // strcpy(players[i].name, buffer);
                    // printf("Player %s joined the game\n", players[i].name);
                    buffer[bytes_read] = '\0'; //manually null-terminate a character after reading data into it
                    remove_newline(buffer);

                    if (strlen(players[i].name) == 0){ //First option is we have to read the players name  
                        strcpy(players[i].name, buffer);
                        printf("Hi %s!\n", players[i].name);                    
                        int players_ready_bool = 0;
                        for (int i = 0; i < MAX_CONN; i++) {
                            if (!(players[i].fd == -1) && !(strlen(players[i].name) == 0)) {
                                players_ready_bool++; //We do this 3 times to make sure we read both our player's names 
                            }
                        }

                        if(!game_start_bool && players_ready_bool == MAX_CONN){ //If we have the right amount of players and game is not started 
                            players_ready_bool = 0; //reset 
                            game_start_bool = 1;
                            current_question = 0;
                            accepting_answers_bool = 1;
                            // server_to_players_message(players, "The game is starting! \n"); This didn't work so I combined them
                            // server_to_player_question(players, &questions[current_question], current_question + 1);
                            char start_and_question[2048];
                            snprintf(start_and_question, sizeof(start_and_question),
                                "The game is starts now!\n\n"
                                "Question %d: %s\n"
                                "1: %s\n"
                                "2: %s\n"
                                "3: %s\n",
                                current_question + 1,
                                questions[current_question].prompt,
                                questions[current_question].options[0],
                                questions[current_question].options[1],
                                questions[current_question].options[2]
                            );

                            server_to_players_message(players, start_and_question);
                        }

                       
                    } else { //If we have players 
                        accepting_answers_bool = 0; // We are not accepting answers we just. got one 
                        char result_msg[1024];
                        int answer = atoi(buffer); //This is the answer 
                        if (answer == questions[current_question].answer_idx){ //correct 
                            players[i].score++;
                            snprintf(result_msg, sizeof(result_msg),
                                "\n%s answered and got it correct!\n"
                                "Correct answer: %s\n"
                                "Scores: %s = %d, %s = %d, %s = %d\n",
                                players[i].name,
                                questions[current_question].options[questions[current_question].answer_idx-1],
                                players[0].name,
                                players[0].score,
                                players[1].name,
                                players[1].score,
                                players[2].name,
                                players[2].score
                            );
                        } else {
                            players[i].score--;
                            snprintf(result_msg, sizeof(result_msg), //wrong
                                "\n%s answered but got it wrong.\n"
                                "Correct answer: %s\n"
                                "Scores: %s = %d, %s = %d, %s = %d\n",
                                players[i].name,
                                questions[current_question].options[questions[current_question].answer_idx-1],
                                players[0].name,
                                players[0].score,
                                players[1].name,
                                players[1].score,
                                players[2].name,
                                players[2].score
                            );
                        }

                        server_to_players_message(players, result_msg); //print the result 
                        current_question++;//update question

                        if (current_question >= num_questions) {
                            char final_msg[1024];
                            int winner = 0;
                            int tie = 0;
                            int max_score = INT_MIN;
                            for (int i = 0; i < MAX_CONN; i++){
                                if (players[i].score > max_score){
                                    max_score = players[i].score;
                                    winner = i;
                                }
                            }

                            for (int i = 0; i < MAX_CONN; i++){
                                if (players[i].score == max_score){
                                    tie++;
                                }
                            }

                            snprintf(final_msg, sizeof(final_msg),
                                "\nGame over!\n"
                                "Final scores:\n"
                                "%s: %d\n"
                                "%s: %d\n"
                                "%s: %d\n",
                                players[0].name,
                                players[0].score,
                                players[1].name,
                                players[1].score,
                                players[2].name,
                                players[2].score
                                );
                            //The concats are line by line, and it doesnt support variables at the end
                            if (tie > 1){
                                for (int i = 0; i < MAX_CONN; i++){
                                    if (players[i].score == max_score){
                                        strncat(final_msg, players[i].name, sizeof(final_msg) - strlen(final_msg) - 1);
                                        strncat(final_msg, "\n", sizeof(final_msg) - strlen(final_msg) - 1);
                                    }
                                }
                            } else {
                                strncat(final_msg, "Congrats, ", sizeof(final_msg) - strlen(final_msg) - 1);
                                strncat(final_msg, players[winner].name, sizeof(final_msg) - strlen(final_msg) - 1);
                                strncat(final_msg, "!\n", sizeof(final_msg) - strlen(final_msg) - 1);
                            }

                            server_to_players_message(players, final_msg);

                            close(players[0].fd);
                            close(players[1].fd);
                            close(players[2].fd);
                            close(server_fd);

                            return 0;

                        }

                        accepting_answers_bool = 1;//we will accpet answers again 

                        server_to_player_question(players, 
                                           &questions[current_question],
                                           current_question + 1);//give questions to playerss
                    }

                }
            }
        }
    }

    close(server_fd);
    return 0;
}



