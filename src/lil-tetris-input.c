#include <SDL2/SDL.h>
#include <stdbool.h>
#include <assert.h>

typedef enum
{
    INPUTEVENT_QUIT = 0,
    INPUTEVENT_UP,
    INPUTEVENT_DOWN,
    INPUTEVENT_LEFT,
    INPUTEVENT_RIGHT,
    INPUTEVENT_ROTATERIGHT,
    INPUTEVENT_ROTATELEFT,
    INPUTEVENT_HOLD,
    INPUTEVENT_PAUSE,
    INPUTEVENT_BEGINGAME,
    INPUTEVENT_MAX
} InputEvent;

#define MAX_INPUT_SCANCODES 3
typedef struct
{
    SDL_Scancode Scancodes[MAX_INPUT_SCANCODES];
    Uint8 NumScancodes;
} InputMapEntry;

typedef struct
{
    bool KeyStateCurrentFrame[INPUTEVENT_MAX];
    bool KeyStatePreviousFrame[INPUTEVENT_MAX];
    Uint64 KeyStateDurationFrames[INPUTEVENT_MAX];

    InputMapEntry InputMap[INPUTEVENT_MAX];
} InputContext;

bool InputHasScancodes(InputMapEntry* pEntry, const Uint8* pKeyboardState)
{
    assert(pEntry);
    assert(pEntry->NumScancodes < MAX_INPUT_SCANCODES);
    for (Uint8 i = 0; i < pEntry->NumScancodes; ++i)
    {
        if (pKeyboardState[pEntry->Scancodes[i]])
        {
            return true;
        }
    }

    return false;
}

#define UPDATE_KEYSTATE(pContext, inputEvent,  pKeys)                                                   \
    {                                                                                                   \
        InputMapEntry* pEntry = &pContext->InputMap[inputEvent];                                        \
        pContext->KeyStatePreviousFrame[inputEvent] = pContext->KeyStateCurrentFrame[inputEvent];       \
        pContext->KeyStateCurrentFrame[inputEvent] = InputHasScancodes(pEntry, pKeys);                  \
                                                                                                        \
        if (pContext->KeyStatePreviousFrame[inputEvent] == pContext->KeyStateCurrentFrame[inputEvent])  \
        {                                                                                               \
            pContext->KeyStateDurationFrames[inputEvent]++;                                             \
        }                                                                                               \
        else                                                                                            \
        {                                                                                               \
            pContext->KeyStateDurationFrames[inputEvent] = 0;                                           \
        }                                                                                               \
    }

void InputInitializeContext(InputContext* pContext)
{
    memset(pContext->KeyStateCurrentFrame, 0, sizeof(pContext->KeyStateCurrentFrame));
    memset(pContext->KeyStatePreviousFrame, 0, sizeof(pContext->KeyStatePreviousFrame));
    memset(pContext->KeyStateDurationFrames, 0, sizeof(pContext->KeyStateDurationFrames));
    memset(pContext->InputMap, 0, sizeof(pContext->InputMap));
}

// Must be called after pumping SDL event loop
void InputUpdateContext(InputContext* pContext, Uint64 currentFrame)
{
    const Uint8* pKeys = SDL_GetKeyboardState(NULL);

    UPDATE_KEYSTATE(pContext, INPUTEVENT_QUIT, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_UP, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_DOWN, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_LEFT, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_RIGHT, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_ROTATERIGHT, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_ROTATELEFT, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_HOLD, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_PAUSE, pKeys);
    UPDATE_KEYSTATE(pContext, INPUTEVENT_BEGINGAME, pKeys);
}
