/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "data/coderepository.h"
#include "data/data.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"

namespace ShaderThing
{

void CodeRepository::renderGui()
{
    if (!isGuiOpen_)
        return;

    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(600,600), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags
        (
            ImGuiWindowFlags_NoCollapse
        );
        ImGui::Begin("Code repository", &isGuiOpen_, windowFlags);
        static bool setIcon(false);
        if (!setIcon)
        {
            setIcon = vir::ImGuiRenderer::setWindowIcon
            (
                "Code repository", 
                IconData::sTIconData, 
                IconData::sTIconSize,
                false
            );
        }
    }
    float fontSize(ImGui::GetFontSize());
    float textWidth(46.0f*fontSize);
    auto codeColor =ImVec4(.15f, .6f, 1.f, 1.f);
    auto codeHighlightColor = ImVec4(1.f, 1.f, .0f, 1.f);
    ImGui::PushTextWrapPos
    (
        isGuiInMenu_ ?
        (ImGui::GetCursorPos().x+textWidth) : 
        ImGui::GetContentRegionAvail().x
    );

    ImGui::Text(
"This is a repository of helpful GLSL functions that can be copy-pasted into "
"any fragment shader"
    );
    ImGui::Separator();

#define CODE_ENTRY(name, description, code)                                 \
if (ImGui::TreeNode(name))                                                  \
{                                                                           \
    ImGui::SameLine();                                                      \
    bool textCopied = ImGui::SmallButton("Copy to clipboard");              \
    bool isCopyClicked = ImGui::IsItemActive();                             \
    ImGui::Text(description);                                               \
    static std::string codeString(code);                                    \
    ImGui::TextColored(                                                     \
        textCopied || isCopyClicked ?                                       \
            codeHighlightColor:                                             \
            codeColor,                                                      \
        codeString.c_str()                                                  \
    );                                                                      \
    if (textCopied)                                                         \
        ImGui::SetClipboardText(codeString.c_str());                        \
    ImGui::TreePop();                                                       \
}

    //------------------------------------------------------------------------//
    if (ImGui::TreeNode("Noise"))
    {

        //--------------------------------------------------------------------//
        if (ImGui::TreeNode("2D"))
        {

            CODE_ENTRY(
"Pseudo-random number generator",
"Returns a pseudo-random float given an input vec2 seed 'x', e.g., the default"
"quad coordinate 'qc' or texture coordinate 'tc'",
R"(float random2D(vec2 x)
{
    return fract(138912*sin(dot(x, vec2(138.9, 191.2))));
})")

            CODE_ENTRY(
"Perlin noise",
"Returns a pseudo-random float representative of a single octave of Perlin-like"
"noise at 2D coordinates 'x', e.g., the default quad coordinate 'qc' or texture"
"coordinate 'tc'. A 'random2D' function should be defined as well, e.g., the "
"one provided in Code repository -> Noise -> 2D -> Pseudo-random number "
"generator",
R"(float noise2D(vec2 x)
{
    vec2 l = floor(x);
    vec2 r = fract(x);
    vec2 e = vec2(1.,0.);
    vec2 f = r*r*(3.-2.*r);
    return mix
    (
        mix(random2D(l),     random2D(l+e.xy),f.x),
        mix(random2D(l+e.yx),random2D(l+e.xx),f.x),
        f.y
    );
})")

            CODE_ENTRY(
"Three-point noise",
"A more efficient implementation of Perlin-like noise (though it is not "
"properly Perlin) which only requires sampling 3 random numbers instead of "
"4, as seen in the Perlin noise function. Can result in performance gains in "
"the range of 5-10%% at the expense of some noise bias. A 'random2D' function "
"should be defined as well, e.g., the one provided in Code repository -> "
"Noise -> 2D -> Pseudo-random number generator",
R"(float noise2D(vec2 x)
{
    vec2 l = floor(x);
    vec2 r = fract(x);
    float s = float(int(r.x+r.y > 1.));
    vec2 e = vec2(1.,0.);
    r.y = s+r.y*(1.-2.*s);
    r.x = (r.x-s*r.y)/(1.-r.y);
    r *= r*(3.-2.*r);
    return 
        mix
        (
            mix(random2D(l+s*e.yx), random2D(l+s*e.yx+e.xy), r.x), 
            random2D(l+s*e.xy+(1.-s)*e.yx), 
            r.y
        );
})")

            CODE_ENTRY(
"Fractional noise",
"Returns a pseudo-random float representative of 'o' octaves of fractional "
"noise at 2D coordinates 'x', e.g., the default quad coordinate 'qc' or "
"texture coordinate 'tc'. A 'noise2D' function should be defined as well, "
"e.g., the one provided in Code repository -> Noise -> 2D -> Perlin noise. The "
"fractional noise is smoother for increasing values of the 'h' parameter. In "
"particular: h=0 for Pink noise; h=0.5 for Brown noise; h=1.0 for the most "
"common implementation referred to as 'fractional brownian motion'",
R"(float fractionalNoise2D(vec2 x, float h, uint o)
{
    float n = 0.;
    float s = exp2(-h);
    float A = 0.;
    vec2 af = vec2(1., 1.);
    for (int i=0; i<o; i++)
    {
        n += af.x*noise2D(af.y*x);
        A += af.x;
        af *= vec2(s, 2.);
    }
    return n/A;
})")
            ImGui::TreePop();
        }

        //--------------------------------------------------------------------//
        if (ImGui::TreeNode("3D"))
        {
            CODE_ENTRY(
"Pseudo-random number generator",
"Returns a pseudo-random float given an input vec3 seed 'x'",
R"(float random3D(vec3 x)
{
    return fract(138912*sin(dot(x, vec3(138.9, 191.2, 695.7))));
})")

            CODE_ENTRY(
"Perlin noise",
"Returns a pseudo-random float representative of a single octave of Perlin-like "
"noise at 3D coordinates 'x'. A 'random3D' function should be defined as well, "
"e.g., the one provided in Code repository -> Noise -> 3D -> Pseudo-random "
"number generator",
R"(float noise3D(vec3 x)
{
    vec3 l = floor(x);
    vec3 r = fract(x);
    vec2 e = vec2(1.,0.);
    vec3 f = r*r*(3.-2.*r);
    return 
        mix
        (
            mix
            (
                mix(random3D(l),      random3D(l+e.xyy),f.x),
                mix(random3D(l+e.yxy),random3D(l+e.xxy),f.x),
                f.y
            ),
            mix
            (
                mix(random3D(l+e.yyx),random3D(l+e.xyx),f.x),
                mix(random3D(l+e.yxx),random3D(l+e.xxx),f.x),
                f.y
            ),
            f.z
        );
})")

            CODE_ENTRY(
"Six-point noise",
"A more efficient implementation of Perlin-like noise (though it is not "
"properly Perlin) which only requires sampling 6 random numbers instead of "
"8, as seen in the Perlin noise function. Can result in performance gains in "
"the range of 10-20%% at the expense of some noise bias. A 'random3D' function "
"should be defined as well, e.g., the one provided in Code repository -> "
"Noise -> 3D -> Pseudo-random number generator",
R"(float noise3D(vec3 x)
{
    vec3 l = floor(x);
    vec3 r = fract(x);
    float s = float(int(r.x+r.y > 1.));
    vec2 e = vec2(1.,0.);
    r.y = s+r.y*(1.-2.*s);
    r.x = (r.x-s*r.y)/(1.-r.y);
    r *= r*(3.-2.*r);
    return 
        mix
        (
            mix
            (
                mix(random3D(l+s*e.yxy), random3D(l+s*e.yxy+e.xyy), r.x), 
                random3D(l+s*e.xyy+(1.-s)*e.yxy), 
                r.y
            ),
            mix
            (
                mix(random3D(l+s*e.yxy+e.yyx),random3D(l+s*e.yxy+e.xyx),r.x),
                random3D(l+s*e.xyy+(1.-s)*e.yxy+e.yyx), 
                r.y
            ),
            r.z
        );
})")

            CODE_ENTRY(
"Fractional noise",
"Returns a pseudo-random float representative of 'o' octaves of fractional "
"noise at 3D coordinates 'x'. A 'noise3D' function should be defined as well, "
"e.g., the one provided in Code repository -> Noise -> 3D -> Perlin noise. The "
"fractional noise is smoother for increasing values of the 'h' parameter. In "
"particular: h=0 for Pink noise; h=0.5 for Brown noise; h=1.0 for the most "
"common implementation referred to as 'fractional brownian motion'",
R"(float fractionalNoise3D(vec2 x, float h, uint o)
{
    float n = 0.;
    float s = exp2(-h);
    float A = 0.;
    vec2 af = vec2(1., 1.);
    for (int i=0; i<o; i++)
    {
        n += af.x*noise3D(af.y*x);
        A += af.x;
        af *= vec2(s, 2.);
    }
    return n/A;
})")
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    //------------------------------------------------------------------------//
    if (ImGui::TreeNode("Color manipulation"))
    {
        CODE_ENTRY
        (
"Luminance",
"This function takes a color 'c' as input and returns its perceived luminance",
R"(float luminance(vec3 c)
{
    return dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
})"
        )

        CODE_ENTRY
        (
"sRGB to linear color",
"Convert a color 'c' from non-linear sRGB space to linear RGB space. Should be "
"used on a color before manipulating it in linear space (e.g., before "
"blurring, filtering, etc.), otherwise luminosity is not preserved ",
R"(vec3 sRGBToLinear(vec3 c)
{
    return vec3
    (
        c.x <= .04045f ? c.x/12.92f : pow((c.x+.055f)/1.055, 2.4f),
        c.y <= .04045f ? c.y/12.92f : pow((c.y+.055f)/1.055, 2.4f),
        c.z <= .04045f ? c.z/12.92f : pow((c.z+.055f)/1.055, 2.4f)
    );
})"
        )

        CODE_ENTRY
        (
"Linear to sRGB color",
"Convert a color 'c' from linear RGB space to non-linear sRGB space. Should be "
"used on a color after manipulating it in linear space (e.g., after "
"blurring, filtering, etc.), before finally rendering it ",
R"(vec3 linearToSRGB(vec3 c)
{
    return vec3
    (
        c.x <= .0031308f?c.x*12.92f:max(pow(c.x, 1.f/2.4f)*1.055f-.055f,0.f),
        c.y <= .0031308f?c.y*12.92f:max(pow(c.y, 1.f/2.4f)*1.055f-.055f,0.f),
        c.z <= .0031308f?c.z*12.92f:max(pow(c.z, 1.f/2.4f)*1.055f-.055f,0.f)
    );
})"
        )

