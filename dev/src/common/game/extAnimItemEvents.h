/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/jobGenericJobs.h"

enum EItemAction
{
	IA_Mount,
	IA_MountToHand,
	IA_MountToLeftHand,
	IA_MountToRightHand,
	IA_MountToCustomSlot1,
	IA_MountToCustomSlot2,
	IA_Unmount
};

BEGIN_ENUM_RTTI( EItemAction )
	ENUM_OPTION( IA_Mount )
	ENUM_OPTION( IA_MountToHand )
	ENUM_OPTION( IA_MountToLeftHand )
	ENUM_OPTION( IA_MountToRightHand )
	ENUM_OPTION( IA_MountToCustomSlot1 )
	ENUM_OPTION( IA_MountToCustomSlot2 )
	ENUM_OPTION( IA_Unmount )
END_ENUM_RTTI()

enum EGettingItem
{
	GI_ByName,
	GI_ByCategory,
};

BEGIN_ENUM_RTTI( EGettingItem )
	ENUM_OPTION( GI_ByName )
	ENUM_OPTION( GI_ByCategory )
END_ENUM_RTTI()

class CExtAnimItemEvent : public CExtAnimEvent, public INamesListOwner
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemEvent )

public:
	CExtAnimItemEvent();

	CExtAnimItemEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimItemEvent();

	virtual void GetNamesList( TDynArray< CName >& names ) const;
	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	static void PreprocessItem( CGameplayEntity* gpEnt, SItemUniqueId& spawnedItem, CName item, CName ignoreItemsWithTag );
	EItemAction GetAction() const { return m_action; }
	CName GetCategory() const { return m_category; }
	CName GetItemName() const { return m_itemName_optional; }
	CName GetIgnoreTag() const { return m_ignoreItemsWithTag; }

protected:

	SItemUniqueId GetItemByName( CInventoryComponent* inventory, CName itemName ) const;
	SItemUniqueId GetItemByCategory( CInventoryComponent* inventory, CName category ) const;
	EItemAction		m_action;	
	EGettingItem	m_itemGetting;
	CName			m_category;
	CName			m_itemName_optional;
	CName			m_ignoreItemsWithTag;
};

BEGIN_CLASS_RTTI( CExtAnimItemEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_CUSTOM_EDIT( m_category, TXT( "Category of the item" ), TXT("ItemCategorySelection") )
	PROPERTY_CUSTOM_EDIT( m_itemName_optional, TXT( "Item name (optional)" ), TXT("SelfSuggestedListSelection") );
	PROPERTY_EDIT( m_action, TXT("What to do with the item ?") );
	PROPERTY_EDIT( m_ignoreItemsWithTag, TXT("When finding item by category") );
	PROPERTY_EDIT( m_itemGetting, TXT("How do we want to look for the item in inventory first?") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EItemEffectAction
{
	IEA_Start,
	IEA_Stop
};

BEGIN_ENUM_RTTI( EItemEffectAction )
	ENUM_OPTION( IEA_Start )
	ENUM_OPTION( IEA_Stop )
END_ENUM_RTTI()

class CExtAnimItemEffectEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemEffectEvent )

public:
	CExtAnimItemEffectEvent();

	CExtAnimItemEffectEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );

	virtual ~CExtAnimItemEffectEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

public:
	CName	GetEffectName() const { return m_effectName; }
	EItemEffectAction GetEffectAction() const { return m_action; }

protected:
	CName				m_itemSlot;
	CName				m_effectName;
	EItemEffectAction	m_action;
};

BEGIN_CLASS_RTTI( CExtAnimItemEffectEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_effectName, TXT( "Name of the effect to start or stop" ) );
	PROPERTY_EDIT( m_itemSlot, TXT( "Item in which slot should play the item effect" ) );
	PROPERTY_EDIT( m_action, TXT( "Should effect be started or stopped" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimItemEffectDurationEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemEffectDurationEvent )

public:
	CExtAnimItemEffectDurationEvent();

	CExtAnimItemEffectDurationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );

	virtual ~CExtAnimItemEffectDurationEvent() {}

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

public:
	CName	GetEffectName() const { return m_effectName; }

protected:
	CName				m_itemSlot;
	CName				m_effectName;

	void ProcessEffect( const CAnimationEventFired& info, CAnimatedComponent* component, EItemEffectAction action ) const;
};

BEGIN_CLASS_RTTI( CExtAnimItemEffectDurationEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_effectName, TXT( "Name of the effect to start or stop" ) );
	PROPERTY_EDIT( m_itemSlot, TXT( "Item in which slot should play the item effect" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimItemAnimationEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemAnimationEvent )

public:
	CExtAnimItemAnimationEvent();

	CExtAnimItemAnimationEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );

	virtual ~CExtAnimItemAnimationEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	CName	m_itemCategory;
	CName	m_itemAnimationName;
};

