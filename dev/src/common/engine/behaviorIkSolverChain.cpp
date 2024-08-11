/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphOutput.h"
#include "behaviorIkSolverChain.h"

#ifdef USE_HAVOK_ANIMATION
void IChainSolver::SyncPoses( const SBehaviorGraphOutput& poseIn, hkaPose& poseOut, Int32 boneStart, Int32 boneEnd ) const
{
	hkArray<hkQsTransform>& poseLocalTrans = poseOut.accessUnsyncedPoseLocalSpace();
	
	// Not sure what this is meant to assert
	//ASSERT( poseIn.m_numBones, poseLocalTrans.getSize() );
	ASSERT( boneStart < (Int32)poseIn.m_numBones );
	ASSERT( boneEnd < (Int32)poseIn.m_numBones );

	for ( Int32 i=0; i<=boneEnd; i++ )
	{
		poseLocalTrans[i] = poseIn.m_outputPose[i];
	}

	poseOut.syncModelSpace();
}
#else
RED_MESSAGE( "IChainSolver::SyncPoses uses hkaPose which has been depricated" )
#endif
#ifdef USE_HAVOK_ANIMATION
void IChainSolver::SyncPoses( const hkaPose& poseIn, SBehaviorGraphOutput& poseOut, Int32 boneStart, Int32 boneEnd ) const
{;
	ASSERT( boneStart < (Int32)poseOut.m_numBones );
	ASSERT( boneEnd < (Int32)poseOut.m_numBones );

	for ( Int32 i=boneStart; i<=boneEnd; i++ )
	{
		poseOut.m_outputPose[i] = poseIn.getBoneLocalSpace( i );
	}
}
#else
RED_MESSAGE( "IChainSolver::SyncPoses uses hkaPose which has been depricated" )
#endif

Bool IChainSolver::Solve( const IChainSolver::SolverData& data, const CSkeleton* skeleton, SBehaviorGraphOutput& poseInOut ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton* havokSkeleton = skeleton->GetHavokSkeleton();
	if ( !havokSkeleton )
	{
		return false;
	}

	// Input data
	hkaCcdIkSolver::IkConstraint constraint;
	constraint.m_endBone = (hkInt16)data.m_endBone;
	constraint.m_startBone = (hkInt16)data.m_startBone;
	constraint.m_targetMS = data.m_targetMS;

	// Have to convert behavior pose to havok pose - this is slow...
	hkaPose havokPose( havokSkeleton );
	SyncPoses( poseInOut, havokPose, data.m_startBone, data.m_endBone );

	// Constraint array
	hkArray< hkaCcdIkSolver::IkConstraint > constraints;
	constraints.pushBack( constraint );

	// Solver
	hkaCcdIkSolver solver( data.m_solverSteps, data.m_weight );
	Bool ret = solver.solve( constraints, havokPose );

	SyncPoses( havokPose, poseInOut, data.m_startBone, data.m_endBone );

	return ret;
#else
	HALT( "Really needs to be implemented" );
	return false;
#endif
}

#ifdef USE_HAVOK_ANIMATION
Bool IChainSolver::Solve( const IChainSolver::SolverData& data, hkaPose& poseInOut ) const
{
	// Input data
	hkaCcdIkSolver::IkConstraint constraint;
	constraint.m_endBone = (hkInt16)data.m_endBone;
	constraint.m_startBone = (hkInt16)data.m_startBone;
	constraint.m_targetMS = data.m_targetMS;

	// Constraint array
	hkArray< hkaCcdIkSolver::IkConstraint > constraints;
	constraints.pushBack( constraint );

	// Solve
	hkaCcdIkSolver solver( data.m_solverSteps, data.m_weight );
	return solver.solve( constraints, poseInOut );
}
#else
RED_MESSAGE( "IChainSolver::Solve uses hkaPose which has been depricated" )
#endif