#include "Context.h"
#include "Dictionary.h"
#include "Window.h"
#include <xcb/xcb.h>
#include <stdio.h>

static xcb_connection_t *xcb_connection = NULL;

static beacon_oop_t beacon_Window_open(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_Window_t *beaconWindow = (beacon_Window_t*)receiver;
    if(!xcb_connection)
        xcb_connection = xcb_connect(NULL, NULL);

    // Get the first screen
    const xcb_setup_t      *setup  = xcb_get_setup (xcb_connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
    xcb_screen_t           *screen = iter.data;

    // Create the window
    xcb_window_t windowHandle = xcb_generate_id(xcb_connection);
    beaconWindow->handle = beacon_encodeSmallInteger(windowHandle);
    
    beacon_MethodDictionary_atPut(context, context->roots.windowHandleMap, (beacon_Symbol_t*)beaconWindow->handle, (beacon_oop_t)beaconWindow);
    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t valwin[] = {
        XCB_EVENT_MASK_EXPOSURE
            | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE 
            | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    };

    xcb_create_window(xcb_connection, XCB_COPY_FROM_PARENT, windowHandle, screen->root,
            0, 0, beacon_decodeSmallInteger(beaconWindow->width), beacon_decodeSmallInteger(beaconWindow->height),
        10, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
        mask, valwin);

    xcb_map_window (xcb_connection, windowHandle);
    xcb_flush (xcb_connection);

    (void)context;
    (void)receiver;
    (void)argumentCount;
    (void)arguments;
    return receiver;
}

static beacon_oop_t beacon_WindowClass_enterMainLoop(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    xcb_generic_event_t *event;
    while((event = xcb_wait_for_event(xcb_connection)))
    {
        switch (event->response_type & ~0x80)
        {
        case XCB_EXPOSE:
            {
                xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                        (beacon_Symbol_t*)beacon_encodeSmallInteger(expose->window));
                if(beaconWindow)
                {
                    beacon_WindowExposeEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowExposeEventClass, sizeof(beacon_WindowExposeEvent_t), BeaconObjectKindPointers);
                    beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onExpose:"), (beacon_oop_t)event);
                }
                
            }
            break;
        case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *pressEvent = (xcb_button_press_event_t*)event;
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                    (beacon_Symbol_t*)beacon_encodeSmallInteger(pressEvent->event));
                if(beaconWindow)
                {
                    beacon_WindowMouseButtonEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers);
                    beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseButtonDown:"), (beacon_oop_t)event);
                }
            }
            break;
        case XCB_BUTTON_RELEASE:
            {
                xcb_button_release_event_t *releaseEvent = (xcb_button_release_event_t*)event;
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                    (beacon_Symbol_t*)beacon_encodeSmallInteger(releaseEvent->event));
                if(beaconWindow)
                {
                    beacon_WindowMouseButtonEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers);
                    beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseButtonUp:"), (beacon_oop_t)event);
                }
            }
            break;
            case XCB_KEY_PRESS:
            {
                xcb_button_release_event_t *releaseEvent = (xcb_button_release_event_t*)event;
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                    (beacon_Symbol_t*)beacon_encodeSmallInteger(releaseEvent->event));
                if(beaconWindow)
                {
                    beacon_WindowKeyboardEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowKeyboardEvent_t), BeaconObjectKindPointers);
                    beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onKeyPressed:"), (beacon_oop_t)event);
                }
            }
            break;
            case XCB_KEY_RELEASE:
            {
                xcb_button_release_event_t *releaseEvent = (xcb_button_release_event_t*)event;
                beacon_Window_t *beaconWindow = (beacon_Window_t*)beacon_MethodDictionary_atOrNil(context, context->roots.windowHandleMap,
                    (beacon_Symbol_t*)beacon_encodeSmallInteger(releaseEvent->event));
                if(beaconWindow)
                {
                    beacon_WindowKeyboardEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowKeyboardEvent_t), BeaconObjectKindPointers);
                    beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onKeyReleased:"), (beacon_oop_t)event);
                }
            }
            break;
        }

    }
    return receiver;
}

void beacon_context_registerWindowSystemPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "open", 0, beacon_Window_open);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.windowClass), "enterMainLoop", 0, beacon_WindowClass_enterMainLoop);
}