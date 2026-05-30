#pragma once

#include <Windows.h>

namespace QuickStashInput {

inline void MoveCursorScreen(int x, int y) {
    SetCursorPos(x, y);
}

inline void CtrlDown() {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_CONTROL;
    SendInput(1, &in, sizeof(INPUT));
}

inline void CtrlUp() {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_CONTROL;
    in.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));
}

inline void LeftClickAtCursor() {
    INPUT inputs[2]{};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

inline void CtrlClickScreen(int x, int y) {
    CtrlDown();
    MoveCursorScreen(x, y);
    LeftClickAtCursor();
    CtrlUp();
}

inline bool IsRightMouseDown() {
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

inline void SleepMs(int ms) {
    if (ms > 0)
        Sleep(static_cast<DWORD>(ms));
}

} // namespace QuickStashInput
