#ifndef V_INPUT_CODES_H
#define V_INPUT_CODES_H

#include <string>
#include <unordered_map>

namespace vir
{

// All of the key codes are taken from GLFW for simplicity

// Keyboard keys -------------------------------------------------------------//

#define VIR_KEY_UNKNOWN            -1

// Printable keys
#define VIR_KEY_SPACE              32
#define VIR_KEY_APOSTROPHE         39  /* ' */
#define VIR_KEY_COMMA              44  /* , */
#define VIR_KEY_MINUS              45  /* - */
#define VIR_KEY_PERIOD             46  /* . */
#define VIR_KEY_SLASH              47  /* / */
#define VIR_KEY_0                  48
#define VIR_KEY_1                  49
#define VIR_KEY_2                  50
#define VIR_KEY_3                  51
#define VIR_KEY_4                  52
#define VIR_KEY_5                  53
#define VIR_KEY_6                  54
#define VIR_KEY_7                  55
#define VIR_KEY_8                  56
#define VIR_KEY_9                  57
#define VIR_KEY_SEMICOLON          59  /* ; */
#define VIR_KEY_EQUAL              61  /* = */
#define VIR_KEY_A                  65
#define VIR_KEY_B                  66
#define VIR_KEY_C                  67
#define VIR_KEY_D                  68
#define VIR_KEY_E                  69
#define VIR_KEY_F                  70
#define VIR_KEY_G                  71
#define VIR_KEY_H                  72
#define VIR_KEY_I                  73
#define VIR_KEY_J                  74
#define VIR_KEY_K                  75
#define VIR_KEY_L                  76
#define VIR_KEY_M                  77
#define VIR_KEY_N                  78
#define VIR_KEY_O                  79
#define VIR_KEY_P                  80
#define VIR_KEY_Q                  81
#define VIR_KEY_R                  82
#define VIR_KEY_S                  83
#define VIR_KEY_T                  84
#define VIR_KEY_U                  85
#define VIR_KEY_V                  86
#define VIR_KEY_W                  87
#define VIR_KEY_X                  88
#define VIR_KEY_Y                  89
#define VIR_KEY_Z                  90
#define VIR_KEY_LEFT_BRACKET       91  /* [ */
#define VIR_KEY_BACKSLASH          92  /* \ */
#define VIR_KEY_RIGHT_BRACKET      93  /* ] */
#define VIR_KEY_GRAVE_ACCENT       96  /* ` */
#define VIR_KEY_WORLD_1            161 /* non-US #1 */
#define VIR_KEY_WORLD_2            162 /* non-US #2 */

// Function keys
#define VIR_KEY_ESCAPE             256
#define VIR_KEY_ENTER              257
#define VIR_KEY_TAB                258
#define VIR_KEY_BACKSPACE          259
#define VIR_KEY_INSERT             260
#define VIR_KEY_DELETE             261
#define VIR_KEY_RIGHT              262
#define VIR_KEY_LEFT               263
#define VIR_KEY_DOWN               264
#define VIR_KEY_UP                 265
#define VIR_KEY_PAGE_UP            266
#define VIR_KEY_PAGE_DOWN          267
#define VIR_KEY_HOME               268
#define VIR_KEY_END                269
#define VIR_KEY_CAPS_LOCK          280
#define VIR_KEY_SCROLL_LOCK        281
#define VIR_KEY_NUM_LOCK           282
#define VIR_KEY_PRINT_SCREEN       283
#define VIR_KEY_PAUSE              284
#define VIR_KEY_F1                 290
#define VIR_KEY_F2                 291
#define VIR_KEY_F3                 292
#define VIR_KEY_F4                 293
#define VIR_KEY_F5                 294
#define VIR_KEY_F6                 295
#define VIR_KEY_F7                 296
#define VIR_KEY_F8                 297
#define VIR_KEY_F9                 298
#define VIR_KEY_F10                299
#define VIR_KEY_F11                300
#define VIR_KEY_F12                301
#define VIR_KEY_F13                302
#define VIR_KEY_F14                303
#define VIR_KEY_F15                304
#define VIR_KEY_F16                305
#define VIR_KEY_F17                306
#define VIR_KEY_F18                307
#define VIR_KEY_F19                308
#define VIR_KEY_F20                309
#define VIR_KEY_F21                310
#define VIR_KEY_F22                311
#define VIR_KEY_F23                312
#define VIR_KEY_F24                313
#define VIR_KEY_F25                314
#define VIR_KEY_KP_0               320
#define VIR_KEY_KP_1               321
#define VIR_KEY_KP_2               322
#define VIR_KEY_KP_3               323
#define VIR_KEY_KP_4               324
#define VIR_KEY_KP_5               325
#define VIR_KEY_KP_6               326
#define VIR_KEY_KP_7               327
#define VIR_KEY_KP_8               328
#define VIR_KEY_KP_9               329
#define VIR_KEY_KP_DECIMAL         330
#define VIR_KEY_KP_DIVIDE          331
#define VIR_KEY_KP_MULTIPLY        332
#define VIR_KEY_KP_SUBTRACT        333
#define VIR_KEY_KP_ADD             334
#define VIR_KEY_KP_ENTER           335
#define VIR_KEY_KP_EQUAL           336
#define VIR_KEY_LEFT_SHIFT         340
#define VIR_KEY_LEFT_CONTROL       341
#define VIR_KEY_LEFT_ALT           342
#define VIR_KEY_LEFT_SUPER         343
#define VIR_KEY_RIGHT_SHIFT        344
#define VIR_KEY_RIGHT_CONTROL      345
#define VIR_KEY_RIGHT_ALT          346
#define VIR_KEY_RIGHT_SUPER        347
#define VIR_KEY_MENU               348

#define VIR_N_KEYS                 349

// Modifiers
//#define VIR_MOD_SHIFT           0x0001
/*! @brief If this bit is set one or more Control keys were held down.
 *
 *  If this bit is set one or more Control keys were held down.
 */
//#define VIR_MOD_CONTROL         0x0002
/*! @brief If this bit is set one or more Alt keys were held down.
 *
 *  If this bit is set one or more Alt keys were held down.
 */
//#define VIR_MOD_ALT             0x0004
/*! @brief If this bit is set one or more Super keys were held down.
 *
 *  If this bit is set one or more Super keys were held down.
 */
//#define VIR_MOD_SUPER           0x0008
/*! @brief If this bit is set the Caps Lock key is enabled.
 *
 *  If this bit is set the Caps Lock key is enabled and the @ref
 *  VIR_LOCK_KEY_MODS input mode is set.
 */
//#define VIR_MOD_CAPS_LOCK       0x0010
/*! @brief If this bit is set the Num Lock key is enabled.
 *
 *  If this bit is set the Num Lock key is enabled and the @ref
 *  VIR_LOCK_KEY_MODS input mode is set.
 */
//#define VIR_MOD_NUM_LOCK        0x0020

//#define VIR_N_MOD_KEYS          0x0021

// Mouse button codes --------------------------------------------------------//

#define VIR_MOUSE_BUTTON_1         0
#define VIR_MOUSE_BUTTON_2         1
#define VIR_MOUSE_BUTTON_3         2
#define VIR_MOUSE_BUTTON_4         3
#define VIR_MOUSE_BUTTON_5         4
#define VIR_MOUSE_BUTTON_6         5
#define VIR_MOUSE_BUTTON_7         6
#define VIR_MOUSE_BUTTON_8         7
#define VIR_MOUSE_BUTTON_LAST      VIR_MOUSE_BUTTON_8
#define VIR_MOUSE_BUTTON_LEFT      VIR_MOUSE_BUTTON_1
#define VIR_MOUSE_BUTTON_RIGHT     VIR_MOUSE_BUTTON_2
#define VIR_MOUSE_BUTTON_MIDDLE    VIR_MOUSE_BUTTON_3

#define VIR_N_MOUSE_BUTTONS        8

// Buttons names -------------------------------------------------------------//

extern std::unordered_map<int, std::string> keyCodeToName;

// Mapping functions for different input libraries ---------------------------//

// Mapping from a vir input code to a glfw input code
int inputKeyCodeVirToGlfw(int);
int inputMouseCodeVirToGlfw(int);
// Mapping from a glfw input code to a vir input code
int inputKeyCodeGlfwToVir(int);
int inputMouseCodeGlfwToVir(int);

}

#endif