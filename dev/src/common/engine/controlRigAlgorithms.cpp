
#include "build.h"
#include "controlRigAlgorithms.h"
#include "behaviorGraphOutput.h"
#include "behaviorIncludes.h"

namespace
{

	void ProjectNormal( const RedVector4& normal, const RedVector4& vector, RedVector4& inPlaneOut, RedVector4& outOfPlaneOut )
	{
		RED_ASSERT( normal.IsNormalized3(), TXT("Normal vector is not normalized") );
		RedFloat1 dot( Dot3( vector, normal ) );
		outOfPlaneOut = Mul( normal, dot );
		inPlaneOut = Sub( vector, outOfPlaneOut );
	}
}

#define IN_RANGE(x,a,b) (((x)>=(a))&&((x)<=(b)))

/*Bool Ik2Solver::CalcEndBoneMS( const Setup& setup, const SBehaviorGraphOutput& poseLSInOut, hkQsTransform& out )
{
	const Int32 firstJointParent = setup.m_skeleton->GetParentBoneIndex( setup.m_firstJointIdx );
	const Int32 secondJointParent = setup.m_skeleton->GetParentBoneIndex( setup.m_secondJointIdx );
	const Int32 endJointParent = setup.m_skeleton->GetParentBoneIndex( setup.m_endBoneIdx );

	hkQsTransform firstJointTransformMS;
	hkQsTransform secondJointTransformMS;
	hkQsTransform endTransformMS;

	const Bool oneSimpleChain = setup.m_firstJointIdx == secondJointParent && setup.m_secondJointIdx == endJointParent;
	if ( oneSimpleChain )
	{
		hkQsTransform firstJointParentTransformMS;
		firstJointParentTransformMS = setup.m_hasFirstJointParentTransformMS ? setup.m_firstJointParentTransformMS : poseLSInOut.GetBoneModelTransform( firstJointParent, setup.m_skeleton->GetParentIndices() );

		firstJointTransformMS.setMul( firstJointParentTransformMS, poseLSInOut.m_outputPose[ setup.m_firstJointIdx ] );
		secondJointTransformMS.setMul( firstJointTransformMS, poseLSInOut.m_outputPose[ setup.m_secondJointIdx ] );
		endTransformMS.setMul( secondJointTransformMS, poseLSInOut.m_outputPose[ setup.m_endBoneIdx ] );

		//firstJointTransformMS = poseLSInOut.GetBoneModelTransform( setup.m_firstJointIdx, setup.m_skeleton->GetParentIndices() );
		//secondJointTransformMS = poseLSInOut.GetBoneModelTransform( setup.m_secondJointIdx, setup.m_skeleton->GetParentIndices() );
		//endTransformMS = poseLSInOut.GetBoneModelTransform( setup.m_endBoneIdx, setup.m_skeleton->GetParentIndices() );

		out = endTransformMS;
		return true;
	}
	else
	{
		// TODO
		return false;
	}
}*/

