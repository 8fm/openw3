/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionNode.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphTransitionBlend.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "skeletalAnimationEntry.h"
#include "animSyncInfo.h"
#include "animatedComponent.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SBehaviorGraphTransitionSetInternalVariable );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateTransitionNode );
IMPLEMENT_ENGINE_CLASS( IBehaviorStateTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CVariableValueStateTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CInternalVariableStateTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CEventStateTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CAlwaysTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CIsRagdolledTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CMultiTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CCompositeTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CCompositeSimultaneousTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CAnimEventTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CDelayStateTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CTimeThresholdStateTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CParentInputValueStateTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CAnimationEndCondition );
IMPLEMENT_RTTI_ENUM( ECompareFunc );

CName IBehaviorStateTransitionCondition::GenerateValueSocketName() const
{
	CName socketName = CNAME( Value );
	CBehaviorGraphNode* parentNode = GetParentNode();
	ASSERT( parentNode );

	if ( parentNode )
	{
		CGraphSocket* socket = parentNode->CGraphBlock::FindSocket( socketName );

		int i = 0;
		while ( socket )
		{
			socketName = CName( String::Printf( TXT("Value%d"), i++ ) );
			socket = parentNode->CGraphBlock::FindSocket( socketName );
		}
	}

	return socketName;
}

 
CBehaviorGraphNode* IBehaviorStateTransitionCondition::GetParentNode() const
{
	CObject *parent = GetParent();

	while ( parent && !parent->IsA< CBehaviorGraphNode >() )
	{
		parent = parent->GetParent();
	}
	
	return SafeCast< CBehaviorGraphNode >( parent) ;
}

void IBehaviorStateTransitionCondition::CacheConnections()
{
}

Bool IBehaviorStateTransitionCondition::Contain( const IBehaviorStateTransitionCondition* transition ) const
{
	return ( this == transition );
}

Bool IBehaviorStateTransitionCondition::ContainClass( const CName& className, Bool hierarchical ) const
{
	return ( GetClass()->GetName() == className );
}

void IBehaviorStateTransitionCondition::HierarchicalTest( CBehaviorGraphInstance& instance, String& conditions ) const
{
	conditions += Test( instance ) ? TXT("1") : TXT("0");
}

String IBehaviorStateTransitionCondition::GetCaptionTest( CBehaviorGraphInstance* instance ) const
{
	return instance && ! instance->IsOpenInEditor() ? ( Test(*instance)? TXT("[v] ") : TXT("[ ] ") ) : TXT("");
}

//////////////////////////////////////////////////////////////////////////

CVariableValueStateTransitionCondition::CVariableValueStateTransitionCondition()
	: m_compareValue( 0.0f )
	, m_compareFunc( CF_Equal )
	, m_socketName( TXT("Value") )
	, m_useAbsoluteValue( false )
{	
}

Bool CVariableValueStateTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedVariableNode && m_cachedVariableNode->IsActive( instance ) )
	{	
		// Get current value of tested variable
		Float variableValue = m_cachedVariableNode->GetValue( instance );

		variableValue = m_useAbsoluteValue? Abs( variableValue ) : variableValue;

		// Check using compare function
		switch( m_compareFunc )
		{
			case CF_Equal:
				return variableValue == m_compareValue;

			case CF_NotEqual:
				return variableValue != m_compareValue;

			case CF_Less:
				return variableValue < m_compareValue;

			case CF_LessEqual:
				return variableValue <= m_compareValue;

			case CF_Greater:
				return variableValue > m_compareValue;

			case CF_GreaterEqual:
				return variableValue >= m_compareValue;
		}
	}

	// Condition not met
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CVariableValueStateTransitionCondition::OnRebuildBlockSockets( CBehaviorGraphNode *parent )
{	
	// Generate some default name
	if ( !m_socketName )
	{
		m_socketName = GenerateValueSocketName();
	}

	// Create socket
	parent->CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( m_socketName ) );
}

void CVariableValueStateTransitionCondition::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Name of the socket changed
	if ( property->GetName() == CNAME( socketName ) )
	{
		CBehaviorGraphNode *parentNode = GetParentNode();
		ASSERT( parentNode );
		if ( parentNode )
		{
			parentNode->OnRebuildSockets();
		}
	}
}

#endif

void CVariableValueStateTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedVariableNode )
	{
		m_cachedVariableNode->Activate( instance );
	}
}

void CVariableValueStateTransitionCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedVariableNode )
	{
		m_cachedVariableNode->Deactivate( instance );
	}
}

void CVariableValueStateTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedVariableNode )
	{
		m_cachedVariableNode->Update( context, instance, timeDelta );
	}
}

void CVariableValueStateTransitionCondition::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	if ( m_cachedVariableNode )
	{
		m_cachedVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CVariableValueStateTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	if (transition->IsA<CVariableValueStateTransitionCondition>())
	{
		const CVariableValueStateTransitionCondition* templateCondition = static_cast<const CVariableValueStateTransitionCondition*>(transition);
		m_socketName		= templateCondition->m_socketName;
		m_compareValue		= templateCondition->m_compareValue;
		m_compareFunc		= templateCondition->m_compareFunc;
		m_useAbsoluteValue	= templateCondition->m_useAbsoluteValue;
	}
}

void CVariableValueStateTransitionCondition::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connection to condition variable
	CBehaviorGraphNode* block = GetParentNode();
	ASSERT( block );
	m_cachedVariableNode = block->CacheValueBlock( m_socketName.AsString() );
}

/*void CVariableValueStateTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
	if ( m_cachedVariableNode )
	{
		// Get the current value
	 	Float variableValue = m_cachedVariableNode->GetValue( instance );
 
		// Format the message
 		String message = String::Printf(TXT("  Variable: %f"), variableValue ); 
 		switch ( m_compareFunc )
 		{
 			case CF_Equal:
 				message += TXT(" == ");
 				break;

 			case CF_NotEqual:
 				message += TXT(" != ");
 				break;

 			case CF_Less:
 				message += TXT(" < ");
 				break;

 			case CF_LessEqual:
 				message += TXT(" <= ");
 				break;

 			case CF_Greater:
 				message += TXT(" > ");
 				break;

 			case CF_GreaterEqual:
 				message += TXT(" >= ");
 				break;
 		}
 
		// Tail
 		message += String::Printf( TXT("%.1f => "), m_compareValue ) + ToString( Test( instance ) );
		info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
	}
}*/


void CVariableValueStateTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	// Add value info
	String message = m_socketName.AsString();

	if ( m_useAbsoluteValue )
	{
		message = TXT("|") + message + TXT("|");
	}

	// Compare function
	switch ( m_compareFunc )
	{
	case CF_Equal:
		message += TXT(" == ");
		break;

	case CF_NotEqual:
		message += TXT(" != ");
		break;

	case CF_Less:
		message += TXT(" < ");
		break;

	case CF_LessEqual:
		message += TXT(" <= ");
		break;

	case CF_Greater:
		message += TXT(" > ");
		break;

	case CF_GreaterEqual:
		message += TXT(" >= ");
		break;
	}

	// Tail
	captions.PushBack( ( getCaptionTests? GetCaptionTest( instance ) : TXT("") ) + message + String::Printf(TXT("%.1f"), m_compareValue) );
}

//////////////////////////////////////////////////////////////////////////

CInternalVariableStateTransitionCondition::CInternalVariableStateTransitionCondition()
	: m_compareValue( 0.0f )
	, m_compareFunc( CF_Equal )
{	
}

Bool CInternalVariableStateTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	if ( m_variableName )
	{	
		// Get current value of tested variable
		const Float variableValue = instance.GetInternalFloatValue( m_variableName );

		// Check using compare function
		switch( m_compareFunc )
		{
			case CF_Equal:
				return variableValue == m_compareValue;

			case CF_NotEqual:
				return variableValue != m_compareValue;

			case CF_Less:
				return variableValue < m_compareValue;

			case CF_LessEqual:
				return variableValue <= m_compareValue;

			case CF_Greater:
				return variableValue > m_compareValue;

			case CF_GreaterEqual:
				return variableValue >= m_compareValue;
		}
	}

	// Condition not met
	return false;
}

void CInternalVariableStateTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	if (transition->IsA<CInternalVariableStateTransitionCondition>())
	{
		const CInternalVariableStateTransitionCondition* templateCondition = static_cast<const CInternalVariableStateTransitionCondition*>(transition);
		m_variableName		= templateCondition->m_variableName;
		m_compareValue		= templateCondition->m_compareValue;
		m_compareFunc		= templateCondition->m_compareFunc;
	}
}

void CInternalVariableStateTransitionCondition::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	intVar.PushBack( m_variableName );
}

void CInternalVariableStateTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	// Add value info
	String message = m_variableName.AsString();

	// Compare function
	switch ( m_compareFunc )
	{
	case CF_Equal:
		message += TXT(" == ");
		break;

	case CF_NotEqual:
		message += TXT(" != ");
		break;

	case CF_Less:
		message += TXT(" < ");
		break;

	case CF_LessEqual:
		message += TXT(" <= ");
		break;

	case CF_Greater:
		message += TXT(" > ");
		break;

	case CF_GreaterEqual:
		message += TXT(" >= ");
		break;
	}

	// Tail
	captions.PushBack( ( getCaptionTests? GetCaptionTest( instance ) : TXT("") ) + message + String::Printf(TXT("%.1f"), m_compareValue) );
}

//////////////////////////////////////////////////////////////////////////

CAnimationEndCondition::CAnimationEndCondition()
	: m_useTransitionTimeOffset( true )
	, m_backTimeOffset( 0.f )
{

}

void CAnimationEndCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_fulfilled;
	compiler << i_sourceNode;
	compiler << i_transitionDuration;
}

void CAnimationEndCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	instance[ i_fulfilled ] = false;

	FindSourceNodeAndTransitionDuration( instance[ i_sourceNode ], instance[ i_transitionDuration ] );
}

Bool CAnimationEndCondition::Check( CBehaviorGraphInstance& instance ) const
{
	const CBehaviorGraphNode* node = instance[ i_sourceNode ];
	return instance[ i_fulfilled ];
}

void CAnimationEndCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	const CBehaviorGraphNode* node = instance[ i_sourceNode ];
	if ( node )
	{
		CSyncInfo info;
		node->GetSyncInfo( instance, info );

		const Float transitionTime = instance[ i_transitionDuration ];

		instance[ i_fulfilled ] = timeDelta + info.m_currTime + transitionTime + m_backTimeOffset >= info.m_totalTime;
	}
}

void CAnimationEndCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_fulfilled ] = false;
}

void CAnimationEndCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CAnimationEndCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CAnimationEndCondition::FindSourceNodeAndTransitionDuration( CBehaviorGraphNode*& node, Float& duration ) const
{
	CBehaviorGraphStateTransitionNode* parent = FindParent< CBehaviorGraphStateTransitionNode >();
	ASSERT( parent );

	node = parent ? parent->GetSourceState() : NULL;

	if ( m_useTransitionTimeOffset && parent && parent->IsA< CBehaviorGraphStateTransitionBlendNode >() )
	{
		duration = static_cast< CBehaviorGraphStateTransitionBlendNode* >( parent )->GetTransitionTime();
	}
	else
	{
		duration = 0.f;
	}
}

//////////////////////////////////////////////////////////////////////////

CParentInputValueStateTransitionCondition::CParentInputValueStateTransitionCondition()
	: m_compareValue( 0.0f )
	, m_compareFunc( CF_Equal )
	, m_socketName( TXT("Value") )
	, m_useAbsoluteValue( false )
	, m_epsilon( 0.0001f )
{	
}


Bool CParentInputValueStateTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	if  ( m_cachedParentInput && m_cachedParentInput->IsActive( instance ) )
	{
		// Get the tested value
		Float variableValue = m_cachedParentInput->GetValue( instance );

		variableValue = m_useAbsoluteValue? Abs( variableValue ) : variableValue;

		Float compareValue = m_compareValue;

		// Get the value to test with
		if ( m_cachedCompareParentInput ) 
		{
			// compare parent value has highest priority
			compareValue = m_cachedCompareParentInput->GetValue( instance );
		}
		else if ( m_cachedTestedValue ) 
		{
			// If variable node exists get value form it
			compareValue = m_cachedTestedValue->GetValue( instance );
		}

		// Compare values
		switch( m_compareFunc )
		{
			case CF_Equal:
				return variableValue >= compareValue - m_epsilon && variableValue <= compareValue + m_epsilon;

			case CF_NotEqual:
				return variableValue < compareValue - m_epsilon || variableValue > compareValue + m_epsilon;

			case CF_Less:
				return variableValue < compareValue;

			case CF_LessEqual:
				return variableValue <= compareValue;

			case CF_Greater:
				return variableValue > compareValue;

			case CF_GreaterEqual:
				return variableValue >= compareValue;
		}
	}

	// Condition not met
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CParentInputValueStateTransitionCondition::OnRebuildBlockSockets( CBehaviorGraphNode *parent )
{	
	// Auto generate socket name
	if ( !m_socketName )
	{
		m_socketName = GenerateValueSocketName();
	}

	// Create socket
	parent->CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( m_socketName ) );
}

