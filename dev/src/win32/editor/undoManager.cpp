#include "build.h"

#include "undoManager.h"
#include "undoProperty.h"
#include "popupNotification.h"

IMPLEMENT_ENGINE_CLASS( CEdUndoManager );
IMPLEMENT_ENGINE_CLASS( CEdUndoGroupMarker );

RED_DECLARE_DEBUG_CALLBACK_TAG( Undo );

namespace
{
	struct Globals
	{
		Bool m_dbgCallbackRegistered;
		THashMap< const CEdUndoManager*, TUndoHistory > m_history;
		CEdUndoManager* m_activeUndoMgr;
		String m_activeUndoMgrDesc;
		Red::Threads::CMutex m_mutex;

		Globals()
			: m_dbgCallbackRegistered( false )
			, m_activeUndoMgr( nullptr )
			{ }
	};

	Globals& GetGlobals()
	{
		static Globals s_globals;
		return s_globals;
	}
}

CEdUndoStepIterator::CEdUndoStepIterator( IUndoStep* head, Bool iterateInsideGroups )
	: m_iterateInsideGroups( iterateInsideGroups )
{
	m_cur = head;
	// the next step is stored upfront each time, so we are safe to iterate even if the current step gets removed
	StoreNext();	
}

void CEdUndoStepIterator::operator ++()
{
	m_cur = m_next;
	StoreNext();	
}

void CEdUndoStepIterator::StoreNext()
{
	m_next = m_cur ? m_cur->GetNextStep() : nullptr;

	// NOTE: It is done in the way that not-yet-closed undo groups are not visible (m_next is nullptr)
	if ( !m_iterateInsideGroups && m_cur && m_next && m_cur->IsA< CEdUndoGroupMarker >() )
	{
		// Skip over an undo group
		CEdUndoGroupMarker* marker = SafeCast< CEdUndoGroupMarker >( m_cur );
		ASSERT( marker->IsBegin() );

		do 
		{
			CEdUndoGroupMarker* nextMarker = Cast< CEdUndoGroupMarker >( m_next ) ;
			m_next = m_next->GetNextStep();

			if ( nextMarker )
			{
				if ( nextMarker->GetLevel() == marker->GetLevel() )
				{
					ASSERT( !nextMarker->IsBegin() && nextMarker->GetName() == marker->GetName() );
					break;
				}
			}

		}
		while ( m_next );
	}
}


//

CEdUndoManager::ScopedHistoryLock::ScopedHistoryLock( CEdUndoManager& manager )
	: m_manager( manager )
{
	m_manager.m_bLockHistoryChangedEvent = true;
}

CEdUndoManager::ScopedHistoryLock::~ScopedHistoryLock()
{
	m_manager.m_bLockHistoryChangedEvent = false;

	if ( m_manager.m_blockedHistoryChangedEventOccured )
	{
		m_manager.m_blockedHistoryChangedEventOccured = false;
		m_manager.HistoryChanged();
	}
}

//

CEdUndoManager::Transaction::Transaction( CEdUndoManager& manager, const String& name )
	: m_manager( manager )
	, m_name( name )
{
	m_manager.UndoGroupBegin( m_name );
}

CEdUndoManager::Transaction::~Transaction()
{
	m_manager.UndoGroupEnd( m_name );
}

//

CEdUndoManager::CEdUndoManager( wxWindow* messageWindowParent, Bool useDump )
	: m_stepHistoryHead    ( nullptr )
	, m_stepHistoryTail    ( nullptr )
	, m_stepHistoryCurr    ( nullptr )
	, m_stepHistoryCount   ( 0    )
	, m_stepHistoryCountMax( 50   )
	, m_undoInProgress     ( false )
	, m_world              ( nullptr )
	, m_bLockHistoryChangedEvent( false )
	, m_blockedHistoryChangedEventOccured( false )
	, m_messageWindowParent( messageWindowParent )
	, m_useDump            ( useDump )
	, m_undoListener       ( nullptr )
{
	Globals& G = GetGlobals();
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( G.m_mutex );

		if ( !G.m_dbgCallbackRegistered )
		{
			RED_REGISTER_DEBUG_CALLBACK( Undo, CEdUndoManager::DebugCallback );
			G.m_dbgCallbackRegistered = true;
		}
	}

	String hash = ToString( reinterpret_cast< Int32 >( this ) );
	m_dumpFilePath = GFileManager->GetBaseDirectory() + TXT("undo_dump_") + hash + TXT(".temp");
	GFileManager->DeleteFile( m_dumpFilePath );

	SEvents::GetInstance().RegisterListener( RED_NAME( ActiveWorldChanging ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( ScriptCompilationStart ), this ); 
	SEvents::GetInstance().RegisterListener( RED_NAME( Detached ), this );

	LoadOptionsFromConfig();
}

