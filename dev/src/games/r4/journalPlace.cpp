#include "build.h"
#include "journalPlace.h"

IMPLEMENT_ENGINE_CLASS( CJournalPlaceDescription );

CJournalPlaceDescription::CJournalPlaceDescription()
{

}

CJournalPlaceDescription::~CJournalPlaceDescription()
{

}

Bool CJournalPlaceDescription::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalPlace >();
}

void CJournalPlaceDescription::funcGetDescriptionStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_description.GetIndex() );
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalPlace );

CJournalPlace::CJournalPlace()
{

}

CJournalPlace::~CJournalPlace()
{

}

Bool CJournalPlace::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalPlaceGroup >();
}

void CJournalPlace::funcGetNameStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_name.GetIndex() );
}

void CJournalPlace::funcGetImage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( m_image );
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalPlaceGroup );

CJournalPlaceGroup::CJournalPlaceGroup()
{

}

CJournalPlaceGroup::~CJournalPlaceGroup()
{

}

Bool CJournalPlaceGroup::IsParentClass( CJournalBase* other ) const
{
	return other->IsA< CJournalPlaceRoot >();
}

void CJournalPlaceGroup::funcGetNameStringId( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_name.GetIndex() );
}

void CJournalPlaceGroup::funcGetImage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( m_image );
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalPlaceRoot );

CJournalPlaceRoot::CJournalPlaceRoot()
{

}

CJournalPlaceRoot::~CJournalPlaceRoot()
{

}

Bool CJournalPlaceRoot::IsParentClass( CJournalBase* other ) const
{
	return false;
}
