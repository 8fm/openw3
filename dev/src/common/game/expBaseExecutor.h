
#pragma once

#include "expIntarface.h"
#include "../../common/engine/behaviorGraphAnimationManualSlot.h"

class ExpBaseExecutor : public IExpExecutor
{
protected:
	const CEntity*					m_entity;
	Float							m_timeMul;
	Float							m_duration;
	Float							m_blendIn;
	Float							m_blendOut;
	Float							m_earlyEndOffset;
	CName							m_raiseBehaviorEventAtEnd;
	CName							m_callScriptEventAtEnd;
	Bool							m_firstUpdate;
	Bool							m_raisedBehaviorEventAtEnd;
	CBehaviorManualSlotInterface	m_slot;
	Bool							m_blendOutOnEnd;
	Bool							m_stayInSlotAfterEnd;

	ExpBaseExecutor( const ExecutorSetup& setup, Float blendIn = 0.f, Float blendOut = 0.f, Float earlyEndOffset = 0.0f );

public:
	virtual ~ExpBaseExecutor();

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	virtual void SyncAnim( Float time ) = 0;

	void SetTimeMul( Float p );
	void SyncAnimToStart();
	void SyncAnimToEnd();

	void RaiseBehaviorEventAtEnd( CName& raiseBehaviorEventAtEnd ) { m_raiseBehaviorEventAtEnd = raiseBehaviorEventAtEnd; }
	void CallScriptEventAtEnd( CName& callScriptEventAtEnd ) { m_callScriptEventAtEnd = callScriptEventAtEnd; }

	void BlendOutOnEnd( Bool blendOutOnEnd = true ) { m_blendOutOnEnd = blendOutOnEnd; }

	void StayInSlotAfterEnd( Bool stayInSlotAfterEnd = true ) { m_stayInSlotAfterEnd = stayInSlotAfterEnd; if ( m_stayInSlotAfterEnd ) { m_blendOut = 0.0f; } }

	void OnActionStoped();

protected:
	virtual Bool UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result ) = 0;

	void CloseSlot();
	const CSkeletalAnimationSetEntry* FindAnimation( const CEntity* entity, const CName& animation );

	void WrapTime( Float& time, Float& marker ) const;
	void UnwrapTime( Float& time, Float& marker ) const;

	Float CalcWeight( Float time ) const;
	Bool IsBlendingOut( Float time ) const;

	void AlignPreTimeToCurr( Float& prev, const Float curr ) const;

	void RaiseBehaviorEventAtEnd(); // raise it now

private:
	Bool FindAnimSlot( const CEntity* entity, const CName& slotName, CBehaviorManualSlotInterface& slot );
};
