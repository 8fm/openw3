/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphBlendMultipleNode.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../core/feedback.h"

#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "graphConnectionRebuilder.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorIncludes.h"
#include "animSyncInfo.h"
#include "extAnimEvent.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( MinControlValue )
RED_DEFINE_STATIC_NAME( MaxControlValue )

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendMultipleNode );

const Float CBehaviorGraphBlendMultipleNode::ACTIVATION_THRESHOLD = 0.001f;

namespace
{
	Bool IsFirstInputActive( Float var )
	{
		return var < (1.0f - CBehaviorGraphBlendMultipleNode::ACTIVATION_THRESHOLD);
	}

	Bool IsSecondInputActive( Float var )
	{
		return var > CBehaviorGraphBlendMultipleNode::ACTIVATION_THRESHOLD;
	}
}

CBehaviorGraphBlendMultipleNode::CBehaviorGraphBlendMultipleNode()
	: m_synchronize( true )
	, m_syncMethod( NULL )
	, m_minControlValue( 0.0f )
	, m_maxControlValue( 1.0f )
	, m_radialBlending( false )
	, m_takeEventsFromMoreImportantInput( false )
{
}

void CBehaviorGraphBlendMultipleNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_selectedInputA;
	compiler << i_selectedInputB;
	compiler << i_controlValue;
	compiler << i_blendWeight;
	compiler << i_minControlValue;
	compiler << i_maxControlValue;
}

void CBehaviorGraphBlendMultipleNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_minControlValue ] = m_minControlValue;
	instance[ i_maxControlValue ] = m_maxControlValue;

	InternalReset( instance );
}

void CBehaviorGraphBlendMultipleNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_selectedInputA );
	INST_PROP( i_selectedInputB );
	INST_PROP( i_controlValue );
	INST_PROP( i_blendWeight );
	INST_PROP( i_minControlValue );
	INST_PROP( i_maxControlValue );
}

void CBehaviorGraphBlendMultipleNode::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );
	file << m_sortedInputs;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlendMultipleNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == CNAME( inputValues ) )
	{
		OnInputListChange();
	}
}

#endif

void CBehaviorGraphBlendMultipleNode::OnInputListChange()
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	OnRebuildSockets();
#endif

	SortInputs();
}

struct Pred
{
public:
	Bool operator()( const CBehaviorGraphBlendMultipleNode::tSortedInput& lhs, 
		const CBehaviorGraphBlendMultipleNode::tSortedInput &rhs ) const
	{
		return lhs.m_second < rhs.m_second;
	}
};

void CBehaviorGraphBlendMultipleNode::SortInputs()
{
	m_sortedInputs.Clear();
	for( Uint32 i=0; i<m_inputValues.Size(); ++i )
		m_sortedInputs.PushBack( MakePair( i, m_inputValues[i] ) );

	Sort( m_sortedInputs.Begin(), m_sortedInputs.End(), Pred() );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlendMultipleNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );

	for( Uint32 i=0; i<m_inputValues.Size(); ++i )
	{
		const String socketName = String::Printf( TXT("Input%d"), i );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CName( socketName ) ) );
	}

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MinControlValue ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MaxControlValue ), false ) );
}

#endif

Uint32 CBehaviorGraphBlendMultipleNode::GetNumInputs() const
{
	return m_inputValues.Size();
}

void CBehaviorGraphBlendMultipleNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Get control variable
	m_cachedControlValueNode = CacheValueBlock( CNAME( Weight ) );
	m_cachedMinControlValue = CacheValueBlock( CNAME( MinControlValue ) );
	m_cachedMaxControlValue = CacheValueBlock( CNAME( MaxControlValue ) );

	// Cache input nodes
	m_cachedInputNodes.Clear();
	const Uint32 numInputs = GetNumInputs();
	for ( Uint32 i=0; i<numInputs; i++ )
	{
		const String socketName = String::Printf( TXT("Input%d"), i );
		m_cachedInputNodes.PushBack( CacheBlock( socketName ) );
	}
}

void CBehaviorGraphBlendMultipleNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( BlendMulti );

	if ( m_inputValues.Empty() )
	{
		return;
	}

	// update variable 
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );
	}
	if ( m_cachedMinControlValue )
	{
		m_cachedMinControlValue->Update( context, instance, timeDelta );
	}
	if ( m_cachedMaxControlValue )
	{
		m_cachedMaxControlValue->Update( context, instance, timeDelta );
	}

	// copy variable value (so it's constant across update and sample)
	UpdateControlValue( instance );

	// process activations
	ProcessActivations( instance );

	// synchronize children playback
	Synchronize( instance, timeDelta );

	// update nodes
	const Float weight = GetBlendWeight( instance );
	const Int32 inputA = GetInputA( instance );
	const Int32 inputB = GetInputB( instance );

	if ( !IsSecondInputActive( weight ) )
	{
		RED_ASSERT( inputA != -1 );
		const CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		if ( firstInput )
		{
			firstInput->Update( context, instance, timeDelta );
		}
	}
	else if ( !IsFirstInputActive( weight ) )
	{
		RED_ASSERT( inputB != -1 );
		const CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
		if ( secondInput )
		{
			secondInput->Update( context, instance, timeDelta );
		}
	}
	else
	{
		RED_ASSERT( inputA != -1 );
		RED_ASSERT( inputB != -1 );
		RED_ASSERT( inputA != inputB );

		const CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		if ( firstInput ) 
		{
			firstInput->Update( context, instance, timeDelta );
		}

		const CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
		if ( secondInput )
		{
			secondInput->Update( context, instance, timeDelta );
		}
	}

}

void CBehaviorGraphBlendMultipleNode::UpdateControlValue( CBehaviorGraphInstance& instance ) const
{
	Float& value = instance[ i_controlValue ];
	const Float inputValue = m_cachedControlValueNode ? m_cachedControlValueNode->GetValue( instance ) : 0.0f;
	value = inputValue;

	if ( m_cachedMinControlValue )
	{
		const Float inputValueMin = m_cachedMinControlValue->GetValue( instance );
		instance [ i_minControlValue ] = inputValueMin;
	}
	if ( m_cachedMaxControlValue )
	{
		const Float inputValueMax = m_cachedMaxControlValue->GetValue( instance );
		instance [ i_maxControlValue ] = inputValueMax;
	}
}

void CBehaviorGraphBlendMultipleNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( BlendMulti );

	if ( m_inputValues.Empty() )
	{
		return;
	}

	const Float weight = GetBlendWeight( instance );
	const Int32 inputA = GetInputA( instance );
	const Int32 inputB = GetInputB( instance );

	if ( !IsSecondInputActive( weight ) )
	{
		RED_ASSERT( inputA != -1 );
		const CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		if ( firstInput ) 
		{
			firstInput->Sample( context, instance, output );
		}
	}
	else if ( !IsFirstInputActive( weight ) )
	{
		RED_ASSERT( inputB != -1 );
		const CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
		if ( secondInput )
		{
			secondInput->Sample( context, instance, output );
		}
	}
	else
	{
		RED_ASSERT( inputA != -1 );
		RED_ASSERT( inputB != -1 );
		RED_ASSERT( inputA != inputB );

		CCacheBehaviorGraphOutput cachePose1( context );
		CCacheBehaviorGraphOutput cachePose2( context );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			const CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
			if ( firstInput ) 
			{
				firstInput->Sample( context, instance, *temp1 );
			}

			const CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
			if ( secondInput )
			{
				secondInput->Sample( context, instance, *temp2 );
			}

#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				// Interpolate poses
				output.SetInterpolate( *temp1, *temp2, weight );
			}
			else
			{
				// Interpolate poses
				output.SetInterpolateME( *temp1, *temp2, weight );
			}
#else
			output.SetInterpolate( *temp1, *temp2, weight );
#endif

			// Merge events and used anims
			if ( m_takeEventsFromMoreImportantInput )
			{
				SBehaviorGraphOutput* outputToMerge = ( weight < 0.5f ) ? temp1 : temp2;
				output.MergeEvents( *outputToMerge, 1.0f );
			}
			else
			{
				output.MergeEventsAndUsedAnims( *temp1, *temp2, 1.0f - weight, weight );
			}
		}
	}
}

void CBehaviorGraphBlendMultipleNode::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_inputValues.Empty() )
	{
		return;
	}

	const Float blendWeight = GetBlendWeight( instance );
	const Int32 inputA = GetInputA( instance );
	const Int32 inputB = GetInputB( instance );

	if ( !IsSecondInputActive( blendWeight ) )
	{
		RED_ASSERT( inputA != -1 );
		CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		if ( firstInput ) 
		{
			firstInput->GetSyncData( instance ).Reset();
		}
	}
	else if ( !IsFirstInputActive( blendWeight ) )
	{
		RED_ASSERT( inputB != -1 );
		CBehaviorGraphNode *secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
		if ( secondInput )
		{
			secondInput->GetSyncData( instance ).Reset();
		}
	}
	else
	{
		RED_ASSERT( inputA != -1 );
		RED_ASSERT( inputB != -1 );
		RED_ASSERT( inputA != inputB );

		CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;

		if ( m_synchronize && m_syncMethod && firstInput && secondInput )
		{
			m_syncMethod->Synchronize( instance, firstInput, secondInput, blendWeight, timeDelta );
		}
		else
		{
			if ( firstInput )
			{
				firstInput->GetSyncData( instance ).Reset();
			}

			if ( secondInput )
			{
				secondInput->GetSyncData( instance ).Reset();
			}
		}
	}
}

void CBehaviorGraphBlendMultipleNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_inputValues.Empty() )
	{
		return;
	}

	Float blendWeight = GetBlendWeight( instance );
	Int32 inputA = GetInputA( instance );
	Int32 inputB = GetInputB( instance );

	if ( inputA == -1 && inputB == -1 )
	{
		SelectInputsAndCalcBlendWeight( instance, inputA, inputB, blendWeight );
	}

	if ( !IsSecondInputActive( blendWeight ) )
	{
		RED_ASSERT( inputA != -1 );
		CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		if ( firstInput ) 
		{
			firstInput->GetSyncInfo( instance, info );
		}
	}
	else if ( !IsFirstInputActive( blendWeight ) )
	{
		RED_ASSERT( inputB != -1 );
		CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
		if ( secondInput )
		{
			secondInput->GetSyncInfo( instance, info );
		}
	}
	else
	{
		RED_ASSERT( inputA != -1 );
		RED_ASSERT( inputB != -1 );

		CSyncInfo firstSyncInfo;
		CSyncInfo secondSyncInfo;

		CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		if ( firstInput ) 
		{
			firstInput->GetSyncInfo( instance, firstSyncInfo );
		}

		CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
		if ( secondInput )
		{
			secondInput->GetSyncInfo( instance, secondSyncInfo );
		}

		info.SetInterpolate( firstSyncInfo, secondSyncInfo, blendWeight );
	}
}

void CBehaviorGraphBlendMultipleNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_inputValues.Empty() )
	{
		return;
	}

	const Int32 inputA = GetInputA( instance );
	const Int32 inputB = GetInputB( instance );

	CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
	CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;

	if ( m_syncMethod && m_synchronize )
	{
		if ( firstInput ) 
		{
			m_syncMethod->SynchronizeTo( instance, firstInput, info );
		}
		
		if ( secondInput )
		{
			m_syncMethod->SynchronizeTo( instance, secondInput, info );
		}
	}
	else
	{
		if ( firstInput )
		{
			firstInput->SynchronizeTo( instance, info );
		}

		if ( secondInput )
		{
			secondInput->SynchronizeTo( instance, info );
		}
	}
}


void CBehaviorGraphBlendMultipleNode::ProcessActivations( CBehaviorGraphInstance& instance ) const
{
	const Float prevWeight = GetBlendWeight( instance );
	const Int32 prevInputA = GetInputA( instance );
	const Int32 prevInputB = GetInputB( instance );

	const Bool prevFirstActive = prevInputA != -1 && IsFirstInputActive( prevWeight );
	const Bool prevSecondActive = prevInputB != -1 && IsSecondInputActive( prevWeight );

	// New version
	//CSyncInfo syncInfo;
	//GetSyncInfo( instance, syncInfo );
	//++ Old version
	const CBehaviorGraphNode* syncSource = prevInputA != -1 ? m_cachedInputNodes[ prevInputA ] : nullptr;
	if ( !prevFirstActive || !syncSource )
	{
		syncSource = NULL;
		if ( prevSecondActive )
		{
			syncSource = prevInputB != -1 ? m_cachedInputNodes[ prevInputB ] : nullptr;
		}
	}
	CSyncInfo syncInfo;
	if ( syncSource && m_synchronize && m_syncMethod )
	{
		syncSource->GetSyncInfo( instance, syncInfo );
	}
	//-- Old version

	Int32 currInputA = -1;
	Int32 currInputB = -1;
	Float currBlendWeight = 0.f;
	SelectInputsAndCalcBlendWeight( instance, currInputA, currInputB, currBlendWeight );

	RED_ASSERT( currInputA != -1 );
	RED_ASSERT( currInputB != -1 );
	RED_ASSERT( currInputA != currInputB );

	const Bool currFirstActive = IsFirstInputActive( currBlendWeight );
	const Bool currSecondActive = IsSecondInputActive( currBlendWeight );

	// Handle deactivations
	if ( prevFirstActive )
	{
		if ( !( ( currFirstActive && ( prevInputA == currInputA ) ) ||
			( currSecondActive && ( prevInputA == currInputB ) ) ) )
		{
			CBehaviorGraphNode* input = prevInputA != -1 ? m_cachedInputNodes[ prevInputA ] : nullptr;
			if ( input )
			{
				input->Deactivate( instance );
			}
		}
	}

	if ( prevSecondActive )
	{
		if ( !( ( currFirstActive && ( prevInputB == currInputA ) ) ||
			( currSecondActive && ( prevInputB == currInputB ) ) ) )
		{
			CBehaviorGraphNode* input = prevInputB != -1 ? m_cachedInputNodes[ prevInputB ] : nullptr;
			if ( input )
			{
				input->Deactivate( instance );
			}
		}
	}

	// Handle activations
	if ( currFirstActive )
	{
		if ( !( ( prevFirstActive && ( currInputA == prevInputA ) ) ||
			( prevSecondActive && ( currInputA == prevInputB ) ) ) )
		{
			RED_ASSERT( currInputA != -1 );
			CBehaviorGraphNode *input = m_cachedInputNodes[ currInputA ];
			if ( input )
			{
				input->Activate( instance );

				if ( syncSource && m_synchronize && m_syncMethod )
				{
					m_syncMethod->SynchronizeTo( instance, input, syncInfo );
				}
			}
		}
	}

	if ( currSecondActive )
	{
		if ( !( ( prevFirstActive && ( currInputB == prevInputA ) ) ||
			( prevSecondActive && ( currInputB == prevInputB ) ) ) )
		{
			RED_ASSERT( currInputB != -1 );
			CBehaviorGraphNode *input = m_cachedInputNodes[ currInputB ];
			if ( input )
			{
				input->Activate( instance );

				if ( syncSource && m_synchronize && m_syncMethod )
				{
					m_syncMethod->SynchronizeTo( instance, input, syncInfo );
				}
			}
		}
	}

	/*
	const Bool isPrevACurrB = prevInputA == currInputB && prevInputA != -1;
	const Bool isPrevBCurrA = prevInputB == currInputA && prevInputB != -1;

	if ( prevInputA != currInputA )
	{
		const CBehaviorGraphNode* prevNodeA = prevInputA != -1 ? m_cachedInputNodes[ prevInputA ] : nullptr;
		const CBehaviorGraphNode* currNodeA = currInputA != -1 ? m_cachedInputNodes[ currInputA ] : nullptr;

		if ( prevNodeA && !isPrevACurrB )
		{
			prevNodeA->Deactivate( instance );
		}
		if ( currNodeA && !isPrevBCurrA )
		{
			currNodeA->Activate( instance );

			if ( m_synchronize && m_syncMethod )
			{
				m_syncMethod->SynchronizeTo( instance, currNodeA, syncInfo );
			}
		}
	}

	if ( prevInputB != currInputB )
	{
		const CBehaviorGraphNode* prevNodeB = prevInputB != -1 ? m_cachedInputNodes[ prevInputB ] : nullptr;
		const CBehaviorGraphNode* currNodeB = currInputB != -1 ? m_cachedInputNodes[ currInputB ] : nullptr;

		if ( prevNodeB && !isPrevBCurrA )
		{
			prevNodeB->Deactivate( instance );
		}
		if ( currNodeB && !isPrevACurrB )
		{
			currNodeB->Activate( instance );

			if ( m_synchronize && m_syncMethod )
			{
				m_syncMethod->SynchronizeTo( instance, currNodeB, syncInfo );
			}
		}
	}
	*/

	{
		const CBehaviorGraphNode* currNodeA = currInputA != -1 ? m_cachedInputNodes[ currInputA ] : nullptr;
		if ( currNodeA && currFirstActive )
		{
			ASSERT( currNodeA->IsActive( instance ) );
		}
		const CBehaviorGraphNode* currNodeB = currInputB != -1 ? m_cachedInputNodes[ currInputB ] : nullptr;
		if ( currNodeB && currSecondActive )
		{
			ASSERT( currNodeB->IsActive( instance ) );
		}
	}

	instance[ i_selectedInputA ] = currInputA;
	instance[ i_selectedInputB ] = currInputB;
	instance[ i_blendWeight ] = currBlendWeight;
}