void CParentInputValueStateTransitionCondition::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Name of the socket has changed
	if ( property->GetName() == CNAME( socketName ) )
	{
		CBehaviorGraphNode *parentNode = GetParentNode();
		ASSERT( parentNode );
		if ( parentNode )
		{
			parentNode->OnRebuildSockets();
		}
	}
}

#endif

void CParentInputValueStateTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedTestedValue )
	{
		m_cachedTestedValue->Activate( instance );
	}

	if ( m_cachedParentInput )
	{
		m_cachedParentInput->Activate( instance );
	}

	if ( m_cachedCompareParentInput )
	{
		m_cachedCompareParentInput->Activate( instance );
	}
}

void CParentInputValueStateTransitionCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedTestedValue )
	{
		m_cachedTestedValue->Deactivate( instance );
	}

	if ( m_cachedParentInput )
	{
		m_cachedParentInput->Deactivate( instance );
	}

	if ( m_cachedCompareParentInput )
	{
		m_cachedCompareParentInput->Deactivate( instance );
	}
}

void CParentInputValueStateTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedTestedValue )
	{
		m_cachedTestedValue->Update( context, instance, timeDelta );
	}

	if ( m_cachedParentInput )
	{
		m_cachedParentInput->Update( context, instance, timeDelta );
	}

	if ( m_cachedCompareParentInput )
	{
		m_cachedCompareParentInput->Update( context, instance, timeDelta );
	}
}

void CParentInputValueStateTransitionCondition::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	if ( m_cachedTestedValue )
	{
		m_cachedTestedValue->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedParentInput )
	{
		m_cachedParentInput->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedCompareParentInput )
	{
		m_cachedCompareParentInput->ProcessActivationAlpha( instance, alpha );
	}
}

void CParentInputValueStateTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	if (transition->IsA<CParentInputValueStateTransitionCondition>())
	{
		const CParentInputValueStateTransitionCondition* templateCondition = static_cast<const CParentInputValueStateTransitionCondition*>(transition);
		m_socketName		= templateCondition->m_socketName;
		m_parentValueName	= templateCondition->m_parentValueName;
		m_compareValue		= templateCondition->m_compareValue;
		m_compareFunc		= templateCondition->m_compareFunc;
		m_useAbsoluteValue	= templateCondition->m_useAbsoluteValue;
	}
}

/*void CParentInputValueStateTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
	// Generate info
	if ( m_cachedParentInput )
	{
		// Add value info
		Float variableValue = m_cachedParentInput->GetValue( instance );
		String message = String::Printf(TXT("  Parent: %f"), variableValue );

		// Compare function
		switch ( m_compareFunc )
		{
			case CF_Equal:
				message += TXT(" == ");
				break;

			case CF_NotEqual:
				message += TXT(" != ");
				break;

			case CF_Less:
				message += TXT(" < ");
				break;

			case CF_LessEqual:
				message += TXT(" <= ");
				break;

			case CF_Greater:
				message += TXT(" > ");
				break;

			case CF_GreaterEqual:
				message += TXT(" >= ");
				break;
		}

		// Tail
		message += String::Printf(TXT("%.1f => "), m_compareValue) + ToString( Test( instance ) );
		info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
	}
}*/

void CParentInputValueStateTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	// Generate info
	if ( m_cachedParentInput )
	{
		// Add value info
		String message = m_parentValueName.AsString();

		if ( m_useAbsoluteValue )
		{
			message = TXT("|") + message + TXT("|");
		}

		// Compare function
		switch ( m_compareFunc )
		{
			case CF_Equal:
				message += TXT(" == ");
				break;

			case CF_NotEqual:
				message += TXT(" != ");
				break;

			case CF_Less:
				message += TXT(" < ");
				break;

			case CF_LessEqual:
				message += TXT(" <= ");
				break;

			case CF_Greater:
				message += TXT(" > ");
				break;

			case CF_GreaterEqual:
				message += TXT(" >= ");
				break;
		}

		String compareValue = String::Printf(TXT("%.1f"), m_compareValue);

		// Get the value to test with
		if ( m_cachedCompareParentInput ) 
		{
			compareValue = TXT("parent:") + m_compareParentInputName.AsString();
		}
		else if ( m_cachedTestedValue ) 
		{
			compareValue = TXT("socket");
		}

		// Tail
		captions.PushBack( ( getCaptionTests? GetCaptionTest( instance ) : TXT("") ) + message + compareValue );
	}
}

void CParentInputValueStateTransitionCondition::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	{
		// Cache the tested value 
		CBehaviorGraphNode* parent = GetParentNode();
		ASSERT( parent );
		m_cachedTestedValue = parent ? parent->CacheValueBlock( m_socketName.AsString() ) : NULL;

		// Cache parent input
		m_cachedParentInput = NULL;
		while ( parent && !m_cachedParentInput )
		{
			m_cachedParentInput = parent->CacheValueBlock( m_parentValueName.AsString() );
			parent = Cast< CBehaviorGraphNode >( parent->GetParent() );
		}
	}

	// Cache compare parent input
	m_cachedCompareParentInput = NULL;
	if ( ! m_compareParentInputName.Empty() )
	{
		CBehaviorGraphNode* parent = GetParentNode();
		ASSERT( parent );

		while ( parent && !m_cachedCompareParentInput )
		{
			m_cachedCompareParentInput = parent->CacheValueBlock( m_compareParentInputName.AsString() );
			parent = Cast< CBehaviorGraphNode >( parent->GetParent() );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CDelayStateTransitionCondition::CDelayStateTransitionCondition()
	: m_delayTime( 0.0f )
	, m_resetTime( true )
{
}

void CDelayStateTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_currTime;
}

void CDelayStateTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	instance[ i_currTime ] = 0.f;
}

Bool CDelayStateTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_currTime ] > m_delayTime ? true : false;
}

void CDelayStateTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	if ( m_resetTime ) instance[ i_currTime ] = 0;
}

void CDelayStateTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_currTime ] = 0;
}

void CDelayStateTransitionCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_currTime ] = 0;
}

void CDelayStateTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	instance[ i_currTime ] += timeDelta;
}

void CDelayStateTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	/*if (transition->IsA<CDelayStateTransitionCondition>())
	{
		const CDelayStateTransitionCondition* templateCondition = static_cast<const CDelayStateTransitionCondition*>(transition);
		m_delayTime		= templateCondition->m_delayTime;
		m_currTime		= templateCondition->m_currTime;
		m_resetTime		= templateCondition->m_resetTime;
	}*/
}

/*void CDelayStateTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
 	String message = String::Printf(TXT("  Time: %.1f < %.1f => "), instance[ i_currTime ], m_delayTime ) + ToString( Test( instance ) );
	info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
}*/

void CDelayStateTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	captions.PushBack( ( getCaptionTests? GetCaptionTest( instance ) : TXT("") ) + String::Printf( TXT("Delay: %.2f"), m_delayTime ) );
}

//////////////////////////////////////////////////////////////////////////

CTimeThresholdStateTransitionCondition::CTimeThresholdStateTransitionCondition()
	: m_minActivationTime( 0.0f )
	, m_maxActivationTime( 0.0f )
	, m_resetTime( true )
{	
}

void CTimeThresholdStateTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_currTime;
}

void CTimeThresholdStateTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	instance[ i_currTime ] = 0.f;
}

Bool CTimeThresholdStateTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	Bool ret = false;

	// Check min time
	if ( m_minActivationTime == 0.0f || instance[ i_currTime ] > m_minActivationTime )
	{
		ret = true;
	}

	// Check max time
	if ( m_maxActivationTime != 0.0f && instance[ i_currTime ] >= m_maxActivationTime )
	{
		ret = false;
	}

	return ret;
}

void CTimeThresholdStateTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	if ( m_resetTime ) instance[ i_currTime ] = 0;
}

void CTimeThresholdStateTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_currTime ] = 0;
}

void CTimeThresholdStateTransitionCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_currTime ] = 0;
}

void CTimeThresholdStateTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	instance[ i_currTime ] += timeDelta;
}

void CTimeThresholdStateTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	/*if (transition->IsA<CTimeThresholdStateTransitionCondition>())
	{
		const CTimeThresholdStateTransitionCondition* templateCondition = static_cast<const CTimeThresholdStateTransitionCondition*>(transition);
		m_maxActivationTime	= templateCondition->m_maxActivationTime;
		m_minActivationTime	= templateCondition->m_minActivationTime;
		m_currTime			= templateCondition->m_currTime;
		m_resetTime			= templateCondition->m_resetTime;
	}*/
}

/*void CTimeThresholdStateTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
 	String message = String::Printf(TXT("  Threshold: %f < %f < %f => "), m_minActivationTime, instance[ i_currTime ], m_maxActivationTime) + ToString( Test( instance ) );
	info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
}*/

void CTimeThresholdStateTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + String::Printf( TXT("Threshold: %.2f, %.2f"), m_minActivationTime, m_maxActivationTime ) );
}

//////////////////////////////////////////////////////////////////////////

CEventStateTransitionCondition::CEventStateTransitionCondition()
{
}

void CEventStateTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_eventId;
	compiler << i_eventOccured;
}

void CEventStateTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	instance[ i_eventId ] = instance.GetEventId( m_eventName );
	instance[ i_eventOccured ] = false;
}

void CEventStateTransitionCondition::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == CNAME( eventName ) )
	{
		CBehaviorGraphNode* block = GetParentNode();
		ASSERT( block );
		CBehaviorGraph *graph = block->GetGraph();

		Uint32 eventId = graph->GetEvents().GetEventId( m_eventName );

		if ( eventId == CBehaviorEventsList::NO_EVENT )
		{
			graph->GetEvents().AddEvent( m_eventName );
		}
	}
}

Bool CEventStateTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_eventOccured ];	
}

void CEventStateTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_eventOccured ] = false;
}

Bool CEventStateTransitionCondition::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( event.GetEventID() != instance[ i_eventId ] )
		return false;

	instance[ i_eventOccured ] = true;

	return true;
}

void CEventStateTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CEventStateTransitionCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CEventStateTransitionCondition::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	instance.OnEventProcessed( m_eventName );
	Reset( instance );
}

void CEventStateTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	/*if (transition->IsA<CEventStateTransitionCondition>())
	{
		const CEventStateTransitionCondition* templateCondition = static_cast<const CEventStateTransitionCondition*>(transition);
		m_eventId		= templateCondition->m_eventId;
		m_eventName		= templateCondition->m_eventName;
		m_eventOccured	= templateCondition->m_eventOccured;
	}*/
}

/*void CEventStateTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
	String message = TXT("  Event: ") + m_eventName.AsString() + ToString( Test( instance ) );
	info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
}*/

void CEventStateTransitionCondition::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	events.PushBack( m_eventName );
}

void CEventStateTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + m_eventName.AsString() );
}

Bool CEventStateTransitionCondition::UseEvent( const CName& event, Bool hierarchical ) const 
{ 
	return event == m_eventName ? true : false; 
}

CBehaviorGraph* CEventStateTransitionCondition::GetParentGraph()
{
	CBehaviorGraphNode *node = FindParent< CBehaviorGraphNode >();
	ASSERT( node );
	CBehaviorGraph* graph = node->GetGraph();
	return graph;
}

void CEventStateTransitionCondition::SetEventName( const CName& eventName )
{
	m_eventName = eventName;
}

//////////////////////////////////////////////////////////////////////////

CAnimEventTransitionCondition::CAnimEventTransitionCondition()
{
}

void CAnimEventTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_eventOccured;
	compiler << i_poseProvided;
}

void CAnimEventTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_eventOccured ] = false;
	instance[ i_poseProvided ] = false;
}

