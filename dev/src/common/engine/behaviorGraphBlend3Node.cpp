
#include "build.h"
#include "behaviorGraphBlend3Node.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/mathUtils.h"
#include "animSyncInfo.h"
#include "behaviorGraphNode.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlend3Node );

const Float CBehaviorGraphBlend3Node::ACTIVATION_THRESHOLD = 0.001f;

CBehaviorGraphBlend3Node::CBehaviorGraphBlend3Node()
	: m_synchronize( true )
	, m_syncMethod( NULL )
	, m_useCustomSpace( false )
	, m_takeEventsFromMostImportantInput( false )
	, m_A( 1.f, 0.f, 0.f )
	, m_B( 0.f, 1.f, 0.f )
	, m_C( 0.f, 0.f, 0.f )
	, m_D( 1.f, 1.f, 0.f )
{
}

void CBehaviorGraphBlend3Node::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlValue_A;
	compiler << i_prevControlValue_A;
	compiler << i_controlValue_B;
	compiler << i_prevControlValue_B;
}

void CBehaviorGraphBlend3Node::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_controlValue_A ] = 0.f;
	instance[ i_prevControlValue_A ] = 0.f;
	instance[ i_controlValue_B ] = 0.f;
	instance[ i_prevControlValue_B ] = 0.f;
}

