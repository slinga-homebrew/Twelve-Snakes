/*
Twelve Snakes v3.0.0 - a 12 player snake clone by Slinga
*/

/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <jo/jo.h> // Required for basic sgl functions

#define MAX_PLAYERS 12
#define MIN_SCORE -99
#define MAX_SCORE 999
#define MAX_SLOWDOWN 9
#define MIN_SLOWDOWN 0
#define MAX_SUBOPTION_VALUES 5
#define INITIAL_SLOWDOWN 5
#define PORT_TWO 9
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_RIGHT 2
#define DIR_LEFT 3
#define OFF_SCREEN -1

#define MIN_Y 7
#define MAX_Y 23
#define MIN_X 2
#define MAX_X 37

#define MAX_SUDDEN_DEATH_X (MAX_X - MIN_X + 1)
#define MAX_SUDDEN_DEATH_Y (MAX_Y - MIN_Y + 1)

#define GAME_FREE_FOR_ALL     0
#define GAME_SCORE_ATTACK     1
#define GAME_BATTLE_ROYALE    2
#define GAME_SURVIVOR         3
#define GAME_KING_OF_THE_HILL 4

// stdlib function prototypes to keep compiler happy
void* memcpy(void *dst, const void *src, unsigned int len);
int rand(void);
void srand(unsigned int seed);
void *malloc(unsigned int size);
void free(void *ptr);
void *memset(void *s, int c, unsigned int n);
int strcmp(const char* s1, const char* s2);

struct location
{
    int x;
    int y;
    struct location* next;
};

struct food
{
    char shape[2]; // The shape of the food
    int x;
    int y;
};

struct options
{
    int gameType;
    int maxLives;
    int maxScore;
    int maxTime;
    int slowdown; // factor used to adjust the speed of the game

    int startTime; // what time in seconds the game was started
    int joinTimeStopped; // no longer allowed to join the game
    int suddenDeath; // are we in sudden death mode for Battle Royale
};

struct suboptions
{
    char optionName[16];
    char optionType[8];
    int position;
    int values[MAX_SUBOPTION_VALUES];
};

struct sudden_death_grid
{
    int lastX;
    int lastY;
    int count;
    int dir;
    char grid[MAX_SUDDEN_DEATH_X][MAX_SUDDEN_DEATH_X];
};

const struct suboptions SUBOPTION_TIME_LIMIT =  {"Time Limit: ", "min",    1, {1, 3, 5, 7, 10}};
const struct suboptions SUBOPTION_LIVES_LIMIT = {"Lives Limit:", "lives",  2, {1, 3, 5, 7, 10}};
const struct suboptions SUBOPTION_SCORE_LIMIT = {"Score Limit:", "points", 2, {10, 15, 25, 50, 100}};
const struct suboptions SUBOPTION_SLOWDOWN =    {"Slowdown:   ", "delay",  2, {3, 4, 5, 6, 7}};

struct snake
{
    int ID; // index into the array of players
    int controllerNum; // which port to read data from. On multitap 2 this it != ID
    char shape[2]; // the shape of the snake
    int dir; // current direction snake is moving in
    int active; // Is this player playing or not
    int everActive; // has the player ever been active?
    int dying; // Is player marked for death?
    struct location* head; // points to the head of the snake
    struct location* tail; // points to the tail of the snake

    // variables for score
    int numApples;
    int numDeaths;
    int numKills;
    int numPlayersEaten;
    int currLength;
    int maxLength;
    int score;
};

// init functions
void initializePlayer(struct snake* somePlayer, struct options* gameOptions);
void initializeFood(struct food* theFood, char theShape);
void initializePlayerNumbers(struct snake* players);

// display\drawing functions
void displayText(); // Displays the heading information
void displayJoinText(struct snake* players, struct options* gameOptions);
void displayMenu(); // Displays the menu choices
int displaySubMenu(struct options* gameOptions, char* gameMode, int numSubOptions, struct suboptions* subOptions);
void displaySSMTFPresents(); // Displays Sega Saturn Multiplayer Task Force logo
void drawGrid(); // Draws the playing field
void drawSnake(struct snake* somePlayer); // Updates the snake, collision detection
void drawFood(struct food* someFood, struct snake* players); // Draws the food on the screen
void drawSuddenDeathGrid(struct sudden_death_grid* deathGrid);
void displayScore(struct snake* players, struct options* gameOptions);
int displayScoreBar(struct snake* players, struct options* gameOptions);
void clearScreen();
void redrawScreen(struct snake* players, struct food* theFood, struct sudden_death_grid* deathGrid);
void redrawSuddenDeathGrid(struct sudden_death_grid* suddenDeath);
void titleScreen();

// game functions
void killPlayer(struct snake* somePlayer);
void eraseSnake(struct location* snakeHead); // erases the snake, free its memory
void pressStart(struct snake* players, struct options* gameOptions);
int safeFood(struct food* someFood, struct snake* players); // returns a 1 if the new position of the food is "safe"
void clearScore(struct snake* players); // clears the game score
void checkPlayerOneCommands(struct snake* players, struct food* theFood, struct options* gameOptions, struct sudden_death_grid* deathGrid);
void checkForCollisions(struct snake* someSnake, struct snake* players);
void checkForSuddenDeathCollisions(struct snake* players, struct options* gameOptions, struct sudden_death_grid* deathGrid);
void validateScore(struct snake* players, struct options* gameOptions);
int isAllowedToSpawn(struct snake* somePlayer, struct options* gameOptions);
void growSnake(struct snake* player, int amount);
int changeSuddenDeathDir(struct sudden_death_grid* deathGrid);

// utility functions
void getTime(jo_datetime* currentTime);
unsigned int getSeconds();
void checkForABCStart();

int g_DisplayedSSMTF = 0;



