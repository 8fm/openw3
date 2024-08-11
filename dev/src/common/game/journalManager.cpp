#include "build.h"

#include "journalManager.h"
#include "journalBase.h"
#include "journalPath.h"
#include "../core/gameSave.h"

IMPLEMENT_RTTI_ENUM( EJournalStatus );
IMPLEMENT_ENGINE_CLASS( SJournalEntryStatus );
IMPLEMENT_ENGINE_CLASS( CJournalManager );
IMPLEMENT_ENGINE_CLASS( SJournalEvent );
IMPLEMENT_ENGINE_CLASS( SJournalStatusEvent );

// CNames used for serialisation
RED_DEFINE_STATIC_NAME( JActiveEntries );
RED_DEFINE_STATIC_NAME( JActiveEntryMap );
RED_DEFINE_STATIC_NAME( JCategoryGroups );
RED_DEFINE_STATIC_NAME( JEntryUnread );
RED_DEFINE_STATIC_NAME( JChildren );
RED_DEFINE_STATIC_NAME( JUniqueScriptIdentifiers );

// ----------------------------------------------------------------------------------

IJournalEventListener::IJournalEventListener()
{

}

IJournalEventListener::~IJournalEventListener()
{

}

// ----------------------------------------------------------------------------------

CJournalManager::CJournalManager()
{
}

CJournalManager::~CJournalManager()
{
}

void CJournalManager::Initialize()
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CJournalManager::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

void CJournalManager::AddCacheMetadata( const CProperty* prop )
{
	m_propertyCacheMetadata[ prop->GetParent() ].PushBackUnique( prop );
}

void CJournalManager::AddEntryToCache( Uint32 entryIndex )
{
	CJournalBase* entry = m_activeEntries[ entryIndex ].m_entry;

	const CClass* entryClass = entry->GetClass();

	TPropertyArray* propertiesToCache = m_propertyCacheMetadata.FindPtr( entryClass );

	if( propertiesToCache )
	{
		for( Uint32 i = 0; i < propertiesToCache->Size(); ++i )
		{
			const CProperty* prop = (*propertiesToCache)[ i ];

			Uint32 size = prop->GetType()->GetSize();

			TDynArray< Uint8 > valueDump;
			valueDump.Resize( size );
			prop->Get( entry, valueDump.Data() );
			
			Uint32 hash = GetArrayHash( valueDump.TypedData(), size );

			m_propertyCache[ prop ][ hash ].PushBack( entryIndex );
		}
	}
}

const CJournalManager::TEntryGroup* CJournalManager::GetEntryGroupFromCache( const CProperty* prop, const void* value ) const
{
	Uint32 size = prop->GetType()->GetSize();

	const Uint8* valueBytes = reinterpret_cast< const Uint8* >( value );

	Uint32 hash = GetArrayHash( valueBytes, size );

	const THashToEntries* hashToEntries = m_propertyCache.FindPtr( prop );

	if( hashToEntries )
	{
		return hashToEntries->FindPtr( hash );
	}

	return NULL;
}

void CJournalManager::OnGameStart( const CGameInfo& gameInfo )
{
	if ( gameInfo.IsSavedGame() )
	{
		OnLoadGame( gameInfo.m_gameLoadStream );
	}
}

void CJournalManager::OnGameEnd( const CGameInfo& gameInfo )
{
	if ( !gameInfo.IsChangingWorld() )
	{
		m_activeEntries.Clear();
		m_activeEntryMap.Clear();
		m_activeCategoryGroups.Clear();
		m_activeEntryChildren.Clear();
		m_uniqueIdentifierActiveEntryMap.Clear();

		m_propertyCacheMetadata.Clear();
		m_propertyCache.Clear();

		CJournalPath::ClearCache();
	}
}

