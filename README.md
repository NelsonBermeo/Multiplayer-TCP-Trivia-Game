# Multiplayer TCP Trivia Game Project

This is a simple multiplayer trivia game built in C using TCP sockets. The project uses a client-server model where multiple players connect to a server, enter their names, and compete by answering trivia questions in real time. The server starts by listening on a given IP address and port. Players connect as clients and enter their names. Once all players have joined, the server sends trivia questions to everyone from a local txt file. Players answer by selecting one of the numbered choices. Correct answers increase a player's score, while incorrect answers decrease it. The winner/tie is revealed to the players and ends the game once the questions have been gone through. 

Txtfile format: 

What planet is known as the Red Planet?
Mars Venus Jupiter
Mars

What is the largest ocean on Earth?
Pacific Atlantic Indian
Pacific
...

Makefile: 

Compiles both files. 

How to Play: 

1. Make files
2. Run the server with the command line options:
-f question_file   Default: qshort.txt
-i IP_address      Default: 127.0.0.1
-p port_number     Default: 25555
-h                 Display help information
3. Run the client with the same command line options:
4. Once 3 clients are connected they type their names and the game automatically starts

Technologies: 

* C
* TCP sockets
* select()
* File I/O
* Client-server networking



