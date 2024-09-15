#include <SDL2/SDL.h>
#include <stdbool.h>
#include <assert.h>

// Should be way more than we need
#define MAX_PARTICLES 255

typedef struct {
    Uint8 FramesSinceSpawn;
    Uint8 Lifetime;
    int   X;
    int   Y;
    int   Size;
} SquareParticle_t;

typedef struct {
    SquareParticle_t Particles[MAX_PARTICLES];
    bool             Valid[MAX_PARTICLES];
    bool             Initialized;
    Uint8            Count;
    SDL_Color        Color;
} ParticleSystem_t;

void ParticleSystemInitialize(ParticleSystem_t* pParticleSystem)
{
    if (!pParticleSystem)
    {
        return;
    }

    memset(pParticleSystem->Particles, 0, sizeof(pParticleSystem->Particles));
    memset(pParticleSystem->Valid, 0, sizeof(pParticleSystem->Valid));
    pParticleSystem->Count = 0;
    pParticleSystem->Initialized = true;

    SDL_Color kWhite = { 255, 255, 255, SDL_ALPHA_OPAQUE };
    pParticleSystem->Color = kWhite;
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
    Sint8 nextParticleIndex = -1;
    for (Uint8 i = 0; i < MAX_PARTICLES; ++i)
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

    Sint8 particlesRemaining = pParticleSystem->Count;
    Uint8 index = 0;
    while(particlesRemaining > 0)
    {
        if (pParticleSystem->Valid[index])
        {
            // TODO
            SquareParticle_t* pParticle = &(pParticleSystem->Particles[index]);
            pParticle->FramesSinceSpawn++;

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

    SDL_Rect particleRects[MAX_PARTICLES];
    Sint8 particlesRemaining = pParticleSystem->Count;
    Uint8 index = 0;
    Uint8 numRects = 0;
    while(particlesRemaining > 0)
    {
        if (pParticleSystem->Valid[index])
        {
            SquareParticle_t* pParticle = &(pParticleSystem->Particles[index]);

            // Don't render out of bounds
            if (pParticle->X > leftBounds && 
                (pParticle->X + pParticle->Size) < rightBounds)
            {
                SDL_Rect* pRect = (&particleRects[numRects]);

                pRect->x = pParticle->X;
                pRect->y = pParticle->Y;
                pRect->w = pParticle->Size;
                pRect->h = pParticle->Size;

                numRects++;
            }

            particlesRemaining--;
        }
        index++;
    }

    if (!numRects)
    {
        // Nothing to draw
        return;
    }

    SDL_SetRenderDrawColor(
        pRenderer,
        pParticleSystem->Color.r,
        pParticleSystem->Color.g,
        pParticleSystem->Color.b,
        pParticleSystem->Color.a);
    SDL_RenderFillRects(pRenderer, particleRects, numRects);
}

