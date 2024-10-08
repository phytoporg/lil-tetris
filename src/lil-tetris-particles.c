#include <SDL2/SDL.h>
#include <stdbool.h>
#include <assert.h>

// Should be way more than we need
#define MAX_PARTICLES 2048

typedef enum
{
    BEHAVIOR_DROP,
    BEHAVIOR_LINE_CLEAR
} ParticleBehavior;

typedef struct {
    Uint8     FramesSinceSpawn;
    Uint8     Lifetime;
    int       X;
    int       Y;
    int       Size;
    SDL_Color Color;
} SquareParticle_t;

typedef struct {
    ParticleBehavior Behavior;
    SquareParticle_t Particles[MAX_PARTICLES];
    bool             Valid[MAX_PARTICLES];
    bool             Initialized;
    Uint16           Count;
} ParticleSystem_t;

void ParticleSystemInitialize(
    ParticleSystem_t* pParticleSystem,
    ParticleBehavior behavior)
{
    if (!pParticleSystem)
    {
        return;
    }

    pParticleSystem->Behavior = behavior;
    memset(pParticleSystem->Particles, 0, sizeof(pParticleSystem->Particles));
    memset(pParticleSystem->Valid, 0, sizeof(pParticleSystem->Valid));
    pParticleSystem->Count = 0;
    pParticleSystem->Initialized = true;
}

SquareParticle_t* ParticleSystemMakeParticle(ParticleSystem_t* pParticleSystem)
{
    assert(pParticleSystem->Initialized);

    if (pParticleSystem->Count >= MAX_PARTICLES)
    {
        fprintf(stderr, "Exceeded max particle count\n");
        return NULL;
    }

    // Find the next available particle slot
    Sint16 nextParticleIndex = -1;
    for (Uint16 i = 0; i < MAX_PARTICLES; ++i)
    {
        if (!pParticleSystem->Valid[i])
        {
            nextParticleIndex = i;
            break;
        }
    }

    if (nextParticleIndex < 0)
    {
        // We shouldn't get here
        assert(false);
        fprintf(stderr, "Could not find available particle in MakeParticle()\n");
        return NULL;
    }

    SquareParticle_t* pParticle = &(pParticleSystem->Particles[nextParticleIndex]);
    memset(pParticle, 0, sizeof(*pParticle));

    pParticleSystem->Valid[nextParticleIndex] = true;
    ++pParticleSystem->Count;

    return pParticle;
}

void ParticleSystemTick(ParticleSystem_t* pParticleSystem)
{
    assert(pParticleSystem->Initialized);

    Sint16 particlesRemaining = pParticleSystem->Count;
    Uint16 index = 0;
    while(particlesRemaining > 0)
    {
        if (pParticleSystem->Valid[index])
        {
            // TODO
            SquareParticle_t* pParticle = &(pParticleSystem->Particles[index]);
            pParticle->FramesSinceSpawn++;

            // TODO: CHECK BEHAVIOR
            if (pParticleSystem->Behavior == BEHAVIOR_DROP)
            {
                if (pParticle->FramesSinceSpawn == pParticle->Lifetime)
                {
                    // This particle has expired
                    pParticleSystem->Valid[index] = false;
                    pParticleSystem->Count--;
                }
                else if (pParticle->FramesSinceSpawn > 6 && 
                         (pParticle->FramesSinceSpawn % 4 == 0))
                {
                    pParticle->Y += 2;
                    pParticle->Size = (pParticle->Size > 2 ? pParticle->Size - 1 : 2);
                }
            }
            else if (pParticleSystem->Behavior == BEHAVIOR_LINE_CLEAR)
            {
                // TODO
            }

            particlesRemaining--;
        }
        index++;
    }
}

void ParticleSystemRender(
    ParticleSystem_t* pParticleSystem,
    SDL_Renderer* pRenderer,
    int leftBounds,
    int rightBounds)
{
    assert(pParticleSystem->Initialized);

    Sint16 particlesRemaining = pParticleSystem->Count;
    Uint16 index = 0;
    while(particlesRemaining > 0)
    {
        if (pParticleSystem->Valid[index])
        {
            SquareParticle_t* pParticle = &(pParticleSystem->Particles[index]);

            // Don't render out of bounds
            if (pParticle->X > leftBounds && 
                (pParticle->X + pParticle->Size) < rightBounds)
            {
                SDL_Rect rect;

                rect.x = pParticle->X;
                rect.y = pParticle->Y;
                rect.w = pParticle->Size;
                rect.h = pParticle->Size;

                SDL_SetRenderDrawColor(
                    pRenderer,
                    pParticle->Color.r,
                    pParticle->Color.g,
                    pParticle->Color.b,
                    pParticle->Color.a);
                SDL_RenderFillRect(pRenderer, &rect);
            }

            particlesRemaining--;
        }
        index++;
    }
}

