#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "lil-tetris-patterns.c"

// Constants
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define FPS 60.0f

#define GRID_HEIGHT 20
#define GRID_WIDTH 10
#define GRID_UPPER_X 200
#define GRID_UPPER_Y 50
#define GRID_CELL_WIDTH 20
#define GRID_CELL_HEIGHT 20

#define NEXT_PATTERN_X 500
#define NEXT_PATTERN_Y 100
#define NEXT_PATTERN_BORDER 5

#define SPAWN_DELAY_FRAMES 40
#define CLEAR_LINES_FRAMES 60
#define CLEAR_LINES_FLASH_DURATION 10

#define START_DROP_SPEED 2

// Types
typedef struct
{
    Uint8 r, g, b;
} Color;

typedef struct
{
    Uint8      x;
    Uint8      y;
    PatternType_t patternType;
} GridCell;

typedef struct
{
    SDL_Rect Rects[GRID_HEIGHT * GRID_WIDTH];
    Uint8 NumRects;
} SDLRectArray;

typedef struct
{
    // One for each cell type
    SDLRectArray RectArrays[(int)PATTERN_MAX_VALUE];
} SDLRectArrays;

typedef struct
{
    PatternType_t nextPatternType;
    PatternType_t currentPatternType;
    Uint8      patternGridX;
    Uint8      patternGridY;
    Uint8      currentPatternRotation;
    Uint64     currentFrame;
    Uint64     lastDropFrame;
    Uint64     lastSpawnFrame;
    Uint8      dropSpeed;
    Sint8      clearLines[4];
    Uint64     clearLinesFrame;
    bool       inputLeftPressed;
    bool       inputRightPressed;
    bool       inputDownPressed;
    bool       inputUpPressed;
    bool       inputRotateRightPressed;
    bool       inputRotateLeftPressed;
    bool       toggleFlash;
} GameState;

// Globals
static GameState g_GameState;
static GridCell g_Grid[GRID_HEIGHT][GRID_WIDTH];
static SDLRectArrays g_RectArrays;
static Color g_CellColors[(int)PATTERN_MAX_VALUE] = {
    { 0  ,   0,   0 }, // PATTERN_NONE,
    { 100,  50,  50 }, // PATTERN_L_L,
    { 100,  50,  50 }, // PATTERN_L_R,
    { 150, 150,   0 }, // PATTERN_Z_L,
    { 150, 150,   0 }, // PATTERN_Z_R,
    { 150, 150, 100 }, // PATTERN_T_SHAPE,
    { 200, 100, 100 }, // PATTERN_LINE_SHAPE,
    {  50, 200, 250 }, // PATTERN_SQUARE_SHAPE,
};
static Pattern** g_PatternLUT[(int)PATTERN_MAX_VALUE] = {
    EmptyPatternRotations,
    LPatternLeftRotations,
    LPatternRightRotations,
    ZPatternLeftRotations,
    ZPatternRightRotations,
    TPatternRotations,
    LinePatternRotations,
    SquarePatternRotations,
};

PatternType_t randomPatternType()
{
    return (PatternType_t)(rand() % (int)(PATTERN_MAX_VALUE - 1) + 1);
}

void resetInputStates()
{
    g_GameState.inputLeftPressed = false;
    g_GameState.inputRightPressed = false;
    g_GameState.inputDownPressed = false;
    g_GameState.inputUpPressed = false;
    g_GameState.inputRotateRightPressed = false;
    g_GameState.inputRotateLeftPressed = false;
}

void initializeGameState()
{
    g_GameState.nextPatternType = randomPatternType();
    g_GameState.currentPatternType = randomPatternType();
    g_GameState.currentPatternRotation = 0;
    g_GameState.patternGridX = 
        (GRID_WIDTH / 2) - 
        (g_PatternLUT[g_GameState.currentPatternType][0]->cols / 2);
    g_GameState.patternGridY = 0;
    g_GameState.currentFrame = 0;
    g_GameState.lastDropFrame = 0;
    g_GameState.lastSpawnFrame = 0;
    g_GameState.dropSpeed = START_DROP_SPEED; // Drops per second
    memset(g_GameState.clearLines, -1, sizeof(g_GameState.clearLines));
    g_GameState.clearLinesFrame = 0;
    g_GameState.toggleFlash = false;

    resetInputStates();
}

Pattern* getCurrentPattern()
{
    return
        g_PatternLUT[g_GameState.currentPatternType][g_GameState.currentPatternRotation];
}

