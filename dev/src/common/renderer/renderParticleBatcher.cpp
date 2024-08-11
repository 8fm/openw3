/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderParticleBatcher.h"
#include "renderParticleEmitter.h"
#include "renderProxyParticles.h"
#include "renderElementParticleEmitter.h"
#include "renderMaterial.h"
#include "renderMesh.h"
#include "renderTexture.h"
#include "renderSkinManager.h"
#include "../engine/meshEnum.h"
#include "../engine/particleDrawer.h"
#include "../engine/renderFragment.h"
#include "../core/taskRunner.h"
#include "../core/taskDispatcher.h"

#ifdef USE_ANSEL
	extern Matrix anselCameraTransform;
	extern Bool isAnselSessionActive;
	extern Float anselParticlesPrewarmTime;
#endif // USE_ANSEL

#define PARTICLE_INSTANCING_BUFFER_SIZE 4096

#define MAX_PROXIES_BOUNDING_BOXES 350

#define MAX_SCREEN_EMITTERS 32

namespace Config
{
	TConfigVar< Float > cvMaxOffscreenSimulateTime( "Rendering/Particles", "MaxOffscreenSimTime", 2.0f );
	TConfigVar< Float > cvMaxNonRenderedSimulateTime( "Rendering/Particles", "MaxNoRenderSimTime", 10.0f );

	extern TConfigVar< Int32, Validation::IntRange< 0, 63 > >		cvParticlesLateAllocVSLimit;
}


CRenderPartricleBatcher::CRenderPartricleBatcher()
	: m_frameInstancesDataElemOffset( 0 )
	, m_instancesDataElemOffset( 0 )
	, m_meshParticleBufferDiscardedThisFrame( false )
	, m_simulationStartedThisFrame( false )
	, m_lastGameTime( -1.0f )
{
	m_proxiesBoundingBoxes.Reserve( MAX_PROXIES_BOUNDING_BOXES );
}

CRenderPartricleBatcher::~CRenderPartricleBatcher()
{
	FinishSimulateParticles();

	CParticleVertexBuffer::DeallocateBuffer();

	GpuApi::SafeRelease( m_indexBufferRef );
	GpuApi::SafeRelease( m_instancesBufferRef );

	THashMap< CRenderProxy_Particles*, Box >::iterator it = m_proxiesBoundingBoxes.Begin();
	for ( ; it != m_proxiesBoundingBoxes.End(); ++it )
	{
		it->m_first->Release();
	}
	m_proxiesBoundingBoxes.ClearFast();
}

template< Int32 N >
struct ParticleIndexBuffer
{
	Uint16		m_indices[ N*6 ];

	ParticleIndexBuffer()
	{
		Uint16 baseIndex = 0;
		Uint16* write = &m_indices[0];
		for ( Int32 i=0; i<N; i++, write+=6, baseIndex+=4 )
		{
			write[0] = baseIndex + 0;
			write[1] = baseIndex + 1;
			write[2] = baseIndex + 2;
			write[3] = baseIndex + 0;
			write[4] = baseIndex + 2;
			write[5] = baseIndex + 3;
		}
	}
};

struct CompareEmitterDistance
{
	RED_INLINE Bool operator () ( const CRenderElement_ParticleEmitter* emitterA, const CRenderElement_ParticleEmitter* emitterB ) const
	{
		return 
			emitterA->GetSortingKey() > emitterB->GetSortingKey() ||
			( emitterA->GetSortingKey() == emitterB->GetSortingKey() && emitterA > emitterB );		//< FIX: this is hacky solution to prevent runtime particles flickering in case of equal sortingKeys (may differ between particle systems - depends on address). this is a frequent case for sky particles.
	}
};

