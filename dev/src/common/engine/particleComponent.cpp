/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "particleComponent.h"
#include "effectDummyPoint.h"
#include "../physics/physicsParticleWrapper.h"
#include "renderCommands.h"
#include "particleSystem.h"
#include "particleEmitter.h"
#include "../core/gatheredResource.h"
#include "../core/resourceUsage.h"
#include "../renderer/renderProxy.h"
#include "../physics/physicsSettings.h"
#include "layer.h"
#include "tickManager.h"
#include "world.h"
#include "worldShowFlags.h"
#include "bitmapTexture.h"
#include "baseEngine.h"
#include "entity.h"
#include "../physics/physicsWorld.h"
#include "physicsDataProviders.h"

IMPLEMENT_ENGINE_CLASS( CParticleComponent );
CGatheredResource resParticleIcon( TXT("engine\\textures\\icons\\particleicon.xbm"), RGF_NotCooked );

CParticleComponent::CParticleComponent()
	: m_particleSystem( NULL )
	, m_transparencySortGroup( TSG_Scene )
	, m_globalEmissionScale( 1.0f )
	, m_envAutoHideGroup( EAHG_None )
	, m_targetBoneIndex( -1 )
	, m_wrapper( nullptr )
#ifndef NO_EDITOR
	, m_timeMultiplier( 1.0f )
#endif
	, m_updateSimulationContext( true )
{
}

CParticleComponent::~CParticleComponent()
{
	if( m_wrapper )
	{
		m_wrapper->Release();
		m_wrapper = 0;
	}
}

CBitmapTexture* CParticleComponent::GetSpriteIcon() const
{
	return resParticleIcon.LoadAndGet< CBitmapTexture >();
}

Color CParticleComponent::CalcSpriteColor() const
{
	if ( IsSelected() )
	{
		return Color::GREEN;
	}
	else
	{
		return Color::WHITE;
	}
}

Vector3 CParticleComponent::GetWindAtPos( Bool useforces)
{
	CWorld* world;
#ifndef NO_EDITOR
	world = GetEntity()->GetLayer()->GetWorld();
#else
	world = GGame->GetActiveWorld();
#endif
	if( world )
	{
		return world->GetWindAtPointForVisuals( m_localToWorld.GetTranslationRef(), false, useforces );
	}

	return Vector3(0,0,0);
}

void CParticleComponent::Reset()
{
	// Recreate proxy
	if ( IsAttached() && GetLayer()->IsAttached() )
	{
		CWorld* world = GetLayer()->GetWorld();
		ConditionalAttachToRenderScene( world );		
		CreatePhysicalWrapper();

		m_updateSimulationContext = true;
	}
}

void CParticleComponent::OnAttached( CWorld* world )
{
	// Add to editor fragment group
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BboxesParticles );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sprites );

	if( m_particleSystem )
	{
		ASSERT( !m_isRegisteredForComponentLODding );
		m_isRegisteredForComponentLODding = true;
		world->GetComponentLODManager().Register( this );
	}

	// Pass to base class
	TBaseClass::OnAttached( world );
	
	// All particle related updates happen here
	world->GetTickManager()->AddToGroup( this, TICK_PostPhysics );
}

void CParticleComponent::OnDetached( CWorld* world )
{
	if( m_wrapper )
	{
		m_wrapper->Release();
		m_wrapper = 0;
	}

	if ( m_isRegisteredForComponentLODding )
	{
		world->GetComponentLODManager().Unregister( this );
		m_isRegisteredForComponentLODding = false;
	}

	// All particle related updates happen here
	world->GetTickManager()->RemoveFromGroup( this, TICK_PostPhysics );
	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );

	// Pass to base class
	TBaseClass::OnDetached( world );

	// Remove from editor fragments group
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BboxesParticles );
}

Float CParticleComponent::GetAutoHideDistance() const
{
	return GetParticleSystem() ? GetParticleSystem()->GetAutoHideDistance() : GetDefaultAutohideDistance();
}

