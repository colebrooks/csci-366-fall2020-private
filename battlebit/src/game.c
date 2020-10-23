//
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "game.h"

// STEP 10 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server.  Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.

static game * GAME = NULL;

void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int game_fire(game *game, int player, int x, int y) {
    // Step 5 - This is the crux of the game.  You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    //  - You will need up update the players 'shots' value
    //  - you You will need to see if the shot hits a ship in the opponents ships value.  If so, record a hit in the
    //    current players hits field
    //  - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    //  If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    //  PLAYER_1_WINS or PLAYER_2_WINS depending on who won.
    int opponent = !player;
    unsigned long long shot = xy_to_bitval(x, y);
    game->players[player].shots = game->players[player].shots | shot;
    if (game->players[opponent].ships & shot) {
        game->players[player].hits = game->players[player].hits | shot;
        game->players[opponent].ships = game->players[opponent].ships | shot;
        return 1;
    }
    if (!game->players[opponent].ships) {
        game->status = player ? PLAYER_1_WINS : PLAYER_0_WINS;
    }
    return 0;
}

unsigned long long int xy_to_bitval(int x, int y) {
    // Step 1 - implement this function.  We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b00000...0001 (the one is in the first position)
    // x:1, y: 0 == 0b00000...10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.
    return (x > 7 || x < 0 || y > 7 || y < 0) ? 0 : (1ull << (x + 8ull * y));
}

struct game * game_get_current() {
    return GAME;
}

int game_load_board(struct game *game, int player, char * spec) {
    // Step 2 - implement this function.  Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //

    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //add_ship_horizontal
    // if it is invalid, you should return -1
    char ships[5] = {'C', 'B', 'D', 'S', 'P'};
    int lengths[5] = {5, 4, 3, 3, 2};
    if (spec == NULL || strlen(spec) != 15) { // Check if spec is even the right length
        return -1;
    }
    for(int i = 0; i < 15; i = i + 3) {
        char ship = spec[i];
        int x = spec[i+1] - '0';
        int y = spec[i+2] - '0';
        if (strchr(ships, toupper(ship)) == NULL || *strchr(ships, toupper(ship)) == '0') { // Check if ship is allowed
            return -1;                                                                     // character or already used
        }
        int length = lengths[strchr(ships, toupper(ship)) - ships]; // Grab length from corresponding length array.
        *strchr(ships, toupper(ship)) = '0'; // Update to reflect ship being used.
        if (ship >= 'A' && ship <= 'Z') {
            if (add_ship_horizontal(&game->players[player], x, y, length) == -1) {
                return -1;
            }
        } else if (add_ship_vertical(&game->players[player], x, y, length) == -1) {
                return -1;
        }
    }
    return 1;
}

int add_ship_horizontal(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    if (length == 0) {
        return 1;
    }
    if (player->ships & xy_to_bitval(x, y) || length > (8 - x)) {
        player->ships = 0ull;
        return -1;
    }
    else {
        player->ships = player->ships | xy_to_bitval(x, y);
        add_ship_horizontal(player, x+1, y, length-1);
    }
}

int add_ship_vertical(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    if (length == 0) {
        return 1;
    }
    if (player->ships & xy_to_bitval(x, y) || length > (8 - y)) {
        player->ships = 0ull;
        return -1;
    }
    else {
        player->ships = player->ships | xy_to_bitval(x,y);
        add_ship_vertical(player, x, y+1, length-1);
    }
}
