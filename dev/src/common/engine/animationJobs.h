
#pragma once

#include "../core/task.h"

struct STaskBatchSyncToken;

struct CJobImmediateUpdateAnimationContext
{
	Float	m_timeDelta;
	Bool	m_updateAnimations;
	Bool	m_updatePoseConstraints;

	CJobImmediateUpdateAnimationContext() : m_timeDelta( 0.f ), m_updateAnimations( false ), m_updatePoseConstraints( false ) {}
};

/// Immediate job used to update animation
class CJobImmediateUpdateAnimation : public CTask
{
private:
	STaskBatchSyncToken&				m_syncToken;

private:
	CAnimatedComponent*					m_component;
	CJobImmediateUpdateAnimationContext m_context;

public:
	CJobImmediateUpdateAnimation( STaskBatchSyncToken& syncToken, CAnimatedComponent* component );

	RED_INLINE void Setup( CJobImmediateUpdateAnimationContext context ) { m_context = context; }

public:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const { return TXT("UpdateAnimation"); }
	virtual Uint32 GetDebugColor() const { return COLOR_UINT32( 255, 255, 255 ); }
#endif

	//! Process task
	virtual void Run() override;
};

/// Async job used to update animation
class CJobAsyncUpdateAnimation : public CTask
{
private:
	CAnimatedComponent*					m_component;
	CJobImmediateUpdateAnimationContext m_context;

public:
	CJobAsyncUpdateAnimation( CAnimatedComponent* component, CJobImmediateUpdateAnimationContext context );
	virtual ~CJobAsyncUpdateAnimation();

public:
	//! Process the job, is called from job thread
	void Run() override;

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const { return TXT("AsyncUpdateAnimation"); }
	virtual Uint32 GetDebugColor() const { return COLOR_UINT32( 255, 255, 255 ); }
#endif

};
