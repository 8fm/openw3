/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphFilterNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphFilterNode );

CBehaviorGraphFilterNode::CBehaviorGraphFilterNode()
	: m_filterTransform(false)
	, m_filterRotation(true)
	, m_filterScale(false)
{
}

void CBehaviorGraphFilterNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_masterWeight;
}

void CBehaviorGraphFilterNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_masterWeight ] = 1.f;
}

void CBehaviorGraphFilterNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_masterWeight );
}

TDynArray<SBehaviorGraphBoneInfo>* CBehaviorGraphFilterNode::GetBonesProperty()
{
	return &m_bones;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphFilterNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );		
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Alpha ) ) );
}

#endif

void CBehaviorGraphFilterNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( FilterNode );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedControlNode )
	{
		m_cachedControlNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedControlNode )
	{
		instance[ i_masterWeight ] = Clamp( m_cachedControlNode->GetValue( instance ), 0.0f, 1.0f );
	}
	else
	{
		instance[ i_masterWeight ] = 1.f;
	}
}

void CBehaviorGraphFilterNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( FilterNode );

	TBaseClass::Sample( context, instance, output );

	Int32 boneNum = (Int32)output.m_numBones;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform filterTrans;
#else
	RedQsTransform filterTrans;
#endif

	for( Uint32 i=0; i<m_bones.Size(); ++i )
	{
#ifdef USE_HAVOK_ANIMATION
		filterTrans.setIdentity();
#else
		filterTrans.SetIdentity();
#endif
		Int32 outputIndex = m_bones[i].m_num;

		if ( outputIndex < boneNum )
		{
#ifdef USE_HAVOK_ANIMATION
			if (!m_filterTransform)	filterTrans.setTranslation(output.m_outputPose[outputIndex].getTranslation());
			if (!m_filterRotation)	filterTrans.setRotation(output.m_outputPose[outputIndex].getRotation());
			if (!m_filterScale)		filterTrans.setScale(output.m_outputPose[outputIndex].getScale());

			hkReal weight = m_bones[i].m_weight * instance[ i_masterWeight ];
			output.m_outputPose[ outputIndex ].setInterpolate4( filterTrans, output.m_outputPose[outputIndex], weight );	
#else
			if(!m_filterTransform)
            {
				filterTrans.SetTranslation(output.m_outputPose[outputIndex].GetTranslation());
			}
			if(!m_filterRotation)	
			{
				filterTrans.SetRotation(output.m_outputPose[outputIndex].GetRotation());
			}
			if(!m_filterScale)		
			{
				filterTrans.SetScale(output.m_outputPose[outputIndex].GetScale());
			}
			Float weight = m_bones[i].m_weight * instance[ i_masterWeight ];
			output.m_outputPose[ outputIndex ].Lerp( filterTrans, output.m_outputPose[outputIndex], weight );	
#endif
		}		
	}
}

void CBehaviorGraphFilterNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlNode ) 
	{
		m_cachedControlNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphFilterNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlNode )
	{
		m_cachedControlNode->Activate( instance );
	}
}

void CBehaviorGraphFilterNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlNode )
	{
		m_cachedControlNode->Deactivate( instance );
	}
}

void CBehaviorGraphFilterNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlNode = CacheValueBlock( TXT("Alpha") );
}