void CBehaviorGraphBlend3Node::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue_A );
	INST_PROP( i_prevControlValue_A );
	INST_PROP( i_controlValue_B );
	INST_PROP( i_prevControlValue_B );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlend3Node::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( A ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( B ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( BaseC ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( BaseD ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( WeightA ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( WeightB ) ) );
}

#endif

void CBehaviorGraphBlend3Node::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputNode_A = CacheBlock( TXT("A") );
	m_cachedInputNode_B = CacheBlock( TXT("B") );
	m_cachedInputNode_C = CacheBlock( TXT("BaseC") );
	m_cachedInputNode_D = CacheBlock( TXT("BaseD") );

	m_cachedControlVariableNode_A = CacheValueBlock( TXT("WeightA") );
	m_cachedControlVariableNode_B = CacheValueBlock( TXT("WeightB") );
}

void CBehaviorGraphBlend3Node::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Blend3 );

	// Update variables
	if ( m_cachedControlVariableNode_A )
	{
		m_cachedControlVariableNode_A->Update( context, instance, timeDelta );
	}
	if ( m_cachedControlVariableNode_B )
	{
		m_cachedControlVariableNode_B->Update( context, instance, timeDelta );
	}

	// Copy variable value (so it's constant across update and sample)
	UpdateControlValues( instance );

	// Process activations
	ProcessActivations( instance );

	// Synchronize children playback
	Synchronize( instance, timeDelta );

	// Update appropriate child nodes
	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	Float weights[ 4 ];
	GetWeights4( instance, weights );

	for ( Uint32 i=0; i<4; ++i )
	{
		const CBehaviorGraphNode* node = nodes[ i ];
		if ( node && IsInputActive( weights[ i ] ) )
		{
			ASSERT( node->IsActive( instance ) );

			node->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphBlend3Node::UpdateControlValues( CBehaviorGraphInstance& instance ) const
{
	if ( m_useCustomSpace )
	{
		instance[ i_prevControlValue_A ]	= instance[ i_controlValue_A ];
		instance[ i_controlValue_A ]		= m_cachedControlVariableNode_A ? m_cachedControlVariableNode_A->GetValue( instance ) : 0.0f;

		instance[ i_prevControlValue_B ]	= instance[ i_controlValue_B ];
		instance[ i_controlValue_B ]		= m_cachedControlVariableNode_B ? m_cachedControlVariableNode_B->GetValue( instance ) : 0.0f;
	}
	else
	{
		instance[ i_prevControlValue_A ]	= instance[ i_controlValue_A ];
		instance[ i_controlValue_A ]		= m_cachedControlVariableNode_A ? Clamp( m_cachedControlVariableNode_A->GetValue( instance ), 0.f, 1.0f ) : 0.0f;

		instance[ i_prevControlValue_B ]	= instance[ i_controlValue_B ];
		instance[ i_controlValue_B ]		= m_cachedControlVariableNode_B ? Clamp( m_cachedControlVariableNode_B->GetValue( instance ), 0.f, 1.0f ) : 0.0f;
	}
}

void CBehaviorGraphBlend3Node::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( Blend3 );

	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	Float weights[ 4 ];
	GetWeights4( instance, weights );

	Bool active[ 4 ];
	active[ 0 ] = IsInputActive( weights[ 0 ] );
	active[ 1 ] = IsInputActive( weights[ 1 ] );
	active[ 2 ] = IsInputActive( weights[ 2 ] );
	active[ 3 ] = IsInputActive( weights[ 3 ] );

	const CBehaviorGraphNode* nodeA = NULL;
	const CBehaviorGraphNode* nodeB = NULL;
	const CBehaviorGraphNode* nodeC = NULL;

	Float weight = 0.f;
	Float weight2 = 0.f;
	Float weight3 = 0.f;

	if ( active[ 2 ] )
	{
		ASSERT( !active[ 3 ] );
	}
	if ( active[ 3 ] )
	{
		ASSERT( !active[ 2 ] );
	}

	if ( active[ 0 ] && active[ 1 ] && active[ 2 ] )
	{
		nodeA = nodes[ 0 ];
		nodeB = nodes[ 1 ];
		nodeC = nodes[ 2 ];

		weight = weights[ 0 ];
		weight2 = weights[ 1 ];
		weight3 = weights[ 2 ];

		ASSERT( MAbs( weight + weight2 + weight3 - 1.f ) < 0.01f );
	}
	else if ( active[ 0 ] && active[ 1 ] && active[ 3 ] )
	{
		nodeA = nodes[ 0 ];
		nodeB = nodes[ 1 ];
		nodeC = nodes[ 3 ];

		weight = weights[ 0 ];
		weight2 = weights[ 1 ];
		weight3 = weights[ 3 ];

		ASSERT( MAbs( weight + weight2 + weight3 - 1.f ) < 0.01f );
	}
	else if ( active[ 0 ] && active[ 1 ] )
	{
		nodeA = nodes[ 0 ];
		nodeB = nodes[ 1 ];

		weight = weights[ 1 ];

		ASSERT( MAbs( weights[ 0 ] + weights[ 1 ] - 1.f ) < 0.01f );
	}
	else if ( active[ 0 ] && active[ 2 ] )
	{
		nodeA = nodes[ 0 ];
		nodeB = nodes[ 2 ];

		weight = weights[ 2 ];

		ASSERT( MAbs( weights[ 0 ] + weights[ 2 ] - 1.f ) < 0.01f );
	}
	else if ( active[ 0 ] && active[ 3 ] )
	{
		nodeA = nodes[ 0 ];
		nodeB = nodes[ 3 ];

		weight = weights[ 3 ];

		ASSERT( MAbs( weights[ 0 ] + weights[ 3 ] - 1.f ) < 0.01f );
	}
	else if ( active[ 1 ] && active[ 2 ] )
	{
		nodeA = nodes[ 1 ];
		nodeB = nodes[ 2 ];

		weight = weights[ 2 ];

		ASSERT( MAbs( weights[ 1 ] + weights[ 2 ] - 1.f ) < 0.01f );
	}
	else if ( active[ 1 ] && active[ 3 ] )
	{
		nodeA = nodes[ 1 ];
		nodeB = nodes[ 3 ];

		weight = weights[ 3 ];

		ASSERT( MAbs( weights[ 1 ] + weights[ 3 ] - 1.f ) < 0.01f );
	}
	else if ( active[ 0 ] )
	{
		nodeA = nodes[ 0 ];
	}
	else if ( active[ 1 ] )
	{
		nodeA = nodes[ 1 ];
	}
	else if ( active[ 2 ] )
	{
		nodeA = nodes[ 2 ];
	}
	else if ( active[ 3 ] )
	{
		nodeA = nodes[ 3 ];
	}

	ASSERT( weight >= 0.f && weight <= 1.f );

	if ( nodeA && nodeB && nodeC )
	{
		ASSERT( weight2 >= 0.f && weight2 <= 1.f );
		ASSERT( weight3 >= 0.f && weight3 <= 1.f );

		CCacheBehaviorGraphOutput cachePose1( context );
		CCacheBehaviorGraphOutput cachePose2( context );
		CCacheBehaviorGraphOutput cachePose3( context );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();
		SBehaviorGraphOutput* temp3 = cachePose3.GetPose();

		if ( temp1 && temp2 && temp3 )
		{
			nodeA->Sample( context, instance, *temp1 );
			nodeB->Sample( context, instance, *temp2 );
			nodeC->Sample( context, instance, *temp3 );

			ASSERT( MAbs( weight + weight2 + weight3 - 1.f ) < 0.01f );

			BehaviorUtils::BlendingUtils::SetPoseZero( output );

			BehaviorUtils::BlendingUtils::BlendPosesNormal( output, *temp1, weight );
			BehaviorUtils::BlendingUtils::BlendPosesNormal( output, *temp2, weight2 );
			BehaviorUtils::BlendingUtils::BlendPosesNormal( output, *temp3, weight3 );

			BehaviorUtils::BlendingUtils::RenormalizePose( output, 1.f );

			if ( m_takeEventsFromMostImportantInput )
			{
				SBehaviorGraphOutput* outputToMerge = ( weight > weight2 && weight > weight3 ) ? temp1 :
																		 ( weight2 > weight3 ) ? temp2 : temp3;
				output.MergeEvents( *outputToMerge, 1.0f );
			}
			else
			{
				output.MergeEventsAndUsedAnims( *temp1, weight );
				output.MergeEventsAndUsedAnims( *temp2, weight2 );
				output.MergeEventsAndUsedAnims( *temp3, weight3 );
			}
		}
	}
	else if ( nodeA && nodeB )
	{
		CCacheBehaviorGraphOutput cachePose1( context );
		CCacheBehaviorGraphOutput cachePose2( context );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			nodeA->Sample( context, instance, *temp1 );
			nodeB->Sample( context, instance, *temp2 );

#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				output.SetInterpolate( *temp1, *temp2, weight );
			}
			else
			{
				output.SetInterpolateME( *temp1, *temp2, weight );
			}
#else
			output.SetInterpolate( *temp1, *temp2, weight );
#endif

			// Merge events and used anims
			if ( m_takeEventsFromMostImportantInput )
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
	else if ( nodeA )
	{
		nodeA->Sample( context, instance, output );
	}
}

void CBehaviorGraphBlend3Node::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const
{	
	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	for ( Uint32 i=0; i<4; ++i )
	{
		if ( nodes[ i ] )
		{
			nodes[ i ]->GetSyncData( instance ).Reset();
		}
	}

	if ( m_synchronize && m_syncMethod )
	{
		const Int32 best = GetBestMainNodeIndex( instance );

		const CBehaviorGraphNode* bestNode = nodes[ best ];
		if ( bestNode )
		{
			Float weights[ 4 ];
			GetWeights4( instance, weights );

			const Int32 nextA = ( best + 1 ) % 4;
			const Int32 nextB = ( best + 2 ) % 4;
			const Int32 nextC = ( best + 3 ) % 4;

			ASSERT( nextA >= 0 && nextA <= 3 );
			ASSERT( nextB >= 0 && nextB <= 3 );
			ASSERT( nextC >= 0 && nextC <= 3 );

			if ( nodes[ nextA ] && IsInputActive( weights[ nextA ] ) )
			{
				m_syncMethod->Synchronize( instance, bestNode, nodes[ nextA ], weights[ nextA ], timeDelta );
			}
			if ( nodes[ nextB ] && IsInputActive( weights[ nextB ] ) )
			{
				m_syncMethod->Synchronize( instance, bestNode, nodes[ nextB ], weights[ nextB ], timeDelta );
			}
			if ( nodes[ nextC ] && IsInputActive( weights[ nextC ] ) )
			{
				m_syncMethod->Synchronize( instance, bestNode, nodes[ nextC ], weights[ nextC ], timeDelta );
			}
		}
	}
}

void CBehaviorGraphBlend3Node::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	const CBehaviorGraphNode* bestNode = GetBestMainNode( instance );
	if ( bestNode )
	{
		bestNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphBlend3Node::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	if ( m_synchronize && m_syncMethod )
	{		
		for ( Uint32 i=0; i<4; ++i )
		{
			if ( nodes[ i ] )
			{
				m_syncMethod->SynchronizeTo( instance, nodes[ i ], info );
			}
		}
	}
	else
	{
		for ( Uint32 i=0; i<4; ++i )
		{
			if ( nodes[ i ] )
			{
				nodes[ i ]->SynchronizeTo( instance, info );
			}
		}
	}
}

void CBehaviorGraphBlend3Node::ProcessActivations( CBehaviorGraphInstance& instance ) const
{
	Float currWeights[ 4 ];
	GetWeights4( instance, currWeights );

	Float prevWeights[ 4 ];
	GetPrevWeights4( instance, prevWeights );

	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	const Int32 bestPrevNodeIdx = GetBestPrevMainNodeIndex( instance );
	const Int32 bestCurrNodeIdx = GetBestMainNodeIndex( instance );

	Bool act[ 4 ];
	Bool deact[ 4 ];

	for ( Uint32 i=0; i<4; ++i )
	{
		const CBehaviorGraphNode* node = nodes[ i ];

		const Float prevWeight = prevWeights[ i ];
		const Float currWeight = currWeights[ i ];

		if ( node && !IsInputActive( prevWeight ) && IsInputActive( currWeight ) )
		{
			act[ i ] = true;
			deact[ i ] = false;
		}
		else if ( node && IsInputActive( prevWeight ) && !IsInputActive( currWeight ) )
		{
			act[ i ] = false;
			deact[ i ] = true;
		}
		else
		{
			act[ i ] = false;
			deact[ i ] = false;
		}
	}

	const CBehaviorGraphNode* bestPrevNode = nodes[ bestPrevNodeIdx ];
	const CBehaviorGraphNode* bestCurrNode = nodes[ bestCurrNodeIdx ];

	if ( m_synchronize && m_syncMethod && act[ bestCurrNodeIdx ] && bestPrevNode && bestPrevNode != bestCurrNode )
	{
		ASSERT( bestCurrNode );

		bestCurrNode->Activate( instance );

		CSyncInfo syncInfo;
		bestPrevNode->GetSyncInfo( instance, syncInfo );
		m_syncMethod->SynchronizeTo( instance, bestCurrNode, syncInfo );

		act[ bestCurrNodeIdx ] = false;
	}

	for ( Uint32 i=0; i<4; ++i )
	{
		const CBehaviorGraphNode* node = nodes[ i ];

		if ( act[ i ] )
		{
			ASSERT( node );
			ASSERT( node != bestPrevNode );

			node->Activate( instance );

			if ( bestCurrNode && m_synchronize && m_syncMethod )
			{
				ASSERT( node != bestCurrNode );

				CSyncInfo syncInfo;
				bestCurrNode->GetSyncInfo( instance, syncInfo );
				m_syncMethod->SynchronizeTo( instance, node, syncInfo );
			}
		}
		else if ( deact[ i ] )
		{
			ASSERT( node );

			node->Deactivate( instance );
		}
	}
}

Bool CBehaviorGraphBlend3Node::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	Float weights[ 4 ];
	GetWeights4( instance, weights );

	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	for ( Uint32 i=0; i<4; ++i )
	{
		const CBehaviorGraphNode* node = nodes[ i ];

		if ( node && IsInputActive( weights[ i ] ) && node->ProcessEvent( instance, event ) )
		{
			retVal = true;
		}
	}

	return retVal;
}

