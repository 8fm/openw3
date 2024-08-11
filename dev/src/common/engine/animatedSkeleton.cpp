/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "animatedSkeleton.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "playedAnimation.h"
#include "entity.h"
#include "animatedComponent.h"


CAnimatedSkeleton::CAnimatedSkeleton( const CSkeleton *skeleton )
	: m_baseSkeleton( NULL )
	, m_constraintLastId( 0 )
{
	SetSkeleton( skeleton );
}

CAnimatedSkeleton::~CAnimatedSkeleton()
{
	// Remove all animations
	m_animations.ClearPtr();

	// Remove all constraints
	const Uint32 cSize = m_constraints.Size();
	for ( Uint32 i=0; i<cSize; ++i )
	{
		delete m_constraints[ i ].m_second;
	}
	m_constraints.Clear();
}

void CAnimatedSkeleton::SetSkeleton( const CSkeleton *skeleton )
{
	// Remember pointer to source skeleton
	m_baseSkeleton = skeleton;
}

Uint32 CAnimatedSkeleton::GetNumPlayedAnimations() const
{
	return m_animations.Size();
}

void CAnimatedSkeleton::Update( CAnimatedComponent* ac, Float deltaT )
{
	// Removed finished animations
	for ( Int32 i=m_animations.SizeInt()-1; i>=0; --i )
	{
		CPlayedSkeletalAnimation* animation = m_animations[i];
		if ( animation->ShouldBeRemoved() )
		{
			m_animations.RemoveAt( i );
		}
	}

	// Update all animations
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		CPlayedSkeletalAnimation* animation = m_animations[i];

		Bool fireEffect = animation->ShouldFireAnimEffect();

 		animation->Update( deltaT );

		if ( fireEffect )
		{
			CEntity* entity = ac->GetEntity();

			CName animationName( animation->GetName() );
			ASSERT( animationName != CName::NONE );

			Float localTime = animation->GetTime();

			// Play animation related effect
			entity->PlayEffectForAnimation( animationName, localTime );
		}
	}

	const Uint32 cSize = m_constraints.Size();
	for ( Uint32 i=0; i<cSize; ++i )
	{
		m_constraints[ i ].m_second->Update( ac, deltaT );
	}
}

SBehaviorGraphOutput& CAnimatedSkeleton::Sample( CAnimatedComponent* ac, SBehaviorSampleContext* context ) const
{
	ASSERT( context );

	// Prepare context
	SBehaviorGraphOutput& output = context->PrepareForSample();

	Bool poseCorrection = context->ShouldCorrectPose();
	if ( poseCorrection )
	{
		context->SetPoseCorrection( output );
	}

	const Uint32 cSize = m_constraints.Size();
	for ( Uint32 i=0; i<cSize; ++i )
	{
		m_constraints[ i ].m_second->PreSample( ac, context, output );
	}

	if ( m_animations.Size() == 1 )
	{
		// Sample all animations
		{
			// Get animation
			const CPlayedSkeletalAnimation* animation = m_animations[0];

			// Sample
			//CCacheBehaviorGraphOutput cachePose( context );
			//SBehaviorGraphOutput* pose = cachePose.GetPose();
			// if ( pose )
			if ( animation )
			{
				animation->Sample( context, output );

				// Pose correction
				if ( poseCorrection )
				{
					context->SetPoseCorrection( output );
				}

				// Trajectory extraction
				if ( ac->HasTrajectoryBone() && ac->UseExtractedTrajectory() )
				{
					output.ExtractTrajectory( ac );
				}
				else if ( output.m_numBones > 0 )
				{
#ifdef USE_HAVOK_ANIMATION
					output.m_outputPose[ 0 ].m_rotation.normalize();
#else
					output.m_outputPose[ 0 ].Rotation.Normalize();
#endif
				}

				// Motion extraction
				if ( !ac->UseExtractedMotion() )
				{	
#ifdef USE_HAVOK_ANIMATION
					output.m_deltaReferenceFrameLocal.setIdentity();
#else
					output.m_deltaReferenceFrameLocal.SetIdentity();
#endif
				}

				// Events
				animation->CollectEvents( ac, output );

				// Type
				//ESkeletalAnimationType animType = animation->GetType();

				// Weight
				//Float animWeight = animation->GetWeight();

				// Normalize weight
				//Float normWeight = animWeight / totalWeight;

				// Blend
				//BlendPoses( output, pose, normWeight, animType );

				// Merge events and used anims
				//output.MergeEventsAndUsedAnims( *pose, normWeight );
			}
		}
	}

	for ( Uint32 i=0; i<cSize; ++i )
	{
		m_constraints[ i ].m_second->PostSample( ac, context, output );
	}

	// Return final pose
	return output;
}

void CAnimatedSkeleton::BlendPoses( SBehaviorGraphOutput& output, const SBehaviorGraphOutput* pose, Float weight, ESkeletalAnimationType type ) const
{
	if ( type == SAT_Normal )
	{
		output.SetInterpolate( output, *pose, weight );
	}
	else if ( type == SAT_Additive )
	{
		output.SetAddMul( output, *pose, weight );
	}
	else
	{
		ASSERT( 0 );
	}
}

