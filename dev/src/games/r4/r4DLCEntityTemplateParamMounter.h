/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "../../common/engine/entityTemplateModifier.h"

class CR4EntityTemplateParamDLCMounter : public IGameplayDLCMounter, public IEntityTemplateModifier
{
	DECLARE_ENGINE_CLASS( CR4EntityTemplateParamDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4EntityTemplateParamDLCMounter();
	virtual ~CR4EntityTemplateParamDLCMounter();

public:
	//! IGameplayDLCMounter
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual bool DoAnalyze( CAnalyzerOutputList& outputList ) override;
	Bool IsIncludeEntityTemplate( const CEntityTemplate* entityTemplate, const Char* baseEntityTemplateDepotPath ) const;
	virtual void OnSerialize( class IFile& file ) override;

#endif // !NO_EDITOR

private:
	//! IEntityTemplateModifier
	virtual void EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate ) override;
	virtual void EntityTemplateOnFinalize( CEntityTemplate* entityTemplate ) override;

private:
	void AddParameters( CEntityTemplate* entityTemplate, TDynArray<CEntityTemplateParam*>& entityTemplateParamInstances );
	void RemoveParameters( CEntityTemplate* entityTemplate, TDynArray<CEntityTemplateParam*>& entityTemplateParamInstances );

private:
	void Activate();
	void Deactivate();

	typedef TDynArray< String > TEntityTemplatePaths;
	TEntityTemplatePaths m_entityTemplatePaths;															// List of entity templates to modify.
	
	typedef TDynArray< CEntityTemplateParam * > TEntityTemplateParamEntries;
	TEntityTemplateParamEntries m_entityTemplateParams;													// List of params to add to each entity template.

	Bool m_mounterStarted;

	typedef THashMap< CEntityTemplate*, TDynArray< CEntityTemplateParam* > > TEntityTemplateEntries;
	TEntityTemplateEntries m_entityTemplates;															// List of entity templates already modified, along with params that were added.
};

BEGIN_CLASS_RTTI( CR4EntityTemplateParamDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT_ARRAY( m_entityTemplatePaths, TXT("Paths to entity template") );
PROPERTY_INLINED( m_entityTemplateParams, TXT("Entity template extra params loaded form DLC") );
END_CLASS_RTTI();
