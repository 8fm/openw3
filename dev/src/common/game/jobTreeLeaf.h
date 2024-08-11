/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "actionPointManager.h"
#include "../core/2darray.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/actorInterface.h"

struct SJobActionExecutionContext
{
	struct SItemInfo
	{
		SItemUniqueId					m_apItem;					//!< Item from ap
		SItemUniqueId					m_npcItem;					//!< Item from ap, copied in npc's inventory

		SItemInfo()
		{
			Reset();
		}

		void Reset()
		{
			m_apItem = SItemUniqueId::INVALID;
			m_npcItem = SItemUniqueId::INVALID;
		}

		Bool IsUsed() const
		{
			return m_apItem != SItemUniqueId::INVALID;
		}
	};

	SItemInfo						m_item1;
	SItemInfo						m_item2;
	CNewNPC*						m_NPCActor;					//!< NPC ptr stored for community system interaction

	SJobActionExecutionContext() 
	{
		Reset();
	}

	void Reset()
	{
		m_NPCActor = NULL;
		m_item1.Reset();
		m_item2.Reset();
	}

	RED_INLINE Bool IsValid() const { return m_NPCActor != NULL; }
	RED_INLINE Bool IsUsingItems() const { return m_item1.IsUsed() || m_item2.IsUsed(); }
};

class CAnimationsCategoriesResourcesManager: public C2dArraysResourcesManager
{
public:
	CAnimationsCategoriesResourcesManager();
};

typedef TSingleton< CAnimationsCategoriesResourcesManager, TDefaultLifetime, TCreateUsingNew > SAnimationsCategoriesResourcesManager;

class CJobActionBase : public CObject, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CJobActionBase, CObject, 0 );

protected:
	String				m_animCategory;
	CName				m_animName;
	Float				m_animBlendIn;
	Float				m_animBlendOut;
	Bool				m_fireBlendedEvents;
	Bool				m_isSkippable;
	ELookAtLevel		m_allowedLookAtLevel;
	CName				m_ignoreIfItemMounted;
		//m_isLookAtEnabled;
#ifndef NO_EDITOR
	CName				m_animType;
#endif

public:
	CJobActionBase();

public:
	//! Get the animation category name
	RED_INLINE String	GetAnimCategory() const { return m_animCategory; }

	//! Get the animation name
	RED_INLINE const CName& GetAnimName() const { return m_animName; }

	//! Animation type for debugging purposes: enter, anim, exit, forceExit...
#ifndef NO_EDITOR
	RED_INLINE void SetAnimType( CName& animType ) { m_animType = animType; }
	RED_INLINE const CName& GetAnimType() const { return m_animType; }
#endif

	//! Get animation blend in duration
	RED_INLINE Float GetBlendIn() const { return m_animBlendIn; }

	//! Get animation blend out duration
	RED_INLINE Float GetBlendOut() const { return m_animBlendOut; }

	//! Should fire blended in/out events
	RED_INLINE Bool ShouldFireBlendedEvents() const { return m_fireBlendedEvents; }

	RED_INLINE ELookAtLevel GetAllowedLookatLevel() const { return m_allowedLookAtLevel; }

	RED_INLINE CName GetIgnoreIfItemMounted() const { return m_ignoreIfItemMounted; }

	RED_INLINE Bool IsSkippable() const { return m_isSkippable; }

	//! Get the place of the work
	virtual const CName& GetPlace() const { return CName::NONE; }

	//! Get name of the item associated with action
	virtual const CName& GetItemName() const { return CName::NONE; }

	//! Gets the motion extraction for the action
	void GetMotionExtraction( CAnimatedComponent* animatedComp, Vector& translation, Float& rotation, Float* duration = nullptr ) const;
	void GetMotionExtractionForTime( CAnimatedComponent* animatedComp, Vector& translation, Float& rotation, Float timeRatio ) const;

	virtual Float GetDurationMultiplier() const;

	//! Collect animation events of given type
	template < typename EventType >
	Bool CollectEvents( CAnimatedComponent* animatedComp, TDynArray< EventType* > & events ) const;

	//! Collect animation events of given type
	template < typename EventType, typename Collector >
	Bool CollectEvents( CAnimatedComponent* animatedComp, Collector& collector ) const;

	// ------------------------------------------------------------------------
	// Action execution
	// ------------------------------------------------------------------------
	//! Called when the action is started
	virtual void OnStarted( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const;

	//! Called when the action is executed
	virtual void OnAnimEvent( TActionPointID currentApId, SJobActionExecutionContext& actionContext, const CAnimationEventFired &event ) const;

	//! Called when the action is finished
	virtual void OnFinished( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const;

	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

#ifndef NO_EDITOR
	RED_INLINE void SetAnimName( const CName& animName ) { m_animName = animName; }
#endif


protected:
	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties );
};

