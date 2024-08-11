/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorIkSolverLookAt.h"

IMPLEMENT_RTTI_ENUM( ELookAtSolverType );

//////////////////////////////////////////////////////////////////////////

Bool IQuatLookAtSolver::SetupSolver( IQuatLookAtSolver::SolverData& data ) const
{
	// Get axis vectors
	Vector forwardAxis = BehaviorUtils::VectorFromAxis( data.m_forward );
	Vector upAxis = BehaviorUtils::VectorFromAxis( data.m_up );

#ifdef USE_HAVOK_ANIMATION
	// Transform in MS
	const hkQsTransform& boneMS = data.m_inOutBoneMS;
	const hkQsTransform& parentBoneMS = data.m_boneParentMS;

	// Limit
	hkVector4 limit; limit.setZero4();
	COPY_VECTOR_TO_HK_VECTOR( forwardAxis, limit );
	data.m_setup.m_limitAxisMS.setRotatedDir( parentBoneMS.getRotation(), limit );
	data.m_setup.m_limitAxisMS.normalize3();

	// Forward
	hkVector4 hkForwardLS;
	COPY_VECTOR_TO_HK_VECTOR( forwardAxis, hkForwardLS );
	data.m_setup.m_fwdLS = hkForwardLS;

	hkVector4 fwdMS;
	fwdMS.setRotatedDir( parentBoneMS.getRotation(), data.m_setup.m_fwdLS );
	fwdMS.normalize3();

	// Local offset
	if ( data.m_localOffset )
	{
		hkVector4 locOffset; locOffset.setZero4();
		Vector lOff = *data.m_localOffset;
		COPY_VECTOR_TO_HK_VECTOR( lOff, locOffset );
		data.m_setup.m_eyePositionLS = locOffset;
	}
	else
	{
		data.m_setup.m_eyePositionLS.setZero4();
	}

		// Cone limit angle
	Float coneAngle = Max ( data.m_horizontalLimit, Max( data.m_upLimit, data.m_downLimit ) );

	data.m_setup.m_limitAngle = DEG2RAD( coneAngle );

	// Range limits
	hkaLookAtIkSolver::RangeLimits& rangeLimits = data.m_limits;
	rangeLimits.m_limitAngleDown =	-DEG2RAD( data.m_downLimit );
	rangeLimits.m_limitAngleUp =	 DEG2RAD( data.m_upLimit );
	rangeLimits.m_limitAngleLeft =	 DEG2RAD( data.m_horizontalLimit );
	rangeLimits.m_limitAngleRight =	-DEG2RAD( data.m_horizontalLimit );

	// Calc up axis in model space
	hkVector4 hkUpVector; hkUpVector.setZero4();
	COPY_VECTOR_TO_HK_VECTOR( upAxis, hkUpVector );
	rangeLimits.m_upAxisMS.setRotatedDir( parentBoneMS.getRotation(), hkUpVector );
	rangeLimits.m_upAxisMS.normalize3();

	// Check solver setup data
	hkVector4 temp;
	temp.setCross( rangeLimits.m_upAxisMS, data.m_setup.m_limitAxisMS );
	if ( !temp.isNormalized3() )
	{
		// Range Up Axis is not perpendicular to Limit Axis
		return false;
	}

	return true;
#else
	HALT( "Needs to be implmented" );

	// Transform in MS
//	const RedQsTransform& boneMS = data.m_inOutBoneMS;
	const RedQsTransform& parentBoneMS = data.m_boneParentMS;

	// Limit
	RedVector4 limit = reinterpret_cast< const RedVector4& >( forwardAxis );

	//data.m_setup.m_limitAxisMS.setRotatedDir( parentBoneMS.getRotation(), limit );
	//data.m_setup.m_limitAxisMS.normalize3();

	// Forward
	RedVector4 RedForwardLS = reinterpret_cast< const RedVector4& >( forwardAxis );
	
	//data.m_setup.m_fwdLS = hkForwardLS;

	RedVector4 fwdMS;
	fwdMS.RotateDirection( parentBoneMS.GetRotation(), RedForwardLS /*data.m_setup.m_fwdLS*/ );
	fwdMS.Normalize3();

	// Local offset
	if ( data.m_localOffset )
	{
		RedVector4 locOffset = reinterpret_cast< const RedVector4& >( *data.m_localOffset ); 
		//data.m_setup.m_eyePositionLS = locOffset;
	}
	else
	{
		//data.m_setup.m_eyePositionLS.setZero4();
	}

		// Cone limit angle
	//Float coneAngle = Max ( data.m_horizontalLimit, Max( data.m_upLimit, data.m_downLimit ) );
	//data.m_setup.m_limitAngle = DEG2RAD( coneAngle );

	// Range limits
	//hkaLookAtIkSolver::RangeLimits& rangeLimits = data.m_limits;
	//rangeLimits.m_limitAngleDown =	-DEG2RAD( data.m_downLimit );
	//rangeLimits.m_limitAngleUp =	 DEG2RAD( data.m_upLimit );
	//rangeLimits.m_limitAngleLeft =	 DEG2RAD( data.m_horizontalLimit );
	//rangeLimits.m_limitAngleRight =	-DEG2RAD( data.m_horizontalLimit );

	// Calc up axis in model space
	RedVector4 RedUpVector = reinterpret_cast< const RedVector4& >( upAxis );

	//rangeLimits.m_upAxisMS.setRotatedDir( parentBoneMS.getRotation(), hkUpVector );
	//rangeLimits.m_upAxisMS.normalize3();

	// Check solver setup data
	//RedVector4 temp;
	//temp.setCross( rangeLimits.m_upAxisMS, data.m_setup.m_limitAxisMS );
	//if ( !temp.isNormalized3() )
	//{
		// Range Up Axis is not perpendicular to Limit Axis
	//	return false;
	//}

	return true;
#endif

}

