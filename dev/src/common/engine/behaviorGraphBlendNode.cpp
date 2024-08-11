/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorGraphNode.h"
#include "animSyncInfo.h"
#include "extAnimEvent.h"
#include "extAnimDurationEvent.h"
#include "baseEngine.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

//#define BEH_SYNC_DEBUG

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( IBehaviorSyncMethod );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodNone );
IMPLEMENT_ENGINE_CLASS( IBehaviorSyncMethodEvent );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodEventStart );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodEventProp );

IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodTime );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodDuration );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodOffset );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodProp );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodSyncPoints );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorSyncMethodSyncPointsStartOnly );

//////////////////////////////////////////////////////////////////////////

Bool IBehaviorSyncMethodEvent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == CNAME( trackName ) && readValue.GetType() == GetTypeName< CName >() )
	{
		CName eventName;
		if ( readValue.AsType< CName >( eventName ) )
		{
			m_eventName = eventName;
			return true;
		}
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

const CName& IBehaviorSyncMethodEvent::GetEventName() const
{
	return m_eventName;
}

void IBehaviorSyncMethodEvent::SetEventName( const CName& name )
{
	m_eventName = name;
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorSyncMethodEventStart::SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const
{
	ASSERT( target );

	// Get sync events
	//const CExtAnimEvent* syncEvent = GetSyncEventInTime< CExtAnimEvent >( sourceSyncInfo );
	//if ( syncEvent )
	{
		CSyncInfo targetSyncInfo;
		target->GetSyncInfo( instance, targetSyncInfo );

		// Get first target events
		const CExtAnimEvent* targetEvent = nullptr;

		if ( m_startAtRandomEvent )
		{
			const TDynArray< CExtAnimEvent* >& inputEvents = targetSyncInfo.m_syncEvents;
			const CExtAnimEvent** validEvents = (const CExtAnimEvent**)RED_ALLOCA( sizeof(CExtAnimEvent*) * inputEvents.Size() );
			const CExtAnimEvent** validEvent = validEvents;
			
			for ( Uint32 i=0; i<inputEvents.Size(); ++i )
			{
				const CExtAnimEvent* event = inputEvents[i];

				if ( event->GetEventName() == m_eventName && IsType< CExtAnimEvent >( event ) )
				{
					*validEvent = event;
					++ validEvent;
				}
			}

			if ( validEvent != validEvents )
			{
				targetEvent = validEvents[ GEngine->GetRandomNumberGenerator().Get< Int32 >( (Int32)( validEvent - validEvents ) ) ];
			}
		}
		else
		{
			// get first
			targetEvent = GetSyncEvent< CExtAnimEvent >( targetSyncInfo );
		}

		if ( targetEvent )
		{
			// Synchronize to first event in track
			targetSyncInfo.m_currTime = targetEvent->GetStartTime();
		}

		// Synchronize
		target->SynchronizeTo( instance, targetSyncInfo );
	}
}

void CBehaviorSyncMethodEventStart::Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const
{
	
}

//////////////////////////////////////////////////////////////////////////

CBehaviorSyncMethodEventProp::CBehaviorSyncMethodEventProp()
	: m_offset( 0.f )
{

}

void CBehaviorSyncMethodEventProp::SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const
{
	ASSERT( target );

	// Get sync event. Only duration events
	const CExtAnimDurationEvent* syncEvent = GetSyncEventInTime< CExtAnimDurationEvent >( sourceSyncInfo );
	if ( syncEvent )
	{
		Float progress = GetDurationProgress( syncEvent, sourceSyncInfo );
		if ( progress < 0.f )
		{
			return;
		}

		// Add offset
		progress = Clamp< Float >( progress + m_offset/100.f, 0.f, 1.f );

		// Get sync info from target node
		CSyncInfo targetSyncInfo;
		target->GetSyncInfo( instance, targetSyncInfo );

		// Get first target events
		const CExtAnimDurationEvent* targetEvent = GetSyncEvent< CExtAnimDurationEvent >( targetSyncInfo );
		if ( targetEvent )
		{
			// Calc target node's current time
			targetSyncInfo.m_currTime = GetTimeFromDurationProgress( targetEvent, progress, targetSyncInfo.m_totalTime );

			ASSERT( targetSyncInfo.m_currTime <= targetSyncInfo.m_totalTime );

			// Synchronize
			target->SynchronizeTo( instance, targetSyncInfo );
		}
	}
}

void CBehaviorSyncMethodEventProp::Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const
{

}

Float CBehaviorSyncMethodEventProp::GetDurationProgress( const CExtAnimDurationEvent* event, const CSyncInfo &info ) const
{
	Float start = info.m_prevTime;
	Float end = info.m_currTime < info.m_prevTime ? info.m_currTime + info.m_totalTime : info.m_currTime;

	Float evtStart = event->GetStartTime();
	Float duration = event->GetDuration();
	Float evtEnd = event->GetEndTimeWithoutClamp();

	Float progress = 0.f;

	if ( evtStart <= end && end <= evtEnd )
	{
		progress = ( evtEnd - end ) / duration;
	}
	else if ( evtStart <= start && start <= evtEnd )
	{
		progress = ( evtEnd - start ) / duration;
	}
	else
	{
#ifdef BEH_SYNC_DEBUG
		ASSERT( 0 );
#endif
		return -1.f;
	}

#ifdef BEH_SYNC_DEBUG
	ASSERT( progress == 0.f );
	ASSERT( progress >= -0.01f && progress <= 1.01f );
#endif
	
	return Clamp( progress, 0.f, 1.f );
}

Float CBehaviorSyncMethodEventProp::GetTimeFromDurationProgress( const CExtAnimDurationEvent* event, const Float progress, const Float totalTime ) const
{
	Float start = event->GetStartTime();
	Float duration = event->GetDuration();

	Float time = start + progress * duration;

	if ( time > totalTime )
	{
		// Looped
		time -= totalTime;
	}

#ifdef BEH_SYNC_DEBUG
	ASSERT( time >= 0.f );
#endif
	
	return time;
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorSyncMethodTime::Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const
{
	CSyncInfo firstSyncInfo;
	CSyncInfo secondSyncInfo;

	if ( firstInput && secondInput )
	{
		firstInput->GetSyncInfo( instance, firstSyncInfo );
		secondInput->GetSyncInfo( instance, secondSyncInfo );

		const Float synchronizedTime = firstSyncInfo.m_currTime * ( 1.0f - alpha ) + secondSyncInfo.m_currTime * alpha;
		firstSyncInfo.m_currTime = synchronizedTime;

		firstInput->SynchronizeTo( instance, firstSyncInfo );
		secondInput->SynchronizeTo( instance, firstSyncInfo );
	}
}

void CBehaviorSyncMethodTime::SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const
{
	if ( !target ||sourceSyncInfo.m_totalTime < 0.01f )
	{
		return;
	}

	CSyncInfo targetSyncInfo;

	target->GetSyncInfo( instance, targetSyncInfo );

	targetSyncInfo.m_currTime = Clamp( sourceSyncInfo.m_currTime, 0.0f, targetSyncInfo.m_totalTime );

	target->SynchronizeTo( instance, targetSyncInfo );
}

//////////////////////////////////////////////////////////////////////////

static Float NormalizeProgressDiff( Float diff )
{
	ASSERT(! Red::Math::NumericalUtils::IsNan( diff ) );
	while ( diff > 0.5f )
	{
		diff -= 1.0f;
	}
	while ( diff < -0.5f )
	{
		diff += 1.0f;
	}
	return diff;
}

void CBehaviorSyncMethodDuration::Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const
{
	CSyncInfo firstSyncInfo;
	CSyncInfo secondSyncInfo;

	if ( firstInput && secondInput )
	{
		firstInput->GetSyncInfo( instance, firstSyncInfo );
		secondInput->GetSyncInfo( instance, secondSyncInfo );

		const Float blendedDuration = firstSyncInfo.m_totalTime * ( 1.0f - alpha ) + secondSyncInfo.m_totalTime * alpha;

		if ( blendedDuration > 0.f )
		{
			// try to match progress between sync infos
			// when they run at different speed this will try to match progress but due to different speed, faster one will be slightly ahead - which is okayish enough
			const Float firstProgress			= firstSyncInfo.m_totalTime != 0.0f? firstSyncInfo.m_currTime / firstSyncInfo.m_totalTime : 0.0f;
			const Float firstPrevProgress		= firstSyncInfo.m_totalTime != 0.0f? firstSyncInfo.m_prevTime / firstSyncInfo.m_totalTime : 0.0f;
			const Float secondProgress			= secondSyncInfo.m_totalTime != 0.0f? secondSyncInfo.m_currTime / secondSyncInfo.m_totalTime : 0.0f;
			const Float firstProgressDiff		= NormalizeProgressDiff( firstProgress - firstPrevProgress ); // if + is going forward, if - is going backward
			const Float progressDiff			= NormalizeProgressDiff( secondProgress - firstProgress ); // if + second is faster, if - second is slower (for forward playback)
			const Float progressCoef			= firstProgressDiff > 0.0f? 1.0f + progressDiff : 1.0f - progressDiff; // progressDiff is <-0.5, 0.5> so coef will be <0.5, 1.5>
			const Float blendDuractionInv		= blendedDuration != 0.0f? 1.0f / blendedDuration : 1.0f;
			const Float firstChildMultiplier	= ( firstSyncInfo.m_totalTime * blendDuractionInv ) * progressCoef; // second is faster (>1.0f)? speed it up
			const Float secondChildMultiplier	= ( secondSyncInfo.m_totalTime * blendDuractionInv ) / progressCoef; // second is faster (>1.0f)? slow it down

			firstInput->GetSyncData( instance ).Set( firstChildMultiplier );
			secondInput->GetSyncData( instance ).Set( secondChildMultiplier );
		}
	}
}

void CBehaviorSyncMethodDuration::SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const
{
	if ( !target ||sourceSyncInfo.m_totalTime < 0.01f )
	{
		return;
	}

	CSyncInfo targetSyncInfo;

	target->GetSyncInfo( instance, targetSyncInfo );

	Float progress = sourceSyncInfo.m_currTime / sourceSyncInfo.m_totalTime;
	targetSyncInfo.m_currTime = progress * targetSyncInfo.m_totalTime;

	target->SynchronizeTo( instance, targetSyncInfo );
}

//////////////////////////////////////////////////////////////////////////

CBehaviorSyncMethodOffset::CBehaviorSyncMethodOffset()
:	m_timeOffset( 0.0f )
{
}

void CBehaviorSyncMethodOffset::Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const
{
}

void CBehaviorSyncMethodOffset::SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const
{
	if ( !target ||sourceSyncInfo.m_totalTime < 0.01f )
	{
		return;
	}

	CSyncInfo targetSyncInfo;

	target->GetSyncInfo( instance, targetSyncInfo );

	targetSyncInfo.m_currTime = Clamp( m_timeOffset, 0.0f, targetSyncInfo.m_totalTime );

	target->SynchronizeTo( instance, targetSyncInfo );
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorSyncMethodProp::Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const
{
	
}

void CBehaviorSyncMethodProp::SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const
{
	if ( !target ||sourceSyncInfo.m_totalTime < 0.01f )
	{
		return;
	}

	CSyncInfo targetSyncInfo;

	target->GetSyncInfo( instance, targetSyncInfo );

	Float progress = sourceSyncInfo.m_currTime / sourceSyncInfo.m_totalTime;
	targetSyncInfo.m_currTime = progress * targetSyncInfo.m_totalTime;

	target->SynchronizeTo( instance, targetSyncInfo );
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorSyncMethodSyncPoints::OnPostLoad()
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	CBehaviorGraphNode* node = Cast< CBehaviorGraphNode >( GetParent() );
	ASSERT( node );
	
	node->SetDeprecated( true, TXT("Node uses deprecated synchronization - CBehaviorSyncMethodSyncPoints") );
#endif
}

void CBehaviorSyncMethodSyncPointsStartOnly::OnPostLoad()
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	CBehaviorGraphNode* node = Cast< CBehaviorGraphNode >( GetParent() );
	ASSERT( node );

	node->SetDeprecated( true, TXT("Node uses deprecated synchronization - CBehaviorSyncMethodSyncPointsStartOnly") );
#endif
}

//////////////////////////////////////////////////////////////////////////

const Float CBehaviorGraphBlendNode::ACTIVATION_THRESHOLD = 0.001f;

CBehaviorGraphBlendNode::CBehaviorGraphBlendNode()
	: m_synchronize( true )
	, m_firstInputValue( 0.0f )
	, m_secondInputValue( 1.0f ) 
	, m_syncMethod( NULL )
{
}

void CBehaviorGraphBlendNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlValue;
	compiler << i_prevControlValue;
}

void CBehaviorGraphBlendNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_controlValue ] = 0.f;
	instance[ i_prevControlValue ] = 0.f;
}

void CBehaviorGraphBlendNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue );
	INST_PROP( i_prevControlValue );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlendNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( A ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( B ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

void CBehaviorGraphBlendNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedFirstInputNode = CacheBlock( TXT("A") );
	m_cachedSecondInputNode = CacheBlock( TXT("B") );
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphBlendNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Blend );

	// update variable 
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
	}

	// copy variable value (so it's constant across update and sample)
	UpdateControlValue( instance );

	// process activations
	ProcessActivations( instance );

	// synchronize children playback
	Synchronize( instance, timeDelta );

	// update appropriate child nodes
	Float alpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( !IsSecondInputActive( alpha ) )
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->Update( context, instance, timeDelta );
		}
	}
	else if ( !IsFirstInputActive( alpha ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Update( context, instance, timeDelta );
		}
	}
	else
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->Update( context, instance, timeDelta );
		}

		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphBlendNode::UpdateControlValue( CBehaviorGraphInstance& instance ) const
{
	instance[ i_prevControlValue ]	= instance[ i_controlValue ];
	instance[ i_controlValue ]		= m_cachedControlVariableNode ? m_cachedControlVariableNode->GetValue( instance ) : 0.0f;
}

void CBehaviorGraphBlendNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( Blend );

	ANIM_NODE_PRE_SAMPLE

	Float alpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( !IsSecondInputActive( alpha ) )
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->Sample( context, instance, output );
		}
	}
	else if ( !IsFirstInputActive( alpha ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Sample( context, instance, output );
		}
	}
	else
	{
		CCacheBehaviorGraphOutput cachePose1( context );
		CCacheBehaviorGraphOutput cachePose2( context );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			if ( m_cachedFirstInputNode ) 
			{
				m_cachedFirstInputNode->Sample( context, instance, *temp1 );
			}

			if ( m_cachedSecondInputNode )
			{
				m_cachedSecondInputNode->Sample( context, instance, *temp2 );
			}

#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				// Interpolate poses
				output.SetInterpolate( *temp1, *temp2, alpha );
			}
			else
			{
				// Interpolate poses
				output.SetInterpolateME( *temp1, *temp2, alpha );
			}