void CEdUndoManager::OnFinalize()
{
	// this should go to the destructor after planned change to Discard semantics. Now it has to be here
	// to prevent sending events to discarded objects
	SEvents::GetInstance().UnregisterListener( this );
}

CEdUndoManager::~CEdUndoManager()
{
	GFileManager->DeleteFile( m_dumpFilePath );

	Globals& G = GetGlobals();
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( G.m_mutex );

		G.m_history.Erase( this );

		if ( G.m_activeUndoMgr == this )
		{
			G.m_activeUndoMgr = nullptr;
		}
	}
}

void CEdUndoManager::SetMenuItems( wxMenuItem* undoItem, wxMenuItem* redoItem )
{
	ASSERT ( undoItem != nullptr && redoItem != nullptr );
	m_undoMenuItem = undoItem;
	m_redoMenuItem = redoItem;
}

void CEdUndoManager::SetUndoListener( IEdUndoListener* listener )
{
	m_undoListener = listener;
}

void CEdUndoManager::NotifyObjectRemoved( CObject *object )
{
	if ( !m_objectsTrackedForRemoval.Exist( object ) ) // notify steps only for object that are not being tracked by the undo system
	{
		ScopedHistoryLock lockHistory( *this );

		for ( auto it = GetStepsIterator( true ); it; ++it )
		{
			it->OnObjectRemoved( object );
		}
	}
	else
	{
		m_objectsTrackedForRemoval.Remove( object );
	}
}

void CEdUndoManager::NotifyObjectTrackedForRemoval( CObject* obj ) 
{ 
	m_objectsTrackedForRemoval.PushBackUnique( obj ); 
}

void CEdUndoManager::UndoGroupBegin( const String& name )
{
	if ( m_groupStack.Empty() || m_groupStack.Back() != name )
	{
		( new CEdUndoGroupMarker( *this, name, m_groupStack.Size(), true ) )->PushStep();
		m_groupStack.PushBack( name );
	}
	else
	{
		ASSERT( false, TXT("Cannot open the same group twice") );
	}
}

void CEdUndoManager::UndoGroupEnd( const String& name )
{
	if ( !m_groupStack.Empty() && m_groupStack.Back() == name )
	{
		m_groupStack.PopBack();
		( new CEdUndoGroupMarker( *this, name, m_groupStack.Size(), false ) )->PushStep();
	}
	else
	{
		ASSERT( false, TXT("Trying to close a different undo group that was opened") );
	}
}

void CEdUndoManager::SetStepToAdd( IUndoStep *stepToAdd ) 
{ 
	ASSERT( m_stepToAdd == nullptr || stepToAdd == nullptr ); 
	m_stepToAdd = stepToAdd;
	UpdateActiveUndoMgr();
}

void CEdUndoManager::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == RED_NAME( ScriptCompilationEnd ) )
	{
		ScopedHistoryLock lockHistory( *this );

		for ( auto it = GetStepsIterator( true ); it; ++it )
		{
			if ( CUndoProperty* up = Cast< CUndoProperty >( it.Get() ) )
			{
				up->RemoveInvalidProperties();
			}
		}
	}
	else 
	{
		if ( m_world )
		{
			if ( name == RED_NAME( Detached ) )
			{
				CObject *obj = GetEventData< CObject* >( data );
				NotifyObjectRemoved( obj );
			}
			else if ( name == RED_NAME( ActiveWorldChanging ) ) 
			{
				if ( m_world == GetEventData< CWorld* >( data ) )
				{
					m_world = nullptr;
					ClearHistory();
				}
			}
		}
	}
}

