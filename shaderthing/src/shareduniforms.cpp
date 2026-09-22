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

#include "vir/include/vir.h"

#include "shaderthing/include/app.h"
#include "shaderthing/include/shareduniforms.h"

namespace ShaderThing
{

void SharedUniforms::initialize
(
    RenderState& renderState, 
    ExportState& exportState
)
{
    // Init CPU block data
    auto window = vir::Window::instance();
    if (!window->iconified())
       iResolution = {window->width(), window->height()};
    iAspectRatio = iResolution.x/iResolution.y;
    for (int i=0; i<256; i++)
        iKeyboard[i] = glm::ivec3({0,0,0});

    // Init cameras
    screenCamera = vir::makeUnique<vir::Camera>();
    shaderCamera = vir::makeUnique<vir::InputCamera>();
    screenCamera->setProjectionType
    (
        vir::Camera::ProjectionType::Orthographic
    );
    screenCamera->setViewportHeight
    (
        std::min(1.0f, 1.0f/iAspectRatio)
    );
    screenCamera->setPosition({0, 0, 1});
    screenCamera->setPlanes(.01f, 100.f);
    shaderCamera->setZPlusIsLookDirection(true);
    shaderCamera->setDirection(iLook);
    shaderCamera->setPosition(iWASD);
    screenCamera->update();
    shaderCamera->update();

    // Init random random number
    iRandom = std::uniform_real_distribution<float>(0, 1)(rndGenerator);

    // Init uniform buffers, bind to designated binding points and set
    // initial data
    vertex.uniformBuffer = 
            vir::DynamicUniformBuffer::create(64, "vertexSharedUniformBlock");
    vertex.uniformBuffer->bind();
    vertex.uniformBufferBindingPoint = 1;
    vertex.uniformBuffer->setBindingPoint
    (
        vertex.uniformBufferBindingPoint
    );

    iMVPUniform = Uniform::create(&vertex).getWeak();
    iMVPUniform->name() = "iMVP";
    iMVPUniform->setValuePtr
    (
        &(screenCamera->projectionViewMatrix()), 
        Uniform::Type::Mat4
    );
    iMVPUniform->gui.showBounds = false;

    fragment.uniformBuffer = 
            vir::DynamicUniformBuffer::create(8196, "sharedUniformBlock");
    fragment.uniformBuffer->bind();
    fragment.uniformBufferBindingPoint = 0;
    fragment.uniformBuffer->setBindingPoint
    (
        fragment.uniformBufferBindingPoint
    );

    // Init uniform wrappers
    iFrameUniform = Uniform::create(&fragment).getWeak();
    iFrameUniform->name() = "iFrame";
    iFrameUniform->setValuePtr
    (
        &renderState.frameIndex, 
        Uniform::Type::Int
    );
    iFrameUniform->gui.showBounds = false;

    iRenderPassUniform = Uniform::create(&fragment).getWeak();
    iRenderPassUniform->name() = "iRenderPass";
    iRenderPassUniform->setValuePtr
    (
        &renderState.passIndex, 
        Uniform::Type::Int
    );
    iRenderPassUniform->gui.showBounds = false;
    
    iTimeUniform = Uniform::create(&fragment).getWeak();
    iTimeUniform->name() = "iTime";
    iTimeUniform->setValuePtr(&iTime, Uniform::Type::Float);
    
    iTimeDeltaUniform = Uniform::create(&fragment).getWeak();
    iTimeDeltaUniform->name() = "iTimeDelta";
    iTimeDeltaUniform->setValuePtr(&iTimeDelta, Uniform::Type::Float);
    iTimeDeltaUniform->gui.showBounds = false;

    iRandomUniform = Uniform::create(&fragment).getWeak();
    iRandomUniform->name() = "iRandom";
    iRandomUniform->setValuePtr(&iRandom, Uniform::Type::Float);
    iRandomUniform->gui.showBounds = false;

    iUserActionUniform = Uniform::create(&fragment).getWeak();
    iUserActionUniform->name() = "iUserAction";
    iUserActionUniform->setValuePtr(&iUserAction, Uniform::Type::Bool);
    iUserActionUniform->gui.showBounds = false;

    iExportUniform = Uniform::create(&fragment).getWeak();
    iExportUniform->name() = "iExport";
    iExportUniform->setValuePtr
    (
        &exportState.isActive, 
        Uniform::Type::Bool
    );
    iExportUniform->gui.showBounds = false;

    iWASDUniform = Uniform::create(&fragment).getWeak();
    iWASDUniform->name() = "iWASD";
    iWASDUniform->setValuePtr(&iWASD, Uniform::Type::Float3);

    iLookUniform = Uniform::create(&fragment).getWeak();
    iLookUniform->name() = "iLook";
    iLookUniform->setValuePtr(&iLook, Uniform::Type::Float3);
    iLookUniform->gui.showBounds = false;

    iMouseUniform = Uniform::create(&fragment).getWeak();
    iMouseUniform->name() = "iMouse";
    iMouseUniform->setValuePtr(&iMouse, Uniform::Type::Float4);
    iMouseUniform->gui.showBounds = false;

    iAspectRatioUniform = Uniform::create(&fragment).getWeak();
    iAspectRatioUniform->name() = "iWindowAspectRatio";
    iAspectRatioUniform->setValuePtr(&iAspectRatio, Uniform::Type::Float);
    iAspectRatioUniform->gui.showBounds = false;

    iResolutionUniform = Uniform::create(&fragment).getWeak();
    iResolutionUniform->name() = "iWindowResolution";
    iResolutionUniform->setValuePtr(&iResolution, Uniform::Type::Float2);
    iResolutionUniform->gui.showBounds = false;

    iKeyboardUniform = Uniform::create(&fragment).getWeak();
    iKeyboardUniform->name() = "iKeyboard";
    iKeyboardUniform->setValuePtr(&iKeyboard, Uniform::Type::Int3, 256);
    iKeyboardUniform->gui.showBounds = false;
}

}