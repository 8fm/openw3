/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CBehTreeMachine;
class IBehTreeNodeInstance;

class IBehTreeMachineListener
{
public:
	//! Node status changed
	virtual Bool OnNodeResultChanged( const IBehTreeNodeInstance* node, const Bool active ) = 0;

	//! Node reporting in
	virtual Bool OnNodeReport( const IBehTreeNodeInstance* node ) = 0;

	virtual void OnForceDebugColorSet( const IBehTreeNodeInstance* node, Uint8 R, Uint8 G, Uint8 B ) = 0;

	//! On tree state changed (stopped, started)
	virtual void OnTreeStateChanged() = 0;
	
	//! Stop listening request
	virtual void OnStopListeningRequest() = 0;

	//! Breakpoint reached
	virtual Bool OnBreakpoint( const IBehTreeNodeInstance* node ) = 0;
};

#ifdef EDITOR_AI_DEBUG

class IBehTreeDebugInterface
{
public:
	//! Start debugging tree
	virtual void DebugBehTreeStart( CBehTreeMachine* machine ) = 0;

	//! Start debugging all trees
	virtual void DebugBehTreeStopAll() = 0;
};

#endif	//EDITOR_AI_DEBUG