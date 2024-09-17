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
#include "lil-tetris-particles.c"

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

#define GRID_DISPLACE_AMOUNT 3
#define GRID_DISPLACE_DURATION 3

#define STATS_LOC_X 445
#define STATS_LOC_Y 300
#define STATS_W 140
#define STATS_H 95
#define STATS_TEXT_BORDERLEFT_X 5
#define STATS_LEVEL_LOC_Y (STATS_LOC_Y + 30)
#define STATS_BEST_LOC_Y (STATS_LEVEL_LOC_Y + 30)

#define STATS_BEST_FILEPATH "/tmp/highscore"

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

#define SPAWN_DELAY_FRAMES 30
#define CLEAR_LINES_FRAMES 30
#define CLEAR_LINES_FLASH_DURATION 10

#define LOCK_DELAY_FRAMES 60

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
    PatternType_t patternQueue[PATTERN_MAX_VALUE - 1];
    PatternType_t nextPatternType;
    PatternType_t currentPatternType;
    PatternType_t holdPatternType;
	PatternTheme* pCurrentTheme;
    ParticleSystem_t DropParticles;
    ParticleSystem_t LineClearParticles;
    Sint8      patternGridX;
    Sint8      patternGridY;
    Uint8      currentPatternRotation;
    Uint8      patternQueueIndex;
    Uint64     currentFrame;
    Uint64     lastDropFrame;
    Uint64     preSpawnFrame;
    Uint64     lastSpawnFrame;
    Uint64     lockBeginFrame;
    Uint8      dropSpeed;
    Sint8      clearLines[GRID_HEIGHT];
    Uint64     clearLinesFrame;
    Uint16     totalClearedLines;
    Uint8      currentLevel;
    Uint8      currentBest;
    HTEXT      hNextText;
    HTEXT      hHoldText;
    HTEXT      hLinesText;
    HTEXT      hLevelText;
    HTEXT      hBestText;
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
    bool       isPaused;
    bool       isIntro;
    bool       isGameOver;
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

