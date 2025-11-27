#include "Manager.h"
#include <vector>
#include <windows.h>
#include <Xinput.h>
#pragma comment(lib, "Xinput.lib")

struct KeyboardState
{
    bool keys[256] = { false };
};

struct MouseState
{
    bool buttons[8] = { false };
    LONG x = 0;
    LONG y = 0;
};

struct PadState
{
    bool active = false;      // ê⁄ë±èÛë‘
    bool buttons[32] = { false };
    float axis[8] = { 0.0f };
};

static KeyboardState g_Keyboard;
static MouseState    g_Mouse;
static PadState      g_Controllers[4];  // XInput ç≈ëÂ 4ë‰

void InitInput()
{
    ZeroMemory(&g_Keyboard, sizeof(g_Keyboard));
    ZeroMemory(&g_Mouse, sizeof(g_Mouse));
    ZeroMemory(&g_Controllers, sizeof(g_Controllers));
}

void UpdateInput()
{
    // ------------------------------
    // Keyboard (èÌÇ…1Ç¬)
    // ------------------------------
    BYTE keyState[256];
    GetKeyboardState(keyState);

    for (int i = 0; i < 256; i++)
        g_Keyboard.keys[i] = (keyState[i] & 0x80);

    // ------------------------------
    // Mouse (OSÇ≈1Ç¬Ç…ìùçá)
    // ------------------------------
    POINT p;
    GetCursorPos(&p);
    g_Mouse.x = p.x;
    g_Mouse.y = p.y;

    g_Mouse.buttons[0] = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
    g_Mouse.buttons[1] = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
    g_Mouse.buttons[2] = GetAsyncKeyState(VK_MBUTTON) & 0x8000;

    // ------------------------------
    // GamePad é©ìÆê⁄ë±îªíË (ç≈ëÂ4)
    // ------------------------------
    for (int i = 0; i < 4; i++)
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(state));

        DWORD result = XInputGetState(i, &state);

        PadState& pad = g_Controllers[i];

        if (result == ERROR_SUCCESS)
        {
            // ê⁄ë±íÜ
            pad.active = true;

            WORD b = state.Gamepad.wButtons;
            ZeroMemory(pad.buttons, sizeof(pad.buttons));

            pad.buttons[Pad_A] = (b & XINPUT_GAMEPAD_A) != 0;
            pad.buttons[Pad_B] = (b & XINPUT_GAMEPAD_B) != 0;
            pad.buttons[Pad_X] = (b & XINPUT_GAMEPAD_X) != 0;
            pad.buttons[Pad_Y] = (b & XINPUT_GAMEPAD_Y) != 0;

            pad.buttons[Pad_L1] = (b & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
            pad.buttons[Pad_R1] = (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

            // Trigger
            pad.axis[0] = state.Gamepad.bLeftTrigger / 255.0f;
            pad.axis[1] = state.Gamepad.bRightTrigger / 255.0f;

            // Stick
            pad.axis[2] = state.Gamepad.sThumbLX / 32767.0f;
            pad.axis[3] = state.Gamepad.sThumbLY / 32767.0f;
            pad.axis[4] = state.Gamepad.sThumbRX / 32767.0f;
            pad.axis[5] = state.Gamepad.sThumbRY / 32767.0f;

            pad.buttons[Pad_D_UP] = (b & XINPUT_GAMEPAD_DPAD_UP);
            pad.buttons[Pad_D_DOWN] = (b & XINPUT_GAMEPAD_DPAD_DOWN);
            pad.buttons[Pad_D_LEFT] = (b & XINPUT_GAMEPAD_DPAD_LEFT);
            pad.buttons[Pad_D_RIGHT] = (b & XINPUT_GAMEPAD_DPAD_RIGHT);
        }
        else
        {
            // ñ¢ê⁄ë±
            pad.active = false;
            ZeroMemory(pad.buttons, sizeof(pad.buttons));
            ZeroMemory(pad.axis, sizeof(pad.axis));
        }
    }
}

void ReleaseInput()
{
    // ì¡Ç…âï˙Ç∑ÇÈÇ‡ÇÃÇ»Çµ
}

int GetKeyBoardNum() { return 1; }  // OSìIÇ…èÌÇ…1
int GetMouseNum() { return 1; }  // OSìIÇ…èÌÇ…1
int GetControllerNum()
{
    int n = 0;
    for (auto& p : g_Controllers)
        if (p.active) n++;
    return n;
}

bool GetInputState(Input input, int index)
{
    // --- Keyboard ---
    if (input >= Key_0 && input <= Key_F12)
    {
        int vk = 0;
        switch (input)
        {
        case Key_0: vk = '0'; break;
        case Key_1: vk = '1'; break;
        case Key_A: vk = 'A'; break;
        case Key_Z: vk = 'Z'; break;
        case Key_SPACE: vk = VK_SPACE; break;
        case Key_TAB: vk = VK_TAB; break;
        case Key_CTRL: vk = VK_CONTROL; break;
        case Key_SHIFT: vk = VK_SHIFT; break;
        case Key_ENTER: vk = VK_RETURN; break;
        case Key_BACKSPACE: vk = VK_BACK; break;
        case Key_ALT: vk = VK_MENU; break;
        case Key_ESC: vk = VK_ESCAPE; break;
        case Key_PGUP: vk = VK_PRIOR; break;
        case Key_PGDN: vk = VK_NEXT; break;
        case Key_HOME: vk = VK_HOME; break;
        case Key_END: vk = VK_END; break;
        case Key_F1: vk = VK_F1; break;
        case Key_F12: vk = VK_F12; break;
        }
        return g_Keyboard.keys[vk];
    }

    // --- Mouse ---
    if (input >= Mouse_LEFT && input <= Mouse_M5)
    {
        switch (input)
        {
        case Mouse_LEFT:   return g_Mouse.buttons[0];
        case Mouse_RIGHT:  return g_Mouse.buttons[1];
        case Mouse_CENTER: return g_Mouse.buttons[2];
        }
        return false;
    }

    // --- Gamepad ---
    if (input >= Pad_A && input <= Pad_D_DOWN)
    {
        if (index < 0 || index >= 4) return false;
        if (!g_Controllers[index].active) return false;
        return g_Controllers[index].buttons[input];
    }

    return false;
}

bool Input_GetKey(Input input)
{
    GetInputState(input, 0);
}
bool Input_GetPad(Input input, int index)
{
    GetInputState(input, index);
}