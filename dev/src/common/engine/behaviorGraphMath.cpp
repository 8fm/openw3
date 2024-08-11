/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphMath.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "baseEngine.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMathNode );
IMPLEMENT_RTTI_ENUM( EBehaviorMathOp );

CBehaviorGraphMathNode::CBehaviorGraphMathNode()	
	: m_operation( BMO_Add )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMathNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Arg1 ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Arg2 ) ) );
}

#endif

void CBehaviorGraphMathNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache inputs
	m_cachedFirstInputNode = CacheValueBlock( TXT("Arg1") );
	m_cachedSecondInputNode = CacheValueBlock( TXT("Arg2") );
}

void CBehaviorGraphMathNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
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

void CBehaviorGraphMathNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Activate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Activate( instance );
	}
}

void CBehaviorGraphMathNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Deactivate( instance );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMathNode::GetCaption() const
{ 
	switch( m_operation )
	{
	case BMO_Add:
		return TXT("Add");
	case BMO_Subtract:
		return TXT("Sub");
	case BMO_Multiply:
		return TXT("Mul");
	case BMO_Divide:
		return TXT("Div");
	case BMO_SafeDivide:
		return TXT("Safe Div");
	case BMO_ATan:
		return TXT("ATan");
	case BMO_AngleDiff:
		return TXT("AngleDiff");
	case BMO_Length:
		return TXT("Length");
	case BMO_Abs:
		return TXT("Absolute");
	}

	return TXT("Math op"); 	
}

#endif

Float CBehaviorGraphMathNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	BEH_PROFILER_LEVEL_3( MathNode );

	Float arg1 = 0.0f;
	Float arg2 = 0.0f;

	if ( m_cachedFirstInputNode )
	{
		arg1 = m_cachedFirstInputNode->GetValue( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		arg2 = m_cachedSecondInputNode->GetValue( instance );
	}

	switch( m_operation )
	{
	case BMO_Add:
		return arg1 + arg2;
	case BMO_Subtract:
		return arg1 - arg2;
	case BMO_Multiply:
		return arg1 * arg2;
	case BMO_Divide:
		{
			if ( MAbs( arg2 ) < NumericLimits< Float >::Epsilon() )
			{
				arg2 = MSign( arg2 ) * NumericLimits< Float >::Epsilon();
			}
			return arg1 / arg2;
		}
	case BMO_SafeDivide:
		{
			if ( MAbs( arg2 ) < NumericLimits< Float >::Epsilon() )
			{
				return 0.f;
			}
			return arg1 / arg2;
		}
	case BMO_ATan:
		return atan2f( arg1, arg2 ) / M_PI;
	case BMO_AngleDiff:
		{
			Float retVal = arg1 - arg2;
			while( retVal < -1.0f ) retVal += 2.0f;
			while( retVal > 1.0f ) retVal -= 2.0f;
			return retVal;
		}
	case BMO_Length:
		return sqrtf( arg1 * arg1 + arg2 * arg2 );
	case BMO_Abs:
		return MAbs( arg1 );
	}
		
	return 0.0f;
}

void CBehaviorGraphMathNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphFloatValueNode );

CBehaviorGraphFloatValueNode::CBehaviorGraphFloatValueNode()
	: m_value( 0.0f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphFloatValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
}

String CBehaviorGraphFloatValueNode::GetCaption() const
{
	return String::Printf( TXT("Float value [ %.3f ]"), m_value );
}

#endif

Float CBehaviorGraphFloatValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return m_value;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphEditorValueNode );

CBehaviorGraphEditorValueNode::CBehaviorGraphEditorValueNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphEditorValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
}
#endif

void CBehaviorGraphEditorValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
}

void CBehaviorGraphEditorValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_value ] = 0.f;
}

void CBehaviorGraphEditorValueNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_value );
}

void CBehaviorGraphEditorValueNode::OnOpenInEditor( CBehaviorGraphInstance& instance ) const
{ 
	instance[ i_value ] = 1.f; 
}

void CBehaviorGraphEditorValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{ 
	instance[ i_value ] = 0.f; 
}

Float CBehaviorGraphEditorValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_value ];
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRandomValueNode );

CBehaviorGraphRandomValueNode::CBehaviorGraphRandomValueNode()
	: m_value( 0.f )
	, m_rand( true )
	, m_cooldown( 1.f )
	, m_min( 0.f )
	, m_max( 1.f )
{
}

void CBehaviorGraphRandomValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_randValue;
	compiler << i_timer;
}

void CBehaviorGraphRandomValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_randValue ] = m_randDefaultValue ? GEngine->GetRandomNumberGenerator().Get< Float >( m_min , m_max ) : m_value;
	instance[ i_timer ] = 0.f;
}

void CBehaviorGraphRandomValueNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_randValue );
	INST_PROP( i_timer );
}

void CBehaviorGraphRandomValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_randValue ] = m_randDefaultValue ? GEngine->GetRandomNumberGenerator().Get< Float >( m_min , m_max ) : m_value;
	instance[ i_timer ] = 0.f;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRandomValueNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("min") || property->GetName() == TXT("max") )
	{
		if ( m_max <= m_min )
		{
			m_max = m_min + 1.f;
		}
	}
}

#endif

void CBehaviorGraphRandomValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	OnReset( instance );
}


#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRandomValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
}

#endif

void CBehaviorGraphRandomValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( RandomValue );

	if ( m_rand )
	{
		// Inc timer
		instance[ i_timer ] += timeDelta;
		
		if ( instance[ i_timer ] >= m_cooldown )
		{
			// Rand new value
			instance[ i_randValue ] = GEngine->GetRandomNumberGenerator().Get< Float >( m_min , m_max );
			instance[ i_timer ] = 0.f;
		}
	}
}

String CBehaviorGraphRandomValueNode::GetCaption() const
{
	return TXT("Rand value");
}

Float CBehaviorGraphRandomValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_randValue ];
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphEventWatchdogNode );

CBehaviorGraphEventWatchdogNode::CBehaviorGraphEventWatchdogNode()
	: m_trueValue( 1.f )
	, m_falseValue( 0.f )
	, m_maxTime( 0.f )
	, m_timeOut( 5.f )
{

}

void CBehaviorGraphEventWatchdogNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
	compiler << i_event;
	compiler << i_timer;
	compiler << i_eventOccured;
	compiler << i_timeoutTimer;
}

void CBehaviorGraphEventWatchdogNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_event ] = instance.GetEventId( m_eventName );
	instance[ i_value ] = false;
	instance[ i_timer ] = 0.f;
	instance[ i_timeoutTimer ] = 0.f;
	instance[ i_eventOccured ] = false;
}

void CBehaviorGraphEventWatchdogNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_value );
	INST_PROP( i_timer );
	INST_PROP( i_event );
	INST_PROP( i_eventOccured );
}

void CBehaviorGraphEventWatchdogNode::OnReset( CBehaviorGraphInstance& instance ) const
{ 
	instance[ i_value ] = false;
	instance[ i_timer ] = 0.f;
	instance[ i_timeoutTimer ] = 0.f;
}

Float CBehaviorGraphEventWatchdogNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_value ] ? m_trueValue : m_falseValue;
}

Bool CBehaviorGraphEventWatchdogNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( event.GetEventID() == instance[ i_event ] )
	{
		instance[ i_eventOccured ] = true;

		if ( m_cachedInputNode )
		{
			m_cachedInputNode->ProcessEvent( instance, event );
		}

		return true;
	}
	else if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphEventWatchdogNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	BEH_NODE_UPDATE( EventWatchdog );

	Bool& eventOccured = instance[ i_eventOccured ];
	Bool& value = instance[ i_value ];
	Float& timer = instance[ i_timer ];
	Float& timeout = instance[ i_timeoutTimer ];

	if ( eventOccured )
	{
		value = true;
		eventOccured = false;
		timer = 0.f;

		if ( m_timeOut > 0.f )
		{
			if ( timeout < m_timeOut )
			{
				timeout += timeDelta;
			}
			else
			{
				value = false;
			}
		}
	}
	else if ( value && m_maxTime > 0.f )
	{
		timer += timeDelta;

		if ( timer > m_maxTime )
		{
			timer = m_maxTime;
			value = false;
		}

		timeout = 0.f;
	}
	else
	{
		value = false;
		timeout = 0.f;
	}
}

