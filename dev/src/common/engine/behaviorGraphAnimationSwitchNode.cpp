/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationSwitchNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphValueNode.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animSyncInfo.h"
#include "baseEngine.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationSwitchNode );

CBehaviorGraphAnimationSwitchNode::CBehaviorGraphAnimationSwitchNode()
	: m_inputNum( 0 )
	, m_interpolation( IT_Bezier )
	, m_synchronizeOnSwitch( false )
{
}

void CBehaviorGraphAnimationSwitchNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_selectInput;
	compiler << i_prevSelectInput;
	compiler << i_blendTimer;
	compiler << i_blendTimeDuration;
	compiler << i_blending;
	compiler << i_activatedInputsMask;
}

void CBehaviorGraphAnimationSwitchNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );

	instance[ i_blendTimer ] = 0.f;
	instance[ i_blendTimeDuration ] = m_blendTime;

	BehaviorGraphTools::BitTools::ClearMask( instance[ i_activatedInputsMask ] );

	RED_FATAL_ASSERT( BehaviorGraphTools::BitTools::IsNumberConvertibleToFlag<Uint64>( m_inputNum ), "We have to many inputs! -> inputs size: %i!", m_inputNum ); // we can't have more inputs than mask can hold
}

void CBehaviorGraphAnimationSwitchNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_selectInput );
	INST_PROP( i_prevSelectInput );
	INST_PROP( i_blending );
	INST_PROP( i_blendTimer );
	INST_PROP( i_blendTimeDuration );
}

void CBehaviorGraphAnimationSwitchNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimSwitch );

	if ( m_cachedBlendTimeValueNode )
	{
		m_cachedBlendTimeValueNode->Update( context, instance, timeDelta );
		instance[ i_blendTimeDuration ] = Max( m_cachedBlendTimeValueNode->GetValue( instance ), 0.f );
	}
	else
	{
		instance[ i_blendTimeDuration ] = m_blendTime;
	}

	if ( ProcessInputSelection( instance, timeDelta ) )
	{
		Int32 newSelection = SelectInput( context, instance, timeDelta );

		CheckActivations( instance, newSelection );
	}

	CheckBlendingActivation( instance );

	const CBehaviorGraphNode* selNode = GetSelectInput( instance );
	if ( selNode )
	{
		selNode->Update( context, instance, timeDelta );
	}

	if ( IsBlending( instance ) )
	{
		UpdateBlendTimer( instance, timeDelta );

		const CBehaviorGraphNode* prevNode = GetPrevSelectInput( instance );
		if ( prevNode )
		{
			prevNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphAnimationSwitchNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( AnimSwitch );

	const CBehaviorGraphNode* node = GetSelectInput( instance );
	
	if ( IsBlending( instance ) )
	{
		const CBehaviorGraphNode* prevNode = GetPrevSelectInput( instance );

		CCacheBehaviorGraphOutput cachePose1( context, GetPoseType() );
		CCacheBehaviorGraphOutput cachePose2( context, GetPoseType() );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			if ( node ) 
			{
				node->Sample( context, instance, *temp2 );
			}

			if ( prevNode )
			{
				prevNode->Sample( context, instance, *temp1 );
			}

			const Float weight = GetBlendWeight( instance );
			ASSERT( weight >= 0.f && weight <= 1.f );

			// Interpolate poses
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
			output.MergeEventsAndUsedAnims( *temp1, *temp2, 1.0f - weight, weight );
		}
	}
	else if ( node )
	{
		node->Sample( context, instance, output );
	}
}

Bool CBehaviorGraphAnimationSwitchNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	const CBehaviorGraphNode* node = GetSelectInput( instance );
	if ( node )
	{
		return node->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphAnimationSwitchNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphAnimationSwitchNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_selectInput ] = -1;
	instance[ i_prevSelectInput ] = -1;
	instance[ i_blending ] = false;
}

void CBehaviorGraphAnimationSwitchNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
	const CBehaviorGraphNode* node = GetSelectInput( instance );
	const CBehaviorGraphNode* prevNode = GetPrevSelectInput( instance );
	ASSERT( !node );
	ASSERT( !prevNode );

	if ( m_cachedBlendTimeValueNode )
	{
		m_cachedBlendTimeValueNode->Activate( instance );
	}

	// update variable 
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );

		Float var = m_cachedControlValueNode->GetValue( instance );

		Int32 newSelection = -1;

		if ( var >= 0.f && var < (Float)m_inputNum )
		{
			newSelection = BehaviorUtils::ConvertFloatToInt( var );
		}
		else if ( m_inputNum > 0 )
		{
			newSelection = 0;
		}

		if ( newSelection != -1 )
		{
			Int32& sel = instance[ i_selectInput ];
			Int32& prevSel = instance[ i_prevSelectInput ];

			ASSERT( sel == -1 && prevSel == -1 );

			sel = newSelection;
			prevSel = newSelection;

			ActivateInput( instance, sel );
		}
	}
}

void CBehaviorGraphAnimationSwitchNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	// Deactivate only those input that were activated by this node and haven't been deactivated yet:
	Uint64& inputsMask = instance[ i_activatedInputsMask ];
	for ( Uint32 i = 0; i < m_inputNum; ++i )
	{
		if ( BehaviorGraphTools::BitTools::IsFlagSet( inputsMask, BehaviorGraphTools::BitTools::NumberToFlag<Uint64>( (Uint64)i ) ) )
		{
			DeactivateInput( instance, i );
		}
	}

	RED_ASSERT( BehaviorGraphTools::BitTools::IsMaskCleared( inputsMask ) ); // mask should be cleared here!

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}

	if ( m_cachedBlendTimeValueNode )
	{
		m_cachedBlendTimeValueNode->Deactivate( instance );
	}

	InternalReset( instance );
}

void CBehaviorGraphAnimationSwitchNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	const CBehaviorGraphNode* node = GetSelectInput( instance );

	if ( IsBlending( instance ) )
	{
		const Float weight = GetBlendWeight( instance );

		const CBehaviorGraphNode* prevNode = GetPrevSelectInput( instance );

		if ( node )
		{
			node->ProcessActivationAlpha( instance, alpha * weight );
		}
		if ( prevNode )
		{
			prevNode->ProcessActivationAlpha( instance, alpha * ( 1.f - weight ) );
		}
	}
	else if ( node )
	{
		node->ProcessActivationAlpha( instance, alpha );
		
	}

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedBlendTimeValueNode )
	{
		m_cachedBlendTimeValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

Bool CBehaviorGraphAnimationSwitchNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	/*const CBehaviorGraphNode* node = GetSelectInput( instance );
	if ( node )
	{
		return node->PreloadAnimations( instance );
	}*/

	if ( m_cachedControlValueNode )
	{
		Float var = m_cachedControlValueNode->GetValue( instance );

		Int32 newSelection = -1;

		if ( var >= 0.f && var < (Float)m_inputNum )
		{
			newSelection = BehaviorUtils::ConvertFloatToInt( var );
		}

		if ( newSelection != -1 )
		{
			CBehaviorGraphNode* node = m_cachedInputNodes[ newSelection ];
			if ( node )
			{
				return node->PreloadAnimations( instance );
			}
		}
	}

	return true;
}

CBehaviorGraphNode* CBehaviorGraphAnimationSwitchNode::GetSelectInput( CBehaviorGraphInstance& instance ) const
{
	Int32 select = instance[ i_selectInput ];

	if ( select >= 0 && select < (Int32)m_cachedInputNodes.Size() )
	{
		return m_cachedInputNodes[ select ];
	}

	return NULL;
}

CBehaviorGraphNode* CBehaviorGraphAnimationSwitchNode::GetPrevSelectInput( CBehaviorGraphInstance& instance ) const
{
	Int32 select = instance[ i_prevSelectInput ];

	if ( select >= 0 && select < (Int32)m_cachedInputNodes.Size() )
	{
		return m_cachedInputNodes[ select ];
	}

	return NULL;
}

void CBehaviorGraphAnimationSwitchNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	const CBehaviorGraphNode* node = GetSelectInput( instance );
	if ( node )
	{
		node->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphAnimationSwitchNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	const CBehaviorGraphNode* node = GetSelectInput( instance );
	if ( node )
	{
		node->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphAnimationSwitchNode::CanBlend( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_blendTimeDuration ] > 0.f && instance[ i_selectInput ] != -1 && instance[ i_prevSelectInput ] != -1;
}

Bool CBehaviorGraphAnimationSwitchNode::IsBlending( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_selectInput ] != instance[ i_prevSelectInput ] && instance[ i_selectInput ] != -1;
}

Float CBehaviorGraphAnimationSwitchNode::GetBlendWeight( CBehaviorGraphInstance& instance ) const
{
	if ( CanBlend( instance ) )
	{
		Float value = 1.f - instance[ i_blendTimer ] / instance[ i_blendTimeDuration ];
		return m_interpolation == IT_Linear ? value : BehaviorUtils::BezierInterpolation( value );
	}

	return 1.f;
}

void CBehaviorGraphAnimationSwitchNode::CheckActivations( CBehaviorGraphInstance& instance, const Int32 newSel ) const
{
	Int32& currSel = instance[ i_selectInput ];
	Int32& prevSel = instance[ i_prevSelectInput ];

	if ( currSel != newSel )
	{
		Bool blendBack = false;
		if ( IsBlending( instance ) )
		{
			if ( newSel == prevSel )
			{
				blendBack = true;
			}
			else
			{
				DeactivateInput( instance, prevSel );
			}
		}

		if ( currSel != -1 )
		{
			ASSERT( IsActiveInput( instance, currSel ) );
		}

		if ( newSel != -1 )
		{
			if ( ! blendBack )
			{
				ActivateInput( instance, newSel );
			}

			prevSel = currSel;
			currSel = newSel;

			if ( ! blendBack && m_synchronizeOnSwitch && m_syncOnSwitchMethod )
			{
				CBehaviorGraphNode* prevNode = GetPrevSelectInput( instance );
				CBehaviorGraphNode* currNode = GetSelectInput( instance );
				if ( prevNode && currNode )
				{
					CSyncInfo syncInfo;
					prevNode->GetSyncInfo( instance, syncInfo );
					m_syncOnSwitchMethod->SynchronizeTo( instance, currNode, syncInfo );
				}
			}

			if ( CanBlend( instance ) )
			{
				if ( blendBack )
				{
					instance[ i_blendTimer ] = instance[ i_blendTimeDuration ] - instance[ i_blendTimer ];
				}
				else
				{
					StartBlending( instance );
				}
			}
			else
			{
				if ( prevSel != -1 )
				{
					DeactivateInput( instance, prevSel );
				}
				prevSel = currSel;
			}
		}
		else
		{
			DeactivateInput( instance, currSel );
			currSel = -1;
			prevSel = -1;
		}
	}
}

void CBehaviorGraphAnimationSwitchNode::CheckBlendingActivation( CBehaviorGraphInstance& instance ) const
{
	if ( IsBlending( instance ) && ( instance[ i_blendTimeDuration ] <= 0.f || instance[ i_blendTimer ] <= 0.f ) )
	{
		EndBlending( instance );
	}
}

void CBehaviorGraphAnimationSwitchNode::StartBlending( CBehaviorGraphInstance& instance ) const
{
	instance[ i_blendTimer ] = instance[ i_blendTimeDuration ];
}

void CBehaviorGraphAnimationSwitchNode::EndBlending( CBehaviorGraphInstance& instance ) const
{
	ASSERT( IsActiveInput( instance, instance[ i_selectInput ] ) );

	DeactivateInput( instance, instance[ i_prevSelectInput ] );

	instance[ i_prevSelectInput ] = instance[ i_selectInput ];
}

void CBehaviorGraphAnimationSwitchNode::UpdateBlendTimer( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	instance[ i_blendTimer ] = Max( instance[ i_blendTimer ] - timeDelta, 0.f );
}

void CBehaviorGraphAnimationSwitchNode::ActivateInput( CBehaviorGraphInstance& instance, const Int32 num ) const
{
	if ( num >= 0 && num < (Int32)m_cachedInputNodes.Size() && m_cachedInputNodes[ num ] )
	{
		m_cachedInputNodes[ num ]->Activate( instance );

		// update active inputs mask
		BehaviorGraphTools::BitTools::SetFlag( instance[ i_activatedInputsMask ], BehaviorGraphTools::BitTools::NumberToFlag( (Uint64) num ) );
	}
}

