/**
7* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "apexDestructionWrapper.h"
#include "apexWrapperBase.h"
#include "destructionSystemComponent.h"
#include "apexDestructionResource.h"
#include "renderCommands.h"
#include "performableAction.h"
#include "..\physics\physicsWorld.h"
#ifdef USE_APEX
#include <NxDestructibleActor.h>
#endif
#include "collisionCache.h"

#include "../core/gameSave.h"
#include "../core/scriptStackFrame.h"
#include "../core/dataError.h"
#include "../physics/physicsSettings.h"
#include "renderFragment.h"
#include "renderProxy.h"
#include "world.h"
#include "tickManager.h"
#include "layer.h"
#include "soundEmitter.h"
#include "entity.h"
#include "..\physics\physicsWorld.h"
#include "meshEnum.h"
#include "physicsDataProviders.h"

IMPLEMENT_ENGINE_CLASS( CDestructionSystemComponent );

RED_DEFINE_STATIC_NAME( destruction_fx )
IMPLEMENT_RTTI_ENUM( EDestructionPreset );

SDestructionParameters::SDestructionParameters()
	: 	m_dispacherSelection( EDS_GPU_IF_AVAILABLE ),
	m_dynamic( false ),
	m_damageCap( 0 ),
	m_damageThreshold( 1 ),
	m_damageToRadius( 0.1f ),
	m_debrisDepth( -1 ),
	m_debrisDestructionProbability( 1.0f ),
	m_debrisLifetimeMin( 0 ),
	m_debrisLifetimeMax( 0 ),
	m_debrisMaxSeparationMin( 1.0f ),
	m_debrisMaxSeparationMax( 10.0f ),
	m_fadeOutTime( 1.0f ),
	m_essentialDepth( 0 ),
	m_forceToDamage( 0 ),
	m_fractureImpulseScale( 0 ),
	m_impactDamageDefaultDepth( -1 ),
	m_impactVelocityThreshold( 0 ),
	m_materialStrength( 0.0f ),
	m_maxChunkSpeed( 0 ),
	m_minimumFractureDepth( 0 ),
	m_useStressSolver( false ),
	m_stressSolverTimeDelay( 0.0f ),
	m_stressSolverMassThreshold( 0.0f ),
	m_supportDepth( 0 ),
	m_useAssetDefinedSupport( false ),
	m_useWorldSupport( false ),
	m_sleepVelocityFrameDecayConstant( 1.0f ),
	m_useHardSleeping( false ),
	m_accumulateDamage( false ),
	m_debrisTimeout( false ),
	m_debrisMaxSeparation( false ),
	m_crumbleSmallestChunks( false ),
	m_usePreviewAsset( false ),
	m_physicalCollisionType( CNAME( Destructible ) ),
	m_fracturedPhysicalCollisionType( CNAME( Destructible ) )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CDestructionSystemComponent class - destructible component
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDestructionSystemComponent::CDestructionSystemComponent() 
	: m_preset( CUSTOM_PRESET_D )
	, m_pathLibCollisionType( PLC_Disabled )
	, m_disableObstacleOnDestruction( true )
	, m_wasDestroyed( false )
	, m_lastAmontOfBaseFractures( 0 )
#ifdef USE_APEX
	, m_wrapper(NULL)
#endif
	, m_shadowDistanceOverride( -1.0f )
{
	m_drawableFlags = DF_IsVisible | DF_CastShadows | DF_UseInAllApperances;
}

CDestructionSystemComponent::~CDestructionSystemComponent()
{
#ifdef USE_APEX
	RED_ASSERT( m_wrapper == nullptr, TXT("APEX wrapper was not released. Was the OnDetach called ?") );
#endif
}


Float CDestructionSystemComponent::GetShadowDistance( Uint8& outRenderMask ) const
{
	// No resource and no override, we can't really get a default...
	if ( m_shadowDistanceOverride < 0.0f && m_parameters.m_resource == nullptr )
	{
		return 0.0f;
	}


	CWorld* world = GetWorld();
	RED_ASSERT( world != nullptr, TXT("Requesting shadow distance and I'm not attached to a world") );

	Float extent = GetBoundingBox().CalcExtents().Mag3();

	outRenderMask = MCR_Scene | MCR_LocalShadows | MCR_Cascade1 | MCR_Cascade2;

	Float shadowDistance = m_shadowDistanceOverride;
	if ( shadowDistance < 0.0f )
	{
		shadowDistance = m_parameters.m_resource->CalcShadowDistance( world, GetBoundingBox() );
	}

	// Extend render mask if needed.
	if ( shadowDistance + extent > world->GetShadowConfig().m_cascadeRange2 )
	{
		outRenderMask |= MCR_Cascade3;
	}
	if ( shadowDistance + extent > world->GetShadowConfig().m_cascadeRange3 )
	{
		WARN_ENGINE( TXT("Destruction component '%ls' has shadow reaching into cascade 4! Seriously consider shortening the distance! If this is using default distance (-1), and so is the resource, then it's probably a code bug"), GetFriendlyName().AsChar() );
		outRenderMask |= MCR_Cascade4;
	}

	return shadowDistance;
}


void CDestructionSystemComponent::OnAttached( CWorld* world )
{
	{
		PC_SCOPE_PIX( CDestructionSystemComponent_OnAttached1 );

#ifndef NO_EDITOR
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Meshes );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Collision );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BBoxesDestruction );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_ApexFracturePoints );
#endif

		m_parameters.m_pose = GetLocalToWorld();

		// Pass to base class
		TBaseClass::OnAttached( world );

		if ( !(IsStreamed() && GetEntity()->ShouldBeStreamed()) )
		{
			InitWrapper();
		}

#ifndef  NO_EDITOR
		// In editor, after returning from PIE session, CDestructionSystemComponent is getting detached,
		// but entity is not being unstreamed. Therefore, when entity is getting reattached to world, it's components
		// are being reattached as well, but since it was already streamed in previously, streaming ones don't have their
		// OnStreamIn method fired. This results in InitWrapper not firing either and m_wrapper not being created.
		// To fix that, if the component is a streamed one and should be streamed, but the wrapper is not created, 
		// while entity says it already had been streamed in, we Init it here.
#ifdef USE_APEX
		const CEntity* ent = Cast< const CEntity >(GetParent());
		if( (IsStreamed() && GetEntity()->ShouldBeStreamed()) && ent->IsStreamedIn() && m_wrapper == nullptr )
		{
			InitWrapper();
		}
#endif
#endif // ! NO_EDITOR

	}
}

void CDestructionSystemComponent::InitWrapper()
{
	{
		PC_SCOPE_PIX( CDestructionSystemComponent_InitWrapper );

#ifdef USE_APEX
		Bool shouldAdjustAutohide = !m_wasDestroyed;
#ifndef RED_FINAL_BUILD
		shouldAdjustAutohide &= !SPhysicsSettings::m_dontCreateDestruction;
#endif
#ifdef NO_EDITOR
		shouldAdjustAutohide &= ( m_parameters.m_resource && ICollisionCache::eResult_Valid == GCollisionCache->HasCollision( m_parameters.m_resource->GetDepotPath(), m_parameters.m_resource->GetFileTime() ) );
#endif

		if( shouldAdjustAutohide )
		{
			CPhysicsWorld* physicalWorld = nullptr;
			if( GetWorld()->GetPhysicsWorld( physicalWorld ) )
			{
				if( !m_wrapper )
				{
					m_wrapper = physicalWorld->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), &m_parameters, physicalWorld, GetEntity()->GetVisibilityQuery() ) ;
				}
				if( m_wrapper )
				{
					SWrapperContext* context = physicalWorld->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->GetContext(m_wrapper);
					context->m_desiredDistanceSquared = GetAutoHideDistance();
					if( context->m_desiredDistanceSquared > SPhysicsSettings::m_destructionSimulationDistanceLimit )
					{
						context->m_desiredDistanceSquared = SPhysicsSettings::m_destructionSimulationDistanceLimit;
					}
					m_wrapper->SetAutoHideDistance( context->m_desiredDistanceSquared );
					context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;
					context->m_requestProcessingFlag = true;
				}
			}
		}
#endif	//USE_APEX
	}


	{
#ifdef USE_APEX
		if( IsEnabled() && m_wrapper )
		{
			if( !m_targetEntityCollisionScriptEventName.Empty() )
			{
				m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnCollision, this, this );
			}
			if( !m_parentEntityCollisionScriptEventName.Empty() )
			{
				m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnCollision, this, m_parentEntityCollisionScriptEventName );
			}
		}
#endif

		if( !IsObstacleDisabled() )
		{
			IObstacleComponent::Attach( GetWorld() );
		}
	}
}

void CDestructionSystemComponent::OnDetached( CWorld* world )
{
#ifndef NO_EDITOR
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Meshes );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Collision );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BBoxesDestruction );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_ApexFracturePoints );

#endif

	if( !IsObstacleDisabled() )
	{
		IObstacleComponent::Detach( world );
	}

#ifdef USE_APEX
	ReleaseWrapper();
#endif

	// Pass to base class
	TBaseClass::OnDetached( world );
}

void CDestructionSystemComponent::ReleaseWrapper()
{
#ifdef USE_APEX
	if ( m_wrapper )
	{
		( ( IRenderObject* ) m_wrapper )->Release();
		m_wrapper = nullptr;
	}
#endif
}

void CDestructionSystemComponent::RefreshRenderProxies()
{
	ForceUpdateBoundsNode();
	TBaseClass::RefreshRenderProxies();
}

Bool CDestructionSystemComponent::CanAttachToRenderScene() const
{
#ifdef USE_APEX
	if( !m_wrapper ) return false;
	if( !m_wrapper->IsReady() ) return false;
#endif

	return CDrawableComponent::CanAttachToRenderScene();
}

#ifndef NO_EDITOR
void CDestructionSystemComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	CWorld* world = GetWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : nullptr;
	if ( pathlib )
	{
		CEntity* entity = GetEntity();
		if ( entity )
		{
			entity->ForceUpdateTransformNodeAndCommitChanges();
			UpdateObstacle();
		}
	}
#ifdef USE_APEX
	if ( m_wrapper )
	{
		m_wrapper->SetFlag( PRBW_PoseIsDirty, true );
	}
#endif
}

void CDestructionSystemComponent::EditorOnTransformChangeStop()
{
	TBaseClass::EditorOnTransformChangeStop();
	if ( CEntity* e = GetEntity() )
	{
		e->ForceUpdateTransformNodeAndCommitChanges();
		e->ForceUpdateBoundsNode();
	}
}

void CDestructionSystemComponent::EditorPreDeletion()
{
	TBaseClass::EditorPreDeletion();

	CWorld* world = GetWorld();
	if ( world )
	{
		IObstacleComponent::Remove( world );
	}
}

void CDestructionSystemComponent::OnNavigationCook( CWorld* world, CNavigationCookingContext* cookerData )
{
#ifdef USE_APEX
	ASSERT( GIsCooker );
	if ( !m_wrapper )
	{
		CPhysicsWorld* physicalWorld = nullptr;
		if( world->GetPhysicsWorld( physicalWorld ) )
		{
			m_parameters.m_pose = GetLocalToWorld();
			m_wrapper = physicalWorld->GetWrappersPool< CApexDestructionWrapper, SWrapperContext > ()->Create( CPhysicsWrapperParentComponentProvider( this ), &m_parameters, physicalWorld, 0 );
		}
	}
	if ( m_wrapper && m_wrapper->GetActorsCount() == 0 )
	{
		m_wrapper->Create( m_parameters );
	}
	
#endif
}

void CDestructionSystemComponent::PostNavigationCook( CWorld* world )
{
	ReleaseWrapper();
}

#endif		// !NO_EDITOR

void CDestructionSystemComponent::OnUpdateBounds()
{
#ifdef USE_APEX
	if ( m_wrapper )
	{
		// bounds returned here is already in world space. If getting bounds fails, then the input box is unchanged,
		// so we don't need to do anything extra for that.
		m_boundingBox = m_wrapper->GetBounds();
	}
#endif
}

void CDestructionSystemComponent::OnTickPostPhysics( Float timeDelta )
{
	PC_SCOPE( DestructionSystemComponent_OnTickPostPhysics )

#ifdef USE_APEX
	CWorld* world = GetWorld();
	world->GetTickManager()->RemoveFromGroup( this, TICK_PostPhysics );

	if( !m_wrapper )
	{
		return;
	}

	CName fxName;
	const char* soundEventName = nullptr;

	if(	CApexDestructionResource* resource = Cast< CApexDestructionResource >( m_parameters.m_resource.Get() ) )
	{
		fxName = resource->GetFractureFxName();
		soundEventName = resource->GetFractureEventName();
	}

	if ( !m_wasDestroyed )
	{
		CEntity* entity = GetEntity();
		if( !fxName.Empty() )
		{
			RED_ASSERT( entity->HasEffect( fxName ) == false, TXT("Efect on fructure defined in resource isnt existing in entity") );
			entity->PlayEffect( fxName );
		}

		m_wasDestroyed = true;

		if ( m_disableObstacleOnDestruction && (m_pathLibCollisionType == PLC_Dynamic || m_pathLibCollisionType == PLC_Immediate) )
		{
			CWorld* world = entity->GetLayer()->GetWorld();
			if ( world )
			{
				IObstacleComponent::Disable( world );
			}
		}

		IPerformableAction::PerformAll( m_eventOnDestruction, entity );		
	}

	if( soundEventName )
	{
		CSoundEmitterComponent* soundEmitterComponent = GetEntity()->GetSoundEmitterComponent();
		soundEmitterComponent->SoundParameter( "phx_fracture_amount", ( Float ) m_lastAmontOfBaseFractures );
		soundEmitterComponent->SoundEvent( soundEventName );
	}
#endif	
}

void CDestructionSystemComponent::ScheduleTick( Uint32 fractureAmount )
{
	if( CWorld* world = GetWorld() )
	{
		m_lastAmontOfBaseFractures = fractureAmount;
		world->GetTickManager()->AddToGroup( this, TICK_PostPhysics );
	}
}

void CDestructionSystemComponent::UpdateObstacle()
{
	CLayer* layer = GetEntity()->GetLayer();
	if ( layer )
	{
		CWorld* world = layer->GetWorld();
		if ( world )
		{
			IObstacleComponent::Update( world );
		}
	}
}
void CDestructionSystemComponent::SetPathLibCollisionGroupInternal( EPathLibCollision collisionGroup )
{
	m_pathLibCollisionType = collisionGroup;
}
EPathLibCollision CDestructionSystemComponent::GetPathLibCollisionGroup() const
{
	return m_pathLibCollisionType;
}
CComponent* CDestructionSystemComponent::AsEngineComponent()
{
	return this;
}
PathLib::IComponent* CDestructionSystemComponent::AsPathLibComponent()
{
	return this;
}

#ifdef USE_APEX
void CDestructionSystemComponent::SetResource( CResource* resource )
{
	RED_ASSERT( !resource || resource->IsA< CApexDestructionResource >(), TXT("Cannot set '%ls' to '%ls' component."), resource->GetFile()->GetFileName().AsChar(), m_name.AsChar() ); 
	CApexDestructionResource* destrRes = Cast< CApexDestructionResource >( resource );
	if ( destrRes != nullptr || resource == nullptr )
	{
		m_parameters.m_resource =  THandle< CApexDestructionResource >( destrRes );
		m_parameters.m_pose = GetLocalToWorld();
		CApexDestructionWrapper::FillResourceParameters( m_parameters );

		if ( m_wrapper )
		{
			m_wrapper->RequestReCreate( m_parameters );
			SheduleRenderingResourceChange();
		}
	}
}
 void CDestructionSystemComponent::GetResource( TDynArray< const CResource* >& resources ) const
 {
	 resources.PushBack( m_parameters.m_resource.Get() );
 }
#endif

#ifndef NO_EDITOR_FRAGMENTS
void CDestructionSystemComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flags );
#ifdef USE_APEX
	if( m_wrapper )
	{
		if ( flags == SHOW_BBoxesDestruction )
		{
			frame->AddDebugAxis( m_localToWorld.GetTranslationRef(), m_localToWorld, 0.1f, true );
			Float distFromViewport = 0.f;
			Vector wPos(0,0,0);
			
			if( CPhysicsWorld* world = m_wrapper->GetPhysicsWorld() )
			{
				SWrapperContext* context = world->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->GetContext( m_wrapper );
				distFromViewport = context->m_resultDistanceSquared;
				wPos.X = context->m_x;
				wPos.Y = context->m_y;
				wPos.Z = 0;//context->m_z;
			}

			String dist = String::Printf( TXT("%.2f"), Red::Math::MSqrt( distFromViewport ) );
			Color col = Color::GREEN;
			if ( m_parameters.m_useHardSleeping )
			{
				col = Color::RED;
			}

			frame->AddDebugText( m_localToWorld.GetTranslationRef(), dist, 0, 0, false, col );				// distance to components l2w
			frame->AddDebugSphere( m_boundingBox.CalcCenter(), 0.05f, Matrix::IDENTITY, col, true, true );	// bbox center
			frame->AddDebugSphere( wPos, 0.15f, Matrix::IDENTITY, Color::YELLOW, true, true );				// wrapper pos
			frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::RED, true );						// bbox from component
			frame->AddDebugBox( m_wrapper->GetBounds(), Matrix::IDENTITY, Color::MAGENTA, true );			// bbox from apex
		}

#ifndef NO_EDITOR
		if ( flags == SHOW_Collision )
		{
			physx::apex::NxDestructibleActor* actor = ( physx::apex::NxDestructibleActor* )m_wrapper->GetApexActor();
			if( actor )
			{
				Uint32 numVisibleChunks = actor->getNumVisibleChunks();
				const Uint16* visibleChunks = actor->getVisibleChunks();

				for ( Uint32 i = 0; i < numVisibleChunks; ++i )
				{
					IRenderResource* chunkMesh = m_wrapper->GetChunkDebugMesh( visibleChunks[ i ] );
					if ( chunkMesh )
					{
						Matrix chunkLocalToWorld = m_wrapper->GetChunkLocalToWorld( visibleChunks[ i ] );
						new ( frame ) CRenderFragmentDebugMesh( frame, chunkLocalToWorld, chunkMesh, true );
						new ( frame ) CRenderFragmentDebugMesh( frame, chunkLocalToWorld, chunkMesh, false, true );
					}
				}
			}
		}
		if( flags == SHOW_ApexFracturePoints )
		{
			const TDynArray< Vector >& buffer = m_wrapper->GetDebugFracturePoints();
			for( Uint32 i = 0; i != buffer.Size(); ++i )
			{
				frame->AddDebugSphere( buffer[ i ], 0.1f, Matrix::IDENTITY, Color::RED );
			}
		}
#endif	//NO_EDITOR
	}
#endif	//USE_APEX
}
#endif


#ifndef NO_EDITOR
void CDestructionSystemComponent::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("m_physicalCollisionType") || property->GetName() == TXT("m_fracturedPhysicalCollisionType") )
	{
		return;
	}
	if ( property->GetName() == TXT("m_resource") )
	{
#ifdef USE_APEX
		if( !CApexDestructionWrapper::FillResourceParameters( m_parameters ) ) return;
#endif
	}
	// Path lib collision changed
	else if ( property->GetName() == TXT("pathLibCollisionType") )
	{
		CLayer* layer = GetEntity()->GetLayer();
		if ( layer )
		{
			CWorld* world = layer->GetWorld();
			if ( world )
			{
				IObstacleComponent::OnCollisionGroupUpdated( world, m_pathLibCollisionType );
			}
		}
	}
	else if ( m_preset != CUSTOM_PRESET_D )
	{
		LoadPresetParams( m_preset );
	}

	Reset();
}

Uint32 CDestructionSystemComponent::GetMinimumStreamingDistance() const
{
	if ( m_parameters.m_resource.IsValid() )
	{
		return (Uint32)Red::Math::MRound( m_parameters.m_resource.Get()->GetAutoHideDistance()*1.1f );
	}
	return 0;
}
#endif

void CDestructionSystemComponent::ApplyForce( const Vector& force, const Vector& point, Float deltaTime )
{
#ifdef USE_APEX
	m_wrapper->ApplyForce(force, point );
#endif
}

void CDestructionSystemComponent::SetEnabled( Bool enabled )
{
#ifdef USE_APEX
	if( !m_wrapper ) return;
	
	if( !enabled )
	{
		m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnCollision, nullptr, nullptr );
		m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnCollision );
		
		return;
	}

	if( !m_targetEntityCollisionScriptEventName.Empty() )
	{
		m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnCollision, this, this );
		m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnCollision, this, m_targetEntityCollisionScriptEventName );
	}
	m_wrapper->ForceDynamicState();
#endif
}

void CDestructionSystemComponent::onCollision( const SPhysicalCollisionInfo& info )
{
	if( !m_targetEntityCollisionScriptEventName.Empty() )
	{
		CComponent* targetComponent = nullptr;
		if( info.m_otherBody->GetParent( targetComponent ) )
		{
			CEntity* targetEntity = targetComponent->GetEntity();
			if( targetEntity)
			{
				THandle< CComponent > handle = this;
				targetEntity->CallEvent( m_targetEntityCollisionScriptEventName, handle );
			}
		}
	}
}

void CDestructionSystemComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	saver->WriteValue( CNAME( state ), m_wasDestroyed );
}

void CDestructionSystemComponent::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	loader->ReadValue( CNAME( state ), m_wasDestroyed );
	if( m_wasDestroyed )
	{
		// We check if wrapper was already created. In that case, we need to release it and
		// recreate as destroyed already. It may seem that simply requesting fracture would suffice,
		// but that would result in destruction collapsing in place and fading away (if set) which wouldn't look good.
		if( m_wrapper )
		{
			ReleaseWrapper();
			InitWrapper();
			RefreshRenderProxies();
		}
	}	
}

void CDestructionSystemComponent::OnStreamIn()
{
	TBaseClass::OnStreamIn();

	if ( GetWorld() )
	{
		InitWrapper();
	}
}

void CDestructionSystemComponent::funcIsDestroyed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifdef USE_APEX
	if( !m_wrapper )
	{
		RETURN_BOOL( false );
		return;
	}
	RETURN_BOOL( m_wrapper->GetActor( 0 ) == nullptr );
#else
	RETURN_BOOL( false );
#endif
}

void CDestructionSystemComponent::funcIsObstacleDisabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsObstacleDisabled() );
}

void CDestructionSystemComponent::funcGetFractureRatio( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifdef USE_APEX
	if( !m_wrapper )
	{
		RETURN_FLOAT( 0.0f );
		return;
	}
	RETURN_FLOAT( m_wrapper->GetFractureRatio() );
#else
	RETURN_FLOAT( 0.0f );
#endif
}


void CDestructionSystemComponent::funcApplyFracture( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;


	const Bool res = 
#ifdef USE_APEX
		GetDestructionBodyWrapper() ? GetDestructionBodyWrapper()->ApplyFracture() : false;
#else
		false;
#endif

	RETURN_BOOL( res );
}

void CDestructionSystemComponent::funcApplyForce( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, force, Vector() );		
	GET_PARAMETER( Vector, point, Vector() );	
	GET_PARAMETER_OPT( Float, radius, 1.0f );	
	GET_PARAMETER_OPT( Int32, idx, 0 );			
	FINISH_PARAMETERS;

	CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
	if ( wrapper )
	{
		const Uint32 actorIndex( Clamp< Int32 > ( idx, 0, INT_MAX ) );
		wrapper->ApplyForce( force, point, actorIndex );
	}
}


void CDestructionSystemComponent::funcApplyDamageAtPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, damage, 1.f );
	GET_PARAMETER( Float, momentum, 0.f );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Vector, direction, Vector::EZ );
	FINISH_PARAMETERS;

	const Bool res = 
#ifdef USE_APEX
		GetDestructionBodyWrapper()->ApplyDamageAtPoint( damage, momentum, point, direction ); 
#else
		false;
#endif

	RETURN_BOOL( res );
}

void CDestructionSystemComponent::funcApplyRadiusDamage( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, damage, 1.f );
	GET_PARAMETER( Float, momentum, 0.f );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 1.f );
	GET_PARAMETER( Bool, falloff, false );
	FINISH_PARAMETERS;

	const Bool res = 
#ifdef USE_APEX
		GetDestructionBodyWrapper()->ApplyRadiusDamage( damage, momentum, point, radius, falloff ); 
#else
		false;
#endif

	RETURN_BOOL( res );
}

#ifndef NO_EDITOR
void CDestructionSystemComponent::SetUsePreview( Bool usePreview )
{
	m_parameters.m_usePreviewAsset = usePreview;
	m_parameters.m_pose = GetLocalToWorld();

#ifdef USE_APEX
	if ( m_wrapper )
	{
		m_wrapper->RequestReCreate( m_parameters );
		SheduleRenderingResourceChange();
	}
#endif
}


void CDestructionSystemComponent::Reset()
{
	CEntity* entity = GetEntity();
	IRenderScene* ex = GetLayer()->GetWorld()->GetRenderSceneEx();

	Bool refreshProxy = m_renderProxy && IsAttached() && GetLayer()->IsAttached();
	if ( refreshProxy )
	{
		( new CRenderCommand_RemoveProxyFromScene( ex, m_renderProxy ) )->Commit();

		m_renderProxy->Release();
		m_renderProxy = NULL;
		entity->OnProxyDetached( this );
	}

#ifdef USE_APEX
	if( m_wrapper )
	{
		m_wrapper->RequestReCreate( m_parameters );
	}
#endif

	if( refreshProxy )
	{
		OnInitializeProxy();
		( new CRenderCommand_AddProxyToScene( ex, m_renderProxy ) )->Commit();
		entity->OnProxyAttached( this, m_renderProxy );
	}
}

void CDestructionSystemComponent::LoadPresetParams( Uint32 p )
{
	//	enum name, probability, CrubleChunks, AccumulateDam, damCap, damThreshold, damToRad, forceToDam, fracImpulseScale, impactDamDefaultDepth, impactVelThreshold, materialStrangth, maxChunkSpeed, useWorldSupport, UseHardSleeping, useStressSolver, stressSolverDelay, stressSolverMassThreshold, sleepVelocityFrameDecay.
	String name = destructionPreset[p].name;
	m_parameters.m_debrisDestructionProbability		=	destructionPreset[p].probability;
	m_parameters.m_crumbleSmallestChunks			=	destructionPreset[p].crumbleChunks;
	m_parameters.m_accumulateDamage					=	destructionPreset[p].accumulateDam;
	m_parameters.m_damageCap						=	destructionPreset[p].damCap;
	m_parameters.m_damageThreshold					=	destructionPreset[p].damThres;
	m_parameters.m_damageToRadius					=	destructionPreset[p].damToRad;
	//m_parameters.m_forceToDamage					=	destructionPreset[p].forToDam;
	m_parameters.m_fractureImpulseScale				=	destructionPreset[p].fracImpScale;
	m_parameters.m_impactDamageDefaultDepth			=	destructionPreset[p].impDefDepth;
	m_parameters.m_impactVelocityThreshold			=	destructionPreset[p].impVelThres;
	m_parameters.m_materialStrength					=	destructionPreset[p].matStrength;
	m_parameters.m_maxChunkSpeed					=	destructionPreset[p].maxChunkSpeed;
	m_parameters.m_useWorldSupport					=	destructionPreset[p].useWorldSupport;
	m_parameters.m_useHardSleeping					=	destructionPreset[p].useHardSleep;
	m_parameters.m_useStressSolver					=	destructionPreset[p].useStresSolver;
	m_parameters.m_stressSolverTimeDelay			=	destructionPreset[p].stressSolverDelay;
	m_parameters.m_stressSolverMassThreshold		=	destructionPreset[p].stressSolverMassThres;
	m_parameters.m_sleepVelocityFrameDecayConstant	=	destructionPreset[p].sleepVelFrameDecay;
}

#endif