void CRenderPartricleBatcher::RenderParticles( const CRenderCollector& collector, const RenderingContext& context, const TDynArray< CRenderElement_ParticleEmitter* >& particles)
{
	PC_SCOPE_PIX( BatcherRenderParticles );

	// Empty crap
	if ( particles.Empty() )
	{
		return;
	}

	// Do not draw particles
	if ( !collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_Particles ) )
	{
		return;
	}

	GpuApi::SetVsWaveLimits( 0, Config::cvParticlesLateAllocVSLimit.Get() );

	// Some locally cached shit
	Int32 lastVertexFactoryIndex = -1;

	// Reset state cache
	m_lastMaterial = NULL;
	m_lastMaterialParameters = NULL;

	// Init local state cache
	Vector lastEnvColor ( -1, -1, -1, 0 );
	Float lastAlpha	= -1.f;

	// Use default selection color for particles
	const Vector defaultSelectionColor( 0.2f, 1.0f, 0.4f, 0.35f );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, defaultSelectionColor );
	const Vector defaultSelectionEffect( 0.0f, 0.0f, 0.0f, 0.0f );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_SelectionEffect, defaultSelectionEffect );

	// Assure that drawcontext won't change in this function
	const CGpuApiScopedDrawContext drawContextOriginalRestore;
	const CGpuApiScopedTwoSidedRender scopedForcedTwoSided;

	// Rendering to GBuffer, mask by lighting channel, enable stencil
	const Bool deferredLighting = (context.m_pass == RP_GBuffer);
	const CGpuApiScopedDrawContext scopedDrawContext;
	if ( deferredLighting )
	{
		GpuApi::eDrawContext curContext = scopedDrawContext.GetOrigDrawContext();
		GpuApi::eDrawContext newContext = GpuApi::GetStencilLightChannelsContext( curContext );
		if ( newContext != curContext )
		{
			const Uint8 lightChannel = LC_DynamicObject;
			GpuApi::SetDrawContext( newContext, lightChannel );
		}
	}

	// Finish simulation job now!
	FinishOnScreenStartOffScreenSimulation( collector );	

	// Sort emitters back to front
	TDynArray< CRenderElement_ParticleEmitter* >& nonConstParticles = const_cast< TDynArray< CRenderElement_ParticleEmitter* >& >( particles );	// Sigh
	Sort( nonConstParticles.Begin(), nonConstParticles.End(), CompareEmitterDistance() );

	// Postpone screen emitters to the end, to save some context rolls
	CRenderElement_ParticleEmitter* screenEmitters[ MAX_SCREEN_EMITTERS ];
	Uint32 numScreenEmitters = 0;

	// Draw non-screen ones
	for ( Uint32 i=0; i<particles.Size(); ++i )
	{
		CRenderElement_ParticleEmitter* cur = particles[i];
		if ( cur->GetEmitter()->GetVertexType() == PVDT_Screen )
		{
			if ( numScreenEmitters == MAX_SCREEN_EMITTERS )
			{
				RED_HALT( "Too many screen particle emitters!!!" );
				continue;
			}
			screenEmitters[numScreenEmitters++] = cur;
		}
		else
		{
			RenderEmitter( collector, context, cur );
		}
	}

	// Draw screen ones
	for ( Uint32 i=0; i<numScreenEmitters; ++i )
	{
		RenderEmitter( collector, context, screenEmitters[i] );
	}

	// Restore default transparency fadeout params	
	GetRenderer()->GetStateManager().SetPixelConst( PSC_TransparencyParams, Vector::ONES );

	GpuApi::ResetVsWaveLimits();
}

void CRenderPartricleBatcher::RenderEmitter(const CRenderCollector& collector, const RenderingContext& context, CRenderElement_ParticleEmitter* element)
{
#ifndef NO_EDITOR
	if ( !element->GetEmitter() ) 
	{
		return;
	}
#endif
	CRenderProxy_Particles *proxy = static_cast< CRenderProxy_Particles * >( element->GetProxy() );

	// Skip empty elements
	IParticleData* data = element->GetData();
	if ( !data )
	{
		return;
	}

	// Get emitter
	CRenderParticleEmitter* emitter = element->GetEmitter();
	ASSERT( emitter );

#ifdef USE_DX10 // TEMPORARY HACK
	if ( emitter->GetVertexType() == PVDT_Rain )
	{
		return;
	}
#endif

	if ( emitter->GetMaxParticles() > MAX_PARTICLES_IN_BATCH )
	{
		// Exceeded batch limit
		return;
	}

	if ( !context.CheckLightChannels( proxy->GetLightChannels() ) )
	{
		// Doesn't match light channel filter.
		return;
	}

	PC_SCOPE_PIX( DrawParticles );

	// Simulation performed, if there are particles to draw, do that
	const Bool hasParticles = ( data->GetParticleBufferInterface()->GetNumParticles() > 0 );
	if ( hasParticles )
	{
		// Calculate particle system fadeout alpha
		Float currAlpha = 1.f * collector.GetRenderFrameInfo().m_envParametersArea.m_colorGroups.GetAlphaForColorGroup(element->GetEnvColorGroup());
		const Float autoHideDistance = proxy->GetAutoHideDistance( collector );
		if ( autoHideDistance > 0 )
		{
			Float distanceSquare = element->GetDistanceFromCameraSquared();
			if ( distanceSquare >= autoHideDistance * autoHideDistance )
			{
				return;
			}

			const Float autoHideInvRange = proxy->GetAutoHideInvRange( collector );
			currAlpha = Min( 1.f, (autoHideDistance - MSqrt(distanceSquare)) * autoHideInvRange );
			ASSERT( currAlpha >= 0.f );
		}

		// Update transparency params
		const EEnvColorGroup currEnvColorGroup = element->GetEnvColorGroup();
		Vector currEnvColor = collector.GetRenderFrameInfo().m_envParametersArea.m_colorGroups.GetCurveForColorGroup(currEnvColorGroup).GetColorScaledGammaToLinear(true);
	
		if ( currEnvColor.X < 0 || currEnvColor.Y < 0 || currEnvColor.Z < 0 )
		{
			currEnvColor = Vector::ZEROS;
			return; // AC: why continue here if color zero? what about alpha blended stuff? anyway if this gets refactored, then one should also update CRenderProxy_Particles::CollectElements
		}

		GetRenderer()->GetStateManager().SetPixelConst( PSC_TransparencyParams, Vector (currEnvColor.X, currEnvColor.Y, currEnvColor.Z, currAlpha) );

		// Update effect params
		const SRenderProxyDrawableEffectParams* effectParams = proxy->GetEffectParams();
		if ( effectParams != NULL )
		{
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, effectParams->m_customParam0 );
		}

		Box bboxToAdd = data->m_box;
		const Vector bboxInfo( bboxToAdd.Min.Z, bboxToAdd.Max.Z, bboxToAdd.Max.Z - bboxToAdd.Min.Z, 0.0f );
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_2, bboxInfo );

		const Float distanceSqForTextures = proxy->AdjustCameraDistanceSqForTextures( proxy->GetCachedDistanceSquared() );
		// Draw using meshes
		if ( emitter->GetParticleType() == RPT_MeshParticle )
		{
			DrawMeshParticles( element, context, distanceSqForTextures );
		}
		else
		{
			// Set world transform if expected
			Matrix transform = Matrix::IDENTITY;
			if ( emitter->m_keepSimulationLocal )
			{
				transform = element->GetProxy()->GetLocalToWorld();
			}

			GetRenderer()->GetStateManager().SetLocalToWorld( &transform );

			// Bind render states

			if ( SetBatchRenderStates( context, element, distanceSqForTextures ) )
			{
				// Determine system vertex type
				EMaterialVertexFactory currVertexFactory = element->GetVertexFactory();
				
				GpuApi::eBufferChunkType systemVertexType = GpuApi::BCT_VertexBillboard;
				switch ( currVertexFactory )
				{
				case MVF_ParticleScreen:		// falldown
				case MVF_ParticleBilboard:		// falldown
				case MVF_ParticleParallel:
				case MVF_ParticleSphereAligned:
				case MVF_ParticleVerticalFixed:
					systemVertexType = GpuApi::BCT_VertexSystemParticleStandard;
					break;

				case MVF_ParticleMotionBlur:
				case MVF_ParticleBilboardRain:
					systemVertexType = GpuApi::BCT_VertexSystemParticleMotionBlur;
					break;

				case MVF_ParticleTrail:
					systemVertexType = GpuApi::BCT_VertexSystemParticleTrail;
					break;
				case MVF_ParticleFacingTrail:
				case MVF_ParticleBeam:
					systemVertexType = GpuApi::BCT_VertexSystemParticleFacingTrail_Beam;
					break;

				default:
					HALT( "Invalid vertex factory for particle rendering" );
					return;
				}

#ifndef RED_FINAL_BUILD
				// Count stats
				{
					IParticleBuffer* particleBuffer = data->GetParticleBufferInterface();
					extern SceneRenderingStats GRenderingStats;
					GRenderingStats.m_numParticleEmitters += 1;
					GRenderingStats.m_numParticles += particleBuffer->GetNumParticles();
				}
#endif

				// TODO: basically particles need batching. Therefore I temporarily put this mostly redundant index buffer bind here.
				GpuApi::BindIndexBuffer( m_indexBufferRef );

				// Bind vertex buffer for drawing. Index buffer should be bound already.

				GpuApi::BufferRef bufferRef = CParticleVertexBuffer::GetCurrentBufferRef();
				const Uint32 stride = GpuApi::GetChunkTypeStride( systemVertexType );
				const Uint32 offset = data->m_byteOffset;
				GpuApi::SetVertexFormatRaw( systemVertexType );
				GpuApi::BindVertexBuffers( 0, 1, &bufferRef, &stride, &offset );

				// Draw call
				GpuApi::DrawIndexedPrimitive( 
					GpuApi::PRIMTYPE_TriangleList, 
					0, 
					0, 
					data->m_numVertices, 
					0, 
					data->m_numVertices / 2 
					);
			}
		}
	}
}

