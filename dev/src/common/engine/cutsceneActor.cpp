
#include "build.h"

#include "behaviorGraphCutsceneNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphInstance.inl"
#include "behaviorGraph.inl"
#include "behaviorGraphStack.h"
#include "behaviorGraphTopLevelNode.h"
#include "cutsceneActor.h"
#include "extAnimCutsceneSoundEvent.h"
#include "cutsceneDebug.h"
#include "animatedComponent.h"
#include "entity.h"
#include "skeletalAnimationContainer.h"
#include "cutsceneInstance.h"
#include "animSyncInfo.h"
#include "animationEvent.h"
#include "visualDebug.h"
#include "behaviorGraphContext.h"
#include "actorInterface.h"
#include "mimicComponent.h"
#include "..\core\mathUtils.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( Cutscene2 )

void CutsceneActorPlayableElement::Set( CAnimatedComponent* ac )
{
	ASSERT( !m_component );
	m_component = ac;
	m_componentName = ac ? ac->GetName() : String::EMPTY;
}

const CAnimatedComponent* CutsceneActorPlayableElement::GetRestrict( const CutsceneActor& a ) const
{
	return m_component.Get();
}

CAnimatedComponent* CutsceneActorPlayableElement::GetRestrict( const CutsceneActor& a )
{
	return m_component.Get();
}

const CAnimatedComponent* CutsceneActorPlayableElement::Get( const CutsceneActor& a ) const
{
	const CAnimatedComponent* ac = m_component.Get();
	if ( !ac && !m_componentName.Empty() )
	{
		if ( const CEntity* e = a.m_entity.Get() )
		{
			CAnimatedComponent* newAc = e->FindComponent< CAnimatedComponent >( m_componentName );
			if ( newAc )
			{
				CutsceneActor& aa = const_cast< CutsceneActor& >( a );
				CutsceneActorPlayableElement& ee  = const_cast< CutsceneActorPlayableElement& >( *this );

				ee.Set( newAc );

				if ( a.m_useMimic )
				{
					if ( CMimicComponent* m = Cast< CMimicComponent >( newAc ) )
					{
						m->MimicHighOn();
					}
				}

				aa.Relock( ee );

				ac = newAc;
			}
		}
	}

	return ac;
}

CAnimatedComponent* CutsceneActorPlayableElement::Get( const CutsceneActor& a )
{
	return const_cast< CAnimatedComponent* >( static_cast< const CutsceneActorPlayableElement* >( this )->Get( a ) );
}

CutsceneActor::CutsceneActor( const CCutsceneInstance* csInstance )
	: m_setDeathPose( false )
	, m_type( CAT_None )
	, m_cutscene( csInstance )
	, m_isLocked( false )
	, m_wasHiddenByScene( false )
	, m_wasSnapedToNavigableSpace( false )
	, m_wasSwitchedToGameplay( false )
	, m_switchedToGameplayBlendTime( 0.f )
	, m_source( AS_Scene )
{

}

void CutsceneActor::AddPlayableElement( CAnimatedComponent* ac, const CName& animation )
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];
		if ( elem.IsFor( ac ) )
		{
			elem.m_animations.PushBack( animation );
			return;
		}
	}

	CutsceneActorPlayableElement elem;
	elem.Set( ac );
	elem.m_animations.PushBack( animation );
	m_playableElements.PushBack( elem );
}

