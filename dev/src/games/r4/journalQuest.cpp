#include "build.h"
#include "journalQuest.h"
#include "journalCreature.h"

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalQuestItemTag );

CJournalQuestItemTag::CJournalQuestItemTag()
{

}

CJournalQuestItemTag::~CJournalQuestItemTag()
{

}

Bool CJournalQuestItemTag::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuestObjective >();
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalQuestEnemyTag );

CJournalQuestEnemyTag::CJournalQuestEnemyTag()
{

}

CJournalQuestEnemyTag::~CJournalQuestEnemyTag()
{

}

Bool CJournalQuestEnemyTag::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuestObjective >();
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_RTTI_ENUM( EJournalMapPinType );
IMPLEMENT_ENGINE_CLASS( CJournalQuestMapPin );

CJournalQuestMapPin::CJournalQuestMapPin()
	: m_type( EJMPT_Default )
	, m_enabledAtStartup(true)
{

}

CJournalQuestMapPin::~CJournalQuestMapPin()
{

}

Bool CJournalQuestMapPin::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuestObjective >();
}

void CJournalQuestMapPin::funcGetMapPinID( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_NAME( m_mapPinID )
}

void CJournalQuestMapPin::funcGetRadius( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_FLOAT( m_radius )
}

#ifdef FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS
String CJournalQuestMapPin::GetFriendlyName() const 
{
	return String::Printf( TXT("'CJournalQuestMapPin, mapPinID: %s'"), m_mapPinID );
}
#endif //FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalQuestObjective );
IMPLEMENT_RTTI_ENUM( eQuestObjectiveType );

CJournalQuestObjective::CJournalQuestObjective()
{

}

CJournalQuestObjective::~CJournalQuestObjective()
{

}

void CJournalQuestObjective::DefaultValues()
{
	m_counterType = Type_Manual;
	m_world = 0;
	m_mutuallyExclusive = false;
}

Bool CJournalQuestObjective::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuestPhase >();
}

Uint32 CJournalQuestObjective::GetWorld() const
{
	if( m_world > 0 )
	{
		// For objectives, the area indices are shifted by one to make way for the "Inherit from Parent" option, hence the "- 1"
		return m_world - 1;
	}
	else //if( m_world == 0 )
	{
		// Return the parent quest area instead
		CObject* phaseOb = GetParent();
		ASSERT( phaseOb != NULL );

		CObject* questOb = phaseOb->GetParent();
		ASSERT( questOb != NULL );
		ASSERT( questOb->IsA< CJournalQuest >() );

		CJournalQuest* quest = static_cast< CJournalQuest* >( questOb );
		return quest->GetWorld();
	}
}

void CJournalQuestObjective::funcGetTitleStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_title.GetIndex() );
}

void CJournalQuestObjective::funcGetWorld( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( GetWorld() );
}

void CJournalQuestObjective::funcGetCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_count );
}

void CJournalQuestObjective::funcGetCounterType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_counterType );
}

void CJournalQuestObjective::funcIsMutuallyExclusive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_mutuallyExclusive );
}

void CJournalQuestObjective::funcGetBookShortcut( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( m_bookShortcut );
}

void CJournalQuestObjective::funcGetItemShortcut( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( m_itemShortcut );
}

void CJournalQuestObjective::funcGetRecipeShortcut( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( m_recipeShortcut );
}

void CJournalQuestObjective::funcGetMonsterShortcut( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	CJournalBase* entry = NULL;

	if ( m_monsterShortcut.IsValid() )
	{
		entry = m_monsterShortcut->GetTarget();
	}

	RETURN_OBJECT( entry );
}

void CJournalQuestObjective::funcGetParentQuest( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetParentQuest() );
}

void CJournalQuestObjective::funcGetTitle( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( GetTitle() );
}

#ifdef FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS
String CJournalQuestObjective::GetFriendlyName() const 
{
	return String::Printf( TXT("'CJournalQuestObjective title: %s, image: %s, area: %ld, counterType: %s, count: %ld'"),
		m_title.GetString().AsChar(), m_image.AsChar(), m_area, CEnum::ToString< eQuestObjectiveType > ( m_counterType ).AsChar(), m_count );
}
#endif //FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalQuestPhase );

CJournalQuestPhase::CJournalQuestPhase()
{

}

CJournalQuestPhase::~CJournalQuestPhase()
{

}

Bool CJournalQuestPhase::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuest >();
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalQuest );
IMPLEMENT_RTTI_ENUM( eQuestType ); 
IMPLEMENT_RTTI_ENUM( EJournalContentType );

CJournalQuest::CJournalQuest()
	: m_contentType( EJCT_Vanilla )
{

}

CJournalQuest::~CJournalQuest()
{

}

void CJournalQuest::DefaultValues()
{
	m_type = QuestType_Chapter;
}

Bool CJournalQuest::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuestGroup >();
}

const CJournalCreature* CJournalQuest::GetHuntingQuestCreature()
{
	if ( !m_huntingQuestPath )
	{
	    return NULL;
	}

	const CJournalBase* target = m_huntingQuestPath->GetTarget();
	if ( target && target->IsA< CJournalCreature >() )
	{
	    return Cast< CJournalCreature >( target );
	}

	return NULL;
}

void CJournalQuest::funcGetTitleStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_title.GetIndex() );
}


void CJournalQuest::funcGetType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_ENUM( m_type );
}

void CJournalQuest::funcGetContentType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_ENUM( m_contentType );
}

void CJournalQuest::funcGetWorld( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( GetWorld() );
}

void CJournalQuest::funcGetHuntingQuestCreatureTag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CName tag;
	const CJournalCreature* creature = GetHuntingQuestCreature();
	if ( creature )
	{
		tag = creature->GetUniqueScriptIdentifier();
	}
	
	RETURN_NAME( tag );
}

#ifdef FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS
String CJournalQuest::GetFriendlyName() const 
{
	return String::Printf( TXT("'CJournalQuest type: %s, area: %ld, huntingQuestPath: %s, questPhase: %s'"), 
		CEnum::ToString< eQuestType > ( m_type ).AsChar(), 
		m_area,
		m_huntingQuestPath.Get() ? m_huntingQuestPath.Get()->GetPathAsString().AsChar() : TXT("null"), 
		m_questPhase.GetPath().AsChar() );
}
#endif //FULL_DESCRIPTIONS_FOR_JOURNAL_BLOCKS

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalQuestGroup )

CJournalQuestGroup::CJournalQuestGroup()
{

}

CJournalQuestGroup::~CJournalQuestGroup()
{

}

Bool CJournalQuestGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuestRoot >();
}

void CJournalQuestGroup::funcGetTitleStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_title.GetIndex() );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalQuestRoot );

CJournalQuestRoot::CJournalQuestRoot()
{

}

CJournalQuestRoot::~CJournalQuestRoot()
{

}

Bool CJournalQuestRoot::IsParentClass( CJournalBase* ) const
{
	return false;
}


IMPLEMENT_ENGINE_CLASS( CJournalQuestDescriptionEntry );

Bool CJournalQuestDescriptionEntry::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuestDescriptionGroup >();
}

void CJournalQuestDescriptionEntry::DefaultValues()
{
	m_baseName = TXT("New Description");
}

void CJournalQuestDescriptionEntry::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_description.GetIndex() );
}

IMPLEMENT_ENGINE_CLASS( CJournalQuestDescriptionGroup );

Bool CJournalQuestDescriptionGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalQuest >();
}

void CJournalQuestDescriptionGroup::DefaultValues()
{
	m_baseName = TXT("Descriptions");
}
