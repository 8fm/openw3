
#pragma once

class CJournalBase;

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CJournalResource, CResource, "journal", "Journal Entry" );

public:
	CJournalResource();
	~CJournalResource();

	//CJournalQuestBase* operator->() { return m_quest; }
	//const CJournalQuestBase* operator->() const { return m_quest; }

	CJournalBase* Get() { return m_entry; }
	const CJournalBase* Get() const { return m_entry; }

	void Set( CJournalBase* entry ) { m_entry = entry; }

	virtual void OnPostLoad();

private:
	void funcGetEntry( CScriptStackFrame& stack, void* result );

private:
	CJournalBase* m_entry;
};

BEGIN_CLASS_RTTI( CJournalResource )
	PARENT_CLASS( CResource )
	PROPERTY_INLINED( m_entry, TXT( "Entry" ) )
	NATIVE_FUNCTION( "GetEntry", funcGetEntry )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CJournalInitialEntriesResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CJournalInitialEntriesResource, CResource, "w2je", "Journal Entries" );

public:
	CJournalInitialEntriesResource();
	~CJournalInitialEntriesResource();

	virtual void OnPostLoad();

#ifndef NO_EDITOR
	void AddEntry( THandle< CJournalPath > entry );
	void RemoveEntry( THandle< CJournalPath > entry );
	Bool ExistsEntry( THandle< CJournalPath > entry );
	void SetQuestCount( Uint32 regularCount, Uint32 monsterCount, Uint32 treasureCount );
#endif //NO_EDITOR

	RED_INLINE const TDynArray< THandle< CJournalPath > >& GetEntries() const	{ return m_entries; } 
	RED_INLINE TDynArray< THandle< CJournalPath > >& GetEntries()				{ return m_entries; } 
	RED_INLINE Uint32 GetRegularQuestCount() const								{ return m_regularQuestCount; }
	RED_INLINE Uint32 GetMonsterHuntQuestCount() const							{ return m_monsterHuntQuestCount; }
	RED_INLINE Uint32 GetTreasureHuntQuestCount() const							{ return m_treasureHuntQuestCount; }

private:
    TDynArray< THandle< CJournalPath > >	m_entries;
	Uint32						m_regularQuestCount;
	Uint32						m_monsterHuntQuestCount;
	Uint32						m_treasureHuntQuestCount;
};

BEGIN_CLASS_RTTI( CJournalInitialEntriesResource )
	PARENT_CLASS( CResource )
	PROPERTY_RO( m_entries, TXT("Active entries") )
	PROPERTY_RO( m_regularQuestCount, TXT("Regular quest count") )
	PROPERTY_RO( m_monsterHuntQuestCount, TXT("Monster hunt quest count") )
	PROPERTY_RO( m_treasureHuntQuestCount, TXT("Treasure hunt quest count") )
END_CLASS_RTTI()