#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include "socket.h"
#include "gameplay.h"


#ifndef PORT
    #define PORT 50001
#endif
#define MAX_QUEUE 5


void add_player(struct client **top, int fd, struct in_addr addr);
void add_player_to_game(struct client **top, int fd, struct in_addr addr, char *name);
void add_player_to_game_first(struct client **turn, struct client **top, int fd, struct in_addr addr, char *name);
int check_name(struct client **game_head, char *name);
int check_guess(char *guess);
void remove_player(struct client **top, int fd);
void remove_new_player(struct client **top, int fd);

/* These are some of the function prototypes that we used in our solution
 * You are not required to write functions that match these prototypes, but
 * you may find the helpful when thinking about operations in your program.
 */
/* Send the message in outbuf to all clients */
void broadcast(struct game_state *game, char *outbuf);
void announce_turn(struct game_state *game);
void announce_winner(struct game_state *game, struct client *winner);
/* Move the has_next_turn pointer to the next active client */
void advance_turn(struct game_state *game);


/* The set of socket descriptors for select to monitor.
 * This is a global variable because we need to remove socket descriptors
 * from allset when a write to a socket fails.
 */
fd_set allset;

/* Add a client to the head of the linked list
 */
void add_player(struct client **top, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));

    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));

    p->fd = fd;
    p->ipaddr = addr;
    p->name[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *top;
    *top = p;
}
/*
Add a new player to the game that has a player in it already.
*/
 void add_player_to_game(struct client **top, int fd, struct in_addr addr, char *name) {
     struct client *p = malloc(sizeof(struct client));

     if (!p) {
         perror("malloc");
         exit(1);     }
     p->fd = fd;
     p->ipaddr = addr;
     strcpy(p->name, name);
     p->in_ptr = p->inbuf;
     p->inbuf[0] = '\0';
     p->next = *top;
     *top = p;
 }
 /*
 Add a new player to the game that has no players in it.
 */
void add_player_to_game_first(struct client **turn, struct client **top, int fd, struct in_addr addr, char *name) {
    struct client *p = malloc(sizeof(struct client));

    if (!p) {
        perror("malloc");
        exit(1);
    }
    p->fd = fd;
    p->ipaddr = addr;
    strcpy(p->name, name);
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *top;
    *top = p;
    *turn = p;
}
/*
Check if the name is already taken.
*/
int check_name(struct client **game_head, char *name){
  struct client **p;
  for (p = game_head; *p && strcmp((*p)->name, name) != 0; p = &(*p)->next)
      ;
  if (*p) {
    return 1;
  }
  return 0;

}
/*
Check if the guess is in fact a letter
*/
int check_guess(char *guess){
  if (strlen(guess) == 1){
    if ((*guess) >= 97 && (*guess) <= 122){
      return ((*guess)-97);
    }
  }

  return -1;

}
/* Removes client from the linked list and closes its socket.
 * Also removes socket descriptor from allset
 */