Bool CJournalManager::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock managerBlock( saver, GetStaticClass()->GetName() );

	// Active Entries
	{
		CGameSaverBlock entriesBlock( saver, CNAME( JActiveEntries ) );

		saver->WriteValue( CNAME( Size ), m_activeEntries.Size() );

		for( Uint32 i = 0; i < m_activeEntries.Size(); ++i )
		{
			CGameSaverBlock entryBlock( saver, GetTypeName< SJournalEntryStatus >() );

			// Since we only have an entry, we'll need to create a new path to this entry
			THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( m_activeEntries[ i ].m_entry );

			#ifndef NO_SAVE_VERBOSITY
				RED_LOG( JournalPath, TXT( "Saving Path %u/%u:" ), i, m_activeEntries.Size() );
				path->DebugPrintTree();
			#endif

			path.Get()->SaveGame( saver );
			saver->WriteValue( CNAME( status ), m_activeEntries[ i ].m_status );
			saver->WriteValue( CNAME( JEntryUnread ), m_activeEntries[ i ].m_unread );
		}
	}

	END_TIMER_BLOCK( time )

	return true;
}

Bool CJournalManager::OnLoadGame( IGameLoader* loader )
{
	CGameSaverBlock managerBlock( loader, GetStaticClass()->GetName() );

	// Active Entries
	{
		CGameSaverBlock entriesBlock( loader, CNAME( JActiveEntries ) );

		Uint32 numberOfActiveItems = loader->ReadValue< Uint32 >( CNAME( Size ) );
		m_activeEntries.Reserve( numberOfActiveItems );

		IRTTIType* type = SRTTI::GetInstance().FindType( ::GetTypeName< THandle< CJournalPath > >() );

		for( Uint32 i = 0; i < numberOfActiveItems; ++i )
		{
			CGameSaverBlock entryBlock( loader, GetTypeName< SJournalEntryStatus >() );

			THandle< CJournalPath > path;
			SJournalEntryStatus status;

			#ifndef NO_SAVE_VERBOSITY
				RED_LOG( JournalPath, TXT( "Loading Path %u/%u:" ), i, m_activeEntries.Size() );
			#endif

			if ( loader->GetSaveVersion() < SAVE_VERSION_OPTIMIZED_JOURNAL_SAVING )
			{
				loader->ReadValue( CNAME( path ), type, &path, this );
			}
			else
			{
				path = CJournalPath::LoadGame( loader );
			}

			loader->ReadValue( CNAME( status ), status.m_status );
			loader->ReadValue( CNAME( JEntryUnread ), status.m_unread );

			if ( !path )
			{
				RED_HALT( "CJournalManager::OnLoadGame - empty journal path!" );
				continue;
			}

			path->DebugPrintTree();

			status.m_entry = path->GetTarget();
			if ( !status.m_entry )
			{
				RED_HALT( "CJournalManager::OnLoadGame - no journal entry!" );
				continue;
			}

			ActivateEntry( status.m_entry, status.m_status, true );

			m_activeEntries.Back().m_unread = status.m_unread;
		}
	}

	return true;
}

void CJournalManager::OnInitGuiDuringGameLoad() 
{
	// TODO: insert gui restore code here
}

void CJournalManager::OnInitGuiDuringWorldChange()
{
	// TODO: insert gui restore code here
}

EJournalStatus CJournalManager::GetEntryStatus( const THandle< CJournalPath > pathToEntry ) const
{
	return GetEntryStatus( pathToEntry->GetTarget() );
}

EJournalStatus CJournalManager::GetEntryStatus( const CJournalBase* entry ) const
{
	Uint32 index;
	if( m_activeEntryMap.Find( entry->GetGUID(), index ) )
	{
		return m_activeEntries[ index ].m_status;
	}

	// Entry has not been activated before now
	return JS_Inactive;
}

Int32 CJournalManager::GetEntryIndex( const CJournalBase* entry ) const
{
	Uint32 index = static_cast< Uint32 >( -1 );

	if ( !entry )
	{
		return index;
	}

	m_activeEntryMap.Find( entry->GetGUID(), index );
	return static_cast< Int32 >( index );
}

Bool CJournalManager::IsEntryUnread( const CGUID& guid )
{
	Uint32 index;
	if( m_activeEntryMap.Find( guid, index ) )
	{
		return m_activeEntries[ index ].m_unread;
	}
	else
	{
		HALT(  "Cannot check entry unread status - Invalid or inactive GUID specified: '%ls'" , ToString( guid ).AsChar() );
	}

	return true;
}

void CJournalManager::SetEntryUnread( const CGUID& guid, Bool unread )
{
	Uint32 index;
	if( m_activeEntryMap.Find( guid, index ) )
	{
		m_activeEntries[ index ].m_unread = unread;
	}
	else
	{
		HALT(  "Cannot check entry unread status - Invalid or inactive GUID specified: '%ls'" , ToString( guid ).AsChar() );
	}
}

