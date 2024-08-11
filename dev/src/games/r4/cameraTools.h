#pragma once

class CEntity;
class CSkeletalAnimation;
struct SCameraMovementData;
struct SBehaviorUsedAnimationData;

//////////////////////////////////////////////////////////////////////////
//
//	This is a bunch of not-so-related tools, used in camera.
//
//////////////////////////////////////////////////////////////////////////

namespace CamTools
{
	// Returns first recently used anim, that name contains animNameSubstring. 
	const SBehaviorUsedAnimationData* FindRecentlyUsedAnim( const CEntity& entity, const String& animNameSubstring );

	// Returns future entity transform after playing anim from startTime to endTime.
	AnimQsTransform GetFutureTransformBasedOnAnim( const CSkeletalAnimation& anim, Float startTime, Float endTime, const AnimQsTransform currEntityTransform );

	// Gets camera model space transform at selected time from anim.
	AnimQsTransform GetCameraTransformFromAnim( const CSkeletalAnimation& anim, Float time, TDynArray< AnimQsTransform >& bones, TDynArray< Float >& tracks );

	// Returns true if values is between min and max.
	template < typename TYPE > RED_INLINE Bool IsBetween( TYPE value, TYPE min, TYPE max );
}

//////////////////////////////////////////////////////////////////////////
//
// INLINES:
//
//////////////////////////////////////////////////////////////////////////

template < typename TYPE >
Bool CamTools::IsBetween( TYPE value, TYPE min, TYPE max )
{
	return value >= min && value <= max;
}