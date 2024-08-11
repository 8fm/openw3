#include "animMath.h"
/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT
//#define DEBUG_TWO_BONES_IK_SOLVER
#endif

//////////////////////////////////////////////////////////////////////////

struct STwoBonesIKSolverBoneData;
struct STwoBonesIKSolverData;
struct STwoBonesIKSolver;
struct SBehaviorGraphOutput;
class CRenderFrame;

//////////////////////////////////////////////////////////////////////////

struct STwoBonesIKSolverBoneData
{
	DECLARE_RTTI_STRUCT( STwoBonesIKSolverBoneData );
	CName m_name;

	STwoBonesIKSolverBoneData();
};

BEGIN_CLASS_RTTI( STwoBonesIKSolverBoneData );
	PROPERTY_CUSTOM_EDIT( m_name, TXT("Bone"), TXT( "BehaviorBoneSelection" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct STwoBonesIKSolverData
{
	DECLARE_RTTI_STRUCT( STwoBonesIKSolverData );

	STwoBonesIKSolverBoneData m_upperBone;
	STwoBonesIKSolverBoneData m_jointBone;
	STwoBonesIKSolverBoneData m_subLowerBone; // used for horses - between joint and lower - doesn't use auto setup dir!
	STwoBonesIKSolverBoneData m_lowerBone;
	STwoBonesIKSolverBoneData m_ikBone;
	Vector m_nextDirUpperBS;
	Vector m_nextDirJointBS;
	Vector m_nextDirLowerBS;
	Vector m_sideDirUpperBS;
	Vector m_sideDirJointBS;
	Vector m_sideDirLowerBS;
	Vector m_bendDirUpperBS;
	Vector m_bendDirJointBS;
	Vector m_bendDirLowerBS;
	Float m_jointSideWeightUpper;
	Float m_jointSideWeightJoint;
	Float m_jointSideWeightLower;
	Bool m_reverseBend;
	Bool m_autoSetupDirs;
	Bool m_allowToLock;
	Float m_limitToLengthPercentage;

	STwoBonesIKSolverData();
};

BEGIN_CLASS_RTTI( STwoBonesIKSolverData );
	PROPERTY_EDIT( m_upperBone, TXT("Start of limb") );
	PROPERTY_EDIT( m_jointBone, TXT("Joint (elbow/knee)") );
	PROPERTY_EDIT( m_subLowerBone, TXT("Between joint and lower. For horses") );
	PROPERTY_EDIT( m_lowerBone, TXT("End of limb (hand/foot)") );
	PROPERTY_EDIT( m_ikBone, TXT("End of limb (hand/foot)") );
	PROPERTY_EDIT( m_limitToLengthPercentage, TXT("Limit to length percentage") );
	PROPERTY_EDIT( m_reverseBend, TXT("Reverse bend") );
	PROPERTY_EDIT( m_allowToLock, TXT("Allow to lock in reverse bend dir") );
	PROPERTY_EDIT( m_autoSetupDirs, TXT("Auto calculate joint side dir") );
	PROPERTY_EDIT( m_jointSideWeightUpper, TXT("How much this bone affects joint's side dir") );
	PROPERTY_EDIT( m_jointSideWeightJoint, TXT("How much this bone affects joint's side dir") );
	PROPERTY_EDIT( m_jointSideWeightLower, TXT("How much this bone affects joint's side dir") );
	PROPERTY_EDIT_NAME( m_nextDirUpperBS, TXT("Joint to-next dir in upper's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in upper's bone space") );
	PROPERTY_EDIT_NAME( m_nextDirJointBS, TXT("Joint to-next dir in joint's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in joint's bone space") );
	PROPERTY_EDIT_NAME( m_nextDirLowerBS, TXT("Joint to-next dir in lower's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in lower's bone space") );
	PROPERTY_EDIT_NAME( m_sideDirUpperBS, TXT("Joint side dir in upper's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in upper's bone space") );
	PROPERTY_EDIT_NAME( m_sideDirJointBS, TXT("Joint side dir in joint's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in joint's bone space") );
	PROPERTY_EDIT_NAME( m_sideDirLowerBS, TXT("Joint side dir in lower's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in lower's bone space") );
	PROPERTY_EDIT_NAME( m_bendDirUpperBS, TXT("Joint bend dir in upper's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in upper's bone space") );
	PROPERTY_EDIT_NAME( m_bendDirJointBS, TXT("Joint bend dir in joint's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in joint's bone space") );
	PROPERTY_EDIT_NAME( m_bendDirLowerBS, TXT("Joint bend dir in lower's BS"), TXT("Additional offset to make sure that joint bends in right direction - it is in lower's bone space") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct STwoBonesIKSolver
{
	DECLARE_RTTI_STRUCT( STwoBonesIKSolver );
	Int32 m_parentBone; // parent bone of upper
	Int32 m_upperBone; // upper bone, start of limb
	Int32 m_jointBone; // joint, elbow/knee
	Int32 m_subLowerBone; // for horses - between joint and lower
	Int32 m_lowerBone; // lower bone, hand/foot
	Int32 m_ikBone; // provided ik bone
	AnimVector4 m_nextDirUpperBS;
	AnimVector4 m_nextDirJointBS;
	AnimVector4 m_nextDirLowerBS;
	AnimVector4 m_sideDirUpperBS;
	AnimVector4 m_sideDirJointBS;
	AnimVector4 m_sideDirLowerBS;
	AnimVector4 m_bendDirUpperBS;
	AnimVector4 m_bendDirJointBS;
	AnimVector4 m_bendDirLowerBS;
	AnimFloat m_jointSideWeightUpper;
	AnimFloat m_jointSideWeightJoint;
	AnimFloat m_jointSideWeightLower;
	AnimFloat m_upperLength;
	AnimFloat m_jointLength;
	AnimFloat m_limitToLengthPercentage;
	AnimFloat m_lock;

	AnimQsTransform m_targetLowerTMS; // target lower mat (this is exactly where that lower bone will be, as this is low level solver) (in model space!)
	AnimVector4 m_additionalSideDirMS;

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	Matrix m_upperMatWS;
	Matrix m_jointMatWS;
	Matrix m_lowerMatWS;
	Vector m_debugRawSideDirMS;
	Vector m_debugSideDirMS;
	Vector m_debugJointSideMS;
	Vector m_debugJointSideDirUpperBS;
	Vector m_debugJointSideDirJointBS;
	Vector m_debugJointSideDirLowerBS;
	Matrix m_upperMatWSPost;
	Matrix m_jointMatWSPost;
	Matrix m_lowerMatWSPost;
	Matrix m_debugTargetWS;
	Matrix m_debugBoneAWS_1;
	Matrix m_debugBoneBWS_1;
	Matrix m_debugBoneCWS_1;
	Matrix m_debugBoneDWS_1;
	Matrix m_debugBoneAWS_2;
	Matrix m_debugBoneBWS_2;
	Matrix m_debugBoneCWS_2;
	Matrix m_debugBoneDWS_2;
	Matrix m_debugBoneAWS_3;
	Matrix m_debugBoneBWS_3;
	Matrix m_debugBoneCWS_3;
	Matrix m_debugBoneDWS_3;
	Float m_debugA;
	Float m_debugB;
	Float m_debugC;
	Float m_debugD;
	Float m_debugE;
#endif

public:
	STwoBonesIKSolver();

	void Setup( CBehaviorGraphInstance& instance, const STwoBonesIKSolverData& data );

public:
	/** Get transform for upper and lower bone or IK if present(as it is in output) */
	void GetUpperAndLowerIKTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outUpper, AnimQsTransform& outLower ) const;
		
	/** Get transform for upper and lower bone (as it is in output) */
	void GetUpperAndLowerTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outUpper, AnimQsTransform& outLower ) const;

	/** Get transform for lower bone (as it is in output) */
	void GetLowerTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outLower ) const;

	/** Get transform for upper bone (as it is in output) */
	void GetUpperTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outUpper ) const;

	/** Get lower bone in reference pose */
	void GetRefLowerTMS( CBehaviorGraphInstance& instance, AnimQsTransform& outLower ) const;

	/** Set target transform for lower bone */
	void SetTargetLowerTMS( const AnimQsTransform& targetTMS );

	/** Set additional side dir that will move joint bone (knee, elbow) */
	void SetAdditionalSideDirMS( const AnimVector4& additionalSideDirMS );

	/** Update pose - lower bone should be where target bone is */
	void UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const STwoBonesIKSolverData& data, Float weight, Float timeDelta );

	/** Get length of whole limb */
	Float GetLength() const { return m_upperLength + m_jointLength; }

public:
	void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	void GatherDebugData( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const STwoBonesIKSolverData& data );
#endif
};

BEGIN_CLASS_RTTI( STwoBonesIKSolver );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

