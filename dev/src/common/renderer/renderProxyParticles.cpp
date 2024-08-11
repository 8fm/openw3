/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxyParticles.h"
#include "renderElementParticleEmitter.h"
#include "renderElementMap.h"
#include "renderScene.h"
#include "..\engine\renderer.h"
#include "renderParticleBatcher.h"
#include "renderVisibilityQueryManager.h"
#include "..\physics\physicsParticleWrapper.h"
#include "..\engine\particleSystem.h"
#include "..\engine\particleComponent.h"
#include "renderParticleEmitter.h"
#include "..\engine\material.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Bool isAnselParticleAttach;
extern Float anselParticlesPrewarmTime;
#endif // USE_ANSEL

CRenderProxy_Particles::CRenderProxy_Particles( const RenderProxyInitInfo& initInfo )
	: IRenderProxyDrawable( RPT_Particles, initInfo )
	, m_envAutoHideGroup( EAHG_None )
	, m_autoHideDistance( 0 )
	, m_autoHideDistanceSquared( 0 )
	, m_autoHideInvRange( 9999.f )
	, m_prewarmingTime( 0.0f )
	, m_renderPriority( 10 )
	, m_isVisibleThroughWalls( false )
{
	// Create emitters
	if ( initInfo.m_component->IsA< CParticleComponent >() )
	{
		const CParticleComponent* pc = static_cast< const CParticleComponent* >( initInfo.m_component );
		if ( pc->GetParticleSystem() )
		{
			const CParticleSystem* ps = pc->GetParticleSystem();
			m_prewarmingTime = ps->GetPrewarmingTime();
			m_renderingPlane = ps->GetRenderingPlane();
			m_isVisibleThroughWalls = ps->IsVisibleThroughWalls();

			// Get emitters
			const TDynArray< CParticleEmitter* >& emitters = ps->GetEmitters();
			for ( Uint32 i=0; i<emitters.Size(); i++ )
			{
				const CParticleEmitter* pe = emitters[i];
				IMaterial* material = pe->GetMaterial();
				if ( pe && material && material->GetMaterialDefinition() && pe->IsEnabled() )
				{
					// Create render side particle emitter, based on engine side particle emitter setup
					IRenderResource* rr = pe->GetRenderResource();
					CRenderParticleEmitter* renderParticleEmitter = (CRenderParticleEmitter*) rr;
					if ( !renderParticleEmitter ) 
					{
						continue;
					}

					CRenderElement_ParticleEmitter* element = new CRenderElement_ParticleEmitter( this, material, pe->GetEnvColorGroup(), i, renderParticleEmitter );
					if ( element->GetMaterial() && element->GetMaterialParams() )
					{
						m_emitters.PushBack( element );
					}
					else
					{
						const String brokenMaterialName = material->GetDepotPath();
						RED_FATAL_ASSERT( element->GetMaterial() && element->GetMaterialParams(), "ParticleEmitter with broken material! Please inspect following resource: '%ls'", brokenMaterialName.AsChar() );
					}
				}
			}


			m_lodDistancesSquared.Resize( ps->GetLODCount() );
			for ( Uint32 i = 0; i < ps->GetLODCount(); ++i )
			{
				const Float lodDist = ps->GetLOD( i ).m_distance;
				m_lodDistancesSquared[i] = lodDist*lodDist;
			}

			// Auto hide distance/range
			m_autoHideDistance = Max( 0.f, ps->GetAutoHideDistance() );
			m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;
			m_autoHideInvRange = 1.f / Max( 0.001f, ps->GetAutoHideRange() );
		}

		m_envAutoHideGroup = pc->GetEnvAutoHideGroup();
	}

	m_boundingBox = Box( m_localToWorld.GetTranslation(), 10000.0f );
}

CRenderProxy_Particles::~CRenderProxy_Particles()
{
	if( m_simulationContext.m_wrapper )
	{
		m_simulationContext.m_wrapper->Release();
		m_simulationContext.m_wrapper = nullptr;
	}
	// Release emitter
	for ( Uint32 i=0; i<m_emitters.Size(); i++ )
	{
		SAFE_RELEASE( m_emitters[i] );
	}
}



void CRenderProxy_Particles::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	const Float distanceSq = CalcCameraDistanceSqForTextures( prefetch->GetCameraPosition(), prefetch->GetCameraFovMultiplierUnclamped() );
	for ( CRenderElement_ParticleEmitter* emitter : m_emitters )
	{
		prefetch->AddMaterialBind( emitter->GetMaterial(), emitter->GetMaterialParams(), distanceSq );
	}
}