void CRenderPartricleBatcher::SimulateOnScreenParticles(const CRenderCollector& collector)
{
	// No need to simulate sth that is not going to be rendered anyway
	if ( !collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_Particles ) )
	{
		return;
	}

	if ( !CParticleVertexBuffer::IsAllocated() )
	{
		CreateBuffers();
	}

	if( !m_taskOnScreen && collector.m_renderCollectorData->m_particles[SCREENCAT_OnScreen].Size() > 0 )
	{
		CParticleVertexBuffer::Map();

		m_taskOnScreen = new ( CTask::Root ) CSimulateParticlesTask( &collector, &m_proxiesBoundingBoxes, m_velocityTimeDelta, true );
		GTaskManager->Issue( *m_taskOnScreen, TSP_Critical );

		m_simulationStartedThisFrame = true;
	}
}

void CRenderPartricleBatcher::SimulateOffScreenParticles(const CRenderCollector& collector)
{
	if( !m_taskOffScreen && m_simulationStartedThisFrame && collector.m_renderCollectorData->m_particles[SCREENCAT_OffScreen].Size() > 0 )
	{
		m_taskOffScreen = new ( CTask::Root ) CSimulateParticlesTask( &collector, &m_proxiesBoundingBoxes, m_velocityTimeDelta, false );
		GTaskManager->Issue( *m_taskOffScreen, TSP_Normal );
	}
}

void CRenderPartricleBatcher::FinishSimulateOnScreenParticles()
{
	if( m_taskOnScreen )
	{
		PC_SCOPE( FinishOnScreenStartOffScreenSimulation );

		CTaskDispatcher taskDispatcher( *GTaskManager );
		CTaskRunner taskRunner;
		taskRunner.RunTask( *m_taskOnScreen, taskDispatcher );

		while( !m_taskOnScreen->IsFinished() ){RED_BUSY_WAIT();}

		m_taskOnScreen->Release();
		m_taskOnScreen = nullptr;

		CParticleVertexBuffer::Unmap();
	}
}

void CRenderPartricleBatcher::FinishSimulateOffScreenParticles()
{
	if( m_taskOffScreen )
	{
		PC_SCOPE( FinishSimulateOffScreenParticles );

		// If the task wasn't already running skip it this frame as we're too busy anyway.
		// World ain't gonna end without off-screen particles simulation.
		if ( m_taskOffScreen->MarkRunning() )
		{
			m_taskOffScreen->MarkFinished();
		}

		while ( !m_taskOffScreen->IsFinished() ){RED_BUSY_WAIT();}

		m_taskOffScreen->Release();
		m_taskOffScreen = nullptr;
	}
}

