/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyMesh.h"

struct	SBoneIndicesHelper;
class	CComponent;
/// Rendering proxy for destruction mesh
class CRenderProxy_DestructionMesh : public CRenderProxy_Mesh
{
	TDynArray< Uint16 >				m_activeIndices;
	TDynArray< Uint32 >				m_chunkOffsets;
	TDynArray< Uint32 >				m_chunkNumIndices;
	Bool							m_needUpdateBoneIndices;
public:
	CRenderProxy_DestructionMesh( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_DestructionMesh();

	virtual Bool IsMorphedMesh() const override { return false; }

	virtual void UpdateActiveIndices( TDynArray<Uint16>&& indices, TDynArray< Uint32 >&& newOffsets, TDynArray< Uint32 >&& newNumIndices );

	virtual void CollectedTick( CRenderSceneEx* scene ) override;

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector );

	//! Collect elements for a list of shadow cascades
	virtual void CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults );

	//! Collect local shadow elements ( for point and spot lights )
	virtual void CollectLocalShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector );

	//! Collect local shadow elements for STATIC shadows ( highest LOD, no dissolve )
	virtual void CollectStaticShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector );

	//! Collect elements for rendering hires shadows
	virtual void CollectHiResShadowsElements( CRenderCollector& collector, CRenderCollector::HiResShadowsCollector& elementCollector, Bool forceHighestLOD );

protected:
	/// Chain for update function that should happen only once per frame
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );
};