#else
			output.SetInterpolate( *temp1, *temp2, alpha );
#endif

			// Merge events and used anims
			if ( m_takeEventsFromMostImportantInput )
			{
				SBehaviorGraphOutput* outputToMerge = ( alpha < 0.5f ) ? temp1 : temp2;
				output.MergeEvents( *outputToMerge, 1.0f );
			}
			else
			{
				output.MergeEventsAndUsedAnims( *temp1, *temp2, 1.0f - alpha, alpha );
			}
		}
	}

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphBlendNode::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const
{	
	Float alpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( !IsSecondInputActive( alpha ) )
	{
		// simply pass synchronization request down
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->GetSyncData( instance ).Reset();
		}
	}
	else if ( !IsFirstInputActive( alpha ) )
	{
		// simply pass synchronization request down
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->GetSyncData( instance ).Reset();
		}
	}
	else
	{
		if ( m_synchronize && m_syncMethod )
		{
			m_syncMethod->Synchronize( instance, m_cachedFirstInputNode, m_cachedSecondInputNode, alpha, timeDelta );	
		}
		else
		{
			if ( m_cachedFirstInputNode )
			{
				m_cachedFirstInputNode->GetSyncData( instance ).Reset();
			}

			if ( m_cachedSecondInputNode )
			{
				m_cachedSecondInputNode->GetSyncData( instance ).Reset();
			}
		}
	}
}

void CBehaviorGraphBlendNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	Float alpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( !IsSecondInputActive( alpha ) )
	{
		if ( m_cachedFirstInputNode )
		{
			m_cachedFirstInputNode->GetSyncInfo( instance, info );
		}
	}
	else if ( !IsFirstInputActive( alpha ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->GetSyncInfo( instance, info );
		}
	}
	else
	{
		CSyncInfo firstSyncInfo;
		CSyncInfo secondSyncInfo;

		if ( m_cachedFirstInputNode )
		{
			m_cachedFirstInputNode->GetSyncInfo( instance, firstSyncInfo );
		}

		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->GetSyncInfo( instance, secondSyncInfo );
		}

		info.SetInterpolate( firstSyncInfo, secondSyncInfo, alpha );
	}
}

void CBehaviorGraphBlendNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_synchronize && m_syncMethod )
	{		
		if ( m_cachedFirstInputNode )
		{
			m_syncMethod->SynchronizeTo( instance, m_cachedFirstInputNode, info );
		}
		
		if ( m_cachedSecondInputNode )
		{
			m_syncMethod->SynchronizeTo( instance, m_cachedSecondInputNode, info );
		}
	}
	else
	{
		if ( m_cachedFirstInputNode )
		{
			m_cachedFirstInputNode->SynchronizeTo( instance, info );
		}

		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->SynchronizeTo( instance, info );
		}
	}
}

