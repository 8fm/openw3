/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorIkSolverMimicLookAt.h"

const Float IMimicLookAtSolver::VER_RANGE = 1.f;
const Float IMimicLookAtSolver::HOR_RANGE = 1.f;

Bool IMimicLookAtSolver::Solve( SolverData& data ) const
{
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform& boneMS = data.m_placerMS;

	const hkVector4& bonePosMS = boneMS.getTranslation();
	const hkVector4& targetMS = data.m_targetMS;

	hkQsTransform boneInvMS; boneInvMS.setInverse( boneMS );
	hkQsTransform target( hkQsTransform::IDENTITY );
	target.m_translation = targetMS;
	hkQsTransform targetPS; targetPS.setMul( boneInvMS, target );

	hkVector4 vecToTargetMS = targetPS.getTranslation();
	Float targetDist; targetDist = vecToTargetMS.length3();
	vecToTargetMS.normalize3();

	Bool insideLimits = false;

	// Angle vertical
	Float cosVer = vecToTargetMS.dot3( hkVector4( 0.f, 1.f, 0.f ) );
	Float angleVer = M_PI / 2.f - MAcos_safe( cosVer );

	Float angleVerDeg = RAD2DEG( angleVer );

	// Angle horizontal
	hkVector4 vecToTargetHorMS = vecToTargetMS; vecToTargetHorMS.zeroElement( 1 ); vecToTargetHorMS.normalize3();
	Float cosHor = vecToTargetHorMS.dot3( hkVector4( 0.f, 0.f, 1.f ) );
	Float angleHor = - ( M_PI / 2.f - MAcos_safe( cosHor ) );

	Float angleHorDeg = RAD2DEG( angleHor );

#else
	RedQsTransform& boneMS = data.m_placerMS;

//	const RedVector4& bonePosMS = boneMS.GetTranslation();
	const RedVector4& targetMS = data.m_targetMS;

	RedQsTransform boneInvMS; 
	boneInvMS.SetInverse( boneMS );

	RedQsTransform target( RedQsTransform::IDENTITY );
	target.Translation = targetMS;

	RedQsTransform targetPS; 
	targetPS.SetMul( boneInvMS, target );

	RedVector4 vecToTargetMS = targetPS.GetTranslation();
	Float targetDist; 
	targetDist = vecToTargetMS.Length3();
	vecToTargetMS.Normalize3();

	Bool insideLimits = false;

	// Angle vertical
	Float cosVer = Dot3( vecToTargetMS, RedVector4( 0.0f, 1.0f, 0.0f, 0.0f ) );
	Float angleVer = - ( M_PI / 2.f - MAcos_safe( cosVer ) );

	// Angle horizontal
	RedVector4 vecToTargetHorMS = vecToTargetMS; 
	vecToTargetHorMS.Normalize3();

	Float cosHor = Dot3( vecToTargetHorMS, RedVector4( 0.0f, 0.0f, 1.0f, 0.0f ) );
	Float angleHor = - ( M_PI / 2.f - MAcos_safe( cosHor ) );
#endif
	//BEH_LOG( TXT("ver %.2f"), RAD2DEG( angleVer ) );
	//BEH_LOG( TXT("hor %.2f"), RAD2DEG( angleHor ) );

	if ( data.m_mirrored )
	{
		angleVer *= -1.f;
		angleHor *= -1.f;
	}

	static Bool INVERSE_HOR_VALUE = true;
	static Bool INVERSE_VER_VALUE = false;
	if ( INVERSE_HOR_VALUE )
	{
		angleHor *= -1.f;
	}
	if ( INVERSE_VER_VALUE )
	{
		angleVer *= -1.f;
	}

	ASSERT( data.m_verMax >= 0.f );
	ASSERT( data.m_verMin >= 0.f );
	ASSERT( data.m_horMax >= 0.f );

	const Float verMaxWithSide = angleVer >= 0.f ? data.m_verMax : data.m_verMin;

	// Final tracks
	static Bool SWAP_AXIS = true;
	if ( !SWAP_AXIS )
	{
		data.m_horTrackValue = angleHor * data.m_weight / DEG2RAD( data.m_horMax );
		data.m_verTrackValue = ( angleVer + DEG2RAD( data.m_verOffset ) ) * data.m_weight / DEG2RAD( verMaxWithSide );
	}
	else
	{
		data.m_horTrackValue = angleVer * data.m_weight / DEG2RAD( data.m_horMax );
		data.m_verTrackValue = ( angleHor + DEG2RAD( data.m_verOffset ) ) * data.m_weight / DEG2RAD( verMaxWithSide );
	}
	
	if ( data.m_verTrackValue > VER_RANGE )
	{
		data.m_verTrackValue = VER_RANGE;
		insideLimits = true;
	}
	else if ( data.m_verTrackValue < -VER_RANGE )
	{
		data.m_verTrackValue = -VER_RANGE;
		insideLimits = true;
	}

	if ( data.m_horTrackValue > HOR_RANGE )
	{
		data.m_horTrackValue = HOR_RANGE;
		insideLimits = true;
	}
	else if ( data.m_horTrackValue < -HOR_RANGE )
	{
		data.m_horTrackValue = -HOR_RANGE;
		insideLimits = true;
	}

	//BEH_LOG( TXT("t ver %.2f"), data.m_verTrackValue );
	//BEH_LOG( TXT("t hor %.2f"), data.m_horTrackValue );

	return insideLimits;
}
