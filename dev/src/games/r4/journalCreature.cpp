#include "build.h"
#include "journalCreature.h"

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureGameplayHint );

CJournalCreatureGameplayHint::CJournalCreatureGameplayHint()
{

}

CJournalCreatureGameplayHint::~CJournalCreatureGameplayHint()
{

}

Bool CJournalCreatureGameplayHint::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreatureGameplayHintGroup >();
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureHuntingClue );

CJournalCreatureHuntingClue::CJournalCreatureHuntingClue()
{

}

CJournalCreatureHuntingClue::~CJournalCreatureHuntingClue()
{

}

Bool CJournalCreatureHuntingClue::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreatureHuntingClueGroup >();
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureDescriptionEntry );

CJournalCreatureDescriptionEntry::CJournalCreatureDescriptionEntry()
{

}

CJournalCreatureDescriptionEntry::~CJournalCreatureDescriptionEntry()
{

}

Bool CJournalCreatureDescriptionEntry::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreatureDescriptionGroup >();
}

void CJournalCreatureDescriptionEntry::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_description.GetIndex() );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureGameplayHintGroup );

CJournalCreatureGameplayHintGroup::CJournalCreatureGameplayHintGroup()
{

}

CJournalCreatureGameplayHintGroup::~CJournalCreatureGameplayHintGroup()
{

}

Bool CJournalCreatureGameplayHintGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreature >();
}

void CJournalCreatureGameplayHintGroup::DefaultValues()
{
	m_baseName = TXT( "Gameplay Hints" );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureHuntingClueGroup );

CJournalCreatureHuntingClueGroup::CJournalCreatureHuntingClueGroup()
{

}

CJournalCreatureHuntingClueGroup::~CJournalCreatureHuntingClueGroup()
{

}

Bool CJournalCreatureHuntingClueGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreature >();
}

void CJournalCreatureHuntingClueGroup::DefaultValues()
{
	m_baseName = TXT( "Hunting Clues" );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureDescriptionGroup );

CJournalCreatureDescriptionGroup::CJournalCreatureDescriptionGroup()
{

}

CJournalCreatureDescriptionGroup::~CJournalCreatureDescriptionGroup()
{

}

Bool CJournalCreatureDescriptionGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreature >();
}

void CJournalCreatureDescriptionGroup::DefaultValues()
{
	m_baseName = TXT( "Descriptions" );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreature );

CJournalCreature::CJournalCreature()
{

}

CJournalCreature::~CJournalCreature()
{

}

Bool CJournalCreature::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreatureGroup >() || other->IsA< CJournalCreatureVirtualGroup >();
}

void CJournalCreature::funcGetNameStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_name.GetIndex() );
}

void CJournalCreature::funcGetImage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( m_image );
}

void CJournalCreature::funcGetEntityTemplateFilename( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( m_entityTemplate.GetPath() );
}

void CJournalCreature::funcGetItemsUsedAgainstCreature( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< CName > & resultArr = *(TDynArray< CName >*) result;
		resultArr = m_itemsUsedAgainstCreature;
	}
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureGroup )

CJournalCreatureGroup::CJournalCreatureGroup()
{

}

CJournalCreatureGroup::~CJournalCreatureGroup()
{

}

Bool CJournalCreatureGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreatureRoot >();
}

void CJournalCreatureGroup::funcGetNameStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_name.GetIndex() );
}

void CJournalCreatureGroup::funcGetImage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( m_image );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureVirtualGroup )

CJournalCreatureVirtualGroup::CJournalCreatureVirtualGroup()
{

}

CJournalCreatureVirtualGroup::~CJournalCreatureVirtualGroup()
{

}

Bool CJournalCreatureVirtualGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreatureRoot >();
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCreatureRoot );

CJournalCreatureRoot::CJournalCreatureRoot()
{

}

CJournalCreatureRoot::~CJournalCreatureRoot()
{

}

Bool CJournalCreatureRoot::IsParentClass( CJournalBase* ) const
{
	return false;
}
////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CJournalCreatureVitalSpotEntry );
IMPLEMENT_ENGINE_CLASS( CJournalCreatureVitalSpotGroup );

Bool CJournalCreatureVitalSpotEntry::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreatureVitalSpotGroup >();
}

Bool CJournalCreatureVitalSpotGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCreature >();
}

void CJournalCreatureVitalSpotGroup::DefaultValues()
{
	m_baseName = TXT("Vital spots");
}

void CJournalCreatureVitalSpotEntry::funcGetTitleStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_title.GetIndex() );
}

void CJournalCreatureVitalSpotEntry::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_description.GetIndex() );
}

void CJournalCreatureVitalSpotEntry::funcGetCreatureEntry( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	CObject * parent = GetParent();
	if( parent )
	{
		CJournalCreatureVitalSpotGroup* group = static_cast<CJournalCreatureVitalSpotGroup*>( parent );
		CJournalCreature * creature = static_cast<CJournalCreature*>( group->GetParent() );	
		RETURN_HANDLE( CJournalCreature, creature );
	}
	else
	{
		RETURN_HANDLE( CJournalCreature, NULL );
	}	
}
