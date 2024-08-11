#include "build.h"
#include "journalTutorial.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalTutorial );

RED_DEFINE_NAME( video )

CJournalTutorial::CJournalTutorial()
{

}

CJournalTutorial::~CJournalTutorial()
{

}

Bool CJournalTutorial::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalTutorialGroup >();
}

void CJournalTutorial::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_description.GetIndex() )
}

void CJournalTutorial::funcGetNameStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_name.GetIndex() )
}

void CJournalTutorial::funcGetImagePath( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_STRING( m_image )
}

void CJournalTutorial::funcGetVideoPath( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_STRING( m_video )
}

void CJournalTutorial::funcGetDLCLock( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_NAME( m_dlcLock )
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalTutorialGroup );

CJournalTutorialGroup::CJournalTutorialGroup()
{

}

CJournalTutorialGroup::~CJournalTutorialGroup()
{

}

Bool CJournalTutorialGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalTutorialRoot >();
}

void CJournalTutorialGroup::funcGetNameStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS
	RETURN_INT( m_name.GetIndex() )
}

void CJournalTutorialGroup::funcGetImage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( m_image );
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalTutorialRoot );

CJournalTutorialRoot::CJournalTutorialRoot()
{

}

CJournalTutorialRoot::~CJournalTutorialRoot()
{

}

Bool CJournalTutorialRoot::IsParentClass( CJournalBase* other ) const
{
	return false;
}