void CBehaviorGraphBlend3Node::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlVariableNode_A )
	{
		m_cachedControlVariableNode_A->Activate( instance );
	}

	if ( m_cachedControlVariableNode_B )
	{
		m_cachedControlVariableNode_B->Activate( instance );
	}

	UpdateControlValues( instance );

	Float weights[ 4 ];
	GetWeights4( instance, weights );

	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	for ( Uint32 i=0; i<4; ++i )
	{
		if ( nodes[ i ] && IsInputActive( weights[ i ] ) )
		{
			nodes[ i ]->Activate( instance );
		}
	}
}

void CBehaviorGraphBlend3Node::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	for ( Uint32 i=0; i<4; ++i )
	{
		if ( nodes[ i ] )
		{
			nodes[ i ]->Deactivate( instance );
		}
	}

	if ( m_cachedControlVariableNode_A )
	{
		m_cachedControlVariableNode_A->Deactivate( instance );
	}

	if ( m_cachedControlVariableNode_B )
	{
		m_cachedControlVariableNode_B->Deactivate( instance );
	}
}

void CBehaviorGraphBlend3Node::GetWeights3( CBehaviorGraphInstance& instance, Float weights[3] ) const
{
	GetWeights3( instance, weights[ 0 ], weights[ 1 ], weights[ 2 ] );
}