bool patternCollides(Pattern* pPattern, Sint8 dX, Sint8 dY)
{
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                Sint8 gridX = x + g_GameState.patternGridX + dX;
                Sint8 gridY = y + g_GameState.patternGridY + dY;

                // Collides with bottom?
                if (gridY >= GRID_HEIGHT)
                {
                    return true;
                }

                // Collides with left?
                if (gridX < 0)
                {
                    return true;
                }

                // Collides with right?
                if (gridX >= GRID_WIDTH)
                {
                    return true;
                }

                // Collides with committed cells?
                if (g_Grid[gridY][gridX].patternType != PATTERN_NONE)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void commitAndSpawnPattern()
{
    Pattern* pPattern = getCurrentPattern();
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                Uint8 gridX = x + g_GameState.patternGridX;
                Uint8 gridY = y + g_GameState.patternGridY;
                g_Grid[gridY][gridX].patternType = g_GameState.currentPatternType;
            }
        }
    }

    g_GameState.currentPatternType = g_GameState.nextPatternType;
    g_GameState.nextPatternType = randomPatternType();
    g_GameState.currentPatternRotation = 0;
    g_GameState.patternGridX = 
        (GRID_WIDTH / 2) - 
        (getCurrentPattern()->cols / 2);
    g_GameState.patternGridY = 0;

    g_GameState.lastSpawnFrame = g_GameState.currentFrame;
}