void CRenderPartricleBatcher::FinishSimulateParticles()
{
	FinishSimulateOnScreenParticles();
	FinishSimulateOffScreenParticles();
}

void CRenderPartricleBatcher::FinishOnScreenStartOffScreenSimulation( const CRenderCollector& collector )
{
	FinishSimulateOnScreenParticles();
	SimulateOffScreenParticles( collector );
}

void CRenderPartricleBatcher::SimulateEmittersOverTime( CRenderProxy_Particles* proxyToUpdate, Float timeDeltaLeft, Float simulationFrequencyMultiplier )
{
	PC_SCOPE( SimulateEmittersOverTime );

	FinishSimulateParticles();

	// Limit the time delta to some sane value
	const Float maxParticleUpdateTime = 
#ifdef USE_ANSEL
		isAnselSessionActive ? anselParticlesPrewarmTime :
#endif
		Config::cvMaxNonRenderedSimulateTime.Get();	

	timeDeltaLeft = Min( timeDeltaLeft, maxParticleUpdateTime );

	// Prepare to update bounding box.
	Box* bbox = m_proxiesBoundingBoxes.FindPtr( proxyToUpdate );
	if ( !bbox )
	{
		RED_ASSERT( m_proxiesBoundingBoxes.Size() < MAX_PROXIES_BOUNDING_BOXES, TXT("Max proxies bounding boxes value exceeded the limit. Consider optimizing amount of particles present in the scene or increase the limit.") );

		proxyToUpdate->AddRef();
		m_proxiesBoundingBoxes.Insert( proxyToUpdate, proxyToUpdate->GetBoundingBox() );
		bbox = m_proxiesBoundingBoxes.FindPtr( proxyToUpdate );
	}

#ifdef USE_ANSEL
	if ( isAnselSessionActive )
	{
		proxyToUpdate->m_simulationContext.m_cameraPosition = anselCameraTransform.GetTranslation();
	}
#endif // USE_ANSEL

	// Update simulation context transform
	proxyToUpdate->m_simulationContext.m_componentTransform = proxyToUpdate->GetLocalToWorld();

	const TDynArray< CRenderElement_ParticleEmitter* >& emitters = proxyToUpdate->GetEmitters();
	for ( Uint32 eIdx = 0; eIdx < emitters.Size(); ++eIdx )
	{
		CRenderElement_ParticleEmitter* emitter = emitters[ eIdx ];
		IParticleData* data = emitter->GetData();

		if ( !data )
		{
			continue;
		}

		CRenderParticleEmitter* renderEmitter = emitter->GetEmitter();
		RED_ASSERT( renderEmitter, TXT("Trying to simulate particle emitter without proper data") );

#ifdef USE_DX10 // TEMPORARY HACK
		if ( renderEmitter->GetVertexType() == PVDT_Rain )
		{
			continue;
		}
#endif
		Float freq = renderEmitter->GetEmitterDuration( proxyToUpdate->m_simulationContext.m_lod ) * simulationFrequencyMultiplier;
		Float timeLeft = timeDeltaLeft;

		while ( timeLeft > 0 )
		{
			proxyToUpdate->m_simulationContext.m_timeDelta = Min( timeLeft, freq );

			data->MarkAsNotUpdatedThisFrame();
			RED_ASSERT( !data->WasUpdatedThisFrame(), TXT("Offscreen particle simulate has an emitter that was already updated?") );

			// Update interpolator if sub frame emission is desired
			if ( !data->WasUpdatedThisFrame() && renderEmitter->m_useSubFrameEmission )
			{
				proxyToUpdate->UpdateInterpolator();
			}

			const Bool onScreen = 
#ifdef USE_ANSEL
				isAnselSessionActive;
#else
				false;
#endif // USE_ANSEL

			renderEmitter->Simulate( data, proxyToUpdate->m_simulationContext, onScreen );
			bbox->AddBox( CParticleVertexBuffer::BoundingBox() );
			
			timeLeft -= freq;
		}
	}
}

void CRenderPartricleBatcher::ComputeMeshParticleFreeOrientation( const MeshParticle* particle, const Vector3& position, EulerAngles angles, Matrix& transform )
{
	Float roll = particle->m_rotation.X * 360.0f;
	Float yaw = particle->m_rotation.Z * 360.0f;
	Float pitch = particle->m_rotation.Y * 360.0f;

	angles.Roll += roll;
	angles.Pitch += pitch;
	angles.Yaw += yaw;

	transform = angles.ToMatrix();
	transform.SetTranslation( position );
	transform.SetScale33( particle->m_size );
}

void CRenderPartricleBatcher::ComputeMeshParticleFreeNoRotOrientation( const MeshParticle* particle, const Vector3& position, Matrix& transform )
{
	Float roll = particle->m_rotation.X * 360.0f;
	Float yaw = particle->m_rotation.Z * 360.0f;
	Float pitch = particle->m_rotation.Y * 360.0f;

	EulerAngles angles( roll, pitch, yaw );
	transform = angles.ToMatrix();
	transform.SetTranslation( position );
	transform.SetScale33( particle->m_size );
}

