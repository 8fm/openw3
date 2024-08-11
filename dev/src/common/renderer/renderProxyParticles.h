/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyDrawable.h"
#include "renderParticleEmitter.h"
#include "renderCollector.h" // used in inline functions

class CRenderElement_ParticleEmitter;
class IParticleData;
struct SParticlesSimulationContext;
class CSubframeInterpolator;
struct SSimulationContextUpdate;

/// Proxy for particles
class CRenderProxy_Particles : public IRenderProxyDrawable
{
protected:
	TDynArray< CRenderElement_ParticleEmitter* >	m_emitters;
	EEnvAutoHideGroup								m_envAutoHideGroup;
	Float											m_autoHideDistance;
	Float											m_autoHideDistanceSquared;
	Float											m_autoHideInvRange;
	Float											m_prewarmingTime;

	Bool											m_didCollect;
	Bool											m_isVisibleThroughWalls;
	Uint8											m_renderPriority;

	TDynArray< Float >								m_lodDistancesSquared;

public:
	SSimulationContext								m_simulationContext;

public:
	RED_FORCE_INLINE Bool DidCollectSinceLastUpdate() const { return m_didCollect; }

	//! Is using environment auto hide parameters
	RED_FORCE_INLINE Bool IsUsingEnvAutoHideParams() const { return EAHG_None != m_envAutoHideGroup; }

	RED_FORCE_INLINE Bool IsVisibleThroughWalls() const { return m_isVisibleThroughWalls; }

	//! Get environment auto hide group
	RED_FORCE_INLINE EEnvAutoHideGroup GetEnvAutoHideGroup() const { return m_envAutoHideGroup; }

	//! Get auto hide distance
	RED_FORCE_INLINE Float GetAutoHideDistance( const CRenderCollector &collector ) const 
	{ 
		if ( IsUsingEnvAutoHideParams() )
		{
			Float distance = collector.GetRenderFrameInfo().m_envParametersArea.m_colorModTransparency.GetParamsForAutoHideGroup( m_envAutoHideGroup ).m_distance.GetScalar();
			return Max( 0.001f, distance ); // epsilon above zero, because zero it treated as no autohide
		}

		return m_autoHideDistance; 
	}

	//! Get auto hide distance
	RED_FORCE_INLINE Float GetAutoHideDistanceSquared( const CRenderCollector &collector ) const 
	{ 
		if ( IsUsingEnvAutoHideParams() )
		{
			Float distance = collector.GetRenderFrameInfo().m_envParametersArea.m_colorModTransparency.GetParamsForAutoHideGroup( m_envAutoHideGroup ).m_distance.GetScalar();
			return Max( 0.000001f, distance * distance ); // epsilon above zero, because zero it treated as no autohide
		}

		return m_autoHideDistanceSquared; 
	}

	//! Get auto hide range
	RED_FORCE_INLINE Float GetAutoHideInvRange( const CRenderCollector &collector ) const 
	{
		if ( IsUsingEnvAutoHideParams() )
		{
			Float range = collector.GetRenderFrameInfo().m_envParametersArea.m_colorModTransparency.GetParamsForAutoHideGroup( m_envAutoHideGroup ).m_range.GetScalar();
			return 1.f / Max( 0.001f, range );
		}

		return m_autoHideInvRange; 
	}

	//! Get render priority
	RED_FORCE_INLINE Uint8 GetRenderPriority( ) const { return m_renderPriority; }
	RED_FORCE_INLINE void SetRenderPriority (Uint8 priority ) { m_renderPriority = priority; }

	//! Update simulation context
	RED_INLINE void UpdateWindVector( const Vector3& windAtPos ){ m_simulationContext.m_windVector = windAtPos; }
	RED_INLINE void UpdateWindVectorOnly( const Vector3& windOnlyAtPos ){ m_simulationContext.m_windVectorOnly = windOnlyAtPos; }
	void UpdateSimulationContext( const SSimulationContextUpdate& context );
	void UpdateInterpolator();

	const SSimulationContext& GetSimulationContext() const { return m_simulationContext; }

public:
	CRenderProxy_Particles( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_Particles();

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	virtual void AttachToScene( CRenderSceneEx* scene );

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector ) override;

	RED_FORCE_INLINE const TDynArray< CRenderElement_ParticleEmitter* >& GetEmitters() const { return m_emitters; }

#ifndef NO_DEBUG_PAGES
	virtual void GetDescription( TDynArray< String >& descriptionLines ) const override;
	virtual Bool ValidateOptimization( TDynArray< String >* commentLines ) const override;
#endif

#ifndef NO_EDITOR
	//! Update or create render element corresponding with given emitter
	void UpdateOrCreateEmitter( CRenderParticleEmitter* emitter, CRenderMaterial* material, CRenderMaterialParameters* materialParameters, EEnvColorGroup envColorGroup );

	//! Remove render element corresponding to given emitter
	void RemoveEmitter( Int32 uniqueId );
#endif

	const EFrameUpdateState UpdateOncePerFrame( const CRenderCollector& collector );

protected:
	void CalculateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame );
};