void updateGameState()
{
    Uint64 sinceLastDrop = g_GameState.currentFrame - g_GameState.lastDropFrame;
    Uint64 sinceLastSpawn = g_GameState.currentFrame - g_GameState.lastSpawnFrame;
    Uint64 dropFrameTarget = (FPS / g_GameState.dropSpeed);

    const bool WaitingForSpawn = sinceLastSpawn < SPAWN_DELAY_FRAMES;
    if (WaitingForSpawn)
    {
        g_GameState.currentFrame++;
        return;
    }

    const bool ClearingLines = g_GameState.clearLines[0] >= 0;
    if (ClearingLines)
    {
        Uint64 sinceClearedLines = g_GameState.currentFrame - g_GameState.clearLinesFrame;
        if (sinceClearedLines >= CLEAR_LINES_FRAMES)
        {
            // Nuke out the cleared lines
            for (int i = 0; i < 4; ++i)
            {
                Sint8 y = g_GameState.clearLines[i];
                if (y < 0)
                {
                    break;
                }

                for (int x = 0; x < GRID_WIDTH; ++x)
                {
                    g_Grid[y][x].patternType = PATTERN_NONE;
                }
            }

            // Collapse the cleared lines
            Sint8 lastNonClearY = -1;
            Sint8 lastClearY = -1;
            for (int y = 0; y < GRID_HEIGHT; ++y)
            {
                // Is the current line clear?
                bool isClear = true;
                for (int x = 0; x < GRID_WIDTH; ++x)
                {
                    if (g_Grid[y][x].patternType != PATTERN_NONE)
                    {
                        isClear = false;
                        break;
                    }
                }

                if (!isClear || y == (GRID_HEIGHT - 1))
                {
                    if (y == (GRID_HEIGHT - 1))
                    {
                        // Treat the last line as the last clear Y
                        lastClearY = y;
                    }

                    // If this line isn't clear, and the previous line is, we may have
                    // non-clear lines to copy across a gap of clear lines
                    if (lastClearY == y - 1 && lastNonClearY >= 0 || y == (GRID_HEIGHT - 1))
                    {
                        Sint8 src = lastNonClearY;
                        Sint8 dst = lastClearY;

                        while(src >= 0) 
                        {
                            // Copy src line to dst line
                            for (int x = 0; x < GRID_WIDTH; ++x)
                            {
                                g_Grid[dst][x].patternType = g_Grid[src][x].patternType;
                            }
                            --src;
                            --dst;
                        }
                    }
                    lastNonClearY = y;
                }
                else
                {
                    lastClearY = y;
                }
            }

            memset(g_GameState.clearLines, -1, sizeof(g_GameState.clearLines));
        }

        g_GameState.currentFrame++;
        return;
    }

    // Check for natural drops, player-induced drops or quick drops
    if (dropFrameTarget <= sinceLastDrop || g_GameState.inputDownPressed)
    {
        if (!patternCollides(getCurrentPattern(), 0, 1))
        {
            g_GameState.patternGridY++;
        }
        else
        {
            commitAndSpawnPattern();
        }

        g_GameState.lastDropFrame = g_GameState.currentFrame;
    }
    else if (g_GameState.inputUpPressed)
    {
        // Figure out how far to drop the current pattern
        int patternHeight = 1;
        while (!patternCollides(getCurrentPattern(), 0, patternHeight))
        {
            ++patternHeight;
        }

        g_GameState.patternGridY += patternHeight - 1;
        commitAndSpawnPattern();

        g_GameState.lastDropFrame = g_GameState.currentFrame;
    }

    // Checking cleared lines or lose condition if there was a drop
    if (g_GameState.lastDropFrame == g_GameState.currentFrame)
    {
        // TODO: check for losing condition
        
        memset(g_GameState.clearLines, -1, sizeof(g_GameState.clearLines));

        // Check for cleared lines
        Uint8 linesCleared = 0;
        for (int y = 0; y < GRID_HEIGHT; ++y) {
            bool lineCleared = true;
            for (int x = 0; x < GRID_WIDTH; ++x) {
                if (g_Grid[y][x].patternType == PATTERN_NONE)
                {
                    lineCleared = false;
                    break;
                }
            }

            if (lineCleared)
            {
                g_GameState.clearLines[linesCleared] = y;
                ++linesCleared;
            }
        }

        if (linesCleared > 0)
        {
            g_GameState.clearLinesFrame = g_GameState.currentFrame;
        }
    }

    // Handle player inputs
    Pattern* pPattern = getCurrentPattern();
    if (g_GameState.inputLeftPressed && !g_GameState.inputRightPressed)
    {
        if (!patternCollides(getCurrentPattern(), -1, 0))
        {
            g_GameState.patternGridX--;
        }
    }
    else if (g_GameState.inputRightPressed && !g_GameState.inputLeftPressed)
    {
        if (!patternCollides(getCurrentPattern(), 1, 0))
        {
            g_GameState.patternGridX++;
        }
    }
    
    if (g_GameState.inputRotateRightPressed && !g_GameState.inputRotateLeftPressed)
    {
        int numRotations = PatternNumRotations[g_GameState.currentPatternType];
        int rotationIndex = (g_GameState.currentPatternRotation + 1) % numRotations;

        Pattern* pRotatedPattern = g_PatternLUT[g_GameState.currentPatternType][rotationIndex];
        if (!patternCollides(pRotatedPattern, 0, 0))
        {
            g_GameState.currentPatternRotation = rotationIndex;
        }
    }
    else if (g_GameState.inputRotateLeftPressed && !g_GameState.inputRotateRightPressed)
    {
        int numRotations = PatternNumRotations[g_GameState.currentPatternType];
        int rotationIndex = 
            !g_GameState.currentPatternRotation ? 
                numRotations : 
                g_GameState.currentPatternRotation - 1;
        Pattern* pRotatedPattern = g_PatternLUT[g_GameState.currentPatternType][rotationIndex];
        if (!patternCollides(pRotatedPattern, 0, 0))
        {
            g_GameState.currentPatternRotation = rotationIndex;
        }
    }

    g_GameState.currentFrame++;
}

void initializeGrid()
{
    memset(g_Grid, 0, sizeof(g_Grid));
    for (Uint8 y = 0; y < GRID_HEIGHT; ++y) {
        for (Uint8 x = 0; x < GRID_WIDTH; ++x) {
            GridCell* pCell = &g_Grid[y][x];
            pCell->x = x;
            pCell->y = y;
            pCell->patternType = PATTERN_NONE;
        }
    }

}

void renderPattern(SDL_Renderer* pRenderer)
{
    // Don't render the current pattern if we're still waiting to spawn
    Uint64 sinceLastSpawn = g_GameState.currentFrame - g_GameState.lastSpawnFrame;
    if (sinceLastSpawn < SPAWN_DELAY_FRAMES && g_GameState.lastSpawnFrame > 0)
    {
        return;
    }

    PatternType_t patternType = g_GameState.currentPatternType;
    Pattern* pPattern = getCurrentPattern();

    int toDrawIndex = 0;
    SDL_Rect toDraw[4];
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                SDL_Rect* pRect = &toDraw[toDrawIndex];
                pRect->x = (x + g_GameState.patternGridX) * GRID_CELL_WIDTH + GRID_UPPER_X;
                pRect->y = (y + g_GameState.patternGridY) * GRID_CELL_HEIGHT + GRID_UPPER_Y;
                pRect->w = GRID_CELL_WIDTH;
                pRect->h = GRID_CELL_HEIGHT;

                ++toDrawIndex;
            }
        }
    }

    assert(toDrawIndex == 4);
    Color* pColor = &g_CellColors[g_GameState.currentPatternType];
    SDL_SetRenderDrawColor(
        pRenderer,
        pColor->r,
        pColor->g,
        pColor->b,
        SDL_ALPHA_OPAQUE);

    SDL_RenderFillRects(pRenderer, toDraw, toDrawIndex);
}

