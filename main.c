/*
    12 Player Snake Clone
            by
        Slinga

        Version 2.05
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
#define MAX_SLOWDOWN 20
#define MIN_SLOWDOWN 0
#define INITIAL_SLOWDOWN 5
#define PORT_TWO 9
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_RIGHT 2
#define DIR_LEFT 3

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

struct snake
{
    int ID; // index into the array of players
    int controllerNum; // which port to read data from. On multitap 2 this it != ID
    char shape[2]; // the shape of the snake
    int dir; // current direction snake is moving in
    int active; // Is this player playing or not
    int dying; // Is player marked for death?
    struct location* head; // points to the head of the snake
    struct location* tail; // points to the tail of the snake

    // variables for score
    int numApples;
    int numDeaths;
    int numKills;
    int score;
};

// init functions
void initializePlayer(struct snake* somePlayer, int controllerNum);
void initializeFood(struct food* theFood, char theShape);
void initializePlayerNumbers(struct snake* players);

// display\drawing functions
void displayText(); // Displays the heading information
void displaySSMTFPresents(); // Displays Sega Saturn Multiplayer Task Force logo
void drawGrid(); // Draws the playing field
void drawSnake(struct snake* somePlayer, struct snake* players); // Updates the snake, collision detection
void drawFood(struct food* someFood, struct snake* players); // Draws the food on the screen
void displayScore(struct snake* players);
void displayScoreBar(struct snake* players);
void clearScreen();
void redrawScreen(struct snake* players, struct food* theFood);
void titleScreen();
void calcResolution();

// game functions
void killPlayer(struct snake* somePlayer);
void eraseSnake(struct location* snakeHead); // erases the snake, free its memory
void pressStart(struct snake* players);
int safeFood(struct food* someFood, struct snake* players); // returns a 1 if the new position of the food is "safe"
void clearScore(struct snake* players); // clears the game score
void checkPlayerOneCommands(struct snake* players, struct food* theFood, Sint16* slowdown);
void checkForCollisions(struct snake* someSnake, struct snake* players);
void validateScore(struct snake* players);

void jo_main(void)
{
    int i = 0;
    Uint16 data = 0;
    Sint16 slowdown = 0;
    struct snake players[MAX_PLAYERS] = {0};
    struct food theFood = {0};

    // Initializing functions
    slInitSystem(TV_320x240, NULL, 1); // Initializes screen

    displaySSMTFPresents(); // SSMTF logo
    drawGrid(); // Draws the playing field box

    displayText();
    titleScreen();

    srand(rand()); // set the random seed
    initializeFood(&theFood, '*');
    initializePlayerNumbers(players);
    clearScore(players);

    slowdown = INITIAL_SLOWDOWN;

    // game loop
    do
    {
        //
        // Check for special player one commands
        //
        checkPlayerOneCommands(players, &theFood, &slowdown);

        //
        // Draw existing players
        //
        for(i = 0; i < MAX_PLAYERS; i++)       
        {            
            data = Smpc_Peripheral[players[i].controllerNum].data;

            // Check if player pressed the A button and is not already plyaing
            if((data & PER_DGT_TA) == 0 && players[i].active == 0)
            {
                initializePlayer(&players[i], players[i].controllerNum);
            }

            if(players[i].active == 1)
            {
                drawSnake(&players[i], players);
            }
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
        
        //
        // Kill snakes that are marked for death
        //
        for(i = 0; i < MAX_PLAYERS; i++)
        {          
            if(players[i].active == 1 && players[i].dying == 1)
            {
                killPlayer(&players[i]);
            }
        }

        //
        // Draw the food
        //
        drawFood(&theFood, players);        
        
        //
        // Draw the grid again in case a Snake crashed into it
        //
        drawGrid();
        
        //
        // synch the screen
        //
        slSynch(); // You won't see anything without this!!
        for(i = 0; i < slowdown; i++)
        {
            slSynch(); // Slow down
        }

    }while(1);

    return;
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

void initializePlayer(struct snake* somePlayer, int controllerNum)
{
    char temp[10] = {0};    
    
    // Create player
    if(somePlayer->active != 1)
    {
        switch (somePlayer->ID)
        {
            case 0:
                somePlayer->shape[0] = (char)14; // block
                break;

            case 1:
                somePlayer->shape[0] = (char)35;	// pound sign
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
                somePlayer->head->y = 5;
                somePlayer->dir = 1;
                break;
                
            case 8:
                somePlayer->head->x = 9;
                somePlayer->head->y = 24;
                somePlayer->dir = 0;
                break;

            case 9:
                somePlayer->head->x = 29;
                somePlayer->head->y = 5;
                somePlayer->dir = 1;
                break;
                
            case 10:
                somePlayer->head->x = 19;
                somePlayer->head->y = 24;
                somePlayer->dir = 0;
                break;                

            case 11:
                somePlayer->head->x = 19;
                somePlayer->head->y = 5;
                somePlayer->dir = 1;
                break;
        }            

        // Initiate rest of snake to offscreen positions
        somePlayer->head->next->x = 50;
        somePlayer->head->next->y = 50;          

        somePlayer->head->next->next->x = 50;
        somePlayer->head->next->next->y = 50;
        
        somePlayer->head->next->next->next = NULL;

        somePlayer->tail = somePlayer->head->next->next;

        // Draw the starting position of the snake
        slPrint(somePlayer->shape, slLocate(somePlayer->head->x, somePlayer->head->y));
        
        somePlayer->active = 1;
        somePlayer->dying = 0;
    }
}

void checkPlayerOneCommands(struct snake* players, struct food* theFood, Sint16* slowdown)
{
    Uint16 data = 0;
    char tempArray[10] = {0};
    
    // Read the 1st player controller
    data = Smpc_Peripheral[0].data;

    // Did player decrease game speed
    if((data & PER_DGT_TL) == 0)
    {
        (*slowdown)++;
        if(*slowdown > MAX_SLOWDOWN)
        {
            *slowdown = MAX_SLOWDOWN;
        }
        sprintf(tempArray, "SD: %i  ", *slowdown);
        slPrint(tempArray, slLocate(32, 4));
    }

    // Did player increase game speed
    if((data & PER_DGT_TR) == 0)
    {
        (*slowdown)--;
        if(*slowdown < MIN_SLOWDOWN)
        {
            *slowdown = MIN_SLOWDOWN;
        }
        sprintf(tempArray, "SD: %i  ", *slowdown);
        slPrint(tempArray, slLocate(32, 4));
    }

    // Does the user want to see the score
    if((data & PER_DGT_ST) == 0)
    {
        displayScore(players);
        pressStart(players);
        clearScreen();
        redrawScreen(players, theFood);
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
    slPrint(top, slLocate(1, 5));
    slPrint(bottom, slLocate(1, 24));

    // Draw the sides
    sprintf(temp, "%c", 22);
    for(j = 6; j<24; j++)
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
    slPrint(bottomRightPit, slLocate(8,5));
    slPrint(topLeftPit, slLocate(8,4));
    slPrint(topPit, slLocate(9,4));
    slPrint(" ", slLocate(9,5));
    slPrint(topRightPit, slLocate(10,4));
    slPrint(bottomLeftPit, slLocate(10,5));

    slPrint(bottomRightPit, slLocate(18,5));
    slPrint(topLeftPit, slLocate(18,4));
    slPrint(topPit, slLocate(19,4));
    slPrint(" ", slLocate(19,5));
    slPrint(topRightPit, slLocate(20,4));
    slPrint(bottomLeftPit, slLocate(20,5));

    slPrint(bottomRightPit, slLocate(28,5));
    slPrint(topLeftPit, slLocate(28,4));
    slPrint(topPit, slLocate(29,4));
    slPrint(" ", slLocate(29,5));
    slPrint(topRightPit, slLocate(30,4));
    slPrint(bottomLeftPit, slLocate(30,5));

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

void checkForCollisions(struct snake* someSnake, struct snake* players)
{
    struct location* temp = NULL;    
    
    // Check collision with ceiling
    if(someSnake->head->y <= 5 && someSnake->dir != DIR_DOWN)
    {
        someSnake->dying = 1;        
        return;
    }
    
    // Check collision with floor
    if(someSnake->head->y >= 24 && someSnake->dir != DIR_UP)
    {
        someSnake->dying = 1;        
        return;
    } 
    
    
    // Check collision with left wall    
    if(someSnake->head->x <= 1 && someSnake->dir != DIR_RIGHT)
    {
        someSnake->dying = 1;        
        return;
    }
    
    // Check collision with right wall
    if(someSnake->head->x >= 38 && someSnake->dir != DIR_LEFT)
    {
        someSnake->dying = 1;        
        return;
    }
    
    // Check for collisions with yourself and other players
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        if(players[i].active==1)
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

void drawSnake(struct snake* someSnake, struct snake* players)
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
    slPrint(" ", slLocate(someSnake->tail->x, someSnake->tail->y));
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

    return;
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
    slPrint("Twelve Snakes Version 2.05 by Slinga", slLocate(1,1));
    slPrint("Press A to join", slLocate(1,2));
}

// Displays the text "Press Start" and waits for the user to hit start
void pressStart(struct snake* players)
{
    Uint16 data;    
    
    do{
        data = Smpc_Peripheral[0].data; // Checks if start button has been pressed        
        slSynch();

    }while((data & PER_DGT_ST) == 0);

    do{
        data = Smpc_Peripheral[0].data; // Checks if start button has been pressed
        slPrint("Press Start", slLocate(15, 22));
        
        // check if the user cleared the scores
        if((data & PER_DGT_TZ) == 0)
        {
            if(players != NULL)
            {
                clearScore(players);
                displayScore(players);
            }                
        }        
        
        slSynch();
        slSynch();
        slSynch();

    }while((data & PER_DGT_ST) != 0);
    
    // BUGBUG: hack in case the user is already holding down the start button
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
    // Decrease your score
    somePlayer->numDeaths++; 

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

    slPrint(" ", slLocate(snakeHead->x, snakeHead->y));
    free(snakeHead);
}

void initializeFood(struct food* theFood, char theShape)
{
    // Shape, and initial position of food
    theFood->shape[0] = theShape;
    theFood->shape[1] = '\0';
    
    // hardcode the first food to be in the center of the screen
    // afterwards we will seed the random number generator with the position
    // of our tail
    theFood->x = 19;
    theFood->y = 15;

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
                srand(players[i].tail->x + players[i].tail->y);

                do{
                    theFood->x = rand()%36 + 2;
                    theFood->y = rand()%18 + 6;
                }while(safeFood(theFood, players) != 1);

                slPrint(theFood->shape, slLocate(theFood->x, theFood->y));

                // Add a new segment, make that segment the tail
                players[i].tail->next = (struct location*)malloc(sizeof(struct location));
                players[i].tail->next->x = players->tail->x;
                players[i].tail->next->y = players->tail->y;
                players[i].tail->next->next = NULL;
                players[i].tail = players[i].tail->next;

                // You ate the apple, increase your score
                players[i].numApples++;
                return;
            }
        }
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
    Uint16 i;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        players[i].numApples = 0;
        players[i].numDeaths = 0;
        players[i].numKills = 0;
        players[i].score = 0;
    }
}

void validateScore(struct snake* players)
{
    Uint16 i;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        players[i].numApples = MIN(players[i].numApples, MAX_SCORE);
        players[i].numApples = MAX(players[i].numApples, MIN_SCORE);
        
        players[i].numDeaths = MIN(players[i].numDeaths, MAX_SCORE);
        players[i].numDeaths = MAX(players[i].numDeaths, MIN_SCORE);
        
        players[i].numKills = MIN(players[i].numKills, MAX_SCORE);
        players[i].numKills = MAX(players[i].numKills, MIN_SCORE);
        
        players[i].score = players[i].numApples + players[i].numKills - players[i].numDeaths;
        
        players[i].score = MIN(players[i].score, MAX_SCORE);
        players[i].score = MAX(players[i].score, MIN_SCORE);        
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

void displayScore(struct snake* players)
{
    char temp[50];
    Uint16 counter = 7;
    struct snake sortedPlayers[MAX_PLAYERS] = {0};

    temp[0] = '\0';
    
    validateScore(players);
    
    memcpy(sortedPlayers, players, sizeof(sortedPlayers));
    
    insertionSort(sortedPlayers);

    slPrint("P#       A#    K#    D#      S#", slLocate(4,counter++));
    slPrint("-------------------------------", slLocate(4,counter++));
    slPrint("                               ", slLocate(4,counter++));

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        // display the score if the player is currently active (or has ever been active)
        if(sortedPlayers[i].active == 1 || sortedPlayers[i].numDeaths > 0)
        {
            sprintf(temp, "%2i %c%c%c  %3i   %3i   %3i   = %3i", i+1, sortedPlayers[i].shape[0], sortedPlayers[i].shape[0], sortedPlayers[i].shape[0], sortedPlayers[i].numApples, sortedPlayers[i].numKills, sortedPlayers[i].numDeaths, sortedPlayers[i].score);
            slPrint(temp, slLocate(4,counter++));
        }
    }
}

void clearScreen()
{
    Uint16 i = 0;

    for(i = 6; i < 24; i++)
    {
        slPrint("                                   ", slLocate(3,i));
    }
}

void redrawScreen(struct snake* players, struct food* theFood)
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
                slPrint(players[i].shape, slLocate(temp->x, temp->y));
                temp = temp->next;
            }
        }
    }

    slPrint(theFood->shape, slLocate(theFood->x, theFood->y));
    
    drawGrid();
}

void titleScreen()
{
    slPrint("TTTTTT", slLocate(3,7));
    slPrint("  TT  ", slLocate(3,8));
    slPrint("  TT  ", slLocate(3,9));
    slPrint("  TT  ", slLocate(3,10));
    slPrint("  TT  ", slLocate(3,11));
    slPrint("  TT  ", slLocate(3,12));

    slPrint("W     W", slLocate(10,7));
    slPrint("W     W", slLocate(10,8));
    slPrint("W  W  W", slLocate(10,9));
    slPrint("WW W WW", slLocate(10,10));
    slPrint(" WWWWW ", slLocate(10,11));
    slPrint(" WW WW ", slLocate(10,12));

    slPrint("EEEE", slLocate(18,7));
    slPrint("E   ", slLocate(18,8));
    slPrint("EEEE", slLocate(18,9));
    slPrint("EEEE", slLocate(18,10));
    slPrint("E   ", slLocate(18,11));
    slPrint("EEEE", slLocate(18,12));

    slPrint("L  ", slLocate(23,7));
    slPrint("L  ", slLocate(23,8));
    slPrint("L  ", slLocate(23,9));
    slPrint("L  ", slLocate(23,10));
    slPrint("L  ", slLocate(23,11));
    slPrint("LLL", slLocate(23,12));

    slPrint("V   V", slLocate(26,7));
    slPrint("V   V", slLocate(26,8));
    slPrint("V   V", slLocate(26,9));
    slPrint("VV VV", slLocate(26,10));
    slPrint(" V V ", slLocate(26,11));
    slPrint(" VVV ", slLocate(26,12));

    slPrint("EEEE", slLocate(32,7));
    slPrint("E   ", slLocate(32,8));
    slPrint("EEEE", slLocate(32,9));
    slPrint("EEEE", slLocate(32,10));
    slPrint("E   ", slLocate(32,11));
    slPrint("EEEE", slLocate(32,12));

    slPrint("SSSS", slLocate(4,14));
    slPrint("S   ", slLocate(4,15));
    slPrint("S   ", slLocate(4,16));
    slPrint("SSSS", slLocate(4,17));
    slPrint("   S", slLocate(4,18));
    slPrint("   S", slLocate(4,19));
    slPrint("SSSS", slLocate(4,20));

    slPrint("N   N", slLocate(9,14));
    slPrint("NN  N", slLocate(9,15));
    slPrint("N N N", slLocate(9,16));
    slPrint("N NNN", slLocate(9,17));
    slPrint("N  NN", slLocate(9,18));
    slPrint("N   N", slLocate(9,19));
    slPrint("N   N", slLocate(9,20));

    slPrint("  A  ", slLocate(15,14));
    slPrint(" AAA ", slLocate(15,15));
    slPrint("AA AA", slLocate(15,16));
    slPrint("A   A", slLocate(15,17));
    slPrint("AAAAA", slLocate(15,18));
    slPrint("A   A", slLocate(15,19));
    slPrint("A   A", slLocate(15,20));

    slPrint("K  K", slLocate(21,14));
    slPrint("K  K", slLocate(21,15));
    slPrint("K K ", slLocate(21,16));
    slPrint("KKK ", slLocate(21,17));
    slPrint("K K ", slLocate(21,18));
    slPrint("K  K", slLocate(21,19));
    slPrint("K  K", slLocate(21,20));

    slPrint("EEEE", slLocate(26,14));
    slPrint("E   ", slLocate(26,15));
    slPrint("E   ", slLocate(26,16));
    slPrint("EEEE", slLocate(26,17));
    slPrint("E   ", slLocate(26,18));
    slPrint("E   ", slLocate(26,19));
    slPrint("EEEE", slLocate(26,20));

    slPrint("SSSS", slLocate(31,14));
    slPrint("S   ", slLocate(31,15));
    slPrint("S   ", slLocate(31,16));
    slPrint("SSSS", slLocate(31,17));
    slPrint("   S", slLocate(31,18));
    slPrint("   S", slLocate(31,19));
    slPrint("SSSS", slLocate(31,20));

    slPrint("Dedicated to the man with one knee", slLocate(3,23));

    pressStart(NULL);
    clearScreen();
}
