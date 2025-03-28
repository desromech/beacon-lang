#include "Context.h"
#include "Exceptions.h"
#include "Window.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif
#include <stdio.h>

static bool hasRegisteredWindowClass;

static LRESULT CALLBACK beacon_Window_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    beacon_Window_t *beaconWindow = (beacon_Window_t*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

    switch(message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if(beaconWindow)
            {
            }
            printf("WM Paint\n");
            EndPaint(hWnd, &ps);
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

    HWND windowHandle = CreateWindowExW(0, L"BeaconLangWindowClass", L"Beacon Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
    if(!windowHandle)
        beacon_exception_error(context, "Failed to create Win32 window.");
    SetWindowLongPtrA(windowHandle, GWLP_USERDATA, (LONG_PTR)beaconWindow);
    beaconWindow->handle = beacon_boxExternalAddress(context, windowHandle);

    ShowWindow(windowHandle, SW_NORMAL);
    UpdateWindow(windowHandle);

    return receiver;
}

static beacon_oop_t beacon_Window_displayForm(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;
    beacon_Form_t *form = (beacon_Form_t*)arguments[0];
    return receiver;
}

static beacon_oop_t beacon_Window_close(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)argumentCount;
    (void)arguments;

    beacon_Window_t *beaconWindow = (beacon_Window_t *)receiver;
    if(beaconWindow->handle)
        DestroyWindow(beacon_unboxExternalAddress(context, beaconWindow->handle));
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