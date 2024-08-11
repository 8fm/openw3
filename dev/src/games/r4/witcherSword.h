/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/itemEntity.h"
#include "../../common/physics/physicalCallbacks.h"

enum EWitcherSwordType
{
	WST_Silver,
	WST_Steel
};

BEGIN_ENUM_RTTI( EWitcherSwordType );
	ENUM_OPTION( WST_Silver );
	ENUM_OPTION( WST_Steel );
END_ENUM_RTTI();

class CWitcherSword : public CItemEntity
{
	DECLARE_ENGINE_CLASS( CWitcherSword, CItemEntity, 0 );

private:
	EWitcherSwordType	m_swordType;

public:
	CWitcherSword() : m_swordType( WST_Steel ) {}

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	// All components of entity has been attached
	virtual void OnAttachFinished( CWorld* world );
	
public:
	EWitcherSwordType GetSwordType() { return m_swordType; }

	// Choose which rune should be displayed on given slot
	void SetRuneIndexForSlot( Int32 slotIndex, Int32 runeIndex );

	// Flash runes
	void FlashRunes();
public:
	// Attachment changed
	virtual void OnAttachmentUpdate();

public:
	// script support
	void funcFlashRunes( CScriptStackFrame& stack, void* result );
	void funcGetSwordType( CScriptStackFrame& stack, void* result );
	void funcSetRuneIndexForSlot( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CWitcherSword );
	PARENT_CLASS( CItemEntity );
	PROPERTY_EDIT( m_swordType, TXT( "Type of the sword" ) );
	NATIVE_FUNCTION( "GetSwordType", funcGetSwordType );
	NATIVE_FUNCTION( "FlashRunes", funcFlashRunes );
	NATIVE_FUNCTION( "SetRuneIndexForSlot", funcSetRuneIndexForSlot );
END_CLASS_RTTI();