CBehaviorGraph* CBehaviorGraphEventWatchdogNode::GetParentGraph()
{
	CBehaviorGraph* graph = GetGraph();
	return graph;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphEventWatchdogNode::GetCaption() const 
{ 
	return m_eventName.Empty() ? TXT("Watchdog") : String::Printf( TXT("Watchdog - %s"), m_eventName.AsString().AsChar() );
}

void CBehaviorGraphEventWatchdogNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );	
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Value ) ) );		
}

#endif

void CBehaviorGraphEventWatchdogNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphEventWatchdogNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphEventWatchdogNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphEventWatchdogNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphEventWatchdogNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphEventWatchdogNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphEventWatchdogNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Input") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphOneMinusNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphValueClampNode );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSelectionValueNode );

CBehaviorGraphSelectionValueNode::CBehaviorGraphSelectionValueNode()
	: m_threshold( 0.5f )
{
	
}

void CBehaviorGraphSelectionValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedSelNode )
	{
		m_cachedSelNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedOneNode )
	{
		m_cachedOneNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedTwoNode )
	{
		m_cachedTwoNode->Update( context, instance, timeDelta );
	}
}

Float CBehaviorGraphSelectionValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedSelNode )
	{
		Float sel = m_cachedSelNode->GetValue( instance );

		if ( sel > m_threshold && m_cachedTwoNode )
		{
			return m_cachedTwoNode->GetValue( instance );
		}
		else if ( sel <= m_threshold && m_cachedOneNode )
		{
			return m_cachedOneNode->GetValue( instance );
		}
	}
	
	return 0.f;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphSelectionValueNode::GetCaption() const 
{ 
	return TXT("Selection");
}

void CBehaviorGraphSelectionValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Selection ) ) );	
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( One ) ) );	
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Two ) ) );	
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );		
}

#endif

void CBehaviorGraphSelectionValueNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedSelNode )
	{
		m_cachedSelNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedOneNode )
	{
		m_cachedOneNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTwoNode )
	{
		m_cachedTwoNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphSelectionValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedSelNode )
	{
		m_cachedSelNode->Activate( instance );
	}

	if ( m_cachedOneNode )
	{
		m_cachedOneNode->Activate( instance );
	}

	if ( m_cachedTwoNode )
	{
		m_cachedTwoNode->Activate( instance );
	}
}

void CBehaviorGraphSelectionValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedSelNode )
	{
		m_cachedSelNode->Deactivate( instance );
	}

	if ( m_cachedOneNode )
	{
		m_cachedOneNode->Deactivate( instance );
	}

	if ( m_cachedTwoNode )
	{
		m_cachedTwoNode->Deactivate( instance );
	}
}

void CBehaviorGraphSelectionValueNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedSelNode = CacheValueBlock( TXT("Selection") );
	m_cachedOneNode = CacheValueBlock( TXT("One") );
	m_cachedTwoNode = CacheValueBlock( TXT("Two") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphValueInterpolationNode );

CBehaviorGraphValueInterpolationNode::CBehaviorGraphValueInterpolationNode()
	: m_x1( 0.f )
	, m_x2( 1.f )
	, m_y1( 0.f )
	, m_y2( 1.f )
{

}

Float CBehaviorGraphValueInterpolationNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		return m_y1 > m_y2 ?
			Clamp( m_y1 + ( m_cachedInputNode->GetValue( instance ) - m_x1 ) * ( m_y2 - m_y1 ) / ( m_x2 - m_x1 ), m_y2, m_y1 ) :
			Clamp( m_y1 + ( m_cachedInputNode->GetValue( instance ) - m_x1 ) * ( m_y2 - m_y1 ) / ( m_x2 - m_x1 ), m_y1, m_y2 );
	}

	return 0.f;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphValueInterpolationNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( m_x1 == m_x2 )
	{
		m_x2 = m_x1 + 1.f;
	}
	else if ( m_y1 == m_y2 )
	{
		m_y2 = m_y1 + 1.f;
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLatchValueNode );
IMPLEMENT_RTTI_ENUM( EBehaviorValueLatchType );

CBehaviorGraphLatchValueNode::CBehaviorGraphLatchValueNode()
	: m_type( BVLT_Activation )
{

}

void CBehaviorGraphLatchValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_type == BVLT_Activation ||
		 m_type == BVLT_Max ||
		 m_type == BVLT_Min )
	{
		Float& latchedValue = instance[ i_value ];
		const Float inputValue = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.f;
		latchedValue = inputValue;
	}
}

void CBehaviorGraphLatchValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( LatchValue );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_type == BVLT_Max )
	{
		Float& latchedValue = instance[ i_value ];
		latchedValue = Max( latchedValue, m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.f );
	}
	else if ( m_type == BVLT_Min )
	{
		Float& latchedValue = instance[ i_value ];
		latchedValue = Min( latchedValue, m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.f );
	}
}

void CBehaviorGraphLatchValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );
	
	instance[ i_value ] = 0.f;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String CBehaviorGraphLatchValueNode::GetCaption() const
{
	if ( m_type == BVLT_Activation )
	{
		return TXT("Latch");
	}
	if ( m_type == BVLT_Max )
	{
		return TXT("Latch max");
	}
	if ( m_type == BVLT_Min )
	{
		return TXT("Latch min");
	}
	return TXT("Latch");
}
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLatchVectorValueNode );

CBehaviorGraphLatchVectorValueNode::CBehaviorGraphLatchVectorValueNode()
{

}

void CBehaviorGraphLatchVectorValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_value ] = m_cachedInputNode ? m_cachedInputNode->GetVectorValue( instance ) : Vector::ZEROS;
}

void CBehaviorGraphLatchVectorValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = Vector::ZEROS;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLatchForSomeTimeValueNode );

CBehaviorGraphLatchForSomeTimeValueNode::CBehaviorGraphLatchForSomeTimeValueNode()
	: m_minTime( 1.0f )
	, m_maxTime( 1.0f )
{

}

void CBehaviorGraphLatchForSomeTimeValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeLeft;
}

void CBehaviorGraphLatchForSomeTimeValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeLeft ] = 0.0f;
}

void CBehaviorGraphLatchForSomeTimeValueNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeLeft );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphLatchForSomeTimeValueNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	//...
}

#endif

Float CBehaviorGraphLatchForSomeTimeValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_value ];
}

void CBehaviorGraphLatchForSomeTimeValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( LatchForSomeTimeValue );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float& timeLeft = instance[ i_timeLeft ];
	timeLeft -= timeDelta;
	if ( timeLeft <= 0.0f )
	{
		LatchValueAndResetTime( instance );
	}
}

void CBehaviorGraphLatchForSomeTimeValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	LatchValueAndResetTime( instance );
}

void CBehaviorGraphLatchForSomeTimeValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = 0.f;
}

void CBehaviorGraphLatchForSomeTimeValueNode::LatchValueAndResetTime( CBehaviorGraphInstance& instance ) const
{
	instance[ i_value ]		= m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.f;
	instance[ i_timeLeft ]	= GEngine->GetRandomNumberGenerator().Get< Float >( m_minTime , m_maxTime );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphTimerValueNode );
IMPLEMENT_RTTI_ENUM( EBehaviorValueTimerType );

RED_DEFINE_STATIC_NAME( CondTimer );
RED_DEFINE_STATIC_NAME( CondReset );