void CBehaviorGraphAnimationSwitchNode::DeactivateInput( CBehaviorGraphInstance& instance, const Int32 num ) const
{
	if ( num >= 0 && num < (Int32)m_cachedInputNodes.Size() && m_cachedInputNodes[ num ] )
	{
		m_cachedInputNodes[ num ]->Deactivate( instance );

		// update active inputs mask
		BehaviorGraphTools::BitTools::ClearFlag( instance[ i_activatedInputsMask ], BehaviorGraphTools::BitTools::NumberToFlag( (Uint64) num  ) );
	}
}

Bool CBehaviorGraphAnimationSwitchNode::IsActiveInput( CBehaviorGraphInstance& instance, const Int32 num ) const
{
	if ( num >= 0 && num < (Int32)m_cachedInputNodes.Size() )
	{
		return m_cachedInputNodes[ num ] ? m_cachedInputNodes[ num ]->IsActive( instance ) : true;
	}
	else
	{
		return false;
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CGraphSocket* CBehaviorGraphAnimationSwitchNode::CreateInputSocket( const CName& name )
{
	return CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( name ) );
}

CGraphSocket* CBehaviorGraphAnimationSwitchNode::CreateOutputSocket( const CName& name )
{
	return CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( name ) );
}

#endif

CBehaviorGraphNode* CBehaviorGraphAnimationSwitchNode::CacheInputBlock( const String& name )
{
	return CacheBlock( name );
}

Bool CBehaviorGraphAnimationSwitchNode::GetPoseType() const
{
	// Animation pose
	return false;
}

Bool CBehaviorGraphAnimationSwitchNode::ProcessInputSelection( CBehaviorGraphInstance& instance, Float& timeDelta ) const
{
	return true;
}

Int32 CBehaviorGraphAnimationSwitchNode::SelectInput( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float& timeDelta ) const
{
	Int32 newSelection = -1;

	// update variable 
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );

		Float var = m_cachedControlValueNode->GetValue( instance );

		if ( var >= 0.f && var < (Float)m_inputNum )
		{
			newSelection = BehaviorUtils::ConvertFloatToInt( var );
		}
	}

	return newSelection;
}

Bool CBehaviorGraphAnimationSwitchNode::WillSelectedInputFinish( CBehaviorGraphInstance& instance, Float& timeDelta ) const
{
	const CBehaviorGraphNode* selNode = GetSelectInput( instance );
	if ( selNode )
	{
		CSyncInfo syncInfo;
		syncInfo.m_wantSyncEvents = false;
		selNode->GetSyncInfo( instance, syncInfo );

		if ( syncInfo.m_currTime + selNode->GetSyncData( instance ).ProcessTimeDelta( timeDelta ) >= syncInfo.m_totalTime )
		{
			Float timeToUpdate = ( syncInfo.m_totalTime - syncInfo.m_currTime ) / selNode->GetSyncData( instance ).m_timeMultiplier;

			timeDelta = Max( 0.0f, timeDelta - timeToUpdate );

			return true;
		}

		return false;
	}

	return true;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimationSwitchNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateOutputSocket( CNAME( Out ) );

	CreateInputSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Select ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Blend ), false ) );
}

#endif

void CBehaviorGraphAnimationSwitchNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Get control variable
	m_cachedControlValueNode = CacheValueBlock( TXT("Select") );
	m_cachedBlendTimeValueNode = CacheValueBlock( TXT("Blend") );

	// Cache input nodes
	m_cachedInputNodes.Clear();
	CacheInputConnections();

	ASSERT( m_cachedInputNodes.Size() == m_inputNum );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationManualSwitchNode );

void CBehaviorGraphAnimationManualSwitchNode::OnInputListChange()
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	OnRebuildSockets();
#endif
}

void CBehaviorGraphAnimationManualSwitchNode::AddInput()
{
	m_inputNum++;

	OnInputListChange();
}

