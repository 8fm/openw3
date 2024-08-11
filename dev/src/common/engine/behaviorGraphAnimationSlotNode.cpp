/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationSlotNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../engine/entity.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "skeletalAnimationContainer.h"
#include "skeletalAnimationEntry.h"
#include "skeleton.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"
//#define DEBUG_SLOT_LISTENERS

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

SBehaviorSlotSetup::SBehaviorSlotSetup()
	: m_looped( false )
	, m_motionEx( true )
	, m_speedMul( 1.f )
	, m_offset( 0.f )
	, m_blendIn( 0.f )
	, m_blendOut( 0.2f )
	, m_blendInType( BTBM_Blending )
	, m_blendOutType( BTBM_Blending )
	, m_mergeBlendedSlotEvents( true )
	, m_useFovTrack( true )
	, m_useDofTrack( true )
	, m_listener( NULL )
	, m_animationShifts( NULL )
	, m_weight( 1.f )
{
}

//////////////////////////////////////////////////////////////////////////

Vector CSlotAnimationShiftingInterval::GetShiftForInterval( Float prevTime, Float currTime )
{
	if ( m_totalTimeInv == 0.f )
		return m_shift;

	Float startTime = ( prevTime < m_startTime ) ? m_startTime : prevTime;
	Float stopTime  = ( currTime > m_stopTime  ) ? m_stopTime  : currTime;
	Float deltaTime = stopTime - startTime;
	ASSERT( deltaTime >= 0.f );

	return m_shift * ( deltaTime * m_totalTimeInv );
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT

IMPLEMENT_ENGINE_CLASS( CAnimatedComponentAnimationSyncToken );

CAnimatedComponentAnimationSyncToken::CAnimatedComponentAnimationSyncToken()
	: m_syncedAnimated( nullptr )
{

}

void CAnimatedComponentAnimationSyncToken::Sync( CName animationName, const CSyncInfo& syncInfo, Float weight ) 
{
	CAnimatedComponent* animated = m_syncedAnimated.Get();
	if ( animated && animated->IsAttached() && animationName )
	{
		animated->PlayAnimationOnSkeletonWithSync( animationName, syncInfo );
	}
}

void CAnimatedComponentAnimationSyncToken::Reset()
{
	CAnimatedComponent* animated = m_syncedAnimated.Get();
	if ( animated )
	{
		animated->StopAllAnimationsOnSkeleton();
		animated->ForceTPose( false );
	}
}

Bool CAnimatedComponentAnimationSyncToken::IsValid() const
{
	CAnimatedComponent* animated = m_syncedAnimated.Get();
	return animated != nullptr;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationBaseSlotNode );

void CBehaviorGraphAnimationBaseSlotNode::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );
}

void CBehaviorGraphAnimationBaseSlotNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("name") )
	{
		// CName conversion - editor only
		m_slotName = CName( GetName() );
	}
}

void CBehaviorGraphAnimationBaseSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_slotAnimationListener;
	compiler << i_slotAnimationShift;
	compiler << i_motionEx;
	compiler << i_looped;
	compiler << i_useFovTrack;
	compiler << i_useDofTrack;
	compiler << i_animationPaused;
	compiler << i_speed;
	compiler << i_offset;
	compiler << i_needToRefreshSyncTokens;
}

void CBehaviorGraphAnimationBaseSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_motionEx ]		= m_applyMotion;
	instance[ i_looped ]		= m_loopPlayback;
	instance[ i_useFovTrack ]	= m_useFovTrack;
	instance[ i_useDofTrack ]	= m_useDofTrack;
	instance[ i_animationPaused ] = false;
	instance[ i_speed ]			= m_playbackSpeed;
	instance[ i_offset ]		= 0.f;
	instance[ i_needToRefreshSyncTokens ] = false;

	instance[ i_animation ] = NULL;
	instance[ i_slotAnimationListener ] = 0;

	SSlotAnimationShift* shifts = new SSlotAnimationShift;
	shifts->m_animationShift = Vector::ZERO_3D_POINT;

	instance[ i_slotAnimationShift ] = reinterpret_cast< TGenericPtr >( shifts );
}

void CBehaviorGraphAnimationBaseSlotNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	SSlotAnimationShift* shifts = GetSlotShift( instance );

	delete shifts;
	shifts = NULL;

	TBaseClass::OnReleaseInstance( instance );
}

void CBehaviorGraphAnimationBaseSlotNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_motionEx );
	INST_PROP( i_looped );
	INST_PROP( i_useFovTrack );
	INST_PROP( i_useDofTrack );
	INST_PROP( i_animationPaused );
	INST_PROP( i_speed );
	INST_PROP( i_offset );
	INST_PROP( i_needToRefreshSyncTokens );
	INST_PROP( i_slotAnimationListener );
	INST_PROP( i_slotAnimationShift );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimationBaseSlotNode::GetCaption() const
{
	return String::Printf( TXT("Animation base slot [ %s ]"), m_name.AsChar() );
}

void CBehaviorGraphAnimationBaseSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

Color CBehaviorGraphAnimationBaseSlotNode::GetTitleColor() const
{
	return Color( 255, 64, 64 );
}

#endif

void CBehaviorGraphAnimationBaseSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimationBaseSlot );
	if ( instance[ i_animation ] )
	{
		// Update animation
		if( instance[ i_animationPaused ] )
		{
			timeDelta = 0.f;
		}

		CBehaviorGraphAnimationNode::OnUpdate( context, instance, timeDelta );

		Bool& needToRefreshSyncTokens = instance[ i_needToRefreshSyncTokens ];
		if ( needToRefreshSyncTokens )
		{
			needToRefreshSyncTokens = false;
			ClearSyncTokens( instance );
			CollectSyncTokens( instance );
		}

		UpdateSlotShifts( instance, timeDelta );
	}
}

void CBehaviorGraphAnimationBaseSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( instance[ i_animation ] )
	{
		// Sample animation
		CBehaviorGraphAnimationNode::Sample( context, instance, output );

		// Add animation shift to translation
		SSlotAnimationShift* shifts = GetSlotShift( instance );

		if ( shifts->m_animationShift != Vector::ZEROS )
		{
#ifdef USE_HAVOK_ANIMATION
			hkVector4 hkTranslation  = output.m_deltaReferenceFrameLocal.getTranslation();
			const hkVector4& hkDelta = TO_CONST_HK_VECTOR_REF( shifts->m_animationShift );
			hkTranslation.add3clobberW( hkDelta );
			output.m_deltaReferenceFrameLocal.setTranslation( hkTranslation );
#else
			RedVector4 translation  = output.m_deltaReferenceFrameLocal.GetTranslation();
			const RedVector4& delta = reinterpret_cast< const RedVector4& >( shifts->m_animationShift );

			translation.X += delta.X;
			translation.Y += delta.Y;
			translation.Z += delta.Z;
			output.m_deltaReferenceFrameLocal.SetTranslation( translation );
#endif
			shifts->m_animationShift = Vector::ZEROS;
		}
	}
}

void CBehaviorGraphAnimationBaseSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if( instance[ i_slotAnimationListener ] != 0 )
	{
		// listener copied to temporary variable so it can be set again in callback
		ISlotAnimationListener* listener = reinterpret_cast< ISlotAnimationListener* >( instance[ i_slotAnimationListener ] );

#ifdef DEBUG_SLOT_LISTENERS
		LOG_ENGINE( TXT("Anim slot OnDeactivated with listener '%ls', 0x%X, '%ls'"), listener->GetListenerName().AsChar(), listener, instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
#endif

		instance[ i_slotAnimationListener ] = 0;
		listener->OnSlotAnimationEnd( this, instance, ISlotAnimationListener::S_Deactivated );
	}

	if ( IsSlotActive( instance ) )
	{
		const CSkeletalAnimationSetEntry* anim = instance[ i_animation ];
		if ( anim )
		{
			BEH_LOG( TXT("Slot node '%ls' for entity '%ls' is running but will be deactivating. Slot animation '%ls' will be stopped."),
				GetName().AsChar(), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar(), anim->GetName().AsString().AsChar() );
		}
		else
		{
			BEH_LOG( TXT("Slot node '%ls' for entity '%ls' is running but will be deactivating. There is no slot animation."),
				GetName().AsChar(), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		}

		OnReset( instance );	
	}
}

void CBehaviorGraphAnimationBaseSlotNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	SlotReset( instance );
}

CBehaviorGraphAnimationBaseSlotNode::SSlotAnimationShift* CBehaviorGraphAnimationBaseSlotNode::GetSlotShift( CBehaviorGraphInstance& instance ) const
{
	SSlotAnimationShift* shift = reinterpret_cast< SSlotAnimationShift* >( instance[ i_slotAnimationShift ] );
	return shift;
}