void jo_main(void)
{
    int i = 0;
    Uint16 data = 0;
    struct snake players[MAX_PLAYERS] = {0};
    struct food theFood = {0};
    struct options gameOptions = {0};
    struct sudden_death_grid deathGrid = {0};

    int redrawGrid = 0;
    int gameEnded = 0;

    // Initializing functions
    slInitSystem(TV_320x240, NULL, 1); // Initializes screen

    if(g_DisplayedSSMTF == 0)
    {
        //displaySSMTFPresents(); // SSMTF logo
        g_DisplayedSSMTF = 1;
    }
    drawGrid(); // Draws the playing field box

    displayText();
    titleScreen();

    do
    {
        //
        // Initialize game specific things
        //
        initializePlayerNumbers(players);
        memset(&deathGrid, 0, sizeof(deathGrid));
        clearScore(players);
        srand(getSeconds());
                
        //
        // Prompt the player for game mode and options
        //
        displayMenu(&gameOptions);
        initializeFood(&theFood, '*');


        //
        // Game play loop
        //
        do
        {
            //
            // Check for special player one commands
            //
            checkPlayerOneCommands(players, &theFood, &gameOptions, &deathGrid);

            //
            // Draw existing players
            //
            for(i = 0; i < MAX_PLAYERS; i++)
            {
                data = Smpc_Peripheral[players[i].controllerNum].data;

                // Check if player pressed the A button and is not already playing
                if((data & PER_DGT_TA) == 0 && players[i].active == 0)
                {
                    initializePlayer(&players[i], &gameOptions);
                }

                if(players[i].active == 1)
                {
                    drawSnake(&players[i]);
                }
            }

            if(gameOptions.suddenDeath == 1)
            {
                drawSuddenDeathGrid(&deathGrid);
            }

            //
            // After all players have moved, check for collisions
            //
            for(i = 0; i < MAX_PLAYERS; i++)
            {
                if(players[i].active == 1)
                {
                    checkForCollisions(&players[i], players);
                }
            }

            if(gameOptions.suddenDeath == 1)
            {
                checkForSuddenDeathCollisions(players, &gameOptions, &deathGrid);
            }

            //
            // Kill snakes that are marked for death
            //
            for(i = 0; i < MAX_PLAYERS; i++)
            {
                if(players[i].active == 1 && players[i].dying == 1)
                {
                    killPlayer(&players[i]);
                    redrawGrid = 1; // someone died so redraw the grid
                }
            }

            //
            // Draw the food
            //
            drawFood(&theFood, players);

            //
            // Draw the grid again in case a Snake crashed into it
            //
            if(redrawGrid == 1)
            {
                redrawGrid = 0;
                redrawScreen(players, &theFood, &deathGrid);
            }

            // display the score bar and check for end of game conditions
            gameEnded = displayScoreBar(players, &gameOptions);
            if(gameEnded == 1)
            {
                clearScreen();
                displayScore(players, &gameOptions);
                pressStart(players, &gameOptions);
                jo_main();
            }

            // "Press A to Join"
            displayJoinText(players, &gameOptions);

            //
            // synch the screen
            //
            slSynch(); // You won't see anything without this!!
            for(i = 0; i < gameOptions.slowdown; i++)
            {
                slSynch(); // Slow down
            }

        }while(1); // game loop

    }while(1); // game type loop
}

void initializePlayerNumbers(struct snake* players)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        int controllerNum = 0;

        if(i < 6)
        {
            // player is on multitap 1
            controllerNum = i;
        }
        else
        {
            // player is on multitap 2
            // the controllerNum is offset
            controllerNum = i + PORT_TWO;
        }

        players[i].ID = i;
        players[i].controllerNum = controllerNum;
    }
}

int isAllowedToSpawn(struct snake* somePlayer, struct options* gameOptions)
{
    //
    // Do not let the player spawn if:
    // - they are playing a game type with lives and have run out
    // - they didn't spawn in the first 30s a
    //

    switch(gameOptions->gameType)
    {
        case GAME_FREE_FOR_ALL:
        case GAME_SURVIVOR:
        case GAME_KING_OF_THE_HILL:
        case GAME_SCORE_ATTACK:
            // FFA, Survivor, and KOTH, SA always allow spawning
            return 1;
            break;

        case GAME_BATTLE_ROYALE:

            if(gameOptions->suddenDeath == 1)
            {
                // don't allow spawning in sudden death
                return 0;
            }

            // don't allow spawning if the player is out of lives
            if(somePlayer->numDeaths >= gameOptions->maxLives)
            {
                return 0;
            }

            return 1;
            break;
    }

    return 0;
}

void initializePlayer(struct snake* somePlayer, struct options* gameOptions)
{
    if(isAllowedToSpawn(somePlayer, gameOptions) == 0)
    {
        return;
    }

    // Create player
    if(somePlayer->active != 1)
    {
        switch (somePlayer->ID)
        {
            case 0:
                somePlayer->shape[0] = (char)14; // block
                break;

            case 1:
                somePlayer->shape[0] = (char)35; // pound sign
                break;

            case 2:
                somePlayer->shape[0] = (char)23; // gaurav's gamma
                break;

            case 3:
                somePlayer->shape[0] = (char)127; // checkerboard
                break;

            case 4:
                somePlayer->shape[0] = (char)92; // looks like a V
                break;

            case 5:
                somePlayer->shape[0] = (char)38; // percent sign
                break;

            case 6:
                somePlayer->shape[0] = (char)64; // copyright symbol
                break;

            case 7:
                somePlayer->shape[0] = (char)56; // number eight
                break;

            case 8:
                somePlayer->shape[0] = (char)37; // percent sign
                break;

            case 9:
                somePlayer->shape[0] = (char)48; // zero
                break;

            case 10:
                somePlayer->shape[0] = (char)81; // letter Q
                break;

            case 11:
                somePlayer->shape[0] = (char)149; // evil snake!
                break;

            default:
                somePlayer->shape[0] = 'e';
                break;
        }

        somePlayer->shape[1] = '\0';

        // allocate three segments for the snake
        somePlayer->head = (struct location*)malloc(sizeof(struct location));
        somePlayer->head->next = (struct location*)malloc(sizeof(struct location));
        somePlayer->head->next->next= (struct location*)malloc(sizeof(struct location));

        switch(somePlayer->ID)
        {
            case 0:
                somePlayer->head->x = 1;
                somePlayer->head->y = 9;
                somePlayer->dir = 2;
                break;

            case 1:
                somePlayer->head->x = 38;
                somePlayer->head->y = 21;
                somePlayer->dir = 3;
                break;

            case 2:
                somePlayer->head->x = 1;
                somePlayer->head->y = 21;
                somePlayer->dir = 2;
                break;

            case 3:
                somePlayer->head->x = 38;
                somePlayer->head->y = 9;
                somePlayer->dir = 3;
                break;

            case 4:
                somePlayer->head->x = 1;
                somePlayer->head->y = 15;
                somePlayer->dir = 2;
                break;

            case 5:
                somePlayer->head->x = 38;
                somePlayer->head->y = 15;
                somePlayer->dir = 3;
                break;

            case 6:
                somePlayer->head->x = 29;
                somePlayer->head->y = 24;
                somePlayer->dir = 0;
                break;

            case 7:
                somePlayer->head->x = 9;
                somePlayer->head->y = 6;
                somePlayer->dir = 1;
                break;

            case 8:
                somePlayer->head->x = 9;
                somePlayer->head->y = 24;
                somePlayer->dir = 0;
                break;

            case 9:
                somePlayer->head->x = 29;
                somePlayer->head->y = 6;
                somePlayer->dir = 1;
                break;

            case 10:
                somePlayer->head->x = 19;
                somePlayer->head->y = 24;
                somePlayer->dir = 0;
                break;

            case 11:
                somePlayer->head->x = 19;
                somePlayer->head->y = 6;
                somePlayer->dir = 1;
                break;
        }

        // Initiate rest of snake to offscreen positions
        somePlayer->head->next->x = OFF_SCREEN;
        somePlayer->head->next->y = OFF_SCREEN;

        somePlayer->head->next->next->x = OFF_SCREEN;
        somePlayer->head->next->next->y = OFF_SCREEN;

        somePlayer->head->next->next->next = NULL;
        somePlayer->tail = somePlayer->head->next->next;

        somePlayer->currLength = 3;
        if(somePlayer->currLength > somePlayer->maxLength)
        {
            somePlayer->maxLength = somePlayer->currLength;
        }

        somePlayer->active = 1;
        somePlayer->everActive = 1;
        somePlayer->dying = 0;

        // Draw the starting position of the snake
        slPrint(somePlayer->shape, slLocate(somePlayer->head->x, somePlayer->head->y));
    }
}

