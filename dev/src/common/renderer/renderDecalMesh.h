/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderMesh.h"


class CRenderDynamicDecal;
class CRenderSkinningData;
class CRenderElement_MeshChunk;


// Render mesh used by CRenderDynamicDecalChunk_Mesh to combine source mesh data with decal data.
class CRenderDecalMesh : public CRenderMesh
{
protected:
	CRenderDecalMesh();

public:
	virtual ~CRenderDecalMesh();

	virtual CName GetCategory() const override { return RED_NAME( RenderDecalMesh ); }


	virtual void GetChunkGeometry( Uint8 chunkIndex, ChunkGeometry& outGeometry ) const override;
	virtual GpuApi::VertexLayoutRef GetChunkVertexLayout( Uint8 chunkIndex ) const override;


public:
	// Create a Decal Mesh from a set of MeshChunk render elements. All chunks must reference the same CRenderMesh.
	static CRenderDecalMesh* Create( const CRenderDynamicDecal* decal, const CRenderElement_MeshChunk* const* meshChunks, Uint8 numChunks, const Matrix& meshLocalToWorld, CRenderSkinningData* skinningData, Uint64 partialRegistrationHash );


protected:
	CRenderMesh*		m_sourceMesh;				// The mesh we're attached to. We take positions and tangent frames from here, instead of duplicating them.
	TDynArray< Int8 >	m_sourceMeshChunkMapping;	// Mapping from chunk index in this mesh to a chunk in the target mesh.

	TDynArray< GpuApi::VertexLayoutRef >	m_chunkVertexLayouts;
};