void remove_player(struct client **top, int fd) {
    struct client **p;

    for (p = top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
}
/* Removes client from the linked list newplayers
 *
 */
void remove_new_player(struct client **top, int fd) {
    struct client **p;

    for (p = top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        struct client *t = (*p)->next;
        //printf("Removing new_player %d %s\n", fd, inet_ntoa((*p)->ipaddr));

        //ree(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
}
/* Find the new line in read
 *
 */
int find_network_newline(char *buf, int n) {
    for (int i = 0; i < n; i++){
      if (*(buf+i) == '\r'){
        return (i);
      }
    }
    return -1;
}
/* Broadcast the outbuf to all players
 *
 */
void broadcast(struct game_state *game, char *outbuf){
    struct client *p;
    for(p = game->head; p != NULL; p = p->next){
        if(write(p->fd, outbuf, strlen(outbuf)) == -1) {
            fprintf(stderr, "Write to client %s failed\n", inet_ntoa(p->ipaddr));
            remove_player(&game->head, p->fd);
        }
    }
}
/* Announce the turns for each player, if it is the player's turn, say "Your guess?"
 * otherwise say "It's *someone else*'s turn"
 *
 */
void announce_turn(struct game_state *game){
    struct client *p;
    char buf[MAX_MSG];
    for(p = game->head; p != NULL; p = p->next){
      if (p == game->has_curr_turn){
        if(write(p->fd, ANNOUNCE_TURN, strlen(ANNOUNCE_TURN)) == -1) {
            fprintf(stderr, "Write to client %s failed\n", inet_ntoa(p->ipaddr));
            remove_player(&game->head, p->fd);
        }
      }
      else {
        strcpy(buf, "It's ");
        strcat(buf, game->has_curr_turn->name);
        strcat(buf, "'s turn.\n");
        write(p->fd, buf, strlen(buf));

      }
      printf("It's %s's turn.\n", game->has_curr_turn->name);
  }
}

// Advance the turns in the game
void advance_turn(struct game_state *game){
    if (game->has_curr_turn->next == NULL){
        game->has_curr_turn = game->head;
        game->has_next_turn = game->has_curr_turn->next;
    }
    else{
      game->has_curr_turn = game->has_next_turn;
      game->has_next_turn = game->has_curr_turn->next;
    }
}

// Announce the winner of the game
void announce_winner(struct game_state *game, struct client *winner){
    struct client *p;
    for(p = game->head; p != NULL; p = p->next){
        if (p == winner){
            if(write(p->fd, "Game over! You win!\n", 20) == -1) {
                fprintf(stderr, "Write to client %s failed\n", inet_ntoa(p->ipaddr));
                remove_player(&game->head, p->fd);
            }
        }
        else{
            char buf[MAX_NAME + 16];
            strcat(buf, "Game over! ");
            strcat(buf, winner->name);
            strcat(buf, " won!\n");
            if(write(p->fd, buf, strlen(buf)) == -1) {
                fprintf(stderr, "Write to client %s failed\n", inet_ntoa(p->ipaddr));
                remove_player(&game->head, p->fd);
            }
        }
    }
}



int main(int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    if(argc != 2){
        fprintf(stderr,"Usage: %s <dictionary filename>\n", argv[0]);
        exit(1);
    }

    // Create and initialize the game state
    struct game_state game;

    srandom((unsigned int)time(NULL));
    // Set up the file pointer outside of init_game because we want to
    // just rewind the file when we need to pick a new word
    game.dict.fp = NULL;
    game.dict.size = get_file_length(argv[1]);

    init_game(&game, argv[1]);

    // head and has_next_turn also don't change when a subsequent game is
    // started so we initialize them here.
    game.head = NULL;
    game.has_curr_turn = NULL;
    game.has_next_turn = NULL;

    /* A list of client who have not yet entered their name.  This list is
     * kept separate from the list of active players in the game, because
     * until the new playrs have entered a name, they should not have a turn
     * or receive broadcast messages.  In other words, they can't play until
     * they have a name.
     */
    struct client *new_players = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, MAX_QUEUE);
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    // initialize allset and add listenfd to the
    // set of file descriptors passed into select
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    // maxfd identifies how far into the set to search
    maxfd = listenfd;
    int player_added = 0;
    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            continue;
        }

        if (FD_ISSET(listenfd, &rset)){
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_player(&new_players, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if(write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_player(&new_players, clientfd);
            };
        }

        /* Check which other socket descriptors have something ready to read.
         * The reason we iterate over the rset descriptors at the top level and
         * search through the two lists of clients each time is that it is
         * possible that a client will be removed in the middle of one of the
         * operations. This is also why we call break after handling the input.
         * If a client has been removed the loop variables may not longer be
         * valid.
         */
        int cur_fd;

        for(cur_fd = 0; cur_fd <= maxfd; cur_fd++) {

            if(FD_ISSET(cur_fd, &rset)) {
                // Check if this socket descriptor is an active player
                for(p = game.head; p != NULL; p = p->next) {
                    if(cur_fd == p->fd) {
                      char guess[2];
                      char buf[MAX_MSG];
                      char incorrect[MAX_MSG];
                      int nbytes = 0;
                      int where = -1;
                      strcpy(incorrect, "  is not in the word.\n");

                      // Read input from active player
                      if ((nbytes = read(cur_fd, p->in_ptr, MAX_BUF)) == 0){
                        char broadcast_join[(MAX_NAME + 19)] = {'\0'};
                        strcat(broadcast_join, "Goodbye ");
                        strcat(broadcast_join, p->name);
                        strcat(broadcast_join, "\n");
                        remove_player(&game.head, cur_fd);
                        broadcast(&game, broadcast_join);
                        printf("%s\n", broadcast_join);


                      }

                      // Find the newline
                      if ((where = find_network_newline(p->in_ptr, MAX_BUF)) >= 0) {
                              printf("[%d] Read %d bytes\n", cur_fd, nbytes);
                              p->in_ptr = p->in_ptr + where;
                              *p->in_ptr = '\0';
                              printf("Found newline %s\n", p->inbuf);
                      }
                      // If no newline, i.e. partial read, then go back and read more
                      else if (where == -1){
                              //printf("%d\n", where);
                              printf("[%d] Read %d bytes\n", cur_fd, nbytes);
                              p->in_ptr = p->in_ptr + nbytes;
                              break;
                      }
                      // If player is the current player
                      if (p == game.has_curr_turn){
                          int checker = check_guess(p->inbuf);
                          // Check if it is a valid guess
                          if (checker == -1 || strlen(p->inbuf)  == 0||
                            (checker >= 0 && game.letters_guessed[checker] == 1)) {
                              p->in_ptr = &p->inbuf[0];
                              announce_turn(&game);
                              break;
                          }
                          // If it is a valid guess,
                          else{
                              strcpy(guess, p->inbuf);
                              p->in_ptr = &p->inbuf[0];
                              game.letters_guessed[checker] = 1;
                              int correct = 0;
                              for (int i = 0; i < MAX_WORD; i++){
                                  if (game.word[i] == guess[0]){
                                      game.guess[i] = guess[0];
                                      correct = 1;
                                  }
                              }

                              // Print the game status
                              broadcast(&game, status_message(buf, &game));
                              int winner = 1;
                              for (int i = 0; i < MAX_WORD; i++){
                                  if (game.guess[i] == '-'){
                                     winner = 0;
                                  }
                              }

                              // If the guess is incorrect...
                              if (correct == 0){
                                  incorrect[0] = guess[0];
                                  if(write(p->fd, incorrect, strlen(incorrect)) == -1) {
                                      fprintf(stderr, "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                                      remove_player(&new_players, clientfd);
                                  }
                                  advance_turn(&game);
                                  game.guesses_left -= 1;
                              }
                              // If the game is over...
                              if (game.guesses_left == 0 || winner == 1){
                                  if (winner == 1){
                                    announce_winner(&game, p);
                                  }
                                  else if (game.guesses_left == 0){
                                    broadcast(&game, "No guesses left. Game over.\n");
                                  }
                                  init_game(&game, argv[1]);
                                  broadcast(&game, status_message(buf, &game));
                              }

                              // Announce next turn regardless
                              announce_turn(&game);
                              break;
                          }
                    }
                    // For players that do not have their turn,
                    // say it's not their turn
                    else {
                        if(write(cur_fd, NOT_YOUR_TURN, strlen(NOT_YOUR_TURN)) == -1) {
                            fprintf(stderr, "Write to client %s failed\n", inet_ntoa(p->ipaddr));
                            remove_player(&game.head, cur_fd);
                        };
                        p->in_ptr = &p->inbuf[0];
                        break;
                    }
                }
            }
                // Check if any new players are entering their names
                for(p = new_players; p != NULL; p = p->next) {
                    if(cur_fd == p->fd) {
                        int nbytes = 0;
                        int where = -1;
                        // Read input
                        if ((nbytes = read(cur_fd, p->in_ptr, MAX_BUF)) == 0){
                          remove_new_player(&new_players, cur_fd);
                        }
                        // Find the newline
                        if ((where = find_network_newline(p->in_ptr, MAX_BUF)) >= 0) {
                                printf("[%d] Read %d bytes\n", cur_fd, nbytes);
                                p->in_ptr = p->in_ptr + where;
                                *p->in_ptr = '\0';
                                printf("Found newline %s\n", p->inbuf);
                        }
                        // If no newline, i.e. partial read, then go back and read more
                        else if (where == -1){
                          //printf("%d\n", where);
                          printf("[%d] Read %d bytes\n", cur_fd, nbytes);
                          p->in_ptr = p->in_ptr + nbytes;
                          break;
                        }
                        // Check if name is not empty or already exists
                        int checker = check_name(&game.head, p->inbuf);
                        if (checker == 1 || strlen(p->inbuf) == 0){
                          p->in_ptr = &p->inbuf[0];
                          write(cur_fd, INVALID_NAME, strlen(INVALID_NAME));
                          break;
                        }
                        //Set up broadcast to the players that new player has joined
                        char broadcast_join[(MAX_NAME + 19)] = {'\0'};
                        strcat(broadcast_join, p->inbuf);
                        strcat(broadcast_join, " has just joined.\n");
                        //Copy the name from inbuf to name
                        strncpy(p->name, p->inbuf, MAX_NAME);
                        // Add player to game
                        if (game.head == NULL){
                          add_player_to_game_first(&game.has_curr_turn, &game.head, cur_fd, p->ipaddr, p->name);
                        }
                        else{
                        add_player_to_game(&game.head, cur_fd, p->ipaddr , p->name);
                        }
                        // Broadcast to players that new player has joined
                        char buf[MAX_MSG];
                        printf("%s", broadcast_join);
                        broadcast(&game, broadcast_join);
                        write(cur_fd, status_message(buf, &game), strlen(status_message(buf,&game)));
                        announce_turn(&game);
                        // Needs another return to join in
                        //write(cur_fd, START_PLAY, strlen(START_PLAY));
                        // Remove player from new_players
                        remove_new_player(&new_players, cur_fd);
                        // Indicate that a new player has been added.
                        player_added += 1;
                        break;
                    }
                }
            }
        }
    }

    return 0;
}
