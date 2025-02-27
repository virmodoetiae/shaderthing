/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

namespace ByteData
{

namespace Font
{
    
extern const unsigned int ProggyCleanSize;
extern const unsigned int ProggyCleanData[9584/4];

extern const unsigned int CousineRegularSize;
extern const unsigned int CousineRegularData[230180/4];

extern const unsigned int DroidSansFallbackSize;
extern const unsigned int DroidSansFallbackData[2335648/4];

extern const unsigned int FontAwesome5FreeSolid900Size;
extern const unsigned int FontAwesome5FreeSolid900Data[141008/4];

} // End of Font

namespace Icon
{
    
extern const unsigned int hotDogIconSize;
extern const unsigned char hotDogIconData[7299];

extern const unsigned int sTIconSize;
extern const unsigned char sTIconData[10039];

} // End of Icon

namespace Image
{

extern const unsigned int virmodoetiaeSize;
extern const unsigned char virmodoetiaeData[422615];

} // End of Image

} // End of ByteData

}