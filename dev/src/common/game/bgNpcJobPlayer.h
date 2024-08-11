
#pragma once

#include "../../common/engine/behaviorGraphAnimationManualSlot.h"
#include "../../common/game/jobTreeNode.h"

class IJobTreeAnimObject
{
public:
	virtual void PlayJobAnimation( const SAnimationState& animation ) = 0;
	virtual void PlayJobAnimations( const SAnimationState& animationA, const SAnimationState& animationB, Float weight ) = 0;
	virtual Float GetJobAnimationDuration( const CName& animation ) const = 0;

public:
	virtual ~IJobTreeAnimObject() {}
};
class CJobTreePlayer : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	TSoftHandle< CJobTree >&	m_tree; // & because of GC, :(
	IJobTreeAnimObject*			m_object;
	
	SJobTreeExecutionContext	m_treeContext;
	CName						m_category;
	Bool						m_randStartingAnimTime;
	Bool						m_canSkipFirstPreJob;
	CName						m_currActionItemName;
	Bool						m_requestedJobEnd;
	Bool						m_hasEnded;

	struct JobNodeState
	{
		Float	m_currTime;
		Float	m_prevTime;

		Float	m_blendTime;
		Float	m_duration;
		Float	m_speed;

		CName	m_animation;

		JobNodeState() : m_currTime( 0.f ), m_prevTime( 0.f ), m_blendTime( 0.f ), m_duration( 0.f ), m_speed( 0.f ) {}

		void Set( Float blendDuration, Float duration, const CName& animation, Float speed )
		{
			m_prevTime = 0.f;
			m_currTime = 0.f;
			m_blendTime = duration - blendDuration;
			m_duration = duration;
			m_animation = animation;
			m_speed = speed;

			if ( m_blendTime < 0.f )
			{
				m_blendTime = m_duration;
			}
		}
	};

	JobNodeState				m_nodeA;
	JobNodeState				m_nodeB;

	enum EJobTreePlayerState
	{
		JTPS_Loading,
		JTPS_Error,
		JTPS_PlayNextAnimation,
		JTPS_UpdateAnimation,
		JTPS_StartBlendingAnimations,
		JTPS_BlendAnimations,
	};

	EJobTreePlayerState			m_state;

public:
	CJobTreePlayer( TSoftHandle< CJobTree >& tree, const CName& category, IJobTreeAnimObject* object );
	~CJobTreePlayer();

	void Update( const Float dt );

	CName GetCurrentActionsItemName() const;

	RED_INLINE Bool HasEnded() const { return m_hasEnded; }
	RED_INLINE void RequestEnd() { m_requestedJobEnd = true; }

private:
	Bool SelectNextJobAnimation( JobNodeState& state );

	void SyncObjectToAnimation( JobNodeState& state );
	void SyncObjectToAnimations( JobNodeState& stateA, JobNodeState& stateB, Float weight );

	void SetState( EJobTreePlayerState state );
};

//////////////////////////////////////////////////////////////////////////

class CJobTreeDebugObject : public IJobTreeAnimObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );

public:
	virtual void PlayJobAnimation( const SAnimationState& animation );
	virtual void PlayJobAnimations( const SAnimationState& animationA, const SAnimationState& animationB, Float weight );
	virtual Float GetJobAnimationDuration( const CName& animation ) const;
};

//////////////////////////////////////////////////////////////////////////

class CJobTreeBehaviorSlot : public IJobTreeAnimObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	CBehaviorManualSlotInterface	m_slot;
	const CAnimatedComponent*		m_owner;

public:
	CJobTreeBehaviorSlot();
	~CJobTreeBehaviorSlot();

	Bool Initialize( const CAnimatedComponent* ac );
	void Clear();

public:
	virtual void PlayJobAnimation( const SAnimationState& animation );
	virtual void PlayJobAnimations( const SAnimationState& animationA, const SAnimationState& animationB, Float weight );
	virtual Float GetJobAnimationDuration( const CName& animation ) const;
};

//////////////////////////////////////////////////////////////////////////

class CBgJobTreeObject : public CJobTreeBehaviorSlot
{
	// TODO
};