Bool CEdUndoManager::PrepareUndo( IUndoStep *step, const String& msgTitle, const String& emptyQueueMsg )
{
    if ( ! step )
	{
		if ( m_messageWindowParent )
		{
			SEdPopupNotification::GetInstance().Show( m_messageWindowParent, msgTitle, emptyQueueMsg );
		}

		return false;
	}

	if ( m_useDump && step->m_flushedToDisk )
	{
		ReadStepFromDisk( step );
	}

	return true;
}

void CEdUndoManager::FinalizeUndo( IUndoStep *step, const String& msgTitle )
{
    HistoryChanged();

	if ( m_messageWindowParent )
	{
		SEdPopupNotification::GetInstance().Show( m_messageWindowParent, msgTitle, step->GetName() );
	}

	if ( m_useDump )
	{
		DumpToDisk();
	}

	SEvents::GetInstance().DispatchEvent( CNAME( EditorPostUndoStep ), CreateEventData( this ) );
}

void CEdUndoManager::Undo()
{
	UpdateActiveUndoMgr();

	// Give editors the chance to end the current editing (must be before getting the step)
	SEvents::GetInstance().DispatchEvent( CNAME( EditorPreUndoStep ), CreateEventData( this ) );

	Red::System::ScopedFlag< Bool > inProgress( m_undoInProgress = true, false );

	IUndoStep *step = GetStepToUndo();

	if ( PrepareUndo( step, TXT("UNDO"), TXT("Nothing to be undone") ) )
	{
		if ( CEdUndoGroupMarker* marker = dynamic_cast< CEdUndoGroupMarker* >( step ) )
		{
			ASSERT ( !marker->IsBegin() );

			do 
			{
				step->DoUndo();
				step = step->GetPrevStep();

				if ( CEdUndoGroupMarker* curMarker = Cast< CEdUndoGroupMarker >( step ) )
				{
					if ( curMarker->GetLevel() == marker->GetLevel() )
					{
						ASSERT ( curMarker->IsBegin() && curMarker->GetName() == marker->GetName() );
						break;
					}
				}
			}
			while ( step );

			ASSERT( step, TXT("Open undo group found") );
		}

		if ( step )
		{
			step->DoUndo();
			m_stepHistoryCurr = step->GetPrevStep();
			FinalizeUndo( step, TXT("UNDO") );
		}
	}
}

void CEdUndoManager::Redo()
{
	UpdateActiveUndoMgr();

	// Give editors the chance to end the current editing (must be before getting the step)
	SEvents::GetInstance().DispatchEvent( CNAME( EditorPreUndoStep ), CreateEventData( this ) );

	Red::System::ScopedFlag< Bool > inProgress( m_undoInProgress = true, false );

	IUndoStep *step = GetStepToRedo();

	if ( PrepareUndo( step, TXT("REDO"), TXT("Nothing to be redone") ) )
	{
		if ( CEdUndoGroupMarker* marker = dynamic_cast< CEdUndoGroupMarker* >( step ) )
		{
			ASSERT ( marker->IsBegin() );

			do 
			{
				step->DoRedo();
				step = step->GetNextStep();

				if ( CEdUndoGroupMarker* curMarker = Cast< CEdUndoGroupMarker >( step ) )
				{
					if ( curMarker->GetLevel() == marker->GetLevel() )
					{
						ASSERT ( !curMarker->IsBegin() && curMarker->GetName() == marker->GetName() );
						break;
					}
				}
			}
			while ( step );

			ASSERT( step, TXT("Open undo group found") );
		}

		if ( step )
		{
			step->DoRedo();
			m_stepHistoryCurr = step;
		}

		FinalizeUndo( step, TXT("REDO") );
	}
}

void CEdUndoManager::WriteStepToDisk( IFile& file, IUndoStep* step )
{
	if ( step->m_flushedToDisk ) return;

	step->m_flushedSize = 0;

	// Assume that file is already pointing to where we should start writing.
	size_t stepOffset = file.GetOffset();

	step->OnSerialize( file );
	step->m_flushedOffset = stepOffset;
	step->m_flushedSize = file.GetOffset() - step->m_flushedOffset;
	step->m_flushedToDisk = true;
}