Bool Ik2Solver::Solve( const Input& input, Output& output )
{
	ASSERT( input.m_hingeAxisLS.IsNormalized3() );
	ASSERT( input.m_endBoneRotationOffsetLS.IsOk() );

	ASSERT( IN_RANGE( input.m_cosineMaxHingeAngle, -1.0f, 1.0f ) );
	ASSERT( IN_RANGE (input.m_cosineMinHingeAngle, -1.0f, 1.0f ) );
	ASSERT( IN_RANGE( input.m_firstJointIkGain, 0.0f, 1.0f ) );
	ASSERT( IN_RANGE( input.m_secondJointIkGain, 0.0f, 1.0f ) );
	ASSERT( IN_RANGE (input.m_endJointIkGain, 0.0f, 1.0f ) );

	if ( !input.m_enforceEndPosition && !input.m_enforceEndRotation )
	{
		return true;
	}

	RedQsTransform firstJointTransformMS;
	RedQsTransform secondJointTransformMS;
	RedQsTransform endTransformMS;

	firstJointTransformMS.SetMul( input.m_firstJointParentTransformMS, input.m_firstJointLS );
	secondJointTransformMS.SetMul( firstJointTransformMS, input.m_secondJointLS );
	endTransformMS.SetMul( secondJointTransformMS, input.m_endBoneLS );

	RedVector4 targetMS;
	RedVector4 endPosMS;

	if ( input.m_enforceEndPosition )
	{
		const RedVector4& firstJointPosMS = firstJointTransformMS.GetTranslation();
		const RedVector4& secondJointPosMS = secondJointTransformMS.GetTranslation();

		if ( !input.m_enforceEndRotation )
		{
			targetMS = input.m_endTargetMS;
			endPosMS.SetTransformedPos( endTransformMS, input.m_endBoneOffsetLS );
		}
		else
		{
			RedVector4 inverseOffset;
			RedMatrix3x3 rotationMatrix;
			rotationMatrix.BuildFromQuaternion( input.m_endBoneRotationOffsetLS.Quat );

			inverseOffset.InverseRotateDirection(rotationMatrix, input.m_endBoneOffsetLS );

			RedVector4 targetDelta;
			targetDelta.RotateDirection( input.m_endTargetRotationMS, inverseOffset );

			targetMS = Sub( input.m_endTargetMS, targetDelta );
			endPosMS = endTransformMS.GetTranslation();
		}

		RedVector4 v1( Sub( secondJointPosMS, firstJointPosMS ) );
		RedVector4 v2( Sub( endPosMS, secondJointPosMS ) );
		RedVector4 iToG( Sub( targetMS, firstJointPosMS ) );

		RedQuaternion rotation = secondJointTransformMS.GetRotation();
		rotation.Normalize();

		RedVector4 hingeMS;
		hingeMS.RotateDirection( rotation, input.m_hingeAxisLS );

		RedVector4 v1ip;
		RedVector4 v1op;
		ProjectNormal( hingeMS, v1, v1ip, v1op );

		RedVector4 v2ip;
		RedVector4 v2op;
		ProjectNormal( hingeMS, v2, v2ip, v2op );

		RedVector4 hop( Sub( v1op, v2op ) );
		Float h_sqr = hop.SquareLength3();

		const Float D_sqr = iToG.SquareLength3();
		const Float Dip_sqr = D_sqr - h_sqr;

		const Float l1ip_sqr = v1ip.SquareLength3();
		const Float l2ip_sqr = v2ip.SquareLength3();
		const Float l1ip = Red::Math::MSqrt( l1ip_sqr );
		const Float l2ip = Red::Math::MSqrt( l2ip_sqr );

		ASSERT( input.m_cosineMaxHingeAngle < input.m_cosineMinHingeAngle );
		ASSERT( input.m_cosineMaxHingeAngle >= -1.0f );
		ASSERT( input.m_cosineMinHingeAngle >= -1.0f );
		ASSERT( input.m_cosineMaxHingeAngle <= 1.0f );
		ASSERT( input.m_cosineMinHingeAngle <= 1.0f );

		const Float cosAngle = Red::Math::NumericalUtils::Max(input.m_cosineMaxHingeAngle,
			Red::Math::NumericalUtils::Min( input.m_cosineMinHingeAngle, 
			( Dip_sqr - l1ip_sqr - l2ip_sqr ) / ( -2.0f * l1ip * l2ip ) ) );

		const Float desiredHingeAngle = Red::Math::MAcos( cosAngle );

		v1ip.Normalize3(); 
		v1ip.Negate();
		v2ip.Normalize3();

		const Float currentHingeAngle = Red::Math::MAcos( Dot3( v1ip, v2ip ) );

		const Float diffAngle = desiredHingeAngle - currentHingeAngle;
		
		RedQsTransform secondJointTransformLS;

		{
			RedQsTransform& refHingeMS = secondJointTransformMS;
			const RedQuaternion currentRot = refHingeMS.GetRotation();
			RedVector4 axis; 
			axis.RotateDirection( currentRot, input.m_hingeAxisLS );

			RedQuaternion extra( axis, diffAngle * input.m_secondJointIkGain * input.m_weight );
			refHingeMS.Rotation = RedQuaternion::Mul( extra, currentRot );
			refHingeMS.Rotation.Normalize();

			secondJointTransformLS.SetMulInverseMul( firstJointTransformMS, secondJointTransformMS );

			endTransformMS.SetMul( secondJointTransformMS, input.m_endBoneLS );
		}

		{
			const RedVector4& initMS = firstJointTransformMS.GetTranslation();
			const RedVector4& goalMS = targetMS;

			RedVector4 endMS;
			if ( input.m_enforceEndRotation )
			{
				endMS = endTransformMS.GetTranslation();
			}
			else
			{
				endMS.SetTransformedPos( endTransformMS, input.m_endBoneOffsetLS );
			} 

			RedVector4 iToE_n( Sub( endMS, initMS ) );  
			iToE_n.Normalize3();
			RedVector4 iToG_n( Sub( goalMS, initMS ) ); 
			iToG_n.Normalize3();

			RedQuaternion extra;
			//extra.SetShortestRotationDamped( input.m_firstJointIkGain * input.m_weight, iToE_n, iToG_n );
			extra.SetShortestRotation( iToE_n, iToG_n );
			extra.SetLerp( RedQuaternion::IDENTITY, extra, input.m_firstJointIkGain * input.m_weight );

			{
				RedQsTransform& refFirstJointMS = firstJointTransformMS;
				const RedQuaternion currentRot = refFirstJointMS.GetRotation();
				refFirstJointMS.Rotation = RedQuaternion::Mul( extra, currentRot );
				refFirstJointMS.Rotation.Normalize();

				secondJointTransformMS.SetMul( firstJointTransformMS, secondJointTransformLS );
				endTransformMS.SetMul( secondJointTransformMS, input.m_endBoneLS );
			}
		}
	}

	if ( input.m_enforceEndRotation )
	{
		const RedQuaternion& modelFromTargetRotation = input.m_endTargetRotationMS;
		const RedQuaternion& endFromTargetRotation = input.m_endBoneRotationOffsetLS;

		RedQuaternion modelFromEndRotationDesired;
		modelFromEndRotationDesired.SetMulInverse( modelFromTargetRotation, endFromTargetRotation );

		const RedQsTransform& modelFromEndCurrent = endTransformMS;

		RedQsTransform modelFromEnd = modelFromEndCurrent;
		modelFromEnd.Rotation.SetSlerp( modelFromEndCurrent.Rotation, modelFromEndRotationDesired, input.m_endJointIkGain * input.m_weight );
		endTransformMS = modelFromEnd;
	}

	output.m_firstJointLS.SetMulInverseMul( input.m_firstJointParentTransformMS, firstJointTransformMS );
	output.m_secondJointLS.SetMulInverseMul( firstJointTransformMS, secondJointTransformMS );
	output.m_endBoneLS.SetMulInverseMul( secondJointTransformMS, endTransformMS );

	output.m_firstJointMS = firstJointTransformMS;
	output.m_secondJointMS = secondJointTransformMS;
	output.m_endBoneMS = endTransformMS;

	return true;
}