void CRenderPartricleBatcher::ComputeMeshParticleMovementDirectionOrientation( const MeshParticle* particle, const Vector3& position, Matrix& transform )
{
	Float yaw = particle->m_rotation.Z * 2.0f * M_PI;
	Float pitch = particle->m_rotation.Y * 2.0f * M_PI;
	Float roll = particle->m_rotation.X * 2.0f * M_PI;
#ifdef USE_HAVOK_ANIMATION
	Vector forward = particle->m_velocity.Normalized();
	Vector side = Vector::Cross( forward, Vector::EZ ).Normalized3();

	// Adjust pitch
	RotateVector( forward, side, pitch );

	// Get up
	Vector up = Vector::Cross( side, forward );

	// Adjust yaw
	RotateVector( forward, up, yaw );

	// Get side
	side = Vector::Cross( forward, up );

	// Adjust roll
	RotateVector( side, forward, roll );

	// Get up again
	up = Vector::Cross( side, forward );

	transform.SetRows( side, forward, up, Vector::EW );
	transform.SetTranslation( position );
	transform.SetScale33( particle->m_size );
#else
	Vector3 normalVel = particle->m_velocity.Normalized();
	RedVector4 forward(normalVel.X, normalVel.Y, normalVel.Z, 0.0f );
	RedVector4 side = Cross( forward, RedVector4::EZ ).Normalized3();

	// Adjust pitch
	AxisRotateVector( forward, side, pitch );

	// Get up
	RedVector4 up = Cross( side, forward );

	// Adjust yaw
	AxisRotateVector( forward, up, yaw );

	// Get side
	side = Cross( forward, up );

	// Adjust roll
	AxisRotateVector( side, forward, roll );

	// Get up again
	up = Cross( side, forward );

	transform.SetRows( reinterpret_cast< const Vector& >( side ), reinterpret_cast< const Vector& >( forward ), reinterpret_cast< const Vector& >( up ), Vector::EW );
	transform.SetTranslation( Vector( position.X, position.Y, position.Z, 0.0f ) );
	transform.SetScale33( particle->m_size );
#endif
}

