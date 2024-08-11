#pragma  once

#include "poseHandle.h"


class CPoseProvider;
struct SBehaviorGraphOutput;

class CAllocatedBehaviorGraphOutput
{
	DECLARE_RTTI_SIMPLE_CLASS( CAllocatedBehaviorGraphOutput );

	CPoseHandle	m_pose;
	Bool					m_mimic;
	CPoseProvider*			m_poseAlloc; // we need this to remove from proper context, or to copy
	CPoseProvider*			m_mimicAlloc; // we need this to remove from proper context, or to copy

public:
	CAllocatedBehaviorGraphOutput();
	~CAllocatedBehaviorGraphOutput();

	void Create( CBehaviorGraphInstance& instance, Bool mimic = false );

	void Cache( CBehaviorGraphInstance& instance );
	void Cache( const CAnimatedComponent* ac );
	void Cache( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &poseToCache );
	void Cache( const CAnimatedComponent* ac, const SBehaviorGraphOutput &poseToCache );
	void CreateAndCache( CBehaviorGraphInstance& instance, Bool mimic = false );

	void Free( CBehaviorGraphInstance& instance );

	RED_INLINE SBehaviorGraphOutput* GetPose() { ASSERT( m_pose ); return m_pose.Get(); }
	RED_INLINE const SBehaviorGraphOutput* GetPose() const { ASSERT( m_pose ); return m_pose.Get(); }
	RED_INLINE SBehaviorGraphOutput* GetPoseUnsafe() { return m_pose.Get(); } // may not exist!
	RED_INLINE const SBehaviorGraphOutput* GetPoseUnsafe() const { return m_pose.Get(); } // may not exist!
	RED_INLINE Bool HasPose() const { return m_pose; }

private:
	void Create( CPoseProvider* poseAlloc, CPoseProvider* mimicAlloc, Bool mimic = false );
	void Free();

public:
	CAllocatedBehaviorGraphOutput& operator=( const CAllocatedBehaviorGraphOutput& rhs );

private:
	CAllocatedBehaviorGraphOutput( const CAllocatedBehaviorGraphOutput& );
};

BEGIN_CLASS_RTTI( CAllocatedBehaviorGraphOutput );
END_CLASS_RTTI();