void CutsceneActor::Update( const CSyncInfo& info, THashMap< const CExtAnimCutsceneSoundEvent*, CAnimatedComponent* >* soundEvents )
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		CAnimatedComponent* ac = elem.Get( *this );
		if ( !ac )
		{
			m_cutscene->ThrowError( String::Printf( TXT("Actor '%ls' was despawned, '%ls'"), m_name.AsChar(), m_cutscene->GetName().AsChar() ) );
			return;
		}

		// Do not freeze characters
		ac->ForceRestartFrameSkipping();

		ASSERT( !ac->IsTickSuppressed() );
		if ( ac->IsTickSuppressed() )
		{
			m_cutscene->ThrowError( String::Printf( TXT("Actor '%ls' was suppressed, '%ls'"), m_name.AsChar(), m_cutscene->GetName().AsChar() ) );
		}

		ASSERT( !ac->IsFrozen() );
		if ( ac->IsFrozen() )
		{
			m_cutscene->ThrowError( String::Printf( TXT("Actor '%ls' is frozen, '%ls'"), m_name.AsChar(), m_cutscene->GetName().AsChar() ) );
		}

		CBehaviorGraphInstance* instance = ac->GetBehaviorStack()->GetBehaviorGraphInstance( CNAME( Cutscene ) );

		if ( instance )
		{
			elem.m_controller->SynchronizeTo( *instance, info );

			// Gets sound events to fire during this update.
			// They must be fired separately, because they need to be stopped after cutscene ends.
			TDynArray< CAnimationEventFired > eventsFired;

			if( soundEvents != NULL )
			{
				// Collect events from animation
				elem.m_controller->CollectEvents( *instance, info, eventsFired );

				// Fill list
				for( Uint32 z = 0; z < eventsFired.Size(); ++z )
				{
					if( IsType< CExtAnimCutsceneSoundEvent >( eventsFired[ z ].m_extEvent ) )
					{
						soundEvents->Insert( static_cast< const CExtAnimCutsceneSoundEvent* >( eventsFired[ z ].m_extEvent ), ac );
					}
				}
			}
		}
		else
		{
			m_cutscene->ThrowError( String::Printf( TXT("Actor '%ls' has been stolen from cutscene '%ls'"), m_name.AsChar(), m_cutscene->GetName().AsChar() ) );
			ASSERT( instance );
		}
	}

	if ( !m_wasSwitchedToGameplay )
	{
		ParanoidCheck();
	}
}

void CutsceneActor::SetBlendFactor( Float factor )
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		CAnimatedComponent* ac = elem.Get( *this );
		if ( ac )
		{
			CBehaviorGraphInstance* instance = ac->GetBehaviorStack()->GetBehaviorGraphInstance( CNAME( Cutscene ) );
			if ( instance )
			{
				elem.m_controller->SetBlendFactor( *instance, factor );
			}
		}
	}
}

void CutsceneActor::Init( CWorld* world, Bool sync )
{
	ASSERT( m_isLocked == false );

	if ( ( sync && ApplyCsAppearance() ) || m_source == AS_Spawned )
	{
		// Temp fix for spawning and change app
		//world->DelayedActions();
	}

	if ( m_useMimic )
	{
		SetMimics( true );
	}
}

void CutsceneActor::PrepareForCutscene()
{
	if ( m_source == AS_Gameplay )
	{
		ApplyCsAppearance();
	}	
}

void CutsceneActor::Destroy( Bool isCutsceneFinished )
{
	ASSERT( m_isLocked == false );

	if ( m_source == AS_Spawned )
	{
		if ( CEntity* ent =  m_entity.Get() )
		{
			ent->Destroy();
		}
	}
	else if( isCutsceneFinished )
	{
		// If actor is not gameplay then he is taken from scene. He may need mimics later in scene
		// so we don't turn it off. Scene will turn off his mimic when it ends.
		if ( m_useMimic == true && m_source == AS_Gameplay )
		{
			SetMimics( false );
		}

		if ( m_setDeathPose )
		{
			SendKillEvent();
			CacheDeathPose();
		}
		else
		{
			RevertCsAppearance();
		}
	}
}

void CutsceneActor::OnSwitchedToGameplay( Bool flag, Float blendTime )
{
	m_wasSwitchedToGameplay = flag;
	m_switchedToGameplayBlendTime = blendTime;

	BlendPoseToGameplay( blendTime );

	LockStacks( false );

	const Matrix offset = m_cutscene->GetCsPosition();

	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		if ( CAnimatedComponent* ac = elem.Get( *this ) )
		{
			//ac->SetUseExtractedTrajectory( flag );

			if ( elem.m_controller )
			{
				if ( CBehaviorGraphInstance* instance = ac->GetBehaviorStack()->GetBehaviorGraphInstance( CNAME( Cutscene ) ) )
				{
					elem.m_controller->SetGameplayMode( *instance, flag, blendTime, offset );
				}
			}

			VERIFY( ac->GetBehaviorStack()->ActivateAllInstances() );
		}
	}
}