        CODE_ENTRY
        (
"sRGB to linear color (fast)",
"Approximate faster conversion of a color 'c' from non-linear sRGB space to "
"linear RGB space. Should be used on a color before manipulating it in linear "
"space (e.g., before blurring, filtering, etc.), otherwise luminosity is not "
" preserved",
R"(vec3 sRGBToLinearFast(vec3 c)
{
    return pow(c, vec3(2.2f));
})"
        )

        CODE_ENTRY
        (
"Linear to sRGB color (fast)",
"Approximate faster conversion of a color 'c' from linear RGB space to "
"non-linear sRGB space. Should be used on a color after manipulating it in "
"linear space (e.g., after blurring, filtering, etc.), before finally "
"rendering it",
R"(vec3 linearToSRGBFast(vec3 c)
{
    return pow(c, vec3(.4545f));
})"
        )

        CODE_ENTRY
        (
"Reinhard tone map",
"Function to map a High Dynamic Range (HDR) color 'c' (e.g., from a loaded "
"texture) to Low Dynamic Range (LDR) via the extended Reinhard tone map. Any "
"color whose luminance matches or exceeds the provided white point luminance "
"'wpl' will be mapped to white",
R"(vec3 tonemapReinhard(vec3 c, float wpl=1.0)
{
    float l = dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
    return c*(1.f + l/wpl/wpl)/(1.f + l);
})"
        )

        CODE_ENTRY
        (
"Reinhard-Jolie tone map",
"Function to map a High Dynamic Range (HDR) color 'c' (e.g., from a loaded "
"texture) to Low Dynamic Range (LDR) via the Reinhard-Jolie tone map",
R"(vec3 tonemapReinhardJolie(vec3 c)
{
    vec3 r = c/(c + 1.0);
    return mix(r/(dot(c, vec3(0.2126f, 0.7152f, 0.0722f)) + 1.0), r, r);
})"
        )

        CODE_ENTRY
        (
"ACES tone map",
"Function to map a High Dynamic Range (HDR) color 'c' (e.g., from a loaded "
"texture) to Low Dynamic Range (LDR) via the Academy Color Encoding System "
"(ACES) filmic tone map (used e.g., by default in Unreal Engine 4)",
R"(vec3 tonemapACES(vec3 v)
{
    v = vec3(
        .59719f*v.x + .35458f*v.y + .04823f*v.z,
        .07600f*v.x + .90834f*v.y + .01566f*v.z,
        .02840f*v.x + .13383f*v.y + .83777f*v.z
    );
    v = (v*(v + .0245786f) - .000090537f)/
        (v*(.983729f*v + .4329510f) + .238081f);
    return vec3(
        1.60475f*v.x + -.53108f*v.y + -.07367f*v.z,
        -.10208f*v.x + 1.10813f*v.y + -.00605f*v.z,
        -.00327f*v.x + -.07276f*v.y + 1.07602f*v.z
    );
})"
        )

        ImGui::TreePop();
    }

    //------------------------------------------------------------------------//
    if (ImGui::TreeNode("Filters & effects"))
    {
        CODE_ENTRY
        (
"1-D Gaussain blur",
"This complete shader applies a 1-D Gaussian blur to a provided 'target' "
"texture in the selected direction. The target texture needs to be added as a "
"layer uniform and needs to be named 'target'. Please note that the original "
"texture is not modified, and the blurred texture is the output of the shader. "
"A 2-D Gaussian blur can be achieved e.g., by inputting the output of a 1-D "
"Gaussian blur shader (in direction 0) into another 1-D Gaussian blur shader "
"(in direction 1)",
R"(#define DIRECTION 0 // 0 Horizontal, 1 is vertical
#define BLUR_RADIUS 10
#ifndef SIGMA // Feel free to set your own value for the gaussian std. dev.
    #define SIGMA sqrt(BLUR_RADIUS)
#endif
#define GAUSS(i) exp(-(i*i)/(2*SIGMA*SIGMA));

void main()
{   
    #if (DIRECTION == 0)
    vec2 d = vec2(1.0/targetResolution.x, 0.0);
    #endif
    #if (DIRECTION == 1)
    vec2 d = vec2(0.0, 1.0/targetResolution.y);
    #endif
    fragColor = texture(target, tc);
    float gaussSum = GAUSS(0);
    for(int i = 1; i < BLUR_RADIUS; i++)
    {
        float g = GAUSS(i);
        gaussSum += g;
        fragColor += .5*g*texture(target, tc+i*d);
        fragColor += .5*g*texture(target, tc-i*d);
    }
    fragColor /= gaussSum;
})"
        )

        CODE_ENTRY
        (
"Fast approximate anti-aliasing (FXAA)",
"This complete shader performes fast approximate anti-aliasing (FXAA) on a "
"target texture2D uniform via the FXAA 3.11 algorithm. The target texture "
"needs to be added as a layer uniform and needs to be named 'target'. Please "
"note that the original texture is not modified, and the anti-aliased texture "
"is the output of this shader. Please also note that this code snipped is too "
"long to be viewed in its entirety, but it can be copied entirely to clipoboard",
R"(/*--------------------------------------------------------------------------*\
  FXAA 3.11 Implementation by effendiian & cleanup by virmodoetiae
  ---------------------------------------------------------------------------
  FXAA implementation based off of the work by Timothy Lottes in the Nvidia 
  white paper:
  https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
*\--------------------------------------------------------------------------*/

// Turn off FXAA.
// #define FXAA 0

// Turn on FXAA.
#define FXAA 1

// Quality presets [0 is lowest, 5 is highest] available for convenience,
// not mandatory to have
#define FXAA_PRESET 5

/*
/    FXAA setting, defined via preprocessor variables
*/
#ifndef FXAA_PRESET
    #define FXAA_PRESET 5
    #define FXAA_DEBUG_SKIPPED 0
    #define FXAA_DEBUG_PASSTHROUGH 0
    #define FXAA_DEBUG_HORZVERT 0
    #define FXAA_DEBUG_PAIR 0
    #define FXAA_DEBUG_NEGPOS 0
    #define FXAA_DEBUG_OFFSET 0
    #define FXAA_DEBUG_HIGHLIGHT 0
    #define FXAA_LUMINANCE 1
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 0)
    #define FXAA_EDGE_THRESHOLD      (1.0/4.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/12.0)
    #define FXAA_SEARCH_STEPS        2
    #define FXAA_SEARCH_ACCELERATION 4
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       1
    #define FXAA_SUBPIX_CAP          (2.0/3.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 1)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/16.0)
    #define FXAA_SEARCH_STEPS        4
    #define FXAA_SEARCH_ACCELERATION 3
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 2)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        8
    #define FXAA_SEARCH_ACCELERATION 2
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 3)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        16
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 4)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        24
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 5)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        32
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#define FXAA_SUBPIX_TRIM_SCALE (1.0/(1.0 - FXAA_SUBPIX_TRIM))