void checkPlayerOneCommands(struct snake* players, struct food* theFood, struct options* gameOptions, struct sudden_death_grid* deathGrid)
{
    Uint16 data = 0;

    // Read the 1st player controller
    data = Smpc_Peripheral[0].data;

    checkForABCStart();

    // Did player decrease game speed
    if((data & PER_DGT_TL) == 0)
    {
        gameOptions->slowdown++;
        if(gameOptions->slowdown > MAX_SLOWDOWN)
        {
            gameOptions->slowdown = MAX_SLOWDOWN;
        }
    }

    // Did player increase game speed
    if((data & PER_DGT_TR) == 0)
    {
        gameOptions->slowdown--;
        if(gameOptions->slowdown < MIN_SLOWDOWN)
        {
            gameOptions->slowdown = MIN_SLOWDOWN;
        }
    }

    // Does the user want to see the score
    if((data & PER_DGT_ST) == 0)
    {
        clearScreen();
        displayScore(players, gameOptions);
        pressStart(players, gameOptions);
        clearScreen();
        redrawScreen(players, theFood, deathGrid);
    }

    // Does the user want to clear score
    if((data & PER_DGT_TZ) == 0)
    {
        clearScore(players);
    }
}

void drawGrid()
{
    int j;

    char temp[10];
    char top[40]; // The top of the playing field
    char bottom[40]; // The bottom of the playing field

    char topLeftPit[2];
    char bottomRightPit[2];
    char topRightPit[2];
    char bottomLeftPit[2];
    char sidePit[2];
    char topPit[2];

    // Fill the arrays with '-'
    sprintf(temp, "%c", 21);
    for(j = 0; j<38; j++)
    {
        top[j] = temp[0];
        bottom[j] = temp[0];
    }

    // Corner pieces
    sprintf(temp, "%c", 23);
    top[0] = temp[0];

    sprintf(temp, "%c", 24);
    top[37] = temp[0];

    sprintf(temp, "%c", 25);
    bottom[0] = temp[0];

    sprintf(temp, "%c", 26);
    bottom[37] = temp[0];

    top[38] = '\0';
    bottom[38] = '\0';

    // Draw the top and bottom borders
    slPrint(top, slLocate(1, 6));
    slPrint(bottom, slLocate(1, 24));

    // Draw the sides
    sprintf(temp, "%c", 22);
    for(j = 7; j<24; j++)
    {
        slPrint(temp, slLocate(1, j));
        slPrint(temp, slLocate(38, j));
    }

    // Draw the snake pits
    sprintf(temp, "%c", 23);
    topLeftPit[0] = temp[0];
    topLeftPit[1] = '\0';

    sprintf(temp, "%c", 25);
    bottomLeftPit[0] = temp[0];
    bottomLeftPit[1] = '\0';

    sprintf(temp, "%c", 24);
    topRightPit[0] = temp[0];
    topRightPit[1] = '\0';

    sprintf(temp, "%c", 26);
    bottomRightPit[0] = temp[0];
    bottomRightPit[1] = '\0';

    sprintf(temp, "%c", 22);
    sidePit[0] = temp[0];
    sidePit[1] = '\0';

    sprintf(temp, "%c", 21);
    topPit[0] = temp[0];
    topPit[1] = '\0';

    // Left Side Pits
    slPrint(bottomRightPit, slLocate(1,8));
    slPrint(topLeftPit, slLocate(0,8));
    slPrint(sidePit, slLocate(0,9));
    slPrint(" ", slLocate(1,9));
    slPrint(bottomLeftPit, slLocate(0,10));
    slPrint(topRightPit, slLocate(1,10));

    slPrint(bottomRightPit, slLocate(1,14));
    slPrint(topLeftPit, slLocate(0,14));
    slPrint(sidePit, slLocate(0,15));
    slPrint(" ", slLocate(1,15));
    slPrint(bottomLeftPit, slLocate(0,16));
    slPrint(topRightPit, slLocate(1,16));

    slPrint(bottomRightPit, slLocate(1,20));
    slPrint(topLeftPit, slLocate(0,20));
    slPrint(sidePit, slLocate(0,21));
    slPrint(" ", slLocate(1,21));
    slPrint(bottomLeftPit, slLocate(0,22));
    slPrint(topRightPit, slLocate(1,22));

    // Right Side Pits
    slPrint(bottomLeftPit, slLocate(38,8));
    slPrint(topRightPit, slLocate(39,8));
    slPrint(sidePit, slLocate(39,9));
    slPrint(" ", slLocate(38,9));
    slPrint(bottomRightPit, slLocate(39,10));
    slPrint(topLeftPit, slLocate(38,10));

    slPrint(bottomLeftPit, slLocate(38,14));
    slPrint(topRightPit, slLocate(39,14));
    slPrint(sidePit, slLocate(39,15));
    slPrint(" ", slLocate(38,15));
    slPrint(bottomRightPit, slLocate(39,16));
    slPrint(topLeftPit, slLocate(38,16));

    slPrint(bottomLeftPit, slLocate(38,20));
    slPrint(topRightPit, slLocate(39,20));
    slPrint(sidePit, slLocate(39,21));
    slPrint(" ", slLocate(38,21));
    slPrint(bottomRightPit, slLocate(39,22));
    slPrint(topLeftPit, slLocate(38,22));

    // Top Pits
    slPrint(bottomRightPit, slLocate(8,6));
    slPrint(topLeftPit, slLocate(8,5));
    slPrint(topPit, slLocate(9,5));
    slPrint(" ", slLocate(9,6));
    slPrint(topRightPit, slLocate(10,5));
    slPrint(bottomLeftPit, slLocate(10,6));

    slPrint(bottomRightPit, slLocate(18,6));
    slPrint(topLeftPit, slLocate(18,5));
    slPrint(topPit, slLocate(19,5));
    slPrint(" ", slLocate(19,6));
    slPrint(topRightPit, slLocate(20,5));
    slPrint(bottomLeftPit, slLocate(20,6));

    slPrint(bottomRightPit, slLocate(28,6));
    slPrint(topLeftPit, slLocate(28,5));
    slPrint(topPit, slLocate(29,5));
    slPrint(" ", slLocate(29,6));
    slPrint(topRightPit, slLocate(30,5));
    slPrint(bottomLeftPit, slLocate(30,6));

    // Bottom Pits
    slPrint(topRightPit, slLocate(8,24));
    slPrint(bottomLeftPit, slLocate(8,25));
    slPrint(topPit, slLocate(9,25));
    slPrint(" ", slLocate(9,24));
    slPrint(bottomRightPit, slLocate(10,25));
    slPrint(topLeftPit, slLocate(10,24));

    slPrint(topRightPit, slLocate(18,24));
    slPrint(bottomLeftPit, slLocate(18,25));
    slPrint(topPit, slLocate(19,25));
    slPrint(" ", slLocate(19,24));
    slPrint(bottomRightPit, slLocate(20,25));
    slPrint(topLeftPit, slLocate(20,24));

    slPrint(topRightPit, slLocate(28,24));
    slPrint(bottomLeftPit, slLocate(28,25));
    slPrint(topPit, slLocate(29,25));
    slPrint(" ", slLocate(29,24));
    slPrint(bottomRightPit, slLocate(30,25));
    slPrint(topLeftPit, slLocate(30,24));
}

