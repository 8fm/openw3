#pragma once


#include "r6Component.h"

class CItemPartSlotDefinition;
class IUpgradeEventHandler;
class CBaseUpgradeStatsModyfier;
class CStatsContainerComponent;

class CUpgradeEventHandlerParam;

class CUpgradeEventDefinition : public CObject, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CUpgradeEventDefinition, CObject, 0 );
private:
	CName										m_eventName;	
	CUpgradeEventHandler*						m_eventHandlerInstance;	

public:	
	RED_INLINE CName GetEventName(){ return m_eventName; }
	RED_INLINE CUpgradeEventHandler* GetHandlerInstance() { return m_eventHandlerInstance; }	

	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
};
BEGIN_CLASS_RTTI( CUpgradeEventDefinition )
	PARENT_CLASS( CObject );	
	PROPERTY_CUSTOM_EDIT( m_eventName			, TXT("Event name")		, TXT("2daValueSelection")  );
	PROPERTY_INLINED( m_eventHandlerInstance	, TXT("Slots")	);
END_CLASS_RTTI();

class CItemPartDefinitionComponent : public CR6Component
{
	DECLARE_ENGINE_CLASS( CItemPartDefinitionComponent, CComponent, 0 );
private:
	TDynArray< CItemPartSlotDefinition* >		m_slots;
	TDynArray< CUpgradeEventDefinition* >		m_avaliableEvents;		
	THandle< CItemPartDefinitionComponent >		m_parentPart;
	CName										m_inParentSlotName;	
	TDynArray< CBaseUpgradeStatsModyfier* >		m_statsModifiers;
	TDynArray< CItemPartSlotDefinition* >		m_slotsUsingParentSlots;	

	RED_INLINE void SetParentPart( CItemPartDefinitionComponent* parentPart ){ m_parentPart = parentPart; }
	RED_INLINE void SetInParentSlotName( CName slotName ){ m_inParentSlotName = slotName; }	
	RED_INLINE TDynArray< CItemPartSlotDefinition* >& GetSlotsUsingParentSlots(){ return m_slotsUsingParentSlots; } 
	RED_INLINE void ClearSlotUsingParentSlots(){ m_slotsUsingParentSlots.Clear(); }	
	
	void SpawnSlotsContent();	
	void SpawnSlotContent( CItemPartSlotDefinition* slot );
	void DoStatsStuff();
	void ApplyStatsModifications( CStatsContainerComponent* stats );
	void HandleInParentSlots( CItemPartDefinitionComponent* childCmp );
	void AttachEntityToSlot( CItemPartSlotDefinition* slot, CEntity* entity );	

	void AttachToParentRootIfNeeded( CAnimatedComponent* parentRoot );
	void DetachSlots();

	Bool PlugEntityToSlot( CEntity* eTamplate, CName slotName, Bool recursive = true );
	Bool PlugEntityTemplateToSlot( const THandle< CEntityTemplate >& eTamplate, CName slotName, Bool recursive = true );	
	Bool DetachSlotContent( CName slotName, Bool destroyEntity = true, Bool recursive = true );

public:
	RED_INLINE CName GetInParentSlotName(){ return m_inParentSlotName; }
	RED_INLINE TDynArray< CItemPartSlotDefinition* > GetSlots(){ return m_slots; }
	void OnAttachFinished( CWorld* world )			override;
	void OnAttachFinishedEditor( CWorld* world )	override;
	void OnDetached( CWorld* world )				override;

	void SendToParent( CName eventName, CUpgradeEventHandlerParam* dynamicParams = NULL );
	Bool ForwardEvent( CName childSlotName, CName eventName, CUpgradeEventHandlerParam* dynamicParams = NULL );
	void ProcessEvent( CName eventName, CUpgradeEventHandlerParam* dynamicParams = NULL, CName childSlotName = CName::NONE );

	CItemPartDefinitionComponent* GetPartPluggedToSlot( CName slotName ); 
		
	Bool PlugEntityToSlotInit( CEntity* eTamplate, CName slotName, Bool recursive = true );	
	Bool PlugEntityTemplateToSlotInit( const THandle< CEntityTemplate >& eTamplate, CName slotName, Bool recursive = true );
	Bool DetachSlotContentInit( CName slotName, Bool destroyEntity = true, Bool recursive = true );

	Bool FindEntityInSlot( CName slotName, CEntity*& retEntity ); // returns true, when slot was found
	//void DestroySlotContent( CName slotName, Bool recursive = true );

	CEntity* GetRootEntity();

private:
	void funProcessEvent( CScriptStackFrame& stack, void* result );
	void funcPlugEntityToSlot( CScriptStackFrame& stack, void* result );
	void funcDetachSlotContent( CScriptStackFrame& stack, void* result );
	void funcFindEntityInSlot( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CItemPartDefinitionComponent )
	PARENT_CLASS( CComponent );	
	PROPERTY_INLINED( m_slots			,TXT( "Slots"				) );		
	PROPERTY_INLINED( m_avaliableEvents	,TXT( "Avaliable events"	) );	
	PROPERTY_INLINED( m_statsModifiers	,TXT( "Stats modifiers" ) );	

	NATIVE_FUNCTION( "I_ProcessEvent"		, funProcessEvent		);
	NATIVE_FUNCTION( "I_PlugEntityToSlot"	, funcPlugEntityToSlot	);
	NATIVE_FUNCTION( "I_DetachSlotContent"	, funcDetachSlotContent	);
	NATIVE_FUNCTION( "I_FindEntityInSlot"	, funcFindEntityInSlot	);

END_CLASS_RTTI();