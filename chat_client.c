# DISCLAIMER: As stated in the README, credit to this project goes to 'Idiot Developer' on YouTube for
# his excellent tutorial on socket programming in C with threads. My publication is merely my following of
# his tutorial available at https://www.youtube.com/watch?v=fNerEo6Lstw

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 5
#define BUFFER_SZ 2048
#define NAME_LEN 32

// Constant flag between all clients
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];

// Overwrites and flushes standard output
void str_overwrite_stdout() {
        printf("\r%s", "> ");
        fflush(stdout);
}

// Trims trailing newline characters
void str_trim_lf(char *arr, int length) {
          for (int i = 0; i < length; i++) {
                if (arr[i] == '\n')
                {
                        arr[i] = '\0';
                        break;
                }
        }
}

// Function catches a CTRL+C for a safe exit
void catch_ctrl_c_and_exit() {
        flag = 1;
}

// Manages incoming messages, prints until nothing else is available
void recv_msg_handler() {
        char message[BUFFER_SZ] = {};
        while (1) {
                int receive = recv(sockfd, message, BUFFER_SZ, 0);

                if (receive > 0) {
                        printf("%s ", message);
                       str_overwrite_stdout();
                } else if (receive == 0) {
                        break;
                }
                bzero(message, BUFFER_SZ);
        }
}

// Manages messages sent from this client
// Also where an exit can be made
void send_msg_handler() {
        char buffer[BUFFER_SZ] = {};
        char message[BUFFER_SZ + NAME_LEN + 2] = {};

        while (1) {
                str_overwrite_stdout(); // Flush standard output
                fgets(buffer, BUFFER_SZ, stdin); // Store buffer from standard input
                str_trim_lf(buffer, BUFFER_SZ); // Trim newline

                if (strcmp(buffer, "exit") == 0) {
                        break;
                } else {
                        sprintf(message, "%s: %s\n", name, buffer); // Store name and buffer into message
                        send(sockfd, message, strlen(message), 0); // Send message to server
                }
                bzero(buffer, BUFFER_SZ);
                bzero(message, BUFFER_SZ + NAME_LEN);
        }
        catch_ctrl_c_and_exit(2);
}

int main(int argc, char **argv){
        if(argc != 2) { // Verifies arguments have been passed
                printf("Usage: %s <port>\n", argv[0]);
                return EXIT_FAILURE;
        }

        char *ip = "127.0.0.1"; // Local host
        int port = atoi(argv[1]); // Use port passed as argument

        signal(SIGINT, catch_ctrl_c_and_exit); // Signal
        printf("Enter your name: ");
        fgets(name, NAME_LEN, stdin);
        str_trim_lf(name, strlen(name));

        if (strlen(name) > NAME_LEN - 1 || strlen(name) < 2) {
                printf("Enter name correctly [Between 2-32 Characters]\n");
                return EXIT_FAILURE;
        } // Read name, trim newline, and verify acceptable length

        struct sockaddr_in server_addr;
        // Socket settings
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(ip);
        server_addr.sin_port = htons(port);

        // Connect to the server. If server on same port number not running,
        // signal an exit failure
        int err = connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
        if (err == -1) {
                printf("ERROR: connect\n");
                return EXIT_FAILURE;
        }

        // Send the user name to the server
 send(sockfd, name, NAME_LEN, 0);
        printf("=== Client Chatroom ===\nEnter 'exit' to exit\n");

        pthread_t send_msg_thread; // Declare a thread for sending messages from this client
        if (pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0) {
                printf("ERROR: pthread\n");
                return EXIT_FAILURE;
        } // If send thread creation fails, exit and signal failure

        pthread_t recv_msg_thread; // Declare a separate thread for reading messages from the server
        if (pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0) {
                printf("ERROR: pthread\n");
                return EXIT_FAILURE;
        } // If receive thread creation fails, exit and signal failure

        while (1) {
                if (flag) { // If the exit flag is ever set, exit the program
                        printf("\nExiting...\n");
                        break;
                }
        }
        close(sockfd); // Cleanup after socket connection, and close safely
        return EXIT_SUCCESS;
}
