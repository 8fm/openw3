/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "renderDynamicDecalChunk.h"


class CRenderDecalMesh;
class CRenderSkinningData;
class CRenderProxy_Mesh;


class CRenderDynamicDecalChunk_Mesh : public CRenderDynamicDecalChunk
{
	CRenderDecalMesh*	m_mesh;
	Uint8				m_chunkIndex;

public:
	CRenderDynamicDecalChunk_Mesh( CRenderDynamicDecal* ownerDecal, CRenderProxy_Mesh* target, CRenderDecalMesh* decalMesh, Uint8 chunkIndex );
	virtual ~CRenderDynamicDecalChunk_Mesh();

	virtual void Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo );

	virtual CRenderSkinningData* GetSkinningData() const;
	virtual GpuApi::VertexLayoutRef GetVertexLayout() const;

	virtual Uint32 GetUsedVideoMemory() const;
};