//////////////////////////////////////////////////////////////////////////

Bool LookAtSolver::Solve( const LookAtSolver::Setup& setup, RedQsTransform& boneInOutLS )
{
	RedQsTransform temp;
	return Solve( setup, boneInOutLS, temp );
}

#ifdef USE_HAVOK_ANIMATION
Bool LookAtSolver::Solve( const LookAtSolver::Setup& setup, hkQsTransform& boneInOutLS, hkQsTransform& boneOutMS )
{
	const hkVector4& targetMS = TO_CONST_HK_VECTOR_REF( setup.m_targetMS );

	hkQsTransform boneMS; boneMS.setMul( setup.m_parentMS, boneInOutLS );

	hkVector4 vecToTargetMS;
	vecToTargetMS.setSub4( targetMS, boneMS.m_translation );

	hkReal targetDist; targetDist = vecToTargetMS.length3();
	vecToTargetMS.normalize3();

	Bool insideLimits = true;

	if ( setup.m_useLimits )
	{
		hkVector4 limitLS = BehaviorUtils::hkVectorFromAxis( setup.m_limitBoneFwdDirection );

		hkVector4 limitMS; 
		limitMS.setRotatedDir( setup.m_parentMS.getRotation(), limitLS );
		limitMS.normalize3();

		hkReal limitAngle = setup.m_limitAngle;

		if ( setup.m_useRangeLimits )
		{
			limitAngle = hkMath::fabs( setup.m_limitAngleLeft );
			limitAngle = hkMath::max2( limitAngle, hkMath::fabs( setup.m_limitAngleRight ) );
			limitAngle = hkMath::max2( limitAngle, hkMath::fabs( setup.m_limitAngleUp ) );
			limitAngle = hkMath::max2( limitAngle, hkMath::fabs( setup.m_limitAngleDown ) );
		}

		const hkReal cosLimitAngle = hkMath::cos( limitAngle );
		const hkReal cosTargetAngle = vecToTargetMS.dot3( limitMS );
		if ( cosTargetAngle < cosLimitAngle )
		{
			hkVector4 cross;
			cross.setCross( limitMS, vecToTargetMS );
			cross.normalize3();

			hkQuaternion q;
			q.setAxisAngle( cross, limitAngle );

			vecToTargetMS.setRotatedDir( q, limitMS );

			insideLimits = false;
		}

		if ( setup.m_useRangeLimits )
		{
			hkVector4 vecToTargetMSOld = vecToTargetMS;

			hkVector4 dirUpLS = BehaviorUtils::hkVectorFromAxis( setup.m_limitBoneUpDirection );

			hkVector4 dirUpMS; 
			dirUpMS.setRotatedDir( setup.m_parentMS.getRotation(), dirUpLS );
			dirUpMS.normalize3();

			hkVector4 sideAxisMS;
			sideAxisMS.setCross( dirUpMS, limitMS );

			hkMatrix3 worldFromFSU;
			hkMatrix3 fsuFromWorld;
			worldFromFSU.setCols( limitMS, sideAxisMS, dirUpMS );
			fsuFromWorld.setTranspose( worldFromFSU );

			hkVector4 vec;
			vec.setMul3( fsuFromWorld, vecToTargetMS );

			BehaviorUtils::SphericalFromCartesian( vec );

			hkVector4 maxVec;
			hkVector4 minVec;
			maxVec.set(  HK_REAL_MAX, setup.m_limitAngleLeft, setup.m_limitAngleUp, 0.0f );
			minVec.set( -HK_REAL_MAX, setup.m_limitAngleRight, setup.m_limitAngleDown, 0.0f );

			hkVector4 limitedVec = vec;
			limitedVec.setMin4( limitedVec, maxVec );
			limitedVec.setMax4( limitedVec, minVec );

			BehaviorUtils::CartesianFromSpherical( limitedVec );

			vecToTargetMS.setMul3( worldFromFSU, limitedVec );

			if ( !vecToTargetMS.equals3( vecToTargetMSOld ) )
			{
				insideLimits = false;
			}
		}
	}

	hkVector4 dirLS = BehaviorUtils::hkVectorFromAxis( setup.m_boneDirection );

	hkVector4 dirMS; 
	dirMS.setRotatedDir( boneMS.getRotation(), dirLS );
	dirMS.normalize3();

	hkVector4 cross;
	cross.setCross( dirMS, vecToTargetMS );
	cross.normalize3();

	const hkReal cosAngle = vecToTargetMS.dot3( dirMS );
	const hkReal angle = hkMath::acos( cosAngle ) * setup.m_weight;

	hkQuaternion q;
	q.setAxisAngle( cross, angle );

	/*
	const hkVector4& localBonePositionLS = TO_CONST_HK_VECTOR_REF( setup.m_localChildBoneOffset );
	const hkReal localOffsetDistLS = localBonePositionLS.length3();

	if ( localOffsetDistLS > 0.0f )
	{
		hkVector4 tmpAxis;
		tmpAxis.setCross( dirLS, localBonePositionLS );
		tmpAxis.normalize3();

		hkVector4 projAxis;
		projAxis.setCross( tmpAxis, dirLS );
		projAxis.normalize3();

		const hkReal boneOffsetHeighLS = projAxis.dot3( localBonePositionLS ) / projAxis.length3();

		const hkReal a1 = boneOffsetHeighLS;
		const hkReal c1 = targetDist;
		const hkReal c12 = c1*c1;
		const hkReal a12 = a1*a1;
		const hkReal b1 = hkMath::sqrt( hkMath::fabs( c12 - a12 ) );
		const hkReal b12 = b1*b1;

		hkReal a2 = 0.5f * hkMath::sqrt( hkMath::fabs( -2.0f*c12*b12+2.0f*b12*a12+c12*c12+2*c12*a12+b12*b12+a12*a12 ) ) / b1;

		hkReal correctionAngle = 0.0f;

		if ( a2 != 0.0f )
		{
			correctionAngle = hkMath::acos( a1/a2 );
		}

		hkVector4 boneOffsetPositionMS;
		boneOffsetPositionMS.setRotatedDir( boneModelSpaceInOut.getRotation(), localBonePositionLS );

		hkVector4 crossCorrection;
		crossCorrection.setCross( dirMS, boneOffsetPositionMS );
		crossCorrection.normalize3();

		hkQuaternion qCorrection;
		qCorrection.setAxisAngle( crossCorrection, -correctionAngle );

		q.mul(qCorrection);
	}*/

	boneMS.m_rotation.setMul( q, boneMS.m_rotation );

	boneInOutLS.setMulInverseMul( setup.m_parentMS, boneMS );
	boneOutMS = boneMS;

	return true;
}
#else
Bool LookAtSolver::Solve( const LookAtSolver::Setup& setup, RedQsTransform& boneInOutLS, RedQsTransform& boneOutMS )
{
	const RedVector4& targetMS = reinterpret_cast< const RedVector4& >( setup.m_targetMS );

	RedQsTransform boneMS; 
	boneMS.SetMul( setup.m_parentMS, boneInOutLS );

	RedVector4 vecToTargetMS( Sub( targetMS, boneMS.Translation ) );

	Float targetDist; 
	targetDist = vecToTargetMS.Length3();
	vecToTargetMS.Normalize3();

	Bool insideLimits = true;

	if ( setup.m_useLimits )
	{
		RedVector4 limitLS = BehaviorUtils::RedVectorFromAxis( setup.m_limitBoneFwdDirection );

		RedVector4 limitMS; 
		limitMS.RotateDirection( setup.m_parentMS.GetRotation(), limitLS );
		limitMS.Normalize3();

		Float limitAngle = setup.m_limitAngle;

		if ( setup.m_useRangeLimits )
		{
			limitAngle = Red::Math::MAbs( setup.m_limitAngleLeft );
			limitAngle = Red::Math::NumericalUtils::Max( limitAngle, Red::Math::MAbs( setup.m_limitAngleRight ) );
			limitAngle = Red::Math::NumericalUtils::Max( limitAngle, Red::Math::MAbs( setup.m_limitAngleUp ) );
			limitAngle = Red::Math::NumericalUtils::Max( limitAngle, Red::Math::MAbs( setup.m_limitAngleDown ) );
		}

		const Float cosLimitAngle = Red::Math::MCos( limitAngle );
		const Float cosTargetAngle = Dot3( vecToTargetMS, limitMS );
		if ( cosTargetAngle < cosLimitAngle )
		{
			RedVector4 cross( Cross( limitMS, vecToTargetMS ) );
			cross.Normalize3();

			RedQuaternion q;
			q.SetAxisAngle( cross, limitAngle );

			vecToTargetMS.RotateDirection( q, limitMS );

			insideLimits = false;
		}

		if ( setup.m_useRangeLimits )
		{
			RedVector4 vecToTargetMSOld = vecToTargetMS;

			RedVector4 dirUpLS = BehaviorUtils::RedVectorFromAxis( setup.m_limitBoneUpDirection );

			RedVector4 dirUpMS; 
			dirUpMS.RotateDirection( setup.m_parentMS.GetRotation(), dirUpLS );
			dirUpMS.Normalize3();

			RedVector4 sideAxisMS( Cross( dirUpMS, limitMS ) );

			RedMatrix4x4 worldFromFSU;
			RedMatrix4x4 fsuFromWorld;

			worldFromFSU.SetCols( limitMS, sideAxisMS, dirUpMS, RedVector4::ZEROS );
			fsuFromWorld = worldFromFSU.Transposed();

			RedVector4 vec( TransformVector( fsuFromWorld, vecToTargetMS ) );

			BehaviorUtils::SphericalFromCartesian( vec );

			RedVector4 maxVec;
			RedVector4 minVec;

			maxVec.Set( FLT_MAX, setup.m_limitAngleLeft, setup.m_limitAngleUp, 0.0f );
			minVec.Set( -FLT_MAX, setup.m_limitAngleRight, setup.m_limitAngleDown, 0.0f );

			RedVector4 limitedVec = vec;
			limitedVec = Min( limitedVec, maxVec );
			limitedVec = Max( limitedVec, minVec );

			BehaviorUtils::CartesianFromSpherical( limitedVec );

			vecToTargetMS = TransformVector( worldFromFSU, limitedVec );

			if ( !vecToTargetMS.IsAlmostEqual( vecToTargetMSOld ) )
			{
				insideLimits = false;
			}
		}
	}

	RedVector4 dirLS = BehaviorUtils::RedVectorFromAxis( setup.m_boneDirection );

	RedVector4 dirMS; 
	dirMS.RotateDirection( boneMS.GetRotation(), dirLS );
	dirMS.Normalize3();

	RedVector4 cross( Cross( dirMS, vecToTargetMS ) );
	cross.Normalize3();

	const Float cosAngle = Dot3( vecToTargetMS, dirMS );
	const Float angle = Red::Math::MCos( cosAngle ) * setup.m_weight;

	RedQuaternion q;
	q.SetAxisAngle( cross, angle );

	/*
	const hkVector4& localBonePositionLS = TO_CONST_HK_VECTOR_REF( setup.m_localChildBoneOffset );
	const hkReal localOffsetDistLS = localBonePositionLS.length3();

	if ( localOffsetDistLS > 0.0f )
	{
		hkVector4 tmpAxis;
		tmpAxis.setCross( dirLS, localBonePositionLS );
		tmpAxis.normalize3();

		hkVector4 projAxis;
		projAxis.setCross( tmpAxis, dirLS );
		projAxis.normalize3();

		const hkReal boneOffsetHeighLS = projAxis.dot3( localBonePositionLS ) / projAxis.length3();

		const hkReal a1 = boneOffsetHeighLS;
		const hkReal c1 = targetDist;
		const hkReal c12 = c1*c1;
		const hkReal a12 = a1*a1;
		const hkReal b1 = hkMath::sqrt( hkMath::fabs( c12 - a12 ) );
		const hkReal b12 = b1*b1;

		hkReal a2 = 0.5f * hkMath::sqrt( hkMath::fabs( -2.0f*c12*b12+2.0f*b12*a12+c12*c12+2*c12*a12+b12*b12+a12*a12 ) ) / b1;

		hkReal correctionAngle = 0.0f;

		if ( a2 != 0.0f )
		{
			correctionAngle = hkMath::acos( a1/a2 );
		}

		hkVector4 boneOffsetPositionMS;
		boneOffsetPositionMS.setRotatedDir( boneModelSpaceInOut.getRotation(), localBonePositionLS );

		hkVector4 crossCorrection;
		crossCorrection.setCross( dirMS, boneOffsetPositionMS );
		crossCorrection.normalize3();

		hkQuaternion qCorrection;
		qCorrection.setAxisAngle( crossCorrection, -correctionAngle );

		q.mul(qCorrection);
	}*/

	boneMS.Rotation.SetMul( q, boneMS.Rotation );

	boneInOutLS.SetMulInverseMul( setup.m_parentMS, boneMS );
	boneOutMS = boneMS;

	return true;
}
#endif
//////////////////////////////////////////////////////////////////////////

