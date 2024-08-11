#pragma once


// An interface for retrieving information that will help visualize the debugged data
class IQuestDebugInfo
{
public:
	virtual ~IQuestDebugInfo() {}

	//! Tells if there's a breakpoint toggled on the specified block
	virtual Bool IsBreakpointToggled( CQuestGraphBlock* block ) const = 0;

	//! Tells if the specified block is currently active
	virtual Bool IsBlockActive( CQuestGraphBlock* block ) const = 0;

	//! Tells if the specified block was visited
	virtual Bool IsBlockVisited( CQuestGraphBlock* block ) const = 0;

	//! Tells if the specified graph is the one from the currently observed thread
	virtual Bool IsGraphInActive( CQuestGraph* graph ) const = 0;
};
