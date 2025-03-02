#include "Context.h"
#include "Exceptions.h"
#include "Window.h"
#include "Dictionary.h"
#include "SDL.h"
#include "SDL_hints.h"
#include <stdlib.h>

static bool hasInitializedSDL2;
static bool isQuitting;
static void ensureSDL2Initialization(void)
{
    if(hasInitializedSDL2)
        return;

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");    
    SDL_Init(SDL_INIT_VIDEO);
}

static void beacon_sdl2_updateDisplayTextureExtent(beacon_context_t *context, beacon_Window_t *beaconWindow)
{
    SDL_Renderer *renderer = beacon_unboxExternalAddress(context, beaconWindow->rendererHandle);
    int textureWidth = beacon_decodeSmallInteger(beaconWindow->width);
    int textureHeight = beacon_decodeSmallInteger(beaconWindow->height);
    SDL_GetRendererOutputSize(renderer, &textureWidth, &textureHeight);

    if(!beaconWindow->textureHandle ||
        textureWidth != beacon_decodeSmallInteger(beaconWindow->textureWidth) ||
       textureHeight != beacon_decodeSmallInteger(beaconWindow->textureHeight))
    {
        SDL_Texture *oldTexture = beacon_unboxExternalAddress(context, beaconWindow->textureHandle);
        if(oldTexture)
            SDL_DestroyTexture(oldTexture);

        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
        beaconWindow->textureHandle = beacon_boxExternalAddress(context, texture);
        beaconWindow->textureWidth = beacon_encodeSmallInteger(textureWidth);
        beaconWindow->textureHeight = beacon_encodeSmallInteger(textureHeight);    
    }
}

static beacon_oop_t beacon_Window_open(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)argumentCount;
    (void)arguments;

    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;
    int width = beacon_decodeSmallInteger(beaconWindow->width);
    int height = beacon_decodeSmallInteger(beaconWindow->height);

    ensureSDL2Initialization();

    SDL_Window *sdlWindow = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if(!sdlWindow)
        beacon_exception_error(context, "Failed to create SDL window.");

    uint32_t sdlWindowID = SDL_GetWindowID(sdlWindow);
    beaconWindow->handle = beacon_encodeSmallInteger(sdlWindowID);
    beacon_MethodDictionary_atPut(context, context->roots.windowHandleMap, (beacon_Symbol_t*)beaconWindow->handle, (beacon_oop_t)beaconWindow);

    if(beaconWindow->useCustomRenderer != context->roots.trueValue)
    {
        SDL_Renderer *renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_PRESENTVSYNC);
        beaconWindow->rendererHandle = beacon_boxExternalAddress(context, renderer);
    
        beacon_sdl2_updateDisplayTextureExtent(context, beaconWindow);    
    }
    return receiver;
}

static beacon_oop_t beacon_Window_close(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)argumentCount;
    (void)arguments;

    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;

    SDL_Window *sdlWindow = SDL_GetWindowFromID(beacon_decodeCharacter(beaconWindow->handle));

    uint32_t sdlWindowID = SDL_GetWindowID(sdlWindow);
    beaconWindow->handle = beacon_encodeSmallInteger(sdlWindowID);
    beacon_MethodDictionary_atPut(context, context->roots.windowHandleMap, (beacon_Symbol_t*)beaconWindow->handle, (beacon_oop_t)beaconWindow);

    SDL_Texture *texture = beacon_unboxExternalAddress(context, beaconWindow->textureHandle);
    if(texture)
        SDL_DestroyTexture(texture);

    SDL_Renderer *renderer = beacon_unboxExternalAddress(context, beaconWindow->rendererHandle);
    if(texture)
        SDL_DestroyRenderer(renderer);
    
    SDL_DestroyWindow(sdlWindow);
    return receiver;
}

static beacon_oop_t beacon_Window_displayForm(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;
    beacon_Form_t *form = (beacon_Form_t*)arguments[0];

    SDL_Renderer *renderer = beacon_unboxExternalAddress(context, beaconWindow->rendererHandle);
    SDL_Texture *texture = beacon_unboxExternalAddress(context, beaconWindow->textureHandle);
    BeaconAssert(context, texture != NULL);

    int textureWidth = beacon_decodeSmallInteger(beaconWindow->textureWidth);
    int textureHeight = beacon_decodeSmallInteger(beaconWindow->textureHeight);

    int formWidth = beacon_decodeSmallInteger(form->width);
    int formHeight = beacon_decodeSmallInteger(form->height);

    int blitWidth = formWidth < textureWidth ? formWidth : textureWidth;
    int blitHeight = formHeight < textureHeight ? formHeight : textureHeight;
    int sourcePitch = beacon_decodeSmallInteger(form->pitch);
    uint8_t *sourceTexturePixels = form->bits->elements;

    // Lock the texture
    void *texturePixels = NULL;
    int texturePitch = 0;
    SDL_LockTexture(texture, NULL, &texturePixels, &texturePitch);

    uint8_t *sourceRow = sourceTexturePixels;
    uint8_t *destRow = texturePixels;
    for(int y = 0; y < blitHeight; ++y)
    {
        memcpy(destRow, sourceRow, blitWidth*4);
        sourceRow += sourcePitch;
        destRow += texturePitch;
    }

    // Unlock the texture.
    SDL_UnlockTexture(texture);

    // Blit the texture to the screen.
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    return receiver;
}