void CParticleComponent::OnDestroyed()
{	
	// Pass to base class
	TBaseClass::OnDestroyed();
}

void CParticleComponent::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );
}

void CParticleComponent::OnInitializeProxy()
{
	TBaseClass::OnInitializeProxy();

	CreatePhysicalWrapper();

	m_updateSimulationContext = true;

#ifdef USE_ANSEL
	extern Bool isAnselSessionActive;
	if ( isAnselSessionActive )
	{
		if ( m_updateSimulationContext )
		{
			SUpdateTransformContext context;

			SSimulationContextUpdate simulationContext;
			SetSimulationContext( simulationContext );

			// Upload rendering data on the rendering side
			context.m_skinningContext.AddCommand_UpdateParticlesSimulatationContext( m_renderProxy, simulationContext );

			context.CommitChanges();
		}
	}
#endif // USE_ANSEL
}

void CParticleComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CParticleComponent );

	// Omit drawable component, it relinks proxy which we don't want to do here
	CBoundedComponent::OnUpdateTransformComponent( context, prevLocalToWorld );

	// THIS IS A MODIFIED CODE FROM DRAWABLE COMPONENT. REMEMBER TO MAKE CHANGES IN BOTH FILES
	// Update in scene
	if ( IsAttached() && m_renderProxy && GGame && GEngine->IsActiveSubsystem( ES_Particles ) )
	{
		if ( m_localToWorld != prevLocalToWorld )
		{
			if ( IsCameraTransformComponentWithoutRotation() || IsCameraTransformComponentWithRotation() )
			{
				// Ok, this logic is strange and kinda hacky: if there is any attachment, it should be special kind of attachment, like
				// sky transform, moon or sun or sth and it's transform is already local one to the camera - so matrix calculated like 
				// transform local * this local is ok; if there is no local transform, we store local transform matrix in local to world (HACK :( )
				// not to use extra memory
				if ( !m_transformParent )
				{
					CalcLocalTransformMatrix( m_localToWorld );
				}
			}

			RenderProxyUpdateInfo info;
			info.m_localToWorld = &m_localToWorld;

			// Upload rendering data on the rendering side
			context.m_skinningContext.AddCommand_ParticlesRelink( m_renderProxy, info, GetWindAtPos(), GetWindAtPos(false) );
		}

#ifndef NO_EDITOR
	RED_PROFILING_TIMER_SCOPED( m_lastSimulationTime );
#endif

		const CNode* simulationTarget = m_simulationTarget.Get();
		if( m_updateSimulationContext || simulationTarget )
		{
			SSimulationContextUpdate simulationContext;
			SetSimulationContext( simulationContext );

			// Upload rendering data on the rendering side
			context.m_skinningContext.AddCommand_UpdateParticlesSimulatationContext( m_renderProxy, simulationContext );

			m_updateSimulationContext = false;
		}
	}
}

void CParticleComponent::OnUpdateBounds()
{
	TBaseClass::OnUpdateBounds();
}

void CParticleComponent::SetGlobalEmissionScale( Float scale )
{
	Float newEmissionScale = Max< Float >( 0.0f, scale ); 
	if( m_globalEmissionScale != newEmissionScale )
	{
		m_globalEmissionScale = newEmissionScale;
		m_updateSimulationContext = true;
	}
}

void CParticleComponent::SetEffectInfo( const SEffectInfo& info )
{
	if( m_effectInfo.m_alpha != info.m_alpha || m_effectInfo.m_size != info.m_size )
	{
		m_effectInfo = info;
		m_updateSimulationContext = true;
	}
}

