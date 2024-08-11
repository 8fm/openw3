#pragma  once

#include "poseHandle.h"

struct SBehaviorGraphOutput;
struct SBehaviorSampleContext;

class CCacheBehaviorGraphOutput
{
	CPoseHandle		m_pose;
	SBehaviorSampleContext&		m_context;
	Bool						m_mimic;

public:
	CCacheBehaviorGraphOutput( SBehaviorSampleContext& context, Bool mimic = false );
	~CCacheBehaviorGraphOutput();

	RED_INLINE Bool IsOk() const { return m_pose != NULL; }

	RED_INLINE SBehaviorGraphOutput* GetPose() { ASSERT( m_pose ); return m_pose.Get(); }

private:
	CCacheBehaviorGraphOutput& operator=( const CCacheBehaviorGraphOutput& rhs );
	CCacheBehaviorGraphOutput( const CCacheBehaviorGraphOutput& );
};
