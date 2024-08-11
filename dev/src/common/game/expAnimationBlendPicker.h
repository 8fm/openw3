
#pragma once

#include "expIntarface.h"

struct SBlendPickerAnimDesc
{
	CName	m_animation;
	Float	m_angle;

	SBlendPickerAnimDesc( const CName& animation, Float angle )
		: m_animation( animation )
		, m_angle( angle ) {};

	SBlendPickerAnimDesc()
		: m_animation( CName::NONE )
		, m_angle( 0.f ) {};
};

struct SBlendPickerAnimInfo
{
	CName	m_animation;
	Float	m_angle;
	Vector	m_offset;
	Vector	m_motion;
};

#if 0
RED_MESSAGE("FIXME>>>>>>>>>> Delete or fix: uses Havok")
template< class SelectPolicy >
class CAnimationBlendPicker
{
private:
	const CEntity*						m_entity;
	TDynArray< SBlendPickerAnimInfo >	m_animations;

public:
	CAnimationBlendPicker( const CEntity* entity, const TDynArray< SBlendPickerAnimDesc >& animations, const CName& boneName, Float syncTime )
		: m_entity( entity )
	{
		// Calculate motion data for each animation
		const CAnimatedComponent* root = entity->GetRootAnimatedComponent();
		CSkeletalAnimationContainer* animContainer = root ? root->GetAnimationContainer() : NULL;

		const TDynArray< SBlendPickerAnimDesc >::const_iterator end = animations.End();
		TDynArray< SBlendPickerAnimDesc >::const_iterator it;

		for( it = animations.Begin(); it != end; ++it )
		{
			hkQsTransform offset( hkQsTransform::IDENTITY );
			hkQsTransform motion( hkQsTransform::IDENTITY );

			const CSkeletalAnimationSetEntry* animEntry = animContainer->FindAnimation( it->m_animation );

			if( animEntry )
			{
				const CSkeletalAnimation* anim = animEntry->GetAnimation();
				if( anim )
				{
					RED_ASSERT(anim->IsLoaded(), TXT("Those animations should always be loaded"));
					motion = anim->GetMovementBetweenTime( 0.f, syncTime, 0 );
					SkeletonBonesUtils::CalcBoneOffset( entity, anim, boneName, syncTime, offset );
				}

				SBlendPickerAnimInfo newAnimInfo;
				newAnimInfo.m_animation = it->m_animation;
				newAnimInfo.m_angle = it->m_angle;
				newAnimInfo.m_offset = TO_CONST_VECTOR_REF( offset.m_translation );
				newAnimInfo.m_motion = TO_CONST_VECTOR_REF( motion.m_translation );

				m_animations.PushBack( newAnimInfo );
			}
		}
	}

	typename SelectPolicy::BlendInfo GetBlendInfoForPosition( const Vector& desiredPosition, Float relativeAngle = 0.f ) const
	{
		return SelectPolicy::Pick( m_entity, m_animations, desiredPosition, relativeAngle );
	}
};
#endif // #if 0

//////////////////////////////////////////////////////////////////////////
// Policies
//////////////////////////////////////////////////////////////////////////

class AngleAndYAxisPolicy
{
public:
	struct BlendInfo
	{
		TPair< CName, Float > m_animationBlend[3];
	};

	static BlendInfo Pick( const CEntity* entity, const TDynArray< SBlendPickerAnimInfo >& animations, const Vector& desiredPosition, Float relativeAngle );
};

class YAxisPolicy
{

public:
	struct BlendInfo
	{
		CName	m_animations[2];
		Float	m_blendWeight;
	};

	static BlendInfo Pick( const CEntity* entity, const TDynArray< SBlendPickerAnimInfo >& animations, const Vector& desiredPosition, Float relativeAngle );
};