// --------------------------------------
// Helper functions.
// --------------------------------------

// ---------------------
// Texture sampler functions.

// Return sampled image from a point + offset texel space.
vec4 textureOffset( sampler2D tex, 
                    vec2 uv, 
                    vec2 offset ) {
    
    // Return color from the specified location.
    return texture(tex, uv + offset); 
        
}

// ---------------------
// Luminance functions.

// Map RGB to Luminance linearly.
float linearRGBLuminance( vec3 color ) {
    
    // Weights for relative luma from here: 
    // https://en.wikipedia.org/wiki/Luma_(video)
    vec3 weight = vec3(0.2126729, 0.7151522, 0.0721750);
    
    // Get the dot product:
    // - color.r * weight.r + color.g * weight.g + color.b * weight*b.
    return dot(color, weight);
}

// Luminance based off of the original specification.
float FXAALuminance( vec3 color ) {
    
    #if FXAA_LUMINANCE == 0
    
    return linearRGBLuminance( color );
    
    #else
    
    return color.g * (0.587/0.299) + color.r;
    
    #endif
}

// ---------------------
// Vertical/Horizontal Edge Test functions.

float FXAAVerticalEdge(float lumaO,
                       float lumaN,
                       float lumaE,
                       float lumaS,
                       float lumaW,
                       float lumaNW,
                       float lumaNE,
                       float lumaSW,
                       float lumaSE)
{
    // Slices to calculate.
    float top = (0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE);
    float middle = (0.50 * lumaW ) + (-1.0 * lumaO) + (0.50 * lumaE );
    float bottom = (0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE);
    
    // Return value.
    return abs(top) + abs(middle) + abs(bottom);
}

float FXAAHorizontalEdge(float lumaO,
                         float lumaN,
                         float lumaE,
                         float lumaS,
                         float lumaW,
                         float lumaNW,
                         float lumaNE,
                         float lumaSW,
                         float lumaSE)
{    
    // Slices to calculate.
    float top = (0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW);
    float middle = (0.50 * lumaN ) + (-1.0 * lumaO) + (0.50 * lumaS );
    float bottom = (0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE);
    
    // Return value.
    return abs(top) + abs(middle) + abs(bottom);
}

// ------------------------
// FXAA specific functions.
// ------------------------

