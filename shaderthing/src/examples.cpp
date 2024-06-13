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
                "fragmentSource": "// Simple ray marching example by virmodoetiae\n// You can click and drag the mouse on the shader window to look around, and\n// use the WASD keys to move the camera around\n\n#define PI 3.14159\n\n// Increase this to avoid visual artifacts. However, at the same time, you\n// should increase the number of ray marching steps\n#define SAFETY_FACTOR 3\n\n// Max number of ray marching steps\n#define MAX_STEPS 250 \n\n// Minimum distance from any scene surface below which ray marching stops\n#define MIN_DIST 1e-4\n\n// Maximum distance from any scene surface above which ray marching stops\n#define MAX_DIST 1e3\n\n// Given a point 'p', this function returns the distance between 'p' and the\n// nearest scene surface. It is this function that describes the 3D geometry\n// of the scene\nfloat sceneSDF(vec3 p)\n{\n    // Sphere SDF\n    float wobblySphere = length(p-vec3(0,1.5,0))-.75;\n    \n    // Make it wobbly\n    wobblySphere += .25*sin(5*p.x*p.y+iTime*2*PI);\n    \n    // Ground plane SDF\n    float ground = p.y; \n    \n    // The min operator is equivalent to a geometric union of the surfaces\n    // represented by the two SDFs\n    return min(wobblySphere, ground);\n}\n\n// Given a point 'p' close to the 3D geometry scene surface, this function\n// returns the surface normal. The 'h' factor is the step used for the\n// derivative calculation. If too low, visual artifacts will result, if too\n// large, the resulting geometry will loose detail\nvec3 sceneNormal(vec3 p)\n{\n    float h = 10*MIN_DIST;\n    vec2 e = vec2(1.0,-1.0);\n    return normalize(e.xyy*sceneSDF(p+e.xyy*h)+\n                     e.yyx*sceneSDF(p+e.yyx*h)+\n                     e.yxy*sceneSDF(p+e.yxy*h)+\n                     e.xxx*sceneSDF(p+e.xxx*h));\n}\n\nstruct Ray\n{\n    vec3 origin;\n    vec3 direction;\n};\n\nRay cameraRay(vec2 uv, vec3 cp, vec3 f, float fov)\n{\n    vec3 l = normalize(cross(vec3(0,1,0),f));\n    vec3 u = normalize(cross(f,l));\n    return Ray(cp,normalize(cp+f*fov-uv.x*l+uv.y*u-cp));\n}\n\n// Propagate a ray 'r' along its direction until either a scene surface is \n// hit or the maximum number of steps has been traversed. Returs the distance\n// to said hit point, otherwise returns 0\nfloat rayMarch(Ray r)\n{\n    float s = 0, ds = 0;\n    for (int step=0; step<MAX_STEPS; step++)\n    {\n        ds = sceneSDF(r.origin+r.direction*s)/SAFETY_FACTOR;\n        s += ds;\n        if (ds < MIN_DIST && ds > 0)\n            break;\n    }\n    return s > MAX_DIST ? 0 : s;\n}\n\nvoid main()\n{\n    // Define light source position and background color (used when no surface\n    // hit or for shadrows\n    const vec3 lightSource = vec3(2,4,2);\n    const vec4 bckgColor = vec4(0,0,0,1);\n\n    // Feel free to replace these with the iWASD, iLook\n    // uniforms to have live control over the camera and move in shader-space!\n    vec3 cameraPosition = iWASD;\n    vec3 cameraDirection = iLook;\n    \n    // Construct ray for this pixel and ray march\n    Ray r = cameraRay(qc,cameraPosition,cameraDirection,1);\n    float d = rayMarch(r);\n    if (d == 0) // If no surface hit\n        fragColor = bckgColor; //set to background color\n    else        // If surface hit\n    {\n        // Find surface hit point and direction from p to light source\n        vec3 p = r.origin+r.direction*d*.99;\n        vec3 pl = lightSource-p;\n\n        // Ray march from the hit point to the light source\n        Ray rl = Ray(p, normalize(pl));\n        float dl = rayMarch(rl);\n\n        // If ray marching does not reach light source it means that the point\n        // is in complete shade (hard shadow)\n        if (dl>0 && dl<.99*length(pl))\n            fragColor = bckgColor;\n        else \n        {\n            // Compute pixel color if fully or partially illuminated in the\n            // simples way, i.e. according to Lambertian cosine law\n            vec3 n = sceneNormal(p); \n            float c = max(dot(n,normalize(lightSource-p)),0);\n            fragColor = vec4(c,c,c,1);\n        }\n    }\n}",
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

