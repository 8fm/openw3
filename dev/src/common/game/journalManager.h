#pragma once

#include "gameSystem.h"
// ----------------------------------------------------------------------------------

class CJournalBase;
class CJournalPath;

enum EJournalStatus
{
	JS_Inactive = 0,
	JS_Active,
	JS_Success,
	JS_Failed,
};

BEGIN_ENUM_RTTI( EJournalStatus )
	ENUM_OPTION( JS_Inactive )
	ENUM_OPTION( JS_Active )
	ENUM_OPTION( JS_Success )
	ENUM_OPTION( JS_Failed )
END_ENUM_RTTI();

// ----------------------------------------------------------------------------------

struct SJournalEntryStatus
{
	DECLARE_RTTI_STRUCT( SJournalEntryStatus )

	CJournalBase* m_entry;
	EJournalStatus m_status;
	Bool m_unread;
};

BEGIN_CLASS_RTTI( SJournalEntryStatus )
	PROPERTY( m_entry )
	PROPERTY( m_status )
	PROPERTY( m_unread )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

#define INVALID_ENTRY_INDEX ( 0xffffffffu )

// ----------------------------------------------------------------------------------

struct SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SJournalEvent );

public:
	const CJournalBase* m_entry;
};

BEGIN_ABSTRACT_CLASS_RTTI( SJournalEvent )
END_CLASS_RTTI();

struct SJournalStatusEvent : public SJournalEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( SJournalStatusEvent );

public:
	EJournalStatus m_oldStatus;
	EJournalStatus m_newStatus;
	Bool           m_silent;
};

BEGIN_CLASS_RTTI( SJournalStatusEvent )
	PARENT_CLASS( SJournalEvent );
	PROPERTY( m_oldStatus )
	PROPERTY( m_newStatus )
	PROPERTY( m_silent )
END_CLASS_RTTI();

class IJournalEventListener
{
public:
	IJournalEventListener();
	virtual ~IJournalEventListener();
	
	virtual void OnJournalBaseEvent( const SJournalEvent& event ) = 0;
};

// ----------------------------------------------------------------------------------

class CJournalManager : public IGameSystem, public IGameSaveSection
{
	DECLARE_ENGINE_CLASS( CJournalManager, IGameSystem, 0 )

	// --- Types ---
protected:
	// Array of all previously entered journal entries
	typedef TDynArray< SJournalEntryStatus > TEntryStatusList;

	// CGUID -> Journal entries
	typedef THashMap< CGUID, Uint32 > TJournalEntryMap;

	// array of indices into [TEntryStatusList m_activeEntries]
	typedef TDynArray< Uint32 > TEntryGroup;

	// Group indices into [m_activeEntries] by class type
	typedef THashMap< const CClass*, TEntryGroup > TEntryGroupMap;

	// Group indices into [m_activeEntries] by parent
	typedef THashMap< CGUID, TEntryGroup > TEntryChildMap;

	// Maps unique identifiers to entry indices
	typedef THashMap< CName, Uint32 > TScriptIdentifierEntryMap;

	typedef TDynArray< const CProperty* > TPropertyArray;

	typedef THashMap< Uint32, TEntryGroup > THashToEntries;

	// --- Event System ---
private:
	TDynArray< IJournalEventListener* >		m_listeners;

	// --- Member Variables (Temporarily exposed to derived classes) ---
protected:

	// Generic Data
	TEntryStatusList							m_activeEntries;
	TJournalEntryMap							m_activeEntryMap;
	TEntryGroupMap								m_activeCategoryGroups;
	TEntryChildMap								m_activeEntryChildren;
	TScriptIdentifierEntryMap					m_uniqueIdentifierActiveEntryMap;

private:	
	THashMap< const CClass*, TPropertyArray >		m_propertyCacheMetadata;
	THashMap< const CProperty*, THashToEntries >	m_propertyCache;
	THashMap< CClass*, TDynArray< CClass* > >		m_exceptionClasses;

	// --- Constructor ---
public:
	CJournalManager();
	virtual ~CJournalManager();

	// --- Interface ---
protected:
	// Implement this method if you need to do something special based on the type of journal entry, when it's activated
	virtual void TypeSpecificActivation( CJournalBase* entry, Uint32 index ) {};

	// -- Event system --
public:
	void RegisterEventListener( IJournalEventListener* listener );
	void UnregisterEventListener( IJournalEventListener* listener );

protected:
	void SendEvent( const SJournalEvent& event );

	// --- Inherited interface methods (Make sure to call these first if overridden) ---
public:
	//! Export to xml
	void Export( CXMLWriter& writer );

	// IGameSystem
	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void OnGameStart( const CGameInfo& gameInfo );
	virtual void OnGameEnd( const CGameInfo& gameInfo );

	virtual bool OnSaveGame( IGameSaver* saver );
	Bool OnLoadGame( IGameLoader* loader );
	virtual void OnInitGuiDuringGameLoad();
	virtual void OnInitGuiDuringWorldChange();
	virtual Uint32 GetDisplayNameIndexForSavedGame() { return 0; }
	virtual Bool CanActivateEntry( const CJournalBase* target, EJournalStatus status ) { return true; }

	// --- Standard Journal Entry Manipulation ---
public:
	void ActivateEntry( THandle< CJournalPath > pathToEntry, EJournalStatus status, Bool silent );
	EJournalStatus GetEntryStatus( const THandle< CJournalPath > pathToEntry ) const;
	EJournalStatus GetEntryStatus( const CJournalBase* entry ) const;
	Int32 GetEntryIndex( const CJournalBase* entry ) const;

