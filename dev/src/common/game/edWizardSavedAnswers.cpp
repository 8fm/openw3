#include "Build.h"
#include "edWizardSavedAnswers.h"


///////////////////////////////////////////////////////
// CEDSavedAnswer
CEDSavedAnswer::CEDSavedAnswer()
	: m_valid( false )
{
}
CEDSavedAnswer::CEDSavedAnswer( CName questionName, const String & answer, Bool valid )
	: m_questionName( questionName )
	, m_answer( answer )
	, m_valid( valid )
{

}

//////////////////////////////////////////////////////
// CEdWizardSavedAnswers
CEdWizardSavedAnswers::CEdWizardSavedAnswers()
{
}
void CEdWizardSavedAnswers::AddAnswer( CName questionName, const String& answer )
{
	for ( Uint32 i = 0, count = m_list.Size(); i < count; ++i )
	{
		if ( m_list[ i ].m_questionName == questionName )
		{
			m_list[ i ].m_answer	= answer;
			m_list[ i ].m_valid		= true;
			return;
		}
	}
	
	m_list.PushBack( CEDSavedAnswer( questionName, answer, true ) );
}
Bool CEdWizardSavedAnswers::FindAnswer( CName questionName, String &answer )
{
	for ( Uint32 i = 0, count = m_list.Size(); i < count; ++i )
	{
		if ( m_list[ i ].m_questionName == questionName )
		{
			answer = m_list[ i ].m_answer;
			return true;
		}
	}
	return false;
}

void CEdWizardSavedAnswers::PostCommit()
{
	for ( Uint32 i = 0, count = m_list.Size(); i < count; ++i )
	{
		m_list[ i ].m_valid = false;
	}
}
void CEdWizardSavedAnswers::Prune()
{
	TDynArray< CEDSavedAnswer >::iterator it = m_list.Begin();
	while ( it != m_list.End() )
	{
		if ( it->m_valid == false )
		{
			m_list.Erase( it );
		}
		else
		{
			++it;
		}
	}
}