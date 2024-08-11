#include "build.h"
#include "languagePack.h"
#include "localizationCache.h"
#include "localizationManager.h"

LanguagePack::LanguagePack() 
	: m_lipsync( NULL )
	, m_textString( String::EMPTY )
	, m_lockCount( 0 )
{
	m_speechBuffer.Clear();
}

LanguagePack::~LanguagePack()
{
	ASSERT( m_lockCount == 0 );

	m_speechBuffer.Clear();
	SetLipsync( NULL );
}

void LanguagePack::SetLipsync( CSkeletalAnimation* value )
{
	if ( m_lipsync.IsValid() )
	{
		SLocalizationManager::GetInstance().GetStorage().UnlockLipsync( m_lipsync );
	}

	m_lipsync = value;

	if ( m_lipsync.IsValid() )
	{
		SLocalizationManager::GetInstance().GetStorage().LockLipsync( m_lipsync );
	}
}