void CParticleComponent::SetSimulationContext( SSimulationContextUpdate& simulationContext )
{
	// Setup simulation context
	simulationContext.m_globalEmissionScale = m_globalEmissionScale;
	simulationContext.m_effectAlpha = m_effectInfo.m_alpha;
	simulationContext.m_effectSize = m_effectInfo.m_size;
#ifndef NO_EDITOR
	simulationContext.m_timeMultiplier = m_timeMultiplier;
#endif
	simulationContext.m_windVector = GetWindAtPos();

	if( m_wrapper )
	{
		simulationContext.m_wrapper = m_wrapper;
		m_wrapper->AddRef();
	}

	const CNode* simulationTarget = m_simulationTarget.Get();
	if ( simulationTarget )
	{
		if ( m_targetBoneIndex >= 0 )
		{
			if ( const CEntity* entity = Cast< CEntity >( simulationTarget ) )
			{
				if( CAnimatedComponent* animatedComponent = entity->GetRootAnimatedComponent() )
				{
					simulationContext.m_targetTranslation = animatedComponent->GetBoneMatrixWorldSpace( m_targetBoneIndex ).GetTranslationRef();
				}
			}
		}
		else
		{
			simulationContext.m_targetTranslation = simulationTarget->GetLocalToWorld().GetTranslationRef();
		}
	}
}

void CParticleComponent::SetTarget( const CNode* target, const CName& bone /* = CName::NONE */ )
{
	m_simulationTarget = target;

	m_targetBoneIndex = -1;
	if ( bone != CName::NONE )
	{
		const CNode* simulationTarget = m_simulationTarget.Get();
		if ( simulationTarget )
		{
			if ( const CEntity* entity = Cast< CEntity >( simulationTarget ) )
			{
				if ( CAnimatedComponent* animatedComponent = entity->GetRootAnimatedComponent() )
				{
					m_targetBoneIndex = animatedComponent->FindBoneByName( bone );
				}
			}
			if ( m_targetBoneIndex < 0 )
			{
				WARN_ENGINE( TXT( "Used target bone name '%ls' for effect, but it is not present in '%ls'" ),
					bone.AsString().AsChar(), simulationTarget->GetFriendlyName().AsChar() );
			}

			// We do want to update simulation context if new target was set
			m_updateSimulationContext = true;
		}
	}
}

void CParticleComponent::SetParticleSystem( CParticleSystem* system )
{
	// Change system
	m_particleSystem = system;

	// Update bounds (bounds are particle system dependant)
	OnUpdateBounds();
}

void CParticleComponent::SetParticleSystemAsync( TSoftHandle< CParticleSystem > resource )
{	
	BaseSoftHandle::EAsyncLoadingResult res = resource.GetAsync();
	if ( res == BaseSoftHandle::ALR_Loaded )
	{
		SetParticleSystem( resource.Get() );
	}
	else if ( res == BaseSoftHandle::ALR_InProgress )
	{
		if ( CLayer* layer = GetLayer() )
		{
			if ( CWorld* world = layer->GetWorld() )
			{
				m_particleSystemResource = resource;
				world->GetTickManager()->AddToGroup( this, TICK_Main );
			}
		}
	}
}

void CParticleComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );
	
	// Draw only if visible
	if ( IsVisible() && IsAttached() && flag == SHOW_Sprites  )
	{
		// Sprite icon
		CBitmapTexture* icon = GetSpriteIcon();
		if ( icon )
		{
			// Draw editor icons
			Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );
			const Float size = 0.25f*screenScale;

#ifndef NO_COMPONENT_GRAPH
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, CalcSpriteColor(), GetHitProxyID(), icon, false );
#else
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, CalcSpriteColor(), CHitProxyID(), icon, false );
#endif
		}
	}

	// Generate bounding boxes
	if ( flag == SHOW_BboxesParticles )
	{
		// That may be unsafe, just comment it if it crashes. The whole simulation happens on renderin thread now so I have to get BB somehow
		if ( m_renderProxy )
		{
			IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_renderProxy );
			ASSERT( proxy );
			frame->AddDebugBox( proxy->GetBoundingBox(), Matrix::IDENTITY, Color::LIGHT_GREEN );
		}
	}
}

Bool CParticleComponent::TryUsingResource( CResource * resource )
{
	CParticleSystem* system = Cast< CParticleSystem >( resource );
	if ( system )
	{
		SetParticleSystem( system );
		return true;
	}
	return false;
}