void CBehaviorGraphBlendMultipleNode::SelectInputsAndCalcBlendWeight( const CBehaviorGraphInstance& instance, Int32& firstIndex, Int32& secondIndex, Float& alpha ) const
{
	if ( m_inputValues.Empty() )
		return;

	const Float minControlValue = instance[ i_minControlValue ];
	const Float maxControlValue = instance[ i_maxControlValue ];
	Float value = instance[ i_controlValue ];

	// clamp control value
	Float clampMin = Max( m_sortedInputs[0].m_second, minControlValue );
	Float clampMax = Min( m_sortedInputs[ m_sortedInputs.Size()-1 ].m_second, maxControlValue );

	if ( m_radialBlending )
	{
		if ( clampMax - clampMin < 0.001f )
		{
			value = clampMin;
		}
		else
		{
			Float reminder = fmodf( value - clampMin, clampMax - clampMin );
			if ( reminder < 0.0f )
			{
				reminder += clampMax;
			}
			value = reminder + clampMin;
		}
	}
	else
	{
		value = Clamp( value, clampMin, clampMax );
	}

	// find 
	if ( m_radialBlending )
	{
		if ( value < m_sortedInputs[ 0 ].m_second )
		{
			firstIndex = m_sortedInputs.Size()-1;
			secondIndex = 0;

			Float segmentLength = ( clampMax - m_sortedInputs[ firstIndex ].m_second ) +
								  ( m_sortedInputs[ secondIndex ].m_second - clampMin );

			if ( segmentLength > 0.001f )
			{
				alpha = ( ( clampMax - m_sortedInputs[ firstIndex ].m_second ) + ( value - clampMin ) ) / segmentLength;
			}
			else
			{
				alpha = 0.0f;
			}
		}
		else if ( value > m_sortedInputs[ m_sortedInputs.Size()-1 ].m_second )
		{
			firstIndex = m_sortedInputs.Size()-1;
			secondIndex = 0;

			Float segmentLength = ( clampMax - m_sortedInputs[ firstIndex ].m_second ) +
								  ( m_sortedInputs[ secondIndex ].m_second - clampMin );

			if ( segmentLength > 0.001f )
			{
				alpha = ( value - m_sortedInputs[ firstIndex ].m_second ) / segmentLength;
			}
			else
			{
				alpha = 0.0f;
			}
		}
		else
		{
			secondIndex = 0;

			while( ( secondIndex + 1 ) < (Int32)m_sortedInputs.Size() && m_sortedInputs[ secondIndex ].m_second <= value )
			{
				++secondIndex;
			}

			firstIndex = secondIndex - 1;
			if ( firstIndex < 0 ) firstIndex = 0;

			alpha = ( value - m_sortedInputs[ firstIndex ].m_second ) / ( m_sortedInputs[ secondIndex ].m_second - m_sortedInputs[ firstIndex ].m_second );
			alpha = Clamp( alpha, 0.0f, 1.0f );
		}

	}
	else
	{
		secondIndex = 0;

		while( ( secondIndex + 1 ) < (Int32)m_sortedInputs.Size() && m_sortedInputs[ secondIndex ].m_second <= value )
		{
			++secondIndex;
		}

		firstIndex = secondIndex - 1;
		if ( firstIndex < 0 ) firstIndex = 0;

		alpha = ( value - m_sortedInputs[ firstIndex ].m_second ) / ( m_sortedInputs[ secondIndex ].m_second - m_sortedInputs[ firstIndex ].m_second );
		alpha = Clamp( alpha, 0.0f, 1.0f );
	}

	firstIndex	= m_sortedInputs[ firstIndex ].m_first;
	secondIndex	= m_sortedInputs[ secondIndex ].m_first;
}

void CBehaviorGraphBlendMultipleNode::AddInput()
{
	m_inputValues.PushBack( 0.0f );

	OnInputListChange();
}

void CBehaviorGraphBlendMultipleNode::RemoveInput( Uint32 index )
{
	ASSERT( index < m_inputValues.Size() );

	m_inputValues.Erase( m_inputValues.Begin() + index );

	OnInputListChange();
}

Bool CBehaviorGraphBlendMultipleNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_inputValues.Empty() )
		return false;

	const Int32 inputA = GetInputA( instance );
	const Int32 inputB = GetInputB( instance );

	Bool retVal = false;

	CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
	if ( firstInput && firstInput->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;
	if ( secondInput && secondInput->ProcessEvent( instance, event ) )
	{
		retVal = true;
	}

	return retVal;
}

void CBehaviorGraphBlendMultipleNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}
	if ( m_cachedMinControlValue )
	{
		m_cachedMinControlValue->Activate( instance );
	}
	if ( m_cachedMaxControlValue )
	{
		m_cachedMaxControlValue->Activate( instance );
	}

	InternalReset( instance );

	UpdateControlValue( instance );

	ProcessActivations( instance );
}

void CBehaviorGraphBlendMultipleNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	const Float blendWeight = GetBlendWeight( instance );
	const Int32 inputA = GetInputA( instance );
	const Int32 inputB = GetInputB( instance );

	CBehaviorGraphNode* inputNodeA = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
	CBehaviorGraphNode* inputNodeB = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;

	if ( inputNodeA && IsFirstInputActive( blendWeight ) )
	{
		RED_ASSERT( inputNodeA->IsActive( instance ) );
		inputNodeA->Deactivate( instance );
	}
	if ( inputNodeB && IsSecondInputActive( blendWeight ) )
	{
		RED_ASSERT( inputNodeB->IsActive( instance ) );
		inputNodeB->Deactivate( instance );
	}
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}
	if ( m_cachedMinControlValue )
	{
		m_cachedMinControlValue->Deactivate( instance );
	}
	if ( m_cachedMaxControlValue )
	{
		m_cachedMaxControlValue->Deactivate( instance );
	}

	InternalReset( instance );

	TBaseClass::OnDeactivated( instance );
}

Int32 CBehaviorGraphBlendMultipleNode::GetInputA( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_selectedInputA ];
}

Int32 CBehaviorGraphBlendMultipleNode::GetInputB( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_selectedInputB ];
}

Float CBehaviorGraphBlendMultipleNode::GetBlendWeight( const CBehaviorGraphInstance& instance ) const
{
	const Float ret = instance[ i_blendWeight ];
	ASSERT( ret >= 0.f && ret <= 1.f );
	return ret;
}

void CBehaviorGraphBlendMultipleNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_selectedInputA ] = -1;
	instance[ i_selectedInputB ] = -1;
	instance[ i_controlValue ] = 0.f;
	instance[ i_blendWeight ] = 0.f;
}

void CBehaviorGraphBlendMultipleNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	// activate inputs
	if ( !m_inputValues.Empty() )
	{
		const Float blendWeight = GetBlendWeight( instance );
		const Int32 inputA = GetInputA( instance );
		const Int32 inputB = GetInputB( instance );

		CBehaviorGraphNode* firstInput = inputA != -1 ? m_cachedInputNodes[ inputA ] : nullptr;
		CBehaviorGraphNode* secondInput = inputB != -1 ? m_cachedInputNodes[ inputB ] : nullptr;

		if ( firstInput && IsFirstInputActive( blendWeight ) )
		{
			firstInput->ProcessActivationAlpha( instance, ( 1.0f - blendWeight ) * alpha );
		}
		if ( secondInput && IsSecondInputActive( blendWeight ) )
		{
			secondInput->ProcessActivationAlpha( instance, blendWeight * alpha );
		}
	}

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}
	if ( m_cachedMinControlValue )
	{
		m_cachedMinControlValue->ProcessActivationAlpha( instance, alpha );
	}
	if ( m_cachedMaxControlValue )
	{
		m_cachedMaxControlValue->ProcessActivationAlpha( instance, alpha );
	}
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CBehaviorGraphBlendMultipleNode::ValidateInEditor( TDynArray< String >& outHumanReadableErrorDesc ) const
{
	Bool ret = true;

	if ( !m_sortedInputs.Empty() )
	{
		if ( m_minControlValue > m_sortedInputs.Front().m_second )
		{
			outHumanReadableErrorDesc.PushBack( TXT("minControlValue is greater than the smallest inputValue!") );
			ret = false;
		}

		if ( m_maxControlValue < m_sortedInputs.Back().m_second )
		{
			outHumanReadableErrorDesc.PushBack( TXT("maxControlValue is lower than the largest inputValue!") );
			ret = false;
		}
	}

	return ret;
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode );

IMPLEMENT_ENGINE_CLASS( IBehaviorGraphBlendMultipleCondNode_DampMethod );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_ConstDampMethod );

IMPLEMENT_ENGINE_CLASS( IBehaviorGraphBlendMultipleCondNode_Condition );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_Multi );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_AnimEvent );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_AnimEnd );

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendMultipleCondNode_Transition );

const Float CBehaviorGraphBlendMultipleCondNode::ACTIVATION_THRESHOLD = 0.001f;

CBehaviorGraphBlendMultipleCondNode::CBehaviorGraphBlendMultipleCondNode()
	: m_synchronizeAnimations( true )
	, m_useTransitions( true )
	, m_useWeightDamp( true )
	, m_useControlValueDamp( true )
	, m_radialBlending( true )
{
}

void CBehaviorGraphBlendMultipleCondNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlValue;
	compiler << i_nodeA;
	compiler << i_nodeB;
	compiler << i_weight;
	compiler << i_nodeA_T;
	compiler << i_nodeB_T;
	compiler << i_weight_T;
	compiler << i_transitionIdx;
	compiler << i_transitionWeight;

	if ( m_weightDampMethod )
	{
		m_weightDampMethod->OnBuildDataLayout( compiler );
	}

	if ( m_controlValueDampMethod )
	{
		m_controlValueDampMethod->OnBuildDataLayout( compiler );
	}

	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->OnBuildDataLayout( compiler );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_controlValue ] = 0.f;
	instance[ i_weight ] = 0.f;
	instance[ i_nodeA ] = -1;
	instance[ i_nodeB ] = -1;
	instance[ i_weight_T ] = 0.f;
	instance[ i_nodeA_T ] = -1;
	instance[ i_nodeB_T ] = -1;
	instance[ i_transitionIdx ] = -1;
	instance[ i_transitionWeight ] = 0.f;
}

void CBehaviorGraphBlendMultipleCondNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );

	if ( m_weightDampMethod )
	{
		m_weightDampMethod->OnInitInstance( instance );
	}

	if ( m_controlValueDampMethod )
	{
		m_controlValueDampMethod->OnInitInstance( instance );
	}

	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->OnInitInstance( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	if ( m_weightDampMethod )
	{
		m_weightDampMethod->OnReleaseInstance( instance );
	}

	if ( m_controlValueDampMethod )
	{
		m_controlValueDampMethod->OnReleaseInstance( instance );
	}

	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->OnReleaseInstance( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue );
	INST_PROP( i_weight );
	INST_PROP( i_nodeA );
	INST_PROP( i_nodeB );
	INST_PROP( i_weight_T );
	INST_PROP( i_nodeA_T );
	INST_PROP( i_nodeB_T );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlendMultipleCondNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == CNAME( inputValues ) )
	{
		OnInputListChange();
	}
}

#endif

