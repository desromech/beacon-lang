#include "Context.h"
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
    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t valwin[] = {
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
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
            }
            break;
        case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *expose = (xcb_button_press_event_t*)event;
                printf("Mouse button press\n");
            }
            break;
        case XCB_BUTTON_RELEASE:
            {
                xcb_button_press_event_t *expose = (xcb_button_press_event_t*)event;
                printf("Mouse button release\n");
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