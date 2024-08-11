///////////////////////////////////////////////////////////////////////
//  OrbisRenderer.cpp
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


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#include "Renderers/Orbis/OrbisRenderer.h"
#include <kernel.h>
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//	Static member variables

sce::Gnmx::GfxContext* Orbis::m_pContext = NULL;
sce::Gnm::RenderTarget* Orbis::m_pRenderTarget = NULL;
sce::Gnm::DepthRenderTarget* Orbis::m_pDepthRenderTarget = NULL;

template<> CShaderTechnique::CVertexShaderCache* CShaderTechnique::m_pVertexShaderCache = NULL;
template<> CShaderTechnique::CPixelShaderCache* CShaderTechnique::m_pPixelShaderCache = NULL;
template<> CTexture::CTextureCache* CTexture::m_pCache = NULL;
template<> CStateBlock::CStateBlockCache* CStateBlock::m_pCache = NULL;

template<> CTexture CRenderState::m_atLastBoundTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
template<> CTexture CRenderState::m_atFallbackTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
template<> st_int32 CRenderState::m_nFallbackTextureRefCount = 0;

template<> CShaderConstantBuffer CForestRender::m_cFrameConstantBuffer = CShaderConstantBuffer( );
template<> SFrameCBLayout CForestRender::m_sFrameConstantBufferLayout = SFrameCBLayout( );

// memory management
struct SMemoryBlock
{
	SMemoryBlock(off_t uiStart = 0, off_t uiEnd = 0) :
		m_uiStart(uiStart),
		m_uiEnd(uiEnd)
	{
	}

	bool operator<(const SMemoryBlock& sR) const
	{
		return (m_uiStart < sR.m_uiStart);
	}

	off_t m_uiStart;
	off_t m_uiEnd;
};

static const unsigned int g_uiSystemMemorySize = 2u * 1024 * 1024 * 1024; // 2GB
static const unsigned int g_uiGPUMemorySize = 2u * 1024 * 1024 * 1024; // 1GB
static const unsigned int g_uiBigBlockAlignment = 1024 * 1024; // 1MB
static off_t g_uiSystemMemoryStart = -1;
static off_t g_uiGPUMemoryStart = -1;

static CArray<SMemoryBlock, false> g_aSystemMemoryBlocks;
static CArray<SMemoryBlock, false> g_aGPUMemoryBlocks;

static CMap<void*, SMemoryBlock, CLess<void*>, false> g_mapUsedSystemBlocks;
static CMap<void*, SMemoryBlock, CLess<void*>, false> g_mapUsedGPUBlocks;


///////////////////////////////////////////////////////////////////////
//	Orbis::SetMainRenderTargets

void Orbis::SetMainRenderTargets(sce::Gnm::RenderTarget* pRenderTarget, sce::Gnm::DepthRenderTarget* pDepthRenderTarget)	
{ 
	m_pRenderTarget = pRenderTarget; 
	m_pDepthRenderTarget = pDepthRenderTarget; 
}


///////////////////////////////////////////////////////////////////////
//	Orbis::BindMainRenderTargets

void Orbis::BindMainRenderTargets(void)
{ 
	m_pContext->setRenderTarget(0, m_pRenderTarget); 
	m_pContext->setDepthRenderTarget(m_pDepthRenderTarget);
	//m_pContext->setRenderTargetMask(0xF);

	for (st_int32 i = 1; i < 8; ++i)
	{
		Orbis::Context( )->setRenderTarget(i, NULL);
	}

	m_pContext->setupScreenViewport(0, 0, m_pRenderTarget->getWidth( ), m_pRenderTarget->getHeight( ), 0.5f, 0.5f);
}


///////////////////////////////////////////////////////////////////////
//	Orbis::Initialize

void Orbis::Initialize(void)
{
	off_t uiMapped = 0;

	// allocate system memory
	sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE, g_uiSystemMemorySize, g_uiBigBlockAlignment, SCE_KERNEL_WB_ONION, &g_uiSystemMemoryStart);
	sceKernelMapDirectMemory((void**)&uiMapped, g_uiSystemMemorySize, SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE | SCE_KERNEL_PROT_GPU_ALL, 0, g_uiSystemMemoryStart, g_uiBigBlockAlignment);
	g_aSystemMemoryBlocks.push_back(SMemoryBlock(uiMapped, uiMapped + g_uiSystemMemorySize));

	// allocate gpu memory
	sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE, g_uiGPUMemorySize, g_uiBigBlockAlignment, SCE_KERNEL_WC_GARLIC, &g_uiGPUMemoryStart);
	sceKernelMapDirectMemory((void**)&uiMapped, g_uiGPUMemorySize, SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE | SCE_KERNEL_PROT_GPU_ALL, 0, g_uiGPUMemoryStart, g_uiBigBlockAlignment);
	g_aGPUMemoryBlocks.push_back(SMemoryBlock(uiMapped, uiMapped + g_uiSystemMemorySize));
}


