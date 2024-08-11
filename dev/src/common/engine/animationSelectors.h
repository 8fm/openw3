
#pragma once

#include "animatedIterators.h"
#include "animationGameParams.h"

class AnimationSelector_Trajectory
{
public:
	struct InputData
	{
		Vector					m_pointWS;
		CName					m_tagId;

		InputData() : m_pointWS( Vector::ZERO_3D_POINT ) {}
	};

private:
	TDynArray< const CSkeletalAnimationSetEntry* >	m_animations;
	TDynArray< Vector >								m_points;
	TDynArray< CName >								m_ids;

public:
	void Init( ComponentAnimationIterator& animationIterator );

	const CSkeletalAnimationSetEntry* DoSelect( const InputData& input, const Matrix& localToWorld ) const;
};

//////////////////////////////////////////////////////////////////////////

class AnimationSelector_Blend2
{
public:
	struct InputData
	{
		Vector					m_pointWS;
		CName					m_tagId;

		InputData() : m_pointWS( Vector::ZERO_3D_POINT ) {}
	};

	struct OutputData
	{
		const CSkeletalAnimationSetEntry*	m_animationA;
		const CSkeletalAnimationSetEntry*	m_animationB;
		Float								m_weight;

		OutputData() : m_animationA( NULL ), m_animationB( NULL ), m_weight( 0.f ) {}
	};

protected:
	TDynArray< const CSkeletalAnimationSetEntry* >	m_animations;
	TDynArray< Vector >								m_points;
	TDynArray< CName >								m_ids;

public:
	AnimationSelector_Blend2();

	void Init( ComponentAnimationIterator& animationIterator );

	Bool DoSelect( const InputData& input, const Matrix& localToWorld, OutputData& output ) const;

protected:
	Bool SelectFiltered( const InputData& input, const Matrix& localToWorld, const TDynArray< Uint32 >& indexes, OutputData& output ) const;
};

//////////////////////////////////////////////////////////////////////////

class AnimationSelector_HitPoint
{
public:
	struct InputData
	{
		Vector	m_pointWS;
		//...
	};

private:
	TDynArray< const CSkeletalAnimationSetEntry* >	m_animations;
	TDynArray< Vector >								m_points;

public:
	AnimationSelector_HitPoint();

	void Init( const TDynArray< const CSkeletalAnimationSetEntry* >& animations );
	void Init( ComponentAnimationIterator& animationIterator );

	const CSkeletalAnimationSetEntry* DoSelect( const InputData& input, const Matrix& localToWorld ) const;
};

//////////////////////////////////////////////////////////////////////////

class AnimationSelector_Blend2Direction : public AnimationSelector_Blend2
{
public:
	struct InputData : public AnimationSelector_Blend2::InputData
	{
		Vector					m_directionWS;

		InputData() : m_directionWS( Vector::ZERO_3D_POINT ) {}
	};

protected:
	TDynArray< EAnimationAttackType >	m_types;

public:
	AnimationSelector_Blend2Direction() {};

	void Init( ComponentAnimationIterator& animationIterator );

	Bool DoSelect( const InputData& input, const Matrix& localToWorld, OutputData& output ) const;

private:
	Vector GetDirectionFromType( EAnimationAttackType type ) const;
};