void getSpawnPosition(PatternType_t patternType, Sint8* pXOut, Sint8* pYOut)
{
    Pattern* pPattern = g_PatternLUT[patternType][0];
    *pXOut = (GRID_WIDTH / 2) - (pPattern->cols / 2);
    *pYOut = -pPattern->rows;
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

void resetPatternQueue()
{
    // Initialize
    const Uint8 Begin = (Uint8)(PATTERN_NONE + 1);
    const Uint8 End = (Uint8)PATTERN_MAX_VALUE;
    for (Uint8 i = Begin; i < End; ++i)
    {
        g_GameState.patternQueue[i - 1] = i;
    }

    // Shuffle
    const Uint8 NumElements =
        sizeof(g_GameState.patternQueue) /
        sizeof(g_GameState.patternQueue[0]);
    for (Uint8 i = Begin; i < End; ++i)
    {
        const Uint8 a = rand() % NumElements;
        Uint8 b = rand() % NumElements;

        // No noops allowed
        while (b == a)
        {
            b = rand() % NumElements;
        }

        // Swap a & b
        const Uint8 temp = g_GameState.patternQueue[a];
        g_GameState.patternQueue[a] = g_GameState.patternQueue[b];
        g_GameState.patternQueue[b] = temp;
    }

    g_GameState.patternQueueIndex = 0;
}

PatternType_t nextPatternTypeFromQueue()
{
    const Uint8 NumElements =
        sizeof(g_GameState.patternQueue) /
        sizeof(g_GameState.patternQueue[0]);
    if (g_GameState.patternQueueIndex >= NumElements)
    {
        resetPatternQueue();
    }

    PatternType_t nextType = g_GameState.patternQueue[g_GameState.patternQueueIndex];
    g_GameState.patternQueueIndex++;

    return nextType;
}

Uint8 readBestFromFilesystem() 
{
    FILE* pFile = fopen(STATS_BEST_FILEPATH, "r");
    if (!pFile)
    {
        return 0;
    }

    Uint8 returnValue = fgetc(pFile);
    if (returnValue == EOF)
    {
        return 0;
    }

    return returnValue;
}

bool writeBestToFilesystem()
{
    FILE* pFile = fopen(STATS_BEST_FILEPATH, "w");
    if (!pFile)
    {
        return false;
    }

    return fputc(g_GameState.currentBest, pFile) == (int)g_GameState.currentBest;
}

void initializeGameState()
{
    resetPatternQueue();

    g_GameState.nextPatternType = nextPatternTypeFromQueue();
    g_GameState.currentPatternType = nextPatternTypeFromQueue();
    g_GameState.holdPatternType = PATTERN_NONE;
	g_GameState.pCurrentTheme = g_DefaultThemes;
    g_GameState.currentPatternRotation = 0;
    g_GameState.currentFrame = 0;
    g_GameState.lastDropFrame = 0;
    g_GameState.preSpawnFrame = 0;
    g_GameState.lastSpawnFrame = 0;
    g_GameState.lockBeginFrame = 0;
    g_GameState.dropSpeed = START_DROP_SPEED; // Drops per second
    memset(g_GameState.clearLines, -1, sizeof(g_GameState.clearLines));
    g_GameState.clearLinesFrame = 0;
    g_GameState.totalClearedLines = 0;
    g_GameState.currentLevel = 1;
    g_GameState.currentBest = readBestFromFilesystem();
    g_GameState.hNextText = TEXT_INVALID_HANDLE;
    g_GameState.hHoldText = TEXT_INVALID_HANDLE;
    g_GameState.hLinesText = TEXT_INVALID_HANDLE;
    g_GameState.hLevelText = TEXT_INVALID_HANDLE;
    g_GameState.hBestText = TEXT_INVALID_HANDLE;
    g_GameState.hPausedText = TEXT_INVALID_HANDLE;
    g_GameState.hIntroText = TEXT_INVALID_HANDLE;
    g_GameState.isPaused = false;
    g_GameState.isIntro = true;
    g_GameState.isGameOver = false;
    g_GameState.renderCells = true;

    getSpawnPosition(
        g_GameState.currentPatternType,
        &(g_GameState.patternGridX),
        &(g_GameState.patternGridY));

    resetInputStates();
}

Pattern* getCurrentPattern()
{
    const PatternType_t CurrentType = g_GameState.currentPatternType;
    const Uint8 CurrentRotation = g_GameState.currentPatternRotation;
    return g_PatternLUT[CurrentType][CurrentRotation];
}

 enum PatternCollision {
     COLLIDES_NONE      = 0,
     COLLIDES_BOTTOM    = 1 << 0,
     COLLIDES_LEFT      = 1 << 1,
     COLLIDES_RIGHT     = 1 << 2,
     COLLIDES_COMMITTED = 1 << 3,
};

Uint8 patternCollides(Pattern* pPattern, Sint8 dX, Sint8 dY)
{
    int collisionFlags = COLLIDES_NONE;
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                Sint8 gridX = x + g_GameState.patternGridX + dX;
                Sint8 gridY = y + g_GameState.patternGridY + dY;

                // Collides with bottom?
                if (gridY >= GRID_HEIGHT)
                {
                    collisionFlags |= COLLIDES_BOTTOM;
                }

                // Collides with left?
                if (gridX < 0)
                {
                    collisionFlags |= COLLIDES_LEFT;
                }

                // Collides with right?
                if (gridX >= GRID_WIDTH)
                {
                    collisionFlags |= COLLIDES_RIGHT;
                }

                // Collides with committed cells?
                if (gridY >= 0 && g_Grid[gridY][gridX].patternType != PATTERN_NONE)
                {
                    collisionFlags |= COLLIDES_COMMITTED;
                }
            }
        }
    }

    return collisionFlags;
}

bool waitingToSpawn()
{
    Uint64 sincePreSpawn = g_GameState.currentFrame - g_GameState.preSpawnFrame;
    return sincePreSpawn < SPAWN_DELAY_FRAMES && g_GameState.preSpawnFrame > 0;
}

bool isSpawnFrame()
{
    Uint64 sincePreSpawn = g_GameState.currentFrame - g_GameState.preSpawnFrame;
    return sincePreSpawn == SPAWN_DELAY_FRAMES && g_GameState.preSpawnFrame > 0;
}

bool isClearingLines()
{
    const bool ClearingLines = g_GameState.clearLines[0] >= 0;
    Uint64 sinceClearedLines = g_GameState.currentFrame - g_GameState.clearLinesFrame;
    return ClearingLines || sinceClearedLines < CLEAR_LINES_FRAMES;
}