Bool SphericalChainIk3Solver::Solve( const SphericalChainIk3Solver::Setup& setup, SBehaviorGraphOutput& poseLSInOut )
{
	RedQsTransform temp;
	return Solve( setup, poseLSInOut, temp );
}

Bool SphericalChainIk3Solver::Solve( const SphericalChainIk3Solver::Setup& setup, SBehaviorGraphOutput& poseLSInOut, RedQsTransform& endBoneMSOut )
{
	ASSERT( IN_RANGE( setup.m_firstJointIdx, 0 , (Int32)poseLSInOut.m_numBones-1 ) );
	ASSERT( IN_RANGE( setup.m_secondJointIdx, 0 , (Int32)poseLSInOut.m_numBones-1 ) );
	ASSERT( IN_RANGE( setup.m_endBoneIdx, 0 , (Int32)poseLSInOut.m_numBones-1 ) );
	ASSERT( IN_RANGE( setup.m_firstJointIkGain, 0.0f, 1.0f ) );
	ASSERT( IN_RANGE( setup.m_secondJointIkGain, 0.0f, 1.0f ) );
	ASSERT( IN_RANGE (setup.m_endJointIkGain, 0.0f, 1.0f ) );
	ASSERT( IN_RANGE (setup.m_weight, 0.0f, 1.0f ) );

	static Float firstFactor = 0.5f;
	static Float secondFacor = 0.5f;

	const Float firstGain = setup.m_firstJointIkGain * firstFactor;
	const Float secondGain = setup.m_secondJointIkGain * secondFacor;
	const Float endGain = setup.m_endJointIkGain;

	RedQsTransform firstJointTransformMS;
	RedQsTransform secondJointTransformMS;
	RedQsTransform endTransformMS;

	RedQsTransform& firstJointTransformLS = poseLSInOut.m_outputPose[ setup.m_firstJointIdx ];
	RedQsTransform& secondJointTransformLS = poseLSInOut.m_outputPose[ setup.m_secondJointIdx ];
	RedQsTransform& endTransformLS = poseLSInOut.m_outputPose[ setup.m_endBoneIdx ];

	RedVector4 vecA;
	RedVector4 vecB;

	RedQuaternion quat, quatDamped;

	RedQsTransform rot( RedQsTransform::IDENTITY );

	const RedVector4& targetMS = reinterpret_cast< const RedVector4& >( setup.m_endTargetMS );
	RED_UNUSED( targetMS );

	const RedVector4 direction1 = BehaviorUtils::RedVectorFromAxis( setup.m_boneDirection1 );
	const RedVector4 direction2 = BehaviorUtils::RedVectorFromAxis( setup.m_boneDirection2 );
	const RedVector4 direction3 = BehaviorUtils::RedVectorFromAxis( setup.m_boneDirection3 );

	RedQsTransform fistMS, secondMS;

	// 1.
	{
		LookAtSolver::Setup lookAtSetup;
		lookAtSetup.m_boneDirection = setup.m_boneDirection1;
		lookAtSetup.m_parentMS = setup.m_firstJointParentTransformMS;
		lookAtSetup.m_targetMS = setup.m_endTargetMS;
		lookAtSetup.m_weight = firstGain;

		LookAtSolver::Solve( lookAtSetup, firstJointTransformLS, fistMS );
	}

	{
		//firstJointTransformMS.setMul( setup.m_firstJointParentTransformMS, firstJointTransformLS );
		firstJointTransformMS = fistMS;

		LookAtSolver::Setup lookAtSetup;
		lookAtSetup.m_boneDirection = setup.m_boneDirection2;
		lookAtSetup.m_parentMS = firstJointTransformMS;
		lookAtSetup.m_targetMS = setup.m_endTargetMS;
		lookAtSetup.m_weight = secondGain;

		LookAtSolver::Solve( lookAtSetup, secondJointTransformLS, secondMS );
	}

	{
		//secondJointTransformMS.setMul( firstJointTransformMS, secondJointTransformLS );
		secondJointTransformMS = secondMS;

		LookAtSolver::Setup lookAtSetup;
		lookAtSetup.m_boneDirection = setup.m_boneDirection3;
		lookAtSetup.m_parentMS = secondJointTransformMS;
		lookAtSetup.m_targetMS = setup.m_endTargetMS;
		lookAtSetup.m_weight = endGain;

		LookAtSolver::Solve( lookAtSetup, endTransformLS, endBoneMSOut );
	}

	return true;
}