Bool CAnimEventTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	if ( ! instance[ i_poseProvided ] )
	{
		if ( const CAnimatedComponent * ac = instance.GetAnimatedComponent() )
		{
			const SBehaviorUsedAnimations& rua = ac->GetRecentlyUsedAnims();
			const SBehaviorUsedAnimationData* anim = rua.m_anims.GetUsedData();
			for ( Uint32 i = 0; i < rua.m_anims.GetNum(); ++ i, ++ anim )
			{
				if ( anim->m_animation )
				{
					for ( CSkeletalAnimationSetEntry::EventsIterator eventIter( anim->m_animation ); eventIter; ++ eventIter )
					{
						const CExtAnimEvent * event = *eventIter;
						if ( event->GetEventName() == m_eventName )
						{
							if ( anim->m_currTime >= event->GetStartTime() && anim->m_currTime <= event->GetEndTimeWithoutClamp() )
							{
								return true;
							}
						}
					}
				}
			}
		}
	}
	return instance[ i_eventOccured ];
}

void CAnimEventTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_eventOccured ] = false;
	instance[ i_poseProvided ] = false;
}

void CAnimEventTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CAnimEventTransitionCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CAnimEventTransitionCondition::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CAnimEventTransitionCondition::OnPoseSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
	instance[ i_poseProvided ] = true;
	for ( Uint32 i=0; i<pose.m_numEventsFired; ++i )
	{
		if ( pose.m_eventsFired[ i ].GetEventName() == m_eventName )
		{
			instance[ i_eventOccured ] = true;
			return;
		}
	}
}

void CAnimEventTransitionCondition::SetEventName( const CName& eventName )
{
	m_eventName = eventName;
}

void CAnimEventTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	/*if (transition->IsA<CAnimEventTransitionCondition>())
	{
		const CAnimEventTransitionCondition* templateCondition = static_cast<const CAnimEventTransitionCondition*>(transition);
		m_eventName		= templateCondition->m_eventName;
		m_eventOccured	= templateCondition->m_eventOccured;

		m_animatedComponent->GetAnimationEventNotifier( m_eventName ).UnregisterHandler( this );
		m_animatedComponent	= templateCondition->m_animatedComponent;
		m_animatedComponent->GetAnimationEventNotifier( m_eventName ).RegisterHandler( this );
	}*/
}

/*void CAnimEventTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
	String message = TXT("  AnimEvent: ") + m_eventName.AsString() + ToString( Test( instance ) );
	info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
}*/

void CAnimEventTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + String::Printf( TXT("Anim event: %s"), m_eventName.AsString().AsChar() ) );
}

Bool CAnimEventTransitionCondition::UseEvent( const CName& event, Bool hierarchical ) const
{
	return event == m_eventName ? true : false;
}

//////////////////////////////////////////////////////////////////////////

void CMultiTransitionCondition::OnPostLoad()
{
	TBaseClass::OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	GetParentNode()->SetDeprecated( false );
#endif
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMultiTransitionCondition::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );

	// damn hack
	CBehaviorGraphNode* parent = GetParentNode();
	ASSERT( parent );
	parent->OnRebuildSockets();

	/*if ( m_conditions.Size() <= 1 )
	{
		GetParentNode()->SetDeprecated( true, TXT("Composite transition condition has got only ONE condition - use simple condition") );
	}
	else*/
	{
		GetParentNode()->SetDeprecated( false );
	}
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMultiTransitionCondition::OnRebuildBlockSockets( CBehaviorGraphNode *parent )
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnRebuildBlockSockets( parent );
	}
}

#endif

void CMultiTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) 
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnBuildDataLayout( compiler );
	}
}

void CMultiTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const 
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnInitInstance( instance );
	}
}

void CMultiTransitionCondition::OnReleaseInstance( CBehaviorGraphInstance& instance ) const 
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnReleaseInstance( instance );
	}
}

void CMultiTransitionCondition::OnActivated( CBehaviorGraphInstance& instance ) const
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnActivated( instance );
	}
}

void CMultiTransitionCondition::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnDeactivated( instance );
	}
}

void CMultiTransitionCondition::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->ProcessActivationAlpha( instance, alpha );
	}
}

void CMultiTransitionCondition::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnStartBlockDeactivated( instance );
	}	
}

void CMultiTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
		{
			m_conditions[i]->OnStartBlockUpdate( context, instance, timeDelta );
		}
	}
}

void CMultiTransitionCondition::OnPoseSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
		{
			m_conditions[i]->OnPoseSampled( instance, pose );
		}
	}
}

void CMultiTransitionCondition::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Pass to child conditions
	for (Uint32 i=0; i<m_conditions.Size(); i++)
	{
		if ( m_conditions[i] )
		{
			m_conditions[i]->CacheConnections();
		}
	}
}