void renderNextPattern(SDL_Renderer* pRenderer)
{
    PatternType_t patternType = g_GameState.nextPatternType;
    Pattern* pPattern = g_PatternLUT[g_GameState.nextPatternType][0];

    // Draw the bg first
    Color black = { 0, 0, 0 };
    SDL_Rect previewBgRect;
    previewBgRect.x = NEXT_PATTERN_X - NEXT_PATTERN_BORDER;
    previewBgRect.y = NEXT_PATTERN_Y - NEXT_PATTERN_BORDER;
    previewBgRect.w = NEXT_PATTERN_BORDER * 2 + 4 * GRID_CELL_WIDTH;
    previewBgRect.h = NEXT_PATTERN_BORDER * 2 + 4 * GRID_CELL_HEIGHT;

    SDL_SetRenderDrawColor(
        pRenderer,
        black.r,
        black.g,
        black.b,
        SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(pRenderer, &previewBgRect);

    // Now draw the pattern
    int offsetX = ((4 - pPattern->cols) * GRID_CELL_WIDTH) / 2;
    int offsetY = ((4 - pPattern->rows) * GRID_CELL_HEIGHT) / 2;
    
    int toDrawIndex = 0;
    SDL_Rect toDraw[4];
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                SDL_Rect* pRect = &toDraw[toDrawIndex];
                pRect->x = x * GRID_CELL_WIDTH + NEXT_PATTERN_X + offsetX;
                pRect->y = y * GRID_CELL_HEIGHT + NEXT_PATTERN_Y + offsetY;
                pRect->w = GRID_CELL_WIDTH;
                pRect->h = GRID_CELL_HEIGHT;

                ++toDrawIndex;
            }
        }
    }

    assert(toDrawIndex == 4);
    Color* pColor = &g_CellColors[g_GameState.nextPatternType];
    SDL_SetRenderDrawColor(
        pRenderer,
        pColor->r,
        pColor->g,
        pColor->b,
        SDL_ALPHA_OPAQUE);
    SDL_RenderFillRects(pRenderer, toDraw, toDrawIndex);
}

void renderGrid(SDL_Renderer* pRenderer) 
{
    // Reset the rect array rect counts
    for (int rectType = 0; rectType < (int)PATTERN_MAX_VALUE; ++rectType) {
        SDLRectArray* pArray = &g_RectArrays.RectArrays[rectType];
        pArray->NumRects = 0;
    }

    // Populate SDL Rect Arrays data structures for rendering
    for (Uint8 y = 0; y < GRID_HEIGHT; ++y) {
        for (Uint8 x = 0; x < GRID_WIDTH; ++x) {
            GridCell* pCell = &g_Grid[y][x];
            assert((int)pCell->patternType >= 0);
            assert((int)pCell->patternType < PATTERN_MAX_VALUE);

            SDLRectArray* pRectArray = &g_RectArrays.RectArrays[pCell->patternType];

            int rectIndex = pRectArray->NumRects;
            SDL_Rect* pRect = &pRectArray->Rects[rectIndex];

            pRect->x = pCell->x * GRID_CELL_WIDTH + GRID_UPPER_X;
            pRect->y = pCell->y * GRID_CELL_HEIGHT + GRID_UPPER_Y;
            pRect->w = GRID_CELL_WIDTH;
            pRect->h = GRID_CELL_HEIGHT;

            pRectArray->NumRects++;
        }
    }

    for (int rectType = 0; rectType < (int)PATTERN_MAX_VALUE; ++rectType) {
        Color* pColor = &g_CellColors[rectType];
        SDL_SetRenderDrawColor(
            pRenderer,
            pColor->r,
            pColor->g,
            pColor->b,
            SDL_ALPHA_OPAQUE);

        SDLRectArray* pArray = &g_RectArrays.RectArrays[rectType];
        SDL_RenderFillRects(pRenderer, pArray->Rects, pArray->NumRects);
    }

    // Render flashing lines if we're clearing lines
    const bool ClearingLines = g_GameState.clearLines[0] >= 0;
    Uint64 sinceClearedLines = g_GameState.currentFrame - g_GameState.clearLinesFrame;
    if (sinceClearedLines < CLEAR_LINES_FRAMES)
    {
        if ((sinceClearedLines % CLEAR_LINES_FLASH_DURATION) == 0)
        {
            g_GameState.toggleFlash = g_GameState.toggleFlash ? false : true;
        }

        if (g_GameState.toggleFlash)
        {
            // At most we have GRID_WIDTH * 4 rects to render
            SDL_Rect FlashRects[GRID_WIDTH * 4];
            int rectIndex = 0;
            int clearLineIndex = 0;
            int y = g_GameState.clearLines[clearLineIndex];
            while (y >= 0)
            {
                for (int x = 0; x < GRID_WIDTH; ++x)
                {
                    GridCell* pCell = &g_Grid[y][x];
                    SDL_Rect* pRect = &FlashRects[rectIndex];
                    pRect->x = pCell->x * GRID_CELL_WIDTH + GRID_UPPER_X;
                    pRect->y = pCell->y * GRID_CELL_HEIGHT + GRID_UPPER_Y;
                    pRect->w = GRID_CELL_WIDTH;
                    pRect->h = GRID_CELL_HEIGHT;
                    ++rectIndex;
                }
                clearLineIndex++;
                y = g_GameState.clearLines[clearLineIndex];
            }

            Color white = { 255, 255, 255 };
            SDL_SetRenderDrawColor(
                pRenderer,
                white.r,
                white.g,
                white.b,
                SDL_ALPHA_OPAQUE);

            SDL_RenderFillRects(pRenderer, FlashRects, rectIndex);
        }
    }
}

