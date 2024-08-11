/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderMesh.h"
#include "renderMaterial.h"


class CMorphedMeshComponent;


enum ERenderMorphedMeshChunkFlags
{
	RMMCF_BlendedMaterial	= FLAG( 0 ),
	RMMCF_FromTargetMesh	= FLAG( 1 ),
};


class CRenderMorphedMesh : public CRenderMesh
{
protected:
	struct AdditionalChunkData
	{
		CRenderMaterial*			m_material;
		CRenderMaterialParameters*	m_materialParams;
		GpuApi::VertexLayoutRef		m_vertexLayout;
		Int32						m_flags;
		Int8						m_sourceChunk;

		AdditionalChunkData();
		AdditionalChunkData( IMaterial* material, GpuApi::VertexLayoutRef vertexLayout, Int32 flags, Int8 sourceChunk );
		~AdditionalChunkData();

		AdditionalChunkData( const AdditionalChunkData& other );
		AdditionalChunkData( AdditionalChunkData&& other );

		AdditionalChunkData& operator =( const AdditionalChunkData& other );
		AdditionalChunkData& operator =( AdditionalChunkData&& other );
	};


protected:
	CRenderMorphedMesh();

public:
	virtual ~CRenderMorphedMesh();

	virtual CName GetCategory() const override { return RED_NAME( RenderMorphedMesh ); }

	//! Return whether the source and target render meshes are fully loaded.
	RED_INLINE Bool AreBaseMeshesFullyLoaded() const { return m_sourceMesh->IsFullyLoaded() && m_targetMesh->IsFullyLoaded(); }

	RED_INLINE CRenderMaterial* GetChunkMaterial( Uint32 chunkIndex ) const { return chunkIndex < m_chunkData.Size() ? m_chunkData[ chunkIndex ].m_material : nullptr; }
	RED_INLINE CRenderMaterialParameters* GetChunkMaterialParameters( Uint32 chunkIndex ) const { return chunkIndex < m_chunkData.Size() ? m_chunkData[ chunkIndex ].m_materialParams : nullptr; }
	RED_INLINE Bool IsChunkMaterialBlended( Uint32 chunkIndex ) const { return chunkIndex < m_chunkData.Size() ? ( m_chunkData[ chunkIndex ].m_flags & RMMCF_BlendedMaterial ) != 0 : false; }
	RED_INLINE Bool IsChunkFromTarget( Uint32 chunkIndex ) const { return chunkIndex < m_chunkData.Size() ? ( m_chunkData[ chunkIndex ].m_flags & RMMCF_FromTargetMesh ) != 0 : false; }

	RED_INLINE CRenderTexture* GetChunkControlTexture( Uint32 chunkIndex ) const
	{
		if ( chunkIndex >= m_chunks.Size() )
		{
			return nullptr;
		}

		Uint8 materialIndex = m_chunks[ chunkIndex ].m_materialId;
		return materialIndex < m_controlTextures.Size() ? m_controlTextures[ materialIndex ] : nullptr;
	}


	Bool ApplyMorphRatio( Float ratio );

	virtual void GetChunkGeometry( Uint8 chunkIndex, ChunkGeometry& outGeometry ) const override;
	virtual GpuApi::VertexLayoutRef GetChunkVertexLayout( Uint8 chunkIndex ) const override;


public:
	static CRenderMorphedMesh* Create( const CMorphedMeshComponent* component, Uint64 partialRegistrationHash );


protected:
	CRenderMesh*						m_sourceMesh;					//!< "Source" mesh, used when ratio is 0.
	CRenderMesh*						m_targetMesh;					//!< "Target" mesh, when ratio is 1.

	TDynArray< AdditionalChunkData >	m_chunkData;

	TDynArray< CRenderTexture* >		m_controlTextures;				//!< 1 per material in mesh (source and target should have same number).
	Bool								m_useControlTexForMorph;

private:
	struct SMorphGenCB
	{
		Vector Source_QS;
		Vector Source_QB;
		Vector Target_QS;
		Vector Target_QB;
		Vector Morph_QS;
		Vector Morph_QB;
		Float Weight;
		Uint32 NumVertices;
		Uint32 Source_PosByteOffset;
		Uint32 Source_TbnByteOffset;
		Uint32 Source_TexByteOffset;
		Uint32 Target_PosByteOffset;
		Uint32 Target_TbnByteOffset;
		Uint32 OutputByteOffset;
	};
	static_assert( sizeof(SMorphGenCB) == 8*4*4, "CB struct has incorrect size. Padded weirdly?" );
};