BEGIN_CLASS_RTTI( CJobActionBase );
	PARENT_CLASS( CObject );
	PROPERTY_CUSTOM_EDIT_NAME( m_animCategory, TXT("categoryName"), TXT("Animation category name"), TXT("2daValueSelection") );
	PROPERTY_CUSTOM_EDIT( m_animName, TXT("Name of the animation to play"), TXT("jobActionAnimSelection") );
	PROPERTY_EDIT( m_animBlendIn, TXT( "Animation blend in duration" ) );
	PROPERTY_EDIT( m_animBlendOut, TXT( "Animation blend out duration" ) );
	PROPERTY_EDIT( m_fireBlendedEvents, TXT( "Fire events if animations is blended in/out" ) );
	PROPERTY_EDIT( m_allowedLookAtLevel, TXT("Allowed look at level") );
	PROPERTY_EDIT( m_ignoreIfItemMounted, TXT("Ignore action if item is mounted") );	
	PROPERTY_EDIT( m_isSkippable, TXT("Is skippable (used only for approach actions)") );	
END_CLASS_RTTI();

template < typename EventType >
Bool CJobActionBase::CollectEvents( CAnimatedComponent* animatedComp, TDynArray< EventType* > & events ) const
{
	CSkeletalAnimationSetEntry* animation = animatedComp->GetAnimationContainer()->FindAnimation( m_animName );
	if ( animation && animation->GetAnimation() )
	{
		animation->GetEventsOfType< EventType >( events );
		return true;
	}
	return false;
}

template < typename EventType, typename Collector >
Bool CJobActionBase::CollectEvents( CAnimatedComponent* animatedComp, Collector& collector ) const
{
	CSkeletalAnimationSetEntry* animation = animatedComp->GetAnimationContainer()->FindAnimation( m_animName );
	if ( animation && animation->GetAnimation() )
	{
		animation->GetEventsOfType< EventType, Collector >( collector );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

class CJobAction : public CJobActionBase
{
	DECLARE_ENGINE_CLASS( CJobAction, CJobActionBase, 0 );
	
protected:
	CName				m_place;
	CName				m_itemName;

public:
	//! Get the place of the work
	virtual const CName& GetPlace() const override;

	//! Get name of the item associated with action
	virtual const CName& GetItemName() const override;

	// ------------------------------------------------------------------------
	// Action execution
	// ------------------------------------------------------------------------
	//! Called when the action is started
	virtual void OnStarted( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const override;

	//! Called when the action is executed
	virtual void OnAnimEvent( TActionPointID currentApId, SJobActionExecutionContext& actionContext, const CAnimationEventFired &event ) const override;

	//! Called when the action is finished
	virtual void OnFinished( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const override;

public:
	CJobAction();

	void PerformPickUp( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const;
	void PerformPut( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const;

protected:

	void PerformPickUp( TActionPointID currentApId, SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo ) const;
	void PerformPut( TActionPointID currentApId, SJobActionExecutionContext& actionContext , SJobActionExecutionContext::SItemInfo& itemInfo ) const;
};

BEGIN_CLASS_RTTI( CJobAction );
	PARENT_CLASS( CJobActionBase );
	PROPERTY_EDIT( m_place, TXT( "Waypoint tag" ) );
	PROPERTY_CUSTOM_EDIT( m_itemName, TXT( "Optional name of the item to grab/put" ), TXT("ChooseItem") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EJobForceOutDropMode
{
	JFODM_DropAll,
	JFODM_DropNonWeapon,
	JFODM_NoDrop
};

BEGIN_ENUM_RTTI( EJobForceOutDropMode );
	ENUM_OPTION( JFODM_DropAll );
	ENUM_OPTION( JFODM_DropNonWeapon );
	ENUM_OPTION( JFODM_NoDrop );
END_ENUM_RTTI();

class CJobForceOutAction : public CJobActionBase
{
	DECLARE_ENGINE_CLASS( CJobForceOutAction, CJobActionBase, 0 );

private:
	EJobForceOutDropMode	m_itemDropMode;
	Float					m_speedMul;
public:
	CJobForceOutAction();

	EJobForceOutDropMode	GetDropMode() const									{ return m_itemDropMode; }
	Float					GetSpeedMul() const									{ return m_speedMul; }

	virtual Float			GetDurationMultiplier() const override;

	// ------------------------------------------------------------------------
	// Action execution
	// ------------------------------------------------------------------------
	//! Called when the action is finished
	virtual void OnFinished( TActionPointID currentApId, SJobActionExecutionContext& actionContext ) const;

	//! Called when the action is executed
	virtual void OnAnimEvent( TActionPointID currentApId, SJobActionExecutionContext& actionContext, const CAnimationEventFired &event ) const;

private:
	void DropActionPointItems( TActionPointID workplaceApId, SJobActionExecutionContext& actionContext ) const;
	void DropActionPointItems( TActionPointID workplaceApId, SJobActionExecutionContext& actionContext, SJobActionExecutionContext::SItemInfo& itemInfo ) const;
};

BEGIN_CLASS_RTTI( CJobForceOutAction );
	PARENT_CLASS( CJobActionBase );
	PROPERTY_EDIT( m_itemDropMode, TXT( "What should happen with held items" ) );
	PROPERTY_EDIT( m_speedMul, TXT( "Playback speed" ) );
END_CLASS_RTTI();