CBehaviorGraphTimerValueNode::CBehaviorGraphTimerValueNode()
	: m_type( BVTT_Activation )
	, m_maxValue( 1.f )
	, m_timeScale( 1.f )
	, m_threshold( 0.5f )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphTimerValueNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	//...
}

#endif

void CBehaviorGraphTimerValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( TimerValue );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float inputValue = 0.f;
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Update( context, instance, timeDelta );
		inputValue = m_cachedFirstInputNode->GetValue( instance );
	}

	Float& val = instance[ i_value ];
	Bool running = true;

	switch ( m_type )
	{
	case BVTT_Activation:
		{
			break;
		}
	case BVTT_ConditionTimer:
		{
			if ( inputValue < m_threshold )
			{
				running = false;
				val = 0.f;
			}
			break;
		}
	case BVTT_ConditionReset:
		{
			if ( inputValue > m_threshold )
			{
				running = false;
				val = 0.f;
			}
			break;
		}
	default:
		ASSERT( 0 );
	}

	if ( running )
	{
		val = Min( val + m_timeScale * timeDelta, m_maxValue );
	}
}

Float CBehaviorGraphTimerValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_value ];
}

void CBehaviorGraphTimerValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Activate( instance );
	}

	instance[ i_value ] = 0.f;
	/*switch ( m_type )
	{
	case BVTT_Condition:
		{
			// TODO
			instance[ i_value ] = 0.f;
			break;
		}
	default:
		instance[ i_value ] = 0.f;
	}*/
}

void CBehaviorGraphTimerValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphTimerValueNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphTimerValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = 0.f;
}

void CBehaviorGraphTimerValueNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedFirstInputNode = NULL;

	switch ( m_type )
	{
	case BVTT_ConditionTimer:
		{
			m_cachedFirstInputNode = CacheValueBlock( TXT("CondTimer") );
			break;
		}
	case BVTT_ConditionReset:
		{
			m_cachedFirstInputNode = CacheValueBlock( TXT("CondReset") );
			break;
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphTimerValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );

	switch ( m_type )
	{
	case BVTT_ConditionTimer:
		{
			CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( CondTimer ) ) );
			break;
		}
	case BVTT_ConditionReset:
		{
			CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( CondReset ) ) );
			break;
		}
	}
}

#endif

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphWrapNode );

CBehaviorGraphWrapNode::CBehaviorGraphWrapNode()
	: m_minValue( 0.f )
	, m_maxValue( 1.f )
	, m_cachedFirstInputNode( NULL )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphWrapNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Input ) ) );
}

#endif

void CBehaviorGraphWrapNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache inputs
	m_cachedFirstInputNode = CacheValueBlock( TXT("Input") );
}

void CBehaviorGraphWrapNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphWrapNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Activate( instance );
	}
}

void CBehaviorGraphWrapNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}
}

Float CBehaviorGraphWrapNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	Float value = 0.0f;

	if ( m_cachedFirstInputNode )
	{
		value = m_cachedFirstInputNode->GetValue( instance );
	}

	if( value < m_minValue || value > m_maxValue )
	{
		Float range = m_maxValue - m_minValue;
		Float value01 = ( value - m_minValue ) / range;
		value01 = value01 - MFloor( value01 );
		value = value01 * range + m_minValue;
	}
	
	return value;	
}

void CBehaviorGraphWrapNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EBehaviorValueModifierType );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphValueModifierNode );

CBehaviorGraphValueModifierNode::CBehaviorGraphValueModifierNode()
	: m_type( BVM_OneMinus )
{

}

Float CBehaviorGraphValueModifierNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	const Float val = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.f;

	switch ( m_type )
	{
	case BVM_OneMinus:
		return 1.f - val;

	case BVM_AdditiveMap:
		return val <= 0.f ? Min( -val, 1.f ) : Min( val + 2.f, 3.f );

	case BVM_Negative:
		return -val;

	case BVM_11To01:
		return 0.f;

	case BVM_11To02:
		return 0.f;

	case BVM_01To11:
		return 0.f;

	case BVM_01To02:
		return val * 2.f;

	case BVM_11To180180:
		return val * 180.f;

	case BVM_180180To11:
		return val / 180.f;

	case BVM_RadToDeg:
		return RAD2DEG( val );

	case BVM_DegToRad:
		return DEG2RAD( val );

	case BVM_RadTo11:
		return val / M_PI;

	case BVM_360To180180:
		return EulerAngles::NormalizeAngle180( val );

	default:
		ASSERT( 0 );
		return 0.f;
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphValueModifierNode::GetCaption() const 
{ 
	switch ( m_type )
	{
	case BVM_OneMinus:
		return TXT("One minus"); 

	case BVM_AdditiveMap:
		return TXT("Additive Map");

	case BVM_Negative:
		return TXT("Negative"); 

	case BVM_11To01:
		return TXT("[-1 1] To [0 1]"); 

	case BVM_11To02:
		return TXT("[-1 1] To [0 2]"); 

	case BVM_01To11:
		return TXT("[0 1] To [-1 1]"); 

	case BVM_01To02:
		return TXT("[0 1] To [0 2]"); 

	case BVM_11To180180:
		return TXT("[-1 1] To [-180 180]"); 

	case BVM_180180To11:
		return TXT("[-180 180] To [-1 1]"); 

	case BVM_RadToDeg:
		return TXT("Rad To Deg"); 

	case BVM_DegToRad:
		return TXT("Deg To Rad"); 

	case BVM_RadTo11:
		return TXT("Rad To [-1 1]"); 

	case BVM_360To180180:
		return TXT("[0 360] To [-180 180]");

	default:
		ASSERT( 0 );
		return TXT("Value modifier"); 
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EBehaviorWaveValueType );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphWaveValueNode );

CBehaviorGraphWaveValueNode::CBehaviorGraphWaveValueNode()
	: m_type( BWVT_Sin )
	, m_freq( 1.f )
	, m_amp( 1.f )
{

}

void CBehaviorGraphWaveValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_var;
}

void CBehaviorGraphWaveValueNode::ResetAllVariables( CBehaviorGraphInstance& instance ) const
{
	instance[ i_var ] = 0.f;
}

void CBehaviorGraphWaveValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( WaveValue );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_freq <= 0.f || m_amp <= 0.f )
	{
		return;
	}

	Float& value = instance[ i_value ];

	switch ( m_type )
	{
	case BWVT_Sin:
		{
			Float& var = instance[ i_var ];
			var += timeDelta;
			value = m_amp * MSin( m_freq * var );
			return;
		}

	case BWVT_Step01:
		{
			const Float factor = 1.f / m_freq;
			Float& var = instance[ i_var ];
			var += timeDelta;
			if ( var > 2.f * factor )
			{
				var = 0.f;
			}
			value = var > factor ? m_amp : 0.f;
			return;
		}

	case BWVT_Step11:
		{
			const Float factor = 1.f / m_freq;
			Float& var = instance[ i_var ];
			var += timeDelta;
			if ( var > 2.f * factor )
			{
				var = 0.f;
			}
			value = var > factor ? m_amp : -m_amp;
			return;
		}

	case BWVT_Triangle01:
		{
			const Float factor = 1.f / m_freq;
			Float& var = instance[ i_var ];
			var += timeDelta;
			if ( var > 2.f * factor )
			{
				var = 0.f;
			}
			value = var > factor ? m_amp * ( 2.f * factor - var ) : m_amp * var;
			return;
		}

	default:
		ASSERT( 0 );
	}
}

Float CBehaviorGraphWaveValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	const Float val = instance[ i_value ];
	return val; // Place for break point
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphWaveValueNode::GetCaption() const 
{ 
	switch ( m_type )
	{
	case BWVT_Sin:
		return TXT("Sin wave"); 

	case BWVT_Step01:
		return TXT("Step 01 wave"); 

	case BWVT_Step11:
		return TXT("Step -11 wave"); 

	case BWVT_Triangle01:
		return TXT("Triangle 01 wave");

	default:
		ASSERT( 0 );
		return TXT("Wave"); 
	}
}

