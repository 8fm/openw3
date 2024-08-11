/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idSystem.h"
#include "idBasicBlocks.h"
#include "idGraphBlockChoice.h"

class CIDGraphBlockBranch;

#ifndef NO_DEBUG_PAGES
	class CDebugPageInteractiveDialogs;
#endif

class CIDThreadInstance : public CIDStateMachine
{
	struct SChoiceOption
	{
		const SIDOption*		m_option;
		const CIDGraphSocket*	m_output;
		Int8					m_outputIndex;

		Bool					m_active			: 1; // Why use more than one bit for a flag?
		Bool					m_visibleNow		: 1;
		Bool					m_visibleLastFrame	: 1;

		Bool IsVisibleNow()	const	{ return m_active && m_visibleNow; };
	};

protected:
	CIDTopicInstance*						m_parent;
	const CIDGraphBlockInput*				m_input;
	const CIDGraphBlock*					m_currentBlock;
	const CIDGraphBlock*					m_checkpoint;
	TDynArray< const CIDGraphBlockBranch* >	m_currentBranches;
	TDynArray< const CIDGraphBlockBranch* >	m_finishedBranches;
	const CIDGraphBlockChoice*				m_lastSelectedChoiceBlock;
	SChoiceOption							m_choiceOptions[ CHOICE_Max ];

public:
	CIDThreadInstance( CIDTopicInstance* parent, const CIDGraphBlockInput* input );
	~CIDThreadInstance();

	RED_INLINE const CIDGraphBlockInput*	GetInput		( ) const									{ return m_input; }
	RED_INLINE void						SetCheckpoint	( const CIDGraphBlock*	checkpoint )		{ m_checkpoint	= checkpoint; };

	void	OnStart						( Float timeDelta );
	void	Tick						( Float timeDelta );
	void	OnInterrupted				( );
	void	OnResume					( Float& timeDelta );
	void	OnFinished					( );

	void	JumpTo						( const CIDGraphSocket* socket ); 

	void	AddChoiceOption				( const SIDOption& option, const CIDGraphSocket* output, Int8 outputIndex );
	void	ClearChoiceOption			( EHudChoicePosition position );
	Bool	IsAnyOptionActive			( );
	void	OnRegularBlockEncountered	( const CIDGraphBlock* block );
	void	OnBranchBlockEncountered	( const CIDGraphBlockBranch* block ); 
	void	OnChoiceBlockEncountered	( const CIDGraphBlockChoice* block ); 
	void	OnDeadEndEncountered		( );

	void	OnChoiceSelected			( EHudChoicePosition position ); 
	void	ActivateChoiceOutput		( const CIDGraphSocket* output );

#ifndef NO_DEBUG_PAGES
	friend class CDebugPageInteractiveDialogs; 

protected:
	TDynArray< const CIDGraphBlock* > m_lastBlocks;
#endif
};