Bool CMultiTransitionCondition::Contain(const IBehaviorStateTransitionCondition* transition) const
{
	if ( this == transition )
	{
		return true;
	}
	else
	{
		// Pass to child conditions
		for (Uint32 i=0; i<m_conditions.Size(); i++)
		{
			if ( m_conditions[i] )
			{
				if ( m_conditions[i]->Contain( transition ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

Bool CMultiTransitionCondition::ContainClass( const CName& className, Bool hierarchical ) const
{
	if ( GetClass()->GetName() == className )
	{
		return true;
	}
	else
	{
		// Pass to child conditions
		for ( Uint32 i=0; i<m_conditions.Size(); i++ )
		{
			if ( m_conditions[i] )
			{
				if ( m_conditions[i]->ContainClass( className, hierarchical ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

void CMultiTransitionCondition::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	// Pass to child conditions
	for (Uint32 i=0; i<m_conditions.Size(); i++)
	{
		if ( m_conditions[i] )
		{
			m_conditions[i]->GetUsedVariablesAndEvents( var, vecVar, events, intVar, intVecVar );
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMultiTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + TXT("[multi]") );
	Uint32 s = m_conditions.Size();
	for( Uint32 i=0; i<s; i++ )
	{
		IBehaviorStateTransitionCondition* cond = m_conditions[i];
		if( cond )
		{
			cond->GetCaption( captions, getCaptionTests, instance );
		}
	}
}

#endif

Bool CMultiTransitionCondition::UseEvent( const CName& event, Bool hierarchical ) const 
{ 
	if ( hierarchical )
	{
		for ( Uint32 i=0; i<m_conditions.Size(); i++)
		{
			if ( m_conditions[i] && m_conditions[i]->UseEvent( event, hierarchical ) )
			{
				return true;
			}
		} 
	}

	return false;
}

Bool CMultiTransitionCondition::UseVariable( const String& var, Bool hierarchical ) const 
{ 
	if ( hierarchical )
	{
		for ( Uint32 i=0; i<m_conditions.Size(); i++)
		{
			if ( m_conditions[i] && m_conditions[i]->UseVariable( var, hierarchical ) )
			{
				return true;
			}
		} 
	}

	return false; 
}

//////////////////////////////////////////////////////////////////////////

CCompositeTransitionCondition::CCompositeTransitionCondition()
{
}

void CCompositeTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currentCondition;
}

void CCompositeTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_currentCondition ] = 0;
}

Bool CCompositeTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	Uint32& currentCondition = instance[ i_currentCondition ];

	if ( currentCondition >= m_conditions.Size() )
	{
		return true;
	}

	while( currentCondition < m_conditions.Size() )
	{
		if ( m_conditions[ currentCondition ] && !m_conditions[ currentCondition ]->Check( instance ) )
		{
			break;
		}

		++currentCondition;

		if ( currentCondition < m_conditions.Size() )
		{
			if ( m_conditions[ currentCondition ] )
				m_conditions[ currentCondition ]->Reset( instance );
		}
	}

	if ( currentCondition < m_conditions.Size() )
	{
		return false;
	}

	return true;
}

Bool CCompositeTransitionCondition::Test( CBehaviorGraphInstance& instance ) const
{
	const Uint32 currentCondition = instance[ i_currentCondition ];

	return currentCondition >= m_conditions.Size() ||
		( currentCondition == m_conditions.Size()-1 && m_conditions[ currentCondition ]->Test( instance ) );
}

void CCompositeTransitionCondition::HierarchicalTest( CBehaviorGraphInstance& instance, String& conditions ) const
{
	conditions += TXT("[");

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
		{
			m_conditions[i]->HierarchicalTest( instance, conditions );
		}
		else
		{
			conditions += TXT("x");
		}
	}

	conditions += TXT("]");
}

Bool CCompositeTransitionCondition::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	const Uint32 currentCondition = instance[ i_currentCondition ];

	if ( currentCondition >= m_conditions.Size() )
	{
		return false;
	}

	if ( m_conditions[ currentCondition ] && m_conditions[ currentCondition ]->ProcessEvent( instance, event ) )
	{
		//return Test(); // do not return true, if the transition is inactive
		return true;
	}

	return false;
}

void CCompositeTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	const Uint32 currentCondition = instance[ i_currentCondition ];

	//for( Uint32 i=0; i<m_conditions.Size(); ++i )
	//{
	//	if ( m_conditions[i] )
	//		m_conditions[i]->Reset();
	//}

	// Reset only current condition. All conditions will be reset on OnStartBlockActivated and OnStartBlockDeactivated
	if ( currentCondition < m_conditions.Size() )
	{
		if ( m_conditions[ currentCondition ] )
			m_conditions[ currentCondition ]->Reset( instance );
	}
}

void CCompositeTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_currentCondition ] = 0;

	if ( m_conditions.Size() > 0 )
	{
		if ( m_conditions[0] )
			m_conditions[0]->Reset( instance );
	}

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnStartBlockActivated( instance );
	}
}

Bool CCompositeTransitionCondition::CanConvertTo(const IBehaviorStateTransitionCondition* transition) const
{
	//if (transition->IsA<CCompositeSimultaneousTransitionCondition>())
	//{
	//	return true;
	//}

	return false;
}

void CCompositeTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	/*if (transition->IsA<CCompositeSimultaneousTransitionCondition>() || transition->IsA<CCompositeTransitionCondition>())
	{
		m_currentCondition = 0;

		// Delete old conditions
		for (Uint32 i=0; i<m_conditions.Size(); i++)
		{
			if (m_conditions[i])
			{
				m_conditions[i]->Discard();
				m_conditions[i] = NULL;
			}
		}
		m_conditions.Clear();

		// Clone conditions
		const TDynArray<IBehaviorStateTransitionCondition*>& templateConditions = transition->IsA<CCompositeTransitionCondition>() ? 
			static_cast<const CCompositeTransitionCondition*>(transition)->GetConditions() : 
			static_cast<const CCompositeSimultaneousTransitionCondition*>(transition)->GetConditions();

		for (Uint32 i=0; i<templateConditions.Size(); i++)
		{
			IBehaviorStateTransitionCondition* newCondition = NULL;

			if (templateConditions[i])
			{
				newCondition = SafeCast<IBehaviorStateTransitionCondition>(templateConditions[i]->Clone(this));
			}

			m_conditions.PushBack(newCondition);
		}
	}*/
}

void CCompositeTransitionCondition::EnumConversions(TDynArray<IBehaviorStateTransitionCondition*>& conversions) const
{
	conversions.PushBack(CCompositeSimultaneousTransitionCondition::GetStaticClass()->GetDefaultObject<CCompositeSimultaneousTransitionCondition>());
}

/*void CCompositeTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
 	if ( instance[ i_currentCondition ] >= m_conditions.Size() )
 	{
 		return;
 	}
 
 	String message = TXT("  Composite ") + ToString( Test( instance ) ) + TXT(" :");
	info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
 
 	for (Uint32 i=0; i<m_conditions.Size(); i++)
 	{
		if ( m_conditions[i] )
		{
	 		m_conditions[i]->OnGenerateInfo( instance, info );
		}
 	}
}*/

void CCompositeTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + TXT("[composite]") );
	Uint32 s = m_conditions.Size();
	for( Uint32 i=0; i<s; i++ )
	{
		IBehaviorStateTransitionCondition* cond = m_conditions[i];
		if( cond )
		{
			cond->GetCaption( captions, getCaptionTests, instance );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CCompositeSimultaneousTransitionCondition::CCompositeSimultaneousTransitionCondition()	
{
}

Bool CCompositeSimultaneousTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	bool retVal = true;

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( !m_conditions[ i ] )
		{
			continue;
		}

		retVal = retVal && m_conditions[ i ]->Check( instance );
	}

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[ i ] )
		{
			m_conditions[ i ]->Reset( instance );
		}
	}

	return retVal;
}

Bool CCompositeSimultaneousTransitionCondition::Test( CBehaviorGraphInstance& instance ) const
{
	bool retVal = true;

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( !m_conditions[ i ] )
		{
			continue;
		}

		retVal = retVal && m_conditions[ i ]->Test( instance );
	}

	return retVal;
}

void CCompositeSimultaneousTransitionCondition::HierarchicalTest( CBehaviorGraphInstance& instance, String& conditions ) const
{
	conditions += TXT("{");

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
		{
			m_conditions[i]->HierarchicalTest( instance, conditions );
		}
		else
		{
			conditions += TXT("x");
		}
	}

	conditions += TXT("}");
}

