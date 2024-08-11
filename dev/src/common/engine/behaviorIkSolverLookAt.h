#include "behaviorIncludes.h"
/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

enum ELookAtSolverType
{
	LST_Quat,
	LST_Euler
};

BEGIN_ENUM_RTTI( ELookAtSolverType );
	ENUM_OPTION( LST_Quat );
	ENUM_OPTION( LST_Euler );
END_ENUM_RTTI();

class IQuatLookAtSolver
{
protected:

	struct SolverData
	{
		Float							m_weight;
		AnimVector4						m_targetMS;
		AnimQsTransform					m_boneParentMS;
		AnimQsTransform					m_inOutBoneMS;
		//Int32								m_bone;
		EAxis							m_forward;
		EAxis							m_up;
		Float							m_horizontalLimit;
		Float							m_upLimit;
		Float							m_downLimit;
		const Vector*					m_localOffset;
#ifdef USE_HAVOK_ANIMATION
		hkaLookAtIkSolver::Setup		m_setup;
		hkaLookAtIkSolver::RangeLimits	m_limits;
#endif
		SolverData() : m_localOffset( NULL ) {}
	};

	//! Solver returns false if the target is over limits 
	Bool Solve( SolverData& data, AnimQsTransform& boneOutLS ) const;

	//! Setup solver
	Bool SetupSolver( SolverData& data ) const;
};

class IEulerLookAtSolver
{
protected:

	struct SolverData
	{
		Float							m_weight;
		AnimVector4						m_targetMS;
		AnimQsTransform					m_boneParentMS;
		AnimQsTransform					m_boneInLS;
		const Vector*					m_boneDirLS;
		Float							m_horizontalLimit;
		Float							m_upLimit;
		Float							m_downLimit;
#ifdef USE_HAVOK_ANIMATION
		hkaLookAtIkSolver::Setup		m_setup;
		hkaLookAtIkSolver::RangeLimits	m_limits;
#endif
		SolverData() : m_boneDirLS( NULL ) {}
	};

	//! Solver returns false if the target is over limits 
	Bool Solve( SolverData& data, AnimQsTransform& boneOutLS ) const;
};

//////////////////////////////////////////////////////////////////////////

class IQuatLookAtChainSolver : public IQuatLookAtSolver
{
protected:

	struct ChainSolverData
	{
		AnimVector4						m_targetMS[ 3 ];
		AnimQsTransform					m_firstBoneParentMS;
		AnimQsTransform					m_firstBoneLS;
		AnimQsTransform					m_secondBoneLS;
		AnimQsTransform					m_thirdBoneLS;

		const Vector*					m_localOffsetForLastBone;

		Vector							m_weightsScale;

		IQuatLookAtSolver::SolverData	m_boneData[ 3 ];

		ChainSolverData() : m_localOffsetForLastBone( NULL ) {}
	};

	//! Solver returns false if the target is over limits. Bones out - first, second, third 
	Bool Solve( ChainSolverData& data, AnimQsTransform bonesOut[ 3 ] ) const;

	//! Setup internal data
	Bool SetupInternalData( ChainSolverData& data, Uint32 num, const AnimQsTransform& boneMS, const AnimQsTransform& parentBoneMS, const Vector* localOffset ) const;

private:
	static Bool SolveRoll( Float gain, const AnimVector4& targetMS, AnimQsTransform& boneModelSpaceInOut );

	/// Wrapper for atan2 function
	static Float lookAtAtan2( Float y, Float x );

	/// Converts from spherical to cartesian coordinates
	/// Like a globe: X axis near London, Y axis near China, Z axis through the North Pole
	/// XYZ <- rho theta phi (radius, longitude, lattitude )
	static void cartesianFromSpherical( AnimVector4& sphericalInCartesianOut );

