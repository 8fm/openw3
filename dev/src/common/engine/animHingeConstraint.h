
#pragma once

#include "animSkeletalDangleConstraint.h"


class hinge_simulator
{
public:
	hinge_simulator()
	{
		x = 0.0f;
		v = 0.0f;
		upperBound = 0.5f; 
		lowwerBound = -0.5f;	
		m_index = -1;
		m_parentIndex = -1;
		bounceMul = 0.9f;
		deampMul = 0.99f;
		limit = 20.0f;
		inertia = 1.0f;
		gravity = 1.0f;
		spring = 0.0f;
	}
	void Setup( int ind, int pind, Matrix & loc )
	{
		m_index = ind;
		m_parentIndex = pind;
		m_localTransform = loc;
	}
	Matrix Evaluate( Float dt, const Matrix & ntransform, bool res = false ) // gravity point and force applied from outside
	{
		if( dt>0.0000001f )
		{
			Float dtc = dt>2.0f ? 2.0f : dt;
			Vector vel = (ntransform.GetTranslation() - position)*0.001f*inertia;
			position = ntransform.GetTranslation();
			Vector acc = Vector(0.0f,0.0f,-0.00327f*gravity) + (vel/-dtc);
			Float accx = Vector::Dot3( acc, ntransform.GetRow(0) );
			Float accy = Vector::Dot3( acc, ntransform.GetRow(1) );
			Vector p( -sinf(x), cosf(x), 0.0f );
			a = accx*-sinf(x) + accy*cosf(x);
			if( !res )
			{
				v += a + ( -x*0.01f*spring );
			}
			else
			{
				reset();
			}
			v *= deampMul;
			if( v>limit ){ v = limit; }
			if( v<-limit ){ v = -limit; }

			
			move(v*dtc);
			//move( v );
		}
		Matrix mat = Matrix::IDENTITY;
		mat.V[0].A[0] = cosf(x);
		mat.V[0].A[1] = sinf(x);

		mat.V[1].A[0] = -sinf(x);
		mat.V[1].A[1] = cosf(x);

		return mat;
	}
	inline void reset()
	{
		//x = 0.0f;
		v = 0.0f;
	}
	inline Int32 getIndex(){ return m_index; }
	inline Int32 getParentIndex(){ return m_parentIndex; }
	const Matrix & getLocalTransform(){ return m_localTransform; }
	inline Float GetLowwerBound(){ return lowwerBound; }
	inline Float GetUpperBound(){ return upperBound; }
	inline Float GetValue(){ return x; } 
	inline Float GetAcc(){ return a; }
	inline void Values( Float lim, Float bonc, Float damp, Float min, Float max, Float inert, Float grav, Float spr )
	{
		inertia = inert;
		limit = lim;
		bounceMul = bonc;
		deampMul = damp;
		lowwerBound = min;
		upperBound = max;
		gravity = grav;
		spring = spr;

		lowwerBound = lowwerBound<-3.1416f ? -3.1416f : lowwerBound;
		lowwerBound = lowwerBound>0.0f ? 0.0f : lowwerBound;

		upperBound = upperBound>3.1416f ? 3.1416f : upperBound;
		upperBound = upperBound<0.1f ? 0.1f : upperBound;
	}
private:
	void move( Float s )
	{
		if( fabs(s)<0.00000001f ){ return; }
		Float t = 0.0f;
		if( s>0.0f )
		{
			t = upperBound - x;
			if( t<=s )
			{
				x += t;
				v *= -bounceMul; 
				move( (t-s) );
			}
			else
			{
				x += s;
			}
		}
		else
		{
			t = lowwerBound - x;
			if( fabs(t)<=fabs(s) )
			{
				x += t;
				v *= -bounceMul;
				move( (t-s) );
			}
			else
			{
				x += s;
			}
		}

	}
	Float x; // position
	Float v; // velocity
	Float a; // acceleration

	Float upperBound; 
	Float lowwerBound;

	Vector position;

	Int32 m_index;
	Int32 m_parentIndex;
	Matrix m_localTransform;
	Float bounceMul;
	Float deampMul;
	Float limit;
	Float inertia;
	Float gravity;
	Float spring;
};


class CAnimDangleConstraint_Hinge : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Hinge, CAnimSkeletalDangleConstraint, 0 );

private:
	Float					m_radius;
	Float					m_limit;
	Float					m_bounce;
	Float					m_damp;
	Float					m_min;
	Float					m_max;
	Float					m_inertia;
	String					m_name;
	Float					m_gravity;
	Float					m_spring;
	Bool					m_forceReset;

private:
	Bool					m_cachedAllBones;
	hinge_simulator			m_hinge;
	Float					m_dt;
public:
	CAnimDangleConstraint_Hinge();


	void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );
	void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones );

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
#ifndef NO_EDITOR
	virtual void OnPropertyPostChange( IProperty* property );
#endif
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

	void draw_ellipse( const Matrix & mat, Float from, Float to, CRenderFrame* frame, Float scale );
protected:
	// Returns true if you have cached bones already
	virtual Bool HasCachedBones() const override;

	// Cache your bone index inside. Parent and/or skeleton can be null
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton ) override;
public:
	virtual void ForceReset() override;
	virtual void ForceResetWithRelaxedState() override;
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Hinge );
PARENT_CLASS( CAnimSkeletalDangleConstraint );
PROPERTY_EDIT( m_name,   TXT("Name") );
PROPERTY_EDIT( m_radius,   TXT("Dont Ask") );
PROPERTY_EDIT( m_limit,   TXT("Max Speed") );
PROPERTY_EDIT( m_bounce,   TXT("Bounce Factor") );
PROPERTY_EDIT( m_damp,   TXT("Dampening Factor") );
PROPERTY_EDIT( m_min,   TXT("Lowwer Bound: 0.0 - -PI") );
PROPERTY_EDIT( m_max,   TXT("Upper Bound: 0.1 - +PI") );
PROPERTY_EDIT( m_inertia,   TXT("Inertia") );
PROPERTY_EDIT( m_gravity,   TXT("Gravity") );
PROPERTY_EDIT( m_spring,   TXT("Spring") );
END_CLASS_RTTI();
