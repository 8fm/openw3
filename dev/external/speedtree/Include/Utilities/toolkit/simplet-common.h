/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef SIMPLET_COMMON_H
#define SIMPLET_COMMON_H

#include <gnmx.h>
#include "toolkit/toolkit.h"
#include <stdio.h>
#ifdef MEASURE_MODE
#include <gnm/measuredrawcommandbuffer.h>
#include <pm4_dump.h>
#endif

#define SAMPLE_FRAMEWORK_FSROOT "/app0"

namespace SimpletUtil
{
	void registerRenderTargetForDisplay(sce::Gnm::RenderTarget *renderTarget);
	void requestFlipAndWait();

	bool handleUserEvents();			// return true if the exit request has been received.

	void Init(uint32_t garlicSize = 1024 * 1024 * 512, uint32_t onionSize = 1024 * 1024 * 64);

	void *allocateMemory(uint32_t size, sce::Gnm::AlignmentType align);
	void *allocateMemory(sce::Gnm::SizeAlign sizeAlign);

	void *allocateGarlicMemory(sce::Gnm::SizeAlign sizeAlign);
	void *allocateOnionMemory(sce::Gnm::SizeAlign sizeAlign);

	sce::Gnmx::Toolkit::IAllocator getGarlicAllocator();
	sce::Gnmx::Toolkit::IAllocator getOnionAllocator();

	int32_t SaveTextureToTGA(const sce::Gnm::Texture *texture, const char* tgaFileName);
	int32_t SaveDepthTargetToTGA(const sce::Gnm::DepthRenderTarget *target, const char* tgaFileName);

	void AcquireFileContents(void*& pointer, uint32_t& bytes, const char* filename);
	void ReleaseFileContents(void* pointer);

	const char *getAssetFileDirectory();

	sce::Gnmx::VsShader* LoadVsShaderFromMemory(const void *shaderImage);
	sce::Gnmx::PsShader* LoadPsShaderFromMemory(const void *shaderImage);
	sce::Gnmx::CsShader* LoadCsShaderFromMemory(const void *shaderImage);
	sce::Gnmx::HsShader* LoadHsShaderFromMemory(const void *shaderImage);
	sce::Gnmx::EsShader* LoadEsShaderFromMemory(const void *shaderImage);
	sce::Gnmx::LsShader* LoadLsShaderFromMemory(const void *shaderImage);
	sce::Gnmx::GsShader* LoadGsShaderFromMemory(const void *shaderImage);
	sce::Gnmx::CsVsShader* LoadCsVsShaderFromMemory(const void *shaderImage);
}

#endif // SIMPLET_COMMON_H
