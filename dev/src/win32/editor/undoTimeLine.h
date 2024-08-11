/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#pragma once

#include "undoStep.h"
#include "undoManager.h"

class CEdTimeline;
class ITimelineItem;

// ---------------------------------------------

struct STimelineItemLayout
{
	STimelineItemLayout() {}
	STimelineItemLayout( ITimelineItem* item );

	Float  m_start;
	Float  m_duration;
	Bool   m_isDuration;

	void Apply( ITimelineItem* item ) const;

	Bool operator==( const STimelineItemLayout& r ) const;
	Bool operator< ( const STimelineItemLayout& r ) const;

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		return *( const Uint32* ) &m_start ^ *( const Uint32* ) &m_duration; // Exclude m_isDuration from hash
	}
};

// ---------------------------------------------

struct STimelineItemStamp
{
	STimelineItemStamp() {}
	STimelineItemStamp( ITimelineItem* item );

	String m_trackName;
	STimelineItemLayout m_layout;

	Bool operator==( const STimelineItemStamp& r ) const;
	Bool operator< ( const STimelineItemStamp& r ) const;

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		return m_trackName.CalcHash() ^ m_layout.CalcHash();
	}
};

// ---------------------------------------------

struct STimelineItemInfo
{
public:
	STimelineItemInfo() {}
	STimelineItemInfo( ITimelineItem* item );
	STimelineItemInfo( ITimelineItem* item, CEdTimeline* timeline );

	TDynArray< Uint8 > m_serializedItem;
	STimelineItemStamp m_existingItem;
};

// ---------------------------------------------

class CUndoTimelineStep : public IUndoStep
{
public:
	CUndoTimelineStep() {}

	CUndoTimelineStep( CEdUndoManager& undoManager, CEdTimeline* timeline )
		: IUndoStep( undoManager )
		, m_timeline( timeline )
		{}

protected:
	ITimelineItem* FindItem( const STimelineItemStamp& stamp ) const;

	CEdTimeline* GetTimeLine() const
		{ return m_timeline; }

	void Invalidate();

private:
	CEdTimeline* m_timeline;
};

// ---------------------------------------------

class CUndoTimelineItemExistance : public CUndoTimelineStep
{
    DECLARE_ENGINE_CLASS( CUndoTimelineItemExistance, CUndoTimelineStep, 0 );	   

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdTimeline* timeline, ITimelineItem* item );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, CEdTimeline* timeline, ITimelineItem* item );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoTimelineItemExistance() {}
	CUndoTimelineItemExistance( CEdUndoManager& undoManager, CEdTimeline* timeLine );

	static CUndoTimelineItemExistance* PrepareStep( CEdUndoManager& undoManager, CEdTimeline* timeline );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	void DoCreationOn( TDynArray< STimelineItemInfo >& infos );
	void DoDeletionOn( TDynArray< STimelineItemInfo >& infos );
	TDynArray< STimelineItemInfo > m_created;
	TDynArray< STimelineItemInfo > m_deleted;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoTimelineItemExistance, CUndoTimelineStep );

// -----------------------------------------------

class CUndoTimelineItemLayout : public CUndoTimelineStep
{
    DECLARE_ENGINE_CLASS( CUndoTimelineItemLayout, CUndoTimelineStep, 0 );	   

	static void PrepareStep( CEdUndoManager& undoManager, CEdTimeline* timeline, ITimelineItem* item );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:

	CUndoTimelineItemLayout() {}
	CUndoTimelineItemLayout( CEdUndoManager& undoManager, CEdTimeline* timeline );

	void DoStep();
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	THashMap< ITimelineItem*, STimelineItemLayout >     m_tmpInfos;
 	THashMap< STimelineItemStamp, STimelineItemLayout > m_infos;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoTimelineItemLayout, CUndoTimelineStep );

// -----------------------------------------------

class CUndoTimelineTrackExistance : public CUndoTimelineStep
{
    DECLARE_ENGINE_CLASS( CUndoTimelineTrackExistance, CUndoTimelineStep, 0 );	 

public:
	static void CreateCreationStep( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& trackName );

	static void CreateDeletionStep( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& trackName );

private:
	CUndoTimelineTrackExistance() {}
	CUndoTimelineTrackExistance( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& trackName, Bool creating );

	void StoreTrackItems();

	void DoStep( Bool creating );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	Bool m_creating;
	String m_name;
	TDynArray< STimelineItemInfo > m_infos;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoTimelineTrackExistance, CUndoTimelineStep );

// -----------------------------------------------

class CUndoTimelineTrackRename : public CUndoTimelineStep
{
    DECLARE_ENGINE_CLASS( CUndoTimelineTrackRename, CUndoTimelineStep, 0 );	 

public:
	static void CreateStep( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& oldName, const String& newName );

private:
	CUndoTimelineTrackRename() {}
	CUndoTimelineTrackRename( CEdUndoManager& undoManager, CEdTimeline* timeline, const String& oldName, const String& newName );

	void DoStep();
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	String m_prevName;
	String m_curName;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoTimelineTrackRename, CUndoTimelineStep );
