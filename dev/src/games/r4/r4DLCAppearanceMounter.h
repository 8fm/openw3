/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "../../common/engine/entityTemplateModifier.h"
#include "../../common/engine/appearanceComponentModifier.h"

class CR4EntityExternalAppearanceDLC: public CObject
{
	DECLARE_ENGINE_CLASS( CR4EntityExternalAppearanceDLC, CObject, 0 );
public: 
	CR4EntityExternalAppearanceDLC(){}
	
public:	
	CName								m_appearanceToRepleace;
	THandle<CEntityExternalAppearance>  m_entityExternalAppearance;
};

BEGIN_CLASS_RTTI( CR4EntityExternalAppearanceDLC );
PARENT_CLASS( CObject );	
	PROPERTY_EDIT( m_appearanceToRepleace,				TXT("Appearance to repleace form base game") )
	PROPERTY_EDIT( m_entityExternalAppearance,			TXT("Entity external appearance") )
END_CLASS_RTTI();

class CR4EntityExternalAppearanceDLCMounter : public IGameplayDLCMounter, public IEntityTemplateModifier, public IAppearanceComponentModifierModifier
{
	DECLARE_ENGINE_CLASS( CR4EntityExternalAppearanceDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4EntityExternalAppearanceDLCMounter();
	virtual ~CR4EntityExternalAppearanceDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual void OnSerialize( class IFile& file ) override;
	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

	//! IEntityTemplateModifier
private:
	virtual void EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate ) override;
	virtual const CName& EntityTemplateOnGetAppearance( const CEntityTemplate* entityTemplate, const CName& appearanceName ) const override;
	virtual void EntityTemplateOnFinalize( CEntityTemplate* entityTemplate ) override;

	//! IAppearanceComponentModifierModifier
private:
	virtual void OnAppearanceChange( CAppearanceComponent* appearanceComponent ) override;
	virtual void AppearanceComponentOnFinalize( CAppearanceComponent* appearanceComponent ) override;

private:
	void AddAppearances( CEntityTemplate* entityTemplate, TDynArray<CR4EntityExternalAppearanceDLC*>& entityExternalAppearances );
	void RemoveAppearances( CEntityTemplate* entityTemplate, TDynArray<CR4EntityExternalAppearanceDLC*>& entityExternalAppearances );

private:
	void AddAppearanceAttachments( CAppearanceComponent* appearanceComponent );
	void RemoveAppearanceAttachments( CAppearanceComponent* appearanceComponent );

private:
	void Activate();
	void Deactivate();

	typedef TDynArray< String > TEntityTemplatePaths;
	TEntityTemplatePaths m_entityTemplatePaths;

	typedef TDynArray< CR4EntityExternalAppearanceDLC* > TEntityExternalAppearances;
	TEntityExternalAppearances m_entityExternalAppearances;

	Bool m_mounterStarted;

	typedef THashMap< CEntityTemplate*, TDynArray< CR4EntityExternalAppearanceDLC* > > TEntityTemplateEntries;
	TEntityTemplateEntries m_entityTemplates;

	typedef THashMap< CAppearanceComponent*, CR4EntityExternalAppearanceDLC* > TAppearanceComponentEntries;
	TAppearanceComponentEntries m_appearanceComponents;
};

BEGIN_CLASS_RTTI( CR4EntityExternalAppearanceDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT_ARRAY( m_entityTemplatePaths, TXT("Paths to entity template") );
PROPERTY_INLINED( m_entityExternalAppearances, TXT("Entity external appearances loaded form DLC") );
END_CLASS_RTTI();