void CRenderPartricleBatcher::DrawMeshParticles( CRenderElement_ParticleEmitter* emitter, const RenderingContext& context, Float proxyDistance )
{
	CRenderParticleEmitter* renderEmitter = emitter->GetEmitter();
	ASSERT( renderEmitter );

	TDynArray< CRenderMesh* >& renderMeshes = renderEmitter->m_renderMeshes;
	const Uint32 numMeshes = renderMeshes.Size();
	if ( numMeshes == 0 )
	{
		return;
	}

	if ( !m_instancesBufferRef )
	{
		// Create the instance buffer
		const Uint32 chunkSize = sizeof( SParticleInstanceDescriptor );
		const Uint32 instanceDataSize = chunkSize * PARTICLE_INSTANCING_BUFFER_SIZE;
		m_instancesBufferRef = GpuApi::CreateBuffer( instanceDataSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		GpuApi::SetBufferDebugPath( m_instancesBufferRef, "batchedParticleInstance" );
		ASSERT( m_instancesBufferRef );
	}

	// Setup mesh particles stencil ref
	if ( GpuApi::IsStencilLightChannelsContext( GpuApi::GetDrawContext() ) )
	{
		const Uint8 lightChannel = renderEmitter->m_drawerData.mesh.m_lightChannels | LC_DynamicObject;
		GpuApi::SetDrawContext( GpuApi::GetDrawContext(), lightChannel );
	}

	// Select material and bind parameters
	CRenderMaterial* material = emitter->GetMaterial();

	GpuApi::SetForcedTwoSidedRender( material->IsTwoSided() );

	// Reset local to world matrix
	GetRenderer()->GetStateManager().SetLocalToWorld( nullptr );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_FXColor, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );

	// Get data required to process mesh particles data
	IRenderProxyDrawable* particleProxy = emitter->GetProxy();
	const Matrix& componentTransform = particleProxy->GetLocalToWorld();
	IParticleData* data = emitter->GetData();
	IParticleBuffer* buffer = data->GetParticleBufferInterface();

	// We don't want to exceed instancing buffer twice in one frame (no multiple discards per frame!)
	Uint32 particlesCount = buffer->GetNumParticles();
	if( m_frameInstancesDataElemOffset + particlesCount > PARTICLE_INSTANCING_BUFFER_SIZE )
	{
		RED_LOG( RED_LOG_CHANNEL(Particles), TXT("The number of mesh particles per frame exceeded. Please optimize your assets.") );
		return;
	}

	Matrix finalMatrix;
	const EulerAngles baseAngles = componentTransform.ToEulerAngles();
	EulerAngles angles;
	MeshDrawingStats outMeshStats;
	const Bool keepSimulationLocal = renderEmitter->m_keepSimulationLocal;
	const EMeshParticleOrientationMode orientationMode = renderEmitter->m_drawerData.mesh.m_orientationMode;

	if( m_instances.Size() < numMeshes )
	{
		m_instances.Grow( numMeshes - m_instances.Size() );
	}
	for( Uint32 i = 0; i < numMeshes; ++i )
	{
		m_instances[i].Reserve( particlesCount );
	}

	for( Uint32 particleIdx = 0; particleIdx < particlesCount; ++particleIdx )
	{
		const MeshParticle* particle = ( MeshParticle* )buffer->GetParticleDataAt( particleIdx );
		Uint32 meshIndex = Red::Math::NumericalUtils::Min( (Uint32)particle->m_frame, numMeshes - 1 );

		// Compute position and vertex color for i'th particle
		Vector3 position = particle->m_position;
		if ( keepSimulationLocal )
		{
			// If simulation locality enabled, move particle according to particle component movement
			position = componentTransform.TransformPoint( position );
		}

		// Orientate particles due to selected mode
		if ( orientationMode == MPOM_MovementDirection )
		{
			ComputeMeshParticleMovementDirectionOrientation( particle, position, finalMatrix );
		}
		else if( orientationMode == MPOM_Normal )
		{
			ComputeMeshParticleFreeOrientation( particle, position, baseAngles, finalMatrix );
		}
		else
		{
			ComputeMeshParticleFreeNoRotOrientation( particle, position, finalMatrix );
		}

		// Per instance data:
		SParticleInstanceDescriptor instance;

		RED_STATIC_ASSERT( 3 * 4 == ARRAY_COUNT( instance.m_localToWorld ) );
		finalMatrix.GetColumnMajor3x4( instance.m_localToWorld );
		instance.m_color.X = particle->m_color.X;
		instance.m_color.Y = particle->m_color.Y;
		instance.m_color.Z = particle->m_color.Z;
		instance.m_color.W = particle->m_alpha;
		m_instances[meshIndex].PushBack(instance);
	}

	{// bind default dissolve texture and sampler in case the mesh particles have a masked material
		GpuApi::TextureRef dissolvePattern = GpuApi::GetInternalTexture( GpuApi::INTERTEX_DissolvePattern);
		GpuApi::BindTextures( PSSMP_UVDissolve, 1, &dissolvePattern, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( PSSMP_UVDissolve, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip );
	}

	for( Uint32 meshInd = 0; meshInd < numMeshes; ++meshInd )
	{
		// Can't draw if not loaded.
		if ( !renderMeshes[meshInd]->IsFullyLoaded() )
		{
			m_instances[meshInd].ClearFast();
			continue;
		}

		const Uint32 numChunks = renderMeshes[meshInd]->GetNumChunks();
		for ( Uint32 j = 0; j < numChunks; ++j )
		{
			// Get chunk info
			const CRenderMesh::Chunk& chunk = renderMeshes[meshInd]->GetChunks()[j];

			// Skip merged chunks
			if ( !(chunk.m_baseRenderMask & MCR_Scene) )
				continue;
			
			// Prepare material context
			MaterialRenderingContext materialContext( context );
			materialContext.m_vertexFactory = (EMaterialVertexFactory)chunk.m_vertexFactory;
			materialContext.m_discardingPass = emitter->GetMaterialParams()->IsMasked();

			// we don't support non-static (skinned especially) instanced meshes yet
			if( materialContext.m_vertexFactory == MVF_MeshStatic )
			{
				materialContext.m_useInstancing = true;
				materialContext.m_useParticleInstancing = true; // this is for the color component of the instanced particles
			}
			else
			{
				// bind skin texture
				GetRenderer()->GetSkinManager()->BindSkinningBuffer();

				const Vector& skinData = GetRenderer()->GetSkinManager()->GetDefaultBindData();
				GpuApi::SetVertexShaderConstF( VSC_SkinningData, &(skinData.A[0]), 1 );
			}

			GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, Vector( emitter->GetMaterialParams()->IsMasked(), 0, 0, 0 ) );

			// Bind material
			if ( material->Bind( materialContext, emitter->GetMaterialParams(), proxyDistance ) )
			{
				// Draw part
				renderMeshes[meshInd]->Bind( j );
				if( materialContext.m_useInstancing )
				{
					DrawMeshParticlesInstanced( renderMeshes[meshInd], j, m_instances[meshInd], outMeshStats );
				}
				else
				{
					for ( Uint32 i = 0; i < m_instances[meshInd].Size() ; ++i )
					{
						GetRenderer()->GetStateManager().SetLocalToWorldF( m_instances[meshInd][i].m_localToWorld );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_FXColor, m_instances[meshInd][i].m_color );
						renderMeshes[meshInd]->DrawChunkNoBind( j, 1, outMeshStats );
					}
				}
			}
		}

		// Clear instancing data
		m_instances[meshInd].ClearFast();
	}

#ifndef RED_FINAL_BUILD
	// update stats
	{
		extern SceneRenderingStats GRenderingStats;
		GRenderingStats.m_numParticleEmitters += 1;
		GRenderingStats.m_numParticleMeshChunks += outMeshStats.m_numChunks;
		GRenderingStats.m_numParticleMeshTriangles += outMeshStats.m_numChunks;
	}
#endif

	////////////////////////////////////////////////////////////////////////////
}