void CBehaviorGraphBlendNode::ProcessActivations( CBehaviorGraphInstance& instance ) const
{
	Float prevAlpha = GetAlphaValue( instance[ i_prevControlValue ] );
	Float currAlpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( m_cachedFirstInputNode )
	{
		if ( !IsFirstInputActive( prevAlpha ) && IsFirstInputActive( currAlpha ) )
		{				
			m_cachedFirstInputNode->Activate( instance );

			if ( m_cachedSecondInputNode && m_synchronize && m_syncMethod )
			{
				CSyncInfo syncInfo;
				m_cachedSecondInputNode->GetSyncInfo( instance, syncInfo );
				m_syncMethod->SynchronizeTo( instance, m_cachedFirstInputNode, syncInfo );
			}
		}

		if ( IsFirstInputActive( prevAlpha ) && !IsFirstInputActive( currAlpha ) )
		{				
			m_cachedFirstInputNode->Deactivate( instance );
		}
	}	

	if ( m_cachedSecondInputNode )
	{
		if ( !IsSecondInputActive( prevAlpha ) && IsSecondInputActive( currAlpha ) )
		{	
			m_cachedSecondInputNode->Activate( instance );

			if ( m_cachedFirstInputNode && m_synchronize && m_syncMethod )
			{
				CSyncInfo syncInfo;
				m_cachedFirstInputNode->GetSyncInfo( instance, syncInfo );
				m_syncMethod->SynchronizeTo( instance, m_cachedSecondInputNode, syncInfo );
			}
		}

		if ( IsSecondInputActive( prevAlpha ) && !IsSecondInputActive( currAlpha ) )
		{	
			m_cachedSecondInputNode->Deactivate( instance );
		}
	}
}

Bool CBehaviorGraphBlendNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedFirstInputNode && m_cachedFirstInputNode->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	if ( m_cachedSecondInputNode && m_cachedSecondInputNode->ProcessEvent( instance, event ) )
	{
		retVal = true;
	}
	
	return retVal;
}

void CBehaviorGraphBlendNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	UpdateControlValue( instance );

	// activate inputs
	Float alpha = GetAlphaValue( instance[ i_controlValue ] );
	if ( IsFirstInputActive( alpha ) )
	{
		if ( m_cachedFirstInputNode )
		{
			m_cachedFirstInputNode->Activate( instance );
		}
	}

	if ( IsSecondInputActive( alpha ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Activate( instance );
		}
	}
}

void CBehaviorGraphBlendNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Deactivate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

Float CBehaviorGraphBlendNode::GetAlphaValue( Float varValue ) const
{
	Float diff = m_secondInputValue - m_firstInputValue;
	if ( diff == 0.0f )
		diff = 0.01f;

	Float retVal = ( varValue - m_firstInputValue ) / diff;

	return Clamp( retVal, 0.0f, 1.0f );
}

Bool CBehaviorGraphBlendNode::IsFirstInputActive( Float var ) const
{
	return var < (1.0f - ACTIVATION_THRESHOLD);
}

Bool CBehaviorGraphBlendNode::IsSecondInputActive( Float var ) const
{
	return var > ACTIVATION_THRESHOLD;
}

void CBehaviorGraphBlendNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	Float controlAlpha = GetAlphaValue( instance[ i_controlValue ] );
	if ( IsFirstInputActive( controlAlpha ) )
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->ProcessActivationAlpha( instance, ( 1.0f - controlAlpha ) * alpha );
		}
	}

	if ( IsSecondInputActive( controlAlpha ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->ProcessActivationAlpha( instance, controlAlpha * alpha );
		}
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, controlAlpha * alpha );
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
