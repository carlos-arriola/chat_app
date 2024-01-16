# Credit: As stated in the README, credit to this project goes to 'Idiot Developer' on YouTube for
# his excellent tutorial on socket programming in C with threads. My publication is merely my following of
# his tutorial available at https://www.youtube.com/watch?v=fNerEo6Lstw

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 5
#define BUFFER_SZ 2048
#define NAME_LEN 32

// Maintains a constant client count across all clients
static _Atomic unsigned int cli_count = 0;
static int uid = 10;

// Client structure
typedef struct {
        struct sockaddr_in address;     // Address information
        int sockfd;                     // Socket file descriptor
        int uid;                        // User ID
        char name[NAME_LEN];            // Client name
} client_t;

client_t *clients[MAX_CLIENTS];

// Initializing mutex to prevent race conditions
// between clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

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

// Enqueues a client into the messaging queue
void queue_add(client_t *cl) {
        pthread_mutex_lock(&clients_mutex);

        for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i]) {
                        clients[i] = cl;
                        break;
                }
        }
        pthread_mutex_unlock(&clients_mutex);
        }

// Dequeues a client from the messaging queue
void queue_remove(int uid) {
        pthread_mutex_lock(&clients_mutex);

        for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i]) {
                        if (clients[i] -> uid == uid) {
                                clients[i] = NULL;
                                break;
                        }
                }

        }
        pthread_mutex_unlock(&clients_mutex);
}


// Function for printing IP
// Used when a client connection is rejected
void print_ip_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// Send message to all clients EXCEPT the original sender
void send_message(char *s, int uid) {
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i]) {
                        if (clients[i]->uid != uid) {
                                if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
                                        printf("ERROR: write to descriptor failed\n");
                                        break;
                                }
                        }
                }

        }
        pthread_mutex_unlock(&clients_mutex);
}

// Manages each individual client
// Also manages the chat_history file
void *handle_client(void *arg) {
        FILE *fptr;
        char buffer[BUFFER_SZ];
        char name[NAME_LEN];
        int leave_flag = 0;     // Determines whether or not the client should be disconnected
  cli_count++;
  
  client_t *cli = (client_t *)arg; // Get client name

  fptr = fopen("chat_history", "w"); // Clear previous chat_history      
  fclose(fptr);
  fptr = fopen("chat_history", "a+");
    
  if (recv(cli->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN - 1) {
    printf("Enter the name correctly\n");
    leave_flag = 1; // Invalid name inputted, disconnect the client for them to try again
  } else {
    strcpy(cli->name, name);
    sprintf(buffer, "%s has joined\n", cli->name);
    fprintf(fptr, "%s has joined\n", cli->name);
    printf("%s", buffer);
    send_message(buffer, cli->uid); // Valid name, save name for this client and notify others
}
bzero(buffer, BUFFER_SZ);

while (1) {
  if (leave_flag) { // leave_flag responsible for disconnecting client
    break;
  }
  
  int receive = recv(cli->sockfd, buffer, BUFFER_SZ, 0);

  if (receive > 0) { // A message was received, print and save it
    if (strlen(buffer) > 0) {
      send_message(buffer, cli->uid);
      str_trim_lf(buffer, strlen(buffer));printf("%s\n", buffer);
      fprintf(fptr, "%s\n", buffer);
    }
  }
  else if (receive == 0 || strcmp(buffer, "exit")) { // A client has exited, release them
    sprintf(buffer, "%s has left\n", cli->name);
    fprintf(fptr, "%s has left\n", cli->name);
    printf("%s", buffer);
    send_message(buffer, cli->uid);
    leave_flag = 1;
  }
  else { // An error has occurred, recv returned -1
    printf("ERROR: -1\n");
    leave_flag = 1;
  }
  bzero(buffer, BUFFER_SZ);
}
  fclose(fptr);
  close(cli->sockfd);
  queue_remove(cli->uid);
  free(cli);
  cli_count--;
  pthread_detach(pthread_self()); // Cleanup after client disconnects
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  } // Verifies arguments have been passed

  char *ip = "127.0.0.1"; // Local host
  int port = atoi(argv[1]); // Port number specified in cmd line args
  int option = 1;
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr; // Server address information
  struct sockaddr_in cli_addr; // Client address information
  pthread_t tid;

  // Socket settings
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  // Signals
  signal(SIGPIPE, SIG_IGN);

  // Verify setsockopt returned a successful value
  if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0) {
    printf("ERROR: setsockopt\n");
    return EXIT_FAILURE;
  }
  
  // Binding, exit upon failure
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))< 0) {
    printf("ERROR: bind\n");
    return EXIT_FAILURE;
  }

  // Listen for a maximum of 5 clients, else exit upon failure
  if (listen(listenfd, MAX_CLIENTS) < 0) {
    printf("ERROR: listen\n");
    return EXIT_FAILURE;
  }

  printf("== Server Chatroom ==\n");

  while(1) {
    socklen_t clilen = sizeof(cli_addr);
    connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);


    // Check if max clients have been reached before connection
    if (cli_count + 1 == MAX_CLIENTS) {
      printf("Maximum clients connected. Connection rejected\n");
      print_ip_addr(cli_addr);
      close(connfd);
      continue;
    }
    
    // Client Settings
    client_t *cli = (client_t *) malloc(sizeof(client_t));
    cli->address = cli_addr;
    cli->sockfd = connfd;
    cli->uid = uid++;

    // Add client to queue
    queue_add(cli);
    pthread_create(&tid, NULL, &handle_client, (void*)cli);

    // Reduce CPU usage
    sleep(1);
  }
        return EXIT_SUCCESS;
}