Bool CParticleComponent::CanAttachToRenderScene() const
{
    return TBaseClass::CanAttachToRenderScene() && m_particleSystem.IsValid();
}

void CParticleComponent::OnTickPostPhysics( Float timeDelta )
{
	ScheduleUpdateTransformNode();
}

void CParticleComponent::OnTick( Float timeDelta )
{
	Bool tick = false;
	if ( !m_particleSystemResource.IsEmpty() )
	{
		BaseSoftHandle::EAsyncLoadingResult res = m_particleSystemResource.GetAsync();
		if ( res == BaseSoftHandle::ALR_Loaded )
		{
			SetParticleSystem( m_particleSystemResource.Get() );
			m_particleSystemResource.Release();
		}
		else if ( res == BaseSoftHandle::ALR_InProgress )
		{
			tick = true;
		}
		else // res == BaseSoftHandle::ALR_Failed
		{
			m_particleSystemResource.Release();
		}
	}
	if ( !tick )
	{
		if ( CLayer* layer = GetLayer() )
		{
			if ( CWorld* world = layer->GetWorld() )
			{
				world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
			}
		}
	}
}

#ifndef NO_DEBUG_PAGES
void CParticleComponent::GetRenderSideBoundingBox( Box& box ) const
{
	if ( m_renderProxy )
	{
		IRenderProxyBase* proxy = static_cast< IRenderProxyBase* >( m_renderProxy );
		box.AddBox( proxy->GetBoundingBox() );
	}
}

#endif

bool CParticleComponent::UsesAutoUpdateTransform()
{
	return false;
}

Bool CParticleComponent::SetEffectParameterValue( CName paramName, const EffectParameterValue &value )
{
	if ( m_renderProxy )
	{
		if ( paramName == CNAME( MeshEffectScalar0 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 0 ) )->Commit();
			return true;
		}
		if ( paramName == CNAME( MeshEffectScalar1 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 1 ) )->Commit();
			return true;
		}
		if ( paramName == CNAME( MeshEffectScalar2 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 2 ) )->Commit();
			return true;
		}
		if ( paramName == CNAME( MeshEffectScalar3 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 3 ) )->Commit();
			return true;
		}
		if ( paramName == CNAME( MeshEffectVector0 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 5 ) )->Commit();
			return true;
		}
	}
	return false;
}

void CParticleComponent::RefreshRenderProxies()
{
	TBaseClass::RefreshRenderProxies();

	CreatePhysicalWrapper();
}

void CParticleComponent::CreatePhysicalWrapper()
{
	if( !m_particleSystem ) return;

	if( m_wrapper )
	{
		return;
	}

#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateParticles ) return;
#endif

	CPhysicsWorld* physicsWorld = nullptr;
	if( !GetWorld() || !GetWorld()->GetPhysicsWorld( physicsWorld ) ) return;

	const TDynArray< CParticleEmitter* >& emitters = m_particleSystem->GetEmitters();
	Uint32 emittersCount = emitters.Size();
	for( Uint32 i = 0; i != emittersCount; ++i  )
	{
		CParticleEmitter* emitter = emitters[ i ];
		SParticleUpdaterData* updaterData = nullptr;
#ifndef RED_FINAL_BUILD
		SParticleUpdaterData updaterDataNoncooked;
#endif
		if( emitter->IsCooked() )
		{
			updaterData = emitter->GetUpdaterData();
		}
#ifndef RED_FINAL_BUILD
		else
		{
			emitter->GenerateApproximatedUpdaterData( updaterDataNoncooked );
			updaterData = &updaterDataNoncooked;
		}
#endif
		if( !updaterData || updaterData->m_collisionEmitterIndex == -1 ) continue;
		if( !m_wrapper)
		{
			m_wrapper = physicsWorld->GetWrappersPool< CPhysicsParticleWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), emittersCount, GetEntity()->GetVisibilityQuery() );
			if( m_wrapper )
			{
				SWrapperContext* context = physicsWorld->GetWrappersPool< CPhysicsParticleWrapper, SWrapperContext >()->GetContext( m_wrapper );
				context->m_desiredDistanceSquared = GetAutoHideDistance();
				if( context->m_desiredDistanceSquared > SPhysicsSettings::m_particleSimulationDistanceLimit )
				{
					context->m_desiredDistanceSquared = SPhysicsSettings::m_particleSimulationDistanceLimit;
				}
				context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;

				SSimulationContextUpdate simulationContext;
				SetSimulationContext( simulationContext );

				// Upload rendering data on the rendering side
				(new CRenderCommand_UpdateParticlesSimulatationContext( m_renderProxy, simulationContext ))->Commit();
			}

			GetAutoHideDistance();
		}
		m_wrapper->AddParticleSystem( i, emitter->GetMaxParticles(), updaterData->m_collisionMask, updaterData->m_collisionDynamicFriction, updaterData->m_collisionStaticFriction, updaterData->m_collisionRestition, updaterData->m_collisionVelocityDamp, updaterData->m_collisionDisableGravity, updaterData->m_collisionDisableGravity, updaterData->m_collisionUseGpuSimulationIfAvaible );
	}
}

