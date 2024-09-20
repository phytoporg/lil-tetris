#include <SDL2/SDL.h>
#include <stdbool.h>

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

typedef struct
{
    bool KeyStateCurrentFrame[INPUTEVENT_MAX];
    bool KeyStatePreviousFrame[INPUTEVENT_MAX];
    Uint64 KeyStateDurationFrames[INPUTEVENT_MAX];

    // TODO: define an input map, scancodes -> input event
} InputContext;

// TODO: reference input map to know the current key state
#define UPDATE_KEYSTATE(inputevent, currentframe) \
    pContext->KeyStatePreviousFrame[inputEvent] = pContext->KeyStateCurrentFrame[inputEvent];         \
    pContext->KeyStateCurrentFrame[inputEvent] = pKeys[SDL_SCANCODE_ESCAPE] || pKeys[SDL_SCANCODE_Q]; \
                                                                                                      \
    if (pContext->KeyStatePreviousFrame[inputEvent] == pContext->KeyStateCurrentFrame[inputEvent])    \
    {                                                                                                 \
        pContext->KeyStateDurationFrames[inputEvent]++;                                               \
    }                                                                                                 \
    else                                                                                              \
    {                                                                                                 \
        pContext->KeyStateDurationFrames[inputEvent] = 0;                                             \
    }

void InputInitializeContext(InputContext* pContext)
{
    memset(pContext->KeyStateCurrentFrame, 0, sizeof(pContext->KeyStateCurrentFrame));
    memset(pContext->KeyStatePreviousFrame, 0, sizeof(pContext->KeyStatePreviousFrame));
    memset(pContext->KeyStateDurationFrames, 0, sizeof(pContext->KeyStateDurationFrames));
}

// Must be called after pumping SDL event loop
void InputUpdateContext(InputContext* pContext, Uint64 currentFrame)
{
    const Uint8* pKeys = SDL_GetKeyboardState(NULL);

    // Quit
    pContext->KeyStatePreviousFrame[INPUTEVENT_QUIT] = pContext->KeyStateCurrentFrame[INPUTEVENT_QUIT];
    pContext->KeyStateCurrentFrame[INPUTEVENT_QUIT] = pKeys[SDL_SCANCODE_ESCAPE] || pKeys[SDL_SCANCODE_Q];

    if (pContext->KeyStatePreviousFrame[INPUTEVENT_QUIT] == pContext->KeyStateCurrentFrame[INPUTEVENT_QUIT])
    {
        pContext->KeyStateDurationFrames[INPUTEVENT_QUIT]++;
    }
    else
    {
        pContext->KeyStateDurationFrames[INPUTEVENT_QUIT] = 0;
    }

    // TODO: the rest of the keys...
}
