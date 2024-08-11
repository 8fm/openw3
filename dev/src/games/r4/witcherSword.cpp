/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "witcherSword.h"
#include "../../common/physics/physicsWrapper.h"

IMPLEMENT_RTTI_ENUM( EWitcherSwordType );
IMPLEMENT_ENGINE_CLASS( CWitcherSword );

void CWitcherSword::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
}

void CWitcherSword::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
}

void CWitcherSword::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );
}

void CWitcherSword::SetRuneIndexForSlot( Int32 slotIndex, Int32 runeIndex )
{
	String meshComponentName = TXT("rune_slot_");
	meshComponentName += ToString( slotIndex );

	CComponent* component = FindComponent( meshComponentName, false );
	if ( component )
	{
		EffectParameterValue effectValue;
		effectValue.SetFloat( (Float)runeIndex );
		component->SetEffectParameterValue( CNAME( MeshEffectScalar0 ), effectValue );
	}
}

void CWitcherSword::FlashRunes()
{
	PlayEffect( CNAME( FlashRunes ) );
}

RED_DEFINE_STATIC_NAME( OnUpdateRunes );

void CWitcherSword::OnAttachmentUpdate()
{
	if ( IsInGame() && m_parentEntity.Get() && m_proxy )
	{
		CallEvent( CNAME( OnUpdateRunes ), m_proxy->m_slotItems );
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CWitcherSword::funcGetSwordType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_ENUM( GetSwordType() );
}

void CWitcherSword::funcFlashRunes( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	FlashRunes();
}

void CWitcherSword::funcSetRuneIndexForSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, slotIndex, 1 );
	GET_PARAMETER( Int32, runeIndex, 0 );
	FINISH_PARAMETERS;

	SetRuneIndexForSlot( slotIndex, runeIndex );
}
