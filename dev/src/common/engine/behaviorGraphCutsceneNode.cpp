/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphCutsceneNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/entity.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphUtils.inl"
#include "skeletalAnimationEntry.h"
#include "animatedComponent.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCutsceneControllerNode );

CBehaviorGraphCutsceneControllerNode::CBehaviorGraphCutsceneControllerNode()
	: m_mimicControl( false )
{
	m_loopPlayback = false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphCutsceneControllerNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT("mimicControl") )
	{
		OnRebuildSockets();
	}
}

Color CBehaviorGraphCutsceneControllerNode::GetTitleColor() const 
{
	if ( m_mimicControl )
	{
		return CBehaviorGraphMimicsAnimationNode::GetTitleColor();
	}
	else
	{
		return CBehaviorGraphAnimationNode::GetTitleColor();
	}
}

void CBehaviorGraphCutsceneControllerNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( In ) ) );

	if ( m_mimicControl )
	{
		CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Mimic ) ) );
	}
	else
	{
		CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Pose ) ) );
	}
}

#endif

void CBehaviorGraphCutsceneControllerNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_blendingAnimations;
	compiler << i_blendingFactor;
	compiler << i_gameplayMode;
	compiler << i_gameplayRefPosition;
	compiler << i_gameplayBlendTime;
}

void CBehaviorGraphCutsceneControllerNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	if ( instance[ i_animation ] )
	{
		const CAnimatedComponent* ac = instance.GetAnimatedComponent();
		BEH_ERROR( TXT("Cutscene animation node has got animation! Actor '%ls'. Please DEBUG."), ac->GetEntity()->GetName().AsChar() );
		//ASSERT( !instance[ i_animation ] );
	}

	instance[ i_animation ] = NULL;
	instance[ i_gameplayMode ] = false;
	instance[ i_gameplayRefPosition ] = Matrix::IDENTITY;
	instance[ i_gameplayBlendTime ] = 0.f;
}

void CBehaviorGraphCutsceneControllerNode::CacheConnections()
{
	m_cachedBaseInputNode = CacheBlock( TXT("In") );
}

void CBehaviorGraphCutsceneControllerNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	// Update base input
	if ( m_cachedBaseInputNode ) 
	{
		m_cachedBaseInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphCutsceneControllerNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	const CSkeletalAnimationSetEntry* entry = instance[ i_animation ];
	if ( entry )
	{
		const CSkeletalAnimation* anim = entry->GetAnimation();
		const CAnimatedComponent* ac = instance.GetAnimatedComponent();

		if ( !anim )
		{
			BEH_ERROR( TXT("Cutscene animation for actor '%ls' is empty! Please DEBUG."), ac->GetEntity()->GetName().AsChar() );
			ASSERT( anim, TXT("Cutscene animation for actor '%ls' is empty!."), ac->GetEntity()->GetName().AsChar() );
			return;
		}

		if ( !anim->IsLoaded() )
		{
			BEH_ERROR( TXT("Cutscene animation for actor '%ls' is NOT LOADED! Please DEBUG."), ac->GetEntity()->GetName().AsChar() );
			ASSERT( anim, TXT("Cutscene animation for actor '%ls' is empty!."), ac->GetEntity()->GetName().AsChar() );
			return;
		}

		TDynArray< CSkeletalAnimationSetEntry* >& blendingAnims = instance[ i_blendingAnimations ];
		if ( blendingAnims.Size() <= 1 )
		{
			// Sample single animation
			if ( m_mimicControl )
			{
				CBehaviorGraphMimicsAnimationNode::Sample( context, instance, output );
			}
			else
			{
				CBehaviorGraphAnimationNode::Sample( context, instance, output );

				output.ClearUsedAnims();

				const Bool isGameplayMode = instance[ i_gameplayMode ];
				if ( isGameplayMode && output.m_numBones > 0 && ac->HasTrajectoryBone() )
				{
					//++ DEMO MODE e3 2014
					/*const Int32 trajIdx = ac->GetTrajectoryBone();

					const AnimQsTransform& rootLS = output.m_outputPose[ 0 ];
					const AnimQsTransform& trajLS = output.m_outputPose[ trajIdx ];

					AnimQsTransform trajMS;
					trajMS.SetMul( rootLS, trajLS );

					const Matrix trajMatMS = AnimQsTransformToMatrix( trajMS );
					const Matrix& l2w = instance[ i_gameplayRefPosition ];

					static Bool DO_THIS = false;
					if ( DO_THIS )
					{
						const Matrix finalWS = Matrix::Mul( l2w, trajMatMS );
						ac->GetEntity()->Teleport( finalWS.GetTranslation(), finalWS.ToEulerAngles() );

						output.ExtractTrajectory( ac );
					}*/
					//-- DEMO MODE e3 2014
				}
			}
		}
		else
		{
			Float blendingFactor = instance[ i_blendingFactor ];
			SampleBlendingAnim( context, instance, output, blendingFactor, blendingAnims );
		}

		instance[ i_prevTime ] = instance[ i_localTime ];
	}
	else if ( m_cachedBaseInputNode ) 
	{
		m_cachedBaseInputNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphCutsceneControllerNode::SampleBlendingAnim( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float blendingFactor, const TDynArray< CSkeletalAnimationSetEntry* >& anims ) const
{
	Uint32 animNum = anims.Size();

	ASSERT( blendingFactor >= 0.f && blendingFactor <= 1.f );

	Float selAnim = (Float)( animNum - 1) * blendingFactor;

	Uint32 selAnim1 = (Uint32)MFloor( selAnim );
	Uint32 selAnim2 = (Uint32)MCeil( selAnim );
	
	ASSERT( selAnim1 < animNum );
	ASSERT( selAnim2 < animNum );

	CSkeletalAnimationSetEntry* anim1 = anims[ selAnim1 ];
	CSkeletalAnimationSetEntry* anim2 = anims[ selAnim2 ];

	CCacheBehaviorGraphOutput cachePose1( context, m_mimicControl );
	CCacheBehaviorGraphOutput cachePose2( context, m_mimicControl );

	SBehaviorGraphOutput* pose1 = cachePose1.GetPose();
	SBehaviorGraphOutput* pose2 = cachePose2.GetPose();

	if ( pose1 && pose2 )
	{
		instance[ i_animation ] = anim1;
		TBaseClass::Sample( context, instance, *pose1 );

		instance[ i_animation ] = anim2;
		TBaseClass::Sample( context, instance, *pose2 );

		Float temp = (Float)( selAnim1 <= selAnim2 ? selAnim1 : selAnim2 );
		Float weight = selAnim - temp;
		ASSERT( weight >= 0.f && weight <= 1.f );

		output.SetInterpolate( *pose1, *pose2, weight );
		output.MergeEventsAndUsedAnims( *pose1, *pose2 );
	}
}

Bool CBehaviorGraphCutsceneControllerNode::AddCutsceneAnimation( CBehaviorGraphInstance& instance, const CName &name ) const
{
	RefreshAnimation( instance, name );

	CSkeletalAnimationSetEntry* anim = instance[ i_animation ];

	if ( anim )
	{
		instance[ i_blendingAnimations ].PushBack( anim );
	}

	return anim ? true : false;
}

void CBehaviorGraphCutsceneControllerNode::ResetRuntimeAnimation( CBehaviorGraphInstance& instance ) const
{
	instance[ i_animation ] = NULL;
	instance[ i_blendingAnimations ].Clear();
	instance[ i_blendingFactor ] = 0.f;
}

void CBehaviorGraphCutsceneControllerNode::SetBlendFactor( CBehaviorGraphInstance& instance, Float factor ) const
{
	instance[ i_blendingFactor ] = factor;
}

void CBehaviorGraphCutsceneControllerNode::SetGameplayMode( CBehaviorGraphInstance& instance, Bool flag, Float blendTime, const Matrix& refPosition ) const
{
	instance[ i_gameplayMode ] = flag;
	instance[ i_gameplayRefPosition ] = refPosition;
	instance[ i_gameplayBlendTime ] = blendTime;
}

Bool CBehaviorGraphCutsceneControllerNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedBaseInputNode ) 
	{
		return m_cachedBaseInputNode->ProcessEvent( instance, event );
	}
	
	return false;
}

void CBehaviorGraphCutsceneControllerNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedBaseInputNode ) 
	{
		m_cachedBaseInputNode->Activate( instance );
	}
}

void CBehaviorGraphCutsceneControllerNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedBaseInputNode ) 
	{
		m_cachedBaseInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphCutsceneControllerNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	// Don't call reset for animation node (base class)
	instance[ i_localTime ] = 0.f;
	instance[ i_prevTime ] = 0.f;
	instance[ i_animation ] = NULL;
}

void CBehaviorGraphCutsceneControllerNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedBaseInputNode ) 
	{
		m_cachedBaseInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphCutsceneControllerNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	instance[ i_localTime ] = info.m_currTime;
	instance[ i_prevTime ]	= info.m_prevTime;
	instance[ i_loops ]		= 0;
}

void CBehaviorGraphCutsceneControllerNode::CollectEvents( CBehaviorGraphInstance& instance, const CSyncInfo &info,
	TDynArray< CAnimationEventFired >& eventsFired ) const
{
	const Float localTime = instance[ i_localTime ];
	const Float prevTime = instance[ i_prevTime ];
	const Int32 loops = instance[ i_loops ];
	const Float eventsAlpha = 1.0f;
	const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];

	if( animation != NULL )
	{
		animation->GetEventsByTime( prevTime, localTime, loops, eventsAlpha, &eventsFired, NULL );
	}
}

Bool CBehaviorGraphCutsceneControllerNode::CanWork( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	if ( ac && ( ( ac->GetMimicSkeleton() && m_mimicControl ) || ( !ac->GetMimicSkeleton() && !m_mimicControl ) ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CBehaviorGraphCutsceneControllerNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	// TODO - animations are now not stored - reset it
	ResetRuntimeAnimation( instance );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
