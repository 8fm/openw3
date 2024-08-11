/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

// In release or no-opts, use DebugOpt vs just Release
// #if !defined( _DEBUG ) && !defined( RED_FINAL_BUILD)
// # ifndef _DEBUGOPT
// #  define _DEBUGOPT
// # endif
// #endif

// TBD: SF_BUILD_DEBUGOPT
#ifdef RED_FINAL_BUILD
# define SF_BUILD_SHIPPING
#endif

#ifdef RED_PLATFORM_DURANGO
# define SF_OS_WINMETRO
# define SF_DURANGO_MONOLITHIC
#endif

#include <GFxConfig.h>

#include "../../common/engine/scaleformHacks.h"
#include "../../common/engine/scaleformTypes.h"

namespace SF = Scaleform;
namespace GFx = SF::GFx;

//// For now until GpuApi
//#ifdef RED_PLATFORM_ORBIS
//# define DEBUG_USE_GFX_REFERENCE_HAL
//#endif

#ifdef DEBUG_USE_GFX_REFERENCE_HAL
# if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
#  define USE_GFX_D3D1x
#  define SF_D3D_VERSION 11
#  include <GFx_Renderer_D3D1x.h>
# elif defined( RED_PLATFORM_ORBIS )
// Looks like SF forgot to add a <GFx_Render_Orbis.h>
#  include <Render/PS4/PS4_HAL.h>
#  include <Render/PS4/PS4_Texture.h>
# else
#  error Unsupported platform!
# endif
#endif

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
 // Defines for lexical substitution (vs namespace aliases,consts, etc) because I don't want to drag in SF D3D1x implementation crap
 // for stuff that'll going away soon.
# define ScaleformGpuApi SF::Render::D3D1x
# define SCALEFORM_GPUAPI_SHADER_VERSION ScaleformGpuApi::ShaderDesc::ShaderVersion_D3D1xFL11X
#elif defined( RED_PLATFORM_ORBIS )
# define ScaleformGpuApi SF::Render::PS4
# define SCALEFORM_GPUAPI_SHADER_VERSION ScaleformGpuApi::ShaderDesc::ShaderVersion_PS4
#else
# error "Scaleform not supported!"
#endif

#include "../../common/engine/scaleformUtils.inl"
