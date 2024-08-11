/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphVectorVariableNode.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVectorVariableBaseNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVectorMathNode );
IMPLEMENT_RTTI_ENUM( EBehaviorVectorMathOp );

CBehaviorGraphVectorVariableBaseNode::CBehaviorGraphVectorVariableBaseNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphVectorVariableBaseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Value ) ) );	

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( X ), false ) );	
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Y ), false ) );	
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Z ), false ) );	
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( W ), false ) );	
}

#endif

// --------------------------------------------------------------------------------------------------------------------
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstantVectorValueNode );

CBehaviorGraphConstantVectorValueNode::CBehaviorGraphConstantVectorValueNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstantVectorValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
}

String CBehaviorGraphConstantVectorValueNode::GetCaption() const
{
	return String::Printf( TXT("Vector [ %.1f %.1f %.1f %.1f ]"), m_value.X, m_value.Y, m_value.Z, m_value.W );
}

#endif

// --------------------------------------------------------------------------------------------------------------------
CBehaviorGraphVectorMathNode::CBehaviorGraphVectorMathNode()	
	: m_operation( BVMO_Add )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphVectorMathNode::OnPropertyPostChange( IProperty *prop )
{
	if ( prop->GetName() == CNAME( operation ) )
	{
		OnRebuildSockets();
	}
}

void CBehaviorGraphVectorMathNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Vector operations
	if ( m_operation == BVMO_Add || m_operation == BVMO_Subtract || m_operation == BVMO_Multiply || m_operation == BVMO_Divide || m_operation == BVMO_CrossProduct )
	{
		CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( OutVector ) ) );
	}

	// Scalar operations
	if ( m_operation == BVMO_DotProduct || m_operation == BVMO_Length || m_operation == BVMO_XComponent || m_operation == BVMO_YComponent || m_operation == BVMO_ZComponent || m_operation == BVMO_WComponent )
	{
		CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( OutScalar ) ) );
	}

	// Input value
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Vector1 ) ) );

	// Input for vector operations
	if ( m_operation == BVMO_Add || m_operation == BVMO_Subtract || m_operation == BVMO_CrossProduct || m_operation == BVMO_DotProduct )
	{
		CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Vector2 ) ) );
	}

	// Input for scalar operations
	if ( m_operation == BVMO_Multiply || m_operation == BVMO_Divide )
	{
		CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Scalar ) ) );
	}
}

#endif

void CBehaviorGraphVectorMathNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedScalarInputNode )
	{
		m_cachedScalarInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphVectorMathNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Activate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Activate( instance );
	}

	if ( m_cachedScalarInputNode )
	{
		m_cachedScalarInputNode->Activate( instance );
	}
}

void CBehaviorGraphVectorMathNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Deactivate( instance );
	}

	if ( m_cachedScalarInputNode )
	{
		m_cachedScalarInputNode->Deactivate( instance );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphVectorMathNode::GetCaption() const
{ 
	// Get operation name
	switch ( m_operation )
	{
		case BVMO_Add:				return TXT("Add Vector1 to Vector2");
		case BVMO_Subtract:			return TXT("Subtract Vector2 from Vector1");
		case BVMO_Multiply:			return TXT("Multiply Vector1 by Scalar");		
		case BVMO_Divide:			return TXT("Divide Vector1 by Scalar");
		case BVMO_CrossProduct:		return TXT("Cross product of Vector1 and Vector2");
		case BVMO_DotProduct:		return TXT("Dot product of Vector1 and Vector2");
		case BVMO_Length:			return TXT("Length of Vector1");
		case BVMO_XComponent:		return TXT("X component of Vector1");
		case BVMO_YComponent:		return TXT("Y component of Vector1");
		case BVMO_ZComponent:		return TXT("Z component of Vector1");
		case BVMO_WComponent:		return TXT("W component of Vector1");
	}

	// Default name
	return TXT("Vector math op"); 	
}

#endif

Vector CBehaviorGraphVectorMathNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	// Get first value
	Vector arg1 = Vector::ZEROS;
	if ( m_cachedFirstInputNode )
	{
		arg1 = m_cachedFirstInputNode->GetVectorValue( instance );
	}

	// Get second value
	Vector arg2 = Vector::ZEROS;
	if ( m_cachedSecondInputNode )
	{
		arg2 = m_cachedSecondInputNode->GetVectorValue( instance );
	}

	// Get scalar value
	Float scalar = 1.0f;
	if ( m_cachedScalarInputNode )
	{
		scalar = m_cachedScalarInputNode->GetValue( instance );
	}

	// Perform operation
	switch ( m_operation )	
	{
	case BVMO_Add:
		return arg1 + arg2;

	case BVMO_Subtract:
		return arg1 - arg2;

	case BVMO_Multiply:
		return arg1 * scalar;

	case BVMO_Divide:
		return arg1 / scalar;

	case BVMO_CrossProduct:
		return Vector::Cross( arg1, arg2 );
	}

	// Default value
	return Vector::ZEROS;
}

Float CBehaviorGraphVectorMathNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	// Get first value
	Vector arg1 = Vector::ZEROS;
	if ( m_cachedFirstInputNode )
	{
		arg1 = m_cachedFirstInputNode->GetVectorValue( instance );
	}

	// Get second value
	Vector arg2 = Vector::ZEROS;
	if ( m_cachedSecondInputNode )
	{
		arg2 = m_cachedSecondInputNode->GetVectorValue( instance );
	}

	// Get scalar value
	Float scalar = 1.0f;
	if ( m_cachedScalarInputNode )
	{
		scalar = m_cachedScalarInputNode->GetValue( instance );
	}

	// Perform operation
	switch ( m_operation )
	{
		case BVMO_DotProduct:
			return Vector::Dot3( arg1, arg2 );

		case BVMO_Length:
			return arg1.Mag3();

		case BVMO_XComponent:
			return arg1.X;

		case BVMO_YComponent:
			return arg1.Y;

		case BVMO_ZComponent:
			return arg1.Z;

		case BVMO_WComponent:
			return arg1.W;
	}
		
	// No operation defined
	return 0.0f;
}

void CBehaviorGraphVectorMathNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
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

	if ( m_cachedScalarInputNode )
	{
		m_cachedScalarInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphVectorMathNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedFirstInputNode = CacheVectorValueBlock( TXT("Vector1") );
	m_cachedSecondInputNode = CacheVectorValueBlock( TXT("Vector2") );
	m_cachedScalarInputNode = CacheValueBlock( TXT("Scalar") );
}
