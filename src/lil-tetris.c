#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "lil-tetris-audio.c"
#include "lil-tetris-patterns.c"
#include "lil-tetris-themes.c"
#include "lil-tetris-text.c"

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
#define GRID_CELL_BORDER 2

#define STATS_LOC_X 445
#define STATS_LOC_Y 300
#define STATS_W 140
#define STATS_H 65
#define STATS_TEXT_BORDERLEFT_X 5
#define STATS_LEVEL_LOC_Y (STATS_LOC_Y + 30)

#define PAUSED_LOC_X 265
#define PAUSED_LOC_Y 200

#define INTRO_LOC_X 130
#define INTRO_LOC_Y 210

#define NEXT_PATTERN_TEXT_X 450
#define NEXT_PATTERN_TEXT_Y 80
#define NEXT_PATTERN_X 450
#define NEXT_PATTERN_Y 100
#define NEXT_PATTERN_BORDER 5

#define HOLD_PATTERN_TEXT_X 60
#define HOLD_PATTERN_TEXT_Y 80
#define HOLD_PATTERN_X 60
#define HOLD_PATTERN_Y 100
#define HOLD_PATTERN_BORDER 5

#define SPAWN_DELAY_FRAMES 40
#define CLEAR_LINES_FRAMES 60
#define CLEAR_LINES_FLASH_DURATION 10

#define START_DROP_SPEED 2

// Types
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
    PatternType_t holdPatternType;
	PatternTheme* pCurrentTheme;
    Sint8      patternGridX;
    Sint8      patternGridY;
    Uint8      currentPatternRotation;
    Uint64     currentFrame;
    Uint64     lastDropFrame;
    Uint64     lastSpawnFrame;
    Uint8      dropSpeed;
    Sint8      clearLines[GRID_HEIGHT];
    Uint64     clearLinesFrame;
    Uint16     totalClearedLines;
    Uint8      currentLevel;
    HTEXT      hNextText;
    HTEXT      hHoldText;
    HTEXT      hLinesText;
    HTEXT      hLevelText;
    HTEXT      hPausedText;
    HTEXT      hIntroText;
    bool       inputLeftPressed;
    bool       inputRightPressed;
    bool       inputDownPressed;
    bool       inputUpPressed;
    bool       inputRotateRightPressed;
    bool       inputRotateLeftPressed;
    bool       inputHoldPiece;
    bool       inputPauseGame;
    bool       inputBeginGame;
    bool       toggleFlash;
    bool       isPaused;
    bool       isIntro;
    bool       renderCells;
} GameState;

// Globals
static GameState g_GameState;
static GridCell g_Grid[GRID_HEIGHT][GRID_WIDTH];
static SDLRectArrays g_RectArrays;
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
    g_GameState.inputHoldPiece = false;
    g_GameState.inputPauseGame = false;
    g_GameState.inputBeginGame = false;
}

void initializeGameState()
{
    g_GameState.nextPatternType = randomPatternType();
    g_GameState.currentPatternType = randomPatternType();
    g_GameState.holdPatternType = PATTERN_NONE;
	g_GameState.pCurrentTheme = g_DefaultThemes;
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
    g_GameState.totalClearedLines = 0;
    g_GameState.currentLevel = 1;
    g_GameState.hNextText = TEXT_INVALID_HANDLE;
    g_GameState.hHoldText = TEXT_INVALID_HANDLE;
    g_GameState.hLinesText = TEXT_INVALID_HANDLE;
    g_GameState.hLevelText = TEXT_INVALID_HANDLE;
    g_GameState.hPausedText = TEXT_INVALID_HANDLE;
    g_GameState.hIntroText = TEXT_INVALID_HANDLE;
    g_GameState.toggleFlash = false;
    g_GameState.isPaused = false;
    g_GameState.isIntro = true;
    g_GameState.renderCells = true;

    resetInputStates();
}