void checkForSuddenDeathCollisions(struct snake* players, struct options* gameOptions, struct sudden_death_grid* deathGrid)
{
    if(gameOptions->gameType != GAME_BATTLE_ROYALE || gameOptions->suddenDeath != 1)
    {
        return;
    }

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        struct snake* someSnake = &players[i];

        if(deathGrid->grid[someSnake->head->x - MIN_X][someSnake->head->y - MIN_Y] != 0)
        {
            someSnake->dying = 1;
            continue;
        }
    }
}

void drawSuddenDeathGrid(struct sudden_death_grid* deathGrid)
{
    int newX = 0;
    int newY = 0;

    // check if this is the first time we are calling this
    if(deathGrid->count == 0)
    {
        deathGrid->dir = DIR_DOWN;
        deathGrid->lastX = 0;
        deathGrid->lastY = -1;
    }

    switch(deathGrid->dir)
    {
        case DIR_DOWN:
            newX = deathGrid->lastX;
            newY = deathGrid->lastY + 1;
            break;
        case DIR_UP:
            newX = deathGrid->lastX;
            newY = deathGrid->lastY - 1;
            break;
        case DIR_LEFT:
            newX = deathGrid->lastX - 1;
            newY = deathGrid->lastY;
            break;
        case DIR_RIGHT:
            newX = deathGrid->lastX + 1;
            newY = deathGrid->lastY;
            break;
    }

    if(newX < 0 || newX > MAX_X - MIN_X)
    {
        changeSuddenDeathDir(deathGrid);
        return;
    }

    if(newY < 0 || newY > MAX_Y - MIN_Y)
    {
        changeSuddenDeathDir(deathGrid);
        return;
    }

    if(deathGrid->grid[newX][newY] == 'X')
    {
        // grid is occupied, try again
        changeSuddenDeathDir(deathGrid);
        return;
    }

    deathGrid->grid[newX][newY] = 'X';

    deathGrid->lastX = newX;
    deathGrid->lastY = newY;

    slPrint("X", slLocate(deathGrid->lastX + MIN_X, deathGrid->lastY + MIN_Y));
    deathGrid->count++;
}

int changeSuddenDeathDir(struct sudden_death_grid* deathGrid)
{
    switch(deathGrid->dir)
    {
        case DIR_DOWN:
            deathGrid->dir = DIR_RIGHT;
            break;
        case DIR_UP:
            deathGrid->dir = DIR_LEFT;
            break;
        case DIR_LEFT:
            deathGrid->dir = DIR_DOWN;
            break;
        case DIR_RIGHT:
            deathGrid->dir = DIR_UP;
            break;
    }

    return 0;
}

void checkForCollisions(struct snake* someSnake, struct snake* players)
{
    struct location* temp = NULL;

    // Check collision with ceiling
    if(someSnake->head->y < MIN_Y && someSnake->dir != DIR_DOWN)
    {
        someSnake->dying = 1;
        return;
    }

    // Check collision with floor
    if(someSnake->head->y > MAX_Y && someSnake->dir != DIR_UP)
    {
        someSnake->dying = 1;
        return;
    }

    // Check collision with left wall
    if(someSnake->head->x < MIN_X && someSnake->dir != DIR_RIGHT)
    {
        someSnake->dying = 1;
        return;
    }

    // Check collision with right wall
    if(someSnake->head->x > MAX_X && someSnake->dir != DIR_LEFT)
    {
        someSnake->dying = 1;
        return;
    }

    // Check for collisions with yourself and other players
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        if(players[i].active == 1)
        {
            // Don't check for collisions with your own head!!
            if(someSnake->ID != i)
            {
                temp = players[i].head;
            }
            else
            {
                temp = players[i].head->next;
            }

            while(temp != NULL)
            {
                // check if there was a collision
                if(someSnake->head->x == temp->x && someSnake->head->y == temp->y)
                {
                    // check for head-on collision
                    if(temp == players[i].head || temp == players[i].head->next)
                    {
                        // if Snake is at least twice as big eat the other snake
                        if(someSnake->currLength >= players[i].currLength * 2)
                        {
                            players[i].dying = 1;
                            someSnake->numPlayersEaten++;

                            // consome the other snake
                            growSnake(someSnake, players[i].currLength);
                            return;
                        }
                    }

                    someSnake->dying = 1;

                    if(someSnake->ID != players[i].ID)
                    {
                        // Other player killed you, reward him
                        players[i].numKills++;
                    }
                    return;
                }
                temp = temp->next;
            }
        }
    }
}

void drawSnake(struct snake* someSnake)
{
    Uint16 data = 0;
    //Uint16 i;
    struct location* temp = NULL;

    // Checks the controller for input
    data = Smpc_Peripheral[someSnake->controllerNum].data;

    // Check vertical movement
    if((data & PER_DGT_KD)== 0)
    {
        if(someSnake->dir != 0)
        {
            someSnake->dir = 1;
        }
    }
    else if((data & PER_DGT_KU)== 0)
    {
        if(someSnake->dir != 1)
        {
            someSnake->dir = 0;
        }
    }
    // Check horizontal movement
    else if((data & PER_DGT_KR)== 0)
    {
        if(someSnake->dir!= 3)
        {
            someSnake->dir = 2;
        }
    }
    else if((data & PER_DGT_KL)== 0)
    {
        if(someSnake->dir!=2)
        {
            someSnake->dir = 3;
        }
    }

    // erase old snake position
    // need it so that it deletes only the tail
    temp = (someSnake->head);

    // Temp will be second to last node
    while(temp->next != someSnake->tail)
    {
        temp = temp->next;
    }

    // Erase the old tail
    if(someSnake->tail->x != OFF_SCREEN && someSnake->tail->y != OFF_SCREEN)
    {
        slPrint(" ", slLocate(someSnake->tail->x, someSnake->tail->y));
    }
    temp->next = NULL;

    // Calc snake's new position
    if(someSnake->dir == DIR_UP)
    {
        // up
        someSnake->tail->y = someSnake->head->y - 1;
        someSnake->tail->x = someSnake->head->x;
    }
    else if(someSnake->dir == DIR_DOWN)
    {
        // down
        someSnake->tail->y = someSnake->head->y + 1;
        someSnake->tail->x = someSnake->head->x;
    }
    else if(someSnake->dir == DIR_RIGHT)
    {
        // right
        someSnake->tail->x = someSnake->head->x + 1;
        someSnake->tail->y = someSnake->head->y;
    }
    else if(someSnake->dir == DIR_LEFT)
    {
        // left
        someSnake->tail->x = someSnake->head->x-1;
        someSnake->tail->y = someSnake->head->y;
    }

    // set the new tail
    someSnake->tail->next = someSnake->head;
    someSnake->head = someSnake->tail;
    someSnake->tail = temp;
    someSnake->tail->next = NULL;
    temp = NULL;

    // draw new snake position
    // draws only the new head
    slPrint(someSnake->shape, slLocate(someSnake->head->x, someSnake->head->y));
}

// Displays the "Sega Saturn Multiplayer Task Force" presents screen
void displaySSMTFPresents()
{
    Uint16 counter = 0;

    do{
        slPrint("The Sega Saturn", slLocate(13, 11));
        slPrint("Multiplayer Task Force", slLocate(9, 14));
        slPrint("Proudly Presents", slLocate(12, 17));

        slSynch();

        counter++;

    }while(counter < 200);

    clearScreen();
}