bool isLineBeingCleared(Sint8 gridY)
{
    // Don't render any pattern cells that are being cleared
    Sint8 clearLineIndex = 0;
    Sint8 y = g_GameState.clearLines[clearLineIndex];
    while (y >= 0 && clearLineIndex < sizeof(g_GameState.clearLines))
    {
        if (y == gridY)
        {
            return true;
        }

        ++clearLineIndex;
        y = g_GameState.clearLines[clearLineIndex];
    }

    return false;
}

void commitCurrentPattern()
{
    Pattern* pPattern = getCurrentPattern();
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                Sint8 gridX = x + g_GameState.patternGridX;
                Sint8 gridY = y + g_GameState.patternGridY;

                if (gridY < 0)
                {
                    // If we commit any cells above the grid, the game is over
                    g_GameState.isGameOver = true;
                    continue;
                }

                g_Grid[gridY][gridX].patternType = g_GameState.currentPatternType;
            }
        }
    }

    g_GameState.lockBeginFrame = 0;
}

void beginSpawnNextPattern()
{
    g_GameState.preSpawnFrame = g_GameState.currentFrame;
}

void spawnNextPattern()
{
    g_GameState.currentPatternType = g_GameState.nextPatternType;
    g_GameState.nextPatternType = nextPatternTypeFromQueue();
    g_GameState.currentPatternRotation = 0;
    getSpawnPosition(
        g_GameState.currentPatternType,
        &(g_GameState.patternGridX),
        &(g_GameState.patternGridY));
    g_GameState.lastSpawnFrame = g_GameState.currentFrame;
}

void getGridPosition(Uint8* pXOut, Uint8* pYOut)
{
    Uint64 sincePreSpawn = g_GameState.currentFrame - g_GameState.preSpawnFrame;
    Uint8 yOffset = 
        (sincePreSpawn < GRID_DISPLACE_DURATION) ? GRID_DISPLACE_AMOUNT : 0;

    *pXOut = GRID_UPPER_X;
    *pYOut = GRID_UPPER_Y + yOffset;
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

    // Ignore other inputs if we're waiting to spawn
    if (waitingToSpawn())
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

        // Resolve any collisions with the stage edge
        Sint8 dX = 0;
        Uint8 CollisionFlags = patternCollides(pRotatedPattern, dX, 0);
        while (CollisionFlags == COLLIDES_RIGHT || CollisionFlags == COLLIDES_LEFT)
        {
            dX += (CollisionFlags == COLLIDES_RIGHT ? -1 : 1);
            CollisionFlags = patternCollides(pRotatedPattern, dX, 0);
        }

        if (CollisionFlags == COLLIDES_NONE)
        {
            g_GameState.currentPatternRotation = rotationIndex;
            g_GameState.patternGridX += dX;
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

        // Resolve any collisions with the stage edge
        Sint8 dX = 0;
        Uint8 CollisionFlags = patternCollides(pRotatedPattern, dX, 0);
        while (CollisionFlags == COLLIDES_RIGHT || CollisionFlags == COLLIDES_LEFT)
        {
            dX += (CollisionFlags == COLLIDES_RIGHT ? -1 : 1);
            CollisionFlags = patternCollides(pRotatedPattern, dX, 0);
        }

        if (CollisionFlags == COLLIDES_NONE)
        {
            g_GameState.currentPatternRotation = rotationIndex;
            g_GameState.patternGridX += dX;
        }
    }

    if (g_GameState.inputHoldPiece)
    {
        if (g_GameState.holdPatternType == PATTERN_NONE)
        {
            g_GameState.holdPatternType = g_GameState.currentPatternType;
            g_GameState.currentPatternType = g_GameState.nextPatternType; 
            g_GameState.nextPatternType = nextPatternTypeFromQueue();
        }
        else
        {
            PatternType_t temp = g_GameState.holdPatternType;
            g_GameState.holdPatternType = g_GameState.currentPatternType;
            g_GameState.currentPatternType = temp;
        }

        g_GameState.currentPatternRotation = 0;
        getSpawnPosition(
            g_GameState.currentPatternType,
            &(g_GameState.patternGridX),
            &(g_GameState.patternGridY));
    }
}

