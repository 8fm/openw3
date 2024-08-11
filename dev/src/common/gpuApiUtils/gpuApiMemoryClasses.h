/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

////////////////////////////////////////////////////////////////////////
// Memory Class Definitions
//	This file should not be included directly ) unless you plan on defining the 
//	DECLARE_MEMORY_CLASS macro to do something interesting

// Macro Usage:
// To add a new memory class use DECLARE_MEMORY_CLASS( Class Name )

#ifdef DECLARE_MEMORY_CLASS

DECLARE_MEMORY_CLASS( MC_Unassigned )
DECLARE_MEMORY_CLASS( MC_TextureData )
DECLARE_MEMORY_CLASS( MC_VertexBuffer )
DECLARE_MEMORY_CLASS( MC_BufferObject )
DECLARE_MEMORY_CLASS( MC_IndexBuffer )
DECLARE_MEMORY_CLASS( MC_ConstantBuffer )
DECLARE_MEMORY_CLASS( MC_StructuredBuffer )
DECLARE_MEMORY_CLASS( MC_RawBuffer )
DECLARE_MEMORY_CLASS( MC_LockedBuffer )		// 'Generic' locked buffer for any buffer types we don't recognise
DECLARE_MEMORY_CLASS( MC_Temporary )
DECLARE_MEMORY_CLASS( MC_Device )
DECLARE_MEMORY_CLASS( MC_BitmapDataBuffer )
DECLARE_MEMORY_CLASS( MC_InPlaceTexture )
DECLARE_MEMORY_CLASS( MC_InPlaceScaleform )
DECLARE_MEMORY_CLASS( MC_InPlaceBuffer )
DECLARE_MEMORY_CLASS( MC_InternalTexture )
DECLARE_MEMORY_CLASS( MC_Label )
DECLARE_MEMORY_CLASS( MC_EnvProbe )
#ifdef RED_PLATFORM_ORBIS
DECLARE_MEMORY_CLASS( MC_ShaderCode )
DECLARE_MEMORY_CLASS( MC_CUEHeap )
DECLARE_MEMORY_CLASS( MC_CPRAMShadow )
DECLARE_MEMORY_CLASS( MC_DCB )
DECLARE_MEMORY_CLASS( MC_CCB )
DECLARE_MEMORY_CLASS( MC_ShaderHeaders )
DECLARE_MEMORY_CLASS( MC_GlobalResTable )
DECLARE_MEMORY_CLASS( MC_GPURingBuffers )
DECLARE_MEMORY_CLASS( MC_SwapChainTexture )
DECLARE_MEMORY_CLASS( MC_SSAORenderData )
DECLARE_MEMORY_CLASS( MC_DynamicTextureBuffer )
DECLARE_MEMORY_CLASS( MC_StagingTextureBuffer )
DECLARE_MEMORY_CLASS( MC_RenderTarget )
DECLARE_MEMORY_CLASS( MC_RenderTarget_UI )
DECLARE_MEMORY_CLASS( MC_RenderTarget_Shadow )
DECLARE_MEMORY_CLASS( MC_RenderTarget_Terrain )
DECLARE_MEMORY_CLASS( MC_RenderTarget_TerrainShadow )
DECLARE_MEMORY_CLASS( MC_DepthStencilTarget )
DECLARE_MEMORY_CLASS( MC_DepthStencilTarget_UI )
DECLARE_MEMORY_CLASS( MC_DepthStencilTarget_Shadow )
DECLARE_MEMORY_CLASS( MC_DepthStencilTarget_Terrain )
DECLARE_MEMORY_CLASS( MC_DepthStencilTarget_TerrainShadow )
DECLARE_MEMORY_CLASS( MC_CMask )
DECLARE_MEMORY_CLASS( MC_HTile )
DECLARE_MEMORY_CLASS( MC_Query )
#endif

#else

#error DECLARE_MEMORY_CLASS must be defined to include gpuApiMemoryClasses.h

#endif