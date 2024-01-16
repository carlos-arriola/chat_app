# chat_app
Establishes a connection between a server and multiple clients in order to facilitate real-time text communication

# CREDIT
Credit to this project goes to 'Idiot Developer' on YouTube for his excellent tutorial on socket programming in C with threads. My publication is merely my following of his tutorial available at https://www.youtube.com/watch?v=fNerEo6Lstw

Usage of this project is simple. First, a server must be initiated with a 4-digit port number on one terminal. Afterwards, up to five unique clients can then be initiated and connected to this server, using an identical port number. Each of these clients must be connected to the same local host as the server in order to function. Finally, once connected, each client can send messages which are then shared to both the server and every other client connected.

The word 'exit' can be entered by the client at any time in order to safely exit the application. CTRL+C can also be utilized as the project provides mechanisms that catch this action and exit safely regardless.