////////////////////////////////////////////////////////////////////////////////
// Drop particles
////////////////////////////////////////////////////////////////////////////////
int dropParticleOffsetX()
{
    // Between -20 and 20
    return (rand() % 40) - 20;
}

int dropParticleOffsetY()
{
    // Between -5 and 5
    return (rand() % 10) - 5;
}

int dropParticleSize()
{
    // Between 2 and 8
    return (rand() % 7) + 2;
}

int dropParticleShouldCreate()
{
    return rand() % 3;
}

////////////////////////////////////////////////////////////////////////////////
// Line clear particles
////////////////////////////////////////////////////////////////////////////////
void emitLineClearParticles(Uint8 y)
{
    Uint8 GridBaseX;
    Uint8 GridBaseY;
    getGridPosition(&GridBaseX, &GridBaseY);

    for (Sint8 x = 0; x < GRID_WIDTH; ++x)
    {
        SquareParticle_t* pNewParticle = 
            ParticleSystemMakeParticle(&(g_GameState.DropParticles));
        pNewParticle->X = x;

        pNewParticle->Size = 5;
        pNewParticle->Lifetime = 15;
        pNewParticle->X =
            x * GRID_CELL_WIDTH + 
            GridBaseX + 
            (GRID_CELL_WIDTH / 2);
        pNewParticle->Y =
            y * GRID_CELL_HEIGHT + 
            GridBaseY;

        Color* pColor = 
            ThemeGetInnerColor(g_GameState.pCurrentTheme, g_Grid[y][x].patternType);
        SDL_Color color = {
            pColor->r,
            pColor->g,
            pColor->b,
            SDL_ALPHA_OPAQUE
        };
        pNewParticle->Color = color;
    }
}

