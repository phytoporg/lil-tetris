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

#define SPAWN_DELAY_FRAMES 60

#define START_DROP_SPEED 8

// Types
typedef enum
{
    CELL_EMPTY,
    CELL_L_SHAPE_L,
    CELL_L_SHAPE_R,
    CELL_Z_SHAPE_L,
    CELL_Z_SHAPE_R,
    CELL_T_SHAPE,
    CELL_LINE_SHAPE,
    CELL_SQUARE_SHAPE,
    CELL_MAX_VALUE
} CellType_t;

typedef struct
{
    Uint8 r, g, b;
} Color;

typedef struct
{
    Uint8      x;
    Uint8      y;
    CellType_t cellType;
} GridCell;

typedef struct
{
    SDL_Rect Rects[GRID_HEIGHT * GRID_WIDTH];
    Uint8 NumRects;
} SDLRectArray;

typedef struct
{
    // One for each cell type
    SDLRectArray RectArrays[(int)CELL_MAX_VALUE];
} SDLRectArrays;

typedef struct
{
    CellType_t nextCellType;
    CellType_t currentCellType;
    Uint8      patternGridX;
    Uint8      patternGridY;
    Uint64     currentFrame;
    Uint64     lastDropFrame;
    Uint64     lastSpawnFrame;
    Uint8      dropSpeed;
} GameState;

// Globals
static GameState g_GameState;
static GridCell g_Grid[GRID_HEIGHT][GRID_WIDTH];
static SDLRectArrays g_RectArrays;
static Color g_CellColors[(int)CELL_MAX_VALUE] = {
    { 0  ,   0,   0 }, // CELL_EMPTY,
    { 100,  50,  50 }, // CELL_L_SHAPE_L,
    { 100,  50,  50 }, // CELL_L_SHAPE_R,
    { 150, 150,   0 }, // CELL_Z_SHAPE_L,
    { 150, 150,   0 }, // CELL_Z_SHAPE_R,
    { 150, 150, 100 }, // CELL_T_SHAPE,
    { 200, 100, 100 }, // CELL_LINE_SHAPE,
    {  50, 200, 250 }, // CELL_SQUARE_SHAPE,
};
static Pattern g_PatternLUT[(int)CELL_MAX_VALUE];

CellType_t GetNextCellType()
{
    return (CellType_t)(rand() % (int)(CELL_MAX_VALUE - 1) + 1);
}

void initializePatternLUT()
{
    g_PatternLUT[CELL_EMPTY]        = EmptyPattern;
    g_PatternLUT[CELL_L_SHAPE_L]    = LShapePatternLeft;
    g_PatternLUT[CELL_L_SHAPE_R]    = LShapePatternRight;
    g_PatternLUT[CELL_Z_SHAPE_L]    = ZShapePatternLeft;
    g_PatternLUT[CELL_Z_SHAPE_R]    = ZShapePatternRight;
    g_PatternLUT[CELL_T_SHAPE]      = TShapePattern;
    g_PatternLUT[CELL_LINE_SHAPE]   = LineShapePattern;
    g_PatternLUT[CELL_SQUARE_SHAPE] = SquareShapePattern;
}

void initializeGameState()
{
    g_GameState.nextCellType = GetNextCellType();
    g_GameState.currentCellType = GetNextCellType();
    g_GameState.patternGridX = 
        (GRID_WIDTH / 2) - 
        (g_PatternLUT[g_GameState.currentCellType].cols / 2);
    g_GameState.patternGridY = 0;
    g_GameState.currentFrame = 0;
    g_GameState.lastDropFrame = 0;
    g_GameState.lastSpawnFrame = 0;
    g_GameState.dropSpeed = START_DROP_SPEED; // Drops per second
}

bool currentPatternCollides()
{
    Pattern* pPattern = &g_PatternLUT[g_GameState.currentCellType];
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                Uint8 gridX = x + g_GameState.patternGridX;
                Uint8 gridY = y + g_GameState.patternGridY;

                if (gridY + 1 >= GRID_HEIGHT)
                {
                    return true;
                }

                if (g_Grid[gridY + 1][gridX].cellType != CELL_EMPTY)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void updateGameState()
{
    Uint64 sinceLastDrop = g_GameState.currentFrame - g_GameState.lastDropFrame;
    Uint64 dropFrameTarget = (FPS / g_GameState.dropSpeed);
    Pattern* pPattern = &g_PatternLUT[g_GameState.currentCellType];

    Uint8 patternBottomY = g_GameState.patternGridY + pPattern->rows;
    if (dropFrameTarget <= sinceLastDrop)
    {
        if (!currentPatternCollides())
        {
            g_GameState.patternGridY++;
        }
        else
        {
            // Commit this pattern and spawn a new one
            for (int y = 0; y < pPattern->rows; ++y) {
                for (int x = 0; x < pPattern->cols; ++x) {
                    if (pPattern->occupancy[y][x])
                    {
                        Uint8 gridX = x + g_GameState.patternGridX;
                        Uint8 gridY = y + g_GameState.patternGridY;
                        g_Grid[gridY][gridX].cellType = g_GameState.currentCellType;
                    }
                }
            }

            g_GameState.currentCellType = g_GameState.nextCellType;
            g_GameState.nextCellType = GetNextCellType();
            g_GameState.patternGridX = 
                (GRID_WIDTH / 2) - 
                (g_PatternLUT[g_GameState.currentCellType].cols / 2);
            g_GameState.patternGridY = 0;
        }

        g_GameState.lastDropFrame = g_GameState.currentFrame;
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
            pCell->cellType = CELL_EMPTY;
        }
    }

}

void renderPattern(SDL_Renderer* pRenderer)
{
    CellType_t cellType = g_GameState.currentCellType;
    Pattern* pPattern = &g_PatternLUT[cellType];

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

    // fprintf(stdout, "index = %d", toDrawIndex);
    if (toDrawIndex != 4)
    {
        fprintf(stderr, "CurrentType = %d", cellType);
    }
    assert(toDrawIndex == 4);
    Color* pColor = &g_CellColors[g_GameState.currentCellType];
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
    for (int rectType = 0; rectType < (int)CELL_MAX_VALUE; ++rectType) {
        SDLRectArray* pArray = &g_RectArrays.RectArrays[rectType];
        pArray->NumRects = 0;
    }

    // Populate SDL Rect Arrays data structures for rendering
    for (Uint8 y = 0; y < GRID_HEIGHT; ++y) {
        for (Uint8 x = 0; x < GRID_WIDTH; ++x) {
            GridCell* pCell = &g_Grid[y][x];
            assert((int)pCell->cellType >= 0);
            assert((int)pCell->cellType < CELL_MAX_VALUE);

            SDLRectArray* pRectArray = &g_RectArrays.RectArrays[pCell->cellType];

            int rectIndex = pRectArray->NumRects;
            SDL_Rect* pRect = &pRectArray->Rects[rectIndex];

            pRect->x = pCell->x * GRID_CELL_WIDTH + GRID_UPPER_X;
            pRect->y = pCell->y * GRID_CELL_HEIGHT + GRID_UPPER_Y;
            pRect->w = GRID_CELL_WIDTH;
            pRect->h = GRID_CELL_HEIGHT;

            pRectArray->NumRects++;
        }
    }

    for (int rectType = 0; rectType < (int)CELL_MAX_VALUE; ++rectType) {
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

    initializePatternLUT();
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
            }
        }

        // Main loop
        Uint64 startTime = SDL_GetPerformanceCounter();

        updateGameState();
        renderGrid(pRender);
        renderPattern(pRender);

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