void displayText()
{
    slPrint("Twelve Snakes Version 3.0.0 by Slinga", slLocate(1,1));
}

void displayJoinText(struct snake* players, struct options* gameOptions)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        // if even one player can join, display the Press A to join button
        if(isAllowedToSpawn(&players[i], gameOptions) == 1)
        {
            slPrint("Press A to join", slLocate(1,2));
            return;
        }
    }

    // no more players can join, erase the text
    slPrint("               ", slLocate(1,2));
}

// Displays the text "Press Start" and waits for the user to hit start
void pressStart(struct snake* players, struct options* gameOptions)
{
    Uint16 data;

    do{
        data = Smpc_Peripheral[0].data; // Checks if start button has been pressed
        slSynch();

    }while((data & PER_DGT_ST) == 0);

    do{
        data = Smpc_Peripheral[0].data; // Checks if start button has been pressed
        slPrint("Press Start", slLocate(15, 23));

        // check if the user cleared the scores
        if((data & PER_DGT_TZ) == 0)
        {
            if(players != NULL && gameOptions != NULL)
            {
                clearScore(players);
                displayScore(players, gameOptions);
            }
        }

        checkForABCStart();

        slSynch();
        slSynch();
        slSynch();

    }while((data & PER_DGT_ST) != 0);

    // hack in case the user is already holding down the start button
    // first check if the start button is pressed, now wait for it to be released
    do{
        data = Smpc_Peripheral[0].data; // Checks if start button has been pressed
        slSynch();

    }while((data & PER_DGT_ST) == 0);
}

void killPlayer(struct snake* somePlayer)
{
    // erase Snake
    eraseSnake(somePlayer->head);

    // You died, so increase your deaths
    somePlayer->numDeaths++;
    somePlayer->currLength = 0;

    somePlayer->active = 0;
    somePlayer->dying = 0;
    somePlayer->dir = 0;
}

void eraseSnake(struct location* snakeHead)
{
    if(snakeHead->next!=NULL)
    {
        eraseSnake(snakeHead->next);
    }

    if(snakeHead->x != OFF_SCREEN && snakeHead->y != OFF_SCREEN)
    {
        slPrint(" ", slLocate(snakeHead->x, snakeHead->y));
    }
    free(snakeHead);
}

void initializeFood(struct food* theFood, char theShape)
{
    // Shape, and initial position of food
    theFood->shape[0] = theShape;
    theFood->shape[1] = '\0';

    theFood->x = (rand()%(MAX_X - MIN_X + 1)) + MIN_X;
    theFood->y = (rand()%(MAX_Y - MIN_Y + 1)) + MIN_Y;

    // Print the food
    slPrint(theFood->shape, slLocate(theFood->x, theFood->y));
}

void drawFood(struct food* theFood, struct snake* players)
{
    struct location* temp = NULL;
    Uint16 i = 0;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        if(players[i].active == 1)
        {
            temp = players[i].head;

            // check if there was a collision
            if(theFood->x == temp->x && theFood->y == temp->y)
            {
                do{
                    theFood->x = (rand()%(MAX_X - MIN_X + 1)) + MIN_X;
                    theFood->y = (rand()%(MAX_Y - MIN_Y + 1)) + MIN_Y;
                }while(safeFood(theFood, players) != 1);

                slPrint(theFood->shape, slLocate(theFood->x, theFood->y));

                // Add a new segment, make that segment the tail
                growSnake(&players[i], 1);

                // You ate the apple, increase your score
                players[i].numApples++;


                if(players[i].currLength > players[i].maxLength)
                {
                    players[i].maxLength = players[i].currLength;
                }

                return;
            }
        }
    }
}

void growSnake(struct snake* player, int amount)
{
    struct location* temp = NULL;

    for(int i = 0; i < amount; i++)
    {
        temp = (struct location*)malloc(sizeof(struct location));

        player->tail->next = temp;
        temp->x = OFF_SCREEN;
        temp->y = OFF_SCREEN;
        temp->next = NULL;

        player->tail = temp;
        player->currLength++;
    }
}

int safeFood(struct food* someFood, struct snake* players)
{
    struct location* temp;

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        if(players[i].active == 1)
        {
            temp = players[i].head;

            while(temp!=NULL)
            {
                // check if there was a collision
                if(someFood->x == temp->x && someFood->y == temp->y)
                {
                    return 0;
                }
                temp = temp->next;
            }
        }
    }

    return 1;
}

void clearScore(struct snake* players)
{
    int i;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        players[i].numApples = 0;
        players[i].numDeaths = 0;
        players[i].numPlayersEaten = 0;
        players[i].numKills = 0;
        players[i].currLength = 0;
        players[i].maxLength = 0;
        players[i].score = 0;
    }
}

void validateScore(struct snake* players, struct options* gameOptions)
{
    int i;
    int gameType = gameOptions->gameType;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        players[i].numApples = MIN(players[i].numApples, MAX_SCORE);
        players[i].numApples = MAX(players[i].numApples, MIN_SCORE);

        players[i].numDeaths = MIN(players[i].numDeaths, MAX_SCORE);
        players[i].numDeaths = MAX(players[i].numDeaths, MIN_SCORE);

        players[i].numKills = MIN(players[i].numKills, MAX_SCORE);
        players[i].numKills = MAX(players[i].numKills, MIN_SCORE);

        players[i].numPlayersEaten = MIN(players[i].numPlayersEaten, MAX_SCORE);
        players[i].numPlayersEaten = MAX(players[i].numPlayersEaten, MIN_SCORE);

        players[i].currLength = MIN(players[i].currLength, MAX_SCORE);
        players[i].currLength = MAX(players[i].currLength, MIN_SCORE);

        players[i].maxLength = MIN(players[i].maxLength, MAX_SCORE);
        players[i].maxLength = MAX(players[i].maxLength, MIN_SCORE);

        if(gameType == GAME_FREE_FOR_ALL || gameType == GAME_SCORE_ATTACK)
        {
            players[i].score = players[i].numApples + players[i].numKills - players[i].numDeaths;
            players[i].score = MIN(players[i].score, MAX_SCORE);
            players[i].score = MAX(players[i].score, MIN_SCORE);
        }
        else if(gameType == GAME_BATTLE_ROYALE)
        {
            players[i].score = gameOptions->maxLives - players[i].numDeaths;
        }
        else if(gameType == GAME_SURVIVOR)
        {
            players[i].score = players[i].currLength;
        }
        else if(gameType == GAME_KING_OF_THE_HILL)
        {
            players[i].score = players[i].maxLength;
        }
    }
}

/* Function to sort an array using insertion sort*/
void insertionSort(struct snake* players)
{
    int i, j;
    struct snake key = {0};
    for (i = 1; i < MAX_PLAYERS; i++) {
        key = players[i];
        j = i - 1;

        /* Move elements of arr[0..i-1], that are
          greater than key, to one position ahead
          of their current position */
        while (j >= 0 && players[j].score < key.score) {
            players[j + 1] = players[j];
            j = j - 1;
        }
        players[j + 1] = key;
    }
}