void CBehaviorGraphBlend3Node::GetWeights3( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC ) const
{
	const Float a = instance[ i_controlValue_A ];
	const Float b = instance[ i_controlValue_B ];

	weightA = a;
	weightB = b;
	weightC = 1.f - a - b;
}

void CBehaviorGraphBlend3Node::GetPrevWeights3( CBehaviorGraphInstance& instance, Float weights[3] ) const
{
	GetPrevWeights3( instance, weights[ 0 ], weights[ 1 ], weights[ 2 ] );
}

void CBehaviorGraphBlend3Node::GetPrevWeights3( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC ) const
{
	const Float a = instance[ i_prevControlValue_A ];
	const Float b = instance[ i_prevControlValue_B ];

	weightA = a;
	weightB = b;
	weightC = 1.f - a - b;
}

void CBehaviorGraphBlend3Node::CalcWeight4( Float a, Float b, Float& weightA, Float& weightB, Float& weightC, Float& weightD ) const
{
	if ( m_useCustomSpace )
	{
		Vector2 P( a, b );

		Float u,v;

		if ( MathUtils::GeometryUtils::IsPointInsideTriangle2D_UV( m_C, m_A, m_B, P, u, v ) )
		{
			weightA = u;
			weightB = v;
			weightC = 1.f - u - v;
			weightD = 0.f;
		}
		else if ( MathUtils::GeometryUtils::IsPointInsideTriangle2D_UV( m_D, m_A, m_B, P, u, v ) )
		{
			weightA = u;
			weightB = v;
			weightC = 0.f;
			weightD = 1.f - u - v;
		}
		else
		{
			// please be more descriptive about asserts, if there is no information why it should or shouldn't fail, I decide to disable it - ASSERT( 0 );

			weightA = a;
			weightB = b;
			weightC = 1.f;
			weightD = 0.f;
		}
	}
	else
	{
		if ( a + b <= 1.f )
		{
			weightA = a;
			weightB = b;
			weightC = 1.f - a - b;
			weightD = 0.f;
		}
		else
		{
			weightA = 1.f - a;
			weightB = 1.f - b;
			weightC = 0.f;
			weightD = a + b - 1.f;
		}
	}
}

