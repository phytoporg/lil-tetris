#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

// Constants
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define FPS 60.0f

#define GRID_HEIGHT 20
#define GRID_WIDTH 10
#define GRID_UPPER_X 50
#define GRID_UPPER_Y 50
#define GRID_CELL_WIDTH 50
#define GRID_CELL_HEIGHT 50

// Types
typedef enum
{
    CELL_EMPTY,
    CELL_L_SHAPE,
    CELL_Z_SHAPE,
    CELL_LINE_SHAPE,
    CELL_SQUARE_SHAPE,
    CELL_MAX_VALUE
} CellType_t;

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

// Globals
static GridCell g_Grid[GRID_HEIGHT][GRID_WIDTH];
static SDLRectArrays g_RectArrays;

void renderGrid() 
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

            pRect->x = pCell->x + GRID_UPPER_X;
            pRect->y = pCell->y + GRID_UPPER_Y;
            pRect->w = pCell->x + GRID_WIDTH;
            pRect->h = pCell->y + GRID_HEIGHT;

            pRectArray->NumRects++;
        }
    }

    // TODO: DRAW!!
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

    // Initialization
    memset(g_Grid, 0, sizeof(g_Grid));

    bool shouldQuit = false;
    while (!shouldQuit)
    {
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
                if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q)
                {
                    shouldQuit = true;
                }
            }
        }

        // Main loop
        Uint64 startTime = SDL_GetPerformanceCounter();

        renderGrid();

        Uint64 endTime = SDL_GetPerformanceCounter();

        float elapsedMs = (endTime - startTime) / (float)SDL_GetPerformanceFrequency() * 1000.0f; 
        float expectedMs = (1.0f / FPS) * 1000.0f;

        SDL_RenderPresent(pRender);
        SDL_Delay(expectedMs - elapsedMs);
    }

    SDL_DestroyRenderer(pRender);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();

    return 0;
} 