int calculatePlayersRemaining(struct snake* players, struct options* gameOptions)
{
    int count = 0;

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        // if the player is alive currently, count them
        if(players[i].active == 1)
        {
            count++;
            continue;
        }

        // if the player was ever active, but still has lives count them as alive
        if(players[i].everActive == 1 && isAllowedToSpawn(&players[i], gameOptions) == 1)
        {
            count++;
            continue;
        }
    }

    return count;
}

int displayScoreBar(struct snake* players, struct options* gameOptions)
{
    int gameLimitReached = 0;
    int pointsRemaining = 0;
    int highScore = 0;
    int playersRemaining = 0;
    int spawnTime = 0;
    int mins = 0;
    int secs = 0;
    int timeDiff = 0;
    int currSeconds = 0;
    int counter = 0;
    struct snake sortedPlayers[MAX_PLAYERS] = {0};
    char temp[16] = {0};

    // sort the player scores. The "score" field will be different depending on the game type
    validateScore(players, gameOptions);
    memcpy(sortedPlayers, players, sizeof(sortedPlayers));
    insertionSort(sortedPlayers);

    // top left square is game options and points remaining
    switch(gameOptions->gameType)
    {
        case GAME_FREE_FOR_ALL:

            // FFA game never ends, but display highest score
            highScore = sortedPlayers[0].score;
            if(highScore < 0)
            {
                highScore = 0;
            }

            sprintf(temp, "%s %03i", "FFA", highScore);
            break;

        case GAME_SCORE_ATTACK:

            // game ends when score is reached
            // display points remainign
            pointsRemaining = gameOptions->maxScore - sortedPlayers[0].score;
            if(pointsRemaining <= 0)
            {
                pointsRemaining = 0;
                gameLimitReached = 1;
            }
            sprintf(temp, " %s %03i", "SA", pointsRemaining);
            break;

        case GAME_BATTLE_ROYALE:

            // game ends when there is only one player left standing
            playersRemaining = calculatePlayersRemaining(players, gameOptions);

            if(playersRemaining <= 1)
            {
                gameLimitReached = 1;
            }
            sprintf(temp, "%s %03i", "BR", playersRemaining);
            break;

        case GAME_SURVIVOR:
            sprintf(temp, "%s %03i", "SRV", sortedPlayers[0].score);
            break;

        case GAME_KING_OF_THE_HILL:
            sprintf(temp, "%s %03i", "KTH", sortedPlayers[0].score);
            break;
    }
    slPrint(temp, slLocate(1,5));

    // 2nd top square is for time remaining
    switch(gameOptions->gameType)
    {
        case GAME_FREE_FOR_ALL:

            // Free-For-All timer counts up
            currSeconds = getSeconds();
            timeDiff = currSeconds - gameOptions->startTime;

            mins = timeDiff / 60;
            secs = timeDiff % 60;
            break;

        case GAME_BATTLE_ROYALE:

            // Battle Royale games can't end before 15 seconds (to allow people to join in)
            // and once the timer hits, the game doesn't end but sudden death starts
            currSeconds = getSeconds();
            spawnTime = gameOptions->startTime + 15 - currSeconds;
            timeDiff = gameOptions->startTime + gameOptions->maxTime - currSeconds;

            if(spawnTime > 0)
            {
                // we cannot end the game before 15 seconds
                gameLimitReached = 0;
            }

            if(timeDiff <= 0)
            {
                gameOptions->suddenDeath = 1;
                timeDiff = 0;
            }

            mins = timeDiff / 60;
            secs = timeDiff % 60;
            break;

        case GAME_SCORE_ATTACK:
        case GAME_SURVIVOR:
        case GAME_KING_OF_THE_HILL:

            // all other game modes have a timer that counts down
            currSeconds = getSeconds();
            timeDiff = gameOptions->startTime + gameOptions->maxTime - currSeconds;

            if(timeDiff <= 0)
            {
                gameLimitReached = 1;
                timeDiff = 0;
            }

            mins = timeDiff / 60;
            secs = timeDiff % 60;
            break;
    }
    sprintf(temp, " %02i:%02i", mins, secs);
    slPrint(temp, slLocate(11,5));

    // 3rd square is for ranking of top 7 players
    counter = 0;
    for(int i = 0; i < MAX_PLAYERS && counter < 7; i++)
    {
        if(sortedPlayers[i].everActive == 0)
        {
            continue;
        }

        sprintf(temp, "%c", sortedPlayers[i].shape[0]);
        slPrint(temp, slLocate(21 + counter, 5));
        counter++;
    }

    // 4th square is for the slow down speed
    sprintf(temp, "SD %1i", gameOptions->slowdown);
    slPrint(temp, slLocate(31,5));

    // bottom four areas are for the 4 highest scoring players
    counter = 0;
    for(int i = 0; i < MAX_PLAYERS && counter < 4; i++)
    {
        if(sortedPlayers[i].everActive == 0)
        {
            continue;
        }

        sprintf(temp, "%c%c%c %03i", sortedPlayers[i].shape[0], sortedPlayers[i].shape[0],
                                     sortedPlayers[i].shape[0], sortedPlayers[i].score);
        slPrint(temp, slLocate(1 + (counter*10), MAX_Y + 2));
        counter++;
    }

    return gameLimitReached;
}