void updateGameState()
{
    Uint64 sinceLastDrop = g_GameState.currentFrame - g_GameState.lastDropFrame;
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
    else if (isSpawnFrame())
    {
        commitCurrentPattern();
        spawnNextPattern();
    }

    if (isClearingLines())
    {
        Uint64 sinceClearedLines = 
            g_GameState.currentFrame -
            g_GameState.clearLinesFrame;
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
        Pattern* pPattern = getCurrentPattern();
        while (!patternCollides(pPattern, 0, patternHeight))
        {
            ++patternHeight;
        }

        const Sint8 OldPatternGridX = g_GameState.patternGridX;
        const Sint8 OldPatternGridY = g_GameState.patternGridY;
        g_GameState.patternGridY += patternHeight - 1;
        const PatternType_t OldPatternType = g_GameState.currentPatternType;

        commitCurrentPattern();
        beginSpawnNextPattern();

        g_GameState.lastDropFrame = g_GameState.currentFrame;

        // Emit particles between the current pattern and the drop location
        for (int i = 0; i < (patternHeight - 1); ++i)
        {
            if (!dropParticleShouldCreate())
            {
                continue;
            }

            // Just one per grid position atm
            SquareParticle_t* pNewParticle = 
                ParticleSystemMakeParticle(&(g_GameState.DropParticles));
            if (!pNewParticle)
            {
                continue;
            }

            // TODO: hm, not sure if this looks best or if the particles should
            // have their own dedicated color(s)
            Color* pColor = 
                ThemeGetOuterColor(g_GameState.pCurrentTheme, OldPatternType);
            const SDL_Color SDLColor = {
                pColor->r,
                pColor->g,
                pColor->b,
                SDL_ALPHA_OPAQUE
            };
            pNewParticle->Color = SDLColor;

            Uint8 GridBaseX;
            Uint8 GridBaseY;
            getGridPosition(&GridBaseX, &GridBaseY);

            pNewParticle->Size = dropParticleSize();
            pNewParticle->Lifetime = i + 15;
            pNewParticle->X =
                OldPatternGridX * GRID_CELL_WIDTH + 
                GridBaseX + 
                ((pPattern->cols * GRID_CELL_WIDTH) / 2) +
                dropParticleOffsetX();
            pNewParticle->Y =
                (OldPatternGridY + i) * GRID_CELL_HEIGHT + 
                GridBaseY +
                dropParticleOffsetY(); 
        }
    }
    else if (dropFrameTarget <= sinceLastDrop || g_GameState.inputDownPressed)
    {
        if (!patternCollides(getCurrentPattern(), 0, 1))
        {
            g_GameState.patternGridY++;
        }
        else if (g_GameState.inputDownPressed)
        {
            commitCurrentPattern();
            beginSpawnNextPattern();
        }
        else if (g_GameState.lockBeginFrame == 0)
        {
            // Start counting lock delay, which gets reset whenever we commit
            // a pattern.
            g_GameState.lockBeginFrame = g_GameState.currentFrame;
        }

        g_GameState.lastDropFrame = g_GameState.currentFrame;
    }

    // Update lock delay if necessary
    if (g_GameState.lockBeginFrame > 0)
    {
        // Reset lock delay if the current pattern isn't ready to lock
        if (!patternCollides(getCurrentPattern(), 0, 1))
        {
            g_GameState.lockBeginFrame = 0;
        }
        else
        {
            const bool LockDelayExpired = 
                g_GameState.lockBeginFrame > 0 &&
                (g_GameState.currentFrame - g_GameState.lockBeginFrame) >= LOCK_DELAY_FRAMES;
            if (LockDelayExpired)
            {
                commitCurrentPattern();
                beginSpawnNextPattern();
            }
        }
    }

    // Check losing condition
    if (g_GameState.isGameOver)
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
        g_GameState.holdPatternType = PATTERN_NONE;
        g_GameState.currentPatternType = nextPatternTypeFromQueue();
        g_GameState.nextPatternType = nextPatternTypeFromQueue();

        // TODO: do this elsewhere?
        g_GameState.isGameOver = false;
        return;
    }

    // Checking cleared lines or lose condition if there was a drop
    if (g_GameState.lastDropFrame == g_GameState.currentFrame)
    {
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
                emitLineClearParticles(y);
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

            if (g_GameState.totalClearedLines > g_GameState.currentBest)
            {
                g_GameState.currentBest = g_GameState.totalClearedLines;
                writeBestToFilesystem();
            }

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
    int numRects,
    Color* pInnerColorOverride,
    Color* pOuterColorOverride)
{
    // Outer
    Color* pOuterColor = 
        pOuterColorOverride ? pOuterColorOverride :
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
        pInnerColorOverride ? pInnerColorOverride :
        ThemeGetInnerColor(g_GameState.pCurrentTheme, (int)patternType);
    SDL_SetRenderDrawColor(
        pRenderer,
        pInnerColor->r,
        pInnerColor->g,
        pInnerColor->b,
        SDL_ALPHA_OPAQUE);
    SDL_RenderFillRects(pRenderer, pRects, numRects);
}

void renderShadowPattern(SDL_Renderer* pRenderer)
{
    if (!g_GameState.renderCells)
    {
        return;
    }

    // Don't render the shadow pattern if we're still waiting to spawn
    if (waitingToSpawn())
    {
        return;
    }

    PatternType_t patternType = g_GameState.currentPatternType;
    Pattern* pPattern = getCurrentPattern();

    // Figure out the shadow pattern location
    int patternHeight = 1;
    while (!patternCollides(pPattern, 0, patternHeight))
    {
        ++patternHeight;
    }

    const int ShadowPatternX = g_GameState.patternGridX;
    const int ShadowPatternY = g_GameState.patternGridY + patternHeight - 1;

    if (ShadowPatternY == g_GameState.patternGridY)
    {
        // We're overlapping the current pattern, so don't draw anything
        return;
    }

    Uint8 GridBaseX;
    Uint8 GridBaseY;
    getGridPosition(&GridBaseX, &GridBaseY);

    int toDrawIndex = 0;
    SDL_Rect toDraw[4];
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                SDL_Rect* pRect = &toDraw[toDrawIndex];
                pRect->x = (x + ShadowPatternX) * GRID_CELL_WIDTH + GridBaseX;
                pRect->y = (y + ShadowPatternY) * GRID_CELL_HEIGHT + GridBaseY;
                pRect->w = GRID_CELL_WIDTH;
                pRect->h = GRID_CELL_HEIGHT;

                ++toDrawIndex;
            }
        }
    }

    Color kShadowColorOuter = { 70, 5, 130 };
    Color kShadowColorInner = { 30, 5, 40 };

    assert(toDrawIndex == 4);
    renderCellArray(
        pRenderer,
        g_GameState.currentPatternType,
        toDraw,
        toDrawIndex,
        &kShadowColorInner, &kShadowColorOuter);
}