Pattern* getCurrentPattern()
{
    const PatternType_t CurrentType = g_GameState.currentPatternType;
    const Uint8 CurrentRotation = g_GameState.currentPatternRotation;
    return g_PatternLUT[CurrentType][CurrentRotation];
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

bool waitingToSpawn()
{
    Uint64 sinceLastSpawn = g_GameState.currentFrame - g_GameState.lastSpawnFrame;
    return sinceLastSpawn < SPAWN_DELAY_FRAMES && g_GameState.lastSpawnFrame > 0;
}

void commitAndSpawnPattern()
{
    Pattern* pPattern = getCurrentPattern();
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                Sint8 gridX = x + g_GameState.patternGridX;
                Sint8 gridY = y + g_GameState.patternGridY;
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

void checkInputs()
{
    if (g_GameState.inputBeginGame && g_GameState.isIntro)
    {
        g_GameState.isIntro = false;
    }

    if (g_GameState.inputPauseGame)
    {
        if (!g_GameState.isPaused)
        {
            AudioPauseMusic();
        }
        else
        {
            AudioResumeMusic();
        }

        g_GameState.isPaused = !g_GameState.isPaused;
    }

    // If the game is paused, don't check any other inputs
    if (g_GameState.isPaused)
    {
        return;
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
            g_GameState.currentPatternRotation == 0 ? 
                numRotations - 1 : 
                g_GameState.currentPatternRotation - 1;
        Pattern* pRotatedPattern = g_PatternLUT[g_GameState.currentPatternType][rotationIndex];
        if (!patternCollides(pRotatedPattern, 0, 0))
        {
            g_GameState.currentPatternRotation = rotationIndex;
        }
    }

    // Ignore hold inputs if we're still waiting to spawn
    if (g_GameState.inputHoldPiece && !waitingToSpawn())
    {

        if (g_GameState.holdPatternType == PATTERN_NONE)
        {
            g_GameState.holdPatternType = g_GameState.currentPatternType;
            g_GameState.currentPatternType = g_GameState.nextPatternType; 
            g_GameState.nextPatternType = randomPatternType();
        }
        else
        {
            PatternType_t temp = g_GameState.holdPatternType;
            g_GameState.holdPatternType = g_GameState.currentPatternType;
            g_GameState.currentPatternType = temp;
        }

        g_GameState.currentPatternRotation = 0;
        g_GameState.patternGridX = 
            (GRID_WIDTH / 2) - 
            (g_PatternLUT[g_GameState.currentPatternType][0]->cols / 2);
        g_GameState.patternGridY = 0;
    }
}

void updateGameState()
{
    Uint64 sinceLastDrop = g_GameState.currentFrame - g_GameState.lastDropFrame;
    Uint64 sinceLastSpawn = g_GameState.currentFrame - g_GameState.lastSpawnFrame;
    Uint64 dropFrameTarget = (FPS / g_GameState.dropSpeed);

    if (g_GameState.isPaused || g_GameState.isIntro)
    {
        g_GameState.renderCells = false;
        return;
    }

    g_GameState.renderCells = true;

    if (waitingToSpawn())
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
            for (int i = 0; i < sizeof(g_GameState.clearLines); ++i)
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
    if (g_GameState.inputUpPressed)
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
    else if (dropFrameTarget <= sinceLastDrop || g_GameState.inputDownPressed)
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

    // Checking cleared lines or lose condition if there was a drop
    if (g_GameState.lastDropFrame == g_GameState.currentFrame)
    {
        // Check losing condition
        if (g_GameState.lastSpawnFrame == g_GameState.currentFrame)
        {
            Pattern* pNextPattern = g_PatternLUT[g_GameState.nextPatternType][0];
            if (patternCollides(pNextPattern, 0, 0))
            {
                // Just clear all lines on lose
                for (Uint8 y = 0; y < GRID_HEIGHT; ++y)
                {
                    g_GameState.clearLines[y] = y;
                }

                g_GameState.clearLinesFrame = g_GameState.currentFrame;
                g_GameState.currentFrame++;

                // Reset stats and level on loss
                g_GameState.totalClearedLines = 0;
                g_GameState.currentLevel = 1;
                g_GameState.pCurrentTheme = g_DefaultThemes;
                g_GameState.dropSpeed = START_DROP_SPEED;
                return;
            }
        }
        
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
            int previousLevel = g_GameState.currentLevel;

            AudioPlayLineClear();
            g_GameState.totalClearedLines += linesCleared;
            g_GameState.currentLevel = (g_GameState.totalClearedLines / 10) + 1;
            g_GameState.dropSpeed = START_DROP_SPEED + g_GameState.currentLevel;
            g_GameState.clearLinesFrame = g_GameState.currentFrame;

            if (g_GameState.currentLevel > previousLevel)
            {
                // Level up!
                g_GameState.pCurrentTheme =
                    ThemeGetNextTheme(g_GameState.pCurrentTheme);
            }
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

void 
renderCellArray(
    SDL_Renderer* pRenderer,
    PatternType_t patternType,
    SDL_Rect* pRects,
    int numRects)
{
    // Outer
    Color* pOuterColor = 
        ThemeGetOuterColor(g_GameState.pCurrentTheme, (int)patternType);
    SDL_SetRenderDrawColor(
        pRenderer,
        pOuterColor->r,
        pOuterColor->g,
        pOuterColor->b,
        SDL_ALPHA_OPAQUE);
    SDL_RenderFillRects(pRenderer, pRects, numRects);

    // Inner
    for (int i = 0; i < numRects; ++i)
    {
        SDL_Rect* pRect = &pRects[i];
        pRect->x += GRID_CELL_BORDER;
        pRect->y += GRID_CELL_BORDER;
        pRect->w -= (GRID_CELL_BORDER * 2);
        pRect->h -= (GRID_CELL_BORDER * 2);
    }
    
    Color* pInnerColor = 
        ThemeGetInnerColor(g_GameState.pCurrentTheme, (int)patternType);
    SDL_SetRenderDrawColor(
        pRenderer,
        pInnerColor->r,
        pInnerColor->g,
        pInnerColor->b,
        SDL_ALPHA_OPAQUE);
    SDL_RenderFillRects(pRenderer, pRects, numRects);
}

void renderCurrentPattern(SDL_Renderer* pRenderer)
{
    if (!g_GameState.renderCells)
    {
        return;
    }

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
    renderCellArray(pRenderer, g_GameState.currentPatternType, toDraw, toDrawIndex);
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

    if (g_GameState.hNextText == TEXT_INVALID_HANDLE)
    {
        g_GameState.hNextText = TextCreateEntry();
        assert(g_GameState.hNextText != TEXT_INVALID_HANDLE);
    }

    TextSetEntryData(g_GameState.hNextText, pRenderer, "NEXT");
    TextDrawEntry(
        g_GameState.hNextText,
        pRenderer,
        NEXT_PATTERN_TEXT_X,
        NEXT_PATTERN_TEXT_Y);

    // Don't render the actual pattern if the game is paused
    if (!g_GameState.renderCells)
    {
        return;
    }

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
    renderCellArray(pRenderer, g_GameState.nextPatternType, toDraw, toDrawIndex);
}

void renderHoldPattern(SDL_Renderer* pRenderer)
{
    PatternType_t patternType = g_GameState.holdPatternType;
    Pattern* pPattern = g_PatternLUT[g_GameState.holdPatternType][0];

    // Draw the bg first
    Color black = { 0, 0, 0 };
    SDL_Rect holdBgRect;
    holdBgRect.x = HOLD_PATTERN_X - HOLD_PATTERN_BORDER;
    holdBgRect.y = HOLD_PATTERN_Y - HOLD_PATTERN_BORDER;
    holdBgRect.w = HOLD_PATTERN_BORDER * 2 + 4 * GRID_CELL_WIDTH;
    holdBgRect.h = HOLD_PATTERN_BORDER * 2 + 4 * GRID_CELL_HEIGHT;

    SDL_SetRenderDrawColor(
        pRenderer,
        black.r,
        black.g,
        black.b,
        SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(pRenderer, &holdBgRect);

    if (g_GameState.hHoldText == TEXT_INVALID_HANDLE)
    {
        g_GameState.hHoldText = TextCreateEntry();
        assert(g_GameState.hHoldText != TEXT_INVALID_HANDLE);
    }

    TextSetEntryData(g_GameState.hHoldText, pRenderer, "HOLD");
    TextDrawEntry(
        g_GameState.hHoldText,
        pRenderer,
        HOLD_PATTERN_TEXT_X,
        HOLD_PATTERN_TEXT_Y);

    // Don't render the actual pattern if no pattern is held.
    if (!g_GameState.renderCells || patternType == PATTERN_NONE)
    {
        return;
    }

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
                pRect->x = x * GRID_CELL_WIDTH + HOLD_PATTERN_X + offsetX;
                pRect->y = y * GRID_CELL_HEIGHT + HOLD_PATTERN_Y + offsetY;
                pRect->w = GRID_CELL_WIDTH;
                pRect->h = GRID_CELL_HEIGHT;

                ++toDrawIndex;
            }
        }
    }

    assert(toDrawIndex == 4);
    renderCellArray(pRenderer, g_GameState.holdPatternType, toDraw, toDrawIndex);
}

void renderStats(SDL_Renderer* pRenderer)
{
    PatternType_t patternType = g_GameState.nextPatternType;
    Pattern* pPattern = g_PatternLUT[g_GameState.nextPatternType][0];

    // Draw the bg first
    Color black = { 0, 0, 0 };
    SDL_Rect statsBgRect;
    statsBgRect.x = STATS_LOC_X;
    statsBgRect.y = STATS_LOC_Y;
    statsBgRect.w = STATS_W;
    statsBgRect.h = STATS_H;

    SDL_SetRenderDrawColor(
        pRenderer,
        black.r,
        black.g,
        black.b,
        SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(pRenderer, &statsBgRect);

    // Lines text
    if (g_GameState.hLinesText == TEXT_INVALID_HANDLE)
    {
        g_GameState.hLinesText = TextCreateEntry();
        assert(g_GameState.hLinesText != TEXT_INVALID_HANDLE);
    }

    const int linesTextX = STATS_LOC_X + STATS_TEXT_BORDERLEFT_X;
    const int linesTextY = STATS_LOC_Y;
    char linesText[256];
    sprintf(linesText, "LINES: %hu", g_GameState.totalClearedLines);
    TextSetEntryData(g_GameState.hLinesText, pRenderer, linesText);
    TextDrawEntry(g_GameState.hLinesText, pRenderer, linesTextX, linesTextY);

    // Level text
    if (g_GameState.hLevelText == TEXT_INVALID_HANDLE)
    {
        g_GameState.hLevelText = TextCreateEntry();
        assert(g_GameState.hLevelText != TEXT_INVALID_HANDLE);
    }

    const int levelTextX = linesTextX;
    const int levelTextY = STATS_LEVEL_LOC_Y;
    char levelText[256];
    sprintf(levelText, "LEVEL: %hhu", g_GameState.currentLevel);
    TextSetEntryData(g_GameState.hLevelText, pRenderer, levelText);
    TextDrawEntry(g_GameState.hLevelText, pRenderer, levelTextX, levelTextY);
}

void renderPauseText(SDL_Renderer* pRenderer)
{
    if (g_GameState.isPaused)
    {
        if (g_GameState.hPausedText == TEXT_INVALID_HANDLE)
        {
            g_GameState.hPausedText = TextCreateEntry();
            assert(g_GameState.hPausedText != TEXT_INVALID_HANDLE);
        }

        TextSetEntryData(g_GameState.hPausedText, pRenderer, "PAUSE");
        TextDrawEntry(g_GameState.hPausedText, pRenderer, PAUSED_LOC_X, PAUSED_LOC_Y);
    }
}

void renderIntroText(SDL_Renderer* pRenderer)
{
    if (g_GameState.isIntro)
    {
        if (g_GameState.hIntroText == TEXT_INVALID_HANDLE)
        {
            g_GameState.hIntroText = TextCreateEntry();
            assert(g_GameState.hIntroText != TEXT_INVALID_HANDLE);
        }

        TextSetEntryData(g_GameState.hIntroText, pRenderer, "PRESS SPACEBAR TO START");
        TextDrawEntry(g_GameState.hIntroText, pRenderer, INTRO_LOC_X, INTRO_LOC_Y);
    }
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

            // Only show empty cells if no cells are to be rendered
            SDLRectArray* pRectArray = !g_GameState.renderCells ? 
                &g_RectArrays.RectArrays[PATTERN_NONE] :
                &g_RectArrays.RectArrays[pCell->patternType];

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
        SDLRectArray* pArray = &g_RectArrays.RectArrays[rectType];
        renderCellArray(pRenderer, rectType, pArray->Rects, pArray->NumRects);
    }

    // Render flashing lines if we're clearing lines
    const bool ClearingLines = g_GameState.clearLines[0] >= 0;
    Uint64 sinceClearedLines = g_GameState.currentFrame - g_GameState.clearLinesFrame;
    if (sinceClearedLines < CLEAR_LINES_FRAMES && g_GameState.renderCells)
    {
        if ((sinceClearedLines % CLEAR_LINES_FLASH_DURATION) == 0)
        {
            g_GameState.toggleFlash = g_GameState.toggleFlash ? false : true;
        }

        if (g_GameState.toggleFlash)
        {
            SDL_Rect FlashRects[GRID_WIDTH * sizeof(g_GameState.clearLines)];
            int rectIndex = 0;
            int clearLineIndex = 0;
            int y = g_GameState.clearLines[clearLineIndex];
            while (y >= 0 && clearLineIndex < sizeof(g_GameState.clearLines))
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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
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

    char* pDefaultAssetRoot = ".";
    char* pAssetRoot = (argc > 1 ? argv[1] : pDefaultAssetRoot);
    if (!AudioInitialize(pAssetRoot))
    {
        fprintf(stderr, "Did not initialize audio\n");
        return -1;
    }

    if (!TextInitialize(pRender, pAssetRoot))
    {
        fprintf(stderr, "Did not initialize text\n");
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
                    g_GameState.inputRotateLeftPressed = true;
                }
                else if (event.key.keysym.sym == SDLK_SPACE)
                {
                    if (g_GameState.isIntro)
                    {
                        g_GameState.isIntro = false;
                    }
                    else
                    {
                        g_GameState.inputHoldPiece = true;
                    }
                }
                else if (event.key.keysym.sym == SDLK_p)
                {
                    g_GameState.inputPauseGame = true;
                }
            }
        }

        // Main loop
        Uint64 startTime = SDL_GetPerformanceCounter();

        if (!g_GameState.isIntro)
        {
            AudioPlayMusic();
        }

        updateGameState();
        checkInputs();
        renderGrid(pRender);
        renderCurrentPattern(pRender);
        renderNextPattern(pRender);
        renderHoldPattern(pRender);
        renderStats(pRender);
        renderPauseText(pRender);
        renderIntroText(pRender);

        Uint64 endTime = SDL_GetPerformanceCounter();

        float elapsedMs = (endTime - startTime) / 
            (float)SDL_GetPerformanceFrequency() * 1000.0f; 
        float expectedMs = (1.0f / FPS) * 1000.0f;

        SDL_RenderPresent(pRender);
        SDL_Delay(expectedMs - elapsedMs);
    }

    AudioUninitialize();

    SDL_DestroyRenderer(pRender);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();

    return 0;
} 
