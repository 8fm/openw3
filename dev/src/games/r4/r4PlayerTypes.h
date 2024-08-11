/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum EBufferActionType
{
	EBAT_EMPTY,
	EBAT_LightAttack,
	EBAT_HeavyAttack,
	EBAT_CastSign,
	EBAT_ItemUse,  		//this includes bombs, traps, etc.
	EBAT_Parry,
	EBAT_Dodge,			//this includes jump, dodge, evade etc.
	EBAT_SpecialAttack_Light,
	EBAT_SpecialAttack_Heavy,
	EBAT_Roll,
	EBAT_Ciri_SpecialAttack,		
	EBAT_Ciri_SpecialAttack_Heavy,	
	EBAT_Ciri_Counter,				
	EBAT_Ciri_Dodge,				
	EBAT_Draw_Steel,				
	EBAT_Draw_Silver,				
	EBAT_Sheathe_Sword,				
};

BEGIN_ENUM_RTTI( EBufferActionType );
	ENUM_OPTION( EBAT_EMPTY );
	ENUM_OPTION( EBAT_LightAttack );
	ENUM_OPTION( EBAT_HeavyAttack );
	ENUM_OPTION( EBAT_CastSign );
	ENUM_OPTION( EBAT_ItemUse );
	ENUM_OPTION( EBAT_Parry );
	ENUM_OPTION( EBAT_Dodge );
	ENUM_OPTION( EBAT_SpecialAttack_Light );
	ENUM_OPTION( EBAT_SpecialAttack_Heavy );
	ENUM_OPTION( EBAT_Roll );
	ENUM_OPTION( EBAT_Ciri_SpecialAttack );
	ENUM_OPTION( EBAT_Ciri_SpecialAttack_Heavy );
	ENUM_OPTION( EBAT_Ciri_Counter );
	ENUM_OPTION( EBAT_Ciri_Dodge );
	ENUM_OPTION( EBAT_Draw_Steel );
	ENUM_OPTION( EBAT_Draw_Silver );
	ENUM_OPTION( EBAT_Sheathe_Sword );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ECombatActionType
{
	CAT_Attack,
	CAT_SpecialAttack,
	CAT_Dodge,
	CAT_Roll,
	CAT_ItemThrow,
	CAT_LayItem,
	CAT_CastSign,
	CAT_Pirouette,
	CAT_PreAttack,
	CAT_Parry,
	CAT_Crossbow,
	CAT_None2,
	CAT_CiriDodge,
};

BEGIN_ENUM_RTTI( ECombatActionType );
	ENUM_OPTION( CAT_Attack );
	ENUM_OPTION( CAT_SpecialAttack );
	ENUM_OPTION( CAT_Dodge );
	ENUM_OPTION( CAT_Roll );
	ENUM_OPTION( CAT_ItemThrow );
	ENUM_OPTION( CAT_LayItem );
	ENUM_OPTION( CAT_CastSign );
	ENUM_OPTION( CAT_Pirouette );
	ENUM_OPTION( CAT_PreAttack );
	ENUM_OPTION( CAT_Parry );
	ENUM_OPTION( CAT_Crossbow );
	ENUM_OPTION( CAT_None2 );
	ENUM_OPTION( CAT_CiriDodge );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EOrientationTarget
{
	OT_Player,
	OT_Actor,
	OT_CustomHeading,
	OT_Camera,
	OT_CameraOffset,
	OT_None,
};

BEGIN_ENUM_RTTI( EOrientationTarget );
	ENUM_OPTION( OT_Player );
	ENUM_OPTION( OT_Actor );
	ENUM_OPTION( OT_CustomHeading );
	ENUM_OPTION( OT_Camera );
	ENUM_OPTION( OT_CameraOffset );
	ENUM_OPTION( OT_None );
END_ENUM_RTTI();