void CJournalManager::AddExceptionClass( CClass* targetClass, CClass* exceptionClass )
{
	TDynArray< CClass* >* classes = m_exceptionClasses.FindPtr( targetClass );
	if ( classes )
	{
		classes->PushBack( exceptionClass );
	}
	else
	{
		TDynArray< CClass* > classes;
		classes.PushBack( exceptionClass );
		m_exceptionClasses.Insert( targetClass, classes );
	}
}

Bool CJournalManager::IsExceptionClass( const CJournalBase* target, const CJournalBase* entry )
{
	if ( !target || !entry )
	{
		return false;
	}
	TDynArray< CClass* >* classes = m_exceptionClasses.FindPtr( target->GetClass() );
	if ( classes )
	{
		for ( Uint32 i = 0; i < classes->Size(); ++i )
		{
			if ( entry->IsA( (*classes)[ i ] ) )
			{
				return true;
			}
		}
	}
	return false;
}

void CJournalManager::ActivateEntry( THandle< CJournalPath > pathToEntry, EJournalStatus status, Bool silent )
{
	ASSERT( pathToEntry && pathToEntry->IsValid(), TXT( "Cannot Activate Invalid Journal path" ) );

	if ( !pathToEntry || !pathToEntry->IsValid() )
	{
		return;
	}

	const CJournalBase* target = pathToEntry->GetTarget();
	if ( !target )
	{
		return;
	}

	// exception check
	if ( !CanActivateEntry( target, status ) )
	{
		return;
	}

	for( CJournalPath::iterator iter = pathToEntry->Begin(); iter != pathToEntry->End(); ++iter )
	{
		CJournalBase* entry = (*iter);

		// there might be some exceptions class (i.e. we don't want to activate CJournalQuest when we activate CJournalQuestDescriptionEntry)
		if ( IsExceptionClass( target, entry ) )
		{
			continue;
		}
		// This check makes sure that we don't set all the parent items to complete
		// when we simply want to set a child item as completed
		if ( status == JS_Active || iter.IsLastItem() )
		{
			ActivateEntry( entry, status, silent );
		}
	}
}