void renderCurrentPattern(SDL_Renderer* pRenderer)
{
    if (!g_GameState.renderCells)
    {
        return;
    }

    PatternType_t patternType = g_GameState.currentPatternType;
    Pattern* pPattern = getCurrentPattern();

    Uint8 GridBaseX;
    Uint8 GridBaseY;
    getGridPosition(&GridBaseX, &GridBaseY);

    int toDrawIndex = 0;
    SDL_Rect toDraw[4];
    for (int y = 0; y < pPattern->rows; ++y) {
        for (int x = 0; x < pPattern->cols; ++x) {
            if (pPattern->occupancy[y][x])
            {
                const int GridX = x + g_GameState.patternGridX;
                const int GridY = y + g_GameState.patternGridY;

                if (GridY < 0)
                {
                    // Don't render any cells "above" the grid
                    continue;
                }

                if (isLineBeingCleared(GridY))
                {
                    // Don't render cells that are being cleared
                    continue;
                }

                SDL_Rect* pRect = &toDraw[toDrawIndex];
                pRect->x = GridX * GRID_CELL_WIDTH + GridBaseX;
                pRect->y = GridY * GRID_CELL_HEIGHT + GridBaseY;
                pRect->w = GRID_CELL_WIDTH;
                pRect->h = GRID_CELL_HEIGHT;

                ++toDrawIndex;
            }
        }
    }

    Color* pInnerColor = NULL;
    Color* pOuterColor = NULL;
    if (waitingToSpawn())
    {
        // Animate color, blend from white to the target color
        Color* pThemeOuterColor = 
            ThemeGetOuterColor(g_GameState.pCurrentTheme, (int)patternType);
        Color* pThemeInnerColor = 
            ThemeGetInnerColor(g_GameState.pCurrentTheme, (int)patternType);

        Uint64 sincePreSpawn = g_GameState.currentFrame - g_GameState.preSpawnFrame;
        float t = (float)sincePreSpawn / SPAWN_DELAY_FRAMES;
        const Color kWhite = { 255, 255, 255 };
        Color blendedInner = {
            kWhite.r * (1.0f - t) + pThemeInnerColor->r * t,
            kWhite.g * (1.0f - t) + pThemeInnerColor->g * t,
            kWhite.b * (1.0f - t) + pThemeInnerColor->b * t,
        };

        Color blendedOuter = {
            kWhite.r * (1.0f - t) + pThemeOuterColor->r * t,
            kWhite.g * (1.0f - t) + pThemeOuterColor->g * t,
            kWhite.b * (1.0f - t) + pThemeOuterColor->b * t,
        };

        pInnerColor = &blendedInner;
        pOuterColor = &blendedOuter;
    }

    renderCellArray(
        pRenderer,
        g_GameState.currentPatternType,
        toDraw,
        toDrawIndex,
        pInnerColor,
        pOuterColor);
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
    if (!TextDrawEntry(
        g_GameState.hNextText,
        pRenderer,
        NEXT_PATTERN_TEXT_X,
        NEXT_PATTERN_TEXT_Y))
    {
        fprintf(stderr, "Failed to draw next text\n");
    }

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
    renderCellArray(
        pRenderer,
        g_GameState.nextPatternType,
        toDraw,
        toDrawIndex, 
        NULL, NULL);
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
    if (!TextDrawEntry(
        g_GameState.hHoldText,
        pRenderer,
        HOLD_PATTERN_TEXT_X,
        HOLD_PATTERN_TEXT_Y))
    {
        fprintf(stderr, "Failed to draw hold text\n");
    }

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
    renderCellArray(
        pRenderer,
        g_GameState.holdPatternType,
        toDraw,
        toDrawIndex,
        NULL, NULL);
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
    if (!TextDrawEntry(g_GameState.hLinesText, pRenderer, linesTextX, linesTextY))
    {
        fprintf(stderr, "Failed to draw lines text\n");
    }

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
    if (!TextDrawEntry(g_GameState.hLevelText, pRenderer, levelTextX, levelTextY))
    {
        fprintf(stderr, "Failed to draw level text\n");
    }

    // Best text
    if (g_GameState.hBestText == TEXT_INVALID_HANDLE)
    {
        g_GameState.hBestText = TextCreateEntry();
        assert(g_GameState.hBestText != TEXT_INVALID_HANDLE);
    }

    const int bestTextX = linesTextX;
    const int bestTextY = STATS_BEST_LOC_Y;
    char bestText[256];
    sprintf(bestText, "BEST: %hhu", g_GameState.currentBest);
    TextSetEntryData(g_GameState.hBestText, pRenderer, bestText);
    if (!TextDrawEntry(g_GameState.hBestText, pRenderer, bestTextX, bestTextY))
    {
        fprintf(stderr, "Failed to draw best text\n");
    }
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
        if (!TextDrawEntry(
                g_GameState.hPausedText,
                pRenderer,
                PAUSED_LOC_X,
                PAUSED_LOC_Y))
        {
            fprintf(stderr, "Failed to draw paused text\n");
        }
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
        if (!TextDrawEntry(
                g_GameState.hIntroText,
                pRenderer,
                INTRO_LOC_X,
                INTRO_LOC_Y))
        {
            fprintf(stderr, "Failed to draw intro text\n");
        }
    }
}