	/// Converts from cartesian to spherical coordinates
	/// Like a globe: X axis near London, Y axis near China, Z axis through the North Pole
	/// XYZ -> rho theta phi (radius, longitude, lattitude )
	static void sphericalFromCartesian( AnimVector4& cartesianInSphericalOut );

#ifdef USE_HAVOK_ANIMATION
	/// Solves the specified lookAt constraint. Check the documentation for hkaLookAtIkSolver for details.
	static hkBool solve ( const hkaLookAtIkSolver::Setup& setup, const hkVector4& targetMS, hkReal gain, hkQsTransform& boneModelSpaceInOut, const hkaLookAtIkSolver::RangeLimits* limits = HK_NULL, hkReal limitEps = 0.001f );
#else
	/// Solves the specified lookAt constraint. Check the documentation for hkaLookAtIkSolver for details.
	//static hkBool solve ( const hkaLookAtIkSolver::Setup& setup, const hkVector4& targetMS, hkReal gain, hkQsTransform& boneModelSpaceInOut, const hkaLookAtIkSolver::RangeLimits* limits = HK_NULL, hkReal limitEps = 0.001f );
#endif

	//////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////

class IEulerLookAtChainSolver
{
protected:

	struct SolverData
	{
		Float							m_weight;

		AnimVector4						m_targetMS;

		AnimQsTransform					m_firstBoneParentMS;
		AnimQsTransform					m_firstBoneLS;
		AnimQsTransform					m_secondBoneLS;
		AnimQsTransform					m_thirdBoneLS;
		AnimQsTransform					m_fourthBoneLS;
		AnimQsTransform					m_fourthBoneMS;
		Vector							m_weightsScale;
		Vector							m_weightsBlend;
	};

	//! Solver returns false if the target is over limits. Bones out - first, second, third 
	Bool Solve( const SolverData& data, AnimQsTransform* bonesOut ) const;
};

//////////////////////////////////////////////////////////////////////////

class IConstraintTarget
{
protected:
	//! Linear damp
	void ApplyLinearDamp( const Vector& /*target*/, Vector& /*clampedTargetInOut*/, Float /*speed*/, Float /*dt*/ ) const
	{
		ASSERT( !TXT("Not implemented") );
	}

	// Progressive damp
	void ApplyProgressiveDamp( const Vector& target, Vector& clampedTargetInOut, Float speed, Float dt ) const
	{
		Vector dr = target - clampedTargetInOut;

		Float step = Clamp( speed * dt, 0.f, 1.f );
		
		dr.Mul3( step );

		clampedTargetInOut += dr;

		//clampedTargetInOut.A[0] = Clamp( clampedTargetInOut.A[0], -target.A[0], target.A[0] );
		//clampedTargetInOut.A[1] = Clamp( clampedTargetInOut.A[1], -target.A[1], target.A[1] );
		//clampedTargetInOut.A[2] = Clamp( clampedTargetInOut.A[2], -target.A[2], target.A[2] );
	}

	//! Dead zone. Param deadZone [m]
	void ApplyDeadZone( const Vector& target, Vector& clampedTargetInOut, Float deadZone ) const
	{
		Vector dir = clampedTargetInOut - target;

		Float lenght = dir.Normalize3();
		
		dir.Mul3( deadZone );

		if ( lenght > deadZone )
		{
			clampedTargetInOut = target + dir;
		}
	}

	//! Dead zone snapping. Param distMax [m]
	Bool ApplyDeadZoneSnapping( const Vector& target, Vector& clampedTargetInOut, Float distMax, Float rangeStart ) const
	{
		/*
		c.pos=b.pos
		local yy = abs( b.pos.y )
		local w= 0.5 * tanh( ( yy-b.pos.x*0.2 )*2 ) +0.5 -- x DEG2RAD
		c.pos.y*=w

		local x = c.pos.y
		local y = 70.0/cosh(x) -- x DEG2RAD
		if c.pos.x<y then c.pos.x=y
		*/

		Bool limit = false;

		Vector tergetCM = target * 100.f;

		clampedTargetInOut = tergetCM;

		/*
		Clamp target to line from eyes
		static Float temp = 2.f;
		static Float temp2 = 0.4f;

		Float arg = ( MAbs( tergetCM.X ) - tergetCM.Y * temp2 ) * temp;
		Float w = .5f * tanh( DEG2RAD( arg ) ) + 0.5f;
		
		clampedTargetInOut.X *= w;*/
		
		Float loc = 100.f * distMax / cosh( DEG2RAD( clampedTargetInOut.X ) ) + rangeStart * MAbs( clampedTargetInOut.X );

		if ( clampedTargetInOut.Y < loc )
		{
			clampedTargetInOut.Y = loc;
			limit = true;
		}

		clampedTargetInOut /= 100.f;

		return limit;
	}
};
