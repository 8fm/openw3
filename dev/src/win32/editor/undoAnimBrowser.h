/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once

#include "undoTimeLine.h"

class CEdAnimBrowser;
class CMemoryFileReader;

class CUndoAnimBrowserAnimChange : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoAnimBrowserAnimChange, IUndoStep, 0 );	   

public:
	static void CreateStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor );

private:
	CUndoAnimBrowserAnimChange() {}
	CUndoAnimBrowserAnimChange( CEdUndoManager& undoManager, CEdAnimBrowser* editor );

	void DoStep();

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CEdAnimBrowser*             m_editor;
	CSkeletalAnimationSetEntry* m_animEntry;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoAnimBrowserAnimChange, IUndoStep );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SUndoAnimBrowserEventRef
{
	CName m_eventName;
	String m_trackName;
	Float m_startTime;

	SUndoAnimBrowserEventRef() {}
	SUndoAnimBrowserEventRef( CExtAnimEvent* event );

	void RemoveFrom( CSkeletalAnimationSetEntry* entry );
	void MoveTo( CSkeletalAnimationSetEntry* entry, Float moveTo );

	Bool Matches( CExtAnimEvent* event ) const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SUndoAnimBrowserPasteEventsEntryInfo
{
public:
	CSkeletalAnimationSetEntry* m_entry;
	TDynArray< SUndoAnimBrowserEventRef > m_existingEvents;

	SUndoAnimBrowserPasteEventsEntryInfo() {}
	SUndoAnimBrowserPasteEventsEntryInfo( CSkeletalAnimationSetEntry* entry, TDynArray< CExtAnimEvent* > const & events );

	void DoUndo( CEdAnimBrowser* editor );
	void DoRedo( CEdAnimBrowser* editor, TDynArray< Uint8 > & serializedItem, CName const & relativeToEventName, Float relPos );

private:
	void StoreEvents( TDynArray< CExtAnimEvent* > const & events );

	void Refresh( CEdAnimBrowser* editor );
};

class CUndoAnimBrowserPasteEvents : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoAnimBrowserPasteEvents, IUndoStep, 0 );	   

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CMemoryFileReader & source, CName const & relativeToEventName = CName::NONE, Float relPos = 0.0f );
	static void PrepareAddForEntry( CEdUndoManager& undoManager, CSkeletalAnimationSetEntry* entry, TDynArray< CExtAnimEvent* > const & events );
	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoAnimBrowserPasteEvents() {}
	CUndoAnimBrowserPasteEvents( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CMemoryFileReader & source, CName const & relativeToEventName, Float relPos );

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

	CEdAnimBrowser* m_editor;
	CName m_relativeToEventName;
	Float m_relPos;
	TDynArray< Uint8 > m_serializedItem;
	TDynArray<SUndoAnimBrowserPasteEventsEntryInfo> m_entries;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoAnimBrowserPasteEvents, IUndoStep );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo
{
public:
	CSkeletalAnimationSetEntry* m_entry;
	TDynArray< Uint8 > m_serializedItem;
	SUndoAnimBrowserEventRef m_eventRef;

	SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo() {}
	SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo( CSkeletalAnimationSetEntry* entry, CExtAnimEvent* event );

	void DoUndo( CEdAnimBrowser* editor );
	void DoRedo( CEdAnimBrowser* editor );

private:
	void Refresh( CEdAnimBrowser* editor );
};

class CUndoAnimBrowserFindAndRemoveEvents : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoAnimBrowserFindAndRemoveEvents, IUndoStep, 0 );	   

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor );
	static void PrepareAddRemovedEvent( CEdUndoManager& undoManager, CSkeletalAnimationSetEntry* entry, CExtAnimEvent* event );
	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoAnimBrowserFindAndRemoveEvents() {}
	CUndoAnimBrowserFindAndRemoveEvents( CEdUndoManager& undoManager, CEdAnimBrowser* editor );

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

	CEdAnimBrowser* m_editor;
	TDynArray<SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo> m_entries;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoAnimBrowserFindAndRemoveEvents, IUndoStep );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CUndoAnimBrowserFindAndMoveEvents : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoAnimBrowserFindAndMoveEvents, IUndoStep, 0 );	   

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CName const & eventName, Float relPos );
	static void PrepareAddForEntry( CEdUndoManager& undoManager, CSkeletalAnimationSetEntry* entry );
	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoAnimBrowserFindAndMoveEvents() {}
	CUndoAnimBrowserFindAndMoveEvents( CEdUndoManager& undoManager, CEdAnimBrowser* editor, CName const & eventName, Float relPos );

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

	CEdAnimBrowser* m_editor;
	CName m_eventName;
	Float m_relPos;
	TDynArray<CSkeletalAnimationSetEntry*> m_entries;

	void MoveBy( CSkeletalAnimationSetEntry* entry, Float moveBy ) const;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoAnimBrowserFindAndMoveEvents, IUndoStep );
