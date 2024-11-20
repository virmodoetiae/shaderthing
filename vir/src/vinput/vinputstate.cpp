#include "vpch.h"

// Platform-specific includes and function declarations
#ifdef _WIN32
    #include <windows.h>
#elif defined(__APPLE__)
    #include <ApplicationServices/ApplicationServices.h>
#elif defined(__linux__)
    #include <X11/Xlib.h>
    #include <X11/extensions/XTest.h>
#endif

namespace vir
{

void InputState::reset()
{
    for (int i=0; i<VIR_N_KEYS; i++)
        keyState_[i].reset();
    for (int i=0; i<VIR_N_MOUSE_BUTTONS; i++)
        mouseButtonState_[i].reset();
}

void InputState::onReceive(Event::KeyPressEvent& event)
{
    KeyState& key(keyState_[event.keyCode]);
    if (key.isPressedOrHeld())
        key.setHeld(true);
    else
    {
        key.switchToggle();
        key.setPressed(true);
    }
}

void InputState::onReceive(Event::KeyReleaseEvent& event)
{
    keyState_[event.keyCode].setPressed(false);
}

void InputState::onReceive(Event::MouseButtonPressEvent& event)
{
    MouseButtonState& mouseButton(mouseButtonState_[event.button]);
    mouseButton.switchToggle();
    mouseButton.setClicked(true);
}

void InputState::onReceive(Event::MouseButtonReleaseEvent& event)
{
    mouseButtonState_[event.button].setClicked(false);
}

void InputState::onReceive(Event::MouseMotionEvent& event)
{
    mousePosition_.x = event.x;
    mousePosition_.y = event.y;
}

void InputState::setMousePositionNativeOS
(
    MousePosition position, 
    int msDelay
)
{
#ifdef _WIN32
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = position.x * (65535 / GetSystemMetrics(SM_CXSCREEN));
    input.mi.dy = position.y * (65535 / GetSystemMetrics(SM_CYSCREEN));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(msDelay);
#elif defined(__linux__)
    ::Display* display = XOpenDisplay(NULL);
    if (display != NULL) 
    {
        ::Window root = DefaultRootWindow(display);
        XWarpPointer(display, None, root, 0, 0, 0, 0, position.x, position.y); 
        XFlush(display);
        XCloseDisplay(display);
    }
#elif defined(__APPLE__)
    CGWarpMouseCursorPosition(CGPointMake(position.x, position.y));
    CGAssociateMouseAndMouseCursorPosition(true); 
#endif
}

void InputState::leftMouseButtonClickNativeOS(int msDelay)
{
#ifdef _WIN32
    
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(msDelay);
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(msDelay);
#elif defined(__linux__)
    ::Display* display = XOpenDisplay(NULL);
    if (display != NULL) 
    {
        XTestFakeButtonEvent(display, 1, True, CurrentTime);
        XTestFakeButtonEvent(display, 1, False, CurrentTime);
        XFlush(display);
        XCloseDisplay(display);
    }
#elif defined(__APPLE__)
    CGEventRef click_down = CGEventCreateMouseEvent(
        NULL, kCGEventLeftMouseDown, CGPointMake(0, 0), kCGMouseButtonLeft);
    CGEventRef click_up = CGEventCreateMouseEvent(
        NULL, kCGEventLeftMouseUp, CGPointMake(0, 0), kCGMouseButtonLeft);
    if (click_down && click_up) {
        CGEventPost(kCGHIDEventTap, click_down);
        CGEventPost(kCGHIDEventTap, click_up);
        CFRelease(click_down);
        CFRelease(click_up);
    }
#endif
}

}
