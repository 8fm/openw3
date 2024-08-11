

#pragma once


#include "randomGenerators.h"
#include "poseHandle.h"

struct CSyncInfo;

class CPlayedSkeletalAnimation;
class CPoseCache;
struct SBehaviorGraphOutput;

struct SAnimationControllerPose
{
	SAnimationControllerPose( CPoseHandle pose );
	~SAnimationControllerPose();

	RED_INLINE Bool IsOk() const { return m_pose != NULL; }

	CPoseHandle	m_pose;
};

//////////////////////////////////////////////////////////////////////////

class IAnimationController : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IAnimationController, CObject );

protected:
	Bool							m_collectEvents;

protected:
	const CSkeleton*				m_skeleton;
	const CSkeletalAnimationSet*	m_set;

public:
	IAnimationController();
	virtual ~IAnimationController();

	virtual Bool Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set );
	virtual void Destroy();

	virtual Bool Update( Float timeDelta ) = 0;
	virtual Bool Sample( const CSkeletalAnimatedComponent * ac, SAnimationControllerPose& pose ) const = 0;

	virtual void CalcBox( Box& box ) const { box = Box( Vector( -0.5f, -0.5f, 0.f ), Vector( 0.5f, 0.5f, 2.f ) ); }

	virtual void SyncTo( const CSyncInfo& info ) {}
	virtual Bool GetSyncInfo( CSyncInfo& info ) { return false; }
	virtual void RandSync() {}

	void CollectEvents( Bool flag );

protected:
	Bool IsCollectingEvents() const;
};

BEGIN_ABSTRACT_CLASS_RTTI( IAnimationController );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_collectEvents, TXT("Collect animation events") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CSingleAnimationController : public IAnimationController
{
	DECLARE_ENGINE_CLASS( CSingleAnimationController, IAnimationController, 0 );

protected:
	CName						m_animationName;

protected:
	CPlayedSkeletalAnimation*	m_animation;

public:
	CSingleAnimationController();

	virtual Bool Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set );
	virtual void Destroy();

	virtual Bool Update( Float timeDelta );
	virtual Bool Sample( const CSkeletalAnimatedComponent * ac, SAnimationControllerPose& pose ) const override;

	virtual void SyncTo( const CSyncInfo& info );
	virtual Bool GetSyncInfo( CSyncInfo& info );
	virtual void RandSync();

	virtual void CalcBox( Box& box ) const;

protected:
	void StopAnimation();

#ifndef NO_EDITOR
public:
	void SetAnimation( const CName& animation ) { m_animationName = animation; }
#endif
};

BEGIN_CLASS_RTTI( CSingleAnimationController );
	PARENT_CLASS( IAnimationController );
	PROPERTY_EDIT( m_animationName, TXT("Animation") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CSequentialAnimationController : public IAnimationController
{
	DECLARE_ENGINE_CLASS( CSequentialAnimationController, IAnimationController, 0 );

protected:
	TDynArray< CName >			m_animations;
	TDynArray< Float >			m_speeds;
	Float						m_startingOffsetRange;
	Float						m_startingOffsetBias;

protected:
	Uint32						m_currIndex;
	CPlayedSkeletalAnimation*	m_animation;

public:
	CSequentialAnimationController();

	virtual Bool Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set );
	virtual void Destroy();

	virtual Bool Update( Float timeDelta );
	virtual Bool Sample( const CSkeletalAnimatedComponent * ac, SAnimationControllerPose& pose ) const override;

public:
	virtual void OnPropertyPostChange( IProperty* property );

protected:
	Bool PlayNextAnimation();
	Bool PlayAnimation( const CName& animName );
	void StopAnimation();

	virtual Bool CalcNextAnim( CName& animName );

	Bool HasSpeedTable() const;
	Float RandSpeed() const;
};

BEGIN_CLASS_RTTI( CSequentialAnimationController );
	PARENT_CLASS( IAnimationController );
	PROPERTY_EDIT( m_animations, TXT("Animations") );
	PROPERTY_EDIT( m_speeds, TXT("Speeds, if array is empty speed for all animations will be 1") );
	PROPERTY_EDIT( m_startingOffsetRange, TXT("") );
	PROPERTY_EDIT( m_startingOffsetBias, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CRandomAnimationController : public CSequentialAnimationController
{
	DECLARE_ENGINE_CLASS( CRandomAnimationController, CSequentialAnimationController, 0 );

protected:
	CRandomIndexPool		m_generator;

public:
	virtual Bool Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set );

protected:
	virtual Bool CalcNextAnim( CName& animName );
};

BEGIN_CLASS_RTTI( CRandomAnimationController );
	PARENT_CLASS( CSequentialAnimationController );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CRandomWithWeightAnimationController : public CSequentialAnimationController
{
	DECLARE_ENGINE_CLASS( CRandomWithWeightAnimationController, CSequentialAnimationController, 0 );

protected:
	TDynArray< Float >					m_weights;

protected:
	virtual Bool CalcNextAnim( CName& animName );

public:
	virtual void OnPropertyPostChange( IProperty* property );

private:
	Uint32 GetRandomIndex( const TDynArray< Float > &weights );
};

BEGIN_CLASS_RTTI( CRandomWithWeightAnimationController );
	PARENT_CLASS( CSequentialAnimationController );
	PROPERTY_EDIT( m_weights, TXT("Weights for random algorithm") );
END_CLASS_RTTI();
