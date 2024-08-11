/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "../../common/engine/entityTemplateModifier.h"

class CR4EntityTemplateSlotDLCMounter : public IGameplayDLCMounter, public IEntityTemplateModifier
{
	DECLARE_ENGINE_CLASS( CR4EntityTemplateSlotDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4EntityTemplateSlotDLCMounter();
	virtual ~CR4EntityTemplateSlotDLCMounter();

	RED_FORCE_INLINE const String& GetBaseEntityTemplatePath() const { return m_baseEntityTemplatePath; }

	//! IGameplayDLCMounter
public:
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

	//! IEntityTemplateModifier
private:
	virtual void EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate ) override;
	virtual void EntityTemplateOnFinalize( CEntityTemplate* entityTemplate ) override;

private:
	void AddSlots( CEntityTemplate* entityTemplate, TDynArray<EntitySlot*>& entityTemplateSlotInstances );
	void RemoveSlots( CEntityTemplate* entityTemplate, TDynArray<EntitySlot*>& entityTemplateSlotInstances );

private:
	void Activate();
	void Deactivate();

	String m_baseEntityTemplatePath;

	typedef TDynArray< String > TEntityTemplatePaths;
	TEntityTemplatePaths m_entityTemplatePaths;
	
	typedef TDynArray< EntitySlot * > TEntityTemplateSlotEntries;
	TEntityTemplateSlotEntries m_entityTemplateSlots;

	Bool m_mounterStarted;

	typedef THashMap< CEntityTemplate*, TDynArray< EntitySlot* > > TEntityTemplateEntries;
	TEntityTemplateEntries m_entityTemplates;
};

BEGIN_CLASS_RTTI( CR4EntityTemplateSlotDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_EDIT( m_baseEntityTemplatePath,TXT("Path to base entity template") );
PROPERTY( m_entityTemplatePaths );
PROPERTY_INLINED( m_entityTemplateSlots, TXT("Entity template extra slots loaded form DLC") );
END_CLASS_RTTI();
