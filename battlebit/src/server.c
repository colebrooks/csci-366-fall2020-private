//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

static game_server *SERVER;

void init_server() {
    if (SERVER == NULL) {
        SERVER = calloc(1, sizeof(struct game_server));
    } else {
        printf("Server already started");
    }
}

int handle_client_connect(int player) {
    // STEP 9 - This is the big one: you will need to re-implement the REPL code from
    // the repl.c file, but with a twist: you need to make sure that a player only
    // fires when the game is initialized and it is there turn.  They can broadcast
    // a message whenever, but they can't just shoot unless it is their turn.
    //
    // The commands will obviously not take a player argument, as with the system level
    // REPL, but they will be similar: load, fire, etc.
    //
    // You must broadcast informative messages after each shot (HIT! or MISS!) and let
    // the player print out their current board state at any time.
    //
    // This function will end up looking a lot like repl_execute_command, except you will
    // be working against network sockets rather than standard out, and you will need
    // to coordinate turns via the game::status field.
    int client_socket_fd = SERVER->player_sockets[player];
    char raw_buffer[2000];
    char_buff *input_buffer = cb_create(2000);
    char_buff *output_buffer = cb_create(2000);

    int read_size;
    cb_append(output_buffer, "Welcome to the battleBit server Player ");
    cb_append_int(output_buffer, player);
    cb_append(output_buffer, "\nbattleBit (? for help) > ");
    cb_write(client_socket_fd, output_buffer);

    while ((read_size = recv(client_socket_fd, raw_buffer, 2000, 0)) > 0) {
        //reset our buffers
        cb_reset(output_buffer);
        cb_reset(input_buffer);
        if (read_size > 0) {
            raw_buffer[read_size] = '\0'; //null terminate read

            //append to input buffer
            cb_append(input_buffer, raw_buffer);

            //tokenize
            char *command = cb_tokenize(input_buffer, " \r\n");
            char *arg1 = cb_next_token(input_buffer);
            char *arg2 = cb_next_token(input_buffer);
            if (strcmp(command, "?") == 0) {
                //create output
                cb_append(output_buffer, "? - show help\n");
                cb_append(output_buffer, "load <string> - load a ship layout\n");
                cb_append(output_buffer, "show - shows the board\n");
                cb_append(output_buffer, "fire [0-7] [0-7] - fires at the given position\n");
                cb_append(output_buffer, "say <string> - Send the string to all players as part of a chat\n");
                cb_append(output_buffer, "exit");
                //output it
                cb_write(client_socket_fd, output_buffer);
            } else if (strcmp(command, "exit") == 0) {
                close(client_socket_fd);
            } else if (strcmp(command, "load") == 0) {
                game_load_board(game_get_current(), player, arg1);
                if (game_get_current()->status == CREATED) {
                    cb_append(output_buffer, "Waiting On Player 1");
                } else if (game_get_current()->status == PLAYER_0_TURN) {
                    cb_append(output_buffer, "All Player Boards Loaded\n");
                    cb_append(output_buffer, "Player 0 Turn");
                }
                cb_write(client_socket_fd, output_buffer);
            } else if (strcmp(command, "show") == 0) {
                repl_print_board(game_get_current(), player, output_buffer);
                cb_write(client_socket_fd, output_buffer);
            } else if (strcmp(command, "fire") == 0) {
                if (game_get_current()->status - 2 == player) {
                    cb_append(output_buffer, "\nPlayer ");
                    cb_append_int(output_buffer, player);
                    cb_append(output_buffer, " fires at ");
                    cb_append(output_buffer, arg1);
                    cb_append(output_buffer, " ");
                    cb_append(output_buffer, arg2);
                    cb_append(output_buffer, " - ");
                    if(game_fire(game_get_current(), player, atoi(arg1), atoi(arg2)) == 1) {
                       cb_append(output_buffer, "HIT");
                       if (game_get_current()->status - 4 == player) {
                           cb_append(output_buffer, " - PLAYER ");
                           cb_append_int(output_buffer, player);
                           cb_append(output_buffer, " WINS!");
                       }
                    } else {
                        cb_append(output_buffer, "MISS");
                    }
                    server_broadcast(output_buffer);
                    continue;
                } else if (game_get_current()->status == CREATED){
                    cb_append(output_buffer, "Game Has Not Begun!");
                } else if (game_get_current()->status - 2 == !player) {
                    switch (game_get_current()->status - 2) {
                        case 0:
                            cb_append(output_buffer, "Player 0 Turn");
                            break;
                        case 1:
                            cb_append(output_buffer, "Player 1 Turn");
                            break;
                    }
                }
                cb_write(client_socket_fd, output_buffer);
            } else if (strcmp(command, "say") == 0) {
                cb_append(output_buffer, "\nPlayer ");
                cb_append_int(output_buffer, player);
                cb_append(output_buffer, " says: ");
                cb_append(output_buffer, arg1);
                cb_append(output_buffer, " ");
                cb_append(output_buffer, arg2);
                char *arg3 = cb_next_token(input_buffer);
                while (arg3 != NULL) {
                    cb_append(output_buffer, " ");
                    cb_append(output_buffer, arg3);
                    arg3 = cb_next_token(input_buffer);
                }
                server_broadcast(output_buffer);
                continue;
            }
            cb_reset(output_buffer);
            cb_append(output_buffer, "\nbattleBit (? for help) > ");
            cb_write(client_socket_fd, output_buffer);
        }
    }
}

void server_broadcast(char_buff *msg) {
    // send message to all players
    cb_write(0, msg);
    cb_append(msg, "\nbattleBit (? for help) > ");
    cb_write(SERVER->player_sockets[0], msg);
    cb_write(SERVER->player_sockets[1], msg);
}

int run_server() {
    // STEP 8 - implement the server code to put this on the network.
    // Here you will need to initalize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously
    int server_socket_fd = socket(AF_INET,
                                  SOCK_STREAM,
                                  IPPROTO_TCP);
    if (server_socket_fd == -1) {
        printf("Could not create socket\n");
    }

    int yes = 1;
    setsockopt(server_socket_fd,
               SOL_SOCKET,
               SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server;

    // fill out the socket information
    server.sin_family = AF_INET;
    // bind the socket on all available interfaces
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9876);

    int request = 0;
    if (bind(server_socket_fd,
            // Again with the cast
             (struct sockaddr *) &server,
             sizeof(server)) < 0) {
        puts("Bind failed");
    } else {
        puts("Bind worked!");
        listen(server_socket_fd, 88);

        //Accept an incoming connection
        puts("Waiting for incoming connections...");

        struct sockaddr_in client;
        socklen_t size_from_connect;
        int client_socket_fd;
        int player = 0;

        while ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client, &size_from_connect)) > 0) {
            puts("Connection accepted");
            SERVER->player_sockets[player] = client_socket_fd;
            pthread_create(&SERVER->player_threads[player], NULL, (void *) handle_client_connect, (void *) player);
            player++;
            if (player > 1) {
                break;
            }
        }
    }
}

int server_start() {
    // STEP 7 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL
    init_server();
    pthread_create(&SERVER->server_thread, NULL, (void *) run_server, NULL);
}