Bool CBehaviorGraphAnimationBaseSlotNode::IsSlotActive( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_animation ] != NULL;
}

Bool CBehaviorGraphAnimationBaseSlotNode::IsPlayingSlotAnimation( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_animation ] != NULL;
}

void CBehaviorGraphAnimationBaseSlotNode::PauseAnimation( CBehaviorGraphInstance& instance, const Bool& pause  ) const
{
	instance[ i_animationPaused ] = pause;
}

Bool CBehaviorGraphAnimationBaseSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, const CName& animation, const SBehaviorSlotSetup* slotSetup ) const
{
#ifndef NO_SLOT_ANIM
	if ( IsSlotActive( instance ) )
	{
		StopAnimation( instance );
	}
	else
	{
		OnReset( instance );
	}

	CName animationName = GetAnimationFullName( animation );

	if ( IsValid( instance ) && SetRuntimeAnimationByName( instance, animationName ) )
	{
		if ( slotSetup )
		{ 
			SetupSlot( instance, slotSetup );
		}

		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

Bool CBehaviorGraphAnimationBaseSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup ) const
{
#ifndef NO_SLOT_ANIM
	if ( IsSlotActive( instance ) )
	{
		StopAnimation( instance );
	}
	else
	{
		OnReset( instance );
	}

	if ( IsValid( instance ) && SetRuntimeAnimation( instance, skeletalAnimation ) )
	{
		if ( slotSetup )
		{ 
			SetupSlot( instance, slotSetup );
		}

		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

Bool CBehaviorGraphAnimationBaseSlotNode::StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime /*= 0.0f*/  ) const
{
	ASSERT( Red::Math::NumericalUtils::Abs( blendOutTime ) < 0.0001f, TXT(" This node does not serve blending at stop!"));

	OnReset( instance );

	if( instance[ i_slotAnimationListener ] != 0 )
	{
		// listener copied to temporary variable so it can be set again in callback
		ISlotAnimationListener* listener = reinterpret_cast< ISlotAnimationListener* >( instance[ i_slotAnimationListener ] );

#ifdef DEBUG_SLOT_LISTENERS
		LOG_ENGINE( TXT("Anim slot StopAnimation with listener '%ls', 0x%X, '%ls'"), listener->GetListenerName().AsChar(), listener, instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
#endif

		instance[ i_slotAnimationListener ] = 0;
		listener->OnSlotAnimationEnd( this, instance, ISlotAnimationListener::S_Stopped );		
	}

	return instance[ i_animation ] != nullptr;
}

Bool CBehaviorGraphAnimationBaseSlotNode::DetachListener( CBehaviorGraphInstance& instance, ISlotAnimationListener* listener ) const
{
	if ( instance[ i_slotAnimationListener ] == reinterpret_cast< void* >( listener ) )
	{
		instance[ i_slotAnimationListener ] = NULL;
		return true;
	}
	else
	{
		return instance[ i_slotAnimationListener ] == NULL;
	}
}

Bool CBehaviorGraphAnimationBaseSlotNode::HasListener( const CBehaviorGraphInstance& instance, ISlotAnimationListener* listener ) const
{
	return instance[ i_slotAnimationListener ] == reinterpret_cast< void* >( listener );
}

CName CBehaviorGraphAnimationBaseSlotNode::GetAnimationFullName( const CName& animation ) const
{
	CName animationName = animation;

	// Add prefix/sufix - VERY HEAVY! But very rarely called.
	if( ! m_animPrefix.Empty() && ! m_animSufix.Empty() )
	{
		animationName = CName( m_animPrefix + animation.AsString() + m_animSufix );
	}
	else if( ! m_animPrefix.Empty() )
	{
		animationName = CName( m_animPrefix + animation.AsString() );
	}
	else if( ! m_animSufix.Empty() )
	{
		animationName = CName( animation.AsString() + m_animSufix );
	}

	return animationName;
}

void CBehaviorGraphAnimationBaseSlotNode::SlotReset( CBehaviorGraphInstance& instance ) const
{
	// Reset time, loop, flags etc.
	InternalReset( instance );

	// Reset aniamtion ptr
	instance[ i_animation ]	= NULL;

	// Reset shifts
	SSlotAnimationShift* shifts = GetSlotShift( instance );
	shifts->m_animationShiftingIntervals.Clear();
	shifts->m_animationShift = Vector::ZEROS;

	// Reset slot params
	instance[ i_motionEx ]		= m_applyMotion;
	instance[ i_looped ]		= m_loopPlayback;
	instance[ i_useFovTrack ]	= m_useFovTrack;
	instance[ i_useDofTrack ]	= m_useDofTrack;
	instance[ i_animationPaused ] = false;
	instance[ i_speed ]			= m_playbackSpeed;
	instance[ i_offset ]		= 0.f;
	instance[ i_needToRefreshSyncTokens ] = false;
}

void CBehaviorGraphAnimationBaseSlotNode::SetupSlot( CBehaviorGraphInstance& instance, const SBehaviorSlotSetup* setup ) const
{
	instance[ i_looped ] =		setup->m_looped;
	instance[ i_motionEx ] =	setup->m_motionEx;
	instance[ i_useFovTrack ] = setup->m_useFovTrack;
	instance[ i_useDofTrack ] = setup->m_useDofTrack;
	instance[ i_animationPaused ] = false;
	instance[ i_speed ]		  = setup->m_speedMul;
	instance[ i_offset ]	  = setup->m_offset;
	instance[ i_needToRefreshSyncTokens ] = false;
	
	SetSlotAnimationListener( instance, setup->m_listener );

	if ( setup->m_animationShifts )
	{
		SSlotAnimationShift* shifts = GetSlotShift( instance );
		shifts->m_animationShiftingIntervals = * setup->m_animationShifts;
	}
}

void CBehaviorGraphAnimationBaseSlotNode::ApplyAnimOffset( CBehaviorGraphInstance& instance ) const
{
	Float offset = instance[ i_offset ];
	if ( offset > 0.f )
	{
		ASSERT( offset <= 1.f );
		offset = Min( offset, 1.f );

		const Float duration = GetDuration( instance );

		Float& prevTime = instance[ i_prevTime ];
		Float& currTime = instance[ i_localTime ];
		
		prevTime = 0.f;
		currTime = offset * duration;
	}
}

Bool CBehaviorGraphAnimationBaseSlotNode::IsLooped( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_looped ];
}

Bool CBehaviorGraphAnimationBaseSlotNode::ApplyMotion( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_motionEx ];
}

Bool CBehaviorGraphAnimationBaseSlotNode::UseFovTrack( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_useFovTrack ];
}