void CBehaviorGraphBlendMultipleCondNode::OnInputListChange()
{
	Float prevVal = -NumericLimits< Float >::Max();

	const Uint32 size = m_inputValues.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		Float val = m_inputValues[ i ];
		if ( val < prevVal )
		{
			GFeedback->ShowWarn( TXT("Wrong order in the m_inputValues. Values must be written from the smallest to the largest. Array will be sorted now.") );

			Sort( m_inputValues.Begin(), m_inputValues.End() );

			break;
		}

		prevVal = val;
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	OnRebuildSockets();
#endif
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlendMultipleCondNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	

	for( Uint32 i=0; i<m_inputValues.Size(); ++i )
	{
		const String socketName = String::Printf( TXT("Input%d"), i );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CName( socketName ) ) );
	}

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

Uint32 CBehaviorGraphBlendMultipleCondNode::GetNumInputs() const
{
	return m_inputValues.Size();
}

void CBehaviorGraphBlendMultipleCondNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Get control variable
	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );

	// Cache input nodes
	m_cachedInputNodes.Clear();
	const Uint32 numInputs = GetNumInputs();
	for ( Uint32 i=0; i<numInputs; i++ )
	{
		const String socketName = String::Printf( TXT("Input%d"), i );
		m_cachedInputNodes.PushBack( CacheBlock( socketName ) );
	}
}

void CBehaviorGraphBlendMultipleCondNode::FindActiveNodes( CBehaviorGraphInstance& instance, Float controlValue, Int32& nodeA, Int32& nodeB, Float& weight ) const
{
	const Uint32 size = m_inputValues.Size();
	ASSERT( size == m_cachedInputNodes.Size() );

	nodeA = 0;
	nodeB = 0;
	weight = 0.f;

	for ( Uint32 i=1; i<size; ++i )
	{
		const Float val = m_inputValues[ i ];

		if ( controlValue < val )
		{
			const Float prev = m_inputValues[ i-1 ];
			
			if ( MAbs( prev - val ) > 0.f )
			{
				const Float w = ( controlValue - prev ) / ( val - prev );

				weight = w;
				nodeA = i-1;
				nodeB = i;

				return;
			}
		}
	}

	if ( size > 0 && controlValue >= m_inputValues[ size-1 ] )
	{
		nodeA = size-2;
		nodeB = size-1;
		weight = 1.f;
	}
}

Float CBehaviorGraphBlendMultipleCondNode::FindNearestBoundedWeight( Int32 nodeA, Int32 nodeB, Int32 newNodeA, Int32 newNodeB ) const
{
	ASSERT( nodeA != newNodeA );
	ASSERT( nodeB != newNodeB );

	if ( newNodeA < nodeA )
	{
		ASSERT( newNodeB < nodeB );

		return 0.f;
	}
	else
	{
		ASSERT( newNodeB > nodeB );

		return 1.f;
	}
}

void CBehaviorGraphBlendMultipleCondNode::FindNearestBoundedWeightAndNodes( Int32 nodeA, Int32 nodeB, Float weight, Int32& newNodeA, Int32& newNodeB, Float& newWeight ) const
{
	if ( m_radialBlending && m_inputValues.Size() > 0 )
	{
		const Float posCurr = Lerp( weight, m_inputValues[ nodeA ], m_inputValues[ nodeB ] );
		const Float posDest = Lerp( newWeight, m_inputValues[ newNodeA ], m_inputValues[ newNodeB ] );

		const Float borderMin = m_inputValues[ 0 ];
		const Float borderMax = m_inputValues[ m_inputValues.Size() - 1 ];

		Float distX = 0.f;
		Float distY = 0.f;

		if ( posDest < posCurr )
		{
			distX = posCurr - posDest;
			distY = borderMax - posCurr + posDest - borderMin;
		}
		else
		{
			distX = posDest - posCurr;
			distY = borderMax - posDest + posCurr - borderMin;
		}

		ASSERT( distX >= 0.f );
		ASSERT( distY >= 0.f );

		if ( distX < distY )
		{
			const Float w = FindNearestBoundedNodesCW( nodeA, nodeB, newNodeA, newNodeB );
			newWeight = w;
		}
		else
		{
			const Float w = FindNearestBoundedNodesCCW( nodeA, nodeB, newNodeA, newNodeB );
			newWeight = w;
		}

		if ( newNodeA < 0 )
		{
			ASSERT( newNodeA == -1 );
			ASSERT( newNodeB == 0 );

			newNodeA = m_inputValues.SizeInt() - 2;
			newNodeB = m_inputValues.SizeInt() - 1;
			newWeight = 0.f;
		}
		if ( newNodeB >= m_inputValues.SizeInt() )
		{
			ASSERT( newNodeB == m_inputValues.SizeInt() );
			ASSERT( newNodeA == m_inputValues.SizeInt()-1 );

			newNodeA = 0;
			newNodeB = 1;
			newWeight = 1.f;
		}
	}
	else
	{
		const Float w = FindNearestBoundedNodesCW( nodeA, nodeB, newNodeA, newNodeB );
		newWeight = w;
	}
}

Float CBehaviorGraphBlendMultipleCondNode::FindNearestBoundedNodesCW( Int32 nodeA, Int32 nodeB, Int32& newNodeA, Int32& newNodeB ) const
{
	Float newWeight = FindNearestBoundedWeight( nodeA, nodeB, newNodeA, newNodeB );

	if ( newNodeA > nodeA )
	{
		newNodeA = nodeA+1;
		newNodeB = nodeA+2;
		newWeight = 1.f;
	}
	else if ( newNodeA < nodeA )
	{
		newNodeA = nodeA-1;
		newNodeB = nodeA;
		newWeight = 0.f;
	}
	else
	{
		ASSERT( 0 );
	}

	return newWeight;	
}

Float CBehaviorGraphBlendMultipleCondNode::FindNearestBoundedNodesCCW( Int32 nodeA, Int32 nodeB, Int32& newNodeA, Int32& newNodeB ) const
{
	Float newWeight = 0.f;

	if ( newNodeA > nodeA )
	{
		newNodeA = nodeA-1;
		newNodeB = nodeA;
	}
	else if ( newNodeA < nodeA )
	{
		newNodeA = nodeA+1;
		newNodeB = nodeA+2;

		return 0.f;
	}
	else
	{
		ASSERT( 0 );
	}

	return newWeight;
}

void CBehaviorGraphBlendMultipleCondNode::PreUpdateTransitions( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->PreUpdate( instance, timeDelta );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::ResetTransitions( CBehaviorGraphInstance& instance ) const
{
	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->Reset( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::PostSampleTransitions( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &pose ) const
{
	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->PostSampled( instance, pose );
		}
	}
}

Int32 CBehaviorGraphBlendMultipleCondNode::FindActiveTransition( CBehaviorGraphInstance& instance ) const
{
	const Int32 tSize = m_transitions.SizeInt();
	for ( Int32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			if ( t->Check( instance ) )
			{
				return i;
			}
		}
	}
	return -1;
}

Bool CBehaviorGraphBlendMultipleCondNode::IsTransitionInProgress( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_nodeA_T ] != -1 && instance[ i_nodeB_T ] != -1;
}

Float CBehaviorGraphBlendMultipleCondNode::GetInputRange( Int32 nodeA, Int32 nodeB ) const
{
	ASSERT( nodeA != -1 );
	ASSERT( nodeB != -1 );

	return MAbs( m_inputValues[ nodeA ] - m_inputValues[ nodeB ] );
}

void CBehaviorGraphBlendMultipleCondNode::ProgressNodesWeight( CBehaviorGraphInstance& instance, Float& currWeight, Float destWeight, Float timeDelta, Float inputRange ) const
{
	ASSERT( m_useWeightDamp && m_weightDampMethod );

	if ( m_useWeightDamp && m_weightDampMethod )
	{
		Float restTime;
		m_weightDampMethod->OnUpdate( instance, timeDelta, inputRange, destWeight, currWeight, restTime );
	}
	else
	{
		currWeight = destWeight;
	}
}

void CBehaviorGraphBlendMultipleCondNode::ProgressNodesBoundedWeight( CBehaviorGraphInstance& instance, Float& currWeight, Float timeDelta, Int32 nodeA, Int32 nodeB, Int32 newNodeA, Int32 newNodeB, Float& restTime ) const
{
	// Find nearest weight
	const Float boundedWeight = FindNearestBoundedWeight( nodeA, nodeB, newNodeA, newNodeB );

	ASSERT( m_useWeightDamp && m_weightDampMethod );
	ASSERT( nodeA != -1 );
	ASSERT( nodeB != -1 );

	// Damp weight
	if ( m_useWeightDamp && m_weightDampMethod )
	{
		m_weightDampMethod->OnUpdate( instance, timeDelta, GetInputRange( nodeA, nodeB ), boundedWeight, currWeight, restTime );
	}
	else
	{
		currWeight = boundedWeight;
	}
}

Float CBehaviorGraphBlendMultipleCondNode::GetTransitionDuration( CBehaviorGraphInstance& instance, Int32 transitionIdx ) const
{
	ASSERT( transitionIdx != -1 );
	ASSERT( transitionIdx < m_transitions.SizeInt() );

	const CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ transitionIdx ];
	ASSERT( t );

	return t->GetTransitionDuration();
}

const IBehaviorSyncMethod* CBehaviorGraphBlendMultipleCondNode::GetTransitionSyncMethod( CBehaviorGraphInstance& instance, Int32 transitionIdx ) const
{
	ASSERT( transitionIdx != -1 );
	ASSERT( transitionIdx < m_transitions.SizeInt() );

	const CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ transitionIdx ];
	ASSERT( t );

	return t->GetSyncMethod();
}

const IBehaviorSyncMethod* CBehaviorGraphBlendMultipleCondNode::GetAnimationSyncMethod() const
{
	return m_synchronizeAnimations ? m_syncMethodAnimation : NULL;
}

