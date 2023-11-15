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
"Returns a pseudo-random float representative of a single octave of Perlin-like"
"noise at 3D coordinates 'x'. A 'random3D' function should be defined as well, "
"e.g., the one provided in Code repository -> Noise -> 3D -> Pseudo-random number generator",
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

    //------------------------------------------------------------------------//
    if (ImGui::TreeNode("Effects"))
    {
        CODE_ENTRY(
"Sobel filter",
"This complete shader applies a Sobel filter to an input texture 'iTexture0', "
"which needs to be added to the uniforms list via the 'Uniforms' tab",
R"(void make_kernel(inout vec4 n[9], sampler2D tex, vec2 coord)
{
    float w = 1.0/iResolution.x;
    float h = 1.0/iResolution.y;
    n[0] = texture(tex, coord+vec2(-w, -h));
    n[1] = texture(tex, coord+vec2(0.0, -h));
    n[2] = texture(tex, coord+vec2(w, -h));
    n[3] = texture(tex, coord+vec2(-w, 0.0));
    n[4] = texture(tex, coord);
    n[5] = texture(tex, coord+vec2(w, 0.0));
    n[6] = texture(tex, coord+vec2(-w, h));
    n[7] = texture(tex, coord+vec2(0.0, h));
    n[8] = texture(tex, coord+vec2(w, h));
}

void main() 
{
    vec4 n[9];
    make_kernel(n, iTexture0, tc);
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
"entities ",
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
"single light source",
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

    ImGui::PopTextWrapPos();
    if (!isGuiInMenu_)
        ImGui::End();
}

}