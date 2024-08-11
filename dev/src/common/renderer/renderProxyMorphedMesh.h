/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyMesh.h"


class CRenderMorphedMesh;


/// Rendering proxy for morphed mesh (interpolates between two compatible meshes)
class CRenderProxy_MorphedMesh : public CRenderProxy_Mesh
{
protected:
	Float										m_morphRatio;					//!< How much to blend between source and target meshes.

	Bool										m_needMorphGenerate;			//!< Flag set when the morph shader needs to be run again. Only needed when morph ratio changes.

public:
	CRenderProxy_MorphedMesh( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_MorphedMesh();

	virtual Bool IsMorphedMesh() const override { return true; }

	void SetMorphRatio( Float ratio );
	Float GetMorphRatio() const { return m_morphRatio; }

	virtual void CollectedTick( CRenderSceneEx* scene ) override;

	virtual CRenderTexture* GetUVDissolveTexture( const CRenderElement_MeshChunk* chunk ) const override;
	virtual Vector GetUVDissolveValues( const CRenderElement_MeshChunk* chunk ) const override;
	virtual Bool DoesUVDissolveUseSeparateTexCoord( const CRenderElement_MeshChunk* chunk ) const override;

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

	virtual void AttachToScene( CRenderSceneEx* scene ) override;

protected:
	virtual Bool ShouldCollectElement( CRenderCollector* collector, const Bool isShadow, const CRenderElement_MeshChunk* chunk ) const override;
	virtual void OnCollectElement( CRenderCollector* collector, Bool isShadow, CRenderElement_MeshChunk* chunk ) override;

	/// Chain for update function that should happen only once per frame
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );
};
