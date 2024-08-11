/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CQuestVariedInputsBlock : public CQuestGraphBlock
{
	friend class CUndoQuestGraphVariedInputBlock;

	DECLARE_ENGINE_CLASS( CQuestVariedInputsBlock, CQuestGraphBlock, 0 )

private:
	Uint32 m_inputsCount;

public:
	CQuestVariedInputsBlock();
	virtual ~CQuestVariedInputsBlock() {}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	void AddInput();
	void RemoveInput();
	Bool CanRemoveInput() const;

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; }

#endif

protected:
	RED_INLINE Uint32 GetInputsCount() const { return m_inputsCount; }
};

BEGIN_CLASS_RTTI( CQuestVariedInputsBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY( m_inputsCount )
END_CLASS_RTTI()