void CEdUndoManager::ReadStepFromDisk( IUndoStep* step )
{
	if ( !step->m_flushedToDisk ) return;

	IFile* file = GFileManager->CreateFileReader( m_dumpFilePath, FOF_AbsolutePath );
	ASSERT( file, TXT("Couldn't open file '%s' to unflush undo step") );

	if ( !file ) return;

	file->Seek( step->m_flushedOffset );

	step->OnSerialize( *file );
	step->m_flushedToDisk = false;
	step->m_flushedSize = 0;

	delete file;
}

void CEdUndoManager::DumpToDisk()
{
	size_t editorAllocatedBytes = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_Editor );

	if ( editorAllocatedBytes > m_dumpToDiskMemoryThreshold )
	{
		IFile* file = GFileManager->CreateFileWriter( m_dumpFilePath, FOF_AbsolutePath | FOF_Append );
		ASSERT( file, TXT("Couldn't open file '%s' to flush undo step") );

		if ( !file ) return;


		// Find a place to start writing. Basically, just find the end of any existing steps.
		size_t stepOffset = 0;
		size_t totalSize = 0;
		for ( IUndoStep* s = m_stepHistoryHead; s; s = s->GetNextStep() )
		{
			if ( s->m_flushedToDisk )
			{
				stepOffset = Max( stepOffset, s->m_flushedOffset + s->m_flushedSize );
				totalSize += s->m_flushedSize;
			}
		}

		// If the file size is much larger than the total dumped size, compact it.
		if ( file->GetSize() > totalSize * 2 )
		{
			// Close our existing file handle, so we can compact it.
			delete file;

			CompactDumpFile();

			// Re-open the dump file.
			file = GFileManager->CreateFileWriter( m_dumpFilePath, FOF_AbsolutePath | FOF_Append );
			ASSERT( file, TXT("Couldn't open file '%s' to flush undo step") );

			if ( !file ) return;

			stepOffset = file->GetSize();
		}


		file->Seek( stepOffset );

		// Start with the furthest "undo", and start dumping to file.
		IUndoStep* step = m_stepHistoryHead;
		while ( step != m_stepHistoryCurr )
		{
			editorAllocatedBytes = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_Editor );

			if ( editorAllocatedBytes < m_dumpToDiskMemoryThreshold )
			{
				break;
			}

			WriteStepToDisk( *file, step );

			step = step->GetNextStep();
		}

		// Same thing, but go through the "redo" steps. Instead of stopping before the current step, we'll
		// include current in the dump. This means the current step will be the last one to be dumped, so
		// it'll be most likely to remain in memory.
		step = m_stepHistoryTail;
		IUndoStep* beforeCurr = m_stepHistoryCurr ? m_stepHistoryCurr->GetPrevStep() : nullptr;
		while ( step != beforeCurr )
		{
			editorAllocatedBytes = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_Editor );

			if ( editorAllocatedBytes < m_dumpToDiskMemoryThreshold )
			{
				break;
			}

			WriteStepToDisk( *file, step );

			step = step->GetPrevStep();
		}


		delete file;
	}
}


