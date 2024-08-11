/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"

struct SUndoHistoryEntry
{
	SUndoHistoryEntry( const String& name, const String& target, Uint32 m_id, Bool current )
		: m_name( name ), m_target( target ), m_current( current )
		{}

	String m_name;
	String m_target;
	Uint32 m_id;
	Bool   m_current;
};

typedef TDynArray< SUndoHistoryEntry > TUndoHistory;

class CEdUndoGroupMarker : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CEdUndoGroupMarker, IUndoStep, 0 );

public:
	CEdUndoGroupMarker() {}

	CEdUndoGroupMarker( CEdUndoManager& undoManager, const String& name, Int32 level, Bool begin )
		: IUndoStep( undoManager )
		, m_name( name )
		, m_begin( begin )
	{}

	virtual String GetName() override { return m_name; }

	Bool IsBegin() const { return m_begin; }

	Int32 GetLevel() const { return m_level; }

private:
	String m_name;
	Int32  m_level;
	Bool   m_begin;
};

DEFINE_SIMPLE_RTTI_CLASS( CEdUndoGroupMarker, IUndoStep );

class IEdUndoListener
{
public:
	IEdUndoListener(){}
	virtual ~IEdUndoListener(){}

	virtual void OnUndoHistoryChanged() =0;
};

class CEdUndoStepIterator
{
public:
	CEdUndoStepIterator( IUndoStep* head, Bool iterateInsideGroups );

	void operator ++();

	IUndoStep& operator*() { return *m_cur; }

	IUndoStep* operator->() { return m_cur; }

	IUndoStep* Get() { return m_cur; }

	Uint32 GetId() { return m_id; }

	operator Bool() const { return m_cur != nullptr; }

private:
	void StoreNext();

	IUndoStep* m_cur;
	IUndoStep* m_next;
	Uint32     m_id;
	Bool       m_iterateInsideGroups;
};

class CEdUndoManager : public CObject, public IEdEventListener, public ISavableToConfig
{
    DECLARE_ENGINE_CLASS( CEdUndoManager, CObject, 0 );	   

public:
	struct Transaction
	{
		Transaction( CEdUndoManager& manager, const String& name );
		~Transaction();
	private:
		CEdUndoManager& m_manager;
		String m_name;
	};

	CEdUndoManager( wxWindow* messageWindowParent = nullptr, Bool useDump = false );
	
	~CEdUndoManager();

	//! Setting this allows the manager to expand the menu items with step names
	void SetMenuItems( wxMenuItem* undoItem, wxMenuItem* redoItem );

	void SetWorld( CWorld *world ) { m_world = world; }

	CWorld *GetWorld() { return m_world; }

	void SetUndoListener( IEdUndoListener* listener );

    IUndoStep *GetCurrentStep();

	CEdUndoStepIterator GetStepsIterator( Bool iterateInsideGroups = false );

	void ClearHistory( Bool redoOnly = false );

	TUndoHistory GetHistory() const;

	void Undo(); // Undo 1 step

	void Redo(); // Redo 1 step

	Bool IsUndoInProgress() const { return m_undoInProgress; }

	void NotifyObjectRemoved( CObject *object );

	void NotifyObjectTrackedForRemoval( CObject* obj );

	IUndoStep *GetStepToAdd() { return m_stepToAdd; }

	void SetStepToAdd( IUndoStep *stepToAdd );

	template < typename UndoStepT > UndoStepT* SafeGetStepToAdd();

protected:
	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

	// Needed by GC
	virtual void OnSerialize( IFile& file ) override;

private:
	struct ScopedHistoryLock
	{
		ScopedHistoryLock( CEdUndoManager& manager );
		~ScopedHistoryLock();
		CEdUndoManager& m_manager;
	};

	IUndoStep*            m_stepToAdd; // Step that is currently being processed and will be added to history soon

	IUndoStep*            m_stepHistoryHead;
	IUndoStep*            m_stepHistoryTail;
	IUndoStep*            m_stepHistoryCurr;
	Uint32                m_stepHistoryCount;
	Uint32                m_stepHistoryCountMax;

	Bool                  m_undoInProgress; // Is Undo/Redo step in progress

	CWorld*               m_world;

	wxWindow*             m_messageWindowParent;

	Bool                  m_bLockHistoryChangedEvent;
	Bool                  m_blockedHistoryChangedEventOccured;

	Bool                  m_useDump;
	String                m_dumpFilePath;				// Path to dump file (for saving undo/redo step data, hopefully lowering memory usage)
	size_t                m_dumpToDiskMemoryThreshold;	// If MC_Editor memory usage goes above this amount, Undo/Redo steps will be given a chance to flush to disk, to save memory.

	wxMenuItem*           m_undoMenuItem;
	wxMenuItem*           m_redoMenuItem;

	TDynArray< String >   m_groupStack;

	IEdUndoListener*      m_undoListener;
	TDynArray< CObject* > m_objectsTrackedForRemoval;

private:
    friend class IUndoStep;

	virtual void OnFinalize() override;

    IUndoStep *GetStepToUndo()  { return m_stepHistoryCurr; }
    IUndoStep *GetStepToRedo()  { return m_stepHistoryCurr ? m_stepHistoryCurr->GetNextStep() : m_stepHistoryHead; }

    void PushStep ( IUndoStep *step );
    void PopStep  ( IUndoStep *step );

	void WriteStepToDisk( IFile& file, IUndoStep* step );	// Let an undo step write data out to a file, so that any used memory can be freed. Uses CObject::OnSerialize() to do the write.
															// Takes an IFile object, so we can open the file once and write a bunch if needed.

	void ReadStepFromDisk( IUndoStep* step );				// Let an undo step read data back, previously written during WriteStepToDisk(). Uses CObject::OnSerialize() to do the read.
	void DumpToDisk();										// Check if our memory threshold is reached, and dump undo steps until we're good again (or all steps are dumped).
	void CompactDumpFile();

	Bool PrepareUndo( IUndoStep *step, const String& msgTitle, const String& emptyQueueMsg ); // 'false' returned means 'do not continue'
	void FinalizeUndo( IUndoStep *step, const String& msgTitle );

	void UpdateMenuItem( wxMenuItem* item, IUndoStep *step, const String& text );
	void UpdateHistory();

    void TruncHistory();
    void HistoryChanged();

	void UpdateActiveUndoMgr();

	// Undo groups can be placed one in another, but cannot be interleaved
	void UndoGroupBegin( const String& name );
	void UndoGroupEnd( const String& name );

	static Uint32 DebugCallback( Char* buffer, Uint32 bufferSize );
};

BEGIN_NOCOPY_CLASS_RTTI( CEdUndoManager )
    PARENT_CLASS( CObject )
END_CLASS_RTTI();

template < typename UndoStepT >
UndoStepT* CEdUndoManager::SafeGetStepToAdd()
{
	if ( IUndoStep* step = GetStepToAdd() )
	{
		UndoStepT* result = Cast< UndoStepT >( step );
		ASSERT ( result, TXT("Finalize the previous undo step before starting a new one") );
		return result;
	}
	else
	{
		return nullptr;
	}
}