BEGIN_CLASS_RTTI( CExtAnimItemAnimationEvent );
PARENT_CLASS( CExtAnimEvent );
PROPERTY_CUSTOM_EDIT( m_itemCategory, TXT( "Category of the item, we wan't to play effect on" ), TXT( "ItemCategorySelection" ) );
PROPERTY_EDIT( m_itemAnimationName, TXT( "Name of the animation" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimItemBehaviorEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemBehaviorEvent )

public:
	CExtAnimItemBehaviorEvent();

	CExtAnimItemBehaviorEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );

	virtual ~CExtAnimItemBehaviorEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	CName	m_itemCategory;
	CName	m_event;
};

BEGIN_CLASS_RTTI( CExtAnimItemBehaviorEvent );
PARENT_CLASS( CExtAnimEvent );
PROPERTY_CUSTOM_EDIT( m_itemCategory, TXT( "Category of the item, we wan't to raise behavior event on" ), TXT( "ItemCategorySelection" ) );
PROPERTY_EDIT( m_event, TXT( "Name of the behavior event" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EDropAction
{
	DA_DropRightHand,
	DA_DropLeftHand,
	DA_DropAny
};

BEGIN_ENUM_RTTI( EDropAction )
	ENUM_OPTION( DA_DropRightHand )
	ENUM_OPTION( DA_DropLeftHand )
	ENUM_OPTION( DA_DropAny )
END_ENUM_RTTI()

class CExtAnimDropItemEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimDropItemEvent )

public:
	CExtAnimDropItemEvent();

	CExtAnimDropItemEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );

	virtual ~CExtAnimDropItemEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	EDropAction		m_action;
};

BEGIN_CLASS_RTTI( CExtAnimDropItemEvent );
PARENT_CLASS( CExtAnimEvent );
PROPERTY_EDIT( m_action, TXT( "Specify drop action" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimLookAtEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimLookAtEvent )

public:
	CExtAnimLookAtEvent();

	CExtAnimLookAtEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	virtual Bool CanBeMergedWith( const CExtAnimEvent* _with ) const
	{ return false; }

protected:
	void SetActorLookAtLevel( CAnimatedComponent* component, ELookAtLevel level ) const;

protected:
	ELookAtLevel	m_level;
};

BEGIN_CLASS_RTTI( CExtAnimLookAtEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_level, TXT( "Level" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimItemSyncEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemSyncEvent )

public:
	CExtAnimItemSyncEvent();

	CExtAnimItemSyncEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimItemSyncEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	CName				m_equipSlot;
	CName				m_holdSlot;
	EItemLatentAction	m_action;
};

BEGIN_CLASS_RTTI( CExtAnimItemSyncEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_equipSlot, TXT( "Equip slot name" ) );
	PROPERTY_EDIT( m_holdSlot, TXT( "Hold slot name" ) );
	PROPERTY_EDIT( m_action, TXT( "Action type" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimItemSyncDurationEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemSyncDurationEvent )

public:
	CExtAnimItemSyncDurationEvent();

	CExtAnimItemSyncDurationEvent( const CName& eventName, 
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimItemSyncDurationEvent() {}

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	CName				m_equipSlot;
	CName				m_holdSlot;
	EItemLatentAction	m_action;
};

BEGIN_CLASS_RTTI( CExtAnimItemSyncDurationEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_equipSlot, TXT( "Equip slot name" ) );
	PROPERTY_EDIT( m_holdSlot, TXT( "Hold slot name" ) );
	PROPERTY_EDIT( m_action, TXT( "Action type" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimItemSyncWithCorrectionEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimItemSyncWithCorrectionEvent )

public:
	CExtAnimItemSyncWithCorrectionEvent();
	CExtAnimItemSyncWithCorrectionEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimItemSyncWithCorrectionEvent() {}

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

protected:
	CName				m_equipSlot;
	CName				m_holdSlot;
	EItemLatentAction	m_action;
	CName				m_correctionBone;
};

BEGIN_CLASS_RTTI( CExtAnimItemSyncWithCorrectionEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_equipSlot, TXT( "Equip slot name" ) );
	PROPERTY_EDIT( m_holdSlot, TXT( "Hold slot name" ) );
	PROPERTY_EDIT( m_action, TXT( "Action type" ) );
	PROPERTY_EDIT( m_correctionBone, TXT( "Correction bone" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimReattachItemEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimReattachItemEvent )

public:
	CExtAnimReattachItemEvent();
	CExtAnimReattachItemEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimReattachItemEvent() {}

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	CName GetItem() const { return m_item; }

protected:
	CName			m_item;
	CName			m_targetSlot;
};

BEGIN_CLASS_RTTI( CExtAnimReattachItemEvent );
PARENT_CLASS( CExtAnimDurationEvent );
PROPERTY_CUSTOM_EDIT( m_item, TXT( "Category of the item" ), TXT("ItemCategorySelection") )
PROPERTY_EDIT( m_targetSlot, TXT( "Target temporary slot" ) );
END_CLASS_RTTI();