// Entry point for the FXAA process.
vec4 applyFXAA(sampler2D src,
               vec2 srcResolution)
{    
    // Normalized pixel coordinates (from 0 to 1).
    vec2 uv = tc;
    
    // Calculate distance between pixels in texture space.
    vec2 texel = vec2(1.0, 1.0) / srcResolution;
    
    // Caculate the luminance.
    // float luma = FXAALuminance(rgbO.xyz);
    // float luma = linearRGBLuminance(clamp(rgbO.xyz, 0.0, 1.0));
    
    //-------------------------
    // 1. LOCAL CONTRAST CHECK
    
    // Sample textures from cardinal directions.
    vec3 rgbN = textureOffset(src, uv, vec2(0, -texel.y)).rgb; // N
    vec3 rgbW = textureOffset(src, uv, vec2(-texel.x, 0)).rgb; // W
    vec3 rgbE = textureOffset(src, uv, vec2(texel.x, 0)).rgb; // E
    vec3 rgbS = textureOffset(src, uv, vec2(0, texel.y)).rgb; // S
    vec4 rgbO4 = textureOffset(src, uv, vec2(0, 0)); // ORIGIN
    vec3 rgbO = rgbO4.rgb;
    float alpha = rgbO4.a;
    
    #if FXAA == 0
    return rgbO4; // Skip FXAA if it is off.
    #endif    
    
    // Calculate the luminance for each sampled value.
    float lumaN = FXAALuminance(rgbN);
    float lumaW = FXAALuminance(rgbW);
    float lumaO = FXAALuminance(rgbO);
    float lumaE = FXAALuminance(rgbE);
    float lumaS = FXAALuminance(rgbS);
    
    // Calculate the minimum luma range.
    float minLuma = min(lumaO, min(min(lumaN, lumaW), min(lumaS, lumaE)));
    float maxLuma = max(lumaO, max(max(lumaN, lumaW), max(lumaS, lumaE)));
    float localContrast = maxLuma - minLuma;    
    
    // Check for early exit.
    if
    (
        localContrast < 
        max(FXAA_EDGE_THRESHOLD_MIN, maxLuma*FXAA_EDGE_THRESHOLD)
    )
    {
        #if FXAA_DEBUG_SKIPPED
        return vec4(0, 0, 0, alpha);
        #else
        return rgbO4;
        #endif
    }
    
    //-------------------------
    // 2. SUB-PIXEL ALIASING TEST
    
    // Calculate the pixel contrast ratio.
    // - Sub-pixel aliasing is detected by taking the ratio of the 
    // pixel contrast over the local contrast. This ratio nears 1.0
    // in the presence of single pixel dots and otherwise falls off
    // towards 0.0 as more pixels contribute to an edge. This ratio
    // is transformed into the amount of lowpass filter to blend in
    // at the end of the algorithm.
    
    #if FXAA_SUBPIX > 0
    // Calculate sum of local samples for the lowpass.
    vec3 rgbL = (rgbN + rgbW + rgbO + rgbE + rgbS);
    
        #if FXAA_SUBPIX_FASTER
        // Average the lowpass now since this skips the addition of the
        // diagonal neighbors (NW, NE, SW, SE).
        rgbL *= (1.0/5.0);
        #endif    

    // Calculate the lowpass luma.
    // - Lowpass luma is calculated as the average between the luma of
    //   neigboring pixels.
    float lumaL = (lumaN + lumaW + lumaS + lumaE) * 0.25;

    // Calculate the pixel contrast.
    // - Pixel contrast is the abs() difference between origin pixel luma
    //   and lowpass luma of neighbors.
    float pixelContrast = abs(lumaL - lumaO);
    
    // Remember: 
    // - pixel contrast is origin - lowpass(neighbors);
    // - local contrast is 
    //   min(origin + neighbors) - max(origin + neighbors) < threshold.
   
    // Calculate the ratio between the pixelContrast and localContrast.
    float contrastRatio = pixelContrast / localContrast;
    // Default is zero. Will be changed depending on subpixel level.
    float lowpassBlend = 0.0; 
    
        #if FXAA_SUBPIX == 1
        // Normal subpixel aliasing. Set based on FXAA algorithm for subpixel
        // aliasing.
        lowpassBlend = 
            max(0.0, contrastRatio-FXAA_SUBPIX_TRIM)*FXAA_SUBPIX_TRIM_SCALE;
        lowpassBlend = min(FXAA_SUBPIX_CAP, lowpassBlend);
        #elif FXAA_SUBPIX == 2
        // Full force subpixel aliasing. Set blend to ratio.
        lowpassBlend = contrastRatio;
        #endif
    #endif
    
    // Show selected pixels if debug mode is active.
    #if FXAA_DEBUG_PASSTHROUGH
        #if FXAA_SUBPIX > 0    
        return vec4(localContrast, lowpassBlend, 0.0, alpha);
        #else 
        return vec4(localContrast, 0.0, 0.0, alpha);    
        #endif
    #endif
    
    //-------------------------
    // 3. VERTICAL & HORIZONTAL EDGE TEST
    
    // Sample the additional diagonal neighbors.
    vec3 rgbNW = textureOffset(src, uv, vec2(-texel.x, -texel.y)).rgb; // NW
    vec3 rgbNE = textureOffset(src, uv, vec2(texel.x, -texel.y)).rgb; // NW
    vec3 rgbSW = textureOffset(src, uv, vec2(-texel.x, texel.y)).rgb; // SW
    vec3 rgbSE = textureOffset(src, uv, vec2(texel.x, texel.y)).rgb; // SE
    
    // Average additional neighbors when sub-pix aliasing is on and it isn't 
    // in 'fast' mode.
    #if FXAA_SUBPIX > 0
        #if FXAA_SUBPIX_FASTER == 0
            // Add missing neighbors and average them.
            rgbL += (rgbNW + rgbNE + rgbSW + rgbSE);  
            rgbL *= (1.0/9.0);
        #endif
    #endif
    
    // Calculate luma for additional neighbors.
    float lumaNW = FXAALuminance(rgbNW);
    float lumaNE = FXAALuminance(rgbNE);
    float lumaSW = FXAALuminance(rgbSW);
    float lumaSE = FXAALuminance(rgbSE);
    
    // Calculate the vertical and horizontal edges. (Uses algorithm from 
    // FXAA white paper).
    float edgeVert = FXAAVerticalEdge(lumaO, lumaN, lumaE, lumaS, lumaW, 
        lumaNW, lumaNE, lumaSW, lumaSE);
    float edgeHori = FXAAHorizontalEdge(lumaO, lumaN, lumaE, lumaS, lumaW, 
        lumaNW, lumaNE, lumaSW, lumaSE);
    
    // Check if edge is horizontal.
    bool isHorizontal = edgeHori >= edgeVert;
    
    #if FXAA_DEBUG_HORZVERT
    if(isHorizontal) 
    {
        return vec4(1.0, 0.75, 0.0, alpha);
    } 
    else 
    {
        return vec4(0.10, 0.10, 1.0, alpha);
    }
    #endif
    
    //-------------------------
    // 4. FIND HIGHEST CONTRAST PAIR 90deg TO EDGE
    
    // Contain the appropriate sign for the top left.
    // Note, if isHorizontal == true, -texel.y is applied (not -texel.x).
    float edgeSign = isHorizontal ? -texel.y : -texel.x; 
    
    // Calculate the gradients. The luma used changes based on the horizontal
    // edge status.
    float gradientNeg = isHorizontal ? abs(lumaN-lumaO) : abs(lumaW-lumaO);
    float gradientPos = isHorizontal ? abs(lumaS-lumaO) : abs(lumaE-lumaO); 
    
    // Calculate the luma based on its direction.
    // It is an average of the origin and the luma in the respective 
    // direction.
    float lumaNeg = isHorizontal ? ((lumaN+lumaO)*0.5) : ((lumaW+lumaO)*0.5);
    float lumaPos = isHorizontal ? ((lumaS+lumaO)*0.5) : ((lumaE+lumaO)*0.5);
    
    // Select the highest gradient pair.
    bool isNegative = (gradientNeg >= gradientPos);
    // Assign higher pair.
    float gradientHighest = isNegative ? gradientNeg : gradientPos; 
    float lumaHighest = isNegative ? lumaNeg : lumaPos;
    
    // If gradient pair in the negative direction is higher, flip the edge 
    // sign.
    if(isNegative) { edgeSign *= -1.0; }
    
    #if FXAA_DEBUG_PAIR
    return isHorizontal ? 
        vec4(0.0, gradientHighest, lumaHighest, alpha) : 
        vec4(0.0, lumaHighest, gradientHighest, alpha); 
    #endif
    
    //-------------------------
    // 5. END-OF-EDGE SEARCH
    
    // Select starting point.
    vec2 pointN = vec2(0.0, 0.0);
    pointN.x = uv.x + (isHorizontal ? 0.0 : edgeSign * 0.5);
    pointN.y = uv.y + (isHorizontal ? edgeSign * 0.5 : 0.0);
    
    // Assign search limiting values.
    gradientHighest *= FXAA_SEARCH_THRESHOLD;
    
    // Prepare variables for search.
    vec2 pointP = pointN; // Start at the same point.
    vec2 pointOffset = 
        isHorizontal ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);
    float lumaNegEnd = lumaNeg;
    float lumaPosEnd = lumaPos;
    bool searchNeg = false;
    bool searchPos = false;
    
    // Apply values based on FXAA flags.
    if(FXAA_SEARCH_ACCELERATION == 1) {
        
        pointN += pointOffset * vec2(-1.0);
        pointP += pointOffset * vec2(1.0);
        // pointOffset *= vec2(1.0);
        
    } else if(FXAA_SEARCH_ACCELERATION == 2) {    
        
        pointN += pointOffset * vec2(-1.5);
        pointP += pointOffset * vec2(1.5);
        pointOffset *= vec2(2.0);
        
    } else if(FXAA_SEARCH_ACCELERATION == 3) {  
        
        pointN += pointOffset * vec2(-2.0);
        pointP += pointOffset * vec2(2.0);
        pointOffset *= vec2(3.0);
        
    } else if(FXAA_SEARCH_ACCELERATION == 4) { 
        
        pointN += pointOffset * vec2(-2.5);
        pointP += pointOffset * vec2(2.5);
        pointOffset *= vec2(4.0);
        
    }
    
    // Perform the end-of-edge search.
    for(int i = 0; i < FXAA_SEARCH_STEPS; i++) 
    {
        if(FXAA_SEARCH_ACCELERATION == 1) 
        {
            if(!searchNeg) 
            {
                lumaNegEnd = FXAALuminance(texture(src, pointN).rgb);
            }
            if(!searchPos) 
            {
                lumaPosEnd = FXAALuminance(texture(src, pointP).rgb);
            }
        } 
        else
        {
            if(!searchNeg) 
            {
                lumaNegEnd = FXAALuminance(textureGrad(src, pointN, 
                    pointOffset, pointOffset).rgb);
            }
            if(!searchPos) 
            {
                lumaPosEnd = FXAALuminance(textureGrad(src, pointP, 
                    pointOffset, pointOffset).rgb);
            }
        }
        
        // Search for significant change in luma compared to current highest
        // pair.
        searchNeg = searchNeg || (abs(lumaNegEnd-lumaNeg) >= gradientNeg);
        searchPos = searchPos || (abs(lumaPosEnd-lumaPos) >= gradientPos);
        
        // Display debug information regarding edges.
        #if FXAA_DEBUG_NEGPOS
        if(searchNeg) { 
            return vec4(abs(lumaNegEnd - gradientNeg), 0.0, 0.0, alpha);
        } else if(searchPos) { 
            return vec4(0.0, 0.0, abs(lumaPosEnd - gradientPos), alpha);
        }
        #endif
        
        // Determine if search is over early.
        if(searchNeg && searchPos) { break; }
        
        // If still searching, increment offset.
        if(!searchNeg) { pointN -= pointOffset; }
        if(!searchPos) { pointP += pointOffset; }
    }
    
    //-------------------------
    // 6. SUB-PIXEL SHIFT
    
    // Determine if sub-pixel center falls on positive or negative side.
    float distanceNeg = isHorizontal ? uv.x - pointN.x : uv.y - pointN.y;
    float distancePos = isHorizontal ? pointP.x - uv.x : pointP.y - uv.y;
       bool isCloserToNegative = distanceNeg < distancePos;
    
    // Assign respective luma.
    float lumaEnd = isCloserToNegative ? lumaNegEnd : lumaPosEnd;
    
    // Check if pixel is in area that receives no filtering.
    if( ((lumaO - lumaNeg) < 0.0) == ((lumaEnd - lumaNeg) < 0.0))
    {
        edgeSign = 0.0;
    }
    
    // Compute sub-pixel offset and filter span.
    float filterSpanLength = (distancePos + distanceNeg);
    float filterDistance = isCloserToNegative ? distanceNeg : distancePos;
    float subpixelOffset = 
        (0.5 + (filterDistance * (-1.0/filterSpanLength)))*edgeSign;
    
    #if FXAA_DEBUG_OFFSET  
    if(subpixelOffset < 0.0) {
        // neg-horizontal (red) : neg-vertical (gold)
        return isHorizontal ? vec4(1.0, 0.0, 0.0, alpha) : 
            vec4(1.0, 0.7, 0.1, alpha);
    } 
    
    if(subpixelOffset > 0.0) {
        return isHorizontal ? vec4(0.0, 0.0, 1.0, alpha) : 
            vec4(0.1, 0.3, 1.0, alpha);
    }
    #endif
    
    // Resample using the subpixel offset
    vec3 rgbOffset = textureLod
    (
        src, 
        vec2
        ( 
            uv.x + (isHorizontal ? 0.0 : subpixelOffset),
            uv.y + (isHorizontal ? subpixelOffset : 0.0)
        ), 
        0.0
    ).rgb;
    
    #if FXAA_DEBUG_HIGHLIGHT
    return isHorizontal ? vec4(1.0, 0.0, 0.0, alpha) :
        vec4(0.0, 1.0, 0.0, alpha);
    #endif
    
    // Return the FXAA effect
    #if FXAA_SUBPIX == 0
    return vec4(rgbOffset, alpha);
    #else
    return vec4(mix(rgbOffset, rgbL, lowpassBlend), alpha);
    #endif
}