void CRenderProxy_Particles::AttachToScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::AttachToScene( scene );

#ifdef USE_ANSEL
	if ( isAnselSessionActive )
	{
		isAnselParticleAttach = true;
		GetRenderer()->GetParticleBatcher()->SimulateEmittersOverTime( this, anselParticlesPrewarmTime, 1.05f );
		isAnselParticleAttach = false;
	}
	else 
#endif // USE_ANSEL		
	if ( m_prewarmingTime > 0.0f )
	{
		GetRenderer()->GetParticleBatcher()->SimulateEmittersOverTime( this, m_prewarmingTime, 1.05f );
	}
}

const EFrameUpdateState CRenderProxy_Particles::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyDrawable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	const Bool wasVisibleLastFrame = ( ret == FUS_UpdatedLastFrame );
	CalculateLOD( collector, wasVisibleLastFrame );

	return ret;
}

void CRenderProxy_Particles::CalculateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
	// Check for a forced LOD
	if ( collector.m_scene->GetForcedLOD() != -1 )
	{
		m_simulationContext.m_lod = Min( (Uint32)collector.m_scene->GetForcedLOD(), m_lodDistancesSquared.Size() - 1 );
		return;
	}

	const Float distFromCameraSquared = GetCachedDistanceSquared();

	const Uint32 lodCount = m_lodDistancesSquared.Size();
	m_simulationContext.m_lod = lodCount - 1;

	for ( Int32 i = lodCount - 1; i >= 0; --i )
	{
		if ( distFromCameraSquared > m_lodDistancesSquared[i] )
		{
			m_simulationContext.m_lod = i;
			break;
		}
	}
}


void CRenderProxy_Particles::CollectElements( CRenderCollector& collector )
{
	if ( !collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_Particles ) )
	{
		return;
	}

	UpdateOncePerFrame( collector );

	const Float autoHideDistanceSquared = GetAutoHideDistanceSquared( collector );
	const Vector& cameraPosition = collector.GetRenderFrameInfo().m_camera.GetPosition();

	for ( Uint32 i=0; i<m_emitters.Size(); i++ )
	{
		CRenderElement_ParticleEmitter* emitter = m_emitters[i];

		const Uint8 internalPriority = emitter->m_emitter->GetInternalPriority();
		emitter->UpdateDistanceFromPoint( cameraPosition , internalPriority , m_renderPriority );

		if ( autoHideDistanceSquared <= 0.f || emitter->GetDistanceFromCameraSquared() < autoHideDistanceSquared )
		{
			m_didCollect = true;
			collector.PushElement( emitter );
		}
	}

	if( m_didCollect )
	{
		UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleScene );
		collector.PushOnScreenParticle( this );
		m_didCollect = false;
	}
}

void CRenderProxy_Particles::UpdateSimulationContext( const SSimulationContextUpdate & context )
{
	m_simulationContext.m_effectAlpha = context.m_effectAlpha;
	m_simulationContext.m_effectSize = context.m_effectSize;
	m_simulationContext.m_globalEmissionScale = context.m_globalEmissionScale;
	m_simulationContext.m_targetTranslation = context.m_targetTranslation;
	m_simulationContext.m_timeMultiplier = context.m_timeMultiplier;
	m_simulationContext.m_windVector = context.m_windVector;
	SAFE_COPY( m_simulationContext.m_wrapper, context.m_wrapper );
}

void CRenderProxy_Particles::UpdateInterpolator()
{
	if ( !m_simulationContext.m_interpolator )
	{
		m_simulationContext.m_interpolator = new CSubframeInterpolator;
		m_simulationContext.m_interpolator->Init( m_localToWorld );
	}
	else
	{
		m_simulationContext.m_interpolator->Update( m_localToWorld );
	}
}

#ifndef NO_DEBUG_PAGES

namespace
{
	String MeshVertexFactoryToString( EMaterialVertexFactory mvf )
	{
		switch ( mvf )
		{
		case MVF_ParticleBilboard:
			return TXT("BILLBOARD");
		case MVF_ParticleBilboardRain:
			return TXT("RAIN");
		case MVF_ParticleParallel:
			return TXT("HORIZONTAL");
		case MVF_ParticleMotionBlur:				
			return TXT("MOTION BLUR");
		case MVF_ParticleSphereAligned:			
			return TXT("SPHERE ALIGNED");
		case MVF_ParticleVerticalFixed:
			return TXT("VERTICAL");
		case MVF_ParticleTrail:
			return TXT("TRAIL");
		case MVF_ParticleFacingTrail:
			return TXT("FACING TRAIL");
		case MVF_ParticleScreen:
			return TXT("SCREEN");
		case MVF_ParticleBeam:
			return TXT("BEAM");
		}

		return TXT("UNKNOWN");
	}
}

