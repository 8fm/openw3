///////////////////////////////////////////////////////////////////////  
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com

#pragma once

#if defined(SPEEDTREE_OPENGL)
	#include "Renderers/OpenGL/OpenGLRenderer.h"
#elif defined (SPEEDTREE_DIRECTX9)
	#include "Renderers/DirectX9/DirectX9Renderer.h"
#elif defined (SPEEDTREE_DIRECTX11)
	#include "Renderers/DirectX11/DirectX11Renderer.h"
#elif defined (_XBOX)
	#include "Renderers/360/360Renderer.h"
#elif defined (_DURANGO)
	#include "Renderers/Durango/DurangoRenderer.h"
#elif defined (__ORBIS__)
	#include "Renderers/Orbis/OrbisRenderer.h"
#elif defined (__CELLOS_LV2__)
	#include "Renderers/PS3/PS3Renderer.h"
#elif defined (__psp2__)
	#include "Renderers/PSP2/PSP2Renderer.h"
#else
	#error There doesn't seem to be a renderer for the platform you're compiling to
#endif


///////////////////////////////////////////////////////////////////////
//  OpenMP settings

#ifdef _OPENMP
	#define SPEEDTREE_OPENMP
	#define SPEEDTREE_OPENMP_MAX_THREADS 4
    #include <omp.h>
#else
	#define SPEEDTREE_OPENMP_MAX_THREADS 1
#endif