void CEdUndoManager::CompactDumpFile()
{
	String hash = ToString( reinterpret_cast< Int32 >( this ) );
	String newDumpFilePath = GFileManager->GetBaseDirectory() + TXT("undo_dump_copy_") + hash + TXT(".temp");

	IFile* oldDumpFile = GFileManager->CreateFileReader( m_dumpFilePath, FOF_AbsolutePath );
	IFile* newDumpFile = GFileManager->CreateFileWriter( newDumpFilePath, FOF_AbsolutePath );

	ASSERT( oldDumpFile && newDumpFile, TXT("Couldn't open files to compact the Undo Dump") );
	if ( !oldDumpFile || !newDumpFile ) return;

	// Use a smallish buffer to copy the data. We're doing this because the editor is using quite a bit of memory,
	// so we probably shouldn't go crazy with allocations here. But, we shouldn't be waiting until the editor is
	// completely out of memory before dumping undo steps, so we should still be okay.
	const size_t BUFFER_SIZE = 4 * 1024 * 1024;
	void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, BUFFER_SIZE );

	// Copy each step's data from the old file into the new file.
	for ( IUndoStep* step = m_stepHistoryHead; step; step = step->GetNextStep() )
	{
		if ( !step->m_flushedToDisk ) continue;

		oldDumpFile->Seek( step->m_flushedOffset );

		// Update the step's offset to match the new dump file.
		step->m_flushedOffset = newDumpFile->GetOffset();

		// Copy this step's data from the old file to the new.
		size_t bytesLeftToCopy = step->m_flushedSize;
		while ( bytesLeftToCopy > 0 )
		{
			size_t bytes = Min( bytesLeftToCopy, BUFFER_SIZE );
			oldDumpFile->Serialize( buffer, bytes );
			newDumpFile->Serialize( buffer, bytes );
			bytesLeftToCopy -= bytes;
		}
	}

	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );

	// Delete the old dump file, rename the new one.
	delete oldDumpFile;
	delete newDumpFile;
	if ( !GFileManager->DeleteFile( m_dumpFilePath ) )
	{
		HALT( "Compacting Undo Dump: Failed to delete old dump file" );
	}
	if ( !GFileManager->MoveFile( newDumpFilePath, m_dumpFilePath ) )
	{
		HALT( "Compacting Undo Dump: Failed to rename new dump file" );
	}
}



void CEdUndoManager::PushStep( IUndoStep *step )
{
	ASSERT ( !m_undoInProgress );

	UpdateActiveUndoMgr();

	ASSERT ( m_stepToAdd == nullptr || m_stepToAdd == step );
	m_stepToAdd = nullptr;

    ASSERT( step );

	if ( IsUndoInProgress() )
	{
		return;
	}

	{
		Red::System::ScopedFlag< Bool > lockHistory( m_bLockHistoryChangedEvent = true, false );
	    ClearHistory( true );
	}

    ++m_stepHistoryCount;
    
    step->m_prevStep = m_stepHistoryCurr;
    step->m_nextStep = GetStepToRedo();

    if ( step->m_prevStep ) step->m_prevStep->m_nextStep = step;
    if ( step->m_nextStep ) step->m_nextStep->m_prevStep = step;

    if ( m_stepHistoryHead == step->m_nextStep ) m_stepHistoryHead = step;
    if ( m_stepHistoryTail == step->m_prevStep ) m_stepHistoryTail = step;
    
    m_stepHistoryCurr = step;
    
    TruncHistory();
    HistoryChanged();

	if ( m_useDump )
	{
		DumpToDisk();
	}
}

void CEdUndoManager::PopStep( IUndoStep *step )
{
	ASSERT ( !m_undoInProgress );
    ASSERT ( step );
	ASSERT ( step->m_undoManager == this );

	IUndoStep* prev = step->m_prevStep;
	IUndoStep* next = step->m_nextStep;

	if ( prev && next && prev->IsA< CEdUndoGroupMarker >() && next->IsA< CEdUndoGroupMarker >() )
	{
		// remove the group, if empty
		if ( prev == m_stepHistoryHead ) m_stepHistoryHead = next->m_nextStep;
		if ( next == m_stepHistoryTail ) m_stepHistoryTail = prev->m_prevStep;
		if ( prev == m_stepHistoryCurr || next == m_stepHistoryCurr ) m_stepHistoryCurr = prev->m_prevStep;

		prev = prev->m_prevStep;
		next = next->m_nextStep;
		m_stepHistoryCount -= 2;
	}

	if ( prev )
	{
		prev->m_nextStep = next;
	}

	if ( next )
	{
		next->m_prevStep = prev;
	}

	--m_stepHistoryCount;

	if ( m_stepHistoryHead == step ) m_stepHistoryHead = next;
	if ( m_stepHistoryTail == step ) m_stepHistoryTail = prev;
	if ( m_stepHistoryCurr == step ) m_stepHistoryCurr = prev;

	HistoryChanged();
}

