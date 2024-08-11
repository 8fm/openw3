///////////////////////////////////////////////////////////////////////
//  Texture.cpp
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
#include "Core/PerlinNoiseKernel.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::Load

st_bool CTextureOrbis::Load(const char* pFilename, st_int32 nMaxAnisotropy)
{
	st_bool bSuccess = false;
	
	ReleaseGfxResources( );

	if (pFilename && strlen(pFilename) > 0)
	{
		CFixedString strFilename(pFilename);
		#ifdef CONVERT_TEXTURE_EXTENSIONS_TO_GNF
			strFilename = strFilename.NoExtension( ) + ".gnf";
		#endif

		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		const size_t c_siBufferSize = pFileSystem->FileSize(strFilename.c_str( ));
		if (c_siBufferSize > 0)
		{
			// load texture file into memory
			st_byte* pFileData = pFileSystem->LoadFile(strFilename.c_str( ));
			if (pFileData)
			{
				sce::Gnf::Header cHeader;
				memcpy(&cHeader, pFileData, sizeof(sce::Gnf::Header));
				if (cHeader.m_magicNumber == sce::Gnf::kMagic && cHeader.m_contentsSize < sce::Gnf::kMaxContents)
				{
					sce::Gnf::Contents* pContents = (sce::Gnf::Contents*)(pFileData + sizeof(sce::Gnf::Header));
					sce::Gnm::SizeAlign cPixelsSizeAlign = sce::Gnf::getTexturePixelsSize(pContents, 0);

					void* pTextureData = Orbis::Allocate(cPixelsSizeAlign.m_size, cPixelsSizeAlign.m_align, true);
					if (pTextureData != NULL)
					{
						st_byte* pPixelData = pFileData + sizeof(sce::Gnf::Header) + cHeader.m_contentsSize + getTexturePixelsByteOffset(pContents, 0);
						memcpy(pTextureData, pPixelData, cPixelsSizeAlign.m_size);
						m_cTexture = *sce::Gnf::patchTextures(pContents, 0, 1, &pTextureData);
						m_cTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
					}
				}

				pFileSystem->Release(pFileData);
			}
		}
	}
	
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::LoadColor

st_bool CTextureOrbis::LoadColor(st_uint32 uiColor)
{
	ReleaseGfxResources( );

	st_int32 nWidth = 4;
	st_int32 nHeight = 4;

	sce::Gnm::SizeAlign cPixelsSizeAlign = m_cTexture.initAs2d(nWidth, nHeight, 1, sce::Gnm::kDataFormatR8G8B8A8Unorm, sce::Gnm::kTileModeDisplay_LinearAligned, sce::Gnm::kNumFragments1);
	void* pTextureData = Orbis::Allocate(cPixelsSizeAlign.m_size, cPixelsSizeAlign.m_align, true);
	if (pTextureData != NULL)
	{
		m_cTexture.setBaseAddress(pTextureData);

		st_byte* pBytes = (st_byte*)(pTextureData);
		st_uint32 uiFinalColor = ((uiColor & 0x000000ff) << 24) + ((uiColor & 0x0000ff00) >> 8) + ((uiColor & 0x00ff0000) >> 8) + ((uiColor & 0xff000000) >> 8);
		for (st_uint32 i = 0; i < nWidth * nHeight; ++i)
		{
			*((st_uint32*)pBytes) = uiFinalColor;
			pBytes += 4;
		}

		m_cTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
	}
	
	return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::LoadNoise

st_bool CTextureOrbis::LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise)
{
	ReleaseGfxResources( );

	sce::Gnm::SizeAlign cPixelsSizeAlign = m_cTexture.initAs2d(nWidth, nHeight, 1, sce::Gnm::kDataFormatR8G8B8A8Unorm, sce::Gnm::kTileModeDisplay_LinearAligned, sce::Gnm::kNumFragments1);
	void* pTextureData = Orbis::Allocate(cPixelsSizeAlign.m_size, cPixelsSizeAlign.m_align, true);
	if (pTextureData != NULL)
	{
		m_cTexture.setBaseAddress(pTextureData);

		CRandom cRandom;
		st_byte* pBytes = (st_byte*)(pTextureData);
		for (st_int32 i = 0; i < cPixelsSizeAlign.m_size; ++i)
		{
			*pBytes = st_byte(cRandom.GetInteger(st_int32(fLowNoise * 255), st_int32(fHighNoise * 255)));
			++pBytes;
		}

		m_cTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
	}

	return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::LoadPerlinNoiseKernel

st_bool CTextureOrbis::LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth)
{
	ReleaseGfxResources( );

	sce::Gnm::SizeAlign cPixelsSizeAlign = m_cTexture.initAs2d(nWidth, nHeight, 1, sce::Gnm::kDataFormatR8G8B8A8Unorm, sce::Gnm::kTileModeDisplay_LinearAligned, sce::Gnm::kNumFragments1);
	void* pTextureData = Orbis::Allocate(cPixelsSizeAlign.m_size, cPixelsSizeAlign.m_align, true);
	if (pTextureData != NULL)
	{
		m_cTexture.setBaseAddress(pTextureData);

		CPerlinNoiseKernel cKernel(nWidth);

		#define XXX 1.0 // todo
		const st_float32 c_afSampleOffsets[4][2] = 
		{
			// todo
			{ -0.5f * XXX, -0.5f * XXX },
			{ -0.5f * XXX,  0.5f * XXX },
			{  0.5f * XXX, -0.5f * XXX },
			{  0.5f * XXX,  0.5f * XXX }
		};

		for (st_uint32 uiRow = 0; uiRow < nHeight; ++uiRow)
		{
			st_byte* pBytes = (st_byte*)(pTextureData) + uiRow * nWidth * 4;

			for (st_uint32 uiCol = 0; uiCol < nWidth; ++uiCol)
			{
				*pBytes++ = st_byte(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[0][0], uiRow + c_afSampleOffsets[0][1])); // red
				*pBytes++ = st_byte(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[1][0], uiRow + c_afSampleOffsets[1][1])); // green
				*pBytes++ = st_byte(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[2][0], uiRow + c_afSampleOffsets[2][1])); // blue
				*pBytes++ = st_byte(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[3][0], uiRow + c_afSampleOffsets[3][1])); // alpha
			}
		}

		m_cTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
	}

	return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::ReleaseGfxResources

st_bool CTextureOrbis::ReleaseGfxResources(void)
{
	st_bool bSuccess = false;
	
	if (IsValid( ))
	{
		Orbis::Release(m_cTexture.getBaseAddress( ));
		memset(&m_cTexture, 0, sizeof(sce::Gnm::Texture));

		bSuccess = true;
	}

	return bSuccess;
}
