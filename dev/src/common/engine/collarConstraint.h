
#pragma once

#include "animSkeletalDangleConstraint.h"

//////////////////////////////////////////////////////////////////////////
// this constraint assumes that bones rotate along Z axis only, X axis is tangent axis.
// bones for collar:  l_collar_01, l_collar_02, b_collar, r_collar_01, r_collar_02, f_collar

class CAnimDangleConstraint_Collar : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Collar, CAnimSkeletalDangleConstraint, 0 );

private:
	Vector					m_offset;
	Float					m_radius;

	Vector					m_offset2;
	Float					m_radius2;

private:
	Int32					m_headIndex;
	Int32					m_colloar[6];
	Int32					m_colloarParents[6];
	Bool					m_cachedAllBones;

	Matrix					m_localBones[6];

	Vector					m_spherePos;
	Vector					m_spherePos2;
	Vector					m_temp;

public:
	CAnimDangleConstraint_Collar();

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

protected:
	// Returns true if you have cached bones already
	virtual Bool HasCachedBones() const override;

	// Cache your bone index inside. Parent and/or skeleton can be null
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton ) override;

private:
	Matrix CalculateOffsets( const Matrix & boneTM, const Vector & sphPos, Float r, const Vector & sphPos2, Float r2 ) const;
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Collar );
	PARENT_CLASS( CAnimSkeletalDangleConstraint );
	PROPERTY_EDIT( m_offset,   TXT("Offset") );
	PROPERTY_EDIT( m_radius,   TXT("Radius") );
	PROPERTY_EDIT( m_offset2,   TXT("Offset2") );
	PROPERTY_EDIT( m_radius2,   TXT("Radius2") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAnimDangleConstraint_Pusher : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Pusher, CAnimSkeletalDangleConstraint, 0 );

private:
	String					m_boneName;
	String					m_collisionName;
	Vector					m_offset;
	Float					m_radius;
	Float					m_maxAngle;

private:
	Int32					m_index;
	Int32					m_collindex;
	Bool					m_cachedAllBones;
	Vector					m_spherePos;
	Matrix					m_boneTransform;

public:
	CAnimDangleConstraint_Pusher();

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

protected:
	// Returns true if you have cached bones already
	virtual Bool HasCachedBones() const override;

	// Cache your bone index inside. Parent and/or skeleton can be null
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton ) override;

private:
	Matrix CalculateOffsets( const Matrix & boneTM, const Vector & sphPos, Float r ) const;
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Pusher );
PARENT_CLASS( CAnimSkeletalDangleConstraint );
PROPERTY_EDIT( m_boneName,   TXT("ConstraintBone") );
PROPERTY_EDIT( m_collisionName,   TXT("CollisionBone") );
PROPERTY_EDIT( m_offset,   TXT("Offset") );
PROPERTY_EDIT( m_radius,   TXT("Radius") );
PROPERTY_EDIT( m_maxAngle,   TXT("MaximumAngle") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAnimDangleConstraint_NobleDressFix : public IAnimDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_NobleDressFix, IAnimDangleConstraint, 0 );

private:
	String		m_boneNameA;
	String		m_boneNameB;

	EAxis		m_boneAxisA;
	EAxis		m_boneAxisB;

	Float		m_boneValueA;
	Float		m_boneValueB;

private:
	Int32		m_boneIdxA;
	Int32		m_boneIdxB;
	Bool		m_cachedBones;

public:
	CAnimDangleConstraint_NobleDressFix();

	virtual void OnDetached( CWorld* world ) override;

	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) override;
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) override;
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) override;

public:
	virtual Uint32 GetRuntimeCacheIndex() const override;
	virtual const struct SSkeletonSkeletonCacheEntryData* GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const override;

private:
	void AddCorrection( SBehaviorGraphOutput* poseLS );
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_NobleDressFix );
	PARENT_CLASS( IAnimDangleConstraint );
	PROPERTY_EDIT( m_boneNameA, String::EMPTY );
	PROPERTY_EDIT( m_boneNameB, String::EMPTY );
	PROPERTY_EDIT( m_boneAxisA, String::EMPTY );
	PROPERTY_EDIT( m_boneAxisB, String::EMPTY );
	PROPERTY_EDIT( m_boneValueA, String::EMPTY );
	PROPERTY_EDIT( m_boneValueB, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
