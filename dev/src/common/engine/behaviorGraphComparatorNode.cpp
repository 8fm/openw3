/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphComparatorNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphComparatorNode );

CBehaviorGraphComparatorNode::CBehaviorGraphComparatorNode()	
	: m_operation( CF_Equal )
	, m_trueValue( 0.0f )
	, m_falseValue( 0.0f )
	, m_firstValue( 0.0f )
	, m_secondValue( 0.0f )
{
}

void CBehaviorGraphComparatorNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_conditionFulfilled;
}

void CBehaviorGraphComparatorNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );	

	instance[ i_conditionFulfilled ] = false;
}

void CBehaviorGraphComparatorNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );
	INST_PROP_INIT;
	INST_PROP( i_conditionFulfilled );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphComparatorNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Arg1 ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Arg2 ), false ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( True ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( False ) ) );
}
#endif

void CBehaviorGraphComparatorNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedFirstInputNode = CacheValueBlock( TXT("Arg1") );
	m_cachedSecondInputNode = CacheValueBlock( TXT("Arg2") );
	m_cachedTrueInputNode = CacheValueBlock( TXT("True") );
	m_cachedFalseInputNode = CacheValueBlock( TXT("False") );
}

void CBehaviorGraphComparatorNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Comparator );
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Update( context, instance, timeDelta );
	}

	Bool & condition = instance[ i_conditionFulfilled ];
	Bool prevCondition = condition;
	UpdateCondition( instance );

	if ( condition != prevCondition )
	{
		if ( condition )
		{
			if ( m_cachedFalseInputNode )
			{
				m_cachedFalseInputNode->Deactivate( instance );
			}

			if ( m_cachedTrueInputNode )
			{
				m_cachedTrueInputNode->Activate( instance );
			}
		}
		else
		{
			if ( m_cachedTrueInputNode )
			{
				m_cachedTrueInputNode->Deactivate( instance );
			}

			if ( m_cachedFalseInputNode )
			{
				m_cachedFalseInputNode->Activate( instance );
			}
		}
	}

	if ( m_cachedTrueInputNode && condition)
	{
		m_cachedTrueInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedFalseInputNode && ! condition )
	{
		m_cachedFalseInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphComparatorNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Activate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Activate( instance );
	}

	UpdateCondition( instance );
	Bool const & condition = instance[ i_conditionFulfilled ];

	if ( m_cachedTrueInputNode && condition )
	{
		m_cachedTrueInputNode->Activate( instance );
	}

	if ( m_cachedFalseInputNode && ! condition)
	{
		m_cachedFalseInputNode->Activate( instance );
	}
}

void CBehaviorGraphComparatorNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Deactivate( instance );
	}

	Bool const & condition = instance[ i_conditionFulfilled ];

	if ( m_cachedTrueInputNode && condition )
	{
		m_cachedTrueInputNode->Deactivate( instance );
	}

	if ( m_cachedFalseInputNode && ! condition )
	{
		m_cachedFalseInputNode->Deactivate( instance );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphComparatorNode::GetCaption() const
{ 
	switch( m_operation )
	{
	case CF_Equal:
		return TXT("==");
	case CF_NotEqual:
		return TXT("!=");
	case CF_Less:
		return TXT("<");
	case CF_LessEqual:
		return TXT("<=");
	case CF_Greater:
		return TXT(">");
	case CF_GreaterEqual:
		return TXT(">=");
	}

	return TXT("Compare"); 	
}

#endif

void CBehaviorGraphComparatorNode::UpdateCondition( CBehaviorGraphInstance& instance ) const
{
	Float arg1 = m_firstValue;
	Float arg2 = m_secondValue;

	if ( m_cachedFirstInputNode )
	{
		arg1 = m_cachedFirstInputNode->GetValue( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		arg2 = m_cachedSecondInputNode->GetValue( instance );
	}

	Bool & condition = instance[ i_conditionFulfilled ];
	switch( m_operation )
	{
	case CF_Equal:
		condition = ( arg1 == arg2 );
		break;
	case CF_NotEqual:
		condition = ( arg1 != arg2 );
		break;
	case CF_Less:
		condition = ( arg1 < arg2 );
		break;
	case CF_LessEqual:
		condition = ( arg1 <= arg2 );
		break;
	case CF_Greater:
		condition = ( arg1 > arg2 );
		break;
	case CF_GreaterEqual:
		condition = ( arg1 >= arg2 );
		break;
	default:
		condition = false;
		break;
	}
}

Float CBehaviorGraphComparatorNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	Bool const & condition = instance[ i_conditionFulfilled ];

	if ( condition )
	{
		return m_cachedTrueInputNode ? m_cachedTrueInputNode->GetValue( instance ) : m_trueValue;
	}
	else
	{
		return m_cachedFalseInputNode ? m_cachedFalseInputNode->GetValue( instance ) : m_falseValue;
	}
}

void CBehaviorGraphComparatorNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
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

	Bool const & condition = instance[ i_conditionFulfilled ];

	if ( m_cachedTrueInputNode && condition )
	{
		m_cachedTrueInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedFalseInputNode && ! condition )
	{
		m_cachedFalseInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphEnumComparatorNode );

CBehaviorGraphEnumComparatorNode::CBehaviorGraphEnumComparatorNode()	
	: m_operation( CF_Equal )
{
}

Int32 CBehaviorGraphEnumComparatorNode::ReadVariantEnum( CBehaviorGraphInstance& instance, const CVariant& variant ) const
{
	Int32 val = 0;

	const IRTTIType* type = variant.GetRTTIType();	
	if( type )
	{
		if( type->GetType() == RT_Enum )
		{

			const CEnum* e = static_cast< const CEnum * >( type );				
			val = e->GetAsInt( (const void * )variant.GetData() );
			CName temp;
			if( !e->FindName( val, temp ) )
			{
				BEH_DUMP_ERROR( instance, String::Printf( TXT("Value %d in enum %s not found"), val, type->GetName().AsString().AsChar() ) );
			}

		}
		else
		{
			ASSERT( 0 && TXT("Type is not enum") );
		}
	}

	return val;
}

void CBehaviorGraphEnumComparatorNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_enumValueVar;
}

void CBehaviorGraphEnumComparatorNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );	

	Int32 val = ReadVariantEnum( instance, m_enumValue );
	instance[ i_enumValueVar ] = Float( val );
}

void CBehaviorGraphEnumComparatorNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );
	INST_PROP_INIT;
	INST_PROP( i_enumValueVar );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphEnumComparatorNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Input ) ) );
}

String CBehaviorGraphEnumComparatorNode::GetCaption() const
{ 
	String str;	
	m_enumValue.ToString( str );

	switch( m_operation )
	{
	case CF_Equal:
		return String::Printf( TXT("== %s"), str.AsChar() );
	case CF_NotEqual:
		return String::Printf( TXT("!= %s"), str.AsChar() );
	case CF_Less:
		return String::Printf( TXT("< %s"), str.AsChar() );
	case CF_LessEqual:		
		return String::Printf( TXT("<= %s"), str.AsChar() );
	case CF_Greater:		
		return String::Printf( TXT("> %s"), str.AsChar() );
	case CF_GreaterEqual:		
		return String::Printf( TXT(">= %s"), str.AsChar() );
	}

	return TXT("Compare enum"); 	
}

#endif

void CBehaviorGraphEnumComparatorNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedFirstInputNode = CacheValueBlock( TXT("Input") );	
}

void CBehaviorGraphEnumComparatorNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphEnumComparatorNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Activate( instance );
	}
}

void CBehaviorGraphEnumComparatorNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}
}

Float CBehaviorGraphEnumComparatorNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	Float arg1 = 0.0f;
	Float arg2 = instance[ i_enumValueVar ];

	if ( m_cachedFirstInputNode )
	{
		arg1 = m_cachedFirstInputNode->GetValue( instance );
	}

	switch( m_operation )
	{
	case CF_Equal:
		return ( arg1 == arg2 ) ? 1.0f : 0.0f;
	case CF_NotEqual:
		return ( arg1 != arg2 ) ? 1.0f : 0.0f;
	case CF_Less:
		return ( arg1 < arg2 ) ? 1.0f : 0.0f;
	case CF_LessEqual:
		return ( arg1 <= arg2 ) ? 1.0f : 0.0f;
	case CF_Greater:
		return ( arg1 > arg2 ) ? 1.0f : 0.0f;
	case CF_GreaterEqual:
		return ( arg1 >= arg2 ) ? 1.0f : 0.0f;
	default:
		ASSERT( 0 );
	}

	return 0.0f;
}

void CBehaviorGraphEnumComparatorNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

