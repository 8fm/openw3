/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

class IChainSolver
{
protected:
	struct SolverData
	{
		Int32			m_startBone;
		Int32			m_endBone;
#ifdef USE_HAVOK_ANIMATION
		hkVector4	m_targetMS;
#else
		RedVector4  m_targetMS;
#endif
		Float		m_weight;
		Int32			m_solverSteps;
	};

	Bool Solve( const SolverData& data, const CSkeleton* skeleton, SBehaviorGraphOutput& poseInOut ) const;
#ifdef USE_HAVOK_ANIMATION
	Bool Solve( const SolverData& data, hkaPose& poseInOut ) const;
#endif
private:
#ifdef USE_HAVOK_ANIMATION
	void SyncPoses( const SBehaviorGraphOutput& poseIn, hkaPose& poseOut, Int32 boneStart, Int32 boneEnd ) const;
	void SyncPoses( const hkaPose& poseIn, SBehaviorGraphOutput& poseOut, Int32 boneStart, Int32 boneEnd ) const;
#endif
};