Bool CutsceneActor::Lock()
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		if ( !Lock( elem ) )
		{
			return false;
		}
	}

	SetExternalControl( true );

	SetStartingPos();

	ParanoidCheck();

	// Actor Visibility
	if ( CEntity* entityPtr = m_entity.Get() )
	{
		m_wasHiddenByScene = ( entityPtr->GetHideReason() & CEntity::HR_Scene ) != 0;
		if ( m_wasHiddenByScene )
		{
			entityPtr->SetHideInGame( false, false, CEntity::HR_Scene );
		}

		if ( CAnimatedComponent* root = entityPtr->GetRootAnimatedComponent() )
		{
			m_wasSnapedToNavigableSpace = root->IsSnapedToNavigableSpace();
			root->SnapToNavigableSpace( false );
		}
	}

	m_isLocked = true;

	return true;
}

Bool CutsceneActor::Lock( CutsceneActorPlayableElement& elem )
{
	Bool ret = AttachGraphAndFindController( elem );
	if ( !ret )
	{
		return false;
	}

	LockStacks( elem, true );

	PreparePlayableElements( elem, true );

	return true;
}

void CutsceneActor::Relock( CutsceneActorPlayableElement& elem )
{
	Lock( elem );
}

RedMatrix4x4 CutsceneActor::HACK_GetPelvisIdleOffset() const
{	
	Uint32 pelvisBone = 9;
	Uint32 rootBone = 0;
	CEntity* ent = m_entity.Get();
	const SCutsceneActorDef* def = m_cutscene->GetCsTemplate()->GetActorDefinition( m_name );

	RedMatrix4x4 pelvisIdle;
	if ( def && def->m_animationAtFinalPosition )
	{
		CAnimatedComponent*		 ac  = def && ent ? ent->GetRootAnimatedComponent() : nullptr;		
		CSkeletalAnimationContainer*	cont = ac ? ac->GetAnimationContainer() : nullptr;
		CSkeletalAnimationSetEntry* seAnim = cont ?  cont->FindAnimation( def->m_animationAtFinalPosition ) : nullptr;
		CSkeletalAnimation*			anim = seAnim ? seAnim->GetAnimation() : nullptr;
		if ( anim && anim->GetBonesNum() > pelvisBone )
		{
			SBehaviorGraphOutput pose;
			pose.Init( anim->GetBonesNum(), anim->GetTracksNum() );
			anim->Sample( 0.f, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );						
			pose.ExtractTrajectory( ac );

			RedQsTransform pelvisTransform = pose.m_outputPose[ pelvisBone ];
			RedQsTransform rootTransform = pose.m_outputPose[ rootBone ];
			RedQsTransform movement; 
			movement.SetMul( pelvisTransform, rootTransform );

			pelvisIdle = movement.ConvertToMatrixNormalized(); 
		}
	}	
	else
	{					
		CAnimatedComponent*		 ac  = def && ent ? ent->GetRootAnimatedComponent() : nullptr;
		CSkeleton* skel = ac ? ac->GetSkeleton() : nullptr;					
		if ( skel && skel->GetFile() && skel->GetFile()->GetFileName() == TXT( "woman_base.w2rig" ) )
		{
			Float data[16] = {
				-0.00349044800, -0.999469876,    0.0323740542, 0.000000000,
				-0.0898630917,  -0.0319299698,  -0.995442271,  0.000000000,
				0.995948195,   -0.00638362765, -0.0897041559, 0.000000000,
				-0.0130693847,  -0.0114661437,   0.996093154,  1.00000000,
			};
			pelvisIdle = RedMatrix4x4( data );
		}
		else // man_base
		{
			Float data[16] = {
				-0.477600098,  -0.878335536,   0.0206184089, 0.000000000,
				0.0294697285, -0.0394704342, -0.998786211,  0.000000000,
				0.878083110,  -0.476412654,   0.0447351933, 0.000000000,
				-0.0104349069, -0.0673804879,  0.992244065,  1.00000000,						
			};
			pelvisIdle = RedMatrix4x4( data );
		}								
	}
	return pelvisIdle;
}


