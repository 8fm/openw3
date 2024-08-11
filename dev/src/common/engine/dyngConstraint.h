
#pragma once

#include "animSkeletalDangleConstraint.h"
#include "dyngSimulator.h"
#include "behaviorGraphAnimationNode.h"
#include "animSyncInfo.h"

class CAnimDangleConstraint_Dyng;

//#define DEBUG_ANIM_CONSTRAINTS

//////////////////////////////////////////////////////////////////////////

struct CAnimationSyncAnim
{
	CName		m_animationName;
	CSyncInfo	m_syncInfo;
	Float		m_weight;

	void Reset()
	{
		m_animationName = CName::NONE;
		m_syncInfo = CSyncInfo();
		m_weight = 0.f;
	}

	Bool IsSet() const
	{
		return m_animationName != CName::NONE;
	}

	CAnimationSyncAnim()
	{
		Reset();
	}
};

//////////////////////////////////////////////////////////////////////////

class CAnimationSyncToken_Dyng : public CAnimationSyncToken
{
	DECLARE_RTTI_SIMPLE_CLASS( CAnimationSyncToken_Dyng );

private:
	CAnimDangleConstraint_Dyng*		m_owner;

public: // CAnimationSyncToken
	virtual void Sync( CName animationName, const CSyncInfo& syncInfo, Float weight );
	virtual void Reset();

public:
	void Bind( CAnimDangleConstraint_Dyng* owner );
};

BEGIN_CLASS_RTTI( CAnimationSyncToken_Dyng );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAnimDangleConstraint_Dyng : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Dyng, CAnimSkeletalDangleConstraint, 0 );

private:
	THandle< CDyngResource >	m_dyng;
	THandle< CSkeletalAnimationSet > m_animSet;

	Bool						m_drawlinks;
	Bool						m_drawcolls;
	Bool						m_drawlimits;
	Float						m_dampening;
	Float						m_gravity;
	Float						m_speed;
	Float						m_shake;
	Float						m_wind;
	Int32						m_max_links_iterations;

private:
	Bool						m_cachedAllBones;
	DyngSimulator				m_simulator;
	Bool						m_planeCollision;
	Bool						m_was_cashed;
	Bool						m_forceReset;
	Bool						m_forceRelaxedState;
	Bool						m_useOffsets;
	Float						m_dt;

	CAnimationSyncToken_Dyng*	m_syncToken;
	CAnimationSyncAnim			m_animation;

public:
	CAnimDangleConstraint_Dyng();
	~CAnimDangleConstraint_Dyng();
	Matrix mixTransforms( const Matrix & a, const Matrix & b, Float w );

#ifndef NO_EDITOR
	virtual void OnPropertyPostChange( IProperty* property );
#endif

	void SetResource( CDyngResource* dyngRes );
	const CDyngResource* GetResource() const { return m_dyng; }

#ifndef NO_EDITOR_FRAGMENTS
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
#endif
public:
	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );
	virtual const CSkeleton* GetSkeleton() const;
	virtual void SetBlendToAnimationWeight( Float w ) override;
	virtual void ForceReset() override;
	virtual void ForceResetWithRelaxedState() override;
	virtual void OnShowDebugRender( Bool flag );

public:
	virtual void SetShakeFactor( Float factor ) override;
	virtual void SetGravityFactor( Float factor );

public:
	void Bind( CAnimationSyncToken_Dyng* syncToken );
	void SetAnimation( const CName& animationName, const CSyncInfo& syncInfo, Float weight );

protected:
	virtual Bool HasCachedBones() const override;
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton ) override;

private:
	void RecreateSkeleton();

#ifndef NO_EDITOR
public:
	virtual Bool PrintDebugComment( String& str ) const override;
#endif
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Dyng );
	PARENT_CLASS( CAnimSkeletalDangleConstraint );
	PROPERTY_EDIT( m_dyng, TXT("DyngResource"));
	PROPERTY_EDIT( m_animSet, TXT(""));
	PROPERTY_EDIT( m_drawlinks, TXT("Display Links"));
	PROPERTY_EDIT( m_drawcolls, TXT("Display Collisions"));
	PROPERTY_EDIT( m_drawlimits, TXT("Display Limits"));
	PROPERTY_EDIT( m_dampening, TXT("Dampening"));
	PROPERTY_EDIT( m_gravity,   TXT("Gravity"));
	PROPERTY_EDIT( m_speed,   TXT("Speed"));
	PROPERTY_EDIT( m_planeCollision,   TXT("Plane Collision"));
	PROPERTY_EDIT( m_useOffsets,   TXT("m_useOffsets"));
	PROPERTY_EDIT( m_shake,   TXT("Shaking value"));
	PROPERTY_EDIT( m_wind,   TXT("Wind weight"));
	PROPERTY_EDIT( m_max_links_iterations,   TXT("Maximum number of iterations for link evaluation"));
END_CLASS_RTTI();