	Bool IsEntryUnread( const CGUID& guid );
	void SetEntryUnread( const CGUID& guid, Bool unread = false );

	void AddExceptionClass( CClass* targetClass, CClass* exceptionClass );
	Bool IsExceptionClass( const CJournalBase* target, const CJournalBase* base );

	// --- Journal Entry Callbacks --
public:
	virtual void EntryStatusChanged( CJournalBase* questEntry, EJournalStatus oldStatus, EJournalStatus newStatus, Bool silent ) {}

protected:
	void ActivateEntry( CJournalBase* questEntry, EJournalStatus status, Bool silent );

	// --- Metadata Interface ---
protected:
	// Specify a property of a journal entry type, to have those entries mapped by the value of that property
	void AddCacheMetadata( const CProperty* prop );

	// Retrieve all journal entries with the specified property and value
	const TEntryGroup* GetEntryGroupFromCache( const CProperty* prop, const void* value ) const;

	// --- Templated Accessor Functions ---
protected:
	// Returns number added to array
	template< typename T >
	int GetAllActivatedOfType( TDynArray< const T* >& entries ) const
	{
		const TEntryGroup* group = m_activeCategoryGroups.FindPtr( ClassID< T >() );

		return FillArrayWithEntryGroup( entries, group );
	}

	template< typename T >
	int GetAllActivatedChildren( const CGUID& guid, TDynArray< const T* >& children ) const
	{
		const TEntryGroup* group = m_activeEntryChildren.FindPtr( guid );

		return FillArrayWithEntryGroup( children, group );
	}

	// TODO: Figure out a way to combine the following two functions:
	template< typename T >
	int FillHandleArrayWithEntryGroup( TDynArray< THandle< T > >& dest, const TEntryGroup* source ) const
	{
		if( source )
		{
			for( Uint32 i = 0; i < source->Size(); ++i )
			{
				Uint32 index = (*source)[ i ];

				CJournalBase* base = m_activeEntries[ index ].m_entry;

				T* entry = Cast< T >( base );

				if( entry )
				{
					THandle< T > handle( entry );
					dest.PushBack( handle );
				}
			}

			//This assumes the array was empty to begin with
			return dest.Size();
		}

		return 0;
	}

	template< typename T >
	int FillArrayWithEntryGroup( TDynArray< const T* >& dest, const TEntryGroup* source ) const
	{
		if( source )
		{
			for( Uint32 i = 0; i < source->Size(); ++i )
			{
				Uint32 index = (*source)[ i ];

				CJournalBase* base = m_activeEntries[ index ].m_entry;

				T* entry = Cast< T >( base );

				if( entry )
				{
					dest.PushBack( entry );
				}
			}

			//This assumes the array was empty to begin with
			return dest.Size();
		}

		return 0;
	}

	// --- Metadata internal ---
private:
	void AddEntryToCache( Uint32 entryIndex );

	// --- Script Functions ---
private:
	void funcActivateEntry( CScriptStackFrame& stack, void* result );
	void funcGetEntryStatus( CScriptStackFrame& stack, void* result );
	void funcGetEntryIndex( CScriptStackFrame& stack, void* result );
	void funcIsEntryUnread( CScriptStackFrame& stack, void* result );
	void funcSetEntryUnread( CScriptStackFrame& stack, void* result );
	void funcGetEntryByTag( CScriptStackFrame& stack, void* result );
	void funcGetEntryByString( CScriptStackFrame& stack, void* result );
	void funcGetEntryByGuid( CScriptStackFrame& stack, void* result );

	void funcGetNumberOfActivatedOfType( CScriptStackFrame& stack, void* result );
	void funcGetActivatedOfType( CScriptStackFrame& stack, void* result );
	void funcGetNumberOfActivatedChildren( CScriptStackFrame& stack, void* result );
	void funcGetActivatedChildren( CScriptStackFrame& stack, void* result );

	void funcGetNumberOfAllChildren( CScriptStackFrame& stack, void* result );
	void funcGetAllChildren( CScriptStackFrame& stack, void* result );

	ASSIGN_GAME_SYSTEM_ID( GS_Journal );
};

BEGIN_CLASS_RTTI( CJournalManager )
	PARENT_CLASS( IGameSystem )

	NATIVE_FUNCTION( "ActivateEntry",		funcActivateEntry )
	NATIVE_FUNCTION( "GetEntryStatus",		funcGetEntryStatus )
	NATIVE_FUNCTION( "GetEntryIndex",		funcGetEntryIndex )

	NATIVE_FUNCTION( "IsEntryUnread",		funcIsEntryUnread )
	NATIVE_FUNCTION( "SetEntryUnread",		funcSetEntryUnread )

	NATIVE_FUNCTION( "GetEntryByTag",		funcGetEntryByTag )
	NATIVE_FUNCTION( "GetEntryByString",	funcGetEntryByString )
	NATIVE_FUNCTION( "GetEntryByGuid",		funcGetEntryByGuid )

	NATIVE_FUNCTION( "GetNumberOfActivatedOfType",		funcGetNumberOfActivatedOfType )
	NATIVE_FUNCTION( "GetActivatedOfType",				funcGetActivatedOfType )
	NATIVE_FUNCTION( "GetNumberOfActivatedChildren",	funcGetNumberOfActivatedChildren )
	NATIVE_FUNCTION( "GetActivatedChildren",			funcGetActivatedChildren )

	NATIVE_FUNCTION( "GetNumberOfAllChildren",	funcGetNumberOfAllChildren )
	NATIVE_FUNCTION( "GetAllChildren",			funcGetAllChildren )
END_CLASS_RTTI();