void CutsceneActor::ResampleFinalPose()
{
	CEntity* e = m_entity.Get();
	CAnimatedComponent* ac = e ? e->GetRootAnimatedComponent() : nullptr;
	SBehaviorSampleContext* cont = ac ?  ac->GetBehaviorGraphSampleContext() : nullptr;

	if ( cont )
	{
		Uint32 pelvisBone = 9;
		Uint32 trajectoryBone = 1;
		Uint32 rootBone = 0;

		if( !cont->IsValid() )
		{
			return;
		}
		SBehaviorGraphOutput& pose = cont->GetMainPose();
		if ( pose.IsValid() && m_cutscene->GetCsTemplate()->SampleActorPoseInTime( m_name, m_cutscene->GetDuration(), pose ) )
		{
			if ( pose.m_numBones > pelvisBone && pose.m_outputPose[trajectoryBone].Translation.DistanceSquaredTo( RedVector4::ZERO_3D_POINT ) < 0.001f )
			{
				const AnimQsTransform& rootA_MS = pose.m_outputPose[ rootBone ];
				const AnimQsTransform& pelvisA_LS = pose.m_outputPose[ pelvisBone ];			

				AnimQsTransform pelvisA_MS;
				pelvisA_MS.SetMul( rootA_MS, pelvisA_LS );
				AnimQsTransform pelvisB_MS;
				pelvisB_MS.Set( HACK_GetPelvisIdleOffset() );

				const AnimVector4 pelvisDirLS = AnimVector4( 0.f, 1.f, 0.f );
				AnimVector4 pelvisDisA_MS;
				AnimVector4 pelvisDisB_MS;
				pelvisDisA_MS.RotateDirection( pelvisA_MS.Rotation, pelvisDirLS );
				pelvisDisB_MS.RotateDirection( pelvisB_MS.Rotation, pelvisDirLS );

				const Vector pelvisDisVecA_MS = reinterpret_cast<Vector&>( pelvisDisA_MS );
				const Vector pelvisDisVecB_MS = reinterpret_cast<Vector&>( pelvisDisB_MS );

				const Float angleRad = MathUtils::VectorUtils::GetAngleRadAroundAxis( pelvisDisVecA_MS, pelvisDisVecB_MS, Vector( 0.f, 0.f, 1.f ) );
				const Float angleDed = RAD2DEG( angleRad );

				AnimQsTransform offset_MS( AnimQsTransform::IDENTITY );
				offset_MS.Translation = Sub( pelvisA_MS.Translation, pelvisB_MS.Translation );			
				offset_MS.Rotation.SetAxisAngle( BehaviorUtils::RedVectorFromAxis( A_Z ), angleRad );
				pose.m_outputPose[trajectoryBone] = offset_MS;
			}

			pose.ExtractTrajectory( ac );

			rootBone = 0;
		}
	}
}

void CutsceneActor::Unlock( Bool isSkipped )
{
	if ( ( isSkipped || !IsCutsceneFinished() ) && !m_wasSwitchedToGameplay )
	{
		FinishAnimation( isSkipped );
	}

	CEntity* entityPtr = m_entity.Get();

	if ( m_wasHiddenByScene && entityPtr )
	{
		entityPtr->SetHideInGame( true, true, CEntity::HR_Scene );
	}

	if ( m_wasSnapedToNavigableSpace && entityPtr )
	{
		if ( CAnimatedComponent* root = entityPtr->GetRootAnimatedComponent() )
		{
			root->SnapToNavigableSpace( true );
		}
	}

	if ( m_source != AS_Spawned )
	{
		// Teleport actor before set enable animated components
		// then moving agent components will find path engine position.
		if ( !m_setDeathPose && !m_wasSwitchedToGameplay )
		{
			SetEndingPos();
			ResampleFinalPose();
		}

		LockStacks( false );

		DetachGraph();

		//if ( m_wasSwitchedToGameplay )
		//{
		//	BlendPoseToGameplay( m_switchedToGameplayBlendTime );
		//}

		// Set animated components after
		PreparePlayableElements( false );

		// Reset external control
		SetExternalControl( false );
	}


	m_isLocked = false;
}

Bool CutsceneActor::IsLocked() const
{
	return m_isLocked;
}

void CutsceneActor::ParanoidCheck()
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CAnimatedComponent* ac = m_playableElements[ i ].Get( *this );
		
		ac->OnCutsceneDebugCheck();
	}

	CEntity* entity = m_entity.Get();
	if( entity )
	{
		Matrix offset = m_cutscene->GetCsPosition();
		Vector pos = offset.GetTranslation();
		EulerAngles rot = offset.ToEulerAngles();

		const Vector& entPos = entity->GetWorldPositionRef();
		EulerAngles entRot = entity->GetWorldRotation();

		entRot.Normalize();
		rot.Normalize();

		if ( !Vector::Equal3( entPos, pos ) || !rot.AlmostEquals( entRot ) )
		{
			SetStartingPos();
		}
	}
}

