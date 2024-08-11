/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "animMath.h"

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////
/*
 *	Note, that this is not IK actually.
 *	Although as it is used with IK solvers, it was decided to have
 *	IK in the name for consistency.
 */
//////////////////////////////////////////////////////////////////////////

struct SApplyRotationIKSolverData;
struct SApplyRotationIKSolver;
class CBehaviorGraphInstance;
struct SBehaviorGraphOutput;

//////////////////////////////////////////////////////////////////////////

struct SApplyRotationIKSolverData
{
	DECLARE_RTTI_STRUCT( SApplyRotationIKSolverData );
	CName m_bone;

	SApplyRotationIKSolverData();
};

BEGIN_CLASS_RTTI( SApplyRotationIKSolverData );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone"), TXT( "BehaviorBoneSelection" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SApplyRotationIKSolver
{
	DECLARE_RTTI_STRUCT( SApplyRotationIKSolver );
	Int32 m_parentBone; // parent bone
	Int32 m_bone; // bone

	AnimVector4 m_startDirMS;
	AnimVector4 m_endDirMS;
	AnimQsTransform m_preIKTMS;

public:
	SApplyRotationIKSolver();

	void Setup( CBehaviorGraphInstance& instance, const SApplyRotationIKSolverData& data );

	RED_INLINE Bool IsValid() const { return m_bone != INDEX_NONE; }

public:
	void StorePreIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output );

	void SetAdjustment( AnimVector4 const & startDirMS, AnimVector4 const & endDirMS );

	void UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SApplyRotationIKSolverData& data, Float weight, Bool updateTranslationToKeepDir = false, Float additionalOffsetAlongEndDir = 0.0f );
};

BEGIN_CLASS_RTTI( SApplyRotationIKSolver );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