Bool CBehaviorGraphAnimationBaseSlotNode::UseDofTrack( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_useDofTrack ];
}

Float CBehaviorGraphAnimationBaseSlotNode::GetPlaybackSpeed( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_speed ];
}

void CBehaviorGraphAnimationBaseSlotNode::FirstUpdate( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::FirstUpdate( instance );

	// Apply anim offset
	ApplyAnimOffset( instance );
}

void CBehaviorGraphAnimationBaseSlotNode::OnAnimationFinished( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnAnimationFinished( instance );

	// Listener
	if( instance[ i_slotAnimationListener ] != 0 )
	{
		// Listener copied to temporary variable so it can be set again in callback
		ISlotAnimationListener* listener = reinterpret_cast< ISlotAnimationListener* >( instance[ i_slotAnimationListener ] );

#ifdef DEBUG_SLOT_LISTENERS
		LOG_ENGINE( TXT("Anim slot OnAnimationFinished with listener '%ls', 0x%X, '%ls'"), listener->GetListenerName().AsChar(), listener, instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
#endif

		instance[ i_slotAnimationListener ] = 0;
		listener->OnSlotAnimationEnd( this, instance, ISlotAnimationListener::S_Finished );						
	}
}

void CBehaviorGraphAnimationBaseSlotNode::SetSlotAnimationListener( CBehaviorGraphInstance& instance, ISlotAnimationListener* listener ) const
{
	instance[ i_slotAnimationListener ] = reinterpret_cast< TGenericPtr >( listener );

	if ( listener )
	{
#ifdef DEBUG_SLOT_LISTENERS
		LOG_ENGINE( TXT("Anim slot SetSlotAnimationListener with listener '%ls' 0x%X, '%ls'"), listener->GetListenerName().AsChar(), listener, instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
#endif
	}
}

Bool CBehaviorGraphAnimationBaseSlotNode::IsValid( CBehaviorGraphInstance& instance ) const
{
	return true;
}

Bool CBehaviorGraphAnimationBaseSlotNode::SetRuntimeAnimationByName( CBehaviorGraphInstance& instance, const CName &name ) const
{
	RefreshAnimation( instance, name );

	if ( !name.Empty() && instance[ i_animation ] == NULL )
	{
		BEH_WARN( TXT("Couldn't find slot animation '%ls' for %s %s"), 
			name.AsString().AsChar(), instance.GetAnimatedComponent()->GetName().AsChar(), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	}

	return instance[ i_animation ] ? true : false;
}

Bool CBehaviorGraphAnimationBaseSlotNode::SetRuntimeAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* anim ) const
{
	instance[ i_animation ] = anim;

	return instance[ i_animation ] ? true : false;
}

void CBehaviorGraphAnimationBaseSlotNode::UpdateSlotShifts( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	Float prevTime = timeDelta > 0.f ? instance[ i_prevTime ] : instance[ i_localTime ];
	Float currTime = timeDelta > 0.f ? instance[ i_localTime ] : instance[ i_prevTime ];

	SSlotAnimationShift* shifts = GetSlotShift( instance );

	// Typical situation - prevTime <= currTime
	if ( prevTime <= currTime )
	{
		for ( Uint32 i = 0; i < shifts->m_animationShiftingIntervals.Size(); ++i )
		{
			CSlotAnimationShiftingInterval & shift = shifts->m_animationShiftingIntervals[i];

			if ( currTime < shift.GetStartTime() )
				break;
			if ( prevTime > shift.GetStopTime() )
				continue;

			shifts->m_animationShift += shift.GetShiftForInterval( prevTime, currTime );
		}
	}
	// Exceptional situation - animation has looped, we have two intervals: [0, currTime] and [prevTime, totalTime]
	else
	{
		for ( Uint32 i = 0; i < shifts->m_animationShiftingIntervals.Size(); ++i )
		{
			CSlotAnimationShiftingInterval & shift = shifts->m_animationShiftingIntervals[i];

			if ( currTime > shift.GetStartTime() )
			{
				shifts->m_animationShift += shift.GetShiftForInterval( shift.GetStartTime(), currTime );
			}
			if ( prevTime < shift.GetStopTime() )
			{
				shifts->m_animationShift += shift.GetShiftForInterval( prevTime, shift.GetStopTime() );
			}
		}
	}
}

const CName& CBehaviorGraphAnimationBaseSlotNode::GetSlotName() const
{
	return m_slotName;
}

void CBehaviorGraphAnimationBaseSlotNode::SetNeedRefreshSyncTokens( CBehaviorGraphInstance& instance, Bool value ) const
{
	if ( m_gatherSyncTokens )
	{
		instance[ i_needToRefreshSyncTokens ] = value;
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphAnimationBaseSlotNode::AppendSyncTokenForEntity( CBehaviorGraphInstance& instance, const CEntity* entity ) const
{
	if ( m_gatherSyncTokens && entity )
	{
		CAnimatedComponent* animated = entity->GetRootAnimatedComponent();
		const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];
		if ( animated && animation && animated->GetAnimationContainer()->FindAnimation( animation->GetName() ) )
		{
			CAnimatedComponentAnimationSyncToken* syncToken = new CAnimatedComponentAnimationSyncToken;
			syncToken->m_syncedAnimated = animated;

			TDynArray< CAnimationSyncToken* >& tokens = instance[ i_syncTokens ];
			tokens.PushBack( syncToken );
		}
	}
}
#endif

void CBehaviorGraphAnimationBaseSlotNode::OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const
{
	TBaseClass::OnLoadingSnapshot( instance, snapshotData );
	// TODO - shift state is not stored at the moment, and if it was in different point in memory, we need to do this
	if (instance[ i_slotAnimationShift ] != snapshotData[ i_slotAnimationShift ] &&
		instance[ i_slotAnimationShift ])
	{
		delete instance[ i_slotAnimationShift ];
		instance[ i_slotAnimationShift ] = NULL;
	}
}

void CBehaviorGraphAnimationBaseSlotNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	// zero listener, as it may no longer exist, or could exist at different address
	// NOTE - this should be used only for debug, so it shouldn't break any gameplay
	instance[ i_slotAnimationListener ] = 0;

	// TODO - shift state is not stored at the moment, and if it was in different point in memory, we need to do this
	if (instance[ i_slotAnimationShift ] != previousData[ i_slotAnimationShift ])
	{
		SSlotAnimationShift* shifts = new SSlotAnimationShift;
		shifts->m_animationShift = Vector::ZERO_3D_POINT;
		instance[ i_slotAnimationShift ] = reinterpret_cast< TGenericPtr >( shifts );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicBaseSlotNode );


#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMimicBaseSlotNode::GetCaption() const
{
	return String::Printf( TXT("Mimic base slot [ %s ]"), m_name.AsChar() );
}

void CBehaviorGraphMimicBaseSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

Bool CBehaviorGraphMimicBaseSlotNode::IsValid( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	ASSERT( ac );

	if ( !ac->GetMimicSkeleton() )
	{
		return false;
	}

	return true;
}

void CBehaviorGraphMimicBaseSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicBaseSlot );
	CBehaviorGraphMimicsAnimationNode::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphMimicBaseSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( context.HasMimic() == false )
	{
		return;
	}

	CBehaviorGraphMimicsAnimationNode::Sample( context, instance, output );
}

//////////////////////////////////////////////////////////////////////////

IBehaviorGraphSlotInterface::IBehaviorGraphSlotInterface()
	: m_instance( nullptr )
	, m_slot( nullptr )
{

}

void IBehaviorGraphSlotInterface::Init( CBehaviorGraphAnimationBaseSlotNode* slot, CBehaviorGraphInstance* instance )
{
	m_slot = slot;
	m_instance = instance;

	ASSERT( IsValid() );
}

Bool IBehaviorGraphSlotInterface::IsValid( CBehaviorGraphInstance *& instance ) const
{
	instance = m_instance.Get();
	return instance != nullptr && m_slot;
}

Bool IBehaviorGraphSlotInterface::IsValid() const
{
	return m_instance.Get() != nullptr && m_slot;
}

Bool IBehaviorGraphSlotInterface::IsActive() const
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return false;
	}

	return m_slot->IsSlotActive( *instance );
}