void displayMenu(struct options* gameOptions)
{
    int counter = 8;
    int cursorPosition = 0;
    int numOptions = 5;
    int numSubOptions = 0;
    int suboptionsResult = 0;
    char* gameMode = NULL;
    struct suboptions subOptions[3] = {0}; // max number of options for subtype is 3

    memset(gameOptions, 0, sizeof(struct options));
    gameOptions->slowdown = INITIAL_SLOWDOWN;

    do
    {
        counter = 8;

        slPrint("Select Game Mode", slLocate(4,counter++));
        slPrint("-------------------------------", slLocate(4,counter++));
        slPrint("                               ", slLocate(4,counter++));

        slPrint("   Free For All", slLocate(4,counter++));
        slPrint("   Score Attack", slLocate(4,counter++));
        slPrint("   Battle Royale", slLocate(4,counter++));
        slPrint("   Survivor", slLocate(4,counter++));
        slPrint("   King of the Hill", slLocate(4,counter++));

        do
        {
            // check if the user is selecting a different option
            if (jo_is_input_key_down(0, JO_KEY_DOWN))
            {
                slPrint("  ", slLocate(4, 11 + cursorPosition));
                cursorPosition++;
            }

            if (jo_is_input_key_down(0, JO_KEY_UP))
            {
                slPrint("  ", slLocate(4, 11 + cursorPosition));
                cursorPosition--;
            }

            if (jo_is_input_key_down(0, JO_KEY_START) ||
                jo_is_input_key_down(0, JO_KEY_A) ||
                jo_is_input_key_down(0, JO_KEY_C))
            {
                break;
            }

            if(cursorPosition < 0)
            {
                cursorPosition = numOptions - 1;
            }

            if(cursorPosition > 4)
            {
                cursorPosition = 0;
            }

            slPrint(">>", slLocate(4, 11 + cursorPosition));
            slSynch();

        }while(1);

        if(cursorPosition < GAME_FREE_FOR_ALL || cursorPosition > GAME_KING_OF_THE_HILL)
        {
            // impossible to get here
            cursorPosition = GAME_FREE_FOR_ALL;
        }
        gameOptions->gameType = cursorPosition;

        // hack to clear the start press
        do
        {
            slSynch();
        }
        while(jo_is_input_key_pressed(0, JO_KEY_START) || jo_is_input_key_pressed(0, JO_KEY_A) || jo_is_input_key_pressed(0, JO_KEY_B));

        // depending on the game type, there are suboptions
        switch(gameOptions->gameType)
        {
            case GAME_FREE_FOR_ALL:
                // no options for free for allocate
                numSubOptions = 0;
                break;

            case GAME_SCORE_ATTACK:
                numSubOptions = 3;
                memcpy(&subOptions[0], &SUBOPTION_SCORE_LIMIT, sizeof(struct suboptions));
                memcpy(&subOptions[1], &SUBOPTION_TIME_LIMIT, sizeof(struct suboptions));
                memcpy(&subOptions[2], &SUBOPTION_SLOWDOWN, sizeof(struct suboptions));
                gameMode = "Score Attack";
                break;

            case GAME_SURVIVOR:
                numSubOptions = 2;
                memcpy(&subOptions[0], &SUBOPTION_TIME_LIMIT, sizeof(struct suboptions));
                memcpy(&subOptions[1], &SUBOPTION_SLOWDOWN, sizeof(struct suboptions));
                gameMode = "Survivor";
                break;

            case GAME_KING_OF_THE_HILL:
                numSubOptions = 2;
                memcpy(&subOptions[0], &SUBOPTION_TIME_LIMIT, sizeof(struct suboptions));
                memcpy(&subOptions[1], &SUBOPTION_SLOWDOWN, sizeof(struct suboptions));
                gameMode = "King of the Hill";
                break;

            case GAME_BATTLE_ROYALE:
                numSubOptions = 3;
                memcpy(&subOptions[0], &SUBOPTION_LIVES_LIMIT, sizeof(struct suboptions));
                memcpy(&subOptions[1], &SUBOPTION_TIME_LIMIT, sizeof(struct suboptions));
                memcpy(&subOptions[2], &SUBOPTION_SLOWDOWN, sizeof(struct suboptions));
                gameMode = "Battle Royale";
                break;
        }

        if(numSubOptions > 0)
        {
            suboptionsResult = displaySubMenu(gameOptions, gameMode, numSubOptions, subOptions);
        }
        else
        {
            suboptionsResult = 1;
        }

        if(suboptionsResult == 0)
        {
            // user hit B, continue
            continue;
        }
        else
        {
            break;
        }

    }while(1);

    clearScreen();
    gameOptions->startTime = getSeconds();
}

int displaySubMenu(struct options* gameOptions, char* gameMode, int numSubOptions, struct suboptions* subOptions)
{
    int counter = 8;
    int cursorPosition = 0;
    char temp[32];

    clearScreen();

    do
    {
        counter = 8;

        sprintf(temp, "%s Options", gameMode);
        slPrint(temp, slLocate(4,counter++));
        slPrint("-------------------------------", slLocate(4,counter++));
        slPrint("                               ", slLocate(4,counter++));

        for(int i = 0; i < numSubOptions; i++)
        {
            int pos = subOptions[i].position;
            sprintf(temp, "   %s %d %s   ", subOptions[i].optionName, subOptions[i].values[pos], subOptions[i].optionType);
            slPrint(temp, slLocate(4,counter++));
        }

        if (jo_is_input_key_down(0, JO_KEY_B))
        {
            // user return B, back up to main menu
            clearScreen();
            return 0;
        }

        // check if the user is selecting a different option
        if (jo_is_input_key_down(0, JO_KEY_DOWN))
        {
            slPrint("  ", slLocate(4, 11 + cursorPosition));
            cursorPosition++;
        }

        if (jo_is_input_key_down(0, JO_KEY_UP))
        {
            slPrint("  ", slLocate(4, 11 + cursorPosition));
            cursorPosition--;
        }

        if(cursorPosition < 0)
        {
            cursorPosition = numSubOptions -1;
        }

        if(cursorPosition > numSubOptions -1)
        {
            cursorPosition = 0;
        }

        // check if the user is selecting a different option
        if (jo_is_input_key_down(0, JO_KEY_LEFT))
        {
            subOptions[cursorPosition].position--;
        }

        if (jo_is_input_key_down(0, JO_KEY_RIGHT))
        {
            subOptions[cursorPosition].position++;
        }

        if(subOptions[cursorPosition].position < 0)
        {
            subOptions[cursorPosition].position = 0;
        }

        if(subOptions[cursorPosition].position > MAX_SUBOPTION_VALUES -1)
        {
            subOptions[cursorPosition].position = MAX_SUBOPTION_VALUES -1;
        }

        if (jo_is_input_key_down(0, JO_KEY_START) ||
            jo_is_input_key_down(0, JO_KEY_A) ||
            jo_is_input_key_down(0, JO_KEY_C))
        {
            break;
        }

        slPrint(">>", slLocate(4, 11 + cursorPosition));
        slSynch();

    }while(1);

    if(cursorPosition < GAME_FREE_FOR_ALL || cursorPosition > GAME_KING_OF_THE_HILL)
    {
        // impossible to get here
        cursorPosition = GAME_FREE_FOR_ALL;
    }
    //gameOptions->gameType = cursorPosition;

    // hack to clear the start press
    do
    {
        slSynch();
    }
    while(jo_is_input_key_pressed(0, JO_KEY_START) || jo_is_input_key_pressed(0, JO_KEY_A) || jo_is_input_key_pressed(0, JO_KEY_B));


    // user hit start, setup the game options
    for(int i = 0; i < numSubOptions; i++)
    {
        int pos = subOptions[i].position;

        if(strcmp(subOptions[i].optionType, "min") == 0)
        {
            gameOptions->maxTime = subOptions[i].values[pos] * 60;
        }
        else if(strcmp(subOptions[i].optionType, "lives") == 0)
        {
            gameOptions->maxLives = subOptions[i].values[pos];
        }
        else if(strcmp(subOptions[i].optionType, "points") == 0)
        {
            gameOptions->maxScore = subOptions[i].values[pos];
        }
        else if(strcmp(subOptions[i].optionType, "delay") == 0)
        {
            gameOptions->slowdown = subOptions[i].values[pos];
        }
    }

    clearScreen();

    return 1;
}

void checkForABCStart()
{
    Uint16 data = 0;

    // Read the 1st player controller
    data = Smpc_Peripheral[0].data;

    // Did player one press ABC+Start?
    if((data & PER_DGT_TA) == 0 &&
       (data & PER_DGT_TB) == 0 &&
       (data & PER_DGT_TC) == 0 &&
       (data & PER_DGT_ST) == 0)
    {
        jo_main(); // this is not technically the correct thing to do
                   // as we are simply recursing and using stack space
    }
}