#ifndef NO_RESOURCE_USAGE_INFO

void CParticleComponent::CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const
{
	TBaseClass::CollectResourceUsage( collector, isStremable );

	// there's no way to handle the streaming of particle component's data unless we are inside the stremable components
	if ( m_particleSystem && isStremable )
		collector.ReportResourceUsage( m_particleSystem.Get() );
}

void CParticleComponent::UpdateLOD(ILODable::LOD newLOD, CLODableManager* manager)
{
	ASSERT( m_currentLOD != newLOD );

	switch ( m_currentLOD )
	{

		// Full tick
	case ILODable::LOD_0:
		if ( newLOD == ILODable::LOD_1 )
		{
			SetTickBudgeted( true, CComponent::BR_Lod );
		}
		else
		{
			SuppressTick( true, SR_Lod );
		}
		break;

		// Budgeted tick
	case ILODable::LOD_1:
		SetTickBudgeted( false, CComponent::BR_Lod );
		if ( newLOD == ILODable::LOD_2 )
		{
			SuppressTick( true, SR_Lod );
		}
		break;

		// Disabled/suppressed tick
	case ILODable::LOD_2:
		SuppressTick( false, SR_Lod );
		if ( newLOD == ILODable::LOD_1 )
		{
			SetTickBudgeted( true, CComponent::BR_Lod );
		}
		break;
	}

	m_currentLOD = newLOD;
}

ILODable::LOD CParticleComponent::ComputeLOD(CLODableManager* manager) const 
{
	const Vector& worldPos = GetEntity()->GetWorldPosition();
	const Float distSqr = manager->GetPosition().DistanceSquaredTo2D( worldPos );

	if ( distSqr < Red::Math::MSqr( m_particleSystem->GetAutoHideDistance() ) ) 
	{
		if( distSqr < manager->GetBudgetableDistanceSqr() )
		{
			return ILODable::LOD_0;
		}

		return ILODable::LOD_1;
	}

	return ILODable::LOD_2;
}

void CParticleComponent::SetResource(CResource* resource)
{
	RED_ASSERT( !resource || resource->IsA< CParticleSystem >(), TXT("Cannot set '%ls' to '%ls' component."), resource->GetFile()->GetFileName().AsChar(), m_name.AsChar() ); 
	CParticleSystem* particleSystem = Cast< CParticleSystem >( resource );
	if ( (particleSystem != nullptr || resource == nullptr) && particleSystem != m_particleSystem )
	{
		// Set new mesh
		m_particleSystem = particleSystem;

		// Update bounding
		OnUpdateBounds();

		// Full recreation
		PerformFullRecreation();
	}
}

void CParticleComponent::GetResource(TDynArray< const CResource* >& resources) const 
{
	resources.PushBack( GetParticleSystem() );
}

#endif