void CBehaviorGraphBlend3Node::GetWeights4( CBehaviorGraphInstance& instance, Float weights[4] ) const
{
	GetWeights4( instance, weights[ 0 ], weights[ 1 ], weights[ 2 ], weights[ 3 ] );
}

void CBehaviorGraphBlend3Node::GetWeights4( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC, Float& weightD ) const
{
	const Float a = instance[ i_controlValue_A ];
	const Float b = instance[ i_controlValue_B ];
	
	CalcWeight4( a, b, weightA, weightB, weightC, weightD );
}

void CBehaviorGraphBlend3Node::GetPrevWeights4( CBehaviorGraphInstance& instance, Float weights[4] ) const
{
	GetPrevWeights4( instance, weights[ 0 ], weights[ 1 ], weights[ 2 ], weights[ 3 ] );
}

void CBehaviorGraphBlend3Node::GetPrevWeights4( CBehaviorGraphInstance& instance, Float& weightA, Float& weightB, Float& weightC, Float& weightD ) const
{
	const Float a = instance[ i_prevControlValue_A ];
	const Float b = instance[ i_prevControlValue_B ];
	
	CalcWeight4( a, b, weightA, weightB, weightC, weightD );
}

Int32 CBehaviorGraphBlend3Node::GetBestMainNodeIndex( CBehaviorGraphInstance& instance ) const
{
	Float weights[ 4 ];
	GetWeights4( instance, weights[ 0 ], weights[ 1 ], weights[ 2 ], weights[ 3 ] );

	Int32 best = 0;
	Float val = weights[ best ];

	for ( Int32 i=1; i<4; i++ )
	{
		if ( weights[ i ] > val )
		{
			best = i;
			val = weights[ i ];
		}
	}

	return best;
}