///////////////////////////////////////////////////////////////////////
//	Orbis::Release

void Orbis::Finalize(void)
{
	unsigned int uiNumSystemBlocks = g_mapUsedSystemBlocks.size( );
	if (uiNumSystemBlocks > 0)
		printf("SpeedTree::Orbis::Finalize() - %d allocated system blocks are still in use.\n", uiNumSystemBlocks);

	unsigned int uiNumGPUBlocks = g_mapUsedGPUBlocks.size( );
	if (uiNumGPUBlocks > 0)
		printf("SpeedTree::Orbis::Finalize() - %d allocated GPU blocks are still in use.\n\n", uiNumGPUBlocks);

	g_aSystemMemoryBlocks.clear( );
	g_aGPUMemoryBlocks.clear( );
	g_mapUsedSystemBlocks.clear( );
	g_mapUsedGPUBlocks.clear( );

	if (g_uiSystemMemoryStart != -1)
	{
		sceKernelReleaseDirectMemory(g_uiSystemMemoryStart, g_uiSystemMemorySize);
		g_uiSystemMemoryStart = -1;
	}

	if (g_uiGPUMemoryStart != -1)
	{
		sceKernelReleaseDirectMemory(g_uiGPUMemoryStart, g_uiGPUMemorySize);
		g_uiGPUMemoryStart = -1;
	}
}


///////////////////////////////////////////////////////////////////////
//	Orbis::RoundUpToAlignment

off_t Orbis::RoundUpToAlignment(off_t uiSize, off_t uiAlignment)
{
	const off_t uiMask = uiAlignment - 1;
	return (uiSize + uiMask) & ~uiMask;
}


///////////////////////////////////////////////////////////////////////
//	Orbis::Allocate

void* Orbis::Allocate(unsigned int uiSize, unsigned int uiAlignment, bool bGPU)
{
	void* pReturn = NULL;

	CArray<SMemoryBlock, false>* pBlocks = bGPU ? &g_aGPUMemoryBlocks : &g_aSystemMemoryBlocks;
	CMap<void*, SMemoryBlock, CLess<void*>, false>* pUsedBlocks = bGPU ? &g_mapUsedGPUBlocks : &g_mapUsedSystemBlocks;

	// find big enough block
	for (int i = pBlocks->size( ) - 1; i > -1; --i)
	{
		SMemoryBlock* pBlock = &pBlocks->at(i);

		off_t uiStart = RoundUpToAlignment(pBlock->m_uiStart, uiAlignment);
		off_t uiEnd = uiStart + uiSize;

		if (pBlock->m_uiEnd >= uiEnd)
		{
			pReturn = (void*)uiStart;

			SMemoryBlock sUsedBlock(pBlock->m_uiStart, uiEnd);
			(*pUsedBlocks)[pReturn] = sUsedBlock;

			if (uiEnd == pBlock->m_uiEnd)
			{
				// used whole block, delete it
				pBlocks->erase(pBlocks->begin( ) + i);
			}
			else
			{
				// fixup block to represent used
				pBlock->m_uiStart = uiEnd;
			}

			break;
		}
	}

	st_assert(pReturn != NULL, "Orbis::Allocate() - Could not find available memory block for allocation");

	return pReturn;
}


///////////////////////////////////////////////////////////////////////
//	Orbis::Release

void Orbis::Release(void* pLocation)
{
	if (pLocation == NULL)
		return;

	bool bGPU = false;

	CMap<void*, SMemoryBlock, CLess<void*>, false>::iterator iterFind = g_mapUsedGPUBlocks.find(pLocation);
	if (iterFind != g_mapUsedGPUBlocks.end( ))
	{
		bGPU = true;
	}
	else
	{
		iterFind = g_mapUsedSystemBlocks.find(pLocation);
		if (iterFind == g_mapUsedSystemBlocks.end( ))
		{
			// couldn't find a record of it
			st_assert(false, "Orbis::Release() - Could not find previously allocated block");
			return;
		}
	}

	CArray<SMemoryBlock, false>* pBlocks = bGPU ? &g_aGPUMemoryBlocks : &g_aSystemMemoryBlocks;
	CMap<void*, SMemoryBlock, CLess<void*>, false>* pUsedBlocks = bGPU ? &g_mapUsedGPUBlocks : &g_mapUsedSystemBlocks;

	pBlocks->insert_sorted(iterFind->second);
	pUsedBlocks->erase(iterFind);

	// merge blocks if possible
	for (unsigned int i = 1; i < pBlocks->size( ); )
	{
		SMemoryBlock* pBlock = &pBlocks->at(i);
		SMemoryBlock* pPreviousBlock = &pBlocks->at(i - 1);

		if (pBlock->m_uiStart == pPreviousBlock->m_uiEnd)
		{
			pPreviousBlock->m_uiEnd = pBlock->m_uiEnd;
			pBlocks->erase(pBlocks->begin( ) + i);
		}
		else
		{
			++i;
		}
	}
}


