#include "build.h"
#include "scriptRegistrationManager.h"

///////////////////////////////////////////////////
//	CScriptRegistration
CScriptRegistration::CScriptRegistration()
{
	CScriptRegistrationManager::GetInstance()->AddScriptRegistration( this );
}








////////////////////////////////////////////////////
//  CScriptRegistrationManager
CScriptRegistrationManager * CScriptRegistrationManager::s_instance	= NULL;

CScriptRegistrationManager::CScriptRegistrationManager()
{
}

CScriptRegistrationManager *const CScriptRegistrationManager::GetInstance()
{
	if ( s_instance == NULL )
	{
		s_instance = new CScriptRegistrationManager();
	}
	return s_instance;
}

void CScriptRegistrationManager::ReleaseInstance()
{
	delete s_instance;
}

void CScriptRegistrationManager::RegisterScriptFunctions()
{
	for ( Uint32 i = 0; i < m_scriptRegistrationArray.Size(); ++i )
	{
		m_scriptRegistrationArray[ i ]->RegisterScriptFunctions();
	}
}
void CScriptRegistrationManager::AddScriptRegistration( const CScriptRegistration *const scriptRegistration )
{
	ASSERT( MAX_SCRIPT_REG_COUNT != m_scriptRegistrationArray.Size(), TXT("m_scriptRegistrationArray too small, Please grow MAX_SCRIPT_REG_COUNT") );
	m_scriptRegistrationArray.PushBack( scriptRegistration );
}