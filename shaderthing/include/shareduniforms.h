/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <random>

#include "shaderthing/include/typedefs.h"
#include "shaderthing/include/uniform.h"

// Forwards

namespace vir
{
class TiledQuad;
class Shader;
class Camera;
}

namespace ShaderThing
{

class RenderState;
class ExportState;

struct SharedUniforms
{
    // Fixed camera used to retrieve the value of the projection view 
    // matrix iMVP
    UPtr<vir::Camera>        screenCamera;
    // Movable camera which responds to keyboard and mouse controls and is
    // used to provide values to iWASD, iLook uniforms
    UPtr<vir::Camera>        shaderCamera;
    // Random number generator
    std::mt19937_64          rndGenerator;
    
    float      iTime          = 0.f;
    float      iTimeDelta     = 0.f;
    float      iRandom        = 0.f;
    bool       iUserAction    = false;
    glm::vec3  iWASD          = {0,0,-1};
    glm::vec3  iLook          = {0,0,1};
    glm::vec4  iMouse         = {0,0,0,0};
    float      iAspectRatio   = 1.f;
    glm::vec2  iResolution    = {512,512};
    glm::ivec3 iKeyboard[256] = {};

    UniformContainer fragment;
    WPtr<Uniform> iFrameUniform;
    WPtr<Uniform> iRenderPassUniform;
    WPtr<Uniform> iTimeUniform;
    WPtr<Uniform> iTimeDeltaUniform;
    WPtr<Uniform> iRandomUniform;
    WPtr<Uniform> iUserActionUniform;
    WPtr<Uniform> iExportUniform;
    WPtr<Uniform> iWASDUniform;
    WPtr<Uniform> iLookUniform;
    WPtr<Uniform> iMouseUniform;
    WPtr<Uniform> iAspectRatioUniform;
    WPtr<Uniform> iResolutionUniform;
    WPtr<Uniform> iKeyboardUniform;

    UniformContainer vertex;
    WPtr<Uniform> iMVPUniform;

    const unsigned int userUniformsStartIndex = 13;

    bool isTimePaused                       = false;
    bool isTimePausedBecauseRenderingPaused = false;
    bool isTimeLooped                       = false;
    bool isTimeResetOnFrameCounterReset     = true;
    bool isTimeDeltaSmooth                  = false;
    bool isRandomNumberGeneratorPaused      = false;
    bool isKeyboardInputEnabled             = true; // iKeyboard
    bool isMouseInputEnabled                = true; // iMouse
    bool isMouseInputClampedToWindow        = false;
    bool mouseInputRequiresLMBHold          = true;
    bool isCameraKeyboardInputEnabled       = true; // iWASD
    bool isCameraMouseInputEnabled          = true; // iLook
    bool cameraMouseInputRequiresLMBHold    = true;

    struct Toggles
    {
        bool updateDataRangeII                  = false;
        bool stepToNextTimeStep                 = false;
    };
    Toggles toggles = {};

    void initialize(RenderState& renderState, ExportState& exportState);
};

}