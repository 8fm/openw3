/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "../../common/engine/entityTemplateModifier.h"

class CR4VoicetagDLCMounter : public IGameplayDLCMounter, public IEntityTemplateModifier
{
	DECLARE_ENGINE_CLASS( CR4VoicetagDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4VoicetagDLCMounter() {}

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;
	
	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

	//! IEntityTemplateModifier
private:
	virtual void EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate ) override;
	virtual void EntityTemplateOnFinalize( CEntityTemplate* entityTemplate ) override;

private:
	void Activate();
	void Deactivate();

	typedef TDynArray< String > TEntityTemplatePaths;
	TEntityTemplatePaths m_entityTemplatePaths;

	typedef TDynArray< VoicetagAppearancePair > TVoicetagAppearances;
	TVoicetagAppearances m_voicetags;

	TDynArray< CEntityTemplate* > m_entityTemplates;
};

BEGIN_CLASS_RTTI( CR4VoicetagDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT_ARRAY( m_entityTemplatePaths, TXT("Paths to entity template") );
PROPERTY_INLINED( m_voicetags, TXT("") );
END_CLASS_RTTI();