void renderGrid(SDL_Renderer* pRenderer) 
{
    // Reset the rect array rect counts
    for (int rectType = 0; rectType < (int)PATTERN_MAX_VALUE; ++rectType) {
        SDLRectArray* pArray = &g_RectArrays.RectArrays[rectType];
        pArray->NumRects = 0;
    }

    Uint8 GridBaseX;
    Uint8 GridBaseY;
    getGridPosition(&GridBaseX, &GridBaseY);

    // Populate SDL Rect Arrays data structures for rendering
    for (Uint8 y = 0; y < GRID_HEIGHT; ++y) {
        for (Uint8 x = 0; x < GRID_WIDTH; ++x) {
            GridCell* pCell = &g_Grid[y][x];
            assert((int)pCell->patternType >= 0);
            assert((int)pCell->patternType < PATTERN_MAX_VALUE);

            const bool DrawEmptyCell =
                !g_GameState.renderCells || isLineBeingCleared(y);

            // Only show empty cells if no cells are to be rendered, or if
            // the current line is being cleared
            SDLRectArray* pRectArray = DrawEmptyCell ? 
                &g_RectArrays.RectArrays[PATTERN_NONE] :
                &g_RectArrays.RectArrays[pCell->patternType];

            int rectIndex = pRectArray->NumRects;
            SDL_Rect* pRect = &pRectArray->Rects[rectIndex];

            pRect->x = pCell->x * GRID_CELL_WIDTH + GridBaseX;
            pRect->y = pCell->y * GRID_CELL_HEIGHT + GridBaseY;
            pRect->w = GRID_CELL_WIDTH;
            pRect->h = GRID_CELL_HEIGHT;

            pRectArray->NumRects++;
        }
    }

    for (int rectType = 0; rectType < (int)PATTERN_MAX_VALUE; ++rectType) {
        SDLRectArray* pArray = &g_RectArrays.RectArrays[rectType];
        renderCellArray(
            pRenderer,
            rectType,
            pArray->Rects,
            pArray->NumRects,
            NULL, NULL);
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

    ParticleSystemInitialize(&(g_GameState.DropParticles), BEHAVIOR_DROP);
    ParticleSystemInitialize(&(g_GameState.LineClearParticles), BEHAVIOR_LINE_CLEAR);

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
        ParticleSystemTick(&(g_GameState.DropParticles));

        checkInputs();
        renderGrid(pRender);
        renderShadowPattern(pRender);
        renderCurrentPattern(pRender);
        renderNextPattern(pRender);
        renderHoldPattern(pRender);
        renderStats(pRender);
        renderPauseText(pRender);
        renderIntroText(pRender);

        Uint8 GridBaseX;
        Uint8 GridBaseY;
        getGridPosition(&GridBaseX, &GridBaseY);

        const int LeftBound = GridBaseX;
        const int RightBound = GridBaseX + GRID_WIDTH * GRID_CELL_WIDTH;
        ParticleSystemRender(
            &(g_GameState.DropParticles),
            pRender,
            LeftBound,
            RightBound);

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