void CRenderPartricleBatcher::DrawMeshParticlesInstanced(CRenderMesh* mesh, Uint32 chunkIndex, const TDynArray< SParticleInstanceDescriptor >& instances, MeshDrawingStats& outMeshStats)
{
	Uint32 numInstances = instances.Size();
	if ( !instances.Empty() && numInstances <= PARTICLE_INSTANCING_BUFFER_SIZE )
	{
		static const Uint32 streamStride = sizeof( SParticleInstanceDescriptor );
		void *instancedPtr = nullptr;

		// Lock the instance buffer for write
		if ( m_instancesDataElemOffset + numInstances >= PARTICLE_INSTANCING_BUFFER_SIZE )
		{
			if( m_meshParticleBufferDiscardedThisFrame )
			{
				RED_LOG( RED_LOG_CHANNEL(Particles), TXT("The number of mesh particles per frame exceeded. Please optimize your assets.") );
				return;
			}

			const Int32 lockOffset = 0;
			const Uint32 lockSize = streamStride * numInstances;
			instancedPtr = GpuApi::LockBuffer( m_instancesBufferRef, GpuApi::BLF_Discard, lockOffset, lockSize );

			m_instancesDataElemOffset = 0;
			m_meshParticleBufferDiscardedThisFrame = true;
		}
		else
		{
			const Uint32 lockOffset = streamStride * m_instancesDataElemOffset;
			const Uint32 lockSize = streamStride * numInstances;
			instancedPtr = GpuApi::LockBuffer( m_instancesBufferRef, GpuApi::BLF_NoOverwrite, lockOffset, lockSize );
		}

		// Emit to instance buffer
		for ( Uint32 i = 0; i < instances.Size(); ++i )
		{
			// Import instance data
			Red::System::MemoryCopy( instancedPtr, &instances[i], streamStride );
			instancedPtr = static_cast<Uint8*>(instancedPtr) + streamStride;
		}

		// Return the buffer
		GpuApi::UnlockBuffer( m_instancesBufferRef );

		// Bind
		Uint32 offset = m_instancesDataElemOffset*streamStride;
		GpuApi::BindVertexBuffers( 7, 1, &m_instancesBufferRef, &streamStride, &offset );

		// Draw
		mesh->DrawChunkNoBindInstanced( chunkIndex, 1, numInstances, outMeshStats );

		// Clear intermediate instancing data
		instancedPtr = nullptr;

		// Update offset in instances buffer
		m_frameInstancesDataElemOffset += numInstances;
		m_instancesDataElemOffset += numInstances;

#ifndef RED_FINAL_BUILD
		if ( numInstances > 1 ) outMeshStats.m_numInstancedBatches += 1;
		outMeshStats.m_biggestInstancedBatch = Max( outMeshStats.m_biggestInstancedBatch, numInstances );
		outMeshStats.m_smallestInstancedBatch = Min( outMeshStats.m_smallestInstancedBatch, numInstances );
#endif
	}
}

Bool CRenderPartricleBatcher::SetBatchRenderStates( const RenderingContext& context, CRenderElement_ParticleEmitter* batch, Float proxyDistance )
{
	// Bind vertex type
	EMaterialVertexFactory vertexFactory = batch->GetVertexFactory();

	// Setup rendering context
	MaterialRenderingContext materialContext( context );
	materialContext.m_selected = batch->GetProxy()->IsSelected();
	materialContext.m_vertexFactory = vertexFactory;
	materialContext.m_discardingPass = batch->GetMaterialParams()->IsMasked();

	GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, Vector( batch->GetMaterialParams()->IsMasked(), 0, 0, 0 ) );

	if ( batch->GetMaterial()->Bind( materialContext, batch->GetMaterialParams(), proxyDistance ) )
	{
		// Update crap
		m_lastMaterial = batch->GetMaterial();
		m_lastMaterialParameters = batch->GetMaterialParams();
		GpuApi::SetForcedTwoSidedRender( true );

		return true;
	}

	return false;
}

CName CRenderPartricleBatcher::GetCategory() const
{
	return CNAME( RenderParticleBatcher );
}

void CRenderPartricleBatcher::OnDeviceLost()
{

}

void CRenderPartricleBatcher::OnDeviceReset()
{

}

Uint32 CRenderPartricleBatcher::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderPartricleBatcher::OnNewFrame()
{
	PC_SCOPE_PIX(CRenderPartricleBatcher::OnNewFrame);

	m_frameInstancesDataElemOffset = 0;
	m_simulationStartedThisFrame = false;
	m_meshParticleBufferDiscardedThisFrame = false;

	Float engineTime = GGame->GetCleanEngineTime();
	if ( m_lastGameTime < 0.0 )
	{
		// Have to initialize last game time value
		m_lastGameTime = engineTime;
	}

	m_velocityTimeDelta = engineTime - m_lastGameTime;

	// Rember last update time
	m_lastGameTime = engineTime;

	CParticleVertexBuffer::OnNewFrame();
}

void CRenderPartricleBatcher::OnNewFrameScreenshot()
{
	m_frameInstancesDataElemOffset = 0;
	m_simulationStartedThisFrame = false;
	m_meshParticleBufferDiscardedThisFrame = false;

	CParticleVertexBuffer::OnNewFrame();
}

void CRenderPartricleBatcher::RelinkProxies()
{
	PC_SCOPE(RelinkParticleProxies);

	FinishSimulateParticles();

	THashMap< CRenderProxy_Particles*, Box >::iterator it = m_proxiesBoundingBoxes.Begin();
	for ( ; it != m_proxiesBoundingBoxes.End(); ++it )
	{
		it->m_first->RelinkBBoxOnly( it->m_second );
		it->m_first->Release();
	}
	m_proxiesBoundingBoxes.ClearFast();
}

