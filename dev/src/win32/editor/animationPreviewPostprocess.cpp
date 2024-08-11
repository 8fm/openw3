
#include "build.h"
#include "animationPreviewPostprocess.h"
#include "animationTrajectoryUtils.h"
#include "../../common/engine/animationTrajectory.h"
#include "../../common/engine/animationGameParams.h"
#include "../../common/engine/skeletalAnimationEntry.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/renderFrame.h"

CEdAnimationPreviewPostProcessTrajectory::CEdAnimationPreviewPostProcessTrajectory()
{

}

void CEdAnimationPreviewPostProcessTrajectory::OnPreviewPostProcessGenerateFragments( CRenderFrame *frame, const CAnimatedComponent* componenet, const CPlayedSkeletalAnimation* animation )
{
	if ( animation && componenet )
	{
		const CSkeletalAnimationSetEntry* animationEntry = animation->GetAnimationEntry();
		if ( animationEntry )
		{
			const CSkeletalAnimationTrajectoryParam* param = animationEntry->FindParam< CSkeletalAnimationTrajectoryParam >();
			if ( param && param->IsParamValid() && componenet )
			{
				AnimationTrajectoryVisualizer::DrawTrajectoryMSinWSWithPtrO( frame, param->GetData(), componenet->GetLocalToWorld(), animation->GetTime(), animation->GetDuration() );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CEdAnimationPreviewPostSkeleton::CEdAnimationPreviewPostSkeleton( const Color& color )
	: m_color( color )
{

}

void CEdAnimationPreviewPostSkeleton::OnPreviewPostProcessGenerateFragments( CRenderFrame *frame, const CAnimatedComponent* componenet, const CPlayedSkeletalAnimation* animation )
{
	if ( componenet )
	{
		SkeletonRenderingUtils::DrawSkeleton( componenet, m_color, frame );
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimationPreviewPostProcessHit::OnPreviewPostProcessGenerateFragments( CRenderFrame *frame, const CAnimatedComponent* componenet, const CPlayedSkeletalAnimation* animation )
{
	if ( animation && componenet )
	{
		const CSkeletalAnimationSetEntry* animationEntry = animation->GetAnimationEntry();
		if ( animationEntry )
		{
			const CSkeletalAnimationHitParam* param = animationEntry->FindParam< CSkeletalAnimationHitParam >();
			if ( param && param->IsParamValid() && componenet )
			{
				Vector pointWS = componenet->GetLocalToWorld().TransformVector( param->GetPointMS() );
				Vector dirWS = componenet->GetLocalToWorld().TransformVector( param->GetDirectionMS() );
				
				Matrix mat( Matrix::IDENTITY );
				mat.SetTranslation( pointWS );
				frame->AddDebugArrow( mat, dirWS, 1.f, Color( 255, 0, 0 ) );
			}
		}
	}
}
