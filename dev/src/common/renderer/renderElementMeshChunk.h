/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderElement.h"

class CRenderProxy_Mesh;
class CRenderMesh;
class CRenderTexture;
class CRenderCollector;
enum EMaterialVertexFactory : CEnum::TValueType;

/// Mesh chunk flags
enum ERenderMeshChunkFlags
{
	RMCF_MimicsMaterial				= FLAG( 0 ),
	RMCF_UseColorShift				= FLAG( 1 ),
	RMCF_UseEffectParam0			= FLAG( 2 ),
	RMCF_UseEffectParam1			= FLAG( 3 ),
	RMCF_MaterialOverride			= FLAG( 4 ),
	RMCF_UsesSkinning				= FLAG( 5 ),
	RMCF_UsesVertexCollapse			= FLAG( 6 ),
	RMCF_UsesDissolve				= FLAG( 7 ),
	RMCF_UsesUVDissolve				= FLAG( 8 ),
	RMCF_IsMaskedMaterial			= FLAG( 9 ),
	RMCF_UsesExtraStreams			= FLAG( 10 ),
	RMCF_CastsShadows				= FLAG( 11 ),
	RMCF_NormalBlendMaterial		= FLAG( 12 ),
	RMCF_NoInstancing				= FLAG( 13 ),
	RMCF_MorphTarget				= FLAG( 14 ),			//!< For morphed mesh, is the chunk from the target mesh?
	RMCF_MorphBlendMaterial			= FLAG( 15 ),			//!< For morphed mesh, does the chunk use a blending material?
	RMCF_AdditionalShadowElement	= FLAG( 16 ),			//!< Draw this element even when the proxy is filtered out by the merged shadows
	RMCF_UsesCascadesDissolve		= FLAG( 17 ),
};

/// Single chunk of drawable mesh
class CRenderElement_MeshChunk : public IRenderElement
{
	DECLARE_RENDER_OBJECT_MEMORYPOOL( MemoryPool_SmallObjects, MC_RenderElementMeshChunk )

protected:
	CRenderMesh*			m_mesh;							//!< Mesh to render
	Uint32					m_elementFlags;					//!< Mesh elements flags
	EMaterialVertexFactory	m_vertexFactory;				//!< Type of vertices to render

	Uint8					m_chunkIndex;					//!< Index of chunk to draw
	Uint8					m_baseRenderMask;				//!< Render mask (base)
	Uint8					m_mergedRenderMask;				//!< Render mask (merged geometry)
	Int8					m_lodGroupIndex;				//!< Lod group index	
	Bool					m_canUseInstancing;				//!< Cached value to check instancing

public:
	//! Get the mesh
	RED_INLINE CRenderMesh* GetMesh() const { return m_mesh; }

	//! Set a new mesh
	void ReplaceMesh( CRenderMesh* newMesh );

	//! Get type of vertices to draw
	RED_INLINE EMaterialVertexFactory GetVertexFactory() const { return m_vertexFactory; }

	//! Get the chunk index
	RED_INLINE Uint8 GetChunkIndex() const { return m_chunkIndex; }

	//! Get lod group index
	RED_INLINE Int8 GetLodGroupIndex() const { return m_lodGroupIndex; }

	//! Check flag
	RED_INLINE Bool HasFlag( ERenderMeshChunkFlags chunkFlag ) const { return 0 != ( m_elementFlags & chunkFlag ); }

	RED_INLINE Uint32 GetFlags() const { return m_elementFlags; }

	void SetMeshChunkFlag( ERenderMeshChunkFlags flag, Bool flagValue = true );

	//! Render mask filter
	RED_INLINE const Uint8 GetBaseRenderMask() const { return m_baseRenderMask; }
	RED_INLINE const Uint8 GetMergedRenderMask() const { return m_mergedRenderMask; }
	RED_INLINE Bool CheckRenderMask( const Uint8 mask, const Bool useMergedData ) const { return 0 != (((useMergedData) ? m_mergedRenderMask : m_baseRenderMask) & mask); }

protected:
	CRenderElement_MeshChunk( IRenderProxyDrawable* proxy, CRenderMesh* mesh, Uint32 chunkIndex, IMaterial* material, Int32 lodGroupIndex, const Uint8 filterRenderMask = 0xFF );
public:
	CRenderElement_MeshChunk( IRenderProxyDrawable* proxy, CRenderMesh* mesh, Uint32 chunkIndex, CRenderMaterial* material, CRenderMaterialParameters* parameters, Int32 lodGroupIndex, const Uint8 filterRenderMask = 0xFF );
	CRenderElement_MeshChunk( const CRenderElement_MeshChunk& toCopy );
	~CRenderElement_MeshChunk();

	//! Create a copy of this render element, but replace the material. Will set the RMCF_MaterialOverride flag on the created element.
	CRenderElement_MeshChunk* CreateCopyWithMaterialOverride( CRenderMaterial* material, CRenderMaterialParameters* parameters ) const;

	//! Set chunk index
	void SetChunkIndex( Uint32 newChunkIndex );

	//! Is this skinnable mesh element
	Bool IsSkinnable() const;

	Bool IsStaticGeometryChunk() const;

	//! Get merged dissolve values
	Vector GetDissolveValues() const;

	//! Calculate the shadow dissolve values for given cascade
	Vector GetShadowDissolveValues() const;

	//! Get special texture used by the UV Dissolve. If NULL, a default will be used instead.
	CRenderTexture* GetUVDissolveTexture() const;

	//! XY hold UV dissolve control values, similar to GetDissolveValues().
	Vector GetUVDissolveValues() const;

	//! If true, the UV dissolve will use TexCoord2 instead of the default TexCoord0.
	Bool DoesUVDissolveUseSeparateTexCoord() const;

	//! Whether this chunk can be drawn using instancing.
	RED_FORCE_INLINE Bool CanUseInstancing() const { return m_canUseInstancing; }

	void RefreshInstancingFlag();
};