Bool Ik2Baker::Bake( const Input& input, Output& output )
{
	Ik2Solver::Input solverInput;
	Ik2Solver::Output solverOutput;

	solverInput.m_firstJointParentTransformMS = input.m_firstJointParentTransformMS;
	solverInput.m_firstJointLS = input.m_firstJointLS;
	solverInput.m_secondJointLS = input.m_secondJointLS;
	solverInput.m_endBoneLS = input.m_endBoneLS;
	solverInput.m_hingeAxisLS = input.m_hingeAxisLS;
	solverInput.m_endTargetMS = input.m_endTargetMS;
	solverInput.m_endTargetRotationMS = input.m_endTargetRotationMS;
	solverInput.m_enforceEndPosition = input.m_enforceEndPosition;
	solverInput.m_enforceEndRotation = input.m_enforceEndRotation;
		
	Bool result = Ik2Solver::Solve( solverInput, solverOutput );

	output.m_firstBoneAdditive.SetMulMulInverse( solverOutput.m_firstJointLS, input.m_firstJointLS );
	output.m_secondBoneAdditive.SetMulMulInverse( solverOutput.m_secondJointLS, input.m_secondJointLS );
	output.m_endBoneAdditive.SetMulMulInverse( solverOutput.m_endBoneLS, input.m_endBoneLS );

	return result;
}