void CRenderProxy_Particles::GetDescription( TDynArray< String >& descriptionLines ) const
{
	String newLine;
	for ( Uint32 j=0; j<m_emitters.Size(); ++j )
	{
		IParticleData* particleData = m_emitters[j]->GetData();
		if ( particleData )
		{
			IParticleBuffer* particleBuffer = particleData->GetParticleBufferInterface();
			if ( particleBuffer )
			{
				String vertexFactoryName = MeshVertexFactoryToString( m_emitters[j]->GetVertexFactory() );
				newLine = String::Printf( TXT("   Vertex type: %s ; Num particles:   %i ; Max particles:   %i ; Spawn counter: %.2f ; Cycle time: %.2f ; Cycle count: %i ; Last burst time: %.2f"), 
					vertexFactoryName.AsChar(), particleBuffer->GetNumParticles(), particleBuffer->GetMaxParticles(), particleData->m_spawnCounter, 
					particleData->m_cycleTime, particleData->m_cycleCount, particleData->m_lastBurstTime );

				descriptionLines.PushBack( newLine );
				const Vector bmin = m_boundingBox.Min;
				const Vector bmax = m_boundingBox.Max;
				newLine = String::Printf( TXT("   Bounding box: MIN: ( %.1f, %.1f, %.1f, %.1f ) ; MAX: ( %.1f, %.1f, %.1f, %.1f )"), bmin.X, bmin.Y, bmin.Z, bmin.W, bmax.X, bmax.Y, bmax.Z, bmax.W );
				descriptionLines.PushBack( newLine );
				descriptionLines.PushBack( TXT("   -----------") );
			}
		}
	}
}

Bool CRenderProxy_Particles::ValidateOptimization( TDynArray< String >* commentLines ) const
{
	Bool result = true;
	for ( Uint32 j=0; j<m_emitters.Size(); ++j )
	{
		IParticleData* particleData = m_emitters[j]->GetData();
		if ( particleData )
		{
			IParticleBuffer* particleBuffer = particleData->GetParticleBufferInterface();
			if ( particleBuffer )
			{
				const Int32 maxParticles = particleBuffer->GetMaxParticles();
				const Int32 activeParticles = particleBuffer->GetNumParticles();
				if ( maxParticles > 10 && activeParticles > 0 && activeParticles < maxParticles * 0.75f )
				{
					result = false;
				}
			}
		}
	}
	if ( !result && commentLines )
	{
		commentLines->PushBack( TXT("Some of the emitters, have TOO HIGH MAX PARTICLES VALUE") );
	}
	return result;
}

#endif

#ifndef NO_EDITOR
void CRenderProxy_Particles::UpdateOrCreateEmitter( CRenderParticleEmitter* emitter, CRenderMaterial* material, CRenderMaterialParameters* materialParameters, EEnvColorGroup envColorGroup )
{
	ASSERT( emitter );
	ASSERT( material );
	ASSERT( materialParameters );

	// Find render element with corresponding render emitter
	for ( Uint32 i=0; i<m_emitters.Size(); ++i )
	{
		if ( m_emitters[i]->GetEmitter()->GetUniqueId() == emitter->GetUniqueId() )
		{
			// Found. Do the update.
			m_emitters[i]->ReplaceEmitter( emitter, material, materialParameters, envColorGroup );

			// Nothing more to do
			return;
		}
	}

	// First time emitter addition. Create render element
	CRenderElement_ParticleEmitter* element = new CRenderElement_ParticleEmitter( this, emitter, material, materialParameters, envColorGroup );
	if ( element->GetMaterial() && element->GetMaterialParams() )
	{
		m_emitters.PushBack( element );
	}
	else
	{
		RED_FATAL_ASSERT( element->GetMaterial() && element->GetMaterialParams(), "RenderElement_ParticleEmitter with no RenderMaterial or RenderMaterialParameters!" );
	}
}

void CRenderProxy_Particles::RemoveEmitter( Int32 uniqueId )
{
	// Find render element with corresponding render emitter
	for ( Uint32 i=0; i<m_emitters.Size(); ++i )
	{
		if ( m_emitters[i]->GetEmitter()->GetUniqueId() == uniqueId )
		{
			// Found. Just kill the element.
			SAFE_RELEASE( m_emitters[i] );
			m_emitters.RemoveAt( i );

			// Nothing more to do.
			return;
		}
	}
}

#endif