Bool CutsceneActor::IsReady( String& msg ) const
{
	if ( m_type == CAT_None )
	{
		msg = String::Printf( TXT("Actor '%ls' type is 'None'"), m_name.AsChar() );
		return false;
	}

	CEntity* ent = m_entity.Get(); 
	if ( ent == NULL )
	{
		msg = String::Printf( TXT("Actor's '%ls' entity was despawned"), m_name.AsChar() );
		return false;
	}

	if ( ent && ent->IsSpawned() == false )
	{
		msg = String::Printf( TXT("Actor '%ls' is spawning"), m_name.AsChar() );
		return false;
	}

	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		const CutsceneActorPlayableElement& elem = m_playableElements[i];

		const CAnimatedComponent* ac = elem.Get( *this );

		if ( ac == NULL )
		{
			msg = String::Printf( TXT("Actor's '%ls' animated component was destroyed"), m_name.AsChar() );
			return false;
		}

		const CBehaviorGraphStack* stack = ac->GetBehaviorStack();

		if ( stack == NULL )
		{
			msg = String::Printf( TXT("Actor's '%ls' '%ls' stack is NULL"), m_name.AsChar(), ac->GetName().AsChar() );
			return false;
		}
	}

	return true;
}

Bool CutsceneActor::AttachGraphAndFindController()
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		if ( !AttachGraphAndFindController( elem ) )
		{
			return false;
		}
	}

	return true;
}

Bool CutsceneActor::AttachGraphAndFindController( CutsceneActorPlayableElement& elem )
{
	// Get animated component
	CAnimatedComponent* ac = elem.Get( *this );

	if ( !ac || !ac->GetBehaviorStack() || !ac->GetAnimationContainer() )
	{
		return false;
	}

	// Add animset
	ac->GetAnimationContainer()->AddAnimationSet( m_cutscene->GetCsTemplate() );

	// Check stack
	CBehaviorGraphStack* stack = ac->GetBehaviorStack();
	if ( stack->IsLocked() )
	{
		m_cutscene->ThrowError( String::Printf( TXT("Actor's '%ls' stack is locked"), m_name.AsChar() ) );
		return false;
	}

	// Deactivate all instances
	VERIFY( stack->DeactivateAndResetAllInstances() );

	// Attack behavior
	if ( stack->AttachBehaviorInstance( CNAME( Cutscene ) ) == false )
	{
		m_cutscene->ThrowError( String::Printf( TXT("Couldn't activate cutscene behavior instance for actor '%ls'"), m_name.AsChar() ) );
		return false;
	}

	// Get behavior instance
	CBehaviorGraphInstance* instance = stack->GetBehaviorGraphInstance( CNAME( Cutscene ) );
	if ( !instance )
	{
		m_cutscene->ThrowError( String::Printf( TXT("Couldn't find cutscene behavior intance for actor '%ls'"), m_name.AsChar() ) );
		ASSERT( instance );
		return false;
	}

	// Oh no... <= curr instance ( default ) is Cutscene and stack->DeactivateAllInstances() + AttachBehaviorInstance
	if ( !instance->IsActive() )
	{
		instance->Activate();
	}

	// Find controller
	String controllerMsg;
	CBehaviorGraphCutsceneControllerNode* controller = FindController( instance, controllerMsg );
	if ( controller )
	{
		elem.m_controller = controller;
	}
	else
	{
		m_cutscene->ThrowError( String::Printf( TXT("%s for actor '%ls' '%ls'"), controllerMsg.AsChar(), m_name.AsChar(), ac->GetName().AsChar() ) );
		return false;
	}

	return true;
}

CBehaviorGraphCutsceneControllerNode* CutsceneActor::FindController( CBehaviorGraphInstance* instance, String& msg ) const
{
	TDynArray< CBehaviorGraphCutsceneControllerNode* > nodes;
	instance->GetNodesOfClass( nodes );

	ASSERT( nodes.Size() > 0 );
	if ( nodes.Size() == 0 )
	{
		msg = TXT("Couldn't find cs behavior controller" );
		return NULL;
	}

	CBehaviorGraphCutsceneControllerNode* controller = NULL;

	for ( Uint32 j=0; j<nodes.Size(); ++j )
	{
		if ( nodes[j]->CanWork( *instance ) )
		{
			ASSERT( !controller );
			if ( controller )
			{
				msg = TXT("Find more then one cs behavior controller" );
				return NULL;
			}

			controller = nodes[j];
		}
	}

	if ( controller )
	{
		return controller;
	}
	else
	{
		msg = String::Printf( TXT("Couldn't find cs behavior controller") );
		ASSERT( controller );
		return NULL;
	}
}

