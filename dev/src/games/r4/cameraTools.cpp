#include "build.h"
#include "cameraTools.h"
#include "../../common/game/commonGame.h"
#include "../../common/engine/visualDebug.h"
#include "../../common/engine/skeletalAnimation.h"
#include "../../common/engine/entity.h"
#include "customCameraControllers.h"

//////////////////////////////////////////////////////////////////////////
const SBehaviorUsedAnimationData* CamTools::FindRecentlyUsedAnim( const CEntity& entity, const String& animNameSubstring ) // TODO: need a predicate here
{
	CAnimatedComponent* ac = entity.GetRootAnimatedComponent();

	const SBehaviorUsedAnimations& last_anims = ac->GetRecentlyUsedAnims();

	for ( const SBehaviorUsedAnimationData& data : last_anims.m_anims.m_usedAnims )
	{
		if ( data.m_animation->GetName().AsString().ContainsSubstring( animNameSubstring ) ) 
		{
			return &data;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
AnimQsTransform CamTools::GetFutureTransformBasedOnAnim( const CSkeletalAnimation& anim, Float startTime, Float endTime, const AnimQsTransform currEntityTransform )
{
	RED_ASSERT( endTime > startTime );
	RED_ASSERT( endTime <= anim.GetDuration() );

	AnimQsTransform anim_movement = anim.GetMovementBetweenTime( startTime, endTime, false );

	AnimQsTransform futureTransform;
	futureTransform.SetMul( currEntityTransform, anim_movement );

	return futureTransform;
}

//////////////////////////////////////////////////////////////////////////
AnimQsTransform CamTools::GetCameraTransformFromAnim( const CSkeletalAnimation& anim, Float time, TDynArray< AnimQsTransform >& bones, TDynArray< Float >& tracks )
{
	anim.Sample( time, bones, tracks );
	ASSERT( !bones.Empty() );

	Uint32 currBone = bones.Size() - 1;
	AnimQsTransform transform_from_anim = bones[currBone];
	--currBone;

	while( currBone != -1 )
	{			
		transform_from_anim.SetMul( bones[currBone], transform_from_anim );
		--currBone;
	}

	return transform_from_anim;
}