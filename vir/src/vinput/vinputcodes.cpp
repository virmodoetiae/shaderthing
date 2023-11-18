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

int inputKeyCodeVirToGlfw(int k)
{
    return k;
}

int inputMouseCodeVirToGlfw(int k)
{
    return k;
}

int inputKeyCodeGlfwToVir(int k)
{
    return k;
}

int inputMouseCodeGlfwToVir(int k)
{
    return k;
}

}