
#include "build.h"
#include "behaviorGraphJoinNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animGlobalParam.h"
#include "behaviorGraphNode.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphJoinNode );

RED_DEFINE_STATIC_NAME( OutputC );
RED_DEFINE_STATIC_NAME( OutputD );
RED_DEFINE_STATIC_NAME( OutputE );
RED_DEFINE_STATIC_NAME( OutputF );
RED_DEFINE_STATIC_NAME( OutputG );
RED_DEFINE_STATIC_NAME( OutputH );
RED_DEFINE_STATIC_NAME( Injected );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphJoinNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputA ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputB ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputC ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputD ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputE ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputF ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputG ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( OutputH ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
}

#endif

void CBehaviorGraphJoinNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
	compiler << i_sampleMarker;
}

void CBehaviorGraphJoinNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_sampleMarker ] = (Uint32)-1;
}

void CBehaviorGraphJoinNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphJoinNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	//CHECK_SAMPLE_ID; Do not use this for joint node

	Uint32& sampled = instance[ i_sampleMarker ];
	if ( sampled != context.GetSampledID() )
	{
		if ( m_cachedInputNode )
		{
			m_cachedInputNode->Sample( context, instance, output );

			CAllocatedBehaviorGraphOutput& allocPose = instance[ i_pose ];
			SBehaviorGraphOutput* pose = allocPose.GetPose();
			if ( pose )
			{
				*pose = output;
			}
		}

		sampled = context.GetSampledID();
	}
	else
	{
		CAllocatedBehaviorGraphOutput& allocPose = instance[ i_pose ];
		SBehaviorGraphOutput* pose = allocPose.GetPose();
		if ( pose )
		{
			output = *pose;
		}
	}
}

void CBehaviorGraphJoinNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
	
	CreatePose( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphJoinNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	DestroyPose( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphJoinNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphJoinNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphJoinNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphJoinNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

Bool CBehaviorGraphJoinNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->PreloadAnimations( instance );
	}

	return true;
}

void CBehaviorGraphJoinNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputNode = CacheBlock( TXT("Input") );
}

void CBehaviorGraphJoinNode::CreatePose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( !instance[ i_pose ].HasPose() );
	instance[ i_pose ].Create( instance );
	ASSERT( instance[ i_pose ].HasPose() );
}

void CBehaviorGraphJoinNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( instance[ i_pose ].HasPose() );
	instance[ i_pose ].Free( instance );
	ASSERT( !instance[ i_pose ].HasPose() );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphInjectorNode );

void CBehaviorGraphInjectorNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_weight;
}

void CBehaviorGraphInjectorNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_weight ] = 0.f;
}

void CBehaviorGraphInjectorNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_weight );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphInjectorNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Injected ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

void CBehaviorGraphInjectorNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedControlNode )
	{
		m_cachedControlNode->Update( context, instance, timeDelta );
		instance[ i_weight ] = Clamp( m_cachedControlNode->GetValue( instance ), 0.0f, 1.0f );
	}

	if ( m_cachedInjectorNode && instance[ i_weight ] > 0.5f )
	{
		m_cachedInjectorNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphInjectorNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_cachedInjectorNode && instance[ i_weight ] > 0.5f )
	{
		m_cachedInjectorNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphInjectorNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInjectorNode ) 
	{
		m_cachedInjectorNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedControlNode ) 
	{
		m_cachedControlNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphInjectorNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInjectorNode )
	{
		m_cachedInjectorNode->Activate( instance );
	}

	if ( m_cachedControlNode )
	{
		m_cachedControlNode->Activate( instance );
	}
}

void CBehaviorGraphInjectorNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInjectorNode )
	{
		m_cachedInjectorNode->Deactivate( instance );
	}

	if ( m_cachedControlNode )
	{
		m_cachedControlNode->Deactivate( instance );
	}
}

void CBehaviorGraphInjectorNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInjectorNode = CacheBlock( TXT("Injected") );
	m_cachedControlNode = CacheValueBlock( TXT("Weight") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStaticConditionNode );
