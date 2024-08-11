
#include "build.h"
#include "behaviorGraphMimicPoseNodes.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSocket.h"
#include "../engine/mimicFac.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/mimicComponent.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/entity.h"
#include "../engine/entityTemplate.h"
#include "../engine/animGlobalParam.h"
#include "behaviorProfiler.h"

IMPLEMENT_RTTI_ENUM( EMimicNodePoseType );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicPoseNode );

CBehaviorGraphMimicPoseNode::CBehaviorGraphMimicPoseNode()
	: m_poseType( MNPS_Pose )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicPoseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Pose ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

Int32 CBehaviorGraphMimicPoseNode::GetSelectedPose( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_selectedPose ];
}

#endif

void CBehaviorGraphMimicPoseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_selectedPose;
	compiler << i_weight;
}

void CBehaviorGraphMimicPoseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );
	
	Int32& poseNum = instance[ i_selectedPose ];
	poseNum = -1;

	if ( const CMimicComponent* head = Cast< const CMimicComponent >( instance.GetAnimatedComponent() ) )
	{
		if ( !m_cachedPoseValueNode && head->GetMimicFace() )
		{
			if ( m_poseType == MNPS_Pose )
			{
				poseNum = head->GetExtendedMimics().FindTrackPose( m_poseName );
			}
			else if ( m_poseType == MNPS_Filter )
			{
				poseNum = head->GetExtendedMimics().FindFilterPose( m_poseName );
			}
			else if ( m_poseType == MNPS_CustomFilter_Full )
			{
				const CEntity* e = instance.GetAnimatedComponent()->GetEntity();
				if ( const CEntityTemplate* templ = e->GetEntityTemplate() )
				{
					if ( const CAnimGlobalParam* param = templ->FindParameter< CAnimGlobalParam >() )
					{
						const CName& customPoseName = param->GetCustomMimicsFilterFullName();
						poseNum = head->GetExtendedMimics().FindFilterPose( customPoseName );
					}
				}
			}
			else if ( m_poseType == MNPS_CustomFilter_Lipsync )
			{
				const CEntity* e = instance.GetAnimatedComponent()->GetEntity();
				if ( const CEntityTemplate* templ = e->GetEntityTemplate() )
				{
					if ( const CAnimGlobalParam* param = templ->FindParameter< CAnimGlobalParam >() )
					{
						const CName& customPoseName = param->GetCustomMimicsFilterLipsyncName();
						poseNum = head->GetExtendedMimics().FindFilterPose( customPoseName );
					}
				}
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}
	
	instance[ i_weight ] = 1.f;
}

void CBehaviorGraphMimicPoseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicPose );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedPoseValueNode )
	{
		m_cachedPoseValueNode->Update( context, instance, timeDelta );

		Int32& selectedPose = instance[ i_selectedPose ];
		selectedPose = BehaviorUtils::ConvertFloatToInt( m_cachedPoseValueNode->GetValue( instance ) );
	}

	if ( m_cachedPoseWeightNode )
	{
		instance[ i_weight ] = Clamp( m_cachedPoseWeightNode->GetValue( instance ), 0.f, 1.f );
	}
}

void CBehaviorGraphMimicPoseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedPoseValueNode )
	{
		m_cachedPoseValueNode->Activate( instance );
	}

	if ( m_cachedPoseWeightNode )
	{
		m_cachedPoseWeightNode->Activate( instance );
	}
}

void CBehaviorGraphMimicPoseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedPoseValueNode )
	{
		m_cachedPoseValueNode->Deactivate( instance );
	}

	if ( m_cachedPoseWeightNode )
	{
		m_cachedPoseWeightNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicPoseNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
	m_cachedPoseValueNode = CacheValueBlock( TXT("Pose") );
	m_cachedPoseWeightNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphMimicPoseNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedPoseValueNode )
	{
		m_cachedPoseValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedPoseWeightNode )
	{
		m_cachedPoseWeightNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMimicPoseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	BEH_NODE_SAMPLE( MimicPose );

	const Int32 poseNum = instance[ i_selectedPose ];
	if ( poseNum != -1 )
	{
		const CMimicComponent* head = static_cast< const CMimicComponent* >( instance.GetAnimatedComponent() );
		CExtendedMimics mimics = head->GetExtendedMimics();

		const Float weight = instance[ i_weight ];

		if ( IsPoseType() )
		{
			mimics.ApplyTrackPose( poseNum, weight, output );
		}
		else
		{
			mimics.ApplyFilterPose( poseNum, weight, output );
		}
	}
}