void CRenderPartricleBatcher::CreateBuffers()
{
	RED_ASSERT( !CParticleVertexBuffer::IsAllocated(), TXT("Particle VB already created") );
	if ( CParticleVertexBuffer::IsAllocated() ) return;

	// Allocate vertex buffer
	CParticleVertexBuffer::AllocateBuffer();

	// Allocate index buffer
	if ( m_indexBufferRef.isNull() )
	{
		// Create for the first time
		ParticleIndexBuffer< MAX_PARTICLES_IN_BATCH * 6 > indices;
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = indices.m_indices;
		m_indexBufferRef = GpuApi::CreateBuffer(  MAX_PARTICLES_IN_BATCH * 6 * 2, GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
		GpuApi::SetBufferDebugPath( m_indexBufferRef, "particleIB" );
	}
}

CSimulateParticlesTask::CSimulateParticlesTask( const CRenderCollector* collector, THashMap< CRenderProxy_Particles*, Box >* proxiesBoundingBoxes, Float velocityTimeDelta, Bool onScreen )
	: CTask( 0 )
	, m_onScreenParticles( onScreen )
	, m_collector( collector )
	, m_proxiesBoundingBoxes( proxiesBoundingBoxes )
	, m_velocityTimeDelta( velocityTimeDelta )
{

}

void CSimulateParticlesTask::Run()
{
	// Simulate OnScreen particles
	SimulateParticles();
}

void CSimulateParticlesTask::SimulateParticles()
{
	// Get particles from collector
	const TDynArray< CRenderElement_ParticleEmitter* >& particles = 
		m_onScreenParticles ? m_collector->m_renderCollectorData->m_particles[SCREENCAT_OnScreen] : 
							  m_collector->m_renderCollectorData->m_particles[SCREENCAT_OffScreen];

	// Empty crap
	if ( particles.Empty() )
	{
		return;
	}

	PC_SCOPE_PIX( SimulateParticles );

	for ( Uint32 i=0; i<particles.Size(); ++i )
	{
		CRenderElement_ParticleEmitter* cur = particles[i];
#ifndef NO_EDITOR
		if ( !cur->GetEmitter() )
		{
			continue;
		}
#endif
		CRenderProxy_Particles *proxy = static_cast< CRenderProxy_Particles * >( cur->GetProxy() );

		// Skip empty elements
		IParticleData* data = cur->GetData();
		if ( !data )
		{
			continue;
		}

		// Get emitter
		CRenderParticleEmitter* emitter = cur->GetEmitter();
		ASSERT( emitter );

#ifdef USE_DX10 // TEMPORARY HACK
		if ( emitter->GetVertexType() == PVDT_Rain )
		{
			continue;
		}
#endif

		if ( emitter->GetMaxParticles() > MAX_PARTICLES_IN_BATCH )
		{
			// Exceeded batch limit
			continue;
		}

		// Update simulation context transform
		SSimulationContext& simulationContext = proxy->m_simulationContext;
		const Matrix& localToWorld = proxy->GetLocalToWorld();
		const Vector& position = localToWorld.GetTranslationRef();
		if( !data->GetDrawerCache().IsSet() )
		{
			data->GetDrawerCache().SetLastPosition( position );
		}
		
		simulationContext.m_componentTransform = proxy->GetLocalToWorld();
		simulationContext.m_cameraPosition = m_collector->GetRenderCamera().GetPosition();
		
		Vector velocity = Vector::ZEROS;
		if ( m_velocityTimeDelta > 0.0f )
		{
			velocity = ( position - data->GetDrawerCache().m_lastPosition ) / m_velocityTimeDelta;
		}
		simulationContext.m_timeDelta = m_velocityTimeDelta * simulationContext.m_timeMultiplier;
		simulationContext.m_velocity = velocity;

		data->GetDrawerCache().SetLastPosition( position );

		// Update interpolator if sub frame emission is desired
		if ( !data->WasUpdatedThisFrame() && emitter->m_useSubFrameEmission )
		{
			proxy->UpdateInterpolator();
		}

		// Simulate!
		emitter->Simulate( data, simulationContext, m_onScreenParticles );

		// Prepare to update bounding box.
		Box* bbox = m_proxiesBoundingBoxes->FindPtr( proxy );
		if ( !bbox )
		{
			proxy->AddRef();
			m_proxiesBoundingBoxes->Insert( proxy, Box() );
			bbox = m_proxiesBoundingBoxes->FindPtr( proxy );
			bbox->Clear();
		}

		// Simulation performed, if there are particles to draw, do that
		const Bool hasParticles = ( data->GetParticleBufferInterface()->GetNumParticles() > 0 );
		if ( hasParticles )
		{
			IParticleBuffer* particleBuffer = data->GetParticleBufferInterface();

			// Use kosher dynamic bounding box
			data->m_box = CParticleVertexBuffer::BoundingBox();
		}
		else
		{
			// No particles to draw, use a minimalistic bounding box for further visibility testing.
			if ( emitter->m_keepSimulationLocal )
			{
				data->m_box = Box( Vector( -0.5f,-0.5f,-0.5f ), Vector( 0.5f,0.5f,0.5f ) );
			}
			else
			{
				data->m_box = proxy->GetLocalToWorld().TransformBox( Box( Vector( -0.5f,-0.5f,-0.5f ), Vector( 0.5f,0.5f,0.5f ) ) );
			}
		}

		// Collect bounding box
		const EMaterialVertexFactory currVertexFactory = cur->GetVertexFactory();
		if ( currVertexFactory == MVF_ParticleBilboardRain || currVertexFactory == MVF_ParticleScreen )
		{
			bbox->AddBox( Box( Vector(-10000,-10000,-10000), Vector(10000,10000,10000) ) );
		}
		else if ( emitter->m_keepSimulationLocal )
		{
			bbox->AddBox( proxy->GetLocalToWorld().TransformBox( data->m_box ) );
		}
		else
		{
			bbox->AddBox( data->m_box );
		}
	}
}