Bool CAnimatedSkeleton::IsPlayingAnimation( CSkeletalAnimationSetEntry *animation ) const
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		const CPlayedSkeletalAnimation* anim = m_animations[i];
		if ( anim->IsEqual( animation ) )
		{
			return true;
		}
	}

	return false;
}

Bool CAnimatedSkeleton::IsPlayingAnimation( const CName& animationName ) const
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		const CPlayedSkeletalAnimation* anim = m_animations[i];
		if ( anim->IsEqual( animationName ) )
		{
			return true;
		}
	}

	return false;
}

CPlayedSkeletalAnimation* CAnimatedSkeleton::GetPlayedAnimation( const CName& name )
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		CPlayedSkeletalAnimation* anim = m_animations[i];
		if ( anim->IsEqual( name ) )
		{
			return anim;
		}
	}

	return NULL;
}

CPlayedSkeletalAnimation* CAnimatedSkeleton::GetPlayedAnimation( Uint32 num )
{
	return num < m_animations.Size() ? m_animations[ num ] : NULL;
}

CPlayedSkeletalAnimation* CAnimatedSkeleton::PlayAnimation( CSkeletalAnimationSetEntry *animation, 
														    Bool repleace /* = true */, Bool looped /* = true */, 
															Float weight /* = 1.f  */,
															ESkeletalAnimationType type /* = SAT_Normal */,
															Bool autoFireEffects /* = true */ )
{
	Bool playingAnimation = IsPlayingAnyAnimation();

	if ( playingAnimation == false || ( playingAnimation == true && repleace ) )
	{
		if ( playingAnimation && repleace )
		{
			StopAllAnimation();
		}

		CPlayedSkeletalAnimation* anim = new CPlayedSkeletalAnimation( animation, m_baseSkeleton );
		if ( anim->Play( looped, weight, 0.f, 0.f, type, autoFireEffects ) )
		{
			m_animations.PushBack( anim );
			return m_animations.Back();
		}
	}

	return NULL;
}

Bool CAnimatedSkeleton::StopAnimation( CSkeletalAnimationSetEntry *animation )
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		CPlayedSkeletalAnimation* anim = m_animations[i];
		if ( anim->IsEqual( animation ) )
		{
			anim->Stop();
			delete anim;

			m_animations.Erase( m_animations.Begin() + i );

			return true;
		}
	}

	return false;
}

void CAnimatedSkeleton::StopAllAnimation()
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		m_animations[ i ]->Stop();
	}

	m_animations.ClearPtr();
}

Bool CAnimatedSkeleton::IsPlayingAnyAnimation() const
{
	return m_animations.Size() > 0 ? true : false;
}

void CAnimatedSkeleton::PauseAllAnimations( Bool flag )
{
	if ( flag )
	{
		for ( Uint32 i=0; i<m_animations.Size(); ++i )
		{
			m_animations[ i ]->Pause();
		}
	}
	else
	{
		for ( Uint32 i=0; i<m_animations.Size(); ++i )
		{
			m_animations[ i ]->Unpause();
		}
	}
}

Bool CAnimatedSkeleton::IsAnyAnimationPaused() const
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( m_animations[ i ]->IsPaused() )
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

Uint32 CAnimatedSkeleton::AddConstraint( IAnimatedSkeletonConstraint* constraint )
{
	Uint32 newId = m_constraintLastId++;

	m_constraints.PushBack( TPair< Uint32, IAnimatedSkeletonConstraint* >( newId, constraint ) );

	return newId;
}

Bool CAnimatedSkeleton::RemoveConstraint( Uint32 constraintId )
{
	ASSERT( constraintId < m_constraintLastId );

	const Uint32 size = m_constraints.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_constraints[ i ].m_first == constraintId )
		{
			delete m_constraints[ i ].m_second;
			m_constraints.RemoveAt( i );
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////

CBehaviorAnimatedSkeleton::CBehaviorAnimatedSkeleton( const CSkeleton *skeleton )
	: CAnimatedSkeleton( skeleton )
{

}

//////////////////////////////////////////////////////////////////////////

/*CMimicAnimatedSkeleton::CMimicAnimatedSkeleton( const CSkeleton* skeleton )
	: CBehaviorAnimatedSkeleton( skeleton )
{

}

CPlayedSkeletalAnimation* CMimicAnimatedSkeleton::PlayMimicAnimation(	CSkeletalAnimationSetEntry *animation,
																		const CMimicFaces* mimicFaces )
{
	if ( IsPlayingAnyAnimation() )
	{
		StopAllAnimation();
	}

	CPlayedMimicSkeletalAnimation* anim = new CPlayedMimicSkeletalAnimation( animation, m_baseSkeleton, mimicFaces );
	if ( anim->Play( true, 1.f, 0.f, 0.f, SAT_Normal, false ) )
	{
		m_animations.PushBack( anim );
		return m_animations.Back();
	}

	return NULL;
}*/


