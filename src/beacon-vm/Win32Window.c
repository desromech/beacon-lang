#include "Context.h"
#include "Exceptions.h"
#include "Window.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <windowsx.h>
#endif
#include <stdio.h>
#include <stdlib.h>

static bool hasRegisteredWindowClass;

typedef struct beacon_WindowUserData_s
{
    beacon_context_t *context;
    beacon_Window_t *beaconWindow;
    HDC paintDC;
    int mouseX;
    int mouseY;
} beacon_WindowUserData_t;

static void beacon_win32_updateDisplayExtent(beacon_context_t *context, beacon_Window_t *beaconWindow);

int mapMouseButtonFromEvent(UINT message)
{
    switch(message)
    {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        return 0;
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        return 1;
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        return 2;
    }
}

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
    case WM_SIZE:
        beacon_win32_updateDisplayExtent(context, beaconWindow);
        beacon_perform(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onSizeChanged"));
        break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            userData->mouseX = xPos;
            userData->mouseY = yPos;

            beacon_WindowMouseButtonEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers);
            event->button = beacon_encodeSmallInteger(mapMouseButtonFromEvent(message));
            event->x = beacon_encodeSmallInteger(xPos);
            event->y = beacon_encodeSmallInteger(yPos);
            beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseButtonDown:"), (beacon_oop_t)event);
        } break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            userData->mouseX = xPos;
            userData->mouseY = yPos;

            beacon_WindowMouseButtonEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseButtonEventClass, sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers);
            event->button = beacon_encodeSmallInteger(mapMouseButtonFromEvent(message));
            event->x = beacon_encodeSmallInteger(xPos);
            event->y = beacon_encodeSmallInteger(yPos);
            beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseButtonUp:"), (beacon_oop_t)event);
        } break;
        case WM_MOUSEMOVE:
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            int xRel = xPos - userData->mouseX;
            int yRel = yPos - userData->mouseY;
            userData->mouseX = xPos;
            userData->mouseY = yPos;

            uint32_t buttons = 0;
            if(wParam & MK_LBUTTON)
                buttons |= 1;
            if(wParam & MK_MBUTTON)
                buttons |= 2;
            if(wParam & MK_RBUTTON)
                buttons |= 4;

            beacon_WindowMouseMotionEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowMouseMotionEventClass, sizeof(beacon_WindowMouseMotionEvent_t), BeaconObjectKindPointers);
            event->buttons = beacon_encodeSmallInteger(buttons);
            event->x = beacon_encodeSmallInteger(xPos);
            event->y = beacon_encodeSmallInteger(yPos);
            event->xrel = beacon_encodeSmallInteger(xRel);
            event->yrel = beacon_encodeSmallInteger(yRel);
            beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onMouseMotion:"), (beacon_oop_t)event);
        } break;
    case WM_KEYDOWN:
        {
            WORD vkCode = LOWORD(wParam);
            WORD keyFlags = HIWORD(lParam);
            WORD scanCode = LOBYTE(keyFlags);
            WORD repeatCount = LOWORD(lParam);

            beacon_WindowKeyboardEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowKeyboardEventClass, sizeof(beacon_WindowKeyboardEvent_t), BeaconObjectKindPointers);
            event->scancode = beacon_encodeSmallInteger(scanCode);
            event->symbol = beacon_encodeSmallInteger(vkCode);
            event->modstate = beacon_encodeSmallInteger(0);
            beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onKeyPressed:"), (beacon_oop_t)event);
        } break;
    case WM_KEYUP:
        {
            WORD vkCode = LOWORD(wParam);
            WORD keyFlags = HIWORD(lParam);
            WORD scanCode = LOBYTE(keyFlags);
            WORD repeatCount = LOWORD(lParam);

            beacon_WindowKeyboardEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowKeyboardEventClass, sizeof(beacon_WindowKeyboardEvent_t), BeaconObjectKindPointers);
            event->scancode = beacon_encodeSmallInteger(scanCode);
            event->symbol = beacon_encodeSmallInteger(vkCode);
            event->modstate = beacon_encodeSmallInteger(0);
            beacon_performWith(context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onKeyReleased:"), (beacon_oop_t)event);
        } break;
    case WM_CHAR:
        {

        } break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            userData->paintDC = BeginPaint(hWnd, &ps);
            beacon_WindowExposeEvent_t *event = beacon_allocateObjectWithBehavior(context->heap, context->classes.windowExposeEventClass, sizeof(beacon_WindowExposeEvent_t), BeaconObjectKindPointers);
            beacon_performWith(userData->context, (beacon_oop_t)beaconWindow, (beacon_oop_t)beacon_internCString(context, "onExpose:"), (beacon_oop_t)event);
            EndPaint(hWnd, &ps);
            userData->paintDC = NULL;
        } break;
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
        .style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW,
        .lpfnWndProc = beacon_Window_proc,
        .hInstance = GetModuleHandle(NULL),
        .lpszClassName = L"BeaconLangWindowClass",
    };

    RegisterClassExW(&class);
    hasRegisteredWindowClass = true;
}

static void beacon_win32_updateDisplayExtent(beacon_context_t *context, beacon_Window_t *beaconWindow)
{
    RECT clientRect;
    HWND window = beacon_unboxExternalAddress(context, beaconWindow->handle);
    GetClientRect(window, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;
    beaconWindow->width = beacon_encodeSmallInteger(clientWidth);
    beaconWindow->height = beacon_encodeSmallInteger(clientHeight);

    if(beaconWindow->textureWidth && clientWidth == beacon_decodeSmallInteger(beaconWindow->textureWidth) &&
        beaconWindow->textureHeight && clientHeight == beacon_decodeSmallInteger(beaconWindow->textureHeight))
        return;

    beaconWindow->textureWidth = beacon_encodeSmallInteger(clientWidth);
    beaconWindow->textureHeight = beacon_encodeSmallInteger(clientHeight);
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
    beacon_win32_updateDisplayExtent(context, beaconWindow);

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

    int formWidth = (int)beacon_decodeSmallInteger(form->width);
    int formHeight = (int)beacon_decodeSmallInteger(form->height);
    int formDepth = (int)beacon_decodeSmallInteger(form->depth);
    BeaconAssert(context, formDepth == 32);

    RECT clientRect;
    GetClientRect(window, &clientRect);
    int windowWidth = clientRect.right - clientRect.left; 
    int windowHeight = clientRect.bottom - clientRect.top;

    BITMAPINFO bitmapInfo = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = formWidth,
            .biHeight = -formHeight,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        }
    };

    if(userData->paintDC)
    {
        StretchDIBits(userData->paintDC,
            0, 0, windowWidth, windowHeight,
            0, 0, formWidth, formHeight,
            form->bits->elements,
            &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        HDC dc = GetDC(window);
        StretchDIBits(dc,
            0, 0, windowWidth, windowHeight,
            0, 0, formWidth, formHeight,
            form->bits->elements, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(window, dc);
    }

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

static beacon_oop_t beacon_Window_hasAcceleratedRendering(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    return context->roots.trueValue;
}

void beacon_context_registerWindowSystemPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "open", 0, beacon_Window_open);
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "displayForm:", 1, beacon_Window_displayForm);
    beacon_addPrimitiveToClass(context, context->classes.windowClass, "close", 0, beacon_Window_close);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.windowClass), "enterMainLoop", 0, beacon_WindowClass_enterMainLoop);
}