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

const std::string Examples::pathMarcher = 
R"({
    "UIScale": 0.6000000238418579,
    "autoSaveEnabled": true,
    "autoSaveInterval": 60.0,
    "sharedUniforms": {
        "windowResolution": [
            512.0,
            512.0
        ],
        "exportWindowResolutionScale": 1.0,
        "time": 4.894183158874512,
        "timePaused": false,
        "timeLooped": true,
        "timeBounds": [
            0.0,
            6.2829999923706055
        ],
        "randomGeneratorPaused": false,
        "iWASD": [
            -2.2941999435424805,
            2.509347438812256,
            -2.7872254848480225
        ],
        "iWASDSensitivity": 2.2890000343322754,
        "iWASDInputEnabled": true,
        "iLook": [
            0.5316748023033142,
            -0.4373071789741516,
            0.7253755331039429
        ],
        "iLookSensitivity": 0.11523546278476715,
        "iLookInputEnabled": true,
        "iMouseInputEnabled": true,
        "iKeyboardInputEnabled": true,
        "smoothTimeDelta": false,
        "resetTimeOnFrameCounterReset": false,
        "vSyncEnabled": true,
        "uniforms": {}
    },
    "sharedFragmentSourceSize": 3139,
    "sharedFragmentSource": "// Common source code is shared by all fragment shaders across all layers and\n// has access to all shared in/out/uniform declarations\n\n#define IF_FRAG_X(X) if (int(gl_FragCoord.x)==X)\n#define IF_FRAG_Y(Y) if (int(gl_FragCoord.y)==Y)\n#define IF_FRAG_XY(X,Y) if (int(gl_FragCoord.x)==X && int(gl_FragCoord.y)==Y)\n\n// Keyboard defs for convenience. To access the state of a key, use the ivec3\n// iKeboard[KEY_XXX] uniform, where KEY_XXX is replaced by one of the defs here\n// below. The three components .x, .y, .z are 1 if the key is pressed (but not\n// held), held, toggled respectively, 0 otherwise\n#define KEY_TAB 9\n#define KEY_LEFT 37\n#define KEY_RIGHT 39\n#define KEY_UP 38\n#define KEY_DOWN 40\n#define KEY_DELETE 46\n#define KEY_BACKSPACE 8\n#define KEY_SPACE 32\n#define KEY_ENTER 13\n#define KEY_ESCAPE 27\n#define KEY_APOSTROPHE 222\n#define KEY_COMMA 188\n#define KEY_MINUS 189\n#define KEY_PERIOD 190\n#define KEY_SLASH 191\n#define KEY_SEMICOLON 186\n#define KEY_EQUAL 187\n#define KEY_LEFT_BRACKET 219\n#define KEY_BACKSLASH 220\n#define KEY_RIGHT_BRACKET 221\n#define KEY_GRAVE_ACCENT 192\n#define KEY_CAPS_LOCK 20\n#define KEY_LEFT_SHIFT 16\n#define KEY_LEFT_CONTROL 17\n#define KEY_LEFT_ALT 18\n#define KEY_LEFT_SUPER 91\n#define KEY_RIGHT_SHIFT 16\n#define KEY_RIGHT_CONTROL 17\n#define KEY_RIGHT_ALT 18\n#define KEY_0 48\n#define KEY_1 49\n#define KEY_2 50\n#define KEY_3 51\n#define KEY_4 52\n#define KEY_5 53\n#define KEY_6 54\n#define KEY_7 55\n#define KEY_8 56\n#define KEY_9 57\n#define KEY_A 65\n#define KEY_B 66\n#define KEY_C 67\n#define KEY_D 68\n#define KEY_E 69\n#define KEY_F 70\n#define KEY_G 71\n#define KEY_H 72\n#define KEY_I 73\n#define KEY_J 74\n#define KEY_K 75\n#define KEY_L 76\n#define KEY_M 77\n#define KEY_N 78\n#define KEY_O 79\n#define KEY_P 80\n#define KEY_Q 81\n#define KEY_R 82\n#define KEY_S 83\n#define KEY_T 84\n#define KEY_U 85\n#define KEY_V 86\n#define KEY_W 87\n#define KEY_X 88\n#define KEY_Y 89\n#define KEY_Z 90\n#define KEY_F1 112\n#define KEY_F2 113\n#define KEY_F3 114\n#define KEY_F4 115\n#define KEY_F5 116\n#define KEY_F6 117\n#define KEY_F7 118\n#define KEY_F8 119\n#define KEY_F9 120\n#define KEY_F10 121\n#define KEY_F11 122\n#define KEY_F12 123\n\n#define CROSSHAIR if (int(fragCoord.x) == int(iResolution.x/2) || int(fragCoord.y) == int(iResolution.y/2)){frag_color = vec4(1,0,0,1);}\n\n// For convenience when importing ShaderToy shaders\n#define SHADERTOY_MAIN void main(){mainImage(fragColor, fragCoord);}\nvec2 fragCoord = gl_FragCoord.xy; \n\nfloat uniteSDFs(float sdf1, float sdf2)\n{\n    return min(sdf1, sdf2);\n}\n\nfloat subtractSDFs(float sdf1, float sdf2)\n{\n    return max(-sdf1, sdf2);\n}\n\nfloat intersectSDFs(float sdf1, float sdf2)\n{\n    return max(sdf1, sdf2);\n}\n\nfloat smoothUniteSDFs(float sdf1, float sdf2, float s)\n{\n    float h = clamp(0.5+0.5*(sdf2-sdf1)/s, 0.0, 1.0);\n    return mix(sdf2, sdf1, h)-s*h*(1.0-h);\n}\n\nfloat smoothSubtractSDFs(float sdf1, float sdf2, float s)\n{\n    float h = clamp(0.5-0.5*(sdf2+sdf1)/s, 0.0, 1.0);\n    return mix(sdf2, -sdf1, h) + s*h*(1.0-h);\n}\n\nfloat smoothIntersectSDFs(float sdf1, float sdf2, float s)\n{\n    float h = clamp(0.5-0.5*(sdf2-sdf1)/s, 0.0, 1.0);\n    return mix(sdf2, sdf1, h)+s*h*(1.0-h);\n}",
    "sharedStorage": {
        "ioIntDataViewStartIndex": 0,
        "ioIntDataViewEndIndex": 7,
        "ioVec4DataViewEndIndex": 50,
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
            "renderTarget": 1,
            "resolution": [
                256,
                256
            ],
            "resolutionRatio": [
                0.5,
                0.5
            ],
            "isAspectRatioBoundToWindow": false,
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
                "exportClearPolicy": 2
            },
            "exportData": {
                "resolutionScale": 1.0,
                "rescaleWithOutput": false,
                "windowResolutionScale": 1.0
            },
            "shader": {
                "fragmentSourceSize": 8693,
                "fragmentSource": "precision mediump float;\n\n//----------------------------------------------------------------------------//\n\n#define ANTI_ALIASING true\n\n// Number of rays propagated per pixel\n#define N_SAMPLES 6\n\n// Maximum number of bounces per ray\n#define N_BOUNCES 6\n\n// Increase to avoid visual artifacts if SDFs get excessively non-Euclidean\n#define SAFETY_FACTOR 2.\n\n// Max number of ray marching steps\n#define MAX_STEPS 400 \n\n// Minimum distance from any scene surface below which ray marching stops\n#define MIN_DIST 1e-4\n\n// Maximum distance from any scene surface beyond which ray marching stops\n#define MAX_DIST 1000.\n\n// Useful quantities\n#define PI 3.14159\n#define UP vec3(0,1,0)\n#define RT vec3(-1,0,0)\n#define FW vec3(0,0,1)\n\n//----------------------------------------------------------------------------//\n\n// Pseudo-random related\n// https://www.shadertoy.com/view/Xt3cDn\n\nuint baseHash(uvec2 p) \n{\n    p = 1103515245U*((p >> 1U)^(p.yx));\n    uint h32 = 1103515245U*((p.x)^(p.y>>3U));\n    return h32^(h32 >> 16);\n}\n\nfloat gSeed = \n    float(baseHash(floatBitsToUint(fragCoord)))/float(0xffffffffU)+iRandom;\n\nvec2 hash2() \n{\n    uint n = baseHash(floatBitsToUint(vec2(gSeed+=.1,gSeed+=.1)));\n    uvec2 rz = uvec2(n, n*48271U);\n    return vec2(rz.xy & uvec2(0x7fffffffU))/float(0x7fffffff);\n}\n\nvec3 hash3() \n{\n    uint n = baseHash(floatBitsToUint(vec2(gSeed+=.1, gSeed+=.1)));\n    uvec3 rz = uvec3(n, n*16807U, n*48271U);\n    return vec3(rz & uvec3(0x7fffffffU))/float(0x7fffffff);\n}\n\nvec3 randomUnitVector()\n{\n    vec3 h = hash3() * vec3(2.,6.283185,1.)-vec3(1,0,0);\n    float phi = h.y;\n    float r = pow(h.z, 1./3.);\n    return r*vec3(sqrt(1.-h.x*h.x)*vec2(sin(phi),cos(phi)),h.x);\n}\n\n//----------------------------------------------------------------------------//\n\n// Structs & struct helpers\n\nstruct Material\n{\n    vec3  color;\n    float metallicity;\n    float roughness;\n};\n\nMaterial Lambertian(vec3 color)\n{\n    return Material(color, 0., 0.);\n}\n\nMaterial Metal(vec3 color, float roughness)\n{\n    return Material(color, 1., roughness);\n}\n\nMaterial dummyMaterial; // Used for subtractions\n\nstruct SDM // Signed distance & material\n{\n    float sd;\n    Material m;\n};\n\nSDM uniteSDMs(SDM sdm1, SDM sdm2)\n{\n    if (sdm1.sd < sdm2.sd)\n        return sdm1;\n    return sdm2;\n}\n\nSDM subtractSDMs(SDM sdm1, SDM sdm2)\n{\n    return SDM(max(-sdm1.sd, sdm2.sd), sdm2.m);\n}\n\nSDM intersectSDMs(SDM sdm1, SDM sdm2)\n{\n    return\n        SDM\n        (\n            max(sdm1.sd, sdm2.sd), \n            Material\n            (\n                (sdm1.m.color+sdm2.m.color)/2.,\n                (sdm1.m.metallicity+sdm2.m.metallicity)/2.,\n                (sdm1.m.roughness+sdm2.m.roughness)/2.\n            )\n        );\n}\n\nSDM smoothUniteSDMs(SDM sdm1, SDM sdm2, float s)\n{\n    float h = clamp(0.5+0.5*(sdm2.sd-sdm1.sd)/s, 0.0, 1.0);\n    return \n        SDM\n        (\n            mix(sdm2.sd, sdm1.sd, h)-s*h*(1.0-h),\n            Material\n            (\n                mix(sdm2.m.color, sdm1.m.color, h),\n                mix(sdm2.m.metallicity, sdm1.m.metallicity, h),\n                mix(sdm2.m.roughness, sdm1.m.roughness, h)\n            )\n        );\n}\n\nSDM smoothSubtractSDMs(SDM sdm1, SDM sdm2, float s)\n{\n    float h = clamp(0.5-0.5*(sdm2.sd+sdm1.sd)/s, 0.0, 1.0);\n    return SDM(mix(sdm2.sd, -sdm1.sd, h) + s*h*(1.0-h), sdm2.m);\n}\n\nSDM smoothIntersectSDMs(SDM sdm1, SDM sdm2, float s)\n{\n    float h = clamp(0.5-0.5*(sdm2.sd-sdm1.sd)/s, 0.0, 1.0);\n    return\n        SDM\n        (\n            mix(sdm2.sd, sdm1.sd, h)+s*h*(1.0-h),\n            Material\n            (\n                mix(sdm2.m.color, sdm1.m.color, h),\n                mix(sdm2.m.metallicity, sdm1.m.metallicity, h),\n                mix(sdm2.m.roughness, sdm1.m.roughness, h)\n            )\n        );\n}\n\nstruct Ray\n{\n    vec3 origin;\n    vec3 direction;\n};\n\nRay cameraRay(vec2 uv, vec3 cp, vec3 f, float fov)\n{\n    if (ANTI_ALIASING)\n        uv += ((hash2()-.5)/iResolution)/2.;\n    vec3 l = normalize(cross(vec3(0,1,0),f));\n    vec3 u = normalize(cross(f,l));\n    return Ray(cp,normalize(cp+f*fov-uv.x*l+uv.y*u-cp));\n}\n\n//----------------------------------------------------------------------------//\n\nvec3 rotate(vec3 v, float t, vec3 a)\n{\n    vec4 q = vec4(sin(t/2.0)*a.xyz, cos(t/2.0));\n    return v + 2.0*cross(q.xyz, cross(q.xyz, v) + q.w*v);\n}\n\nfloat rectCuboidSDF(vec3 p, vec3 l)\n{\n    vec3 q = abs(p)-l/2.;\n    return length(max(q,0.0))+min(max(q.x,max(q.y,q.z)),0.0);\n}\n\nfloat infCylinderSDF(vec3 p, float r)\n{\n    return length(p.yz)-r;\n}\n\nfloat cylinderSDF(vec3 p, float r, float h)\n{\n    return max(max(length(p.yz)-r, p.x-h/2.), -p.x-h/2.);\n}\n\n//----------------------------------------------------------------------------//\n\nSDM sceneSDM(vec3 p)\n{\n    vec3 basel = vec3(.5,.125, 1.);\n    vec3 pilrl = vec3(.25, 1., .25);\n    vec3 rotrl = vec3(.05,.15, 1);\n    SDM item = \n        SDM\n        (\n            rectCuboidSDF(p-UP*basel/2.+FW*basel.z/8, basel)-.01,\n            Lambertian(vec3(.75))\n        );\n    \n    item = uniteSDMs // Pillar\n    (\n        item, \n        SDM\n        (\n            rectCuboidSDF\n            (\n                p-UP*(basel+pilrl/2.)+RT*(basel-pilrl)/2., \n                pilrl\n            )-.01,\n            Lambertian(vec3(.75))\n        )\n    );\n    vec3 p2 = p-UP*(basel+pilrl/2.);\n    item = smoothSubtractSDMs // Hole in pillar\n    (\n        SDM(infCylinderSDF(p2, .075), dummyMaterial),\n        item,\n        .01\n    );\n    SDM item2 = SDM(cylinderSDF(p2, .05, .65), Lambertian(vec3(.75)));\n    //item2 -= .025*sin(-10*p2.x-iTime)-.025;\n    item2 = uniteSDMs\n    (\n        item2,\n        SDM\n        (\n            rectCuboidSDF\n            (\n                rotate\n                (\n                    p2-RT*.25, \n                    (.5*sin(iTime)+.5)*PI/2, \n                    RT\n                )+FW*(.3+.2*sin(iTime)-.1),\n                rotrl*vec3(1,1,.6+.4*sin(iTime))\n            )-.01,\n            Lambertian(vec3(.75))\n        )\n    );\n    \n    item = uniteSDMs(item, item2);\n    item.sd -= .01*sin(10*p.x-4*p.y)+.01*cos(20*p.x*p.z+10*iTime);\n    //item += .01*sin(4*p.x*p.y+iTime*2*PI);\n    \n    // Walls\n    vec3 b = vec3(5,-.25,5);\n    p -= vec3(wallShift.x, 0, wallShift.y);\n    float walls = \n    max\n    (\n        min(min(-p.x+b.x, -p.z+b.z), p.y-b.t), \n        length(p-b)-length(b)\n    );\n    return uniteSDMs(SDM(walls, Material(vec3(.75), mtl, rgh)), item);\n}\n\nvec3 sceneNormal(vec3 p, float h)\n{\n    vec2 e = vec2(1.0,-1.0);\n    return normalize(e.xyy*sceneSDM(p+e.xyy*h).sd+\n                     e.yyx*sceneSDM(p+e.yyx*h).sd+\n                     e.yxy*sceneSDM(p+e.yxy*h).sd+\n                     e.xxx*sceneSDM(p+e.xxx*h).sd);\n}\n\n//----------------------------------------------------------------------------//\n\nSDM rayMarch(Ray r)\n{\n    // FOUND THE FUCKING ISSUE WHEN RUNNING THIS RAYMARCH IN MULTIPLE LOOPS,\n    // IT WAS DUE TO S BEING UNINITIALIZED INSTEAD OF BEING SET TO FUCKING 0!\n    float s = 0; \n    float ds = 0;\n    SDM sdm;\n    for (int q=0; q<MAX_STEPS; q++)\n    {\n        vec3 p = r.origin+r.direction*s;\n        sdm = sceneSDM(p);\n        ds = sdm.sd/SAFETY_FACTOR;\n        s += ds;\n        if ((ds < MIN_DIST && ds > 0) || s > MAX_DIST)\n            break;\n    }\n    sdm.sd = s*float(s <= MAX_DIST);\n    return sdm;\n}\n\n//----------------------------------------------------------------------------//\n\nvoid scatter(inout Ray ray, SDM sdm, out vec3 n)\n{\n    ray.origin += sdm.sd*ray.direction*(1.-MIN_DIST);\n    n = sceneNormal(ray.origin, MIN_DIST);\n    ray.origin += n*10*MIN_DIST;\n    ray.direction =\n        normalize\n        (\n            (1.-sdm.m.metallicity)*(n+randomUnitVector()) +\n            (sdm.m.metallicity)*\n            (\n                ray.direction-\n                2.*dot(n, ray.direction)*\n                (n-sdm.m.roughness*randomUnitVector()/2.)\n            )\n        );\n}\n\n//----------------------------------------------------------------------------//\n\nvec3 computeColor(in Ray ray)\n{\n    vec3 col = vec3(1);\n    for (int i=0; i<N_BOUNCES; i++)\n    {\n        SDM sdm = rayMarch(ray);\n        if (sdm.sd > 0.)\n        {\n            vec3 n;\n            scatter(ray, sdm, n);\n            col *= (.5+.25*dot(n, vec3(0,1,0)));\n        }\n        else\n            return col*mix(vec3(1.), vec3(.5,.7,1.), .5*ray.direction.y+.5);\n    }\n    return col;\n}\n\n//----------------------------------------------------------------------------//\n\nvoid main() \n{   \n    Ray ray;\n    fragColor = vec4(0);\n    for (int i=0; i<N_SAMPLES; i++)\n    {\n        ray = cameraRay(qc, iWASD, iLook, 1);\n        fragColor += vec4(computeColor(ray),1)/N_SAMPLES;\n    }\n    //if (!iUserAction)\n    //    fragColor += texelFetch(iChannel0, ivec2(fragCoord), 0);\n}",
                "uniforms": {
                    "iChannel0": {
                        "type": "texture2D",
                        "shared": false,
                        "value": "Layer 0"
                    },
                    "wallShift": {
                        "type": "vec2",
                        "shared": false,
                        "value": [
                            -4.4894585609436035,
                            -1.9621250629425049
                        ],
                        "min": -5.0,
                        "max": 5.0,
                        "dragStep": 1.0
                    },
                    "mtl": {
                        "type": "float",
                        "shared": false,
                        "value": 1.0,
                        "min": 0.0,
                        "max": 1.0
                    },
                    "rgh": {
                        "type": "float",
                        "shared": false,
                        "value": 0.15000000596046448,
                        "min": 0.0,
                        "max": 1.0
                    }
                }
            }
        },
        "Layer 1": {
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
            "depth": 0.03125,
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
                "fragmentSourceSize": 110,
                "fragmentSource": "void main()\n{\n    fragColor = texture(iChannel0, tc);\n    fragColor.rgb /= fragColor.a;\n    fragColor.a = 1;\n}",
                "uniforms": {
                    "iChannel0": {
                        "type": "texture2D",
                        "shared": false,
                        "value": "Layer 0"
                    }
                }
            }
        }
    },
    "resources": {},
    "exporter": {
        "type": 1,
        "outputFilepath": "Z:\\shared\\programming\\C++\\shaderthing\\private\\v1.0.0\\rm512_10s32rp_av.gif",
        "nRenderPasses": 32,
        "areRenderPassesOnFirstFrameOnly": false,
        "resetFrameCounterAfterExport": true,
        "startTime": 0.0,
        "endTime": 1.0,
        "fps": 50.0,
        "gifPaletteMode": 2,
        "gifPaletteBitDepth": 3,
        "gifAlphaCutoff": 0,
        "gifDitherMode": 0
    },
    "sharedPostProcessData": {}
})";

//----------------------------------------------------------------------------//

void Examples::renderGui(const std::string*& selection)
{
    selection = nullptr;
    if (ImGui::Button("Load"))
        selection = &Examples::pathMarcher;
    ImGui::SameLine();
    ImGui::Text("Ray-marching-based path tracer ");
}

}