void CBehaviorGraphAnimationManualSwitchNode::RemoveInput()
{
	m_inputNum--;

	OnInputListChange();
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimationManualSwitchNode::CreateInputSockets()
{
	for( Uint32 i=0; i<m_inputNum; ++i )
	{
		const String socketName = String::Printf( TXT("Input%d"), i );
		CreateInputSocket( CName( socketName ) );
	}
}

#endif

void CBehaviorGraphAnimationManualSwitchNode::CacheInputConnections()
{
	for ( Uint32 i=0; i<m_inputNum; i++ )
	{
		const String socketName = String::Printf( TXT("Input%d"), i );
		m_cachedInputNodes.PushBack( CacheInputBlock( socketName ) );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationEnumSwitchNode );

CName CBehaviorGraphAnimationEnumSwitchNode::GetInputName( Uint32 num ) const
{
	CEnum* enumType = SRTTI::GetInstance().FindEnum( m_enum );
	if ( enumType )
	{
		const TDynArray< CName >& options = enumType->GetOptions();
		if ( options.Size() > num + m_firstInputNum )
		{
			return options[ num + m_firstInputNum ];
		}
	}

	return CNAME( Unknown );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimationEnumSwitchNode::CreateInputSockets()
{
	for( Uint32 i=0; i<m_inputNum; ++i )
	{
		CreateInputSocket( GetInputName( i ) );
	}
}

#endif

void CBehaviorGraphAnimationEnumSwitchNode::CacheInputConnections()
{
	for ( Uint32 i=0; i<m_inputNum; i++ )
	{
		m_cachedInputNodes.PushBack( CacheInputBlock( GetInputName( i ).AsString() ) );
	}
}

void CBehaviorGraphAnimationEnumSwitchNode::OnInputListChange()
{
	CEnum* enumType = SRTTI::GetInstance().FindEnum( m_enum );
	if ( enumType )
	{
		const TDynArray< CName >& options = enumType->GetOptions();
		Uint32 size = options.Size();

		for ( Uint32 i=0; i<size; ++i )
		{
			Int32 value = -1;
			if ( enumType->FindValue( options[i], value ) && value >= 0 )
			{
				m_firstInputNum = i;
				m_inputNum = size - i;
#ifndef NO_EDITOR_GRAPH_SUPPORT
				OnRebuildSockets();
#endif
				return;
			}
		}
	}

	m_firstInputNum = 0;
	m_inputNum = 0;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	OnRebuildSockets();
#endif
}

void CBehaviorGraphAnimationEnumSwitchNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("enum") )
	{
		OnInputListChange();
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationEnumSequentialSwitchNode );

Bool CBehaviorGraphAnimationEnumSequentialSwitchNode::ProcessInputSelection( CBehaviorGraphInstance& instance, Float& timeDelta ) const
{
	return WillSelectedInputFinish( instance, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationRandomSwitchNode );

CBehaviorGraphAnimationRandomSwitchNode::CBehaviorGraphAnimationRandomSwitchNode()
	: m_randOnlyOnce( false )
{

}

void CBehaviorGraphAnimationRandomSwitchNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	const Uint32 size = m_cachedInputNodes.Size();
	if ( size > 0 )
	{
		const Int32 newSelection = GEngine->GetRandomNumberGenerator().Get< Int32 >( size );
		if ( newSelection != -1 )
		{
			Int32& sel = instance[ i_selectInput ];
			Int32& prevSel = instance[ i_prevSelectInput ];

			ASSERT( sel == -1 && prevSel == -1 );

			sel = newSelection;
			prevSel = newSelection;

			ActivateInput( instance, sel );
		}
	}
}

Bool CBehaviorGraphAnimationRandomSwitchNode::ProcessInputSelection( CBehaviorGraphInstance& instance, Float& timeDelta ) const
{
	return !m_randOnlyOnce && WillSelectedInputFinish( instance, timeDelta );
}

Int32 CBehaviorGraphAnimationRandomSwitchNode::SelectInput( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float& timeDelta ) const
{
	const Uint32 size = m_cachedInputNodes.Size();
	if ( size == 0 )
	{
		return -1;
	}

	return GEngine->GetRandomNumberGenerator().Get< Int32 >( size );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