IUndoStep* CEdUndoManager::GetCurrentStep() 
{ 
	IUndoStep* result = m_stepHistoryCurr;

	if ( CEdUndoGroupMarker* marker = Cast< CEdUndoGroupMarker >( m_stepHistoryCurr ) )
	{
		if ( !marker->IsBegin() )
		{
			// in case the cursor points to the ending group marker, correct it so that it points to the starting one
			do 
			{
				result = result->GetPrevStep();

				if ( CEdUndoGroupMarker* curMarker = Cast< CEdUndoGroupMarker >( result ) )
				{
					if ( curMarker->GetLevel() == marker->GetLevel() )
					{
						break;
					}
				}
			}
			while ( result );
		}
	}

	return result;
}

CEdUndoStepIterator CEdUndoManager::GetStepsIterator( Bool iterateInsideGroups) 
{ 
	return CEdUndoStepIterator( m_stepHistoryHead, iterateInsideGroups ); 
}

void CEdUndoManager::ClearHistory( bool redoOnly /* = false  */ )
{
    if ( !redoOnly )
        m_stepHistoryCurr = nullptr;

    IUndoStep *stepToRemove = GetStepToRedo();
    
    if ( !stepToRemove )
        return;

    while ( stepToRemove )
    {
        stepToRemove->SetParent( nullptr );
        IUndoStep *nextStep = stepToRemove->GetNextStep();
        stepToRemove->Discard();
        stepToRemove = nextStep;
        --m_stepHistoryCount;
    }

    m_stepHistoryTail = m_stepHistoryCurr;

    if ( m_stepHistoryTail )
        m_stepHistoryTail->m_nextStep = nullptr;
    else
        m_stepHistoryHead = nullptr;

    m_groupStack.Clear();

    HistoryChanged();
}

void CEdUndoManager::TruncHistory()
{
    Bool changed = m_stepHistoryCount > m_stepHistoryCountMax;

    // Truncate redo history
    while ( m_stepHistoryTail != m_stepHistoryCurr &&
        m_stepHistoryCount > m_stepHistoryCountMax )
    {
        ASSERT( m_stepHistoryTail );

        m_stepHistoryTail->SetParent( nullptr );
        IUndoStep *nextStep = m_stepHistoryHead->GetPrevStep();
        m_stepHistoryTail->Discard();
        m_stepHistoryTail = nextStep;
        --m_stepHistoryCount;

        if ( m_stepHistoryTail )
            m_stepHistoryTail->m_nextStep = nullptr;
    }

    // Truncate undo history
    while ( m_stepHistoryCurr && 
            m_stepHistoryHead != m_stepHistoryCurr &&
            m_stepHistoryCount > m_stepHistoryCountMax )
    {
        ASSERT( m_stepHistoryHead );

        m_stepHistoryHead->SetParent( nullptr );
        IUndoStep *nextStep = m_stepHistoryHead->GetNextStep();
        m_stepHistoryHead->Discard();
        m_stepHistoryHead = nextStep;
        --m_stepHistoryCount;

        if ( m_stepHistoryHead )
            m_stepHistoryHead->m_prevStep = nullptr;
    }

    if (changed)
        HistoryChanged();
}

void CEdUndoManager::UpdateMenuItem( wxMenuItem* item, IUndoStep *step, const String& text )
{
	String newLabel;
    if ( step )
    {
        String stepName = step->GetName();
        wxMenuUtils::ChangeItemLabelPreservingAccel( item, stepName.Empty() ? text : text + TXT(" ") + stepName );
    }
    else
	{
        wxMenuUtils::ChangeItemLabelPreservingAccel( item, text );
	}
}

void CEdUndoManager::UpdateHistory( )
{
	// pass to the lambda via handle to prevent executing it on the already deleted manager
	THandle< CEdUndoManager > handleToThis( this ); 

	RunLaterOnce( [ handleToThis ]()
	{
		if ( !handleToThis )
		{
			return;
		}

		TUndoHistory history;

		for ( auto step = handleToThis->GetStepsIterator( false ); step; ++step )
		{
			Bool current = ( step.Get() == handleToThis->GetCurrentStep() );
			history.PushBack( SUndoHistoryEntry( step->GetName(), step->GetTarget(), step.GetId(), current ) );
		}

		Globals& G = GetGlobals();
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( G.m_mutex );

			auto it = G.m_history.Find( handleToThis.Get() );

			if ( it != G.m_history.End() )
			{
				it->m_second = Move( history );
			}
			else
			{
				G.m_history.Insert( handleToThis.Get(), Move( history ) );
			}
		}

		if ( handleToThis->m_undoListener )
		{
			handleToThis->m_undoListener->OnUndoHistoryChanged();
		}
	});
}