Bool CBehaviorGraphBlendMultipleCondNode::DoesTransitionBlockEvents( CBehaviorGraphInstance& instance ) const
{
	const Int32 transitionIdx = instance[ i_transitionIdx ];

	ASSERT( transitionIdx != -1 );
	ASSERT( transitionIdx < m_transitions.SizeInt() );

	const CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ transitionIdx ];
	ASSERT( t );

	return t->BlockEvents();
}

void CBehaviorGraphBlendMultipleCondNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	struct InternalLatentAction
	{
		Int32		m_target;
		Bool		m_extraDeltaTimeSet;
		Float		m_extraDeltaTime;
		Bool		m_syncSet;
		CSyncInfo	m_sync;
		Int32		m_transIndex;

		InternalLatentAction() : m_target( -1 ), m_extraDeltaTimeSet( false ), m_syncSet( false ) {}
	};

	BEH_NODE_UPDATE( BlendMultiConds );

	if ( m_inputValues.Empty() )
	{
		return;	
	}

	const Float borderMin = m_inputValues[ 0 ];
	const Float borderMax = m_inputValues[ m_inputValues.Size() - 1 ];

	PreUpdateTransitions( instance, timeDelta );

	Float& controlValue = instance[ i_controlValue ];

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );
		controlValue = Clamp( m_cachedControlValueNode->GetValue( instance ), borderMin, borderMax );
	}

	Int32& nodeA = instance[ i_nodeA ];
	Int32& nodeB = instance[ i_nodeB ];
	Float& weight = instance[ i_weight ];

	Int32& nodeA_T = instance[ i_nodeA_T ];
	Int32& nodeB_T = instance[ i_nodeB_T ];
	Float& weight_T = instance[ i_weight_T ];

	Int32 nodeACached = instance[ i_nodeA ];
	Int32 nodeBCached = instance[ i_nodeB ];
	Float weightCached = instance[ i_weight ];

	Int32 nodeA_TCached = instance[ i_nodeA_T ];
	Int32 nodeB_TCached = instance[ i_nodeB_T ];
	Float weight_TCached = instance[ i_weight_T ];

	Int32 newNodeA = 0;
	Int32 newNodeB = 0;
	Float newWeight = 0.f;

	Int32& transitionIdx = instance[ i_transitionIdx ];
	Float& transitionWeight = instance[ i_transitionWeight ];

	Int32 progressCounter = 0; 

	TStaticArray< InternalLatentAction, 16 > latentSyncs;

	FindActiveNodes( instance, controlValue, newNodeA, newNodeB, newWeight );

	Bool stateT = IsTransitionInProgress( instance );
	Bool running = true;

	EInternalState state = stateT ? IS_UpdateTransition : IS_Update_Init;

	while ( running )
	{
		switch ( state )
		{

		case IS_UpdateTransition:
			{
				ASSERT( nodeA_T != -1 );
				ASSERT( nodeB_T != -1 );
				ASSERT( transitionIdx != -1 );

				if ( transitionWeight >= 1.f )
				{
					// Cache transition's destination
					nodeA = nodeA_T;
					nodeB = nodeB_T;
					weight = weight_T;

					// Reset transition
					nodeA_T = -1;
					nodeB_T = -1;
					transitionIdx = -1;

					// Go to normal state
					state = IS_Update_Init;
				}
				else
				{
					// Progress transition state
					ASSERT( nodeA != nodeA_T );
					ASSERT( nodeB != nodeB_T );
					ASSERT( nodeA != nodeB_T );
					ASSERT( nodeB != nodeA_T );

					const Float duration = GetTransitionDuration( instance, transitionIdx );
					if ( duration > 0.f )
					{
						transitionWeight = Clamp( transitionWeight + timeDelta / duration, 0.f, 1.f );
					}
					else
					{
						transitionWeight = 1.f;
					}

					Float restTime = 0.f;
					Float restTime_T = 0.f;

					if ( nodeA == newNodeA && nodeB == newNodeB )
					{
						ProgressNodesWeight( instance, weight, newWeight, timeDelta, GetInputRange( nodeA, nodeB ) );
					}
					else
					{
						ProgressNodesBoundedWeight( instance, weight, timeDelta, nodeA, nodeB, newNodeA, newNodeB, restTime );
					}

					if ( nodeA_T == newNodeA && nodeB_T == newNodeB )
					{
						ProgressNodesWeight( instance, weight_T, newWeight, timeDelta, GetInputRange( nodeA_T, nodeB_T ) );
					}
					else
					{
						ProgressNodesBoundedWeight( instance, weight_T, timeDelta, nodeA_T, nodeB_T, newNodeA, newNodeB, restTime_T );
					}

					progressCounter++;
					running = false;
				}

				break;
			}


		case IS_Update_Init:
			{
				ASSERT( nodeA_T == -1 );
				ASSERT( nodeB_T == -1 );
				ASSERT( transitionIdx == -1 );

				if ( m_useWeightDamp && m_weightDampMethod )
				{
					if ( nodeA == newNodeA && nodeB == newNodeB )
					{
						state = IS_UpdateDamped_InsideArea;
					}
					else if ( newNodeA == nodeA + 1 || newNodeA == nodeA - 1 )
					{
						state = IS_UpdateDamped_GoToNext;
					}
					else
					{
						if ( m_useTransitions )
						{
							state = IS_UpdateDamped_CheckTransition;
						}
						else
						{
							state = IS_UpdateDamped_GoStepByStep;
						}
					}
				}
				else
				{
					state = IS_Update_GoDirectly;
				}

				break;
			}

		case IS_Update_GoDirectly:
			{
				ASSERT( nodeA_T == -1 );
				ASSERT( nodeB_T == -1 );
				ASSERT( transitionIdx == -1 );

				nodeA = newNodeA;
				nodeB = newNodeB;
				weight = newWeight;

				progressCounter++;
				running = false;

				break;
			}


		case IS_UpdateDamped_InsideArea:
			{
				ASSERT( nodeA_T == -1 );
				ASSERT( nodeB_T == -1 );
				ASSERT( transitionIdx == -1 );
				ASSERT( nodeA == newNodeA && nodeB == newNodeB );

				ProgressNodesWeight( instance, weight, newWeight, timeDelta, GetInputRange( nodeA, nodeB ) );

				progressCounter++;
				running = false;

				break;
			}

		case IS_UpdateDamped_GoStepByStep:
			{
				ASSERT( nodeA_T == -1 );
				ASSERT( nodeB_T == -1 );
				ASSERT( transitionIdx == -1 );

				FindNearestBoundedWeightAndNodes( nodeA, nodeB, weight, newNodeA, newNodeB, newWeight );

				state = IS_UpdateDamped_GoToNext;

				break;
			}

		case IS_UpdateDamped_GoToNext:
			{
				ASSERT( nodeA_T == -1 );
				ASSERT( nodeB_T == -1 );
				ASSERT( transitionIdx == -1 );

				Bool goToPrev = newNodeA < nodeA;
				Bool goToNext = newNodeA > nodeA;
				Bool wrapWeight = false;

				if ( m_radialBlending )
				{
					ASSERT( newNodeA == nodeA + 1 || newNodeA == nodeA - 1 || ( newNodeA == 0 && nodeA == m_inputValues.SizeInt() - 2 ) || ( newNodeA == m_inputValues.SizeInt() - 2 && nodeA == 0 ) );
					ASSERT( newNodeB == nodeB + 1 || newNodeB == nodeB - 1 || ( newNodeB == 1 && nodeB == m_inputValues.SizeInt() - 1 ) || ( newNodeB == m_inputValues.SizeInt() - 1 && nodeB == 1 ) );

					if ( newNodeA == 0 && nodeA == m_inputValues.SizeInt() - 2 )
					{
						goToPrev = false;
						goToNext = true;
						wrapWeight = true;
					}
					else if ( newNodeA == m_inputValues.SizeInt() - 2 && nodeA == 0 )
					{
						goToPrev = true;
						goToNext = false;
						wrapWeight = true;
					}
				}
				else
				{
					ASSERT( newNodeA == nodeA + 1 || newNodeA == nodeA - 1 );
					ASSERT( newNodeB == nodeB + 1 || newNodeB == nodeB - 1 );
				}

				// Go one place, Don't use transitions for this case. Damp weight should be good enough.

				ASSERT( goToNext || goToPrev );

				if ( ( goToNext && weight >= 1.f ) || ( goToPrev && weight <= 0.f ) )
				{
					nodeA = newNodeA;
					nodeB = newNodeB;

					weight = goToNext ? 0.f : 1.f;

					FindActiveNodes( instance, controlValue, newNodeA, newNodeB, newWeight );

					state = IS_Update_Init;
				}
				else
				{
					Float restTime = 0.f;

					{
						Float boundedWeight = FindNearestBoundedWeight( nodeA, nodeB, newNodeA, newNodeB );
						if ( wrapWeight )
						{
							boundedWeight = 1.f - boundedWeight;
						}

						ASSERT( m_useWeightDamp && m_weightDampMethod );

						if ( m_useWeightDamp && m_weightDampMethod )
						{
							m_weightDampMethod->OnUpdate( instance, timeDelta, GetInputRange( nodeA, nodeB ), boundedWeight, weight, restTime );
						}
						else
						{
							weight = boundedWeight;
						}
					}

					if ( restTime > 0.f )
					{
						ASSERT( weight >= 1.f || weight <= 0.f );

						FindActiveNodes( instance, controlValue, newNodeA, newNodeB, newWeight );

						InternalLatentAction actionA;
						actionA.m_target = nodeA;
						actionA.m_extraDeltaTimeSet = true;
						actionA.m_extraDeltaTime = timeDelta-restTime;

						InternalLatentAction actionB;
						actionA.m_target = nodeB;
						actionA.m_extraDeltaTimeSet = true;
						actionA.m_extraDeltaTime = timeDelta-restTime;

						latentSyncs.PushBack( actionA );
						latentSyncs.PushBack( actionB );

						timeDelta = restTime;

						state = IS_Update_Init;
					}
					else
					{
						progressCounter++;
						running = false;
					}
				}

				break;
			}


		case IS_UpdateDamped_CheckTransition:
			{
				ASSERT( nodeA_T == -1 );
				ASSERT( nodeB_T == -1 );
				ASSERT( transitionIdx == -1 );

				const Int32 selectedTransitionIdx = FindActiveTransition( instance );
				if ( selectedTransitionIdx != -1 )
				{
					const Float duration = GetTransitionDuration( instance, selectedTransitionIdx );
					if ( duration > 0.f )
					{
						// Setup transition
						transitionWeight = 0.f;
						transitionIdx = selectedTransitionIdx;

						Int32& nodeA_T = instance[ i_nodeA_T ];
						Int32& nodeB_T = instance[ i_nodeB_T ];
						Float& weight_T = instance[ i_weight_T ];

						nodeA_T = newNodeA;
						nodeB_T = newNodeB;
						weight_T = newWeight;

						ResetTransitions( instance );

						CSyncInfo info;

						CBehaviorGraphNode* source = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
						if ( !source ) 
						{
							source = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
						}
						if ( source )
						{
							source->GetSyncInfo( instance, info );
						}

						InternalLatentAction actionA;
						actionA.m_target = nodeA_T;
						actionA.m_syncSet = true;
						actionA.m_sync = info;
						actionA.m_transIndex = transitionIdx;

						InternalLatentAction actionB;
						actionB.m_target = nodeB_T;
						actionB.m_syncSet = true;
						actionB.m_sync = info;
						actionB.m_transIndex = transitionIdx;

						latentSyncs.PushBack( actionA );
						latentSyncs.PushBack( actionB );

						// Go to transition state
						state = IS_UpdateTransition;
					}
					else
					{
						state = IS_Update_GoDirectly;
					}
				}
				else
				{
					state = IS_UpdateDamped_GoStepByStep;
				}
			}

			break;
		}
	}

	ProcessActivations( instance, nodeACached, nodeBCached, weightCached, nodeA, nodeB, weight, GetAnimationSyncMethod() );
	ProcessActivations( instance, nodeA_TCached, nodeB_TCached, weight_TCached, nodeA_T, nodeB_T, weight_T, GetAnimationSyncMethod() );

	if ( nodeA != -1 && nodeB != -1 && weight > 0.f && weight < 1.f )
	{
		CBehaviorGraphNode* nA = m_cachedInputNodes[ nodeA ];
		CBehaviorGraphNode* nB = m_cachedInputNodes[ nodeB ];

		if ( nA && nB )
		{
			CSyncInfo syncA, syncB;
			nA->GetSyncInfo( instance, syncA );
			nB->GetSyncInfo( instance, syncB );

			if ( syncA.m_currTime != syncA.m_currTime )
			{
				CSyncInfo info;
				CBehaviorGraphNode* target;

				if ( weight > 0.5f )
				{
					target = nA;
					info = syncB;
				}
				else
				{
					target = nB;
					info = syncA;
				}

				const IBehaviorSyncMethod* syncMethod = GetAnimationSyncMethod();
				if ( syncMethod )
				{
					syncMethod->SynchronizeTo( instance, target, info );
				}
				else
				{
					target->SynchronizeTo( instance, info );
				}
			}
		}
	}

	for ( Uint32 is=0; is<latentSyncs.Size(); ++is )
	{
		const InternalLatentAction& action = latentSyncs[ is ];
		if ( action.m_target == -1 )
		{
			continue;
		}

		if ( action.m_target == nodeA || action.m_target == nodeB || action.m_target == nodeA_T || action.m_target == nodeB_T )
		{
			CBehaviorGraphNode* nn = m_cachedInputNodes[ action.m_target ];
			if ( nn )
			{
				if ( action.m_extraDeltaTimeSet )
				{
					CSyncInfo sync;
					nn->GetSyncInfo( instance, sync );

					sync.m_currTime += action.m_extraDeltaTime;

					nn->SynchronizeTo( instance, sync );
				}
				
				if ( action.m_syncSet )
				{
					const IBehaviorSyncMethod* syncMethod = GetTransitionSyncMethod( instance, action.m_transIndex );
					if ( syncMethod )
					{
						syncMethod->SynchronizeTo( instance, nn, action.m_sync );
					}
				}
			}
		}
	}

	Synchronize( instance, timeDelta, nodeA, nodeB, weight );
	Synchronize( instance, timeDelta, nodeA_T, nodeB_T, weight_T );

	if ( nodeA != -1 && nodeB != -1 && weight > 0.f && weight < 1.f )
	{
		CBehaviorGraphNode* nA = m_cachedInputNodes[ nodeA ];
		CBehaviorGraphNode* nB = m_cachedInputNodes[ nodeB ];

		if ( nA && nB )
		{
			CSyncInfo syncA, syncB;
			nA->GetSyncInfo( instance, syncA );
			nB->GetSyncInfo( instance, syncB );

			if ( syncA.m_currTime != syncA.m_currTime )
			{
				CSyncInfo info;
				CBehaviorGraphNode* target;

				if ( weight > 0.5f )
				{
					target = nA;
					info = syncB;
				}
				else
				{
					target = nB;
					info = syncA;
				}

				const IBehaviorSyncMethod* syncMethod = GetAnimationSyncMethod();
				if ( syncMethod )
				{
					syncMethod->SynchronizeTo( instance, target, info );
				}
				else
				{
					target->SynchronizeTo( instance, info );
				}
			}
		}
	}

	InternalUpdateTwoNodes( nodeA, nodeB, weight, context, instance, timeDelta );
	InternalUpdateTwoNodes( nodeA_T, nodeB_T, weight_T, context, instance, timeDelta );

	if ( nodeA != -1 && nodeB != -1 && weight > 0.f && weight < 1.f )
	{
		CBehaviorGraphNode* nA = m_cachedInputNodes[ nodeA ];
		CBehaviorGraphNode* nB = m_cachedInputNodes[ nodeB ];

		if ( nA && nB )
		{
			CSyncInfo syncA, syncB;
			nA->GetSyncInfo( instance, syncA );
			nB->GetSyncInfo( instance, syncB );

			if ( syncA.m_currTime != syncA.m_currTime )
			{
				CSyncInfo info;
				CBehaviorGraphNode* target;

				if ( weight > 0.5f )
				{
					target = nA;
					info = syncB;
				}
				else
				{
					target = nB;
					info = syncA;
				}

				const IBehaviorSyncMethod* syncMethod = GetAnimationSyncMethod();
				if ( syncMethod )
				{
					syncMethod->SynchronizeTo( instance, target, info );
				}
				else
				{
					target->SynchronizeTo( instance, info );
				}
			}
		}
	}

	ASSERT( progressCounter == 1 );
}