int main(int argc, char** argv)
{
    SDL_Window* pWindow = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Failed to initialize SDL2: %s\n", SDL_GetError());
        return -1;
    }

    pWindow = SDL_CreateWindow(
            "lil-tetris",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN);
    if (!pWindow)
    {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Renderer* pRender = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!pRender)
    {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        return -1;
    }

    srand(time(NULL));

    initializeGameState();
    initializeGrid();

    Color clearColor;
    clearColor.r = 100;
    clearColor.g = 100;
    clearColor.b = 100;

    bool shouldQuit = false;
    while (!shouldQuit)
    {
        SDL_SetRenderDrawColor(
            pRender,
            clearColor.r,
            clearColor.g,
            clearColor.b,
            SDL_ALPHA_OPAQUE);
        SDL_RenderClear(pRender);

        resetInputStates();

        SDL_Event event;
        while(SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                shouldQuit = true;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE || 
                    event.key.keysym.sym == SDLK_q)
                {
                    shouldQuit = true;
                }
                else if (event.key.keysym.sym == SDLK_w || 
                         event.key.keysym.sym == SDLK_UP)
                {
                    g_GameState.inputUpPressed = true;
                }
                else if (event.key.keysym.sym == SDLK_s || 
                         event.key.keysym.sym == SDLK_DOWN)
                {
                    g_GameState.inputDownPressed = true;
                }
                else if (event.key.keysym.sym == SDLK_a || 
                         event.key.keysym.sym == SDLK_LEFT)
                {
                    g_GameState.inputLeftPressed = true;
                }
                else if (event.key.keysym.sym == SDLK_d || 
                         event.key.keysym.sym == SDLK_RIGHT)
                {
                    g_GameState.inputRightPressed = true;
                }
                else if (event.key.keysym.sym == SDLK_k)
                {
                    g_GameState.inputRotateRightPressed = true;
                }
                else if (event.key.keysym.sym == SDLK_j)
                {
                    g_GameState.inputRotateRightPressed = true;
                }
            }
        }

        // Main loop
        Uint64 startTime = SDL_GetPerformanceCounter();

        updateGameState();
        renderGrid(pRender);
        renderPattern(pRender);
        renderNextPattern(pRender);

        Uint64 endTime = SDL_GetPerformanceCounter();

        float elapsedMs = (endTime - startTime) / 
            (float)SDL_GetPerformanceFrequency() * 1000.0f; 
        float expectedMs = (1.0f / FPS) * 1000.0f;

        SDL_RenderPresent(pRender);
        SDL_Delay(expectedMs - elapsedMs);
    }

    SDL_DestroyRenderer(pRender);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();

    return 0;
} 