void CJournalManager::ActivateEntry( CJournalBase* entry, EJournalStatus status, Bool silent )
{
	// HACK: This is a temporary measure to keep journal entries in memory at all times
	// Later this will be replaced by packing all journal entries into a single file when cooked
	// Which will be loaded into memory at all times
	CObject* parent = entry->GetParent();
	CJournalResource* resource = Cast< CJournalResource >( parent );
	if( resource )
	{
		if( !resource->IsInRootSet() )
		{
			resource->AddToRootSet();
		}
	}
	else
	{
		ASSERT( parent->IsA< CJournalBase >(), TXT( "Journal entry '%ls' not parented to Journal Resource class or journal entry" ), entry->GetName().AsChar() );
	}

	// Add new entries to
	const CGUID& entryGuid = entry->GetGUID();

	Uint32* entryStatusIndex = m_activeEntryMap.FindPtr( entryGuid );

	if( entryStatusIndex )
	{
		if( m_activeEntries[ *entryStatusIndex ].m_status != status )
		{
			// If this entry is being marked as complete, make sure all it's children are marked as failed if they're still active
			if( status != JS_Active )
			{
				const TEntryGroup* group = m_activeEntryChildren.FindPtr( m_activeEntries[ *entryStatusIndex ].m_entry->GetGUID() );

				if( group )
				{
					for( Uint32 i = 0; i < group->Size(); ++i )
					{
						Uint32 index = (*group)[ i ];

						// Mark all children as "succedeed", if they haven't been explicitly marked as completed
						// Do this recursively to make sure we get the entire tree
						if( m_activeEntries[ index ].m_status == JS_Active )
						{
							ActivateEntry( m_activeEntries[ index ].m_entry, status, silent );
						}
					}
				}
			}

			// Create status change notification
			SJournalStatusEvent event;
			event.m_entry = m_activeEntries[ *entryStatusIndex ].m_entry;
			event.m_oldStatus = m_activeEntries[ *entryStatusIndex ].m_status;
			event.m_newStatus = status;
			event.m_silent = silent;

			// Change the status
			m_activeEntries[ *entryStatusIndex ].m_status = status;

			// call callback for status change
			EntryStatusChanged( entry, event.m_oldStatus, status, silent );

			// Send the event
			SendEvent( event );

			JOURNAL_LOG
			(
				TXT( "Status Change for entry '%ls': '%ls' -> '%ls'" ),
				event.m_entry->GetName().AsChar(),
				SRTTI::GetInstance().FindEnum( CNAME( EJournalStatus ) )->ToString( event.m_oldStatus ).AsChar(),
				SRTTI::GetInstance().FindEnum( CNAME( EJournalStatus ) )->ToString( event.m_newStatus ).AsChar()
			);
		}
	}
	else
	{
		SJournalEntryStatus newStatus;

		newStatus.m_entry = entry;
		newStatus.m_status = status;
		newStatus.m_unread = true;

		Uint32 index = m_activeEntries.Size();

		m_activeEntries.PushBack( newStatus );
		m_activeEntryMap.Insert( entryGuid, index );
		AddEntryToCache( index );

		m_activeCategoryGroups[ entry->GetClass() ].PushBack( index );

		if( entry->GetUniqueScriptIdentifier() != CName::NONE )
		{
			m_uniqueIdentifierActiveEntryMap[ entry->GetUniqueScriptIdentifier() ] = index;
		}

		// Base type activation
		if( entry->IsA< CJournalChildBase >() )
		{
			CJournalChildBase* child = static_cast< CJournalChildBase* >( entry );

			m_activeEntryChildren[ child->GetLinkedParentGUID() ].PushBack( index );
		}

		TypeSpecificActivation( entry, index );

		// Create status change notification
		SJournalStatusEvent event;
		event.m_entry = entry;
		event.m_oldStatus = JS_Inactive;
		event.m_newStatus = status;
		event.m_silent = silent;

		// call callback for status change
		EntryStatusChanged( entry, JS_Inactive, status, silent );

		// Send the event
		SendEvent( event );

		JOURNAL_LOG
		(
			TXT( "Status Change for entry '%ls': '%ls' -> '%ls'" ),
			event.m_entry->GetName().AsChar(),
			SRTTI::GetInstance().FindEnum( CNAME( EJournalStatus ) )->ToString( event.m_oldStatus ).AsChar(),
			SRTTI::GetInstance().FindEnum( CNAME( EJournalStatus ) )->ToString( event.m_newStatus ).AsChar()
		);
	}
}

void CJournalManager::Export( CXMLWriter& writer )
{

}

void CJournalManager::funcActivateEntry( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	GET_PARAMETER_OPT( EJournalStatus, status, JS_Active );
	GET_PARAMETER_OPT( Bool, showInfoOnScreen, true );
	GET_PARAMETER_OPT( Bool, activateParents, true );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();

	if( entry )
	{
		if ( activateParents )
		{
			THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( entry );
			ActivateEntry( path, status, !showInfoOnScreen );
		}
		else
		{
			ActivateEntry( entry, status, !showInfoOnScreen );
		}
	}
}

void CJournalManager::funcGetEntryStatus( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();

	EJournalStatus status = JS_Inactive;
	
	if( entry )
	{
		status = GetEntryStatus( entry );
	}

	RETURN_ENUM( status );
}

void CJournalManager::funcGetEntryIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();

	Int32 index = -1;

	if( entry )
	{
		index = GetEntryIndex( entry );
	}

	RETURN_INT( index );
}

void CJournalManager::funcIsEntryUnread( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();

	Bool isRead = false;

	if( entry )
	{
		isRead = IsEntryUnread( entry->GetGUID() );
	}

	RETURN_BOOL( isRead );
}

void CJournalManager::funcSetEntryUnread( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	GET_PARAMETER( Bool, unread, false );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();

	if( entry )
	{
		SetEntryUnread( entry->GetGUID(), unread );
	}
}

void CJournalManager::funcGetNumberOfActivatedOfType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, journalEntryType, CName::NONE );
	FINISH_PARAMETERS;

	Int32 count = 0;

	CClass* classDef = SRTTI::GetInstance().FindClass( journalEntryType );
	
	if( classDef )
	{
		TEntryGroup* group = m_activeCategoryGroups.FindPtr( classDef );

		if( group )
		{
			count = group->SizeInt();
		}
	}

	RETURN_INT( count );
}

