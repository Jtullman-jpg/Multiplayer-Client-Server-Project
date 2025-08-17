Multiplayer Trivia Game (Terminal-Based)

HOW TO RUN:

1. Open 3 terminal windows:
- One will act as the server.
- Two will act as clients.

SERVER:
Compile and run with:
- gcc -o server server.c
- ./server

CLIENTS:
Compile and run each client with:

- gcc -o client client.c
- ./client

Each client will be prompted to enter a player name.

GAME START:
- Both clients receive a multiple-choice trivia question with 3 options.
- Clients respond by typing 1, 2, or 3 and hitting enter.
- The first client to respond has their answer evaluated.
- The server shows the correct answer and moves to the next question.

GAME END:
- After all questions are answered, the server tallies the points.
- The winner is announced and congratulated.

RULES:
- Correct answer: +1 point
- Incorrect answer: -1 point
- Only the fastest answer per question is evaluated
