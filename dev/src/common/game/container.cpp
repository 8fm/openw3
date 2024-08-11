#include "build.h"
#include "container.h"

IMPLEMENT_ENGINE_CLASS( W3Container );

//////////////////////////////////////////////////////////////////////////////////////////////
W3Container::W3Container()
	: m_shouldBeFullySaved( false )
{

}

Bool W3Container::CheckShouldSave() const
{
	if( HasToBeFullySaved() )
	{
		return TBaseClass::CheckShouldSave();
	}

	return false;
}


void W3Container::SetShouldBeFullySaved( Bool fullySaved )
{
	if ( m_shouldBeFullySaved != fullySaved )
	{
		m_shouldBeFullySaved = fullySaved;
	}

	UpdateShouldBeFullySaved();
}

void W3Container::UpdateShouldBeFullySaved()
{
	SetShouldSave( CheckShouldSave() );
}

void W3Container::funcSetIsQuestContainer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, quest, false );
	FINISH_PARAMETERS;
	SetShouldBeFullySaved( quest );
}
