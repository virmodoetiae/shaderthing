#include "vpch.h"

namespace vir
{

std::unordered_map<int, std::string> keyCodeToName = 
{
    {VIR_KEY_UNKNOWN        , "UNKNOWN"},
    {VIR_KEY_SPACE          , "SPACE"},
    {VIR_KEY_APOSTROPHE     , "'"},
    {VIR_KEY_COMMA          , ","},
    {VIR_KEY_MINUS          , "-"},
    {VIR_KEY_PERIOD         , "."},
    {VIR_KEY_SLASH          , "/"},
    {VIR_KEY_0              , "0"},
    {VIR_KEY_1              , "1"},
    {VIR_KEY_2              , "2"},
    {VIR_KEY_3              , "3"},
    {VIR_KEY_4              , "4"},
    {VIR_KEY_5              , "5"},
    {VIR_KEY_6              , "6"},
    {VIR_KEY_7              , "7"},
    {VIR_KEY_8              , "8"},
    {VIR_KEY_9              , "9"},
    {VIR_KEY_SEMICOLON      , ";"},
    {VIR_KEY_EQUAL          , "="},
    {VIR_KEY_A              , "A"},
    {VIR_KEY_B              , "B"},
    {VIR_KEY_C              , "C"},
    {VIR_KEY_D              , "D"},
    {VIR_KEY_E              , "E"},
    {VIR_KEY_F              , "F"},
    {VIR_KEY_G              , "G"},
    {VIR_KEY_H              , "H"},
    {VIR_KEY_I              , "I"},
    {VIR_KEY_J              , "J"},
    {VIR_KEY_K              , "K"},
    {VIR_KEY_L              , "L"},
    {VIR_KEY_M              , "M"},
    {VIR_KEY_N              , "N"},
    {VIR_KEY_O              , "O"},
    {VIR_KEY_P              , "P"},
    {VIR_KEY_Q              , "Q"},
    {VIR_KEY_R              , "R"},
    {VIR_KEY_S              , "S"},
    {VIR_KEY_T              , "T"},
    {VIR_KEY_U              , "U"},
    {VIR_KEY_V              , "V"},
    {VIR_KEY_W              , "W"},
    {VIR_KEY_X              , "X"},
    {VIR_KEY_Y              , "Y"},
    {VIR_KEY_Z              , "Z"},
    {VIR_KEY_LEFT_BRACKET   , "("},
    {VIR_KEY_BACKSLASH      , "\\"},
    {VIR_KEY_RIGHT_BRACKET  , ")"},
    {VIR_KEY_GRAVE_ACCENT   , "`"},
    {VIR_KEY_WORLD_1        , "WORLD 1"},
    {VIR_KEY_WORLD_2        , "WORLD 2"},
    {VIR_KEY_ESCAPE         , "ESCAPE"},
    {VIR_KEY_ENTER          , "ENTER"},
    {VIR_KEY_TAB            , "TAB"},
    {VIR_KEY_BACKSPACE      , "BACKSPACE"},
    {VIR_KEY_INSERT         , "INSERT"},
    {VIR_KEY_DELETE         , "DELETE"},
    {VIR_KEY_RIGHT          , "RIGHT ARROW"},
    {VIR_KEY_LEFT           , "LEFT ARROW"},
    {VIR_KEY_DOWN           , "DOWN ARROW"},
    {VIR_KEY_UP             , "UP ARROW"},
    {VIR_KEY_PAGE_UP        , "PAGE UP"},
    {VIR_KEY_PAGE_DOWN      , "PAGE_DOWN"},
    {VIR_KEY_HOME           , "HOME"},
    {VIR_KEY_END            , "END"},
    {VIR_KEY_CAPS_LOCK      , "CAPS LOCK"},
    {VIR_KEY_SCROLL_LOCK    , "SCROLL LOCK"},
    {VIR_KEY_NUM_LOCK       , "NUM LOCK"},
    {VIR_KEY_PRINT_SCREEN   , "PRINT SCREEN"},
    {VIR_KEY_PAUSE          , "PAUSE"},
    {VIR_KEY_F1             , "F1"},
    {VIR_KEY_F2             , "F2"},
    {VIR_KEY_F3             , "F3"},
    {VIR_KEY_F4             , "F4"},
    {VIR_KEY_F5             , "F5"},
    {VIR_KEY_F6             , "F6"},
    {VIR_KEY_F7             , "F7"},
    {VIR_KEY_F8             , "F8"},
    {VIR_KEY_F9             , "F9"},
    {VIR_KEY_F10            , "F10"},
    {VIR_KEY_F11            , "F11"},
    {VIR_KEY_F12            , "F12"},
    {VIR_KEY_F13            , "F13"},
    {VIR_KEY_F14            , "F14"},
    {VIR_KEY_F15            , "F15"},
    {VIR_KEY_F16            , "F16"},
    {VIR_KEY_F17            , "F17"},
    {VIR_KEY_F18            , "F18"},
    {VIR_KEY_F19            , "F19"},
    {VIR_KEY_F20            , "F20"},
    {VIR_KEY_F21            , "F21"},
    {VIR_KEY_F22            , "F22"},
    {VIR_KEY_F23            , "F23"},
    {VIR_KEY_F24            , "F24"},
    {VIR_KEY_F25            , "F25"},
    {VIR_KEY_KP_0           , "0 (Keypad)"},
    {VIR_KEY_KP_1           , "1 (Keypad)"},
    {VIR_KEY_KP_2           , "2 (Keypad)"},
    {VIR_KEY_KP_3           , "3 (Keypad)"},
    {VIR_KEY_KP_4           , "4 (Keypad)"},
    {VIR_KEY_KP_5           , "5 (Keypad)"},
    {VIR_KEY_KP_6           , "6 (Keypad)"},
    {VIR_KEY_KP_7           , "7 (Keypad)"},
    {VIR_KEY_KP_8           , "8 (Keypad)"},
    {VIR_KEY_KP_9           , "9 (Keypad)"},
    {VIR_KEY_KP_DECIMAL     , ". (Keypad)"},
    {VIR_KEY_KP_DIVIDE      , "/ (Keypad)"},
    {VIR_KEY_KP_MULTIPLY    , "* (Keypad)"},
    {VIR_KEY_KP_SUBTRACT    , "- (Keypad)"},
    {VIR_KEY_KP_ADD         , "+ (Keypad)"},
    {VIR_KEY_KP_ENTER       , "ENTER (Keypad)"},
    {VIR_KEY_KP_EQUAL       , "= (Keypad)"},
    {VIR_KEY_LEFT_SHIFT     , "LEFT SHIFT"},
    {VIR_KEY_LEFT_CONTROL   , "LEFT CTRL"},
    {VIR_KEY_LEFT_ALT       , "LEFT ALT"},
    {VIR_KEY_LEFT_SUPER     , "LEFT SUPER"},
    {VIR_KEY_RIGHT_SHIFT    , "RIGHT SHIFT"},
    {VIR_KEY_RIGHT_CONTROL  , "RIGHT CONTROL"},
    {VIR_KEY_RIGHT_ALT      , "RIGHT ALT"},
    {VIR_KEY_RIGHT_SUPER    , "RIGHT SUPER"},
    {VIR_KEY_MENU           , "MENU"}
};

int inputKeyCodeVirToGlfw(int key)
{
    return key;
}

int inputMouseCodeVirToGlfw(int key)
{
    return key;
}

int inputKeyCodeGlfwToVir(int key)
{
    return key;
}

int inputMouseCodeGlfwToVir(int key)
{
    return key;
}

int inputKeyCodeVirToImGui(int key)
{
    switch (key)
    {
        case VIR_KEY_TAB: return ImGuiKey_Tab;
        case VIR_KEY_LEFT: return ImGuiKey_LeftArrow;
        case VIR_KEY_RIGHT: return ImGuiKey_RightArrow;
        case VIR_KEY_UP: return ImGuiKey_UpArrow;
        case VIR_KEY_DOWN: return ImGuiKey_DownArrow;
        case VIR_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case VIR_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case VIR_KEY_HOME: return ImGuiKey_Home;
        case VIR_KEY_END: return ImGuiKey_End;
        case VIR_KEY_INSERT: return ImGuiKey_Insert;
        case VIR_KEY_DELETE: return ImGuiKey_Delete;
        case VIR_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case VIR_KEY_SPACE: return ImGuiKey_Space;
        case VIR_KEY_ENTER: return ImGuiKey_Enter;
        case VIR_KEY_ESCAPE: return ImGuiKey_Escape;
        case VIR_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case VIR_KEY_COMMA: return ImGuiKey_Comma;
        case VIR_KEY_MINUS: return ImGuiKey_Minus;
        case VIR_KEY_PERIOD: return ImGuiKey_Period;
        case VIR_KEY_SLASH: return ImGuiKey_Slash;
        case VIR_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case VIR_KEY_EQUAL: return ImGuiKey_Equal;
        case VIR_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case VIR_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case VIR_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case VIR_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case VIR_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case VIR_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case VIR_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case VIR_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case VIR_KEY_PAUSE: return ImGuiKey_Pause;
        case VIR_KEY_KP_0: return ImGuiKey_Keypad0;
        case VIR_KEY_KP_1: return ImGuiKey_Keypad1;
        case VIR_KEY_KP_2: return ImGuiKey_Keypad2;
        case VIR_KEY_KP_3: return ImGuiKey_Keypad3;
        case VIR_KEY_KP_4: return ImGuiKey_Keypad4;
        case VIR_KEY_KP_5: return ImGuiKey_Keypad5;
        case VIR_KEY_KP_6: return ImGuiKey_Keypad6;
        case VIR_KEY_KP_7: return ImGuiKey_Keypad7;
        case VIR_KEY_KP_8: return ImGuiKey_Keypad8;
        case VIR_KEY_KP_9: return ImGuiKey_Keypad9;
        case VIR_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case VIR_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case VIR_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case VIR_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case VIR_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case VIR_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case VIR_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case VIR_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case VIR_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case VIR_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case VIR_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case VIR_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case VIR_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case VIR_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case VIR_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case VIR_KEY_MENU: return ImGuiKey_Menu;
        case VIR_KEY_0: return ImGuiKey_0;
        case VIR_KEY_1: return ImGuiKey_1;
        case VIR_KEY_2: return ImGuiKey_2;
        case VIR_KEY_3: return ImGuiKey_3;
        case VIR_KEY_4: return ImGuiKey_4;
        case VIR_KEY_5: return ImGuiKey_5;
        case VIR_KEY_6: return ImGuiKey_6;
        case VIR_KEY_7: return ImGuiKey_7;
        case VIR_KEY_8: return ImGuiKey_8;
        case VIR_KEY_9: return ImGuiKey_9;
        case VIR_KEY_A: return ImGuiKey_A;
        case VIR_KEY_B: return ImGuiKey_B;
        case VIR_KEY_C: return ImGuiKey_C;
        case VIR_KEY_D: return ImGuiKey_D;
        case VIR_KEY_E: return ImGuiKey_E;
        case VIR_KEY_F: return ImGuiKey_F;
        case VIR_KEY_G: return ImGuiKey_G;
        case VIR_KEY_H: return ImGuiKey_H;
        case VIR_KEY_I: return ImGuiKey_I;
        case VIR_KEY_J: return ImGuiKey_J;
        case VIR_KEY_K: return ImGuiKey_K;
        case VIR_KEY_L: return ImGuiKey_L;
        case VIR_KEY_M: return ImGuiKey_M;
        case VIR_KEY_N: return ImGuiKey_N;
        case VIR_KEY_O: return ImGuiKey_O;
        case VIR_KEY_P: return ImGuiKey_P;
        case VIR_KEY_Q: return ImGuiKey_Q;
        case VIR_KEY_R: return ImGuiKey_R;
        case VIR_KEY_S: return ImGuiKey_S;
        case VIR_KEY_T: return ImGuiKey_T;
        case VIR_KEY_U: return ImGuiKey_U;
        case VIR_KEY_V: return ImGuiKey_V;
        case VIR_KEY_W: return ImGuiKey_W;
        case VIR_KEY_X: return ImGuiKey_X;
        case VIR_KEY_Y: return ImGuiKey_Y;
        case VIR_KEY_Z: return ImGuiKey_Z;
        case VIR_KEY_F1: return ImGuiKey_F1;
        case VIR_KEY_F2: return ImGuiKey_F2;
        case VIR_KEY_F3: return ImGuiKey_F3;
        case VIR_KEY_F4: return ImGuiKey_F4;
        case VIR_KEY_F5: return ImGuiKey_F5;
        case VIR_KEY_F6: return ImGuiKey_F6;
        case VIR_KEY_F7: return ImGuiKey_F7;
        case VIR_KEY_F8: return ImGuiKey_F8;
        case VIR_KEY_F9: return ImGuiKey_F9;
        case VIR_KEY_F10: return ImGuiKey_F10;
        case VIR_KEY_F11: return ImGuiKey_F11;
        case VIR_KEY_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

int inputMouseCodeVirToImGui(int key)
{
    switch (key)
    {
        case VIR_MOUSE_BUTTON_LEFT: return ImGuiKey_MouseLeft;
        case VIR_MOUSE_BUTTON_MIDDLE: return ImGuiKey_MouseMiddle;
        case VIR_MOUSE_BUTTON_RIGHT: return ImGuiKey_MouseRight;
        default: return ImGuiKey_None;
    }
}

int inputKeyCodeImGuiToVir(int key)
{
    switch (key)
    {
        case ImGuiKey_Tab: return VIR_KEY_TAB;
        case ImGuiKey_LeftArrow: return VIR_KEY_LEFT;
        case ImGuiKey_RightArrow: return VIR_KEY_RIGHT;
        case ImGuiKey_UpArrow: return VIR_KEY_UP;
        case ImGuiKey_DownArrow: return VIR_KEY_DOWN;
        case ImGuiKey_PageUp: return VIR_KEY_PAGE_UP;
        case ImGuiKey_PageDown: return VIR_KEY_PAGE_DOWN;
        case ImGuiKey_Home: return VIR_KEY_HOME;
        case ImGuiKey_End: return VIR_KEY_END;
        case ImGuiKey_Insert: return VIR_KEY_INSERT;
        case ImGuiKey_Delete: return VIR_KEY_DELETE;
        case ImGuiKey_Backspace: return VIR_KEY_BACKSPACE;
        case ImGuiKey_Space: return VIR_KEY_SPACE;
        case ImGuiKey_Enter: return VIR_KEY_ENTER;
        case ImGuiKey_Escape: return VIR_KEY_ESCAPE;
        case ImGuiKey_Apostrophe: return VIR_KEY_APOSTROPHE;
        case ImGuiKey_Comma: return VIR_KEY_COMMA;
        case ImGuiKey_Minus: return VIR_KEY_MINUS;
        case ImGuiKey_Period: return VIR_KEY_PERIOD;
        case ImGuiKey_Slash: return VIR_KEY_SLASH;
        case ImGuiKey_Semicolon: return VIR_KEY_SEMICOLON;
        case ImGuiKey_Equal: return VIR_KEY_EQUAL;
        case ImGuiKey_LeftBracket: return VIR_KEY_LEFT_BRACKET;
        case ImGuiKey_Backslash: return VIR_KEY_BACKSLASH;
        case ImGuiKey_RightBracket: return VIR_KEY_RIGHT_BRACKET;
        case ImGuiKey_GraveAccent: return VIR_KEY_GRAVE_ACCENT;
        case ImGuiKey_CapsLock: return VIR_KEY_CAPS_LOCK;
        case ImGuiKey_ScrollLock: return VIR_KEY_SCROLL_LOCK;
        case ImGuiKey_NumLock: return VIR_KEY_NUM_LOCK;
        case ImGuiKey_PrintScreen: return VIR_KEY_PRINT_SCREEN;
        case ImGuiKey_Pause: return VIR_KEY_PAUSE;
        case ImGuiKey_Keypad0: return VIR_KEY_KP_0;
        case ImGuiKey_Keypad1: return VIR_KEY_KP_1;
        case ImGuiKey_Keypad2: return VIR_KEY_KP_2;
        case ImGuiKey_Keypad3: return VIR_KEY_KP_3;
        case ImGuiKey_Keypad4: return VIR_KEY_KP_4;
        case ImGuiKey_Keypad5: return VIR_KEY_KP_5;
        case ImGuiKey_Keypad6: return VIR_KEY_KP_6;
        case ImGuiKey_Keypad7: return VIR_KEY_KP_7;
        case ImGuiKey_Keypad8: return VIR_KEY_KP_8;
        case ImGuiKey_Keypad9: return VIR_KEY_KP_9;
        case ImGuiKey_KeypadDecimal: return VIR_KEY_KP_DECIMAL;
        case ImGuiKey_KeypadDivide: return VIR_KEY_KP_DIVIDE;
        case ImGuiKey_KeypadMultiply: return VIR_KEY_KP_MULTIPLY;
        case ImGuiKey_KeypadSubtract: return VIR_KEY_KP_SUBTRACT;
        case ImGuiKey_KeypadAdd: return VIR_KEY_KP_ADD;
        case ImGuiKey_KeypadEnter: return VIR_KEY_KP_ENTER;
        case ImGuiKey_KeypadEqual: return VIR_KEY_KP_EQUAL;
        case ImGuiKey_LeftShift: return VIR_KEY_LEFT_SHIFT;
        case ImGuiKey_LeftCtrl: return VIR_KEY_LEFT_CONTROL;
        case ImGuiKey_LeftAlt: return VIR_KEY_LEFT_ALT;
        case ImGuiKey_LeftSuper: return VIR_KEY_LEFT_SUPER;
        case ImGuiKey_RightShift: return VIR_KEY_RIGHT_SHIFT;
        case ImGuiKey_RightCtrl: return VIR_KEY_RIGHT_CONTROL;
        case ImGuiKey_RightAlt: return VIR_KEY_RIGHT_ALT;
        case ImGuiKey_RightSuper: return VIR_KEY_RIGHT_SUPER;
        case ImGuiKey_Menu: return VIR_KEY_MENU;
        case ImGuiKey_0: return VIR_KEY_0;
        case ImGuiKey_1: return VIR_KEY_1;
        case ImGuiKey_2: return VIR_KEY_2;
        case ImGuiKey_3: return VIR_KEY_3;
        case ImGuiKey_4: return VIR_KEY_4;
        case ImGuiKey_5: return VIR_KEY_5;
        case ImGuiKey_6: return VIR_KEY_6;
        case ImGuiKey_7: return VIR_KEY_7;
        case ImGuiKey_8: return VIR_KEY_8;
        case ImGuiKey_9: return VIR_KEY_9;
        case ImGuiKey_A: return VIR_KEY_A;
        case ImGuiKey_B: return VIR_KEY_B;
        case ImGuiKey_C: return VIR_KEY_C;
        case ImGuiKey_D: return VIR_KEY_D;
        case ImGuiKey_E: return VIR_KEY_E;
        case ImGuiKey_F: return VIR_KEY_F;
        case ImGuiKey_G: return VIR_KEY_G;
        case ImGuiKey_H: return VIR_KEY_H;
        case ImGuiKey_I: return VIR_KEY_I;
        case ImGuiKey_J: return VIR_KEY_J;
        case ImGuiKey_K: return VIR_KEY_K;
        case ImGuiKey_L: return VIR_KEY_L;
        case ImGuiKey_M: return VIR_KEY_M;
        case ImGuiKey_N: return VIR_KEY_N;
        case ImGuiKey_O: return VIR_KEY_O;
        case ImGuiKey_P: return VIR_KEY_P;
        case ImGuiKey_Q: return VIR_KEY_Q;
        case ImGuiKey_R: return VIR_KEY_R;
        case ImGuiKey_S: return VIR_KEY_S;
        case ImGuiKey_T: return VIR_KEY_T;
        case ImGuiKey_U: return VIR_KEY_U;
        case ImGuiKey_V: return VIR_KEY_V;
        case ImGuiKey_W: return VIR_KEY_W;
        case ImGuiKey_X: return VIR_KEY_X;
        case ImGuiKey_Y: return VIR_KEY_Y;
        case ImGuiKey_Z: return VIR_KEY_Z;
        case ImGuiKey_F1: return VIR_KEY_F1;
        case ImGuiKey_F2: return VIR_KEY_F2;
        case ImGuiKey_F3: return VIR_KEY_F3;
        case ImGuiKey_F4: return VIR_KEY_F4;
        case ImGuiKey_F5: return VIR_KEY_F5;
        case ImGuiKey_F6: return VIR_KEY_F6;
        case ImGuiKey_F7: return VIR_KEY_F7;
        case ImGuiKey_F8: return VIR_KEY_F8;
        case ImGuiKey_F9: return VIR_KEY_F9;
        case ImGuiKey_F10: return VIR_KEY_F10;
        case ImGuiKey_F11: return VIR_KEY_F11;
        case ImGuiKey_F12: return VIR_KEY_F12;
        default: return VIR_KEY_UNKNOWN;
    }
}

int inputMouseCodeImGuiToVir(int key)
{
    switch (key)
    {
        case ImGuiKey_MouseLeft: return VIR_MOUSE_BUTTON_LEFT;
        case ImGuiKey_MouseMiddle: return VIR_MOUSE_BUTTON_MIDDLE;
        case ImGuiKey_MouseRight: return VIR_MOUSE_BUTTON_RIGHT;
        default: return VIR_MOUSE_BUTTON_UNKNOWN;
    }
}

// I still need to map some keys as I currently do not have access to a keypad
int inputKeyCodeVirToShaderToy(int key)
{
    switch (key)
    {
        case VIR_KEY_TAB: return 9;
        case VIR_KEY_LEFT: return 37;
        case VIR_KEY_RIGHT: return 39;
        case VIR_KEY_UP: return 38;
        case VIR_KEY_DOWN: return 40;
        case VIR_KEY_PAGE_UP: return -1;
        case VIR_KEY_PAGE_DOWN: return -1;
        case VIR_KEY_HOME: return -1;
        case VIR_KEY_END: return -1;
        case VIR_KEY_INSERT: return -1;
        case VIR_KEY_DELETE: return 46;
        case VIR_KEY_BACKSPACE: return 8;
        case VIR_KEY_SPACE: return 32;
        case VIR_KEY_ENTER: return 13;
        case VIR_KEY_ESCAPE: return 27;
        case VIR_KEY_APOSTROPHE: return 222;
        case VIR_KEY_COMMA: return 188;
        case VIR_KEY_MINUS: return 189;
        case VIR_KEY_PERIOD: return 190;
        case VIR_KEY_SLASH: return 191;
        case VIR_KEY_SEMICOLON: return 186;
        case VIR_KEY_EQUAL: return 187;
        case VIR_KEY_LEFT_BRACKET: return 219;
        case VIR_KEY_BACKSLASH: return 220;
        case VIR_KEY_RIGHT_BRACKET: return 221;
        case VIR_KEY_GRAVE_ACCENT: return 192;
        case VIR_KEY_CAPS_LOCK: return 20;
        case VIR_KEY_SCROLL_LOCK: return -1;
        case VIR_KEY_NUM_LOCK: return -1;
        case VIR_KEY_PRINT_SCREEN: return -1;
        case VIR_KEY_PAUSE: return -1;
        case VIR_KEY_KP_0: return 48;
        case VIR_KEY_KP_1: return 49;
        case VIR_KEY_KP_2: return 50;
        case VIR_KEY_KP_3: return 51;
        case VIR_KEY_KP_4: return 52;
        case VIR_KEY_KP_5: return 53;
        case VIR_KEY_KP_6: return 54;
        case VIR_KEY_KP_7: return 55;
        case VIR_KEY_KP_8: return 56;
        case VIR_KEY_KP_9: return 57;
        case VIR_KEY_KP_DECIMAL: return -1;
        case VIR_KEY_KP_DIVIDE: return -1;
        case VIR_KEY_KP_MULTIPLY: return -1;
        case VIR_KEY_KP_SUBTRACT: return 189;
        case VIR_KEY_KP_ADD: return -1;
        case VIR_KEY_KP_ENTER: return 13;
        case VIR_KEY_KP_EQUAL: return 187;
        case VIR_KEY_LEFT_SHIFT: return 16;
        case VIR_KEY_LEFT_CONTROL: return 17;
        case VIR_KEY_LEFT_ALT: return 18;
        case VIR_KEY_LEFT_SUPER: return 91;
        case VIR_KEY_RIGHT_SHIFT: return 16;
        case VIR_KEY_RIGHT_CONTROL: return 17;
        case VIR_KEY_RIGHT_ALT: return 18;
        case VIR_KEY_RIGHT_SUPER: return -1;
        case VIR_KEY_MENU: return -1;
        case VIR_KEY_0: return 48;
        case VIR_KEY_1: return 49;
        case VIR_KEY_2: return 50;
        case VIR_KEY_3: return 51;
        case VIR_KEY_4: return 52;
        case VIR_KEY_5: return 53;
        case VIR_KEY_6: return 54;
        case VIR_KEY_7: return 55;
        case VIR_KEY_8: return 56;
        case VIR_KEY_9: return 57;
        case VIR_KEY_A: return 65;
        case VIR_KEY_B: return 66;
        case VIR_KEY_C: return 67;
        case VIR_KEY_D: return 68;
        case VIR_KEY_E: return 69;
        case VIR_KEY_F: return 70;
        case VIR_KEY_G: return 71;
        case VIR_KEY_H: return 72;
        case VIR_KEY_I: return 73;
        case VIR_KEY_J: return 74;
        case VIR_KEY_K: return 75;
        case VIR_KEY_L: return 76;
        case VIR_KEY_M: return 77;
        case VIR_KEY_N: return 78;
        case VIR_KEY_O: return 79;
        case VIR_KEY_P: return 80;
        case VIR_KEY_Q: return 81;
        case VIR_KEY_R: return 82;
        case VIR_KEY_S: return 83;
        case VIR_KEY_T: return 84;
        case VIR_KEY_U: return 85;
        case VIR_KEY_V: return 86;
        case VIR_KEY_W: return 87;
        case VIR_KEY_X: return 88;
        case VIR_KEY_Y: return 89;
        case VIR_KEY_Z: return 90;
        case VIR_KEY_F1: return 112;
        case VIR_KEY_F2: return 113;
        case VIR_KEY_F3: return 114;
        case VIR_KEY_F4: return 115;
        case VIR_KEY_F5: return 116;
        case VIR_KEY_F6: return 117;
        case VIR_KEY_F7: return 118;
        case VIR_KEY_F8: return 119;
        case VIR_KEY_F9: return 120;
        case VIR_KEY_F10: return 121;
        case VIR_KEY_F11: return 122;
        case VIR_KEY_F12: return 123;
        default: return -1;
    }
}

}