Bool CCompositeSimultaneousTransitionCondition::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[ i ] && m_conditions[ i ]->ProcessEvent( instance, event ) )
			retVal = true;
	}

	if ( ! retVal )
		return false;
	
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[ i ] && ! m_conditions[ i ]->Test( instance ) ) // do not return true, if the transition is inactive
			return false;
	}

	return true;
}

void CCompositeSimultaneousTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
		{
			m_conditions[i]->Reset( instance );
		}
	}
}

void CCompositeSimultaneousTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );

	for( Uint32 i=0; i<m_conditions.Size(); ++i )
	{
		if ( m_conditions[i] )
			m_conditions[i]->OnStartBlockActivated( instance );
	}
}

Bool CCompositeSimultaneousTransitionCondition::CanConvertTo(const IBehaviorStateTransitionCondition* transition) const
{
	//if (transition->IsA<CCompositeTransitionCondition>())
	//{
	//	return true;
	//}

	return false;
}

void CCompositeSimultaneousTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	/*if (transition->IsA<CCompositeTransitionCondition>() || transition->IsA<CCompositeSimultaneousTransitionCondition>())
	{
		// Delete old conditions
		for (Uint32 i=0; i<m_conditions.Size(); i++)
		{
			if (m_conditions[i])
			{
				m_conditions[i]->Discard();
				m_conditions[i] = NULL;
			}
		}
		m_conditions.Clear();

		// Clone conditions
		const TDynArray<IBehaviorStateTransitionCondition*>& templateConditions = transition->IsA<CCompositeTransitionCondition>() ? 
			static_cast<const CCompositeTransitionCondition*>(transition)->GetConditions() : 
			static_cast<const CCompositeSimultaneousTransitionCondition*>(transition)->GetConditions();

		for (Uint32 i=0; i<templateConditions.Size(); i++)
		{
			IBehaviorStateTransitionCondition* newCondition = NULL;

			if (templateConditions[i])
			{
				newCondition = SafeCast<IBehaviorStateTransitionCondition>(templateConditions[i]->Clone(this));
			}

			m_conditions.PushBack(newCondition);
		}
	}*/
}

void CCompositeSimultaneousTransitionCondition::EnumConversions(TDynArray<IBehaviorStateTransitionCondition*>& conversions) const
{
	conversions.PushBack(CCompositeTransitionCondition::GetStaticClass()->GetDefaultObject<CCompositeTransitionCondition>());
}

/*void CCompositeSimultaneousTransitionCondition::OnGenerateInfo( CBehaviorGraphInstance& instance, CBehaviorGraphDebugInfo* info ) const
{
 	String message = TXT("  Simultaneous ") + ToString( Test( instance ) ) + TXT(" :");
	info->AddDispText(message, CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
 
 	for (Uint32 i=0; i<m_conditions.Size(); i++)
 	{
		if (!m_conditions[i]) continue;

 		m_conditions[i]->OnGenerateInfo( instance, info );
 	}
}*/

void CCompositeSimultaneousTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + TXT("[simultaneous]") );
	Uint32 s = m_conditions.Size();
	for( Uint32 i=0; i<s; i++ )
	{
		IBehaviorStateTransitionCondition* cond = m_conditions[i];
		if( cond )
		{
			cond->GetCaption( captions, getCaptionTests, instance );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

SBehaviorGraphTransitionSetInternalVariable::SBehaviorGraphTransitionSetInternalVariable()
	: m_value( 0.0f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String SBehaviorGraphTransitionSetInternalVariable::GetCaption() const
{
	return String::Printf( TXT("%s = %.3f"), m_variableName.AsChar(), m_value );
}
#endif

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphStateTransitionNode::CBehaviorGraphStateTransitionNode()
	: m_transitionCondition( NULL )	
	, m_transitionPriority( 0.0f )
	, m_isEnabled( true )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphStateTransitionNode::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( transitionCondition ) )
	{
		// rebuild sockets to show ones created by transitionCondition
		OnRebuildSockets();
	}
}

EGraphBlockDepthGroup CBehaviorGraphStateTransitionNode::GetBlockDepthGroup() const
{
	return GBDG_Foreground;
}

Color CBehaviorGraphStateTransitionNode::GetBorderColor() const
{
	if ( !m_isEnabled )
	{
		return Color::RED;
	}

	return TBaseClass::GetBorderColor();
}

Color CBehaviorGraphStateTransitionNode::GetClientColor() const
{
	if ( !m_isEnabled )
	{
		return Color( 200, 142, 142 );
	}

	return TBaseClass::GetClientColor();
}

void CBehaviorGraphStateTransitionNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphTransitionSocketSpawnInfo( CNAME( Start ), LSD_Input ) );	
	CreateSocket( CBehaviorGraphTransitionSocketSpawnInfo( CNAME( End ), LSD_Output ) );

	if ( m_transitionCondition )
	{
		m_transitionCondition->OnRebuildBlockSockets( this );
	}
}

EGraphBlockShape CBehaviorGraphStateTransitionNode::GetBlockShape() const
{
	return GBS_LargeCircle;
}

#endif

void CBehaviorGraphStateTransitionNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections for condition
	if ( m_transitionCondition )
	{
		m_transitionCondition->CacheConnections();
	}

	// Cache connections
	m_cachedStartStateNode = CacheStateBlock( TXT("Start") );
	m_cachedEndStateNode = CacheStateBlock( TXT("End") );
}

Bool CBehaviorGraphStateTransitionNode::CheckTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	// Check only if we have transition condition and transition is enabled
	if ( m_transitionCondition && m_isEnabled )
	{
		return m_transitionCondition->Check( instance );
	}

	// return false if condition is not provided
	return false;
}

Bool CBehaviorGraphStateTransitionNode::TestTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	// Check only if we have transition condition and transition is enabled
	if ( m_transitionCondition && m_isEnabled )
	{
		return m_transitionCondition->Test( instance );
	}

	// return false if condition is not provided
	return false;
}

void CBehaviorGraphStateTransitionNode::HierarchicalConditionsTest( CBehaviorGraphInstance& instance, String& conditions ) const
{
	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->HierarchicalTest( instance, conditions );
	}
}

void CBehaviorGraphStateTransitionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	if ( m_transitionCondition )
	{
		m_transitionCondition->OnBuildDataLayout( compiler );
	}
}

void CBehaviorGraphStateTransitionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	if ( m_transitionCondition )
	{
		m_transitionCondition->OnInitInstance( instance );
	}
}

void CBehaviorGraphStateTransitionNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	if ( m_transitionCondition )
	{
		m_transitionCondition->OnReleaseInstance( instance );
	}
}

Bool CBehaviorGraphStateTransitionNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	// if transition is inactive process event by its transition condition otherwise
	if ( !IsActive( instance ) && m_transitionCondition && m_isEnabled )
	{
		return m_transitionCondition->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphStateTransitionNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	if ( m_transitionCondition )
	{
		m_transitionCondition->Reset( instance );
	}
}

void CBehaviorGraphStateTransitionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	// set internal variables
	if ( ! m_setInternalVariables.Empty() )
	{
		for ( auto it = m_setInternalVariables.Begin(), end = m_setInternalVariables.End(); it != end; ++it )
		{
			instance.SetInternalFloatValue( it->m_variableName, it->m_value );
		}
	}

	TBaseClass::OnActivated( instance );

	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->OnActivated( instance );
	}
}

void CBehaviorGraphStateTransitionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->OnDeactivated( instance );
	}
}


void CBehaviorGraphStateTransitionNode::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->OnStartBlockActivated( instance );
	}
}

void CBehaviorGraphStateTransitionNode::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->OnStartBlockDeactivated( instance );
	}
}

void CBehaviorGraphStateTransitionNode::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->OnStartBlockUpdate( context, instance, timeDelta );
	}

	/*if ( m_transitionCondition && m_isEnabled )
	{
		info->AddDispText(TXT("---"), CBehaviorGraphDebugInfo::DC_States, &Color::WHITE, false);
		m_transitionCondition->OnGenerateInfo( instance, info );
	}*/
}

void CBehaviorGraphStateTransitionNode::OnPoseSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->OnPoseSampled( instance, pose );
	}
}

void CBehaviorGraphStateTransitionNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );
}

void CBehaviorGraphStateTransitionNode::StartBlockProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	// dont process act alpha of the block itself, only condition
	if ( m_transitionCondition && m_isEnabled )
	{
		m_transitionCondition->ProcessActivationAlpha( instance, alpha );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphStateTransitionNode::GetSetInternalVariableCaptions( TDynArray< String >& captions ) const
{
	for ( auto iSetVariable = m_setInternalVariables.Begin(); iSetVariable != m_setInternalVariables.End(); ++ iSetVariable )
	{
		captions.PushBack( iSetVariable->GetCaption() );
	}
}

void CBehaviorGraphStateTransitionNode::ConvertTransitionConditonTo(const CBehaviorGraphStateTransitionNode* newTransitionOwner)
{
	if ( const IBehaviorStateTransitionCondition* condition = newTransitionOwner->GetTransitionCondition() )
	{
		ConvertTransitionConditonTo( condition );
	}
}

void CBehaviorGraphStateTransitionNode::ConvertTransitionConditonTo(const IBehaviorStateTransitionCondition* transitionTemplate)
{
	if ( m_transitionCondition )
	{
		if ( m_transitionCondition->CanConvertTo( transitionTemplate ) )
		{
			// Create new transition
			IBehaviorStateTransitionCondition* transition = SafeCast<IBehaviorStateTransitionCondition>(transitionTemplate->Clone(this));

			// Convert
			transition->CopyDataFrom( m_transitionCondition );

			// Delete old
			m_transitionCondition->Discard();
			m_transitionCondition = transition;
		}
		else if ( m_transitionCondition->GetClass() == transitionTemplate->GetClass() )
		{
			// Copy data
			m_transitionCondition->CopyDataFrom(transitionTemplate);
		}
		else
		{
			// Delete
			m_transitionCondition->Discard();
			m_transitionCondition = NULL;

			// Create new
			m_transitionCondition = SafeCast<IBehaviorStateTransitionCondition>(transitionTemplate->Clone(this));
		}

		OnRebuildSockets();
	}
	else
	{
		// Create new
		m_transitionCondition = SafeCast<IBehaviorStateTransitionCondition>(transitionTemplate->Clone(this));
		OnRebuildSockets();
	}
}

#endif

void CBehaviorGraphStateTransitionNode::CopyFrom( const CBehaviorGraphStateTransitionNode* node )
{
	m_transitionPriority =				node->m_transitionPriority;
	m_isEnabled =						node->m_isEnabled;

	if ( m_transitionCondition && node->GetTransitionCondition() )
	{
		m_transitionCondition->CopyDataFrom( node->GetTransitionCondition() );
	}
}

void CBehaviorGraphStateTransitionNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	if ( m_transitionCondition )
	{
		m_transitionCondition->GetUsedVariablesAndEvents( var, vecVar, events, intVar, intVecVar );
	}
}

Bool CBehaviorGraphStateTransitionNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	const CBehaviorGraphStateNode* node = GetDestState();
	if ( node )
	{
		return node->PreloadAnimations( instance );
	}
	return true;
}

CBehaviorGraphStateNode* CBehaviorGraphStateTransitionNode::GetSourceState()
{
	return m_cachedStartStateNode;
}

CBehaviorGraphStateNode* CBehaviorGraphStateTransitionNode::GetDestState()
{
	return m_cachedEndStateNode;
}

const CBehaviorGraphStateNode* CBehaviorGraphStateTransitionNode::GetSourceState() const
{
	return m_cachedStartStateNode;
}

const CBehaviorGraphStateNode* CBehaviorGraphStateTransitionNode::GetDestState() const
{
	return m_cachedEndStateNode;
}

CBehaviorGraphStateNode* CBehaviorGraphStateTransitionNode::GetCloserState()
{
	return m_cachedEndStateNode ? m_cachedEndStateNode : m_cachedStartStateNode;
}

const CBehaviorGraphStateNode* CBehaviorGraphStateTransitionNode::GetCloserState() const
{
	return m_cachedEndStateNode ? m_cachedEndStateNode : m_cachedStartStateNode;
}

//////////////////////////////////////////////////////////////////////////

Bool CIsRagdolledTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	return instance.GetAnimatedComponent()->IsRagdolled( true );
}

Bool CIsRagdolledTransitionCondition::Test( CBehaviorGraphInstance& instance ) const
{
	return Check( instance );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