const std::string Examples::pathMarching0 = 
R"({
    "UIScale": 0.6000000238418579,
    "autoSaveEnabled": true,
    "autoSaveInterval": 120.0,
    "sharedUniforms": {
        "windowResolution": [
            512.0,
            512.0
        ],
        "exportWindowResolutionScale": 1.0,
        "time": 0.0,
        "timePaused": true,
        "timeLooped": false,
        "timeBounds": [
            0.0,
            1.0
        ],
        "randomGeneratorPaused": false,
        "iWASD": [
            0.0,
            0.0,
            -4.5
        ],
        "iWASDSensitivity": 2.0,
        "iWASDInputEnabled": false,
        "iLook": [
            0.0,
            0.0,
            1.0
        ],
        "iLookSensitivity": 0.14892518520355225,
        "iLookInputEnabled": false,
        "iMouseInputEnabled": true,
        "iKeyboardInputEnabled": true,
        "smoothTimeDelta": false,
        "resetTimeOnFrameCounterReset": false,
        "vSyncEnabled": true,
        "uniforms": {}
    },
    "sharedFragmentSourceSize": 2429,
    "sharedFragmentSource": "// Common source code is shared by all fragment shaders across all layers and\n// has access to all shared in/out/uniform declarations\n\n#define IF_FRAG_X(X) if (int(gl_FragCoord.x)==X)\n#define IF_FRAG_Y(Y) if (int(gl_FragCoord.y)==Y)\n#define IF_FRAG_XY(X,Y) if (int(gl_FragCoord.x)==X && int(gl_FragCoord.y)==Y)\n\n// Keyboard defs for convenience. To access the state of a key, use the ivec3\n// iKeboard[KEY_XXX] uniform, where KEY_XXX is replaced by one of the defs here\n// below. The three components .x, .y, .z are 1 if the key is pressed (but not\n// held), held, toggled respectively, 0 otherwise\n#define KEY_TAB 9\n#define KEY_LEFT 37\n#define KEY_RIGHT 39\n#define KEY_UP 38\n#define KEY_DOWN 40\n#define KEY_DELETE 46\n#define KEY_BACKSPACE 8\n#define KEY_SPACE 32\n#define KEY_ENTER 13\n#define KEY_ESCAPE 27\n#define KEY_APOSTROPHE 222\n#define KEY_COMMA 188\n#define KEY_MINUS 189\n#define KEY_PERIOD 190\n#define KEY_SLASH 191\n#define KEY_SEMICOLON 186\n#define KEY_EQUAL 187\n#define KEY_LEFT_BRACKET 219\n#define KEY_BACKSLASH 220\n#define KEY_RIGHT_BRACKET 221\n#define KEY_GRAVE_ACCENT 192\n#define KEY_CAPS_LOCK 20\n#define KEY_LEFT_SHIFT 16\n#define KEY_LEFT_CONTROL 17\n#define KEY_LEFT_ALT 18\n#define KEY_LEFT_SUPER 91\n#define KEY_RIGHT_SHIFT 16\n#define KEY_RIGHT_CONTROL 17\n#define KEY_RIGHT_ALT 18\n#define KEY_0 48\n#define KEY_1 49\n#define KEY_2 50\n#define KEY_3 51\n#define KEY_4 52\n#define KEY_5 53\n#define KEY_6 54\n#define KEY_7 55\n#define KEY_8 56\n#define KEY_9 57\n#define KEY_A 65\n#define KEY_B 66\n#define KEY_C 67\n#define KEY_D 68\n#define KEY_E 69\n#define KEY_F 70\n#define KEY_G 71\n#define KEY_H 72\n#define KEY_I 73\n#define KEY_J 74\n#define KEY_K 75\n#define KEY_L 76\n#define KEY_M 77\n#define KEY_N 78\n#define KEY_O 79\n#define KEY_P 80\n#define KEY_Q 81\n#define KEY_R 82\n#define KEY_S 83\n#define KEY_T 84\n#define KEY_U 85\n#define KEY_V 86\n#define KEY_W 87\n#define KEY_X 88\n#define KEY_Y 89\n#define KEY_Z 90\n#define KEY_F1 112\n#define KEY_F2 113\n#define KEY_F3 114\n#define KEY_F4 115\n#define KEY_F5 116\n#define KEY_F6 117\n#define KEY_F7 118\n#define KEY_F8 119\n#define KEY_F9 120\n#define KEY_F10 121\n#define KEY_F11 122\n#define KEY_F12 123\n\n#define CROSSHAIR if(int(fragCoord.x) == int(iResolution.x/2) || int(fragCoord.y) == int(iResolution.y/2)){fragColor = vec4(1,0,0,1);}\n\n// For convenience when importing ShaderToy shaders\n#define SHADERTOY_MAIN void main(){mainImage(fragColor, fragCoord);}\nvec2 fragCoord = gl_FragCoord.xy; ",
    "sharedStorage": {
        "ioIntDataViewStartIndex": 0,
        "ioIntDataViewEndIndex": 7,
        "ioVec4DataViewEndIndex": 50,
        "ioVec4DataViewStartIndex": 0,
        "isVec4DataAlsoShownAsColor": false,
        "ioVec4DataViewPrecision": 6,
        "ioVec4DataViewExponentialFormat": false,
        "ioVec4DataViewComponents": [
            1,
            1,
            1,
            1
        ]
    },
    "layers": {
        "Source": {
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
                "exportClearPolicy": 0
            },
            "exportData": {
                "resolutionScale": 1.0,
                "rescaleWithOutput": false,
                "windowResolutionScale": 1.0
            },
            "shader": {
                "fragmentSourceSize": 15050,
                "fragmentSource": "precision mediump float;\n\n//----------------------------------------------------------------------------//\n\n#define ANTI_ALIASING true\n\n// Number of rays propagated per pixel\n#define N_SAMPLES 1\n\n// Maximum number of bounces per ray\n#define N_BOUNCES 8\n\n// Increase to avoid visual artifacts if SDFs get excessively non-Euclidean\n#define SAFETY_FACTOR 2.\n\n// Max number of ray marching steps\n#define MAX_STEPS 250 \n\n// Minimum distance from any scene surface below which ray marching stops\n#define MIN_DIST 5e-4\n\n// Maximum distance from any scene surface beyond which ray marching stops\n#define MAX_DIST 100\n\n// Useful quantities\n#define PI 3.14159\n#define PIBY2 1.57078\n#define PIBY4 0.785398\n#define UP vec3(0,1,0)\n#define RT vec3(-1,0,0)\n#define FW vec3(0,0,1)\n\n//----------------------------------------------------------------------------//\n// Misc\n\nvec3 rotate(vec3 v, float t, vec3 a)\n{\n    vec4 q = vec4(sin(t/2.0)*a.xyz, cos(t/2.0));\n    return v + 2.0*cross(q.xyz, cross(q.xyz, v) + q.w*v);\n}\n\n//----------------------------------------------------------------------------//\n// Pseudo-random\n// https://www.shadertoy.com/view/Xt3cDn\n\nuint baseHash(uvec2 p) \n{\n    p = 1103515245U*((p >> 1U)^(p.yx));\n    uint h32 = 1103515245U*((p.x)^(p.y>>3U));\n    return h32^(h32 >> 16);\n}\n\nfloat gSeed = \n    float(baseHash(floatBitsToUint(fragCoord)))/float(0xffffffffU)+iRandom;\n\nfloat hash1()\n{\n    uint n = baseHash(floatBitsToUint(vec2(gSeed+=.1,gSeed+=.1)));\n    return float(n)*(1.0/float(0xffffffffU));\n}\n\nvec2 hash2() \n{\n    uint n = baseHash(floatBitsToUint(vec2(gSeed+=.1,gSeed+=.1)));\n    uvec2 rz = uvec2(n, n*48271U);\n    return vec2(rz.xy & uvec2(0x7fffffffU))/float(0x7fffffff);\n}\n\nvec3 hash3() \n{\n    uint n = baseHash(floatBitsToUint(vec2(gSeed+=.1,gSeed+=.1)));\n    uvec3 rz = uvec3(n, n*16807U, n*48271U);\n    return vec3(rz & uvec3(0x7fffffffU))/float(0x7fffffff);\n}\n\nvec3 randomUnitVector()\n{\n    vec3 h = hash3() * vec3(2.,6.283185,1.)-vec3(1,0,0);\n    return pow(h.z, 1./3.)*vec3(sqrt(1.-h.x*h.x)*vec2(sin(h.y),cos(h.y)),h.x);\n}\n\n//----------------------------------------------------------------------------//\n// Materials\n\nstruct Material\n{\n    vec3  color;\n    float emittance;\n    float diffusiveReflectance;\n    float specularReflectance;\n    float transmittance;\n    float roughness;\n    float refractiveIndex;\n};\n\n// Material of the final 'hit' point\nMaterial gSceneMaterial;\n\n// Refractive index of the 'void' material encompassing all regions where the\n// scene SDF is positive\nfloat gVoidRefractiveIndex = 1.0;\n\n// Refractive index of the material that contains the current 'hit' point\nfloat gSceneRefractiveIndex = 1.0;\n\n#define MATERIAL_LIGHT_SOURCE(color, emittance) Material(color, emittance, 1.,0.,0.,0.,0.)\n#define MATERIAL_LAMBERTIAN(color) Material(color, 0.,1.,0.,0.,0.,0.)\n#define MATERIAL_METAL(color, roughness) Material(color, 0.,0.,1.,0.,roughness,0.)\n// Parameters are: \n//      color;\n//      reflectance at 90°  (ref);\n//      transmittance       (tra); \n//      surface roughness   (rgh);\n//      index of refraction (eta);\n// Parameters ref and tra need not sum to 1., they are normalized automatically\n#define MATERIAL_GLASS(color, ref, tra, rgh, eta) Material(color, 0.,0., ref, tra, rgh, eta)\n\nMaterial mixMaterials(Material m1, Material m2, float h)\n{\n    return Material\n    (\n        mix(m2.color, m1.color, h),\n        mix(m2.emittance, m1.emittance, h),\n        mix(m2.diffusiveReflectance, m1.diffusiveReflectance, h),\n        mix(m2.specularReflectance, m1.specularReflectance, h),\n        mix(m2.transmittance, m1.transmittance, h),\n        mix(m2.roughness, m1.roughness, h),\n        mix(m2.refractiveIndex, m1.refractiveIndex, h)\n    );\n}\n\nvoid mixWithSceneMaterial(Material m, float h)\n{\n    gSceneMaterial.color = \n        mix(m.color,                gSceneMaterial.color,                 h);\n    gSceneMaterial.emittance = \n        mix(m.emittance,            gSceneMaterial.emittance,             h);\n    gSceneMaterial.diffusiveReflectance = \n        mix(m.diffusiveReflectance, gSceneMaterial.diffusiveReflectance,  h);\n    gSceneMaterial.specularReflectance = \n        mix(m.specularReflectance,  gSceneMaterial.specularReflectance,   h);\n    gSceneMaterial.transmittance = \n        mix(m.transmittance,        gSceneMaterial.transmittance,         h);\n    gSceneMaterial.roughness = \n        mix(m.roughness,            gSceneMaterial.roughness,             h);\n    gSceneMaterial.refractiveIndex = \n        mix(m.refractiveIndex,      gSceneMaterial.refractiveIndex,       h);\n}\n\n//----------------------------------------------------------------------------//\n// SDFs\n\nfloat sphereSDF(vec3 p, float r)\n{\n    return length(p)-r;\n}\n\nfloat rectCuboidSDF(vec3 p, vec3 l)\n{\n    vec3 q = abs(p)-l/2.;\n    return length(max(q,0.0))+min(max(q.x,max(q.y,q.z)),0.0);\n}\n\nfloat torusSDF(vec3 p, float ro, float ri)\n{\n    vec2 q = vec2(length(p.xz)-ro,p.y);\n    return length(q)-ri;\n}\n\n// SDF Operations (not all of them are used). The 'f' parameter is used as a\n// mixing factor to mix materials to provide smooth material transitions (i.e.,\n// a material that smoothly transitions from metal to e.g. lambertian or glass,\n// as weird as that may look)\n\nfloat uniteSDFs(float sdf1, float sdf2, out float f)\n{\n    if (sdf1 < sdf2)\n    {\n        f = 1.;\n        return sdf1;\n    }\n    f = 0.;\n    return sdf2;\n}\n\nfloat subtractSDFs(float sdf1, float sdf2, out float f)\n{\n    f = 0.;\n    return max(-sdf1, sdf2);\n}\n\nfloat intersectSDFs(float sdf1, float sdf2, out float f)\n{\n    if (sdf1 > sdf2)\n    {\n        f = 1.;\n        return sdf1;\n    }\n    f = 0.;\n    return sdf2;\n}\n\nfloat smoothUniteSDFs(float sdf1, float sdf2, float s, out float f)\n{\n    f = clamp(0.5+0.5*(sdf2-sdf1)/s, 0.0, 1.0);\n    return mix(sdf2, sdf1, f)-s*f*(1.0-f);\n}\n\nfloat smoothSubtractSDFs(float sdf1, float sdf2, float s, out float f)\n{\n    f = clamp(0.5-0.5*(sdf2+sdf1)/s, 0.0, 1.0);\n    return mix(sdf2, -sdf1, f + s*f*(1.0-f));\n}\n\nfloat smoothIntersectSDFs(float sdf1, float sdf2, float s, out float f)\n{\n    f = clamp(0.5-0.5*(sdf2-sdf1)/s, 0.0, 1.0);\n    return mix(sdf2, sdf1, f)+s*f*(1.0-f);\n}\n\n//----------------------------------------------------------------------------//\n// Ray\n\nstruct Ray\n{\n    vec3 origin;\n    vec3 direction;\n};\n\nRay cameraRay(vec2 uv, vec3 cp, vec3 f, float fov)\n{\n    if (ANTI_ALIASING)\n        uv += ((hash2()-.5)/iResolution)/2.;\n    vec3 l = normalize(cross(vec3(0,1,0),f));\n    vec3 u = normalize(cross(f,l));\n    return Ray(cp,normalize(cp+f*fov-uv.x*l+uv.y*u-cp));\n}\n\n//----------------------------------------------------------------------------//\n//----------------------------------------------------------------------------//\n//----------------------------------------------------------------------------//\n\nfloat sceneSDF(vec3 p, bool computeMaterial)\n{\n    float scene, fMix;\n    \n    // Box with light source\n    vec3 box = vec3(3);\n    scene = min(min(p.y+box.y/2., -p.y+box.y/2.), -p.z+box.z/2.);\n    if (computeMaterial)\n        gSceneMaterial = MATERIAL_LAMBERTIAN(vec3(1.));\n    scene = uniteSDFs(scene, p.x+box.x/2., fMix);\n    if (computeMaterial)\n        mixWithSceneMaterial(MATERIAL_LAMBERTIAN(vec3(.75,.15,0.)), fMix);\n    scene = uniteSDFs(scene, -p.x+box.x/2., fMix);\n    if (computeMaterial)\n        mixWithSceneMaterial(MATERIAL_LAMBERTIAN(vec3(0.,.5,0.)), fMix);\n    scene = subtractSDFs(p.z+box.z/2., scene, fMix);\n    float light = rectCuboidSDF(p-UP*(box.y/2.), vec3(box.x/3., .01, box.z/3.));\n    scene = uniteSDFs(scene, light, fMix);\n    if (computeMaterial)\n        mixWithSceneMaterial(MATERIAL_LIGHT_SOURCE(vec3(1.), 40.), fMix);\n        \n    // Stuff in box\n    // Lambertian parallelopiped in on the back-left\n    vec3 p0 = rotate(p+vec3(-box.x/5., .5, -box.z/4.), -.5, UP);\n    scene = uniteSDFs(scene, rectCuboidSDF(p0, vec3(1.,2.,1.)), fMix);\n    if (computeMaterial)\n        mixWithSceneMaterial(MATERIAL_LAMBERTIAN(vec3(1.)), fMix);\n    \n    // Blue-ish metal sphere\n    p0 = p+vec3(box.x/4., -box.y/8., 0.);\n    scene = uniteSDFs(scene, sphereSDF(p0, .625), fMix);\n    if (computeMaterial)\n        mixWithSceneMaterial(MATERIAL_METAL(vec3(.5,.75,1.), .0), fMix);\n        \n    // Ice-cube-like thing\n    p0 = rotate(rotate(p+vec3(.125, .5, .5), 1., UP), 1.1, RT);\n    scene = uniteSDFs(scene, rectCuboidSDF(p0, vec3(.6))-.1, fMix);\n    if (computeMaterial)\n        mixWithSceneMaterial(MATERIAL_GLASS(vec3(1.),.06,1.,0.05,1.33),fMix);\n        \n    // Weird thing to demostrate blending of different materials, e.g., metal\n    // and glass\n    p0 = rotate(rotate(p+vec3(-.75, 1, .75), rot0, RT), rot1, FW);\n    float fMixTor;\n    float tor = torusSDF(p0-vec3(.3,0,0)-.1*sin(5*p0.x*p0.z), .35, .15);\n    p0 = p0+vec3(.3,0,0);\n    tor = smoothUniteSDFs(tor,sphereSDF(p0,.25)+.075*cos(20*p0.z),.25,fMixTor);\n    \n    scene = uniteSDFs(scene, tor, fMix);\n    if (computeMaterial)\n    {\n        Material m = mixMaterials\n        (\n            MATERIAL_METAL(vec3(1.), .05),\n            MATERIAL_GLASS(vec3(1.), .06, 1., 0., 1.33),\n            fMixTor\n        );\n        mixWithSceneMaterial(m, fMix);\n    }\n    \n    return scene;\n}\n\n//----------------------------------------------------------------------------//\n//----------------------------------------------------------------------------//\n//----------------------------------------------------------------------------//\n\n// True if path-marching inside a material\nbool gInside = false;\n\nvec3 sceneNormal(vec3 p, float h)\n{\n    vec2 e = vec2(1.0,-1.0);\n    vec3 n = normalize( e.xyy*sceneSDF(p+e.xyy*h, false)+\n                        e.yxy*sceneSDF(p+e.yxy*h, false)+\n                        e.yyx*sceneSDF(p+e.yyx*h, false)+\n                        e.xxx*sceneSDF(p+e.xxx*h, false));\n    return gInside ? -n : n;\n}\n\n//----------------------------------------------------------------------------//\n\nfloat rayMarch(Ray r)\n{\n    float s = 0;\n    float ds = 0;\n    vec3 p;\n    for (int q=0; q<MAX_STEPS; q++)\n    {\n        p = r.origin+r.direction*s;\n        ds = sceneSDF(p, false)/SAFETY_FACTOR;\n        if (gInside)\n            ds *= -1;\n        s += ds;\n        if ((ds < MIN_DIST && ds > 0) || s > MAX_DIST)\n            break;\n    }\n    s = s*float(s <= MAX_DIST);\n    return s; \n}\n\n//----------------------------------------------------------------------------//\n\n// The result is stored in gSceneMaterial\nvoid computeMaterial(vec3 p, out vec3 n)\n{\n    n = sceneNormal(p, MIN_DIST);\n    sceneSDF(p-n*10*MIN_DIST, true);\n}\n\n//----------------------------------------------------------------------------//\n\nint gBounce = 0;\nint gSample = 0;\nvoid scatter(inout Ray ray, float s)\n{\n    ray.origin += s*ray.direction*(1.-MIN_DIST);\n    vec3 n;\n    computeMaterial(ray.origin, n);\n    ray.origin += n*10*MIN_DIST;\n    \n    vec3 coeff = vec3(\n        gSceneMaterial.diffusiveReflectance,\n        gSceneMaterial.specularReflectance,\n        gSceneMaterial.transmittance);\n    \n    float sum = coeff.x+coeff.y+coeff.z;\n    coeff /= sum;\n    \n    // If material is purely diffusive\n    if (coeff.x > 1.-1e-6)\n    {\n        ray.direction = normalize(n+randomUnitVector());\n        return;\n    }\n    \n    // At near-grazing angles, the only part of the rough surface we can see\n    // are the microfacet 'hill-tops', which tend to have their microfacet\n    // normals reasonably aligned with the macroscopic surface normal.\n    // To 'model' this, I 'attenuate' the roughness based on how grazing the \n    // view angle is. The smoothstep bounds are arbitrary\n    if (gSceneMaterial.roughness > 0.)\n        gSceneMaterial.roughness *= smoothstep(.0, .5, dot(-ray.direction, n));\n       \n    // Microfacet normal & reflected ray\n    vec3 mfn = normalize(n+gSceneMaterial.roughness*randomUnitVector());\n    vec3 mfr = reflect(ray.direction, mfn);\n    \n    // If material is purely reflective\n    if (coeff.x > .999999)\n    {\n        ray.direction = mfr;\n        return;\n    }\n    \n    // Enchance reflectiveness via Schlick approximation for Fresnel reflectance\n    // I am well aware that the base reflectance (i.e., the reflectance at\n    // normal incidence, i.e., my coeff.y) is not a direct input, rather, it is\n    // computed based on the material refractive indices at both inteface sides,\n    // but having the user directly specify the reflectance is more intuitive\n    // when compared to having to reason about the refractive indices of non-\n    // transmissive materials (e.g., wood, steel, you name it...), which make\n    // no intuitive sense. In short, if you want greater physical accuracy, make\n    // sure to always override coeff.y with the ratio of refractive indices\n    // (eta0/eta1 below), and to always provide refractive indices for all \n    // materials \n    coeff.y += (1.-coeff.y)*pow(1.-dot(mfr, n), 5.);\n    coeff.z = max(1.-coeff.x-coeff.y, 0.f);\n    coeff.x = 1.-coeff.y-coeff.z;\n    \n    // Determine what type of scattering the ray undergoes\n    float discriminant = fract(gSeed);\n    \n    if (discriminant <= coeff.x)        // Diffusion\n        ray.direction = normalize(n+randomUnitVector());\n    else if (discriminant < 1.-coeff.z) // Reflection\n        ray.direction = mfr;\n    else                                // Transmission with refraction\n    {\n        ray.origin -= 2*n*10*MIN_DIST;\n        gInside = sceneSDF(ray.origin, false) < 0 ? true : false;\n        // gSceneRefractiveIndex always stores the refractive index of the\n        // material the ray-marched point is currently in\n        float eta0 = gSceneRefractiveIndex;\n        float eta1 = \n            gInside ? gSceneMaterial.refractiveIndex : gVoidRefractiveIndex;\n        gSceneRefractiveIndex = eta1;\n        ray.direction = refract(ray.direction, mfn, eta0/eta1);\n        if (length(ray.direction) == 0) // I.e., total reflection\n            ray.direction = mfr;\n    }\n}\n\n//----------------------------------------------------------------------------//\n\nvec3 computeColor(in Ray ray)\n{\n    vec3 pathColor = vec3(1);\n    vec3 color = vec3(0);\n    for (gBounce=0; gBounce<N_BOUNCES; gBounce++)\n    {\n        float s = rayMarch(ray);\n        if (s > 0.)\n        {\n            scatter(ray, s);\n            color += pathColor*gSceneMaterial.color*gSceneMaterial.emittance;\n            pathColor *= gSceneMaterial.color;\n        }\n        else\n            break;\n    }\n    return color;\n}\n\n//----------------------------------------------------------------------------//\n\nvoid main() \n{   \n    Ray ray;\n    fragColor = vec4(0);\n    for (gSample=0; gSample<N_SAMPLES; gSample++)\n    {\n        gBounce = 0;\n        ray = cameraRay(qc, iWASD, iLook, 1);\n        float s0 = sceneSDF(ray.origin, true);\n        gSceneRefractiveIndex = \n            s0 > 0 ? \n            gVoidRefractiveIndex : \n            gSceneMaterial.refractiveIndex;\n        gInside = s0 > 0 ? false : true;\n        fragColor += vec4(computeColor(ray),1.)/N_SAMPLES;\n    }\n    if (!iUserAction)\n        fragColor += texelFetch(iChannel0, ivec2(fragCoord), 0);\n    if (isnan(fragColor.r))\n        fragColor = vec4(0,0,0,1);\n}",
                "uniforms": {
                    "iChannel0": {
                        "type": "texture2D",
                        "shared": false,
                        "value": "Source"
                    },
                    "rot0": {
                        "type": "float",
                        "shared": false,
                        "value": -0.5400000214576721,
                        "min": -1.5700000524520874,
                        "max": 1.5700000524520874
                    },
                    "rot1": {
                        "type": "float",
                        "shared": false,
                        "value": -0.6299999952316284,
                        "min": -1.5700000524520874,
                        "max": 1.5700000524520874
                    }
                }
            }
        },
        "Window": {
            "renderTarget": 2,
            "resolution": [
                256,
                256
            ],
            "resolutionRatio": [
                0.5,
                0.5
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
                "fragmentSourceSize": 605,
                "fragmentSource": "vec3 tonemapACES(vec3 v)\n{\n    v = vec3(\n        .59719f*v.x + .35458f*v.y + .04823f*v.z,\n        .07600f*v.x + .90834f*v.y + .01566f*v.z,\n        .02840f*v.x + .13383f*v.y + .83777f*v.z\n    );\n    v = (v*(v + .0245786f) - .000090537f)/\n        (v*(.983729f*v + .4329510f) + .238081f);\n    return vec3(\n        1.60475f*v.x + -.53108f*v.y + -.07367f*v.z,\n        -.10208f*v.x + 1.10813f*v.y + -.00605f*v.z,\n        -.00327f*v.x + -.07276f*v.y + 1.07602f*v.z\n    );\n}\n\nvoid main()\n{\n    fragColor = texture(iChannel0, tc);\n    fragColor.rgb = tonemapACES(fragColor.rgb/fragColor.a);\n    fragColor.a = 1.;\n}",
                "uniforms": {
                    "iChannel0": {
                        "type": "texture2D",
                        "shared": false,
                        "value": "Source"
                    }
                }
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
    bool loadExampleConfirmation = false;
    static const std::string* tmpSelection = nullptr;
    selection = nullptr;
    ImGui::Text
    (
R"(This is a collection of built-in project examples which can be freely loaded,
edited and used for learning purposes or as starting points for other projects)"
    );
    ImGui::Separator();

    int id = 0;
    
    ImGui::PushID(id++);
    if (ImGui::Button("Load"))
    {
        loadExampleConfirmation = true;
        tmpSelection = &Examples::rayMarching0;
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Text("Ray marcher I");

    ImGui::PushID(id++);
    if (ImGui::Button("Load"))
    {
        loadExampleConfirmation = true;
        tmpSelection = &Examples::pathMarching0;
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Text("Path marcher I");

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