// ------------------------
// Main function.
// ------------------------

void main()
{
    fragColor = applyFXAA(target, targetResolution);
}
)"
        )

        CODE_ENTRY(
"Sobel filter",
"This complete shader applies a Sobel filter to an input texture 'target', "
"which needs to be added to the layer uniforms list via the 'Uniforms' tab. "
"Please note that the original texture is not modified, and that the Sobel-"
"filtered texture is the output of this shader.",
R"(void make_kernel(inout vec4 n[9], sampler2D src, vec2 uv)
{
    float w = 1.0/iTargetResolution.x;
    float h = 1.0/iTargetResolution.y;
    n[0] = texture(src, uv+vec2(-w, -h));
    n[1] = texture(src, uv+vec2(0.0, -h));
    n[2] = texture(src, uv+vec2(w, -h));
    n[3] = texture(src, uv+vec2(-w, 0.0));
    n[4] = texture(src, uv);
    n[5] = texture(src, uv+vec2(w, 0.0));
    n[6] = texture(src, uv+vec2(-w, h));
    n[7] = texture(src, uv+vec2(0.0, h));
    n[8] = texture(src, uv+vec2(w, h));
}

void main() 
{
    vec4 n[9];
    make_kernel(n, target, tc);
    vec4 sobel_edge_h = n[2]+(2.0*n[5])+n[8]-(n[0]+(2.0*n[3])+n[6]);
    vec4 sobel_edge_v = n[0]+(2.0*n[1])+n[2]-(n[6]+(2.0*n[7])+n[8]);
    vec4 sobel = sqrt((sobel_edge_h*sobel_edge_h)+(sobel_edge_v*sobel_edge_v));
    fragColor = vec4(sobel.rgb, 1.0);
})"
        )
        
        ImGui::TreePop();
    }

    //------------------------------------------------------------------------//
    if (ImGui::TreeNode("Ray marching")) //-----------------------------------//
    {
        //--------------------------------------------------------------------//
        if (ImGui::TreeNode("3D Signed Distance Fields (SDFs)"))
        {
            ImGui::Text(
"Please note that all of the following SDFs describe 3D primitives centered at "
"the origin of the reference frame");

            CODE_ENTRY(
"Infinite plane",
"Signed distance of 'p' from the surface of an infinite plane passing through "
"the origin withh surface normal 'n' and distance 'h' (along the surface "
"normal) from the origin",
R"(float infPlaneSDF(vec3 p, vec3 n, float h)
{
    return dot(p,normalize(n))+h;
})")

            CODE_ENTRY(
"Infinite cylinder",
"Signed distance of 'p' from the surface of an infinite cylinder of radius 'r' "
"along the y axis",
R"(float infCylinderSDF(vec3 p, float r)
{
    return length(p.xz)-r;
})")

            CODE_ENTRY(
"Infinite cone",
"Signed distance of 'p' from the surface of an infinite cone of aperature 'a' "
"(in radians) along the y axis, whose vertex is at the origin",
R"(float infConeSDF( vec3 p, float a )
{
    vec2 c = vec2(sin(a), cos(a));
    vec2 q = vec2(length(p.xz), -p.y);
    float d = length(q-c*max(dot(q,c), 0.0));
    return d*((q.x*c.y-q.y*c.x<0.0)?-1.0:1.0);
})")

            CODE_ENTRY(
"Infinite triangular column",
"Signed distance of 'p' from the surface of an infinite column along the y axis "
"with a regular-triangular cross-section, such that the inscribing circle has "
"radius 'r'",
R"(float infTriangleSDF(vec3 p, float r)
{
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.z = p.z + r/k;
    if( p.x+k*p.z>0.0 ) p.xz = vec2(p.x-k*p.z,-k*p.x-p.z)/2.0;
    p.x -= clamp( p.x, -2.0*r, 0.0 );
    return -length(p.xz)*sign(p.z);
})")

            CODE_ENTRY(
"Infinite rectangular column",
"Signed distance of 'p' from the surface of an infinite rectangular column "
"along the y axis, with side lengths 'lx', 'lz'",
R"(float infRectangleSDF(vec3 p, float lx, float lz)
{
    vec2 d = abs(p.xz)-vec2(lx, lz);
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
})")

            CODE_ENTRY(
"Infinite pentagonal column",
"Signed distance of 'p' from the surface of an infinite column along the y axis"
"with a regular-pentagonal cross-section, such that the inscribing circle has"
"radius 'r'",
R"(float infPentagonSDF(vec3 p, float r)
{
    const vec3 k = vec3(0.809016994,0.587785252,0.726542528);
    p.y = p.z;
    p.x = abs(p.x);
    p.xy -= 2.0*min(dot(vec2(-k.x,k.y),p.xy),0.0)*vec2(-k.x,k.y);
    p.xy -= 2.0*min(dot(vec2( k.x,k.y),p.xy),0.0)*vec2( k.x,k.y);
    p.xy -= vec2(clamp(p.x,-r*k.z,r*k.z),r);    
    return length(p.xy)*sign(p.y);
})")

            CODE_ENTRY(
"Infinite hexagonal column",
"Signed distance of 'p' from the surface of an infinite column along the y axis"
"with a regular-hexagonal cross-section, such that the inscribing circle has"
"radius 'r'",
R"(float infHexagonSDF(vec3 p, float r)
{
    const vec3 k = vec3(-0.866025404,0.5,0.577350269);
    p.y = p.z;
    p = abs(p);
    p.xy -= 2.0*min(dot(k.xy,p.xy),0.0)*k.xy;
    p.xy -= vec2(clamp(p.x, -k.z*r, k.z*r), r);
    return length(p.xy)*sign(p.y);
})")

            CODE_ENTRY(
"Infinite octagonal column",
"Signed distance of 'p' from the surface of an infinite column along the y axis"
"with a regular-octagonal cross-section, such that the inscribing circle has"
"radius 'r'",
R"(float infOctagonSDF(vec3 p, float r)
{
    const vec3 k = vec3(-0.9238795325, 0.3826834323, 0.4142135623);
    p.y = p.z;
    p = abs(p);
    p.xy -= 2.0*min(dot(vec2( k.x,k.y),p.xy),0.0)*vec2( k.x,k.y);
    p.xy -= 2.0*min(dot(vec2(-k.x,k.y),p.xy),0.0)*vec2(-k.x,k.y);
    p.xy -= vec2(clamp(p.x, -k.z*r, k.z*r), r);
    return length(p.xy)*sign(p.y);
})")

            CODE_ENTRY(
"Infinite N-side star column",
"Signed distance of 'p' from the surface of an infinite column along the y axis"
"with a regular-'n'-pointed-star cross-section, such that the inscribing "
"circle has radius 'r' and such that the arm thickness is controlled by "
"the 'm' parameter (should satisfy 2<'m'<'n'). For a regular star, 'm'='n'/2",
R"(float infNStarSDF(vec3 p, float r, int n, float m)
{
    p.y = p.z;
    float an = 3.141593/float(n);
    float en = 3.141593/m;
    vec2  acs = vec2(cos(an),sin(an));
    vec2  ecs = vec2(cos(en),sin(en));
    float bn = mod(atan(p.x,p.y),2.0*an) - an;
    p.xy = length(p.xy)*vec2(cos(bn),abs(sin(bn)));
    p.xy -= r*acs;
    p.xy += ecs*clamp(-dot(p.xy,ecs), 0.0, r*acs.y/ecs.y);
    return length(p.xy)*sign(p.x);
})")

            CODE_ENTRY(
"Sphere",
"Signed distance of 'p' from the surface of a sphere with radius 'r'",
R"(float sphereSDF(vec3 p, float r)
{
    return length(p)-r;
})")

            CODE_ENTRY(
"Torus",
"Signed distance of 'p' from the surface of a torus in the xy plane with outer "
"radius ro and inner radius ri",
R"(float torusSDF(vec3 p, float ro, float ri)
{
    vec2 q = vec2(length(p.xz)-ro,p.y);
    return length(q)-ri;
})")

            CODE_ENTRY(
"Rectangular cuboid",
"Signed distance of 'p' from the surface of a rectangular cuboid with side "
"lenghts lx, ly, lz",
R"(float rectCuboidSDF(vec3 p, float lx, float ly, float lz)
{
    vec3 q = abs(p)-vec(lx,ly,lz);
    return length(max(q,0.0))+min(max(q.x,max(q.y,q.z)),0.0);
})")

            CODE_ENTRY(
"Rectangular cuboid frame",
"Signed distance of 'p' from the surface of the frame of a rectangular cuboid "
"with side lenghts lx, ly, lz, with a square beam cross-section of side length"
" s",
R"(float frameSDF(vec3 p, float lx, float ly, float lz, float s)
{
    p = abs(p)-b;
    vec3 q = abs(p+l)-l;
    return min(min(
        length(max(vec3(p.x,q.y,q.z),0.0))+min(max(p.x,max(q.y,q.z)),0.0),
        length(max(vec3(q.x,p.y,q.z),0.0))+min(max(q.x,max(p.y,q.z)),0.0)),
        length(max(vec3(q.x,q.y,p.z),0.0))+min(max(q.x,max(q.y,p.z)),0.0));
})")

            CODE_ENTRY(
"Pyramid",
"Signed distance of 'p' from the surface of a square pydramid of base length "
"'s' and height 'h'",
R"(float pyramidSDF(vec3 p, float s, float h)
{
    p.y = (p.y+s)/h;
    float y0 = p.y;
    p = abs(p);
    float m = p.x+p.y+p.z-s;
    vec3 q;
    float d = 0;
    if(3.0*p.x < m) q = p.xyz;
    else if(3.0*p.y < m) q = p.yzx;
    else if(3.0*p.z < m) q = p.zxy;
    else d = m*0.57735027;
    float k = clamp(0.5*(q.z-q.y+s),0.0,s); 
    if (d == 0) d=length(vec3(q.x,q.y-s+k,q.z-k)); 
    return max(d, -y0);
})")

            CODE_ENTRY(
"Octahedron",
"Signed distance of 'p' from the surface of a regular octaheadron with edge "
"length s",
R"(float octahedronSDF(vec3 p, float s)
{
    p.y+=s;
    p = abs(p);
    float m = p.x+p.y+p.z-s;
    vec3 q;
    if(3.0*p.x < m) q = p.xyz;
    else if(3.0*p.y < m) q = p.yzx;
    else if(3.0*p.z < m) q = p.zxy;
    else return m*0.57735027;
    float k = clamp(0.5*(q.z-q.y+s),0.0,s);
    return length(vec3(q.x,q.y-s+k,q.z-k)); 
})")

            ImGui::TreePop();
        }

        //--------------------------------------------------------------------//
        if (ImGui::TreeNode("SDF Operations"))
        {
            CODE_ENTRY(
"Union",
"Unite two entities described by signed distance fields 'sdf1', 'sdf2'",
R"(float uniteSDFs(float sdf1, float sdf2)
{
    return min(sdf1, sdf2);
})")

            CODE_ENTRY(
"Subtraction",
"Subtract the second entity from the first, described by signed distance "
"fields 'sdf1', 'sdf2'",
R"(float subtractSDFs(float sdf1, float sdf2)
{
    return max(-sdf1, sdf2);
})")

            CODE_ENTRY(
"Intersection",
"Intersect two entities described by signed distance fields 'sdf1', 'sdf2'",
R"(float intersectSDFs(float sdf1, float sdf2)
{
    return max(sdf1, sdf2);
})")

            CODE_ENTRY(
"Smooth union",
"Unite two entities described by signed distance fields 'sdf1', 'sdf2' while "
"smoothing any hard edges resulting from the union, proportionally to 's' (0 "
"is no smoothing)",
R"(float smoothUniteSDFs(float sdf1, float sdf2, float s)
{
    float h = clamp(0.5+0.5*(sdf2-sdf1)/s, 0.0, 1.0);
    return mix(sdf2, sdf1, h)-s*h*(1.0-h);
})")

            CODE_ENTRY(
"Smooth subtraction",
"Subtract the second entity from the first, described by signed distance "
"fields 'sdf1', 'sdf2' while smoothing any hard edges resulting from the "
"subtraction, proportionally to 's' (0 is no smoothing)",
R"(float smoothSubtractSDFs(float sdf1, float sdf2, float s)
{
    float h = clamp(0.5-0.5*(sdf2+sdf1)/s, 0.0, 1.0);
    return mix(sdf2, -sdf1, h) + s*h*(1.0-h);
})")

            CODE_ENTRY(
"Smooth intersection",
"Intersect two entities described by signed distance fields 'sdf1', 'sdf2' "
"while smoothing any hard edges resulting from the union, proportionally to "
"'s' (0 is no smoothing)",
R"(float smoothIntersectSDFs(float sdf1, float sdf2, float s)
{
    float h = clamp(0.5-0.5*(sdf2-sdf1)/s, 0.0, 1.0);
    return mix(sdf2, sdf1, h)+s*h*(1.0-h);
})")

            ImGui::TreePop();
        }

        //--------------------------------------------------------------------//
        if (ImGui::TreeNode("Sampling"))
        {
            CODE_ENTRY(
"Camera ray",
"Assuming a camera located at 'cp' and pointed in direction 'f', this function "
"returns a camera ray cast from the current screen fragment at position 'qc' "
"which complies with the provided field-of-view factor 'fov'",
R"(vec3 cameraRay(vec2 qc, vec3 cp, vec3 f, float fov)
{
    vec3 l = normalize(cross(vec3(0,1,0),f));
    vec3 u = normalize(cross(f,l));
    return normalize(cp+f*fov-qc.x*l+qc.y*u-cp);
})")

            CODE_ENTRY(
"Forward-differencing surface normal",
"Simple calculation of a scene SDF surface normal at a point 'p' based on "
"forward differencing. The infinitesimal step 'h' can be either set as constant"
", or, to reduce artifacts, it can be set to be proportional to the distance of"
" 'p' from the camera",
R"(vec3 sceneNormal(vec3 p, float h)
{
    float d0 = sceneSDF(p);
    float ddx = sceneSDF(p+vec3(h,0,0))-d0;
    float ddy = sceneSDF(p+vec3(0,h,0))-d0;
    float ddz = sceneSDF(p+vec3(0,0,h))-d0;
    return normalize(vec3(ddx,ddy,ddz));
})")

            CODE_ENTRY(
"Four-point surface normal",
"Advanced 4-point-based calculation of a scene SDF surface normal at a point"
"'p'. The infinitesimal step 'h' can be either set as constant, or, to reduce"
"artifacts, it can be set to be proportional to the distance of 'p' from the"
"camera",
R"(vec3 sceneNormal(vec3 p, float h)
{
    vec2 e = vec2(1.0,-1.0);
    return normalize(e.xyy*sceneSDF(p+e.xyy*h)+
                     e.yyx*sceneSDF(p+e.yyx*h)+
                     e.yxy*sceneSDF(p+e.yxy*h)+
                     e.xxx*sceneSDF(p+e.xxx*h));
})")

            ImGui::TreePop();
        }

        //----------------------------------------------------------------------
        if (ImGui::TreeNode("Examples"))
        {
            CODE_ENTRY(
"Simple ray marcher",
"Full fragment shader code of a very bare-bones ray marcher with a test scene, "
"inclusive of a single light source and hard shadows cast by colorless scene "
"entities",
R"(#define PI 3.14159

// Increase this to avoid visual artifacts. However, at the same time, you
// should increase the number of ray marching steps
#define SAFETY_FACTOR 3

// Max number of ray marching steps
#define MAX_STEPS 500 

// Minimum distance from any scene surface below which ray marching stops
#define MIN_DIST 1e-3

// Maximum distance from any scene surface above which ray marching stops
#define MAX_DIST 1e3

// Given a point 'p', this function returns the distance between 'p' and the
// nearest geometry surface. It is this function that describes the 3D geometry
// of the scene
float sceneSDF(vec3 p)
{
    float sphere = length(p-vec3(0,1.5,0))-.75;
    sphere += .25*sin(5*p.x*p.y+iTime*2*PI);
    return min(sphere, p.y);
}

// Given a point 'p' close to the 3D geometry scene surface, this function
// returns the surface normal. The 'h' factor is the step used for the
// derivative calculation. If too low, visual artifacts will result, if too
// large, the resulting geometry will loose detail
vec3 sceneNormal(vec3 p, float h)
{
    vec2 e = vec2(1.0,-1.0);
    return normalize(e.xyy*sceneSDF(p+e.xyy*h)+
                     e.yyx*sceneSDF(p+e.yyx*h)+
                     e.yxy*sceneSDF(p+e.yxy*h)+
                     e.xxx*sceneSDF(p+e.xxx*h));
}

struct Ray
{
    vec3 origin;
    vec3 direction;
};

Ray cameraRay(vec2 uv, vec3 cp, vec3 f, float fov)
{
    vec3 l = normalize(cross(vec3(0,1,0),f));
    vec3 u = normalize(cross(f,l));
    return Ray(cp,normalize(cp+f*fov-uv.x*l+uv.y*u-cp));
}

// Propagate a ray 'r' along its direction until either a scene surface is 
// hit or the maximum number of steps has been traversed. Returs the distance
// to said hit point, otherwise returns 0
float rayMarch(Ray r)
{
    float s, ds = 0;
    for (int step=0; step<MAX_STEPS; step++)
    {
        ds = sceneSDF(r.origin+r.direction*s)/SAFETY_FACTOR;
        s += ds;
        if (ds < MIN_DIST && ds > 0)
            return s;
        if (s > MAX_DIST)
            return 0;
    }
    return s;
}

void main()
{
    // Define light source position and background color (used when no surface
    // hit or for shadrows
    const vec3 lightSource = vec3(2,4,2);
    const vec4 bckgColor = vec4(0,0,0,1);

    // Feel free to replace these with the iWASD, iLook
    // uniforms to have live control over the camera and move in shader-space!
    vec3 cameraPosition = vec3(1.5,3,3);
    vec3 cameraDirection = -normalize(vec3(1.5,1.5,3));
    
    // Construct ray for this pixel and ray march
    Ray r = cameraRay(qc,cameraPosition,cameraDirection,1);
    float d = rayMarch(r);
    if (d == 0) // If no surfaces hit
        fragColor = bckgColor; //set to background color
    else
    {
        // Find surface hit point and its distance from light source
        vec3 p = r.origin+r.direction*d*0.99;
        vec3 pl = lightSource-p;

        // Ray march from the hit point to the light source
        Ray rl = Ray(p, normalize(pl));
        float dl = rayMarch(rl);

        // If ray marching does not reach light source it means that the point
        // is in complete shade (hard shadow)
        if (dl>0 && dl<0.99*length(pl))
            fragColor = bckgColor;
        else 
        {
            // Compute pixel color if fully or partially illuminated
            vec3 n = sceneNormal(p, 1e-3*d); 
            float c = max(dot(n,normalize(lightSource-p)),0);
            fragColor = vec4(c,c,c,1);
        }
    }
})")

            CODE_ENTRY(
"Advanced ray marcher (with reflections)",
"Full fragment shader code of a more advanced ray marcher, featuring colorful "
"and reflective SDFs, as well as soft shadows cast by said entities from a "
"single light source. Please note that this code snipped is too long to be "
"viewed in its entirety, but it can be entirely copied to clipoboard",
R"(#define MAX_STEPS 250
#define MAX_DIST 1e2
#define MIN_DIST 1e-3
#define OFFSET (10*MIN_DIST)
#define SAFETY_FACTOR 1
#define N_REFLS 3
#define SHADW_SHARPN 10
#define MIN_LIGHT .025
#define TPIT 2.*PI*iTime
#define LIGHT_POS vec3(0.,20.,-20.)
#define BACKG_COL vec3(0.1, .25, .37)

vec3 rotate(vec3 v, float t, vec3 a)
{
    vec4 q = vec4(sin(t/2.0)*a.xyz, cos(t/2.0));
    return v + 2.0*cross(q.xyz, cross(q.xyz, v) + q.w*v);
}

float sphereSDF(vec3 p, float r)
{
    return length(p)-r;
}

float boxSDF(vec3 p, vec3 b)
{
    vec3 q = abs(p) - b;
    return length(max(q, 0.)) + min(max(q.x, max(q.y,q.z)),0.);
}

struct Material
{
    vec3 c; // Color
    float r; // Reflectance
};

struct SDM // Signed distance & material
{
    float d;
    Material m;
};

// All SDM operations for reference, even if not used, to showcase 
// examples for handling mixing of material properties

SDM intersect(SDM o0, SDM o1)
{
    return SDM
    (
        max(o0.d, o1.d), 
        Material((o0.m.c+o1.m.c)/2., (o0.m.r+o1.m.r)/2.)
    );
}

SDM subtract(SDM o0, SDM o1)
{
    return SDM(max(o0.d, -o1.d), o0.m);
}

SDM unite(SDM o0, SDM o1)
{
    if (o0.d < o1.d)
        return o0;
    return o1;
}

SDM smoothUnite(SDM o0, SDM o1, float s)
{
    float h = clamp(0.5+0.5*(o0.d-o1.d)/s, 0.0, 1.0);
    return SDM
    (
        mix(o0.d, o1.d, h)-s*h*(1.0-h), 
        Material
        (
            mix(o0.m.c, o1.m.c, h),
            mix(o0.m.r, o1.m.r, h)
        )
    );
}

// Subtract o0 from o1
SDM smoothSubtract(SDM o0, SDM o1, float s)
{
    float h = clamp(0.5-0.5*(o0.d+o1.d)/s, 0.0, 1.0);
    return SDM
    (
        mix(o1.d, -o0.d, h) + s*h*(1.0-h),
        o1.m
    );
}

SDM smoothIntersect(SDM o0, SDM o1, float s)
{
    float h = clamp(0.5-0.5*(o1.d-o0.d)/s, 0.0, 1.0);
    return SDM
    (
        mix(o1.d, o0.d, h)+s*h*(1.0-h),
        Material((o0.m.c+o1.m.c)/2., (o0.m.r+o1.m.r)/2.)
    );
}

SDM sceneSDF(vec3 p)
{
    // Rotating, orbiting box
    vec3 pb = 
        rotate
        (
            rotate
            (
                p-vec3(9*sin(iTime), 0, 9*cos(iTime)), 
                iTime, 
                vec3(0,1,0)
            ),
            iTime,
            vec3(1,0,0)
        );
    SDM bx = SDM
    (
        boxSDF(pb, vec3(2)), 
        Material(vec3(1.,0.,0.), 0.1)
    );
    // Sphere going up and down
    SDM sp = SDM
    (
        sphereSDF(p+vec3(0,3*sin(iTime),0), 3.), 
        Material(vec3(0.75,0.75,0.75), 0.5)
    );
    // Lower less-than-half-sphere thing
    SDM pl = SDM
    (
        max(p.y+2., sphereSDF(p, 10)),
        Material(vec3(0.5,0.25,0), 0.1)
    );
    sp = smoothUnite(sp, pl, 3);
    // Cut part of lower less-than-half-sphere thing based on 
    // extended cube outline
    sp.d = max(sp.d, -(bx.d-2.));
    return unite(sp,bx);
}

struct Ray 
{
    vec3 orig; // Ray origin
    vec3 dir; // Ray direction
    float af; // Attenuation factor
};

Ray newRay(vec3 orig, vec3 dir)
{
    return Ray(orig, dir, 1.);
}

Ray cameraRay(vec2 qc, vec3 cp, vec3 f, float fov)
{
    vec3 l = normalize(cross(vec3(0,1,0),f));
    vec3 u = normalize(cross(f,l));
    return newRay(cp, normalize(cp+f*fov-qc.x*l+qc.y*u-cp));
}

// Just like the regular rayMarch, but also computes a penumbra
// factor for smooth shadows
SDM rayMarch(Ray ray, out float pn)
{
    SDM sdm;
    float d, dd, dd0 = MIN_DIST;
    pn = 1.;
    for (int iter = 0; iter < MAX_STEPS; iter++)
    {
        dd0 = dd;
        sdm = sceneSDF(ray.orig + ray.dir*d);
        dd = sdm.d/SAFETY_FACTOR;
        pn = min(pn, SHADW_SHARPN*sdm.d/d);
        if (dd > 0 && dd <= MIN_DIST || d >= MAX_DIST)
            break;
        d += dd;
    }
    sdm.d = d;
    return sdm;
}

// Ray march without the penumbra factor
SDM rayMarch(Ray ray)
{
    float pn;
    return rayMarch(ray, pn);
}

vec3 getNormal(vec3 p)
{
    float d = sceneSDF(p).d;
    vec2 e = vec2(OFFSET, 0.);
    vec3 n = d - vec3(
        sceneSDF(p-e.xyy).d,
        sceneSDF(p-e.yxy).d,
        sceneSDF(p-e.yyx).d
    );
    return normalize(n);
}

float getLighting(vec3 p, inout vec3 n)
{
    // Light source position
    n = getNormal(p);
    p += n*OFFSET;
    vec3 dir = normalize(LIGHT_POS-p);
    float pn;
    if (rayMarch(newRay(p,dir),pn).d<length(LIGHT_POS-p)-OFFSET)
        return MIN_LIGHT;
    return max(dot(n, dir)*pn, MIN_LIGHT);
}

// Color from a single ray propagation
void raySubColor(inout Ray ray, inout vec3 col, int iter)
{
    // If the ray is fully attenuated, stop propagating
    if (ray.af == 0.)
        return;
    SDM sdm = rayMarch(ray);
    vec3 p = ray.orig + ray.dir*sdm.d;
    vec3 lCol, n;
    lCol = BACKG_COL;
    if (sdm.d < MAX_DIST)
        lCol = max(getLighting(p, n), 0.)*sdm.m.c;
    // If last reflection, set reflectance of hit 
    // surface to 0 so it is rendered as fully opaque
    sdm.m.r *= float(iter<N_REFLS && sdm.d < MAX_DIST);
    col = (1-ray.af)*col+lCol*ray.af*(1.0-(sdm.m.r));
    ray.af *= sdm.m.r;
    if (ray.af != 0.)
    {
        ray.orig = p+n*OFFSET;
        ray.dir = reflect(ray.dir, n);
    }
}

vec3 rayColor(Ray ray)
{
    vec3 col = vec3(0.);
    // Base pass
    raySubColor(ray, col, 0);
    // Reflection passes
    for (int i = 0; i < N_REFLS; i++)
    {
        raySubColor(ray, col, i+1);
    }
    // Gamma correction and return
    return pow(col, vec3(1./2.2));
}

void main(void)
{
    // Camera position, look-direction, ray and its final color
    vec3 cp = vec3(-15., 12., -15.); // iWASD
    vec3 ld = normalize(vec3(0.,-1.,0.)-cp); // iLook
    Ray ray = cameraRay(qc, cp, ld, 1.);
    fragColor = vec4(rayColor(ray), 1.);
})"
            )

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    //------------------------------------------------------------------------//
    if (ImGui::TreeNode("Miscellaneous")) //----------------------------------//
    {
        //--------------------------------------------------------------------//
        if (ImGui::TreeNode("Vector rotation"))
        {
            CODE_ENTRY(
"2D Rotation",
"Counter-clock-wise rotation of a 2D vector 'v' by a given angle 't' (in "
"radians)",
R"(vec2 rotate(vec2 v, float t)
{
    float s = sin(t);
    float c = cos(t);
    return mat2(c, s, -s, c)*v;
})")

            CODE_ENTRY(
"3D Rotation",
"Counter-clock-wise rotation of a 3D vector 'v' by a given angle 't' (in "
"radians) around a given axis 'a'. This implementation leverages quaternions",
R"(vec3 rotate(vec3 v, float t, vec3 a)
{
    vec4 q = vec4(sin(t/2.0)*a.xyz, cos(t/2.0));
    return v + 2.0*cross(q.xyz, cross(q.xyz, v) + q.w*v);
})")
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    ImGui::PopTextWrapPos();
    if (!isGuiInMenu_)
        ImGui::End();
}

}