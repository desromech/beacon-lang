#include "Context.h"
#include "Exceptions.h"
#include "Window.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

static bool hasRegisteredWindowClass;

typedef struct beacon_WindowUserData_s
{
    beacon_context_t *context;
    beacon_Window_t *beaconWindow;
    HDC paintDC;
} beacon_WindowUserData_t;

static LRESULT CALLBACK beacon_Window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_CREATE)
    {
        CREATESTRUCTW *createParam = (CREATESTRUCTW *)lParam;
        createParam->lpCreateParams;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)createParam->lpCreateParams);
    }

    beacon_WindowUserData_t *userData = (beacon_WindowUserData_t*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
    if(!userData)
        return DefWindowProcW(hWnd, message, wParam, lParam);

    beacon_context_t *context = userData->context;
    beacon_Window_t *beaconWindow = userData->beaconWindow;

    switch(message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            userData->paintDC = BeginPaint(hWnd, &ps);
            beacon_WindowExposeEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowExposeEventClass, sizeof(beacon_WindowExposeEvent_t), BeaconObjectKindPointers);
            beacon_performWith(userData->context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onExpose:"), (beacon_oop_t)event);
            EndPaint(hWnd, &ps);
            userData->paintDC = NULL;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

static void registerWindowClass()
{
    if(hasRegisteredWindowClass)
        return;

    WNDCLASSEXW class = {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = beacon_Window_proc,
        .hInstance = GetModuleHandle(NULL),
        .lpszClassName = L"BeaconLangWindowClass",
    };

    RegisterClassExW(&class);
    hasRegisteredWindowClass = true;
}

static void beacon_win32_updateDisplayTextureExtent(beacon_context_t *context, beacon_Window_t *beaconWindow)
{
    if(beaconWindow->width == beaconWindow->textureWidth && beaconWindow->height == beaconWindow->textureHeight)
        return;

    beaconWindow->textureWidth = beaconWindow->width;
    beaconWindow->textureHeight = beaconWindow->height;
}

static beacon_oop_t beacon_Window_open(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)argumentCount;
    (void)arguments;

    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;
    int width = (int)beacon_decodeSmallInteger(beaconWindow->width);
    int height = (int)beacon_decodeSmallInteger(beaconWindow->height);

    registerWindowClass();

    // Disable accelerated rendering for now..
    beaconWindow->useAcceleratedRendering = context->roots.falseValue;

    beacon_WindowUserData_t *windowUserData = calloc(1, sizeof(beacon_WindowUserData_t));
    windowUserData->context = context;
    windowUserData->beaconWindow = beaconWindow;

    HWND windowHandle = CreateWindowExW(0, L"BeaconLangWindowClass", L"Beacon Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), windowUserData);
    if(!windowHandle)
        beacon_exception_error(context, "Failed to create Win32 window.");
    beaconWindow->handle = beacon_boxExternalAddress(context, windowHandle);
    beacon_win32_updateDisplayTextureExtent(context, beaconWindow);

    ShowWindow(windowHandle, SW_NORMAL);
    UpdateWindow(windowHandle);

    return receiver;
}

static beacon_oop_t beacon_Window_displayForm(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;
    beacon_Form_t *form = (beacon_Form_t*)arguments[0];

    HWND window = beacon_unboxExternalAddress(context, beaconWindow->handle);
    beacon_WindowUserData_t *userData = (beacon_WindowUserData_t*)GetWindowLongPtrA(window, GWLP_USERDATA);

    return receiver;
}

static beacon_oop_t beacon_Window_close(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)argumentCount;
    (void)arguments;

    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;
    if(beaconWindow->handle)
    {
        HWND window = beacon_unboxExternalAddress(context, beaconWindow->handle);
        beacon_WindowUserData_t *userData = (beacon_WindowUserData_t*)GetWindowLongPtrA(window, GWLP_USERDATA);
        free(userData);
        DestroyWindow(window);
    }
    beaconWindow->handle = 0;
    return receiver;
}

static beacon_oop_t beacon_WindowClass_enterMainLoop(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)argumentCount;
    (void)arguments;

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return receiver;
}

void beacon_context_registerWindowSystemPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "open", 0, beacon_Window_open);
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "displayForm:", 1, beacon_Window_displayForm);
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "close", 0, beacon_Window_close);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.windowClass), "enterMainLoop", 0, beacon_WindowClass_enterMainLoop);
}