void CBehaviorGraphBlendMultipleCondNode::InternalUpdateTwoNodes( Int32 nodeA, Int32 nodeB, Float weight, SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	//BEH_LOG( TXT("%d - %d => %1.3f"), nodeA, nodeB, weight );

	ASSERT( weight >= 0.f && weight <= 1.f );

	if ( !IsSecondInputActive( weight ) )
	{
		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInput )
		{
			ASSERT( firstInput->IsActive( instance ) );
			firstInput->Update( context, instance, timeDelta );
		}
	}
	else if ( !IsFirstInputActive( weight ) )
	{
		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInput )
		{
			ASSERT( secondInput->IsActive( instance ) );
			secondInput->Update( context, instance, timeDelta );
		}
	}
	else
	{
		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInput ) 
		{
			ASSERT( firstInput->IsActive( instance ) );
			firstInput->Update( context, instance, timeDelta );
		}

		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInput )
		{
			ASSERT( secondInput->IsActive( instance ) );
			secondInput->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::InternalSampleTwoNodes( Int32 nodeA, Int32 nodeB, Float weight, SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	ASSERT( weight >= 0.f && weight <= 1.f );

	if ( !IsSecondInputActive( weight ) )
	{
		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInput ) 
		{
			firstInput->Sample( context, instance, output );
		}
	}
	else if ( !IsFirstInputActive( weight ) )
	{
		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInput )
		{
			secondInput->Sample( context, instance, output );
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
			CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
			if ( firstInput ) 
			{
				firstInput->Sample( context, instance, *temp1 );
			}

			CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
			if ( secondInput )
			{
				secondInput->Sample( context, instance, *temp2 );
			}

#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				// Interpolate poses
				output.SetInterpolate( *temp1, *temp2, weight );
			}
			else
			{
				// Interpolate poses
				output.SetInterpolateME( *temp1, *temp2, weight );
			}
#else
			output.SetInterpolate( *temp1, *temp2, weight );
#endif

			// Merge events and used anims
			output.MergeEventsAndUsedAnims( *temp1, *temp2, 1.0f - weight, weight );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( BlendMultiConds );

	if ( m_inputValues.Empty() )
	{
		return;
	}

	const Int32 nodeA = instance[ i_nodeA ];
	const Int32 nodeB = instance[ i_nodeB ];

	const Float weight = instance[ i_weight ];

	if ( IsTransitionInProgress( instance ) )
	{
		CCacheBehaviorGraphOutput cachePose1( context );
		CCacheBehaviorGraphOutput cachePose2( context );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			const Int32 nodeA_T = instance[ i_nodeA_T ];
			const Int32 nodeB_T = instance[ i_nodeB_T ];

			const Float weight_T = instance[ i_weight_T ];

			InternalSampleTwoNodes( nodeA, nodeB, weight, context, instance, *temp1 );
			InternalSampleTwoNodes( nodeA_T, nodeB_T, weight_T, context, instance, *temp2 );
			
			const Float transitionWeight = instance[ i_transitionWeight ];

#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				output.SetInterpolate( *temp1, *temp2, transitionWeight );
			}
			else
			{
				output.SetInterpolateME( *temp1, *temp2, transitionWeight );
			}
#else
			output.SetInterpolate( *temp1, *temp2, transitionWeight );
#endif

			if ( DoesTransitionBlockEvents( instance ) )
			{
				output.MergeEvents( *temp1, 1.0f - transitionWeight );
			}
			else
			{
				output.MergeEvents( *temp1, *temp2, 1.0f - transitionWeight, transitionWeight );
			}
			output.MergeUsedAnims( *temp1, *temp2, 1.0f - transitionWeight, transitionWeight );
		}
	}
	else
	{
		InternalSampleTwoNodes( nodeA, nodeB, weight, context, instance, output );
	}

	PostSampleTransitions( instance, output );
}

