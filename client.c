#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int help_me_bool = 0;
    int ip_address_bool = 0;
    int port_number_bool = 0;
    char* ip_address;
    int port_number;
    int opt; 
    while ((opt = getopt(argc, argv, ":i:p:h")) != -1){
        switch (opt) {
            case 'i': 
                ip_address_bool++;
                if (ip_address_bool > 1){
                    fprintf(stderr, "Error: Too many flags");
                    exit(EXIT_FAILURE);
                }
                ip_address = optarg;
                break;
            case 'p':
                port_number_bool++;
                if (port_number_bool > 1){
                    fprintf(stderr, "Error: Too many flags");
                    exit(EXIT_FAILURE);
                }
                port_number = atoi(optarg); //atoi turns out string argument into a int
                break;
            case 'h':
                help_me_bool++;
                if (help_me_bool > 1){
                    fprintf(stderr, "Error: Too many flags");
                    exit(EXIT_FAILURE);
                }
                break;
            case ':':
                fprintf(stderr, "Error: Option '-%c' requires an argument.\n", optopt);
                exit(EXIT_FAILURE);
            case '?': 
                fprintf(stderr, "Error: Unknown option '-<%c>' received\n", optopt);
                //exit or what? 
                exit(EXIT_FAILURE);
        }

    }
    if (ip_address_bool == 0){
        ip_address = "127.0.0.1";
    }
    if (port_number_bool == 0){
        port_number = 25555;
    }
    if (help_me_bool == 1){
        printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n-i IP_address Default to '127.0.0.1';\n-p port_number Default to 25555;\n-h Display this help info.", argv[0]);
        exit(EXIT_SUCCESS);
    }
    char buffer[1024];
    int server_fd;
    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    if (connect(server_fd, (struct sockaddr *) &server_addr, addr_size) < 0) {
        perror("connect");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        fd_set active_fds; //Our active fds
        FD_ZERO(&active_fds); //Zero it out on every loop
        FD_SET(0, &active_fds); //Adding file decriptor std in to the set because that's our keyboard PG 160 TEXTBOOK
        FD_SET(server_fd, &active_fds); //We also add the server 
        int max_fd; //We need the biggest fd because select needs that for somereason to check through all of them, so if server is bigger then we can just max it 

        if (server_fd > 0) {
            max_fd = server_fd;
        } else {
            max_fd = 0;
        }
        int activity = select(max_fd + 1, &active_fds, NULL, NULL, NULL); //Call select 

        if (activity < 0) { //Error Check
            fprintf(stderr, "select error");
            break;
        }
        if (FD_ISSET(server_fd, &active_fds)) { //Server sent which we read 
            int bytes_read = read(server_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) { //If we don't read anything just disconnect 
                printf("Server disconnected.\n");
                break;
            }
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
            fflush(stdout); //in the txtbook we flush in this case to force the message to appear 
        }
        if (FD_ISSET(STDIN_FILENO, &active_fds)) {// User type in response 
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                break;
            }
            write(server_fd, buffer, strlen(buffer));
        }
    }

    close(server_fd);
    return 0;
}