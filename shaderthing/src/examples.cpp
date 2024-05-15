/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include <string>

#include "shaderthing/include/examples.h"

#include "thirdparty/imgui/imgui.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//

const std::string Examples::rayMarching0 = 
R"({
    "UIScale": 0.6,
    "autoSaveEnabled": true,
    "autoSaveInterval": 60.0,
    "sharedUniforms": {
        "windowResolution": [
            512.0,
            512.0
        ],
        "exportWindowResolutionScale": 1.0,
        "time": 0.0,
        "timePaused": false,
        "timeLooped": false,
        "timeBounds": [
            0.0,
            0.0
        ],
        "randomGeneratorPaused": false,
        "iWASD": [
            2.06640625,
            3.0043227672576904,
            2.998919725418091
        ],
        "iWASDSensitivity": 10.0,
        "iWASDInputEnabled": true,
        "iLook": [
            -0.5431398749351501,
            -0.37137314677238464,
            -0.7530812621116638
        ],
        "iLookSensitivity": 0.20000000298023224,
        "iLookInputEnabled": true,
        "iMouseInputEnabled": true,
        "iKeyboardInputEnabled": true,
        "smoothTimeDelta": false,
        "resetTimeOnFrameCounterReset": true,
        "vSyncEnabled": true,
        "uniforms": {}
    },
    "sharedFragmentSourceSize": 2480,
    "sharedFragmentSource": "// Common source code is shared by all fragment shaders across all layers and\n// has access to all shared in/out/uniform declarations\n\n#define IF_FRAG_X(X) if (int(gl_FragCoord.x)==X)\n#define IF_FRAG_Y(Y) if (int(gl_FragCoord.y)==Y)\n#define IF_FRAG_XY(X,Y) if (int(gl_FragCoord.x)==X && int(gl_FragCoord.y)==Y)\n\n// Keyboard defs for convenience. To access the state of a key, use the ivec3\n// iKeboard[KEY_XXX] uniform, where KEY_XXX is replaced by one of the defs here\n// below. The three components .x, .y, .z are 1 if the key is pressed (but not\n// held), held, toggled respectively, 0 otherwise\n#define KEY_TAB 9\n#define KEY_LEFT 37\n#define KEY_RIGHT 39\n#define KEY_UP 38\n#define KEY_DOWN 40\n#define KEY_DELETE 46\n#define KEY_BACKSPACE 8\n#define KEY_SPACE 32\n#define KEY_ENTER 13\n#define KEY_ESCAPE 27\n#define KEY_APOSTROPHE 222\n#define KEY_COMMA 188\n#define KEY_MINUS 189\n#define KEY_PERIOD 190\n#define KEY_SLASH 191\n#define KEY_SEMICOLON 186\n#define KEY_EQUAL 187\n#define KEY_LEFT_BRACKET 219\n#define KEY_BACKSLASH 220\n#define KEY_RIGHT_BRACKET 221\n#define KEY_GRAVE_ACCENT 192\n#define KEY_CAPS_LOCK 20\n#define KEY_LEFT_SHIFT 16\n#define KEY_LEFT_CONTROL 17\n#define KEY_LEFT_ALT 18\n#define KEY_LEFT_SUPER 91\n#define KEY_RIGHT_SHIFT 16\n#define KEY_RIGHT_CONTROL 17\n#define KEY_RIGHT_ALT 18\n#define KEY_0 48\n#define KEY_1 49\n#define KEY_2 50\n#define KEY_3 51\n#define KEY_4 52\n#define KEY_5 53\n#define KEY_6 54\n#define KEY_7 55\n#define KEY_8 56\n#define KEY_9 57\n#define KEY_A 65\n#define KEY_B 66\n#define KEY_C 67\n#define KEY_D 68\n#define KEY_E 69\n#define KEY_F 70\n#define KEY_G 71\n#define KEY_H 72\n#define KEY_I 73\n#define KEY_J 74\n#define KEY_K 75\n#define KEY_L 76\n#define KEY_M 77\n#define KEY_N 78\n#define KEY_O 79\n#define KEY_P 80\n#define KEY_Q 81\n#define KEY_R 82\n#define KEY_S 83\n#define KEY_T 84\n#define KEY_U 85\n#define KEY_V 86\n#define KEY_W 87\n#define KEY_X 88\n#define KEY_Y 89\n#define KEY_Z 90\n#define KEY_F1 112\n#define KEY_F2 113\n#define KEY_F3 114\n#define KEY_F4 115\n#define KEY_F5 116\n#define KEY_F6 117\n#define KEY_F7 118\n#define KEY_F8 119\n#define KEY_F9 120\n#define KEY_F10 121\n#define KEY_F11 122\n#define KEY_F12 123\n\n// For convenience when importing ShaderToy shaders\n#define SHADERTOY_MAIN void main(){mainImage(fragColor, fragCoord);}\nvec2 fragCoord = gl_FragCoord.xy;\n\n#define CROSSHAIR(color)                           \\\n    if(int(gl_FragCoord.x)==int(iResolution.x/2)|| \\\n       int(gl_FragCoord.y)==int(iResolution.y/2))  \\\n       fragColor.rgb=color;",
    "sharedStorage": {
        "ioIntDataViewStartIndex": 0,
        "ioIntDataViewEndIndex": 7,
        "ioVec4DataViewEndIndex": 7,
        "ioVec4DataViewStartIndex": 0,
        "isVec4DataAlsoShownAsColor": false,
        "ioVec4DataViewPrecision": 3,
        "ioVec4DataViewExponentialFormat": false,
        "ioVec4DataViewComponents": [
            1,
            1,
            1,
            1
        ]
    },
    "layers": {
        "Layer 0": {
            "renderTarget": 0,
            "resolution": [
                512,
                512
            ],
            "resolutionRatio": [
                1.0,
                1.0
            ],
            "isAspectRatioBoundToWindow": true,
            "rescaleWithWindow": true,
            "depth": 0.0,
            "internalFramebuffer": {
                "format": 9,
                "wrapModes": [
                    0,
                    0
                ],
                "magFilterMode": 0,
                "minFilterMode": 1,
                "exportClearPolicy": 0
            },
            "exportData": {
                "resolutionScale": 1.0,
                "rescaleWithOutput": true,
                "windowResolutionScale": 1.0
            },
            "shader": {
                "fragmentSourceSize": 3915,
                "fragmentSource": "// Simple ray marching example by virmodoetiae\n// You can click and drag the mouse on the shader window to look around, and\n// use the WASD keys to move the camera around\n\n#define PI 3.14159\n\n// Increase this to avoid visual artifacts. However, at the same time, you\n// should increase the number of ray marching steps\n#define SAFETY_FACTOR 2\n\n// Max number of ray marching steps\n#define MAX_STEPS 250 \n\n// Minimum distance from any scene surface below which ray marching stops\n#define MIN_DIST 1e-4\n\n// Maximum distance from any scene surface above which ray marching stops\n#define MAX_DIST 1e3\n\n// Given a point 'p', this function returns the distance between 'p' and the\n// nearest scene surface. It is this function that describes the 3D geometry\n// of the scene\nfloat sceneSDF(vec3 p)\n{\n    // Sphere SDF\n    float wobblySphere = length(p-vec3(0,1.5,0))-.75;\n    \n    // Make it wobbly\n    wobblySphere += .25*sin(5*p.x*p.y+iTime*2*PI);\n    \n    // Ground plane SDF\n    float ground = p.y; \n    \n    // The min operator is equivalent to a geometric union of the surfaces\n    // represented by the two SDFs\n    return min(wobblySphere, ground);\n}\n\n// Given a point 'p' close to the 3D geometry scene surface, this function\n// returns the surface normal. The 'h' factor is the step used for the\n// derivative calculation. If too low, visual artifacts will result, if too\n// large, the resulting geometry will loose detail\nvec3 sceneNormal(vec3 p)\n{\n    float h = 10*MIN_DIST;\n    vec2 e = vec2(1.0,-1.0);\n    return normalize(e.xyy*sceneSDF(p+e.xyy*h)+\n                     e.yyx*sceneSDF(p+e.yyx*h)+\n                     e.yxy*sceneSDF(p+e.yxy*h)+\n                     e.xxx*sceneSDF(p+e.xxx*h));\n}\n\nstruct Ray\n{\n    vec3 origin;\n    vec3 direction;\n};\n\nRay cameraRay(vec2 uv, vec3 cp, vec3 f, float fov)\n{\n    vec3 l = normalize(cross(vec3(0,1,0),f));\n    vec3 u = normalize(cross(f,l));\n    return Ray(cp,normalize(cp+f*fov-uv.x*l+uv.y*u-cp));\n}\n\n// Propagate a ray 'r' along its direction until either a scene surface is \n// hit or the maximum number of steps has been traversed. Returs the distance\n// to said hit point, otherwise returns 0\nfloat rayMarch(Ray r)\n{\n    float s = 0, ds = 0;\n    for (int step=0; step<MAX_STEPS; step++)\n    {\n        ds = sceneSDF(r.origin+r.direction*s)/SAFETY_FACTOR;\n        s += ds;\n        if (ds < MIN_DIST && ds > 0)\n            break;\n    }\n    return s > MAX_DIST ? 0 : s;\n}\n\nvoid main()\n{\n    // Define light source position and background color (used when no surface\n    // hit or for shadrows\n    const vec3 lightSource = vec3(2,4,2);\n    const vec4 bckgColor = vec4(0,0,0,1);\n\n    // Feel free to replace these with the iWASD, iLook\n    // uniforms to have live control over the camera and move in shader-space!\n    vec3 cameraPosition = iWASD;\n    vec3 cameraDirection = iLook;\n    \n    // Construct ray for this pixel and ray march\n    Ray r = cameraRay(qc,cameraPosition,cameraDirection,1);\n    float d = rayMarch(r);\n    if (d == 0) // If no surface hit\n        fragColor = bckgColor; //set to background color\n    else        // If surface hit\n    {\n        // Find surface hit point and direction from p to light source\n        vec3 p = r.origin+r.direction*d*.99;\n        vec3 pl = lightSource-p;\n\n        // Ray march from the hit point to the light source\n        Ray rl = Ray(p, normalize(pl));\n        float dl = rayMarch(rl);\n\n        // If ray marching does not reach light source it means that the point\n        // is in complete shade (hard shadow)\n        if (dl>0 && dl<.99*length(pl))\n            fragColor = bckgColor;\n        else \n        {\n            // Compute pixel color if fully or partially illuminated in the\n            // simples way, i.e. according to Lambertian cosine law\n            vec3 n = sceneNormal(p); \n            float c = max(dot(n,normalize(lightSource-p)),0);\n            fragColor = vec4(c,c,c,1);\n        }\n    }\n}",
                "uniforms": {}
            }
        }
    },
    "resources": {},
    "exporter": {
        "type": 0,
        "outputFilepath": "",
        "nRenderPasses": 1,
        "areRenderPassesOnFirstFrameOnly": false,
        "resetFrameCounterAfterExport": true,
        "startTime": 0.0,
        "endTime": 1.0,
        "fps": 60.0,
        "gifPaletteMode": 0,
        "gifPaletteBitDepth": 8,
        "gifAlphaCutoff": 0,
        "gifDitherMode": 0
    },
    "sharedPostProcessData": {}
})";

//----------------------------------------------------------------------------//

void Examples::renderGui(const std::string*& selection)
{
    bool loadExampleConfirmation = false;
    static const std::string* tmpSelection = nullptr;
    selection = nullptr;
    ImGui::Text
    (
R"(This is a collection of built-in project examples which can be freely loaded,
edited and used for learning purposes or as starting points for other projects)"
    );
    ImGui::Separator();
    if (ImGui::Button("Load"))
    {
        loadExampleConfirmation = true;
        tmpSelection = &Examples::rayMarching0;
    }
    ImGui::SameLine();
    ImGui::Text("Ray marcher I");

    if (loadExampleConfirmation)
        ImGui::OpenPopup("Load example project confirmation");
    if 
    (
        ImGui::BeginPopupModal
        (
            "Load example project confirmation", 
            nullptr, 
            ImGuiWindowFlags_NoResize
        )
    )
    {
        ImGui::Text("Are you sure you want to load the selected project?");
        ImGui::Text("Any unsaved edits to the current project will be lost!");
        if (ImGui::Button("Confirm"))
        {
            selection = tmpSelection;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            tmpSelection = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}