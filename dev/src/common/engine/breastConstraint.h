
#pragma once

#include "animSkeletalDangleConstraint.h"
#include "breastConstraintPresets.h"

namespace AnimDangleConstraints
{

class simulator
{
private:
	Float				Collision();
	void				Bounce( Float nx, Float ny );
	void				MovePoint( Float dt );
	Float				CalcPotential();
	void				CalcNormal( Float &nx, Float &ny );

public:
	simulator();
	void				ApplyAcc( Float gx, Float gy );
	void				Simulate( Float dt );
public:

	Vector			m_shape;
	Vector			m_pointPosVel;
	Float			m_velClamp;
	Float			m_velDamp;
	Float			m_bounceDamp;
	Float			m_a;
	Float			m_blackHoleGravity;
	Float			m_gravity;
	Float			m_translationWeight;
	Float			m_rotationWeight;
	Float			m_inertiaScale;
	Float			m_simPointOffset;
};

class BoneSimulator
{
public:
	BoneSimulator();
	~BoneSimulator();

	void set( const Matrix & loc, Int32 ind, Int32 par );				// do it on cash
	void evaluate( Matrix & out, const Matrix & parMS, Float dt, bool reset, Float startSimPointOffset, Int32 numIt );		// calculate
	void Reset( const Matrix& loc );
	
	inline void params( const Vector & ela, Float veldamp, Float boncdamp, Float inacc, Float blak, Float clamp, Float grav, Float movw, Float rotw, Float inert, Float off )
	{
		m_simulator.m_shape = ela;
		m_simulator.m_velClamp = clamp;
		m_simulator.m_velDamp = veldamp;
		m_simulator.m_bounceDamp = boncdamp;
		m_simulator.m_blackHoleGravity = blak;
		m_simulator.m_a = inacc;
		m_simulator.m_gravity = grav;
		m_simulator.m_translationWeight = movw;
		m_simulator.m_rotationWeight = rotw;
		m_simulator.m_inertiaScale = inert;
		m_simulator.m_simPointOffset = off;
	}

	inline void load_preset( int n )
	{
		m_simulator.m_shape.X = breastPreset[n].elAX;
		m_simulator.m_shape.Y = breastPreset[n].elAY;
		m_simulator.m_shape.Z = breastPreset[n].elAZ;
		m_simulator.m_shape.W = breastPreset[n].elAW;
		m_simulator.m_velDamp = breastPreset[n].velDamp;
		m_simulator.m_bounceDamp = breastPreset[n].bounceDamp;
		m_simulator.m_a = breastPreset[n].inAcc;
		m_simulator.m_inertiaScale = breastPreset[n].inertiaScaler;
		m_simulator.m_blackHoleGravity= breastPreset[n].blackHole;
		m_simulator.m_velClamp = breastPreset[n].velClamp;
		m_simulator.m_gravity = breastPreset[n].grav;
		m_simulator.m_translationWeight = breastPreset[n].moveWeight;
		m_simulator.m_rotationWeight = breastPreset[n].rotWeight;
		// p_v14 is for sim dt time
		m_simulator.m_simPointOffset = breastPreset[n].offset;
	}
	
	Matrix								m_local;						// local space of breast bone in the skeleton
	Matrix								m_global;						// global space of breast for drawing ( before calculation )
	Int32								m_index;						// index of bone in skeleton
	Int32								m_parent_index;					// index of parent bone
	AnimDangleConstraints::simulator	m_simulator;					// 2d simulator
	Matrix								m_screen;						// screen matrix in bone space
	Vector								m_target;						// target
	Vector								m_gravity;						// gravity
	Vector								m_position;
	Vector								m_velocity;
	Vector								m_acceleration;
	Vector								m_inertia;
};
}


class CAnimDangleConstraint_Breast : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Breast, CAnimSkeletalDangleConstraint, 0 );

public:
	CAnimDangleConstraint_Breast();

#ifndef NO_EDITOR
	virtual void OnPropertyPostChange( IProperty* property );
#endif
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );
	
protected:
	virtual Bool HasCachedBones() const override;
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton ) override;

public:
	virtual void SetBlendToAnimationWeight( Float w ) override;
	virtual void ForceReset() override;
	virtual void ForceResetWithRelaxedState() override;

protected:
	void DrawEllipse( const Matrix & mat, Float scaleX, Float scaleY, Float posX, Float posY, CRenderFrame* frame );
	void SetPresetValues( Uint32 p );

private:
	Bool											m_cachedAllBones;
	Float											m_dt;

	EBreastPreset									m_preset;
	AnimDangleConstraints::BoneSimulator			m_bones[2];
	Vector											m_elA;
	Vector											m_elB;
	Float											m_simTime;
	Float											m_velDamp;
	Float											m_bounceDamp;
	Float											m_inAcc;
	Float											m_inertiaScaler;
	Float											m_blackHole;
	Float											m_velClamp;
	Float											m_gravity;
	Float											m_movementBoneWeight;
	Float											m_rotationBoneWeight;
	Float											m_startSimPointOffset;
	Matrix											m_parentWS;
	Bool											m_forceRelaxedState;
	Bool											m_forceReset;
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Breast );
	PARENT_CLASS( CAnimSkeletalDangleConstraint );
	PROPERTY_CUSTOM_EDIT( m_preset, TXT("Breast preset behavior"), TXT("EdEnumRefreshableEditor") );
	PROPERTY_EDIT( m_simTime, TXT("simTimeDT") );
	PROPERTY_EDIT( m_elA, TXT("elipse posX, posY, scaleX, scaleY") );
	PROPERTY_EDIT( m_startSimPointOffset, TXT(" simulation point start offset OY axis") );
	PROPERTY_EDIT( m_velDamp, TXT("velocity damper") );
	PROPERTY_EDIT( m_bounceDamp, TXT("bounce damper") );
	PROPERTY_EDIT( m_inAcc, TXT("outside acceleration") );
	PROPERTY_EDIT( m_inertiaScaler, TXT("inertia scaler") );
	PROPERTY_EDIT( m_blackHole, TXT("origin gravity") );
	PROPERTY_EDIT( m_velClamp, TXT("max speed clamper") );
	PROPERTY_EDIT( m_gravity, TXT("gravity") );
	PROPERTY_EDIT( m_movementBoneWeight, TXT("allow bone to move") );
	PROPERTY_EDIT( m_rotationBoneWeight, TXT("allow bone to rotate") );
END_CLASS_RTTI();
