#pragma once
#include "../../common/engine/entitytemplateparams.h"
#include "../../common/game/journalPath.h"
#include "gameplayEffectExecutor.h"

/// Vital spots seen while in combat mode

struct SVitalSpotEnableConditions
{
	DECLARE_RTTI_STRUCT( SVitalSpotEnableConditions );

	SVitalSpotEnableConditions() : m_enableByDefault(true)
	{}

	CName					m_animationEventName;
	CName					m_VSActivatorBehTreeNodeName;
	Bool					m_enableByDefault;
	//Bool					m_callScriptFunction;
};

BEGIN_CLASS_RTTI(SVitalSpotEnableConditions);
	PROPERTY_EDIT( m_animationEventName, TXT("Name of the anim duration event that need to be active") );
	PROPERTY_EDIT( m_VSActivatorBehTreeNodeName, TXT("Name of the VitalSpotActivator node in behaviour tree") );
	PROPERTY_EDIT( m_enableByDefault, TXT("Should this vital spot be enabled by default ?") );
	//PROPERTY_EDIT( m_callScriptFunction, TXT("Should custom script function be called") );
END_CLASS_RTTI();


/// Vital spots seen while in combat mode
class CVitalSpot : public CObject
{
	DECLARE_ENGINE_CLASS( CVitalSpot, CObject, 0 );

public:
	CVitalSpot() : m_destroyAfterExecution(true)
	{}

	Vector											m_normal;
	Vector											m_cutDirection;
	CName											m_editorLabel;
	CName											m_entitySlotName;
	CName											m_visualEffect;
	CName											m_hitReactionAnimation;

	CName											m_animEventName;
	CName											m_aiTaskActive;

	Bool											m_destroyAfterExecution;

	Float											m_focusPointsCost;
	THandle< CJournalPath >									m_vitalSpotEntry;
	String											m_soundOnFocus;
	String											m_soundOffFocus;
	TDynArray< THandle< IGameplayEffectExecutor > >	m_gameplayEffects;
	SVitalSpotEnableConditions						m_enableConditions;


	void funcGetJournalEntry( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CVitalSpot );
	PARENT_CLASS( CObject );
	PROPERTY_RO( m_editorLabel , TXT("Unique tag of Journal entry") );
	PROPERTY_EDIT( m_entitySlotName, TXT("Name of the CEntitySlot that the vital spot is reffering to") );
	PROPERTY_EDIT( m_normal, TXT("The normal of this vital spot defining it's 'front' direction") );
	PROPERTY_EDIT( m_cutDirection, TXT("Cut direction of attack performed to destroy this spot") );
	PROPERTY_CUSTOM_EDIT( m_vitalSpotEntry, TXT("Path to vital spot in Journal editor"), TXT("JournalPropertyBrowserCreatureVitalSpot") )
	PROPERTY_EDIT( m_hitReactionAnimation, TXT("Name of the hit animation") ) 
	PROPERTY_EDIT( m_focusPointsCost, TXT("Number of focus points needed to attack this spot") );
	PROPERTY_EDIT(  m_destroyAfterExecution, TXT("This vital spot can be hit once/multiple times") )
	PROPERTY_INLINED( m_gameplayEffects, TXT("Effects that are applied after destroying this spot") );
	PROPERTY_INLINED( m_enableConditions, TXT("Conditions that need to be fulfilled for effect to be enabled") );
	PROPERTY_EDIT( m_visualEffect, TXT("Name of the FX to play") );
	PROPERTY_CUSTOM_EDIT( m_soundOnFocus, TXT( "Sound event name" ), TXT( "AudioEventBrowser" ) );
	PROPERTY_CUSTOM_EDIT( m_soundOffFocus, TXT( "Sound event name"), TXT( "AudioEventBrowser" ) );
	NATIVE_FUNCTION( "GetJournalEntry", funcGetJournalEntry );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CVitalSpotsParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CVitalSpotsParam, CGameplayEntityParam, 0 );

public:

	THandle< CJournalPath > 			m_journalCreaturVitalSpotsPath;
	TDynArray< CVitalSpot* >			m_vitalSpots;

	const TDynArray< CVitalSpot* >& GetVitalSpots() const
	{
		return m_vitalSpots;
	}
#ifndef NO_EDITOR
	virtual Bool OnPropModified( CName fieldName );
	CVitalSpot*  FindVitalSpotWithTag( CName uniqueScriptIdentifier );
#endif
	
};

BEGIN_CLASS_RTTI( CVitalSpotsParam );
	PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_journalCreaturVitalSpotsPath, TXT("Path to vital spots group in Journal editor"), TXT("JournalPropertyBrowserCreatureVitalSpotGroup") )
	PROPERTY_INLINED( m_vitalSpots, TXT("Vital spots") )
END_CLASS_RTTI();