void CJournalManager::funcGetActivatedOfType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, journalEntryType, CName::NONE );
	GET_PARAMETER_REF( TDynArray< THandle< CJournalBase > >, out, TDynArray< THandle< CJournalBase > >() );
	FINISH_PARAMETERS;

	CClass* classDef = SRTTI::GetInstance().FindClass( journalEntryType );

	if( classDef )
	{
		TEntryGroup* group = m_activeCategoryGroups.FindPtr( classDef );

		if( group )
		{
			FillHandleArrayWithEntryGroup( out, group );
		}
	}
}

void CJournalManager::funcGetNumberOfActivatedChildren( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, parentHandle, NULL );
	FINISH_PARAMETERS;

	Int32 count = 0;

	CJournalBase* parent = parentHandle.Get();

	if( parent )
	{
		const TEntryGroup* group = m_activeEntryChildren.FindPtr( parent->GetGUID() );

		if( group )
		{
			count = group->SizeInt();
		}
	}

	RETURN_INT( count );
}

void CJournalManager::funcGetActivatedChildren( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, parentHandle, NULL );
	GET_PARAMETER_REF( TDynArray< THandle< CJournalBase > >, out, TDynArray< THandle< CJournalBase > >() );
	FINISH_PARAMETERS;

	CJournalBase* parent = parentHandle.Get();

	if( parent )
	{
		const TEntryGroup* group = m_activeEntryChildren.FindPtr( parent->GetGUID() );

		if( group )
		{
			FillHandleArrayWithEntryGroup( out, group );
		}
	}
}

void CJournalManager::funcGetNumberOfAllChildren( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, parentHandle, NULL );
	FINISH_PARAMETERS;

	Int32 count = 0;

	const CJournalContainer* container = Cast< CJournalContainer >( parentHandle.Get() );
	if ( container )
	{
		count = container->GetNumChildren();
	}
	RETURN_INT( count );
}

void CJournalManager::funcGetAllChildren( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, parentHandle, NULL );
	GET_PARAMETER_REF( TDynArray< THandle< CJournalBase > >, out, TDynArray< THandle< CJournalBase > >() );
	FINISH_PARAMETERS;

	out.ClearFast();

	const CJournalContainer* container = Cast< CJournalContainer >( parentHandle.Get() );
	if ( container )
	{
		for ( Uint32 i = 0; i < container->GetNumChildren(); ++i )
		{
			out.PushBack( container->GetChild( i ) );
		}
	}
}

void CJournalManager::funcGetEntryByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	Uint32* index = m_uniqueIdentifierActiveEntryMap.FindPtr( tag );

	CJournalBase* entry = NULL;

	if( index )
	{
		entry = m_activeEntries[ *index ].m_entry;
	}

	RETURN_OBJECT( entry );
}

void CJournalManager::funcGetEntryByString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, str, String::EMPTY );
	FINISH_PARAMETERS;

	Uint32* index = m_uniqueIdentifierActiveEntryMap.FindPtr( CName( str ) );

	CJournalBase* entry = NULL;

	if( index )
	{
		entry = m_activeEntries[ *index ].m_entry;
	}

	RETURN_OBJECT( entry );
}

void CJournalManager::funcGetEntryByGuid( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CGUID, guid, CGUID::ZERO );
	FINISH_PARAMETERS;

	Uint32* index = m_activeEntryMap.FindPtr( guid );

	CJournalBase* entry = NULL;

	if ( index )
	{
		entry = m_activeEntries[ *index ].m_entry;
	}

	RETURN_OBJECT( entry );
}

void CJournalManager::RegisterEventListener( IJournalEventListener* listener )
{
	m_listeners.PushBackUnique( listener );
}

void CJournalManager::UnregisterEventListener( IJournalEventListener* listener )
{
	m_listeners.Remove( listener );
}

void CJournalManager::SendEvent( const SJournalEvent& event )
{
	for( Uint32 i = 0; i < m_listeners.Size(); ++i )
	{
		m_listeners[ i ]->OnJournalBaseEvent( event );
	}
}