void CutsceneActor::DetachGraph()
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		// Get animated component
		CAnimatedComponent* ac = elem.Get( *this );

		if ( !ac || !ac->GetBehaviorStack() || !ac->GetAnimationContainer() )
		{
			continue;
		}

		ac->GetAnimationContainer()->RemoveAnimationSet( m_cutscene->GetCsTemplate() );

		Bool ret = ac->GetBehaviorStack()->HasInstance( CNAME( Cutscene ) );
		if ( ret )
		{
			ac->GetBehaviorStack()->DetachBehaviorInstance( CNAME( Cutscene ) );
		}

		// Activate all instances
		VERIFY( ac->GetBehaviorStack()->ActivateAllInstances() );
	}
}

void CutsceneActor::PreparePlayableElements( Bool flag )
{
	const Uint32 elemCount = m_playableElements.Size();
	for ( Uint32 i=0; i<elemCount; ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		PreparePlayableElements( elem, flag );
	}
}

void CutsceneActor::PreparePlayableElements( CutsceneActorPlayableElement& elem, Bool flag )
{
	CAnimatedComponent* ac = elem.Get( *this );

	if ( ac )
	{
		ac->SetUseExtractedMotion( !flag );
		ac->SetUseExtractedTrajectory( !flag );
		ac->SetDispSkeleton( ACDD_SkeletonBone, flag );
		//ac->ForceUpdateSkinning();
		ac->ForceRestartFrameSkipping();

		// dUd - we don't want to enable death guys
		if ( !m_setDeathPose )
		{
			ac->SetACMotionEnabled( !flag );
		}

		CBehaviorGraphInstance* instance = ac->GetBehaviorStack()->GetBehaviorGraphInstance( CNAME( Cutscene ) );
		if ( !instance )
		{
			return;
		}

		if ( flag )
		{
			for ( Uint32 j=0; j<elem.m_animations.Size(); ++j )
			{
				elem.m_controller->AddCutsceneAnimation( *instance, elem.m_animations[ j ] );
			}
		}
		else
		{
			elem.m_controller->ResetRuntimeAnimation( *instance );
		}
	}
}

void CutsceneActor::LockStacks( Bool flag )
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		LockStacks( elem, flag );
	}
}

void CutsceneActor::LockStacks( CutsceneActorPlayableElement& elem, Bool flag )
{
	CAnimatedComponent* ac = elem.Get( *this );

	if ( ac && ac->GetBehaviorStack() )
	{
		ac->GetBehaviorStack()->Lock( flag );

		if ( !flag )
		{
			ASSERT( !ac->GetBehaviorStack()->IsLocked() );
		}
	}
}

void CutsceneActor::SetExternalControl( Bool flag )
{
    if ( CEntity* e = m_entity.Get() )
    {
		// TODO decide if it can be triggered outside game mode (eg scene preview)
		//if ( m_cutscene->IsInGame() )
		{
			// Notify all components of this entity
			if ( flag )
			{
				e->OnCutsceneStarted();
			}
			else
			{
				e->OnCutsceneEnded();
			}
		}
    }
}

Bool CutsceneActor::IsCutsceneFinished() const
{
	return m_cutscene->GetTime() >= m_cutscene->GetDuration();
}

void CutsceneActor::FinishAnimation( Bool isSkipped )
{
	if ( !isSkipped )
	{
		CSyncInfo info;
		info.m_prevTime = m_cutscene->GetTime();
		info.m_currTime = m_cutscene->GetDuration();

		Update( info, NULL );
	}

	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CutsceneActorPlayableElement& elem = m_playableElements[i];

		CAnimatedComponent* ac = elem.GetRestrict( *this );

		if ( ac && ac->GetBehaviorStack() && ac->GetBehaviorGraphUpdateContext() && elem.m_controller )
		{
			if ( CBehaviorGraphInstance* instance = ac->GetBehaviorStack()->GetBehaviorGraphInstance( CNAME( Cutscene ) ) )
			{
				CSyncInfo info;
				elem.m_controller->GetSyncInfo( *instance, info );

				ASSERT( info.m_prevTime < info.m_currTime );
				ASSERT( MAbs( info.m_currTime - m_cutscene->GetDuration() ) < 0.01f );

				TDynArray< CAnimationEventFired > evts;
				elem.m_controller->CollectEvents( *instance, info, evts );

				ac->GetBehaviorStack()->Update( ac->GetBehaviorGraphUpdateContext(), 0.f );
				ac->ForceBehaviorPose();

				for ( CAnimationEventFired& e : evts )
				{
					e.m_extEvent->Process( e, ac );
				}
			}
		}
	}
}

