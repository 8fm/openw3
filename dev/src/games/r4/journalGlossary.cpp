#include "build.h"
#include "journalGlossary.h"


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalGlossaryDescription );

CJournalGlossaryDescription::CJournalGlossaryDescription()
{

}

CJournalGlossaryDescription::~CJournalGlossaryDescription()
{

}

Bool CJournalGlossaryDescription::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalGlossary >();
}

void CJournalGlossaryDescription::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_description.GetIndex() )
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalGlossary );

CJournalGlossary::CJournalGlossary()
{

}

CJournalGlossary::~CJournalGlossary()
{

}

Bool CJournalGlossary::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalGlossaryGroup >();
}

void CJournalGlossary::funcGetTitleStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_title.GetIndex() )
}

void CJournalGlossary::funcGetImagePath( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_STRING( m_image )
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalGlossaryGroup );

CJournalGlossaryGroup::CJournalGlossaryGroup()
{

}

CJournalGlossaryGroup::~CJournalGlossaryGroup()
{

}

Bool CJournalGlossaryGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalGlossaryRoot >();
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalGlossaryRoot );

CJournalGlossaryRoot::CJournalGlossaryRoot()
{

}

CJournalGlossaryRoot::~CJournalGlossaryRoot()
{

}

Bool CJournalGlossaryRoot::IsParentClass( CJournalBase* other ) const
{
	return false;
}
