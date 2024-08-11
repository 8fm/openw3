
#pragma once

#include "animationTrajectory.h"
#include "skeletalAnimationEntry.h"

class CSkeletalAnimationTrajectoryParam : public ISkeletalAnimationSetEntryParam
{
	DECLARE_RTTI_SIMPLE_CLASS( CSkeletalAnimationTrajectoryParam );

	AnimationTrajectoryData		m_data;
	CName						m_tagId;

public:
	virtual void OnSerialize( IFile& file );

	Bool IsParamValid() const;

	Vector GetSyncPointMS() const;

	const AnimationTrajectoryData& GetData() const;

#ifndef NO_EDITOR
	void Init( AnimationTrajectoryData& data );
#endif

	virtual Bool EditorOnly() const override { return true; }
};

BEGIN_CLASS_RTTI( CSkeletalAnimationTrajectoryParam );
	PARENT_CLASS( ISkeletalAnimationSetEntryParam );
	PROPERTY_EDIT( m_tagId, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EAnimationAttackType
{
	AAT_None,
	AAT_Jab,
	AAT_46,		// the numbers indicate the direction as seen on the NumPad of your keyboard
	AAT_64,
	AAT_82,
	AAT_28,
	AAT_73,
	AAT_91,
	AAT_19,
	AAT_37,
	AAT_Last,
};

BEGIN_ENUM_RTTI( EAnimationAttackType );
	ENUM_OPTION( AAT_None );
	ENUM_OPTION( AAT_Jab );
	ENUM_OPTION( AAT_46 );
	ENUM_OPTION( AAT_64 );
	ENUM_OPTION( AAT_82 );
	ENUM_OPTION( AAT_28 );
	ENUM_OPTION( AAT_73 );
	ENUM_OPTION( AAT_91 );
	ENUM_OPTION( AAT_19 );
	ENUM_OPTION( AAT_37 );
END_ENUM_RTTI();

class CSkeletalAnimationAttackTrajectoryParam : public ISkeletalAnimationSetEntryParam
{
	DECLARE_RTTI_SIMPLE_CLASS( CSkeletalAnimationAttackTrajectoryParam );

	AnimationTrajectoryData		m_dataL;
	AnimationTrajectoryData		m_dataR;

	CName						m_tagId;
	EAnimationAttackType		m_type;

	Float						m_hitDuration;
	Float						m_postHitEnd;
	Float						m_slowMotionStart;
	Float						m_slowMotionEnd;
	Float						m_dampOutEnd;
	Float						m_slowMotionTimeFactor;

public:
	CSkeletalAnimationAttackTrajectoryParam();

	virtual void OnSerialize( IFile& file );

	Bool IsParamValid() const;

	Bool GetSyncPointLeftMS( Vector& point ) const;
	Bool GetSyncPointRightMS( Vector& point ) const;

	const AnimationTrajectoryData& GetDataL() const;
	const AnimationTrajectoryData& GetDataR() const;

	void GetTagId( CName& tagId ) const;

	RED_INLINE EAnimationAttackType GetAttackType() const { return m_type; }

	RED_INLINE Float GetHitDuration()	const			{ return m_hitDuration; }
	RED_INLINE Float GetPostHitEnd() const			{ return m_postHitEnd; }
	RED_INLINE Float GetSlowMotionTimeStart() const	{ return m_slowMotionStart; }
	RED_INLINE Float GetSlowMotionTimeEnd() const		{ return m_slowMotionEnd; }
	RED_INLINE Float GetDampOutEnd() const			{ return m_dampOutEnd; }
	RED_INLINE Float GetTimeFactor() const			{ return m_slowMotionTimeFactor; }

	virtual Bool EditorOnly() const override { return true; }

#ifndef NO_EDITOR
	void Init( AnimationTrajectoryData& dataL, AnimationTrajectoryData& dataR );
#endif
};

BEGIN_CLASS_RTTI( CSkeletalAnimationAttackTrajectoryParam );
	PARENT_CLASS( ISkeletalAnimationSetEntryParam );
	PROPERTY_EDIT( m_tagId, String::EMPTY );
	PROPERTY_EDIT( m_type, String::EMPTY );
	PROPERTY_EDIT( m_slowMotionTimeFactor, String::EMPTY );
	PROPERTY_EDIT( m_hitDuration, String::EMPTY );
	PROPERTY_EDIT( m_postHitEnd, String::EMPTY );
	PROPERTY_EDIT( m_slowMotionStart, String::EMPTY );
	PROPERTY_EDIT( m_slowMotionEnd, String::EMPTY );
	PROPERTY_EDIT( m_dampOutEnd, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CSkeletalAnimationHitParam : public ISkeletalAnimationSetEntryParam
{
	DECLARE_RTTI_SIMPLE_CLASS( CSkeletalAnimationHitParam );

	Vector	m_pointMS;
	Vector	m_directionMS;
	CName	m_boneName;

public:
	CSkeletalAnimationHitParam();

	virtual void OnSerialize( IFile& file );

	Bool IsParamValid() const;

	const Vector& GetPointMS() const;
	const Vector& GetDirectionMS() const;

	virtual Bool EditorOnly() const override { return true; }

#ifndef NO_EDITOR
	struct InitData
	{
		Vector	m_pointMS;
		Vector	m_directionMS;
		CName	m_boneName;
	};

	void Init( const InitData& init );
#endif
};

BEGIN_CLASS_RTTI( CSkeletalAnimationHitParam );
	PARENT_CLASS( ISkeletalAnimationSetEntryParam );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