IMPLEMENT_ENGINE_CLASS( IBehaviorGraphStaticCondition );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStaticCondition_AnimTag );

CBehaviorGraphStaticConditionNode::CBehaviorGraphStaticConditionNode()
	: m_condition( nullptr )
	, m_cachedInputANode( nullptr )
	, m_cachedInputBNode( nullptr ) 
{
}

void CBehaviorGraphStaticConditionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_conditionValue;
}

void CBehaviorGraphStaticConditionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_conditionValue ] = CheckStaticCondition( instance );
}

Bool CBehaviorGraphStaticConditionNode::CheckStaticCondition( const CBehaviorGraphInstance& instance ) const
{
	return m_condition ? m_condition->Check( instance ) : false;
}

const CBehaviorGraphNode* CBehaviorGraphStaticConditionNode::GetNode( CBehaviorGraphInstance& instance ) const
{
	const Bool val = instance[ i_conditionValue ];
	return val ? m_cachedInputANode : m_cachedInputBNode;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String CBehaviorGraphStaticConditionNode::GetCaption() const
{ 
	if ( m_condition )
	{
		return String::Printf( TXT("Condition [%s]"), m_condition->GetCaption().AsChar() );
	}

	return TXT("Condition"); 
}
#endif

void CBehaviorGraphStaticConditionNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_conditionValue );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphStaticConditionNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( True_ ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( False_ ) ) );
}

#endif

void CBehaviorGraphStaticConditionNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputANode = CacheBlock( CNAME( True_ ) );
	m_cachedInputBNode = CacheBlock( CNAME( False_ ) );
}

void CBehaviorGraphStaticConditionNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( const CBehaviorGraphNode* node = GetNode( instance ) ) 
	{
		node->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphStaticConditionNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	if ( const CBehaviorGraphNode* node = GetNode( instance ) ) 
	{
		node->Sample( context, instance, output );
	}
}

void CBehaviorGraphStaticConditionNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	if ( const CBehaviorGraphNode* node = GetNode( instance ) ) 
	{
		node->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphStaticConditionNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( const CBehaviorGraphNode* node = GetNode( instance ) ) 
	{
		node->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphStaticConditionNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( const CBehaviorGraphNode* node = GetNode( instance ) ) 
	{
		return node->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphStaticConditionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( const CBehaviorGraphNode* node = GetNode( instance ) ) 
	{
		node->Activate( instance );
	}
}

void CBehaviorGraphStaticConditionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( const CBehaviorGraphNode* node = GetNode( instance ) ) 
	{
		node->Deactivate( instance );
	}
}

void CBehaviorGraphStaticConditionNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	const Bool val = instance[ i_conditionValue ];
	if ( val )
	{
		if ( m_cachedInputANode )
		{
			m_cachedInputANode->ProcessActivationAlpha( instance, alpha );
		}
		if ( m_cachedInputBNode )
		{
			m_cachedInputBNode->ProcessActivationAlpha( instance, 0.f );
		}
	}
	else
	{
		if ( m_cachedInputANode )
		{
			m_cachedInputANode->ProcessActivationAlpha( instance, 0.f );
		}
		if ( m_cachedInputBNode )
		{
			m_cachedInputBNode->ProcessActivationAlpha( instance, alpha );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CBehaviorGraphStaticCondition_AnimTag::Check( const CBehaviorGraphInstance& instance ) const
{
	const CEntity* e = instance.GetAnimatedComponent()->GetEntity();
	const CEntityTemplate* templ = e->GetEntityTemplate();
	if ( templ )
	{
		if ( const CAnimGlobalParam* animParam = templ->FindParameter< CAnimGlobalParam >( true ) )
		{
			const Bool val = m_animTag == animParam->GetAnimTag();
			return val;
		}
	}

	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String CBehaviorGraphStaticCondition_AnimTag::GetCaption() const
{ 
	return String::Printf( TXT("AnimTag - '%s'"), m_animTag.AsChar() );
}
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