static void beacon_sdl2_fetchAndDispatchEvents(beacon_context_t *context)
{
    SDL_Event sdlEvent;
    while(SDL_PollEvent(&sdlEvent))
    {
        switch(sdlEvent.type)
        {
        case SDL_MOUSEBUTTONDOWN:
        {
            beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.button.windowID));
            if(beaconWindow)
            {
                beacon_WindowMouseButtonEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers);
                event->button = beacon_encodeSmallInteger(sdlEvent.button.button);
                event->x = beacon_encodeSmallInteger(sdlEvent.button.x);
                event->y = beacon_encodeSmallInteger(sdlEvent.button.y);
                beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseButtonDown:"), (beacon_oop_t)event);
            }
        }
            break;
        case SDL_MOUSEBUTTONUP:
        {
            beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.button.windowID));
            if(beaconWindow)
            {
                beacon_WindowMouseButtonEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers);
                event->button = beacon_encodeSmallInteger(sdlEvent.button.button);
                event->x = beacon_encodeSmallInteger(sdlEvent.button.x);
                event->y = beacon_encodeSmallInteger(sdlEvent.button.y);
                beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseButtonUp:"), (beacon_oop_t)event);
            }
        }
            break;
        case SDL_MOUSEMOTION:
        {
            beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.motion.windowID));
            if(beaconWindow)
            {
                beacon_WindowMouseMotionEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseMotionEventClass, sizeof(beacon_WindowMouseMotionEvent_t), BeaconObjectKindPointers);
                event->buttons = beacon_encodeSmallInteger(sdlEvent.motion.state);
                event->x = beacon_encodeSmallInteger(sdlEvent.motion.x);
                event->y = beacon_encodeSmallInteger(sdlEvent.motion.y);
                event->xrel = beacon_encodeSmallInteger(sdlEvent.motion.xrel);
                event->yrel = beacon_encodeSmallInteger(sdlEvent.motion.yrel);

                beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseMotion:"), (beacon_oop_t)event);
            }
        }
            break;
        case SDL_KEYDOWN:
        {
            beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.key.windowID));
            if(beaconWindow)
            {
                beacon_WindowKeyboardEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowKeyboardEventClass, sizeof(beacon_WindowKeyboardEvent_t), BeaconObjectKindPointers);
                event->scancode = beacon_encodeSmallInteger(sdlEvent.key.keysym.scancode);
                event->symbol = beacon_encodeSmallInteger(sdlEvent.key.keysym.sym);
                event->modstate = beacon_encodeSmallInteger(sdlEvent.key.keysym.mod);
                beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onKeyPressed:"), (beacon_oop_t)event);
            }
        }
            break;
        case SDL_KEYUP:
        {
            beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.key.windowID));
            if(beaconWindow)
            {
                beacon_WindowKeyboardEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowKeyboardEventClass, sizeof(beacon_WindowKeyboardEvent_t), BeaconObjectKindPointers);
                event->scancode = beacon_encodeSmallInteger(sdlEvent.key.keysym.scancode);
                event->symbol = beacon_encodeSmallInteger(sdlEvent.key.keysym.sym);
                event->modstate = beacon_encodeSmallInteger(sdlEvent.key.keysym.mod);
                beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onKeyReleased:"), (beacon_oop_t)event);
            }
        }
            break;
        case SDL_TEXTINPUT:
        {
            beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.key.windowID));
            if(beaconWindow)
            {
                beacon_WindowTextInputEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowTextInputEventClass, sizeof(beacon_WindowTextInputEvent_t), BeaconObjectKindPointers);
                event->text = (beacon_oop_t)beacon_importCString(context, sdlEvent.text.text);
                beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onTextInput:"), (beacon_oop_t)event);
            }

        }
            break;
        case SDL_WINDOWEVENT:
            switch(sdlEvent.window.event)
            {
            case SDL_WINDOWEVENT_EXPOSED:
            {
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                    (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.window.windowID));
                if(beaconWindow)
                {
                    beacon_sdl2_updateDisplayTextureExtent(context, beaconWindow);
                    beacon_WindowExposeEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowExposeEventClass, sizeof(beacon_WindowExposeEvent_t), BeaconObjectKindPointers);
                    beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onExpose:"), (beacon_oop_t)event);
                }
            }
                break;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            {
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                    (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.window.windowID));
                if(beaconWindow)
                {
                    SDL_Window *sdlWindow = SDL_GetWindowFromID(sdlEvent.window.windowID);
                    int newWidth = 0;
                    int newHeight = 0;

                    SDL_GetWindowSize(sdlWindow, &newWidth, &newHeight);

                    beaconWindow->width = beacon_encodeSmallInteger(newWidth);
                    beaconWindow->height = beacon_encodeSmallInteger(newHeight);

                    beacon_sdl2_updateDisplayTextureExtent(context, beaconWindow);
                    beacon_perform(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onSizeChanged"));
                }
            }
            break;
            case SDL_WINDOWEVENT_CLOSE:
            {
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                    (beacon_Symbol_t*)beacon_encodeSmallInteger(sdlEvent.window.windowID));
                if(beaconWindow)
                {
                    SDL_Window *sdlWindow = SDL_GetWindowFromID(sdlEvent.window.windowID);
                    beacon_perform(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onCloseRequest"));
                }
            }
            break;
            }
            break;
        case SDL_QUIT:
            isQuitting = true;
            break;
        }
    }
}

static beacon_oop_t beacon_WindowClass_enterMainLoop(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)argumentCount;
    (void)arguments;

    while(!isQuitting)
    {
        beacon_sdl2_fetchAndDispatchEvents(context);
    }

    return receiver;
}

void beacon_context_registerWindowSystemPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "open", 0, beacon_Window_open);
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "close", 0, beacon_Window_close);
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "displayForm:", 1, beacon_Window_displayForm);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.windowClass), "enterMainLoop", 0, beacon_WindowClass_enterMainLoop);
}