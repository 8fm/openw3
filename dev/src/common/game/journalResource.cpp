#include "build.h"
#include "journalResource.h"

#include "journalBase.h"
#include "journalPath.h"

// This is a temporary solution to keep journal resources from being garbage
// collected until we can come up with a more permanent cookable solution
class CJournalRoot : public CObject
{
	DECLARE_ENGINE_CLASS( CJournalRoot, CObject, 0 )

private:
	CJournalRoot()
	{
		AddToRootSet();
	}

	~CJournalRoot()
	{
		RemoveFromRootSet();
		m_objects.ClearFast();
	}

	typedef TDynArray< THandle<CJournalResource > >	TJournalResources;
	TJournalResources		m_objects;

public:
	static CJournalRoot* GetInstance()
	{
		if( !m_instance )
		{
			m_instance = CreateObject< CJournalRoot >();
		}

		return m_instance;
	}
	
	void AddToObjArray(CJournalResource * obj)
	{
		m_objects.PushBackUnique(obj);
	}

private:
	static CJournalRoot* m_instance;
};

BEGIN_CLASS_RTTI( CJournalRoot )
	PARENT_CLASS( CObject )
	PROPERTY(m_objects)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CJournalRoot )
CJournalRoot* CJournalRoot::m_instance = NULL;

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalResource );

CJournalResource::CJournalResource()
{
	SetParent( CJournalRoot::GetInstance() );
	CJournalRoot::GetInstance()->AddToObjArray(this);
}

CJournalResource::~CJournalResource()
{

}

void CJournalResource::OnPostLoad()
{
	SetParent( CJournalRoot::GetInstance() );
}

void CJournalResource::funcGetEntry( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( m_entry );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CJournalInitialEntriesResource );

CJournalInitialEntriesResource::CJournalInitialEntriesResource()
	: m_regularQuestCount( 0 )
	, m_monsterHuntQuestCount( 0 )
	, m_treasureHuntQuestCount( 0 )
{
}

CJournalInitialEntriesResource::~CJournalInitialEntriesResource()
{
}

void CJournalInitialEntriesResource::OnPostLoad()
{
}

#ifndef NO_EDITOR

void CJournalInitialEntriesResource::AddEntry( THandle< CJournalPath > entry )
{
	if ( MarkModified() )
	{
		m_entries.PushBack( entry );
	}
}

void CJournalInitialEntriesResource::RemoveEntry( THandle< CJournalPath > entry )
{
	CJournalBase* base = entry->GetTarget();
	ASSERT( base );
	if ( !base )
	{
	    return;
	}

	if ( MarkModified() )
	{
		for ( Uint32 i = 0; i < m_entries.Size(); i++ )
		{
			CJournalBase* entryBase = m_entries[ i ]->GetTarget();
			ASSERT( entryBase );
			if ( base == entryBase )
			{
				m_entries.Erase( m_entries.Begin() + i );
				return;
			}
		}
	}
}

Bool CJournalInitialEntriesResource::ExistsEntry( THandle< CJournalPath > entry )
{
	CJournalBase* base = entry->GetTarget();
	ASSERT( base );
	if ( !base )
	{
	    return false;
	}
	for ( Uint32 i = 0; i < m_entries.Size(); i++ )
	{
		CJournalBase* entryBase = m_entries[ i ]->GetTarget();
		ASSERT( entryBase );
		if ( base == entryBase )
		{
		    return true;
		}
	}
    return false;
}

void CJournalInitialEntriesResource::SetQuestCount( Uint32 regularCount, Uint32 monsterCount, Uint32 treasureCount )
{
	if ( m_regularQuestCount      != regularCount ||
		 m_monsterHuntQuestCount  != monsterCount ||
		 m_treasureHuntQuestCount != treasureCount )
	{
		if ( MarkModified() )
		{
			m_regularQuestCount      = regularCount;
			m_monsterHuntQuestCount  = monsterCount;
			m_treasureHuntQuestCount = treasureCount;
		}
	}
}

#endif //NO_EDITOR