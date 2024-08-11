#include "build.h"
#include "../../common/core/depot.h"
#include "r4DLCVoicetagMounter.h"

IMPLEMENT_ENGINE_CLASS( CR4VoicetagDLCMounter );

void CR4VoicetagDLCMounter::EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate )
{
	// Modify entity template. Don't do this if entity template is already modified by this mounter.
	const Bool alreadyModified = m_entityTemplates.Exist( entityTemplate );
	if( !alreadyModified )
	{
		m_entityTemplates.PushBack( entityTemplate );
		entityTemplate->AddVoicetagsAppearances( m_voicetags );
	}
}

void CR4VoicetagDLCMounter::EntityTemplateOnFinalize( CEntityTemplate* entityTemplate )
{
	CEntityTemplate** entityTemplateFound = m_entityTemplates.FindPtr( entityTemplate );
	if( entityTemplateFound )
	{
		entityTemplate->RemoveVoicetagsAppearances( m_voicetags );
		m_entityTemplates.Erase( entityTemplateFound );
	}	
}

bool CR4VoicetagDLCMounter::OnCheckContentUsage()
{
	//! nothing
	return false;
}

void CR4VoicetagDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4VoicetagDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4VoicetagDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4VoicetagDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4VoicetagDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	//! nothing
	return true;
}

#endif // !NO_EDITOR

void CR4VoicetagDLCMounter::Activate()
{
	RED_FATAL_ASSERT( !SEntityTemplateModifierManager::GetInstance().IsActive(), "" ); // modified manager is not yet be started
	RED_FATAL_ASSERT( m_entityTemplates.Empty(), "" );								   // we're just about to activate this mounter so there surely are no entity templates already modified

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().RegisteModifier( entityTemplatePath, this );
	}
}

void CR4VoicetagDLCMounter::Deactivate()
{
	RED_FATAL_ASSERT( !SEntityTemplateModifierManager::GetInstance().IsActive(), "" ); // modified manager must already be deactivated
	RED_FATAL_ASSERT( m_entityTemplates.Empty(), "" );								   // must be empty as all modificiations should already be removed during modifier mgr deactivation

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().UnregisteModifier( entityTemplatePath, this );
	}
}
