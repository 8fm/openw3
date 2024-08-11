#include "build.h"
#include "journalCharacter.h"
#include "r4JournalManager.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_RTTI_ENUM( EJournalVisibilityAction );

IMPLEMENT_ENGINE_CLASS( CJournalCharacterDescription );

CJournalCharacterDescription::CJournalCharacterDescription()
	: m_action( JVA_Nothing )
{

}

CJournalCharacterDescription::~CJournalCharacterDescription()
{

}

Bool CJournalCharacterDescription::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCharacter >();
}

void CJournalCharacterDescription::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_description.GetIndex() );
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_RTTI_ENUM( ECharacterImportance );
IMPLEMENT_ENGINE_CLASS( CJournalCharacter );

CJournalCharacter::CJournalCharacter()
{

}

CJournalCharacter::~CJournalCharacter()
{

}

Bool CJournalCharacter::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCharacterGroup >();
}

void CJournalCharacter::funcGetNameStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_name.GetIndex() );
}

void CJournalCharacter::funcGetImagePath( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( m_image )
}

void CJournalCharacter::funcGetCharacterImportance( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_ENUM( m_importance) ;
}

void CJournalCharacter::funcGetEntityTemplateFilename( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();
	if ( !manager )
	{
		RETURN_STRING( String::EMPTY );
		return;
	}

	for ( Int32 i = GetNumChildren() - 1; i >= 0; --i )
	{
		CJournalCharacterDescription* description = Cast< CJournalCharacterDescription >( GetChild( i ) );
		if ( description )
		{
			if ( manager->GetEntryStatus( description ) == JS_Active )
			{
				EJournalVisibilityAction action = description->GetAction();
				if ( action == JVA_Hide )
				{
					RETURN_STRING( String::EMPTY );
					return;
				}
				else if ( action == JVA_Show )
				{
					RETURN_STRING( m_entityTemplate.GetPath());
					return;
				}
			}
		}
	}

	// if we got here, show entity anyway
	RETURN_STRING( m_entityTemplate.GetPath() );
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCharacterGroup );

CJournalCharacterGroup::CJournalCharacterGroup()
{

}

CJournalCharacterGroup::~CJournalCharacterGroup()
{

}

Bool CJournalCharacterGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalCharacterRoot >();
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalCharacterRoot );

CJournalCharacterRoot::CJournalCharacterRoot()
{

}

CJournalCharacterRoot::~CJournalCharacterRoot()
{

}

Bool CJournalCharacterRoot::IsParentClass( CJournalBase* other ) const
{
	return false;
}
