
#include "build.h"
#include "expAnimationBlendPicker.h"
#include "expEvents.h"

//////////////////////////////////////////////////////////////////////////
// Policies
//////////////////////////////////////////////////////////////////////////

AngleAndYAxisPolicy::BlendInfo AngleAndYAxisPolicy::Pick( const CEntity* entity, const TDynArray< SBlendPickerAnimInfo >& animations, const Vector& desiredPosition, Float relativeAngle )
{
	struct AYPAnimInfo
	{
		const SBlendPickerAnimInfo* m_animInfo;
		Float						m_angleDiff;
		Float						m_distDiff;

		AYPAnimInfo() : m_animInfo( NULL ), m_angleDiff( FLT_MAX ), m_distDiff( FLT_MAX ) {};
	};

	AYPAnimInfo closestLeftBack;
	AYPAnimInfo closestLeftFront;
	AYPAnimInfo closestRightBack;
	AYPAnimInfo closestRightFront;

	const Vector& entPos = entity->GetWorldPositionRef();
	const Float currentDist = MAbs( desiredPosition.Y - entPos.Y );

	const TDynArray< SBlendPickerAnimInfo >::const_iterator end = animations.End();
	TDynArray< SBlendPickerAnimInfo >::const_iterator it;

	for( it = animations.Begin(); it != end; ++it )
	{
		const Float animDist = MAbs( it->m_offset.Y + it->m_motion.Y );
		const Float distDiff = animDist - currentDist;
		const Float angleDiff = it->m_angle - relativeAngle;

		if( distDiff >= 0 )
		{
			//closestRightFront
			if( angleDiff >= 0 )
			{
				if( closestRightFront.m_angleDiff >= MAbs( angleDiff ) )
				{
					closestRightFront.m_animInfo = &( *it );
					closestRightFront.m_angleDiff = angleDiff;
					closestRightFront.m_distDiff = distDiff;
				}
			}
			//closestLeftFront
			else
			{
				if( closestLeftFront.m_angleDiff >= MAbs( angleDiff ) )
				{
					closestLeftFront.m_animInfo = &( *it );
					closestLeftFront.m_angleDiff = angleDiff;
					closestLeftFront.m_distDiff = distDiff;
				}
			}
		}
		else
		{
			//closestRightBack
			if( angleDiff >= 0 )
			{
				if( closestRightBack.m_angleDiff >= MAbs( angleDiff ) )
				{
					closestRightBack.m_animInfo = &( *it );
					closestRightBack.m_angleDiff = angleDiff;
					closestRightBack.m_distDiff = distDiff;
				}
			}
			//closestLeftBack
			else
			{
				if( closestLeftBack.m_angleDiff >= MAbs( angleDiff ) )
				{
					closestLeftBack.m_animInfo = &( *it );
					closestLeftBack.m_angleDiff = angleDiff;
					closestLeftBack.m_distDiff = distDiff;
				}
			}
		}
	}

	BlendInfo output;
	output.m_animationBlend[0].m_first = closestLeftBack.m_animInfo->m_animation;
	output.m_animationBlend[1].m_first = closestRightFront.m_animInfo->m_animation;

	const Float halfWay = (closestLeftBack.m_animInfo->m_angle + closestRightBack.m_animInfo->m_angle) / 2.f;
	output.m_animationBlend[2].m_first = relativeAngle < halfWay ? closestLeftFront.m_animInfo->m_animation : closestRightBack.m_animInfo->m_animation;

	return output;
}

YAxisPolicy::BlendInfo YAxisPolicy::Pick( const CEntity* entity, const TDynArray< SBlendPickerAnimInfo >& animations, const Vector& desiredPosition, Float relativeAngle )
{
	TPair< const SBlendPickerAnimInfo*, Float > closestBack( nullptr, FLT_MAX );
	TPair< const SBlendPickerAnimInfo*, Float > closestFront( nullptr, FLT_MAX );

	const Vector& entPos = entity->GetWorldPositionRef();
	const Float entDist = MAbs( desiredPosition.Y - entPos.Y );

	const TDynArray< SBlendPickerAnimInfo >::const_iterator end = animations.End();
	TDynArray< SBlendPickerAnimInfo >::const_iterator it;

	for( it = animations.Begin(); it != end; ++it )
	{
		const Float animDist = MAbs( it->m_offset.Y + it->m_motion.Y );
		const Float distDiff = animDist - entDist;

		if( distDiff >= 0 && distDiff < closestFront.m_second )
		{
			closestFront.m_first = &( *it );
			closestFront.m_second = distDiff;
		}
		else if( MAbs(distDiff) < MAbs(closestBack.m_second) )
		{
			closestBack.m_first = &( *it );
			closestBack.m_second = distDiff;
		}
	}

	ASSERT( closestBack.m_first && closestFront.m_first );

	BlendInfo output;

	// If the position is out of the ranges of animations just return the best one
	if( !closestFront.m_first )
	{
		output.m_animations[0] = closestBack.m_first->m_animation;
		output.m_animations[1] = CName::NONE;
		output.m_blendWeight = 1.f;
	}
	else if( !closestBack.m_first )
	{
		output.m_animations[0] = closestFront.m_first->m_animation;
		output.m_animations[1] = CName::NONE;
		output.m_blendWeight = 1.f;
	}
	else
	{
		output.m_animations[0] = closestBack.m_first ? closestBack.m_first->m_animation : CName::NONE;
		output.m_animations[1] = closestFront.m_first ? closestFront.m_first->m_animation : CName::NONE;

		const Float backAnimDist = entDist + closestBack.m_second;
		const Float frontAnimDist = entDist + closestFront.m_second;

		const Float length = frontAnimDist - backAnimDist;
		output.m_blendWeight = 1.f - ( (entDist - backAnimDist) / length );
	}

	ASSERT( output.m_blendWeight >= 0.f && output.m_blendWeight <= 1.f );

	return output;
}