void CBehaviorGraphBlendMultipleCondNode::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta, Int32 nodeA, Int32 nodeB, Float weight ) const
{
	if ( m_inputValues.Empty() )
	{
		return;
	}

	if ( !IsSecondInputActive( weight ) )
	{
		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInput ) 
		{
			firstInput->GetSyncData( instance ).Reset();
		}			
	}
	else if ( !IsFirstInputActive( weight ) )
	{
		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInput )
		{
			secondInput->GetSyncData( instance ).Reset();
		}			
	}
	else
	{
		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;

		if ( m_synchronizeAnimations && m_syncMethodAnimation )
		{
			if ( firstInput && secondInput )
			{
				m_syncMethodAnimation->Synchronize( instance, firstInput, secondInput, weight, timeDelta );	
			}
		}
		else
		{
			if ( firstInput )
			{
				firstInput->GetSyncData( instance ).Reset();
			}

			if ( secondInput )
			{
				secondInput->GetSyncData( instance ).Reset();
			}
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::SynchronizeTransition( CBehaviorGraphInstance& instance, Int32 firstIndexA, Int32 secondIndexA, Int32 firstIndexB, Int32 secondIndexB, const IBehaviorSyncMethod* syncMethod ) const
{
	CSyncInfo info;

	CBehaviorGraphNode* source = firstIndexA != -1 ? m_cachedInputNodes[ firstIndexA ] : NULL;
	if ( !source ) 
	{
		source = secondIndexA != -1 ? m_cachedInputNodes[ secondIndexA ] : NULL;
	}

	if ( source )
	{
		source->GetSyncInfo( instance, info );
	}

	CBehaviorGraphNode* destA = firstIndexB != -1 ? m_cachedInputNodes[ firstIndexB ] : NULL;
	CBehaviorGraphNode* destB = secondIndexB != -1 ? m_cachedInputNodes[ secondIndexB ] : NULL;

	if ( destA )
	{
		destA->GetSyncData( instance ).Reset();

		if ( syncMethod )
		{
			syncMethod->SynchronizeTo( instance, destA, info );
		}
	}

	if ( destB )
	{
		destB->GetSyncData( instance ).Reset();

		if ( syncMethod )
		{
			syncMethod->SynchronizeTo( instance, destB, info );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_inputValues.Empty() )
	{
		return;
	}

	const Int32 nodeA = instance[ i_nodeA ];
	const Int32 nodeB = instance[ i_nodeB ];

	const Float weight = instance[ i_weight ];

	if ( !IsSecondInputActive( weight ) )
	{
		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInput ) 
		{
			firstInput->GetSyncInfo( instance, info );
		}
	}
	else if ( !IsFirstInputActive( weight ) )
	{
		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInput )
		{
			secondInput->GetSyncInfo( instance, info );
		}
	}
	else
	{
		CSyncInfo firstSyncInfo;
		CSyncInfo secondSyncInfo;

		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInput ) 
		{
			firstInput->GetSyncInfo( instance, firstSyncInfo );
		}

		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInput )
		{
			secondInput->GetSyncInfo( instance, secondSyncInfo );
		}

		info.SetInterpolate( firstSyncInfo, secondSyncInfo, weight );
	}
}

void CBehaviorGraphBlendMultipleCondNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_inputValues.Empty() )
	{
		return;
	}

	const Int32 nodeA = instance[ i_nodeA ];
	const Int32 nodeB = instance[ i_nodeB ];

	CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
	CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;

	if ( m_syncMethodAnimation && m_synchronizeAnimations )
	{
		if ( firstInput ) 
		{
			m_syncMethodAnimation->SynchronizeTo( instance, firstInput, info );
		}

		if ( secondInput )
		{
			m_syncMethodAnimation->SynchronizeTo( instance, secondInput, info );
		}
	}
	else
	{
		if ( firstInput )
		{
			firstInput->SynchronizeTo( instance, info );
		}

		if ( secondInput )
		{
			secondInput->SynchronizeTo( instance, info );
		}
	}
}


void CBehaviorGraphBlendMultipleCondNode::ProcessActivations( CBehaviorGraphInstance& instance, Int32 prevFirstIndex, Int32 prevSecondIndex, Float prevWeight, Int32 currFirstIndex, Int32 currSecondIndex, Float weight, const IBehaviorSyncMethod* syncMethod ) const
{	
	const Bool prevFirstActive = IsFirstInputActive( prevWeight );
	const Bool prevSecondActive = IsSecondInputActive( prevWeight );

	const Bool currFirstActive = IsFirstInputActive( weight );
	const Bool currSecondActive = IsSecondInputActive( weight );

	// handle deactivations
	if ( prevFirstActive && prevFirstIndex != -1 )
	{
		if ( !( ( currFirstActive && ( prevFirstIndex == currFirstIndex ) ) ||
			( currSecondActive && ( prevFirstIndex == currSecondIndex ) ) ) )
		{
			CBehaviorGraphNode *input = m_cachedInputNodes[ prevFirstIndex ];
			if ( input )
			{
				input->Deactivate( instance );
			}
		}
	}

	if ( prevSecondActive && prevSecondIndex != -1 )
	{
		if ( !( ( currFirstActive && ( prevSecondIndex == currFirstIndex ) ) ||
			( currSecondActive && ( prevSecondIndex == currSecondIndex ) ) ) )
		{
			CBehaviorGraphNode *input = m_cachedInputNodes[ prevSecondIndex ];
			if ( input )
			{
				input->Deactivate( instance );
			}
		}
	}

	// find something we can synchronize to (any node active in previous frame will do (at least should ;-))	
	CBehaviorGraphNode *syncSource = prevFirstIndex != -1 ? m_cachedInputNodes[ prevFirstIndex ] : NULL;
	if ( !prevFirstActive || !syncSource )
	{
		syncSource = NULL;
		if ( prevSecondActive && prevSecondIndex != -1 )
		{
			syncSource = m_cachedInputNodes[ prevSecondIndex ];
		}
	}

	// handle activations
	if ( currFirstActive && currFirstIndex != -1 )
	{
		if ( !( ( prevFirstActive && ( currFirstIndex == prevFirstIndex ) ) ||
			( prevSecondActive && ( currFirstIndex == prevSecondIndex ) ) ) )
		{
			CBehaviorGraphNode *input = m_cachedInputNodes[ currFirstIndex ];
			if ( input )
			{
				input->Activate( instance );

				if ( syncSource && syncMethod )
				{
					CSyncInfo syncInfo;
					syncSource->GetSyncInfo( instance, syncInfo );
					syncMethod->SynchronizeTo( instance, input, syncInfo );
				}
			}
		}
	}

	if ( currSecondActive && currSecondIndex != -1 )
	{
		if ( !( ( prevFirstActive && ( currSecondIndex == prevFirstIndex ) ) ||
			( prevSecondActive && ( currSecondIndex == prevSecondIndex ) ) ) )
		{
			CBehaviorGraphNode *input = m_cachedInputNodes[ currSecondIndex ];
			if ( input )
			{
				input->Activate( instance );

				if ( syncSource && syncMethod )
				{
					CSyncInfo syncInfo;
					syncSource->GetSyncInfo( instance, syncInfo );
					syncMethod->SynchronizeTo( instance, input, syncInfo );
				}
			}
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::AddInput()
{
	m_inputValues.PushBack( 0.0f );

	OnInputListChange();
}

void CBehaviorGraphBlendMultipleCondNode::RemoveInput( Uint32 index )
{
	ASSERT( index < m_inputValues.Size() );

	m_inputValues.Erase( m_inputValues.Begin() + index );

	OnInputListChange();
}

Bool CBehaviorGraphBlendMultipleCondNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_inputValues.Empty() )
	{
		return false;
	}

	const Int32 nodeA = instance[ i_nodeA ];
	const Int32 nodeB = instance[ i_nodeB ];

	Bool retVal = false;

	CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
	if ( firstInput && firstInput->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
	if ( secondInput && secondInput->ProcessEvent( instance, event ) )
	{
		retVal = true;
	}

	return retVal;
}

void CBehaviorGraphBlendMultipleCondNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );

		{
			Float& controlValue = instance[ i_controlValue ];
			controlValue = m_cachedControlValueNode->GetValue( instance );

			Int32& nodeA = instance[ i_nodeA ];
			Int32& nodeB = instance[ i_nodeB ];

			Float& weight = instance[ i_weight ];

			{
				const Int32 prevNodeA = nodeA;
				const Int32 prevNodeB = nodeB;

				const Float prevWeight = weight;

				FindActiveNodes( instance, controlValue, nodeA, nodeB, weight );

				ProcessActivations( instance, prevNodeA, prevNodeB, prevWeight, nodeA, nodeB, weight, GetAnimationSyncMethod() );
			}
		}
	}

	if ( m_weightDampMethod )
	{
		m_weightDampMethod->OnActivated( instance );
	}

	if ( m_controlValueDampMethod )
	{
		m_controlValueDampMethod->OnActivated( instance );
	}

	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->OnActivated( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	Uint32 numInputs = GetNumInputs();
	for( Uint32 i=0; i<numInputs; ++i )
	{
		CBehaviorGraphNode *input = m_cachedInputNodes[ i ];
		if ( input )
		{
			input->Deactivate( instance );
		}
	}

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}

	if ( m_weightDampMethod )
	{
		m_weightDampMethod->OnDeactivated( instance );
	}

	if ( m_controlValueDampMethod )
	{
		m_controlValueDampMethod->OnDeactivated( instance );
	}

	const Uint32 tSize = m_transitions.Size();
	for ( Uint32 i=0; i<tSize; ++i )
	{
		CBehaviorGraphBlendMultipleCondNode_Transition* t = m_transitions[ i ];
		if ( t )
		{
			t->OnDeactivated( instance );
		}
	}

	InternalReset( instance );
}

