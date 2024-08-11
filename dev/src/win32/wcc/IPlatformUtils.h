/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

enum PlatformTextureFormat
{
	PUFMT_UNKNOWN,
	PUFMT_A8R8G8B8,
	PUFMT_L8,
	PUFMT_A8L8,
	PUFMT_DXT1,
	PUFMT_DXT3,
	PUFMT_DXT5,
	PUFMT_Float_A32R32G32B32,
	PUFMT_Float_Lin_R32,
	PUFMT_Lin_DXT5,

	PUFMT_A8 = 28// D3DFMT_A8
};
//small interfaces that feeds source data
class IMipDataProvider
{
public:
	virtual PlatformTextureFormat GetFormat(Uint32 sliceIndex) = 0;
	virtual Uint32 GetMipCount() = 0;
	virtual Uint32 GetSliceCount() = 0;
	virtual void* GetMipData(Uint32 sliceIndex, Uint32 mipIndex) = 0;
	virtual Uint32 GetMipPitch(Uint32 mipIndex) = 0;
	//this is needed, becuase otherwise memory could be allocated by DLL and we don't won't that!
	virtual void* AllocateDstBuffer(Uint32 size) = 0;
	virtual void FreeDstBuffer() = 0;
};

//small interfaces that feeds source data for cubmap
class ICubeMipDataProvider
{
public:
	virtual PlatformTextureFormat GetFormat() = 0;
	virtual Uint32 GetMipCount() = 0;
	virtual void* GetMipData(Uint32 face, Uint32 mipIndex) = 0;
	virtual Uint32 GetMipPitch(Uint32 mipIndex) = 0;
	//this is needed, becuase otherwise memory could be allocated by DLL and we don't won't that!
	virtual void* AllocateDstBuffer(Uint32 size) = 0;
	virtual void FreeDstBuffer() = 0;
};

class IShaderDataProvider
{
public:
	virtual const void* GetCompiledData() = 0;
	virtual Uint32 GetCompiledDataSize() = 0;
	virtual void* AllocateDataBuffer(Uint32 size) = 0;
	virtual void* AllocateHeaderBuffer(Uint32 size) = 0;
	virtual void FreeBuffers() = 0;
};

//please make this in sync with mesh.h
struct PlatformVertex
{
	Float	m_position[3];			// Vertex position
	Uint8	m_indices[4];			// Skinning indices
	Float	m_weights[4];			// Skinning weights
	Float	m_normal[3];			// Vertex normal
	Uint32	m_color;				// Vertex color
	Float	m_uv0[2];				// First uv set
	Float	m_uv1[2];				// Second uv set
	Float	m_tangent[3];			// Vertex tangent vector
	Float	m_binormal[3];			// Vertex binormal vector
	Float	m_extraData0[4];		// Extra data
	Float	m_extraData1[4];		// Extra data
	Float	m_extraData2[4];		// Extra data
	Float	m_extraData3[4];		// Extra data
};

class IMeshChunkProvider
{
public:
	virtual Uint32 GetChunkCount() = 0;
	virtual Uint32 GetChunkVertexType( Uint32 chunkIndex) = 0;
	virtual Uint32 GetChunkVertexCount( Uint32 chunkIndex) = 0;
	virtual const PlatformVertex* GetChunkVertices( Uint32 chunkIndex) = 0;
	virtual Uint32 GetChunkIndexCount( Uint32 chunkIndex) = 0;
	virtual const Uint16* GetChunkIndices( Uint32 chunkIndex) = 0;

	//this is needed, becuase otherwise memory could be allocated by DLL and we don't won't that!
	virtual void* AllocateDstBuffer(Uint32 size) = 0;
	virtual void FreeDstBuffer() = 0;
};

#if defined DLL_EXPORT
#define DECL_EXPORT __declspec(dllexport)
#else
#define DECL_EXPORT __declspec(dllimport)
#endif

class DECL_EXPORT IPlatformUtils
{
public:

	//packs all mipmaps into dstBuffer with given size and format
	virtual Uint32 PackTexture(IMipDataProvider& mipProvider, Uint32 dstWidth, Uint32 dstHeight, PlatformTextureFormat dstFormat) = 0;

	//pack cubemap texture into dstBuffer with given size and format
	virtual	Uint32 PackCubeTexture(ICubeMipDataProvider& cubeMipProvider, Uint32 edgeSize, PlatformTextureFormat dstFormat) = 0;

	//shader compilation from already prepared buffers
	virtual Bool PackShader( IShaderDataProvider& shaderProvider, Int32 targetType ) = 0;
};