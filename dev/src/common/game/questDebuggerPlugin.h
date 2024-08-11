/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "questsSystem.h"
#include "../engine/debugServerPlugin.h"

class CQuestDebuggerPlugin : public CDebugServerPlugin, public IQuestSystemListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );

public:
	// common
	virtual Bool Init() final;
	virtual Bool ShutDown() final;

	// life-time
	virtual void GameStarted() final;
	virtual void GameStopped() final;
	virtual void AttachToWorld() final;
	virtual void DetachFromWorld() final;	
	virtual void Tick() final;

	void SendCallstack();
	void ToggleBreakpoint( Uint64 pointer, CGUID guid );
	void StartInteractionDialog( Uint64 pointer );
	void ContinueFromBreakpoint();
	void ContinueFromPin( CQuestThread* thread, CQuestGraphBlock* block, const String& socketName, const String& socketDirection );
	void KillSignal( CQuestThread* thread, CQuestGraphBlock* block );

	// IQuestSystemListener implementation
	virtual void OnQuestStarted( CQuestThread* thread, CQuest& quest ) override;
	virtual void OnQuestStopped( CQuestThread* thread ) override;
	virtual void OnSystemPaused( Bool paused ) override;

	virtual void OnThreadPaused( CQuestThread* thread, Bool paused ) override;
	virtual void OnAddThread( CQuestThread* parentThread, CQuestThread* thread ) override;
	virtual void OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread ) override;
	virtual void OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block ) override;
	virtual void OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block ) override;
	virtual void OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block ) override;

private:
	String CreateBlockName( CQuestThread* thread, const CQuestGraphBlock* block ) const;
	void UpdateBreakpointData( const CQuestGraphBlock* block );
	void CheckBreakpoints( const CQuestGraphBlock* block );
	void PauseQuestSystem();
	void UnpauseQuestSystem();
	void Continue();
	CQuestsSystem* m_questSystem;
	Bool m_requestBreakpoint;
	THashMap<CGUID, const CQuestGraphBlock*> m_breakpoints;
};
