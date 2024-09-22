#include <SDL2/SDL.h>
#include <stdbool.h>
#include <assert.h>

#define INPUT_REPEAT_DELAY_FRAMES 8
#define INPUT_REPEAT_INTERVAL_FRAMES 3

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

void InputInitializeContext(InputContext* pContext)
{
    memset(pContext->KeyStateCurrentFrame, 0, sizeof(pContext->KeyStateCurrentFrame));
    memset(pContext->KeyStatePreviousFrame, 0, sizeof(pContext->KeyStatePreviousFrame));
    memset(pContext->KeyStateDurationFrames, 0, sizeof(pContext->KeyStateDurationFrames));
    memset(pContext->InputMap, 0, sizeof(pContext->InputMap));
}

bool InputHasEventWithRepeat(InputContext* pContext, InputEvent event)
{
    if (event < (InputEvent)0 || event > INPUTEVENT_MAX)
    {
        return false;
    }

    const bool HasThisFrame = pContext->KeyStateCurrentFrame[event];
    const bool HadLastFrame = pContext->KeyStatePreviousFrame[event];
    
    // On initial press, always return rue
    if (HasThisFrame && !HadLastFrame)
    {
        return true;
    }

    // |----Delay----|--Interval--|--Interval--|--Interval--|...
    const Uint64 StateDurationFrames = pContext->KeyStateDurationFrames[event];
    if (HasThisFrame && StateDurationFrames == INPUT_REPEAT_DELAY_FRAMES)
    {
        return true;
    }
    else if (HasThisFrame && StateDurationFrames > INPUT_REPEAT_DELAY_FRAMES)
    {
        const Uint64 DelayBaseline = StateDurationFrames - INPUT_REPEAT_DELAY_FRAMES;
        if (DelayBaseline % INPUT_REPEAT_INTERVAL_FRAMES == 0)
        {
            return true;
        }
    }

    return false;
}

bool InputHasEventPressed(InputContext* pContext, InputEvent event)
{
    if (event < (InputEvent)0 || event > INPUTEVENT_MAX)
    {
        return false;
    }

    const bool HasThisFrame = pContext->KeyStateCurrentFrame[event];
    const bool HadLastFrame = pContext->KeyStatePreviousFrame[event];

    return HasThisFrame && !HadLastFrame;
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

// Must be called after pumping SDL event loop
void InputUpdateContext(InputContext* pContext)
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