Bool CBehaviorGraphBlendMultipleCondNode::IsFirstInputActive( Float var ) const
{
	ASSERT( var >= 0.f && var <= 1.f );
	return var < (1.0f - ACTIVATION_THRESHOLD);
}

Bool CBehaviorGraphBlendMultipleCondNode::IsSecondInputActive( Float var ) const
{
	ASSERT( var >= 0.f && var <= 1.f );
	return var > ACTIVATION_THRESHOLD;
}

void CBehaviorGraphBlendMultipleCondNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_inputValues.Empty() )
	{
		return;
	}

	const Int32 nodeA = instance[ i_nodeA ];
	const Int32 nodeB = instance[ i_nodeB ];

	const Float weight = instance[ i_weight ];

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( IsTransitionInProgress( instance ) )
	{
		const Int32 nodeA_T = instance[ i_nodeA_T ];
		const Int32 nodeB_T = instance[ i_nodeB_T ];

		const Float weight_T = instance[ i_weight_T ];

		const Float transitionWeight = instance[ i_transitionWeight ];

		CBehaviorGraphNode *firstInputA = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInputA && IsFirstInputActive( weight ) )
		{
			firstInputA->ProcessActivationAlpha( instance, ( 1.f - transitionWeight ) * ( 1.0f - weight ) * alpha );
		}

		CBehaviorGraphNode *secondInputA = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInputA && IsSecondInputActive( weight ) )
		{
			secondInputA->ProcessActivationAlpha( instance, ( 1.f - transitionWeight ) * weight * alpha );
		}

		CBehaviorGraphNode *firstInputB = nodeA_T != -1 ? m_cachedInputNodes[ nodeA_T ] : NULL;
		if ( firstInputB && IsFirstInputActive( weight_T ) )
		{
			firstInputB->ProcessActivationAlpha( instance, transitionWeight * ( 1.0f - weight_T ) * alpha );
		}

		CBehaviorGraphNode *secondInputB = nodeB_T != -1 ? m_cachedInputNodes[ nodeB_T ] : NULL;
		if ( secondInputB && IsSecondInputActive( weight_T ) )
		{
			secondInputB->ProcessActivationAlpha( instance, transitionWeight * weight_T * alpha );
		}
	}
	else
	{
		CBehaviorGraphNode *firstInput = nodeA != -1 ? m_cachedInputNodes[ nodeA ] : NULL;
		if ( firstInput && IsFirstInputActive( weight ) )
		{
			firstInput->ProcessActivationAlpha( instance, ( 1.0f - weight ) * alpha );
		}

		CBehaviorGraphNode *secondInput = nodeB != -1 ? m_cachedInputNodes[ nodeB ] : NULL;
		if ( secondInput && IsSecondInputActive( weight ) )
		{
			secondInput->ProcessActivationAlpha( instance, weight * alpha );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehaviorGraphBlendMultipleCondNode_ConstDampMethod::CBehaviorGraphBlendMultipleCondNode_ConstDampMethod()
	: m_speed( 1.f )
{

}

void CBehaviorGraphBlendMultipleCondNode_ConstDampMethod::OnUpdate( CBehaviorGraphInstance& instance, Float timeDelta, Float inputRange, Float destValue, Float& currValue, Float& restTime ) const
{
	const Float cv = currValue;
	Float rt = 0.f;

	restTime = 0.f;

	const Float finalSpeed = inputRange > 0.f ? m_speed / inputRange : m_speed;

	if ( cv < destValue )
	{
		const Float newValue = cv + timeDelta * finalSpeed;
		if ( newValue > destValue )
		{
			currValue = destValue;
			rt = m_speed > 0.f ? ( newValue - destValue ) / finalSpeed : 0.f;
		}
		else
		{
			currValue = newValue;
			rt = 0.f;
		}
	}
	else if ( cv > destValue )
	{
		const Float newValue = cv - timeDelta * finalSpeed;
		if ( newValue < destValue )
		{
			currValue = destValue;
			rt = m_speed > 0.f ? ( destValue - newValue ) / finalSpeed : 0.f;
		}
		else
		{
			currValue = newValue;
			rt = 0.f;
		}
	}

	ASSERT( rt <= timeDelta );

	restTime = Clamp( rt, 0.f, timeDelta );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehaviorGraphBlendMultipleCondNode_Transition::CBehaviorGraphBlendMultipleCondNode_Transition()
	: m_transitionDuration( 0.2f )
	, m_synchronize( true )
	, m_blockEvents( false )
	, m_isEnabled( true )
{

}

void CBehaviorGraphBlendMultipleCondNode_Transition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	if ( m_condition )
	{
		m_condition->OnBuildDataLayout( compiler );
	}
}

void CBehaviorGraphBlendMultipleCondNode_Transition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	if ( m_condition )
	{
		m_condition->OnInitInstance( instance );
	}
}

void CBehaviorGraphBlendMultipleCondNode_Transition::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	if ( m_condition )
	{
		m_condition->OnReleaseInstance( instance );
	}
}

void CBehaviorGraphBlendMultipleCondNode_Transition::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_condition )
	{
		m_condition->OnActivated( instance );
	}
}

void CBehaviorGraphBlendMultipleCondNode_Transition::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_condition )
	{
		m_condition->OnDeactivated( instance );
	}
}

void CBehaviorGraphBlendMultipleCondNode_Transition::Reset( CBehaviorGraphInstance& instance ) const
{
	if ( m_condition )
	{
		m_condition->Reset( instance );
	}
}

void CBehaviorGraphBlendMultipleCondNode_Transition::PreUpdate( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_condition )
	{
		m_condition->PreUpdate( instance, timeDelta );
	}
}

void CBehaviorGraphBlendMultipleCondNode_Transition::PostSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
	if ( m_condition )
	{
		m_condition->PostSampled( instance, pose );
	}
}

Bool CBehaviorGraphBlendMultipleCondNode_Transition::Check( CBehaviorGraphInstance& instance ) const
{
	return m_condition && m_isEnabled ? m_condition->Check( instance ) : false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CBehaviorGraphBlendMultipleCondNode_AnimEvent::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_eventOccured;
}

void CBehaviorGraphBlendMultipleCondNode_AnimEvent::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CBehaviorGraphBlendMultipleCondNode_AnimEvent::OnActivated( CBehaviorGraphInstance& instance ) const
{
	Reset( instance );
}

void CBehaviorGraphBlendMultipleCondNode_AnimEvent::Reset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_eventOccured ] = false;
}

void CBehaviorGraphBlendMultipleCondNode_AnimEvent::PostSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
	Bool& eventOccured = instance[ i_eventOccured ];

	eventOccured = false;

	const Uint32 size = pose.m_numEventsFired;
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( pose.m_eventsFired[ i ].m_extEvent->GetEventName() == m_animEventName )
		{
			eventOccured = true;
		}
	}
}

Bool CBehaviorGraphBlendMultipleCondNode_AnimEvent::Check( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_eventOccured ];
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphBlendMultipleCondNode_Multi::CBehaviorGraphBlendMultipleCondNode_Multi()
	: m_logicAndOr( true )
{

}

void CBehaviorGraphBlendMultipleCondNode_Multi::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->OnBuildDataLayout( compiler );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode_Multi::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->OnInitInstance( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode_Multi::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->OnReleaseInstance( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode_Multi::OnActivated( CBehaviorGraphInstance& instance ) const
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->OnActivated( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode_Multi::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->OnDeactivated( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode_Multi::Reset( CBehaviorGraphInstance& instance ) const
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->Reset( instance );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode_Multi::PreUpdate( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->PreUpdate( instance, timeDelta );
		}
	}
}

void CBehaviorGraphBlendMultipleCondNode_Multi::PostSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
	const Uint32 size = m_conditions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
		if ( c )
		{
			c->PostSampled( instance, pose );
		}
	}
}

Bool CBehaviorGraphBlendMultipleCondNode_Multi::Check( CBehaviorGraphInstance& instance ) const
{
	if ( m_logicAndOr )
	{
		const Uint32 size = m_conditions.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
			if ( c && !c->Check( instance ) )
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		const Uint32 size = m_conditions.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const IBehaviorGraphBlendMultipleCondNode_Condition* c = m_conditions[ i ];
			if ( c && c->Check( instance ) )
			{
				return true;
			}
		}

		return false;
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
