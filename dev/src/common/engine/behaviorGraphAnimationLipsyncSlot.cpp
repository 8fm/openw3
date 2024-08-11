
#include "build.h"
#include "behaviorGraphAnimationLipsyncSlot.h"

#include "../core/instanceDataLayoutCompiler.h"

#include "../engine/entity.h"
#include "../engine/mimicComponent.h"
#include "../engine/graphConnectionRebuilder.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "actorInterface.h"
#include "behaviorProfiler.h"
#include "poseProvider.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAllocatedLipsyncBehaviorGraphOutput );

CAllocatedLipsyncBehaviorGraphOutput::CAllocatedLipsyncBehaviorGraphOutput()
	: m_instance( nullptr )
{

}

CAllocatedLipsyncBehaviorGraphOutput::~CAllocatedLipsyncBehaviorGraphOutput()
{
	ASSERT( !m_pose );
}

CPoseProvider* CAllocatedLipsyncBehaviorGraphOutput::GetAlloc() const
{
	ASSERT( m_instance );
	const CEntity* owner = m_instance->GetAnimatedComponent()->GetEntity();
	const IActorInterface* actor = owner->QueryActorInterface();
	if ( actor )
	{
		const CMimicComponent* head = actor->GetMimicComponent();
		if ( head )
		{
			const CSkeleton* skeleton = head->GetMimicSkeleton();
			if ( skeleton )
			{
				return skeleton->GetPoseProvider();
			}
		}
	}

	return nullptr;
}

void CAllocatedLipsyncBehaviorGraphOutput::Create( CBehaviorGraphInstance& instance )
{
	if ( m_pose )
	{
		ASSERT( !m_pose );
		return;
	}

	m_instance = &instance;
	CPoseProvider* alloc = GetAlloc();
	if ( alloc )
	{
		m_pose = alloc->AcquirePose();
	}
}

void CAllocatedLipsyncBehaviorGraphOutput::Free( CBehaviorGraphInstance& instance )
{
	if ( !m_pose )
	{
		ASSERT( m_pose );
		return;
	}

	m_pose.Reset();
}

CAllocatedLipsyncBehaviorGraphOutput& CAllocatedLipsyncBehaviorGraphOutput::operator=( const CAllocatedLipsyncBehaviorGraphOutput& rhs )
{
	if (m_pose && ! rhs.m_pose)
	{
		Free( *m_instance );
	}
	if (! m_pose && rhs.m_pose)
	{
		Create( *rhs.m_instance );
	}
	if (m_pose && rhs.m_pose)
	{
		m_pose->operator =(*rhs.m_pose);
	}
	return *this;
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationLipsyncSlotNode );

CBehaviorGraphAnimationLipsyncSlotNode::CBehaviorGraphAnimationLipsyncSlotNode()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimationLipsyncSlotNode::GetCaption() const
{
	return String::Printf( TXT("Animation lipsync slot [ %s ]"), m_name.AsChar() );
}

void CBehaviorGraphAnimationLipsyncSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Additive ) ) );
}

#endif

void CBehaviorGraphAnimationLipsyncSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
}

void CBehaviorGraphAnimationLipsyncSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimationLipsyncSlot );

	TBaseClass::OnUpdate( context, instance, timeDelta );
	
	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedAdditiveAnimInputNode )
	{
		m_cachedAdditiveAnimInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphAnimationLipsyncSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Sample( context, instance, output );
	}

	const Bool shouldSample = m_cachedAdditiveAnimInputNode || ( IsSlotActive( instance ) && IsValid( instance ) );
	if ( shouldSample )
	{
		CCacheBehaviorGraphOutput cacheAdditivePose( context );
		SBehaviorGraphOutput* additivePose = cacheAdditivePose.GetPose();
		if ( additivePose )
		{
			if ( m_cachedAdditiveAnimInputNode )
			{
				m_cachedAdditiveAnimInputNode->Sample( context, instance, *additivePose );

				if ( !additivePose->IsTouched() )
				{
					additivePose->SetIdentity();
				}
			}
			else
			{
				additivePose->SetIdentity();
			}

			if ( IsSlotActive( instance ) && IsValid( instance ) )
			{
				SBehaviorGraphOutput* mimicPose = GetAllocedPose( instance );
				if ( mimicPose )
				{
					TBaseClass::Sample( context, instance, *mimicPose );

					BlendLipsyncToAdditivePose( instance, *additivePose, *mimicPose );

					additivePose->Touch();
				}
				else
				{
					ASSERT( mimicPose );
				}
			}

			BlendAddPoseToMainPose( output, *additivePose );
		}
	}
}

void CBehaviorGraphAnimationLipsyncSlotNode::BlendLipsyncToAdditivePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& addPose, const SBehaviorGraphOutput& mimicPose ) const
{
	const CMimicComponent* head = Cast< const CMimicComponent >( instance.GetAnimatedComponent() );
	const CMimicFace* face = head ? head->GetMimicFace() : nullptr;
	if ( face )
	{
		const Float POSE_THRESHOLD = 0.01f;

		const Uint32 num = mimicPose.m_numFloatTracks;
		for ( Uint32 i=0; i<num; ++i )
		{
			const Float poseWeight = mimicPose.m_floatTracks[ i ];

			if ( MAbs( poseWeight ) > POSE_THRESHOLD )
			{
				const Uint32 poseIndex = i + 0;

				SBehaviorGraphOutput outPose( 0 );		// No used anims used
				const Bool ret = face->GetMimicPose( poseIndex, outPose );
				if ( ret )
				{
					face->AddMimicPose( addPose, outPose, poseWeight );
				}
			}
		}
	}
}

