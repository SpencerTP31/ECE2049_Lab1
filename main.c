/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/

#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"

typedef struct Alien
{
    unsigned char key;
    int x;
    int y;
    bool visible;
} Alien;

typedef enum State
{
    WELCOME_SCREEN,
    WAIT_FOR_START,
    START_COUNT_DOWN,
    COUNT_DOWN_SCREEN,
    NEXT_LEVEL_SCREEN,
    START_LEVEL,
    PLAYING_GAME,
    LOSS_SCREEN
} State;

// Function Prototypes
void drawWelcome();
void drawLoss();
void drawNextLevel(int);
bool drawCountdown(int, int);
void drawAliens(Alien*, int);

// Declare globals here

// Main
void main(void)
{
    int i;
    int levelBottom = 85; // The lowest the top of the alien character can go before you lose the level
    unsigned char keyPressed = 0, lastKeyPressed = 0;
    unsigned long int mainCounter = 0, auxCounter = 0;
    State state = START_LEVEL;
    Alien aliens[50]; //Alien* aliens = (Alien*) malloc(50 * sizeof(Alien));
    int alienCount = 0;
    int level = 1;

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

    initLeds();
    configDisplay();
    configKeypad();

    while (1)
    {
        keyPressed = getKey();
        switch (state)
        {
        case WELCOME_SCREEN:
            drawWelcome();
            state = WAIT_FOR_START;
            break;
        case WAIT_FOR_START:
            if (keyPressed == '*')
            {
                state = START_COUNT_DOWN;
            }
            break;
        case START_COUNT_DOWN:
            auxCounter = mainCounter;
            state = COUNT_DOWN_SCREEN;
        case COUNT_DOWN_SCREEN:
            if (drawCountdown(mainCounter, auxCounter))
            {
                state = START_LEVEL;
            }
            break;
        case NEXT_LEVEL_SCREEN:
            drawNextLevel(level);
            state = WAIT_FOR_START;
            break;
        case START_LEVEL:
            // Create aliens based on level number (minimum of one, maximum of 50)
            alienCount = ((rand() % (level * level)) + 1) % 50;

            // Height tracker is used to tell if there is already a current alien in the row
            int heightTracker[5] = { 0, 0, 0, 0, 0 };

            int currentRow = 0;

            for (i = 0; i < alienCount; i++)
            {
                int currentColumn = rand() % 5;

                if (heightTracker[currentColumn])
                {
                    // There's already an alien in this row of this column
                    currentRow++;
                    int j;
                    for (j = 0; j < 5; j++)
                    {
                        heightTracker[j] = 0;
                    }
                }
                heightTracker[currentColumn] = heightTracker[currentColumn] + 1; // Can't use ++ or += because that doesn't modify the value in the array (I think)

                aliens[i].x = 15 * (currentColumn + 1) + 3;
                aliens[i].y = 10 - (20 * currentRow);
                aliens[i].key = "01234"[currentColumn];
                aliens[i].visible = true;
            }
            state = PLAYING_GAME;
            break;
        case PLAYING_GAME:
            Graphics_clearDisplay(&g_sContext);

            // Check if key is pressed to destroy each alien
            // If yes, destroy the lowest one with the pressed key
            // If all aliens are destroyed, increment level counter and move to START_LEVEL
            if (keyPressed != 0 && lastKeyPressed != keyPressed)
            {
                for (i = 0; i < alienCount; i++)
                {
                    Alien currentAlien = aliens[i];
                    if (currentAlien.visible && currentAlien.key == keyPressed)
                    {
                        // Note the direct access to the struct in the array
                        aliens[i].visible = false;

                        bool anyVisible;

                        // It's okay to hijack i here because we're about to break out of the loop anyway
                        for (i = 0, anyVisible = false; i < alienCount; i++)
                        {
                            if (aliens[i].visible)
                            {
                                anyVisible = true;
                                break; // May as well short circuit, we only care if the level is still going on or not
                            }
                        }
                        if (!anyVisible)
                        {
                            level++;
                            state = NEXT_LEVEL_SCREEN;
                        }
                        break;
                    }
                }
            }

            // If an alien reaches the bottom, move to LOSS
            for (i = 0; i < alienCount; i++)
            {
                Alien currentAlien = aliens[i];
                if (currentAlien.visible)
                {
                    if (currentAlien.y >= levelBottom)
                    {
                        state = LOSS_SCREEN;
                    }
                    aliens[i].y += level / 2 + 1; // This is how fast the aliens descend
                }
            }

            drawAliens(aliens, alienCount);
            break;
        case LOSS_SCREEN:
            drawLoss();
            state = WAIT_FOR_START;
            level = 1; // Reset the level so that you start at the beginning of the game
            break;
        }
        mainCounter++;
        lastKeyPressed = keyPressed;
    }
}

void drawWelcome()
{
    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext);                // Clear the display

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, "SPACE", AUTO_STRING_LENGTH, 48,
                                25, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "INVADERS", AUTO_STRING_LENGTH, 48,
                                35, TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48,
                                70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to Start", AUTO_STRING_LENGTH, 48,
                                80, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = { .xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
    Graphics_drawRectangle(&g_sContext, &box);

    // We are now done writing to the display.  However, if we stopped here, we would not
    // see any changes on the actual LCD.  This is because we need to send our changes
    // to the LCD, which then refreshes the display.
    // Since this is a slow operation, it is best to refresh (or "flush") only after
    // we are done drawing everything we need.
    Graphics_flushBuffer(&g_sContext);
}

void drawLoss()
{
    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext);                // Clear the display

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, "You Lost.", AUTO_STRING_LENGTH,
                                48, 25, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, ">:(", AUTO_STRING_LENGTH, 48, 35,
    TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48,
                                70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to Try Again", AUTO_STRING_LENGTH,
                                48, 80, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = { .xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
    Graphics_drawRectangle(&g_sContext, &box);

    Graphics_flushBuffer(&g_sContext);
}

void drawNextLevel(int level)
{

    Graphics_clearDisplay(&g_sContext);

    char buffer[10];
    snprintf(buffer, 9, "Level %d", level);

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, buffer, AUTO_STRING_LENGTH, 48, 35,
                                TRANSPARENT_TEXT);

    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48,
                                70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to Begin", AUTO_STRING_LENGTH, 48,
                                80, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = { .xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
    Graphics_drawRectangle(&g_sContext, &box);

    Graphics_flushBuffer(&g_sContext);
}

bool drawCountdown(int mainCounter, int auxCounter)
{
    if (auxCounter == mainCounter)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48,
                                    50, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }
    else if (mainCounter == auxCounter + 4000)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48,
                                    50, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }
    else if (mainCounter == auxCounter + 8000)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48,
                                    50, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }
    else if (mainCounter == auxCounter + 12000)
    {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_flushBuffer(&g_sContext);
        return true;
    }
    return false;
}

void drawAliens(Alien* aliens, int alienCount)
{
    int i;
    for (i = 0; i < alienCount; i++)
    {
        Alien currentAlien = aliens[i];
        if (currentAlien.visible && currentAlien.y >= 0)
        {
            unsigned char* str = &(currentAlien.key);
            Graphics_drawStringCentered(&g_sContext, str, 1, currentAlien.x,
                                        currentAlien.y, TRANSPARENT_TEXT);
        }
    }
    Graphics_flushBuffer(&g_sContext);
}