Bool IQuatLookAtSolver::Solve( SolverData& data, AnimQsTransform& boneOutLS ) const
{
	if ( !SetupSolver( data ) )
	{
		return false;
	}

#ifdef USE_HAVOK_ANIMATION
	Bool ret = !hkaLookAtIkSolver::solve( data.m_setup, data.m_targetMS, data.m_weight, data.m_inOutBoneMS, &data.m_limits );

	hkQsTransform parentInv;
	parentInv.setInverse( data.m_boneParentMS );

	boneOutLS.setMul( parentInv, data.m_inOutBoneMS );

	return ret;
#else
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool IQuatLookAtChainSolver::SetupInternalData( ChainSolverData& data, Uint32 num,
											    const AnimQsTransform& boneMS, const AnimQsTransform& parentBoneMS,
												const Vector* localOffset ) const
{
	ASSERT( num < 3 );

	data.m_boneData[ num ].m_forward = A_Y;
	data.m_boneData[ num ].m_up = A_X;

	data.m_boneData[ num ].m_inOutBoneMS = boneMS;
	data.m_boneData[ num ].m_boneParentMS = parentBoneMS;

	data.m_boneData[ num ].m_localOffset = localOffset;

	data.m_boneData[ num ].m_targetMS = data.m_targetMS[ num ];
	data.m_boneData[ num ].m_weight = data.m_weightsScale.A[ num ];

	data.m_boneData[ num ].m_downLimit = 20.f;
	data.m_boneData[ num ].m_upLimit = 50.f;
	data.m_boneData[ num ].m_horizontalLimit = 40.f;

	return IQuatLookAtSolver::SetupSolver( data.m_boneData[ num ] );
}

Bool IQuatLookAtChainSolver::Solve( IQuatLookAtChainSolver::ChainSolverData& data, AnimQsTransform bonesOut[ 3 ] ) const
{
#ifdef USE_HAVOK_ANIMATION
	// First
	AnimQsTransform firstBoneMS;
	firstBoneMS.setMul( data.m_firstBoneParentMS, data.m_firstBoneLS );

	if ( SetupInternalData( data, 0, firstBoneMS, data.m_firstBoneParentMS, NULL ) == false )
	{
		return false;
	}
	//solve( data.m_boneData[ 0 ].m_setup, data.m_boneData[ 0 ].m_targetMS, data.m_boneData[ 0 ].m_weight, data.m_boneData[ 0 ].m_inOutBoneMS, &data.m_boneData[ 0 ].m_limits );

	SolveRoll( data.m_weightsScale.A[ 0 ], data.m_boneData[ 0 ].m_targetMS, data.m_boneData[ 0 ].m_inOutBoneMS );

	AnimQsTransform firstBoneOut;
	firstBoneOut.setMulInverseMul( data.m_boneData[ 0 ].m_boneParentMS, data.m_boneData[ 0 ].m_inOutBoneMS );


	// Second

	AnimQsTransform secondBoneMS;
	secondBoneMS.setMul( data.m_boneData[ 0 ].m_inOutBoneMS, data.m_secondBoneLS );

	if ( SetupInternalData( data, 1, secondBoneMS, data.m_boneData[ 0 ].m_inOutBoneMS, NULL ) == false )
	{
		return false;
	}
	//solve( data.m_boneData[ 1 ].m_setup, data.m_boneData[ 1 ].m_targetMS, data.m_boneData[ 1 ].m_weight, data.m_boneData[ 1 ].m_inOutBoneMS, &data.m_boneData[ 1 ].m_limits );

	SolveRoll( data.m_weightsScale.A[ 1 ], data.m_boneData[ 1 ].m_targetMS, data.m_boneData[ 1 ].m_inOutBoneMS );

	AnimQsTransform secondBoneOut;
	secondBoneOut.setMulInverseMul( data.m_boneData[ 1 ].m_boneParentMS, data.m_boneData[ 1 ].m_inOutBoneMS );


	// Third

	AnimQsTransform thirdBoneMS;
	thirdBoneMS.setMul( data.m_boneData[ 1 ].m_inOutBoneMS, data.m_thirdBoneLS );

	if ( SetupInternalData( data, 2, thirdBoneMS, data.m_boneData[ 1 ].m_inOutBoneMS, data.m_localOffsetForLastBone ) == false )
	{
		return false;
	}

	hkReal limitEps = 0.1f;

	Bool limit = !solve( data.m_boneData[ 2 ].m_setup, data.m_boneData[ 2 ].m_targetMS, data.m_boneData[ 2 ].m_weight, data.m_boneData[ 2 ].m_inOutBoneMS, &data.m_boneData[ 2 ].m_limits, limitEps );

	// Output
	bonesOut[ 0 ] = firstBoneOut;
	bonesOut[ 1 ] = secondBoneOut;
	bonesOut[ 2 ].setMulInverseMul( data.m_boneData[ 2 ].m_boneParentMS, data.m_boneData[ 2 ].m_inOutBoneMS );

	return limit;
#else
	return false;
#endif
}

Bool IQuatLookAtChainSolver::SolveRoll( Float gain, const AnimVector4& targetMS, AnimQsTransform& boneModelSpaceInOut )
{
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform animTrans = boneModelSpaceInOut;
	animTrans.m_rotation.normalize();

	hkVector4 tarToDist;
	tarToDist.setSub4( targetMS, boneModelSpaceInOut.getTranslation() );
	tarToDist.normalize3();

	hkTransform tempTrans( animTrans.m_rotation, animTrans.m_translation );
	ASSERT( tempTrans.isOk() );

	hkVector4 r1 = tempTrans.getColumn( 0 ); r1.normalize3();
	hkVector4 r3; r3.setCross( r1, tarToDist ); r3.normalize3();
	hkVector4 r2; r2.setCross( r3, r1 ); r2.normalize3();

	hkTransform tempTrans2;
	tempTrans2.getColumn( 0 ) = r1;
	tempTrans2.getColumn( 1 ) = r2;
	tempTrans2.getColumn( 2 ) = r3;
	tempTrans2.getColumn( 3 ) = tempTrans.getColumn( 3 );

	const hkQuaternion solvedQuat( tempTrans2.getRotation() );
	const hkQuaternion& animQuat = animTrans.m_rotation;

	// Final
	boneModelSpaceInOut.m_rotation.setSlerp( animQuat, solvedQuat, gain );	
#else
	RedQsTransform animTrans = boneModelSpaceInOut;
	animTrans.Rotation.Normalize();

	RedVector4 tarToDist;
	tarToDist = Sub( targetMS, boneModelSpaceInOut.GetTranslation() );
	tarToDist.Normalize3();

	RedTransform tempTrans( animTrans.Rotation, animTrans.Translation );
	ASSERT( tempTrans.IsOk() );

	RedVector4 r1 = tempTrans.GetColumn( 0 );
	r1.Normalize3();

	RedVector4 r3 = Cross( r1, tarToDist );
	r3.Normalize3();

	RedVector4 r2 = Cross( r3, r1 ); 
	r2.Normalize3();

	RedTransform tempTrans2;
	tempTrans2.GetColumn( 0 ) = r1;
	tempTrans2.GetColumn( 1 ) = r2;
	tempTrans2.GetColumn( 2 ) = r3;
	tempTrans2.GetColumn( 3 ) = tempTrans.GetColumn( 3 );

	RedQuaternion solvedQuat;
	solvedQuat.Set( tempTrans2.GetRotation() );
	const RedQuaternion& animQuat = animTrans.Rotation;

	// Final
	boneModelSpaceInOut.Rotation.SetSlerp( animQuat, solvedQuat, gain );
#endif

	return false;
}

//////////////////////////////////////////////////////////////////////////
#ifdef USE_HAVOK_ANIMATION
hkBool IQuatLookAtChainSolver::solve ( const hkaLookAtIkSolver::Setup& setup, const hkVector4& targetMS, hkReal gain, hkQsTransform& boneModelSpaceInOut, const hkaLookAtIkSolver::RangeLimits* range, hkReal limitEps )
{
	hkReal limitAngle = setup.m_limitAngle;

	// Assert that the input is appropriate
	HK_ASSERT2( 0x6cee5e7e, setup.m_fwdLS.isNormalized3(), "setup.m_fwdLS (forward direction vector) should be normalized" );
	HK_ASSERT2( 0x6cee5e7e, setup.m_limitAxisMS.isNormalized3(), "setup.m_limitAxisMS (limiting cone axis) should be normalized" );
	HK_ASSERT2( 0x6cee5e7e, ( limitAngle >= 0.0f ) && ( limitAngle <= HK_REAL_PI ), "limitAngle should be in range [ 0, pi ]" );

	// Check if a range was specified
	if ( range != HK_NULL )
	{
		HK_ASSERT2( 0x6cee5e7e, range->m_upAxisMS.isNormalized3(), "range->m_upAxisMS (range up axis) should be normalized" );

		HK_ASSERT2( 0x6cee5e7e, ( range->m_limitAngleUp >= -HK_REAL_PI/2.0f ) && ( range->m_limitAngleUp <= HK_REAL_PI/2.0f ), "range->m_limitUp should be in range [ -pi/2, pi/2 ]" );
		HK_ASSERT2( 0x6cee5e7e, ( range->m_limitAngleDown >= -HK_REAL_PI/2.0f ) && ( range->m_limitAngleDown <= HK_REAL_PI/2.0f ), "range->m_limitDown should be in range [ -pi/2, pi/2 ]" );
		HK_ASSERT2( 0x6cee5e7e, ( range->m_limitAngleUp >= range->m_limitAngleDown ), "range->m_limitUp should be >= range->m_limitAngleDown" );

		HK_ASSERT2( 0x6cee5e7e, ( range->m_limitAngleLeft >= -HK_REAL_PI ) && ( range->m_limitAngleLeft <= HK_REAL_PI ), "range->m_limitLeft should be in range [ -pi, pi ]" );
		HK_ASSERT2( 0x6cee5e7e, ( range->m_limitAngleRight >= -HK_REAL_PI ) && ( range->m_limitAngleRight <= HK_REAL_PI ), "range->m_limitRight should be in range [ -pi, pi ]" );
		HK_ASSERT2( 0x6cee5e7e, ( range->m_limitAngleLeft >= range->m_limitAngleRight ), "range->m_limitAngleLeft should be >= range->m_limitAngleRight" );

		// Set the limit angle to the maximum range
		limitAngle = hkMath::fabs( range->m_limitAngleLeft );
		limitAngle = hkMath::max2( limitAngle, hkMath::fabs( range->m_limitAngleRight ) );
		limitAngle = hkMath::max2( limitAngle, hkMath::fabs( range->m_limitAngleUp ) );
		limitAngle = hkMath::max2( limitAngle, hkMath::fabs( range->m_limitAngleDown ) );
	}

	const hkVector4& bonePosMS = boneModelSpaceInOut.getTranslation();

	// Calc vector to target in model space
	hkVector4 vecToTargetMS;
	vecToTargetMS.setSub4( targetMS, bonePosMS );
	hkReal targetDist; targetDist = vecToTargetMS.length3();
	vecToTargetMS.normalize3();

	hkBool insideLimits = true;

	// Limit the vecToTargetMS
	hkReal cosLimitAngle = hkMath::cos( limitAngle );
	hkReal cosTargetAngle = vecToTargetMS.dot3( setup.m_limitAxisMS );
	if ( cosTargetAngle < cosLimitAngle )
	{
		// The target is outside of the limit cone

		// Find the vector perpendicular to both the limit axis and the vector to target
		hkVector4 cross;
		cross.setCross(  setup.m_limitAxisMS, vecToTargetMS );
		cross.normalize3();

		// Create a rotation of limit degrees about the found axis
		hkQuaternion q;
		q.setAxisAngle( cross, limitAngle );

		// Rotate the limit axis limit degrees about the axis to form the new target
		vecToTargetMS.setRotatedDir( q,  setup.m_limitAxisMS );

		insideLimits = false;
	}

	// Apply range correction
	if ( range != HK_NULL )
	{
		// Store the old vecToTargetMS for later comparison
		hkVector4 vecToTargetMSOld = vecToTargetMS;

		// Compute the side vector
		hkVector4 sideAxisMS;
		sideAxisMS.setCross( range->m_upAxisMS, setup.m_limitAxisMS );
		HK_ASSERT2( 0x6cee5e7e, sideAxisMS.isNormalized3(), "range->m_upAxisMS is not perpendicular to setup.m_limitAxisMS" );

		// Define a front, side, up coordinate system (known to be orthonormal)
		hkMatrix3 worldFromFSU;
		hkMatrix3 fsuFromWorld;
		worldFromFSU.setCols( setup.m_limitAxisMS, sideAxisMS, range->m_upAxisMS );
		fsuFromWorld.setTranspose( worldFromFSU );

		// Find the vecToTarget in the fsu coordinate system
		hkVector4 vec;
		vec.setMul3( fsuFromWorld, vecToTargetMS );

		// Convert to spherical coordinates
		sphericalFromCartesian( vec );

		// Clamp
		hkVector4 maxVec;
		hkVector4 minVec;
		maxVec.set(  HK_REAL_MAX, range->m_limitAngleLeft, range->m_limitAngleUp, 0.0f );
		minVec.set( -HK_REAL_MAX, range->m_limitAngleRight,  range->m_limitAngleDown, 0.0f );

		hkVector4 limitedVec = vec;
		limitedVec.setMin4( limitedVec, maxVec );
		limitedVec.setMax4( limitedVec, minVec );

		// Convert back to front, side, up
		cartesianFromSpherical( limitedVec );

		// Convert back to modelspace
		vecToTargetMS.setMul3( worldFromFSU, limitedVec );

		// Test if the target was affected by limiting
		if ( !vecToTargetMS.equals3( vecToTargetMSOld, limitEps ) )
		{
			insideLimits = false;
		}
	}


	// Transform the model space of the bone
	{
		// Find the local space forward axis in model space
		hkVector4 fwdMS;
		fwdMS.setRotatedDir( boneModelSpaceInOut.getRotation(), setup.m_fwdLS);

		// Find the vector perpendicular to both the forward and target directions
		hkVector4 cross;
		cross.setCross( fwdMS, vecToTargetMS );
		cross.normalize3();

		// Find an angle proportional to the current error
		hkReal cosAngle = vecToTargetMS.dot3( fwdMS );
		hkReal angle = hkMath::acos(cosAngle) * gain;

		// Rotate around the perpendicular by this amount
		hkQuaternion q;
		q.setAxisAngle( cross, angle );

		// If the eye is not located at the joint, calculate correction angle
		const hkReal eyeDistLS = setup.m_eyePositionLS.length3();

		if ( eyeDistLS > 0.0f )
		{
			// calculate projection axis
			hkVector4 tmpAxis;
			tmpAxis.setCross( setup.m_fwdLS, setup.m_eyePositionLS );
			tmpAxis.normalize3();

			hkVector4 projAxis;
			projAxis.setCross( tmpAxis, setup.m_fwdLS );
			projAxis.normalize3();

			// calculate projection
			const hkReal  eyeHeighLS = projAxis.dot3( setup.m_eyePositionLS )/projAxis.length3();

			// analytically derived solution for correction angle
			//a2 =  1/2*(-2*c1^2*b1^2+2*b1^2*a1^2+c1^4+2*c1^2*a1^2+b1^4+a1^4)^(1/2)/b1
			//a2 =  -1/2*(-2*c1^2*b1^2+2*b1^2*a1^2+c1^4+2*c1^2*a1^2+b1^4+a1^4)^(1/2)/b1

			const hkReal a1 = eyeHeighLS;
			const hkReal c1 = targetDist;
			const hkReal c12 = c1*c1;
			const hkReal a12 = a1*a1;
			const hkReal b1 = hkMath::sqrt( hkMath::fabs( c12 - a12 ) );
			const hkReal b12 = b1*b1;

			hkReal a2 =  0.5f * hkMath::sqrt(hkMath::fabs(-2.0f*c12*b12+2.0f*b12*a12+c12*c12+2*c12*a12+b12*b12+a12*a12))/b1;

			hkReal correctionAngle; correctionAngle = 0.0f;

			if (a2 != 0.0f)
			{
				correctionAngle = hkMath::acos(a1/a2);
			}

			hkVector4 eyePositionMS;
			eyePositionMS.setRotatedDir( boneModelSpaceInOut.getRotation(), setup.m_eyePositionLS);

			hkVector4 crossCorrection;
			crossCorrection.setCross( fwdMS, eyePositionMS );
			crossCorrection.normalize3();

			hkQuaternion qCorrection;
			qCorrection.setAxisAngle( crossCorrection, -correctionAngle );

			q.mul(qCorrection);


		}

		hkQuaternion newRotation; newRotation. setMul( q, boneModelSpaceInOut.getRotation() );
		boneModelSpaceInOut.m_rotation = newRotation;

	}

	return insideLimits;
}
#endif

void IQuatLookAtChainSolver::cartesianFromSpherical( AnimVector4& sphericalInCartesianOut )
{
#ifdef USE_HAVOK_ANIMATION
	const AnimFloat rad = sphericalInCartesianOut( 0 );
	const AnimFloat lon = sphericalInCartesianOut( 1 );
	const AnimFloat lat = sphericalInCartesianOut( 2 );
	const AnimFloat rct = rad * Red::Math::MCos( lat );

	sphericalInCartesianOut.set(
		rct * Red::Math::MCos( lon ),
		rct * Red::Math::MSin( lon ),
		rad * Red::Math::MSin( lat ) );

#else
	const AnimFloat rad = sphericalInCartesianOut.X;
	const AnimFloat lon = sphericalInCartesianOut.Y;
	const AnimFloat lat = sphericalInCartesianOut.Z;
	const AnimFloat rct = rad * Red::Math::MCos( lat );

	sphericalInCartesianOut.Set(
		rct * Red::Math::MCos( lon ),
		rct * Red::Math::MSin( lon ),
		rad * Red::Math::MSin( lat ), 0.0f );
#endif
}

void IQuatLookAtChainSolver::sphericalFromCartesian( AnimVector4& cartesianInSphericalOut )
{
#ifdef USE_HAVOK_ANIMATION
	// A small value to avoid singularities
	const hkReal eps = 1.0e-4f;

	const hkReal rad = cartesianInSphericalOut.length3();

	if ( rad < eps )
	{
		cartesianInSphericalOut.setZero4();
		return;
	}

	const hkReal x = cartesianInSphericalOut( 0 );
	const hkReal y = cartesianInSphericalOut( 1 );
	const hkReal z = cartesianInSphericalOut( 2 );

	// Convert to spherical coordinates
	const hkReal lat = hkMath::asin( z / rad );
	const hkReal lon = lookAtAtan2( y, x );

	cartesianInSphericalOut.set( rad, lon, lat );
#else
	// A small value to avoid singularities
	const Float eps = 1.0e-4f;

	const Float rad = cartesianInSphericalOut.Length3();

	if ( rad < eps )
	{
		cartesianInSphericalOut.SetZeros();
		return;
	}

	const Float x = cartesianInSphericalOut.X;
	const Float y = cartesianInSphericalOut.Y;
	const Float z = cartesianInSphericalOut.Z;

	// Convert to spherical coordinates
	const Float lat = Red::Math::MAsin( z / rad );
	const Float lon = lookAtAtan2( y, x );

	cartesianInSphericalOut.Set( rad, lon, lat, 0.0f );
#endif
}

Float IQuatLookAtChainSolver::lookAtAtan2( Float y, Float x )
{
#ifdef USE_HAVOK_ANIMATION
	return hkMath_atan2fApproximation( y, x );
#else
	return Red::Math::MATan2( y, x );
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool IEulerLookAtSolver::Solve( SolverData& data, AnimQsTransform& boneOutLS ) const
{
	static const Float eps = 0.001f;

	const AnimQsTransform& boneLS = data.m_boneInLS;
	const AnimQsTransform& parentBoneMS = data.m_boneParentMS;

#ifdef USE_HAVOK_ANIMATION
	AnimQsTransform boneMS;
	boneMS.setMul( parentBoneMS, boneLS );

	const AnimVector4& bonePosMS = boneMS.getTranslation();
	const AnimVector4& targetMS = data.m_targetMS;

	AnimQsTransform temp = parentBoneMS;
	temp.m_translation = boneMS.m_translation;

	AnimQsTransform parnetBoneInvMS;
	parnetBoneInvMS.setInverse( temp );
	AnimQsTransform target( AnimQsTransform::IDENTITY );
	target.m_translation = targetMS;
	AnimQsTransform targetPS;
	targetPS.setMul( parnetBoneInvMS, target );

	AnimVector4 vecToTargetMS = targetPS.getTranslation();
	Float targetDist = vecToTargetMS.length3();
	vecToTargetMS.normalize3();

	Bool insideLimits = false;

	// Angle vertical
	Float cosVer = vecToTargetMS.dot3( hkVector4( 1.f, 0.f, 0.f ) );
	Float angleVer = ( M_PI / 2.f - MAcos_safe( cosVer ) ); 
	
	// Angle horizontal
	AnimVector4 vecToTargetHorMS = vecToTargetMS; vecToTargetHorMS.zeroElement( 0 ); vecToTargetHorMS.normalize3();
	Float cosHor = vecToTargetHorMS.dot3( hkVector4( 0.f, 0.f, 1.f ) );
	Float angleHor = - ( M_PI / 2.f - MAcos_safe( cosHor ) );

	// Limits
	if ( vecToTargetMS.dot3( hkVector4( 0.f, 1.f, 0.f ) ) < 0.f )
	{
		insideLimits = true;
	}

	if ( angleVer > data.m_limits.m_limitAngleUp )
	{
		angleVer = data.m_limits.m_limitAngleUp;
		insideLimits = true;
	}
	else if ( angleVer < data.m_limits.m_limitAngleDown )
	{
		angleVer = data.m_limits.m_limitAngleDown;
		insideLimits = true;
	}

	if ( angleHor > data.m_limits.m_limitAngleLeft )
	{
		angleHor = data.m_limits.m_limitAngleLeft;
		insideLimits = true;
	}
	else if ( angleHor < data.m_limits.m_limitAngleRight )
	{
		angleHor = data.m_limits.m_limitAngleRight;
		insideLimits = true;
	}

	Matrix matVer; matVer.SetIdentity(); matVer.SetRotZ33( -angleVer );
	Matrix matHor; matHor.SetIdentity(); matHor.SetRotX33( -angleHor );

	//BEH_LOG( TXT("ver %.2f"), RAD2DEG( angleVer ) );
	//BEH_LOG( TXT("hor %.2f"), RAD2DEG( angleHor ) );

	// Direction correction
	Matrix matDirection = Matrix::IDENTITY;

	if ( data.m_boneDirLS )
	{
		const Vector dirLS = *data.m_boneDirLS;
		const hkVector4 dirVec = TO_CONST_HK_VECTOR_REF( dirLS );

		// TODO:
		// TPose front is -Y :( !!!, animation front is Y
		Float tPoseHack = -1.f;

		Float dirAngle = MAcos_safe( tPoseHack * dirLS.Y );

		hkVector4 dirX; dirX.setCross( hkVector4( 0.f, 1.f, 0.f ), dirVec );
		dirX.normalize3();

		hkQsTransform corrTransform( hkQsTransform::IDENTITY );
		corrTransform.m_rotation.setAxisAngle( dirX, dirAngle );
		
		HavokTransformToMatrix_Renormalize( corrTransform, &matDirection );
	}

	// Final rotations
	Matrix matFinal = matDirection * matVer * matHor;

	hkQsTransform transformLS;
	MatrixToHavokQsTransform( matFinal, transformLS );
	transformLS.m_translation = boneLS.m_translation;

	// Set new rotation in LS

	if ( data.m_weight < 1.f )
	{
		boneOutLS.setInterpolate4( boneLS, transformLS, data.m_weight );
	}
	else
	{
		boneOutLS = transformLS;
	}
	
	return insideLimits;
#else
	RedQsTransform boneMS;
	boneMS.SetMul( parentBoneMS, boneLS );

//	const RedVector4& bonePosMS = boneMS.GetTranslation();
	const RedVector4& targetMS = data.m_targetMS;

	RedQsTransform temp = parentBoneMS;
	temp.Translation = boneMS.Translation;

	RedQsTransform parnetBoneInvMS;
	parnetBoneInvMS.SetInverse( temp );
	RedQsTransform target( RedQsTransform::IDENTITY );
	target.Translation = targetMS;
	RedQsTransform targetPS; 
	targetPS.SetMul( parnetBoneInvMS, target );

	RedVector4 vecToTargetMS = targetPS.GetTranslation();
	Float targetDist;
	targetDist = vecToTargetMS.Length3();
	vecToTargetMS.Normalize3();

	Bool insideLimits = false;

	// Angle vertical
	Float cosVer = Dot3(vecToTargetMS, RedVector4( 1.0f, 0.0f, 0.0f, 0.0f ) );
	Float angleVer = ( M_PI / 2.f - Red::Math::MAcos_safe( cosVer ) ); 
	
	// Angle horizontal
	RedVector4 vecToTargetHorMS = vecToTargetMS; 
	vecToTargetHorMS.X = 0.0f;
	vecToTargetHorMS.Normalize3();

	Float cosHor = Dot3( vecToTargetHorMS, RedVector4( 0.0f, 0.0f, 1.0f, 0.0f) );
	Float angleHor = -( M_PI / 2.f - Red::Math::MAcos_safe( cosHor ) );

	// Limits
	if ( Dot3( vecToTargetMS, RedVector4( 0.0f, 1.0f, 0.0f, 0.0f ) ) < 0.0f )
	{
		insideLimits = true;
	}
	HALT( "Needs to be implemented" );
	//if ( angleVer > data.m_limits.m_limitAngleUp )
	//{
	//	angleVer = data.m_limits.m_limitAngleUp;
	//	insideLimits = true;
	//}
	//else if ( angleVer < data.m_limits.m_limitAngleDown )
	//{
	//	angleVer = data.m_limits.m_limitAngleDown;
	//	insideLimits = true;
	//}

	//if ( angleHor > data.m_limits.m_limitAngleLeft )
	//{
	//	angleHor = data.m_limits.m_limitAngleLeft;
	//	insideLimits = true;
	//}
	//else if ( angleHor < data.m_limits.m_limitAngleRight )
	//{
	//	angleHor = data.m_limits.m_limitAngleRight;
	//	insideLimits = true;
	//}
	
	RedMatrix4x4 matVer; 
	matVer.SetIdentity(); 
	matVer.SetRotZ( -angleVer );

	RedMatrix4x4 matHor; 
	matHor.SetIdentity(); 
	matHor.SetRotX( -angleHor );

	//BEH_LOG( TXT("ver %.2f"), RAD2DEG( angleVer ) );
	//BEH_LOG( TXT("hor %.2f"), RAD2DEG( angleHor ) );

	// Direction correction
	RedMatrix4x4 matDirection = RedMatrix4x4::IDENTITY;

	if ( data.m_boneDirLS )
	{
		const RedVector4 dirVec = reinterpret_cast< const RedVector4& >( *data.m_boneDirLS );

		// TODO:
		// TPose front is -Y :( !!!, animation front is Y
		Float tPoseHack = -1.f;

		Float dirAngle = MAcos_safe( tPoseHack * dirVec.Y );

		RedVector4 dirX( Cross( RedVector4( 0.0f, 1.0f, 0.0f, 0.0f ), dirVec ) ); 
		dirX.Normalize3();

		RedQsTransform corrTransform( RedQsTransform::IDENTITY );
		corrTransform.Rotation.SetAxisAngle( dirX, dirAngle );
		matDirection = corrTransform.ConvertToMatrixNormalized();
	}

	// Final rotations
	RedMatrix4x4 matFinal = Mul( Mul( matHor, matVer ), matDirection );

	RedQsTransform transformLS;
	transformLS.Set( matFinal );
	transformLS.Translation = boneLS.Translation;

	// Set new rotation in LS

	if ( data.m_weight < 1.0f )
	{
		boneOutLS.Lerp( boneLS, transformLS, data.m_weight );
	}
	else
	{
		boneOutLS = transformLS;
	}
	
	return insideLimits;
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool IEulerLookAtChainSolver::Solve( const IEulerLookAtChainSolver::SolverData& data, AnimQsTransform* bonesOut ) const
{
#ifdef USE_HAVOK_ANIMATION
	Bool insideLimits = false;

	Float weightSum = data.m_weightsScale.Sum4();
	if ( weightSum <= 0.f )
	{
		return false;
	}

	// Weights scale
	Vector weightScale = data.m_weightsScale / weightSum;

	// Third bone MS
	hkQsTransform fourthBoneMS;
	fourthBoneMS.setMul( data.m_firstBoneParentMS, data.m_firstBoneLS );
	fourthBoneMS.setMul( fourthBoneMS, data.m_secondBoneLS );
	fourthBoneMS.setMul( fourthBoneMS, data.m_thirdBoneLS );
	fourthBoneMS.setMul( fourthBoneMS, data.m_fourthBoneLS );

	fourthBoneMS = data.m_fourthBoneMS;

	// Temp transform for build new space
	hkQsTransform temp2 = data.m_firstBoneParentMS;
	temp2.m_translation.setTransformedPos( data.m_firstBoneParentMS, hkVector4( 0.71f, 0.f, 0.f ) );
	hkQsTransform temp;
	temp.setInverse( temp2 );

	// Target in parent first bone space
	hkQsTransform target( hkQsTransform::IDENTITY );
	target.m_translation = data.m_targetMS;
	hkQsTransform targetPS; targetPS.setMul( temp, target );

	// Horizontal
	hkVector4 horDir = targetPS.m_translation;
	horDir.zeroElement( 0 );
	horDir.normalize3();

	// Horizontal angle
	Float horAngle = MAcos_safe( horDir( 1 ) );

	// Check sign
	if ( horDir( 2 ) < 0.f )
	{
		horAngle *= -1.f;
	}

	// Horizontal limits
	Float horAngleDeg = RAD2DEG( horAngle );
	if ( horAngleDeg > 120.f )
	{
		horAngle = DEG2RAD( ( horAngleDeg - 180.f ) / ( -60.f ) * 120.f ); // -60 = 120-180
	}
	else if ( horAngleDeg < -120.f )
	{
		horAngle = DEG2RAD( ( horAngleDeg + 180.f ) / ( -60.f ) * 120.f ); // -60 = 120-180
	}


	// Vertical
	hkVector4 verDir = targetPS.m_translation;
	verDir.normalize3();

	static Float FFF = 0.8f;

	// Vertical angle
	Float dotX = verDir( 0 );
// 	if ( dotX > FFF ) dotX = FFF;
// 	else if ( dotX < -FFF ) dotX = -FFF;

	Float verAngle = MAcos_safe( dotX ) - M_PI / 2.f;
	

	// Dead zones
	//...


	// Angles
	Vector horAngles = weightScale * horAngle;
	Vector verAngles = weightScale * verAngle;

	Matrix horFirst;	horFirst.SetTranslation( Vector::ZERO_3D_POINT );	horFirst.SetRotX33(	horAngles.A[0] );
	Matrix horSec;		horSec.SetTranslation( Vector::ZERO_3D_POINT );		horSec.SetRotX33(	horAngles.A[0] + horAngles.A[1] );
	Matrix horThird;	horThird.SetTranslation( Vector::ZERO_3D_POINT );	horThird.SetRotX33(	horAngles.A[0] + horAngles.A[1] + horAngles.A[2] );
	Matrix horFourth;	horFourth.SetTranslation( Vector::ZERO_3D_POINT );	horFourth.SetRotX33(horAngles.A[0] + horAngles.A[1] + horAngles.A[2] + horAngles.A[3] );

	Matrix verFirst;	verFirst.SetTranslation( Vector::ZERO_3D_POINT );	verFirst.SetRotZ33(	verAngles.A[0] );
	Matrix verSec;		verSec.SetTranslation( Vector::ZERO_3D_POINT );		verSec.SetRotZ33(	verAngles.A[0] + verAngles.A[1] );
	Matrix verThird;	verThird.SetTranslation( Vector::ZERO_3D_POINT );	verThird.SetRotZ33(	verAngles.A[0] + verAngles.A[1] + verAngles.A[2] );
	Matrix verFourth;	verFourth.SetTranslation( Vector::ZERO_3D_POINT );	verFourth.SetRotZ33(verAngles.A[0] + verAngles.A[1] + verAngles.A[2] + verAngles.A[3] );

	// Offset
	Matrix tempOffset;	tempOffset.SetTranslation( Vector::ZERO_3D_POINT );	tempOffset.SetRotZ33( DEG2RAD( 20.f ) );

	// Transform in PS
	Matrix firstPS = verFirst * horFirst;
	Matrix secondPS = verSec * horSec;
	Matrix thirdPS = tempOffset * verThird * horThird;
	Matrix fourthPS = verFourth * horFourth;

	{
		const hkQsTransform& pelvisMS = data.m_firstBoneParentMS;
		hkQsTransform pelvisInvMS; pelvisInvMS.setInverse( pelvisMS );

		hkQsTransform torsoMS; torsoMS.setMul( pelvisMS, data.m_firstBoneLS );

		hkQsTransform torsoPS;
		hkQsTransform torso2PS;
		hkQsTransform neckPS;
		hkQsTransform headPS;

		torsoPS.setMul( pelvisInvMS, torsoMS );
		torso2PS.setMul( torsoPS, data.m_secondBoneLS );
		neckPS.setMul( torso2PS, data.m_thirdBoneLS );
		headPS.setMul( neckPS, data.m_fourthBoneLS );

		hkQsTransform torsoLL;	MatrixToHavokQsTransform( firstPS, torsoLL );		torsoLL.m_translation = torsoPS.m_translation;
		hkQsTransform torso2LL;	MatrixToHavokQsTransform( secondPS, torso2LL );		torso2LL.m_translation = torso2PS.m_translation;
		hkQsTransform neckLL;	MatrixToHavokQsTransform( thirdPS, neckLL );		neckLL.m_translation = neckPS.m_translation;
		hkQsTransform headLL;	MatrixToHavokQsTransform( fourthPS, headLL );		headLL.m_translation = headPS.m_translation;

		hkQsTransform torsoF;	torsoF.setInterpolate4( torsoPS, torsoLL, data.m_weightsBlend.A[0] );
		hkQsTransform torso2F;	torso2F.setInterpolate4( torso2PS, torso2LL, data.m_weightsBlend.A[1] );
		hkQsTransform neckF;	neckF.setInterpolate4( neckPS, neckLL, data.m_weightsBlend.A[2] );
		hkQsTransform headF;	headF.setInterpolate4( headPS, headLL, data.m_weightsBlend.A[3] );

		headF.setMulInverseMul( neckF, headF );
		neckF.setMulInverseMul( torso2F, neckF );
		torso2F.setMulInverseMul( torsoF, torso2F );

		bonesOut[ 0 ] = torsoF;
		bonesOut[ 1 ] = torso2F;
		bonesOut[ 2 ] = neckF;
		bonesOut[ 3 ] = headF;

		bonesOut[ 0 ].m_translation = data.m_firstBoneLS.m_translation;
		bonesOut[ 1 ].m_translation = data.m_secondBoneLS.m_translation;
		bonesOut[ 2 ].m_translation = data.m_thirdBoneLS.m_translation;
		bonesOut[ 3 ].m_translation = data.m_fourthBoneLS.m_translation;
	}

	/*
	// Back to LS
	Matrix firstLS = firstPS;							firstLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_firstBoneLS.getTranslation() ) );
	Matrix secondLS = secondPS * firstPS.Inverted();	secondLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_secondBoneLS.getTranslation() ) );
	Matrix thirdLS = thirdPS * secondPS.Inverted();		thirdLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_thirdBoneLS.getTranslation() ) );
	Matrix fourthLS = fourthPS * thirdPS.Inverted();	fourthLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_fourthBoneLS.getTranslation() ) );

	// Output
	MatrixToHavokQsTransform( firstLS, bonesOut[ 0 ] );
	MatrixToHavokQsTransform( secondLS, bonesOut[ 1 ] );
	MatrixToHavokQsTransform( thirdLS, bonesOut[ 2 ] );
	MatrixToHavokQsTransform( fourthLS, bonesOut[ 3 ] );*/

	return insideLimits;
#else
	Bool insideLimits = false;

	Float weightSum = data.m_weightsScale.Sum4();
	if ( weightSum <= 0.f )
	{
		return false;
	}

	// Weights scale
	Vector weightScale = data.m_weightsScale / weightSum;

	// Third bone MS
	RedQsTransform fourthBoneMS;
	fourthBoneMS.SetMul( data.m_firstBoneParentMS, data.m_firstBoneLS );
	fourthBoneMS.SetMul( fourthBoneMS, data.m_secondBoneLS );
	fourthBoneMS.SetMul( fourthBoneMS, data.m_thirdBoneLS );
	fourthBoneMS.SetMul( fourthBoneMS, data.m_fourthBoneLS );

	fourthBoneMS = data.m_fourthBoneMS;

	// Temp transform for build new space
	RedQsTransform temp2 = data.m_firstBoneParentMS;
	temp2.Translation.SetTransformedPos( data.m_firstBoneParentMS, RedVector4( 0.71f, 0.0f, 0.0f ) );
	RedQsTransform temp;
	temp.SetInverse( temp2 );

	// Target in parent first bone space
	RedQsTransform target( RedQsTransform::IDENTITY );
	target.Translation = data.m_targetMS;
	RedQsTransform targetPS; 
	targetPS.SetMul( temp, target );

	// Horizontal
	RedVector4 horDir = targetPS.Translation;
	horDir.X = 0.0f;
	horDir.Normalize3();

	// Horizontal angle
	Float horAngle = Red::Math::MAcos_safe( horDir.Y );

	// Check sign
	if ( horDir.Z < 0.f )
	{
		horAngle *= -1.0f;
	}

	// Horizontal limits
	Float horAngleDeg = RAD2DEG( horAngle );
	if ( horAngleDeg > 120.f )
	{
		horAngle = DEG2RAD( ( horAngleDeg - 180.f ) / ( -60.f ) * 120.f ); // -60 = 120-180
	}
	else if ( horAngleDeg < -120.f )
	{
		horAngle = DEG2RAD( ( horAngleDeg + 180.f ) / ( -60.f ) * 120.f ); // -60 = 120-180
	}


	// Vertical
	RedVector4 verDir = targetPS.Translation;
	verDir.Normalize3();

	static Float FFF = 0.8f;

	// Vertical angle
	Float dotX = verDir.X;
// 	if ( dotX > FFF ) dotX = FFF;
// 	else if ( dotX < -FFF ) dotX = -FFF;

	Float verAngle = MAcos_safe( dotX ) - M_PI / 2.f;
	

	// Dead zones
	//...


	// Angles
	Vector horAngles = weightScale * horAngle;
	Vector verAngles = weightScale * verAngle;

	RedMatrix4x4 horFirst;	
	horFirst.SetTranslation( RedVector4::ZERO_3D_POINT );	
	horFirst.SetRotX( horAngles.A[0] );
	
	RedMatrix4x4 horSec;		
	horSec.SetTranslation( RedVector4::ZERO_3D_POINT );		
	horSec.SetRotX(	horAngles.A[0] + horAngles.A[1] );

	RedMatrix4x4 horThird;	
	horThird.SetTranslation( RedVector4::ZERO_3D_POINT );	
	horThird.SetRotX( horAngles.A[0] + horAngles.A[1] + horAngles.A[2] );
	
	RedMatrix4x4 horFourth;	
	horFourth.SetTranslation( RedVector4::ZERO_3D_POINT );	
	horFourth.SetRotX( horAngles.A[0] + horAngles.A[1] + horAngles.A[2] + horAngles.A[3] );

	RedMatrix4x4 verFirst;	
	verFirst.SetTranslation( RedVector4::ZERO_3D_POINT );	
	verFirst.SetRotZ( verAngles.A[0] );
	
	RedMatrix4x4 verSec;		
	verSec.SetTranslation( RedVector4::ZERO_3D_POINT );		
	verSec.SetRotZ(	verAngles.A[0] + verAngles.A[1] );
	
	RedMatrix4x4 verThird;	
	verThird.SetTranslation( RedVector4::ZERO_3D_POINT );
	verThird.SetRotZ( verAngles.A[0] + verAngles.A[1] + verAngles.A[2] );

	RedMatrix4x4 verFourth;	
	verFourth.SetTranslation( RedVector4::ZERO_3D_POINT );
	verFourth.SetRotZ( verAngles.A[0] + verAngles.A[1] + verAngles.A[2] + verAngles.A[3] );

	// Offset
	RedMatrix4x4 tempOffset;	
	tempOffset.SetTranslation( RedVector4::ZERO_3D_POINT );	
	tempOffset.SetRotZ( DEG2RAD( 20.f ) );

	// Transform in PS
	RedMatrix4x4 firstPS = Mul( horFirst, verFirst );
	RedMatrix4x4 secondPS = Mul( horSec, verSec );
	RedMatrix4x4 thirdPS = Mul( Mul( horThird, verThird ), tempOffset );
	RedMatrix4x4 fourthPS = Mul( horFourth, verFourth );

	{
		const RedQsTransform& pelvisMS = data.m_firstBoneParentMS;
		RedQsTransform pelvisInvMS; 
		pelvisInvMS.SetInverse( pelvisMS );

		RedQsTransform torsoMS; 
		torsoMS.SetMul( pelvisMS, data.m_firstBoneLS );

		RedQsTransform torsoPS;
		RedQsTransform torso2PS;
		RedQsTransform neckPS;
		RedQsTransform headPS;

		torsoPS.SetMul( pelvisInvMS, torsoMS );
		torso2PS.SetMul( torsoPS, data.m_secondBoneLS );
		neckPS.SetMul( torso2PS, data.m_thirdBoneLS );
		headPS.SetMul( neckPS, data.m_fourthBoneLS );

		RedQsTransform torsoLL;
		torsoLL.Set( firstPS );
		torsoLL.Translation = torsoPS.Translation;

		RedQsTransform torso2LL;
		torso2LL.Set( secondPS );
		torso2LL.Translation = torso2PS.Translation;

		RedQsTransform neckLL;
		neckLL.Set( thirdPS );		
		neckLL.Translation = neckPS.Translation;

		RedQsTransform headLL;
		headLL.Set( fourthPS );		
		headLL.Translation = headPS.Translation;

		RedQsTransform torsoF;	
		torsoF.Lerp( torsoPS, torsoLL, data.m_weightsBlend.A[0] );

		RedQsTransform torso2F;	
		torso2F.Lerp( torso2PS, torso2LL, data.m_weightsBlend.A[1] );

		RedQsTransform neckF;	
		neckF.Lerp( neckPS, neckLL, data.m_weightsBlend.A[2] );

		RedQsTransform headF;	
		headF.Lerp( headPS, headLL, data.m_weightsBlend.A[3] );

		headF.SetMulInverseMul( neckF, headF );
		neckF.SetMulInverseMul( torso2F, neckF );
		torso2F.SetMulInverseMul( torsoF, torso2F );

		bonesOut[ 0 ] = torsoF;
		bonesOut[ 1 ] = torso2F;
		bonesOut[ 2 ] = neckF;
		bonesOut[ 3 ] = headF;

		bonesOut[ 0 ].Translation = data.m_firstBoneLS.Translation;
		bonesOut[ 1 ].Translation = data.m_secondBoneLS.Translation;
		bonesOut[ 2 ].Translation = data.m_thirdBoneLS.Translation;
		bonesOut[ 3 ].Translation = data.m_fourthBoneLS.Translation;
	}

	/*
	// Back to LS
	Matrix firstLS = firstPS;							firstLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_firstBoneLS.getTranslation() ) );
	Matrix secondLS = secondPS * firstPS.Inverted();	secondLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_secondBoneLS.getTranslation() ) );
	Matrix thirdLS = thirdPS * secondPS.Inverted();		thirdLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_thirdBoneLS.getTranslation() ) );
	Matrix fourthLS = fourthPS * thirdPS.Inverted();	fourthLS.SetTranslation( TO_CONST_VECTOR_REF( data.m_fourthBoneLS.getTranslation() ) );

	// Output
	MatrixToHavokQsTransform( firstLS, bonesOut[ 0 ] );
	MatrixToHavokQsTransform( secondLS, bonesOut[ 1 ] );
	MatrixToHavokQsTransform( thirdLS, bonesOut[ 2 ] );
	MatrixToHavokQsTransform( fourthLS, bonesOut[ 3 ] );*/

	return insideLimits;
#endif
}
