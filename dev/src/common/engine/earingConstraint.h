
#pragma once

#include "animSkeletalDangleConstraint.h"

namespace AnimDangleConstraints
{

class bonePendulum
{
public:
	bonePendulum();

	void system_moved_to( const Matrix & np, Float mass );
	void evaluate( Float dt, Matrix & out, Float damp, const Vector& s, Float rad, bool colls ); // this applies movement
	void apply_force( const Vector & a, Float dt, Float mass );

	bool move( Float speed, Float dt );
	void correct_speed();
	Float collision( const Vector & norm, Float k );
	bool  collisions( Float sp, Vector & nor, Float & t, Float & k );

	Vector targetPosition;
	Vector targetVelocity;
	Int32		index;

	Matrix globalPosition;
	Vector globalVelocity;
	Vector globalAcceleration;

	Vector dir1;
	Vector dir2;
	Float  k1;
	Float  k2;

	Vector debug_inertia;
};

// this tracks the specified bones transform, velocity and acceleration

class boneTracer
{
public:
	boneTracer();
	void quaternionToAngleAxis( const RedQuaternion & q, RedVector4 & v, Float dt );
	void attach( const RedVector4 & np, const RedQuaternion & nr );
	void motion( const RedVector4 & np, const RedQuaternion & nr, Float dt );
	RedVector4 forceAtPoint( const RedVector4 & p, Float mulVel, Float mulAcc, Float mulMov );
	RedVector4 m_position;
	RedQuaternion m_rotation;
	RedVector4 m_linearVelocity;
	RedVector4 m_angularVelocity;
	RedVector4 m_linearAcceleration;
	RedVector4 m_angularAcceleration;
	Float		m_delta;
};

}

// this constraint animates additional bones in constraint skeleton

class CAnimDangleConstraint_Earing : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Earing, CAnimSkeletalDangleConstraint, 0 );

private:
	Bool					m_cachedAllBones;

	Float					m_lastDt;

	Vector					m_draw;

	Float					m_dampening;
	Float					m_inertia;
	Float					m_display;
	Bool					m_usecolls;

	Vector					m_collision1;
	Vector					m_collision2;

	AnimDangleConstraints::bonePendulum		m_bones[2];

	AnimDangleConstraints:: boneTracer		m_head_tracer;
	Int32									m_head_index;

public:
	CAnimDangleConstraint_Earing();

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	void DrawPlane( CRenderFrame* frame, const Matrix & m, const Vector & d, Float s, Float k );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

protected:
	// Returns true if you have cached bones already
	virtual Bool HasCachedBones() const;

	// Cache your bone index inside. Parent and/or skeleton can be null
	virtual void CacheBones( const CAnimatedComponent* parent, const CSkeleton* skeleton );

private:
	Matrix CalculateOffsets( const Matrix & boneTM, const Vector & sphPos, Float r ) const;
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Earing );
	PARENT_CLASS( CAnimSkeletalDangleConstraint );
	PROPERTY_EDIT( m_dampening,   TXT("Dampening ( kepp it between 0.9 and 0.999 )") );
	PROPERTY_EDIT( m_inertia,   TXT("Inertia ( kepp it positive, bigger than or equel 1)") );
	PROPERTY_EDIT( m_collision1,   TXT("Collision1") );
	PROPERTY_EDIT( m_collision2,   TXT("Collision2") );
	PROPERTY_EDIT( m_display,   TXT("Display scale") );
	PROPERTY_EDIT( m_usecolls,   TXT("Use Collision") );
END_CLASS_RTTI();