const CBehaviorGraphNode* CBehaviorGraphBlend3Node::GetBestMainNode( CBehaviorGraphInstance& instance ) const
{
	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	const Int32 best = GetBestMainNodeIndex( instance );

	ASSERT( best >= 0 && best < 4 );

	return nodes[ best ];
}

Int32 CBehaviorGraphBlend3Node::GetBestPrevMainNodeIndex( CBehaviorGraphInstance& instance ) const
{
	Float weights[ 4 ];
	GetPrevWeights4( instance, weights[ 0 ], weights[ 1 ], weights[ 2 ], weights[ 3 ] );

	Int32 best = 0;
	Float val = weights[ best ];

	for ( Int32 i=1; i<4; i++ )
	{
		if ( weights[ i ] > val )
		{
			best = i;
			val = weights[ i ];
		}
	}

	return best;
}

const CBehaviorGraphNode* CBehaviorGraphBlend3Node::GetBestPrevMainNode( CBehaviorGraphInstance& instance ) const
{
	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	const Int32 best = GetBestPrevMainNodeIndex( instance );

	ASSERT( best >= 0 && best < 4 );

	return nodes[ best ];
}

void CBehaviorGraphBlend3Node::GetNodes3( const CBehaviorGraphNode* nodes[3] ) const
{
	nodes[ 0 ] = m_cachedInputNode_A;
	nodes[ 1 ] = m_cachedInputNode_B;
	nodes[ 2 ] = m_cachedInputNode_C;
}

void CBehaviorGraphBlend3Node::GetNodes4( const CBehaviorGraphNode* nodes[4] ) const
{
	nodes[ 0 ] = m_cachedInputNode_A;
	nodes[ 1 ] = m_cachedInputNode_B;
	nodes[ 2 ] = m_cachedInputNode_C;
	nodes[ 3 ] = m_cachedInputNode_D;
}

Bool CBehaviorGraphBlend3Node::IsInputActive( Float var ) const
{
	return var > ACTIVATION_THRESHOLD;
}

void CBehaviorGraphBlend3Node::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	Float weights[ 4 ];
	GetWeights4( instance, weights );

	const CBehaviorGraphNode* nodes[ 4 ];
	GetNodes4( nodes );

	for ( Uint32 i=0; i<4; ++i )
	{
		if ( nodes[ i ] )
		{
			nodes[ i ]->ProcessActivationAlpha( instance, weights[ i ] * alpha );
		}
	}

	if ( m_cachedControlVariableNode_A )
	{
		m_cachedControlVariableNode_A->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedControlVariableNode_B )
	{
		m_cachedControlVariableNode_B->ProcessActivationAlpha( instance, alpha );
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
