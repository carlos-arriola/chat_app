server: chat_server.c
        gcc -o server -std=c99 -Wall -pthread -O1 chat_server.c
client: chat_client.c
        gcc -o client -std=c99 -Wall -pthread -O1 chat_client.c
clean:
        rm client
        rm server
