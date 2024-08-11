/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyDrawable.h"
#include "renderCollector.h" //for the shadow cascades
#include "renderSwarmAnimationManager.h"

class CRenderElement_MeshChunk;
class CRenderProxyShadowCulling;
class CRenderSkinningData;
class CRenderSwarmData;
class CRenderEntityGroup;
class CSwarmBoidData;

/// Rendering proxy for swarm
class CRenderProxy_Swarm : public IRenderProxyDrawable
{
protected:

	enum { NUM_LODS = 3 };

	CRenderElement_MeshChunk*					m_chunks[ NUM_LODS ];					//!< Mesh chunks (1 per LOD ONLY)

	CSwarmBoidData*								m_swarmBoidsToRender;					// list of agents to render
	CSwarmBoidData*								m_swarmBoidsToSynchonize;				// list of agents for fixing instance animation id data
	Uint32										m_numBoidsToRender;						// amount of agents to render
	TDynArray<const CSwarmBoidData*>			m_swarmLODs[ NUM_LODS ];				// same list as above but split into LOD groups depending on distance from camera

	SwarmMassiveAnimation::MassiveAnimationSetSharedPtr	m_massiveAnimationSet;

public:

	// return LOD group
	const TDynArray<const CSwarmBoidData*>& GetInstanceArrayForLOD( Uint32 lod )	{ return m_swarmLODs[ lod ]; }

	//! Is this drawable allowed to be rendered into envProbes
	virtual Bool IsAllowedForEnvProbes() const override						{ return false; }

public:
	CRenderProxy_Swarm( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_Swarm();

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector ) override;

	// update boids to render from engine side
	void UpdateBoids( CRenderSwarmData* data, Uint32 numBoids );

	//! Attach to scene
	virtual void AttachToScene( CRenderSceneEx* scene ) override;

	const CRenderSkinningData* GetSkinningDataForBoid( const CSwarmBoidData* boid ) const;

protected:
	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );
	void CalculateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame );
};

// we extend this class only to set the render element type to RET_Swarm
class CRenderElement_SwarmChunk : public CRenderElement_MeshChunk
{
public:
	CRenderElement_SwarmChunk( IRenderProxyDrawable* proxy, CRenderMesh* mesh, Uint32 chunkIndex, IMaterial* material, Int32 lodGroupIndex )
		: CRenderElement_MeshChunk( proxy, mesh, chunkIndex, material, lodGroupIndex )
	{
		m_type = RET_Swarm;

		if ( IsSkinnable() ) 
		{
			m_elementFlags |= RMCF_UsesSkinning;
		}
	}
};