void CBehaviorGraphAnimationLipsyncSlotNode::BlendAddPoseToMainPose( SBehaviorGraphOutput& mainPose, const SBehaviorGraphOutput& addPose ) const
{
	mainPose.SetAddMul( addPose, 1.f );
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, const CName& animation, const SBehaviorSlotSetup* slotSetup ) const
{
	const Bool ret = TBaseClass::PlayAnimation( instance, animation, slotSetup );

	if ( IsSlotActive( instance ) && !HasAllocedPose( instance ) )
	{
		AllocPose( instance );
	}

	return ret;
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup ) const
{
	const Bool ret = TBaseClass::PlayAnimation( instance, skeletalAnimation, slotSetup );

	if ( IsSlotActive( instance ) && !HasAllocedPose( instance ) )
	{
		AllocPose( instance );
	}

	return ret;
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime /*= 0.0f*/) const
{
	ASSERT( Red::Math::NumericalUtils::Abs( blendOutTime ) < 0.0001f, TXT(" This node does not serve blending at stop!") );
	
	const Bool ret = TBaseClass::StopAnimation( instance, blendOutTime );

	ASSERT( instance[ i_animation ] == nullptr );

	if ( HasAllocedPose( instance ) )
	{
		DeallocPose( instance );
	}

	return ret;
}

void CBehaviorGraphAnimationLipsyncSlotNode::OnAnimationFinished( CBehaviorGraphInstance& instance ) const
{
	const Bool wasActive = IsSlotActive( instance );

	SlotReset( instance );

	ASSERT( instance[ i_animation ] == nullptr );

	TBaseClass::OnAnimationFinished( instance );

	if ( wasActive )
	{
		if ( HasAllocedPose( instance ) )
		{
			DeallocPose( instance );
		}
		else
		{
			ASSERT( !HasAllocedPose( instance ) );
		}
	}

	ASSERT( !HasAllocedPose( instance ) );
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::UseFovTrack( CBehaviorGraphInstance& instance ) const
{
	return true;
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::UseDofTrack( CBehaviorGraphInstance& instance ) const
{
	return true;
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::ShouldDoPoseCorrection() const
{
	return false;
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::ShouldAddAnimationUsage() const
{
	return false;
}

void CBehaviorGraphAnimationLipsyncSlotNode::AllocPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedLipsyncBehaviorGraphOutput& allocPose = instance[ i_pose ];
	ASSERT( !allocPose.GetPose() );
	allocPose.Create( instance );
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::HasAllocedPose( CBehaviorGraphInstance& instance ) const
{
	const CAllocatedLipsyncBehaviorGraphOutput& allocPose = instance[ i_pose ];
	return allocPose.HasPose();
}

void CBehaviorGraphAnimationLipsyncSlotNode::DeallocPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedLipsyncBehaviorGraphOutput& allocPose = instance[ i_pose ];
	ASSERT( allocPose.GetPose() );
	allocPose.Free( instance );
}

SBehaviorGraphOutput* CBehaviorGraphAnimationLipsyncSlotNode::GetAllocedPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedLipsyncBehaviorGraphOutput& allocPose = instance[ i_pose ];
	return allocPose.GetPose();
}

void CBehaviorGraphAnimationLipsyncSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Activate( instance );
	}

	if ( m_cachedAdditiveAnimInputNode )
	{
		m_cachedAdditiveAnimInputNode->Activate( instance );
	}
}

void CBehaviorGraphAnimationLipsyncSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	Bool wasActive = IsSlotActive( instance );

	TBaseClass::OnDeactivated( instance );

	if ( wasActive )
	{
		if ( HasAllocedPose( instance ) )
		{
			DeallocPose( instance );
		}
		else
		{
			ASSERT( !HasAllocedPose( instance ) );
		}
	}

	ASSERT( !HasAllocedPose( instance ) );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Deactivate( instance );
	}

	if ( m_cachedAdditiveAnimInputNode )
	{
		m_cachedAdditiveAnimInputNode->Deactivate( instance );
	}

	ASSERT( instance[ i_animation ] == nullptr );
	
	SlotReset( instance );
}

void CBehaviorGraphAnimationLipsyncSlotNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedBaseAnimInputNode = CacheBlock( TXT("Base") );
	m_cachedAdditiveAnimInputNode = CacheBlock( TXT("Additive") );
}

void CBehaviorGraphAnimationLipsyncSlotNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedAdditiveAnimInputNode )
	{
		m_cachedAdditiveAnimInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

Bool CBehaviorGraphAnimationLipsyncSlotNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return m_cachedBaseAnimInputNode ? m_cachedBaseAnimInputNode->ProcessEvent( instance, event ) : false;
}