void CutsceneActor::SetStartingPos()
{
	Matrix offset = m_cutscene->GetCsPosition();
	Vector pos = offset.GetTranslation();
	EulerAngles rot = offset.ToEulerAngles();

	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		entity->Teleport( pos, rot );

		if ( IActorInterface* a = entity->QueryActorInterface() )
		{
			if ( a->GetVisualDebug() )
			{
				a->GetVisualDebug()->AddAxis( CNAME( Cutscene ), 0.6f, pos, rot, true, 10.f );
				a->GetVisualDebug()->AddSphere( CNAME( Cutscene2 ), 0.6f, pos, true, Color::GREEN, 10.f );
			}
		}
	}
}

void CutsceneActor::SetEndingPos()
{
	Vector pos = m_endPos.GetTranslation();
	EulerAngles rot = m_endPos.ToEulerAngles();

	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		entity->Teleport( pos, rot );

		if ( CAnimatedComponent* ac = entity->GetRootAnimatedComponent() )
		{
			ac->SetThisFrameTempLocalToWorld( m_endPos );
		}

		if ( m_type == CAT_Prop )
		{
			// Transform have to be forced because next cutscene can get invalid ( not updated ) position as endPos
			entity->ForceUpdateTransformNodeAndCommitChanges();
			entity->ForceUpdateBoundsNode();
		}

		if ( IActorInterface* a = entity->QueryActorInterface() )
		{
			if ( a->GetVisualDebug() )
			{
				a->GetVisualDebug()->AddAxis( CNAME( Cutscene ), 0.6f, pos, rot, true, 10.f );
				a->GetVisualDebug()->AddSphere( CNAME( Cutscene2 ), 0.6f, pos, true, Color::GREEN, 10.f );
			}
		}
	}
}

void CutsceneActor::BlendPoseToGameplay( Float blendTime )
{
	if ( CEntity* e = m_entity.Get() )
	{
		const Matrix pelvisWS = m_cutscene->CalcActorFinalPelvisPosition( e );

		if ( CAnimatedComponent* ac = e->GetRootAnimatedComponent() )
		{
			// Set pelvis offset - this will solve problems with teleport
			ac->BlendPelvisWS( pelvisWS, blendTime );

			// Override pose for frozen nodes ( for smooth transition ) - this will solve problem with pose blending
			ResampleFinalPose();
		}
	}
}

Bool CutsceneActor::ApplyCsAppearance()
{
	//CEntity* entity = m_entity.Get();
	//if ( entity && m_newApp != CName::NONE )
	//{
	//	m_prevApp = entity->GetAppearance();
	//	if ( m_prevApp != m_newApp )
	//	{
	//		entity->ApplyAppearance( m_newApp );
	//		return true;
	//	}
	//	else
	//	{
	//		m_prevApp = CName::NONE;
	//	}
	//	
	//}

	return false;
}

void CutsceneActor::RevertCsAppearance()
{
	//CEntity* entity = m_entity.Get();
	//if ( entity && m_prevApp != CName::NONE )
	//{
	//	entity->ApplyAppearance( m_prevApp );
	//}
}

void CutsceneActor::SetMimics( Bool flag )
{
	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		IActorInterface* actor = entity->QueryActorInterface();
		if ( actor )
		{
			if ( flag )
			{
				if ( !actor->MimicOn() )
				{
					LOG_ENGINE( TXT("Cutscene actor doesn't have mimic high '%ls'"), entity->GetFriendlyName().AsChar() );
				}
			}
			else
			{
				actor->MimicOff();
			}
		}
	}
}

void CutsceneActor::SendKillEvent()
{
	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		entity->CallEvent( CNAME( OnCutsceneDeath ) );
	}
}

void CutsceneActor::CacheDeathPose()
{
	for ( Uint32 i=0; i<m_playableElements.Size(); ++i )
	{
		CAnimatedComponent* ac = m_playableElements[i].Get( *this );
		if ( ac && ac->GetBehaviorStack() )
		{
			ac->GetBehaviorStack()->FreezePose();
		}
	}
}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