Bool IBehaviorGraphSlotInterface::PlayAnimation( const CName& animation, const SBehaviorSlotSetup* slotSetup /* = NULL  */)
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return false;
	}

	return m_slot->PlayAnimation( *instance, animation, slotSetup );
}

Bool IBehaviorGraphSlotInterface::PlayAnimation( CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup /* = NULL  */)
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return false;
	}

	return m_slot->PlayAnimation( *instance, skeletalAnimation, slotSetup );
}

void IBehaviorGraphSlotInterface::StopAnimation()
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return;
	}

	m_slot->StopAnimation( *instance );
}

void IBehaviorGraphSlotInterface::GetSyncInfo( CSyncInfo& info ) const
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return;
	}

	m_slot->GetSyncInfo( *instance, info );
}

void IBehaviorGraphSlotInterface::SynchronizeTo( const CSyncInfo& info )
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return;
	}

	m_slot->SynchronizeTo( *instance, info );
}

const CName& IBehaviorGraphSlotInterface::GetAnimationName() const
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return CName::NONE;
	}

	const CSkeletalAnimationSetEntry* animation = m_slot->GetAnimation( *instance );
	return animation ? animation->GetName() : CName::NONE;
}

CSkeletalAnimationSetEntry* IBehaviorGraphSlotInterface::GetAnimation() const
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return NULL;
	}

	return m_slot->GetAnimation( *instance );
}

Bool IBehaviorGraphSlotInterface::GetSlotPose( SBehaviorGraphOutput& pose ) const
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return false;
	}

	CSkeletalAnimationSetEntry* animEntry = m_slot->GetAnimation( *instance );
	if ( animEntry && animEntry->GetAnimation() )
	{
		animEntry->GetAnimation()->Sample( m_slot->GetAnimTime( *instance ), pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );
	}

	return false;
}

Bool IBehaviorGraphSlotInterface::GetSlotCompressedPose( SBehaviorGraphOutput& pose ) const
{
	CBehaviorGraphInstance* instance( nullptr );
	if ( !IsValid( instance ) )
	{
		return false;
	}

	CSkeletalAnimationSetEntry* animEntry = m_slot->GetAnimation( *instance );
	CSkeleton* skeleton = instance->GetAnimatedComponent()->GetSkeleton();

	if ( skeleton && animEntry && animEntry->GetAnimation() )
	{
		VERIFY( animEntry->GetAnimation()->SampleCompressedPoseWithoutTouch( pose.m_numBones, pose.m_outputPose, pose.m_numFloatTracks, pose.m_floatTracks, skeleton ) );
	}

	return false;
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