#endif

void CBehaviorGraphWaveValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	ResetAllVariables( instance );
}

void CBehaviorGraphWaveValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	ResetAllVariables( instance );
}

void CBehaviorGraphWaveValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	ResetAllVariables( instance );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode );

CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode::CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode()
	:	m_minOutValue( -1.0f )
	,	m_maxOutValue( 1.0f )
	,	m_leftToRight( true )
{
}

Float CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		const Float inValue = m_cachedInputNode->GetValue( instance );
		// get into range -0.5 (right) to 0.5 (left)
		const Float halfNormalizedLocalValue = EulerAngles::NormalizeAngle180( inValue - instance.GetAnimatedComponent()->GetWorldYaw() ) / 360.0f;
		// get into range 0.0 to 1.0
		const Float rangedLocalValue = halfNormalizedLocalValue + 0.5f;
		// give final range
		return m_minOutValue + ( m_maxOutValue - m_minOutValue ) * rangedLocalValue;
	}
	else
	{
		return 0.0f;	
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode::GetCaption() const
{
	return String::Printf( TXT("Map 360WS to [%.3f,%.3f]MS"), m_minOutValue, m_maxOutValue );
}
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMapRangeNode );

CBehaviorGraphMapRangeNode::CBehaviorGraphMapRangeNode()
	:	m_minInValue( 0.0f )
	,	m_maxInValue( 1.0f )
	,	m_minOutValue( 0.0f )
	,	m_maxOutValue( 1.0f )
{
	CalculateBaseAndBias();
}

Float CBehaviorGraphMapRangeNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		return m_base + m_bias * m_cachedInputNode->GetValue( instance );
	}
	else
	{
		return 0.0f;	
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String CBehaviorGraphMapRangeNode::GetCaption() const
{
	return String::Printf( TXT("Map range without clamp [%.3f,%.3f]->[%.3f,%.3f]"), m_minInValue, m_maxInValue, m_minOutValue, m_maxOutValue );
}

void CBehaviorGraphMapRangeNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	CalculateBaseAndBias();
}
#endif

void CBehaviorGraphMapRangeNode::CalculateBaseAndBias()
{
	/*
		out = ((in - minI) / (maxI - minI)) * (maxO - minO) + minO
		mOI = (maxO - minO) / (maxI - minI)
		out = (in - minI) * mOI + minO
		out = in * mOI - minI * mOI + minO
		out = base + in * bias
		bias = mOI
		base = minO - minI * mOI
	*/
	const Float diffIn = (m_maxInValue - m_minInValue);
	if ( diffIn != 0.0f )
	{
		const Float mOI = (m_maxOutValue - m_minOutValue) / diffIn;
		m_bias = mOI;
		m_base = m_minOutValue - m_minInValue * mOI;
	}
	else
	{
		m_bias = 0.0f;
		m_base = m_minOutValue;
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SBehaviorGraphMapToDiscreteMapper );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMapToDiscreteNode );

Float CBehaviorGraphMapToDiscreteNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		Float inValue = m_cachedInputNode->GetValue( instance );
		for ( auto iRange = m_ranges.Begin(); iRange != m_ranges.End(); ++ iRange )
		{
			if ( inValue >= iRange->m_inRange.X && inValue <= iRange->m_inRange.Y )
			{
				return iRange->m_outValue;
			}
		}
		// nothing found
		return 0.0f;
	}
	else
	{
		return 0.0f;	
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String CBehaviorGraphMapToDiscreteNode::GetCaption() const
{
	return String::Printf( TXT("Map ranges to discrete values") );
}
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSnapToZeroNode );

CBehaviorGraphSnapToZeroNode::CBehaviorGraphSnapToZeroNode()
	: m_epsilon( 0.001f )
{

}

Float CBehaviorGraphSnapToZeroNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		const Float inValue = m_cachedInputNode->GetValue( instance );
		return inValue < m_epsilon ? 0.f : inValue;
	}
	else
	{
		return 0.f;
	}
}