void displayScore(struct snake* players, struct options* gameOptions)
{
    char temp[50];
    Uint16 counter = 8;
    struct snake sortedPlayers[MAX_PLAYERS] = {0};
    int rank = 1;

    temp[0] = '\0';

    validateScore(players, gameOptions);

    memcpy(sortedPlayers, players, sizeof(sortedPlayers));

    insertionSort(sortedPlayers);

    slPrint("R# CHR  L#  M#  A#  K#  C#  D#  S#", slLocate(3,counter++));
    slPrint("----------------------------------", slLocate(3,counter++));
    slPrint("                                   ", slLocate(3,counter++));

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        // display the score if the player is currently active (or has ever been active)
        if(sortedPlayers[i].everActive == 1)
        {
            sprintf(temp, "%2i %c%c%c %3i %3i %3i %3i %3i %3i %3i", rank, sortedPlayers[i].shape[0], sortedPlayers[i].shape[0], sortedPlayers[i].shape[0],
                                                                 sortedPlayers[i].currLength, sortedPlayers[i].maxLength, sortedPlayers[i].numApples,
                                                                 sortedPlayers[i].numKills, sortedPlayers[i].numPlayersEaten, sortedPlayers[i].numDeaths,
                                                                 sortedPlayers[i].score);
            slPrint(temp, slLocate(3,counter++));
            rank++;
        }
    }
}

void clearScreen()
{
    Uint16 i = 0;

    for(i = 7; i < 24; i++)
    {
        slPrint("                                    ", slLocate(2,i));
    }

    slPrint("                                    ", slLocate(1,2)); // Press A to join line
    slPrint("                                    ", slLocate(2,26)); // dedication line
}

void redrawScreen(struct snake* players, struct food* theFood, struct sudden_death_grid* deathGrid)
{
    Uint16 i = 0;
    struct location* temp = NULL;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        // redraw only the active players
        if(players[i].active == 1)
        {
            temp = players[i].head;

            while(temp != NULL)
            {
                if(temp->x != OFF_SCREEN && temp->y != OFF_SCREEN)
                {
                    slPrint(players[i].shape, slLocate(temp->x, temp->y));
                }
                temp = temp->next;
            }
        }
    }

    slPrint(theFood->shape, slLocate(theFood->x, theFood->y));

    drawGrid();

    redrawSuddenDeathGrid(deathGrid);
}

#define SUDDEN_DEATH_CHAR 'X'

void redrawSuddenDeathGrid(struct sudden_death_grid* suddenDeath)
{
    if(suddenDeath->count == 0)
    {
        return;
    }

    for(int x = 0; x < MAX_SUDDEN_DEATH_X; x++)
    {
        for(int y = 0; y < MAX_SUDDEN_DEATH_Y; y++)
        {
            if(suddenDeath->grid[x][y] == 'X')
            {
                slPrint("X", slLocate(x + MIN_X, y + MIN_Y));
            }
        }
    }
}

void titleScreen()
{
    slPrint("TTTTTT", slLocate(3,8));
    slPrint("  TT  ", slLocate(3,9));
    slPrint("  TT  ", slLocate(3,10));
    slPrint("  TT  ", slLocate(3,11));
    slPrint("  TT  ", slLocate(3,12));
    slPrint("  TT  ", slLocate(3,13));

    slPrint("W     W", slLocate(10,8));
    slPrint("W     W", slLocate(10,9));
    slPrint("W  W  W", slLocate(10,10));
    slPrint("WW W WW", slLocate(10,11));
    slPrint(" WWWWW ", slLocate(10,12));
    slPrint(" WW WW ", slLocate(10,13));

    slPrint("EEEE", slLocate(18,8));
    slPrint("E   ", slLocate(18,9));
    slPrint("EEEE", slLocate(18,10));
    slPrint("EEEE", slLocate(18,11));
    slPrint("E   ", slLocate(18,12));
    slPrint("EEEE", slLocate(18,13));

    slPrint("L  ", slLocate(23,8));
    slPrint("L  ", slLocate(23,9));
    slPrint("L  ", slLocate(23,10));
    slPrint("L  ", slLocate(23,11));
    slPrint("L  ", slLocate(23,12));
    slPrint("LLL", slLocate(23,13));

    slPrint("V   V", slLocate(26,8));
    slPrint("V   V", slLocate(26,9));
    slPrint("V   V", slLocate(26,10));
    slPrint("VV VV", slLocate(26,11));
    slPrint(" V V ", slLocate(26,12));
    slPrint(" VVV ", slLocate(26,13));

    slPrint("EEEE", slLocate(32,8));
    slPrint("E   ", slLocate(32,9));
    slPrint("EEEE", slLocate(32,10));
    slPrint("EEEE", slLocate(32,11));
    slPrint("E   ", slLocate(32,12));
    slPrint("EEEE", slLocate(32,13));

    slPrint("SSSS", slLocate(4,15));
    slPrint("S   ", slLocate(4,16));
    slPrint("S   ", slLocate(4,17));
    slPrint("SSSS", slLocate(4,18));
    slPrint("   S", slLocate(4,19));
    slPrint("   S", slLocate(4,20));
    slPrint("SSSS", slLocate(4,21));

    slPrint("N   N", slLocate(9,15));
    slPrint("NN  N", slLocate(9,16));
    slPrint("N N N", slLocate(9,17));
    slPrint("N NNN", slLocate(9,18));
    slPrint("N  NN", slLocate(9,19));
    slPrint("N   N", slLocate(9,20));
    slPrint("N   N", slLocate(9,21));

    slPrint("  A  ", slLocate(15,15));
    slPrint(" AAA ", slLocate(15,16));
    slPrint("AA AA", slLocate(15,17));
    slPrint("A   A", slLocate(15,18));
    slPrint("AAAAA", slLocate(15,19));
    slPrint("A   A", slLocate(15,20));
    slPrint("A   A", slLocate(15,21));

    slPrint("K  K", slLocate(21,15));
    slPrint("K  K", slLocate(21,16));
    slPrint("K K ", slLocate(21,17));
    slPrint("KKK ", slLocate(21,18));
    slPrint("K K ", slLocate(21,19));
    slPrint("K  K", slLocate(21,20));
    slPrint("K  K", slLocate(21,21));

    slPrint("EEEE", slLocate(26,15));
    slPrint("E   ", slLocate(26,16));
    slPrint("E   ", slLocate(26,17));
    slPrint("EEEE", slLocate(26,18));
    slPrint("E   ", slLocate(26,19));
    slPrint("E   ", slLocate(26,20));
    slPrint("EEEE", slLocate(26,21));

    slPrint("SSSS", slLocate(31,15));
    slPrint("S   ", slLocate(31,16));
    slPrint("S   ", slLocate(31,17));
    slPrint("SSSS", slLocate(31,18));
    slPrint("   S", slLocate(31,19));
    slPrint("   S", slLocate(31,20));
    slPrint("SSSS", slLocate(31,21));


    slPrint("Dedicated to the man with one knee", slLocate(3,26));

    pressStart(NULL, NULL);
    clearScreen();
}

void getTime(jo_datetime* currentTime)
{
    SmpcDateTime *time = NULL;

    slGetStatus();

    time = &(Smpc_Status->rtc);

    currentTime->day = slDec2Hex(time->date);
    currentTime->year = slDec2Hex(time->year);
    currentTime->month = time->month & 0x0f;

    currentTime->hour = (char)slDec2Hex(time->hour);
    currentTime->minute = (char)slDec2Hex(time->minute);
    currentTime->second = (char)slDec2Hex(time->second);
}

unsigned int getSeconds()
{
    jo_datetime now = {0};
    unsigned int numSeconds = 0;

    getTime(&now);

    numSeconds = now.second + (now.minute * 60) + (now.hour * (60*60)) + (now.day * (24*60*60));

    return numSeconds;
}