void CEdUndoManager::HistoryChanged()
{
	if ( m_bLockHistoryChangedEvent ) 
	{
		m_blockedHistoryChangedEventOccured = true;
		return;
	}

	if ( m_undoMenuItem && m_redoMenuItem )
	{
		UpdateMenuItem( m_undoMenuItem, GetStepToUndo(), TXT("Undo") );
		UpdateMenuItem( m_redoMenuItem, GetStepToRedo(), TXT("Redo") );
	}

	UpdateHistory();
}

void CEdUndoManager::SaveOptionsToConfig()
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Undo") );

    config.Write( TXT("Steps"),               static_cast<Int32>( m_stepHistoryCountMax ) );
    config.Write( TXT("DumpMemoryThreshold"), static_cast<Int32>( m_dumpToDiskMemoryThreshold ) );
}

void CEdUndoManager::LoadOptionsFromConfig()
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Undo") );

    m_stepHistoryCountMax       = config.Read( TXT("Steps"), 50 );
    m_dumpToDiskMemoryThreshold = config.Read( TXT("DumpMemoryThreshold"), 300 * 1024 * 1024 );
}

void CEdUndoManager::OnSerialize( IFile& file )
{
    TBaseClass::OnSerialize( file );

    // GC serialization
    if ( file.IsGarbageCollector() )
    {
        IUndoStep *step = m_stepHistoryHead;
        while ( step )
        {
            file << step;
            step = step->GetNextStep();
        }

        if ( m_stepToAdd )
		{
            file << m_stepToAdd;
		}
    }
}

void CEdUndoManager::UpdateActiveUndoMgr()
{
	String desc;

	if ( m_messageWindowParent )
	{
		if ( wxWindow* topLevel = wxGetTopLevelParent( m_messageWindowParent ) )
		{
			desc = TXT("Owned by ") + topLevel->GetLabel();
		}
	}

	Globals& G = GetGlobals();
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( G.m_mutex );
		G.m_activeUndoMgr = this;
		G.m_activeUndoMgrDesc = desc;
	}
}

TUndoHistory CEdUndoManager::GetHistory() const
{
	TUndoHistory result;
	Globals& G = GetGlobals();
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( G.m_mutex );
	G.m_history.Find( this, result );
	return result;
}

Uint32 CEdUndoManager::DebugCallback( Char* buffer, Uint32 bufferSize )
{
	// NOTE: debug undo stack is gathered lazily on a side in a global var, to avoid performing any risky operations here (API calls, memory allocations, etc.)
	Uint32 bufferUsed = 0;

	Globals& G = GetGlobals();

	TUndoHistory* history = nullptr;
	const Char* undoDesc = nullptr;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( G.m_mutex );
		if ( G.m_activeUndoMgr )
		{
			history = G.m_history.FindPtr( G.m_activeUndoMgr );
			undoDesc = G.m_activeUndoMgrDesc.AsChar();
		}
	}

	if( undoDesc )
	{
		RED_APPEND_ERROR_STRING( buffer, bufferSize, bufferUsed, TXT( "[%s]\n\n" ), undoDesc );
	}

	if( history )
	{
		for ( const SUndoHistoryEntry& historyE : *history )
		{
			if ( historyE.m_current )
			{
				RED_APPEND_ERROR_STRING( buffer, bufferSize, bufferUsed, TXT( "[>] %s [%s]\n" ), historyE.m_name.AsChar(), historyE.m_target.AsChar() );
			}
			else
			{
				RED_APPEND_ERROR_STRING( buffer, bufferSize, bufferUsed, TXT( "[  ] %s [%s]\n" ), historyE.m_name.AsChar(), historyE.m_target.AsChar() );
			}
		}
	}

	return bufferUsed;
}
