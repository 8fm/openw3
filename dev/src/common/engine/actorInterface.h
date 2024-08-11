/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "actorLod.h"

class CMimicComponent;
class CVisualDebug;

enum ELookAtLevel
{
	LL_Body,
	LL_Head,
	LL_Eyes,
	LL_Null,
};

BEGIN_ENUM_RTTI( ELookAtLevel );
	ENUM_OPTION( LL_Body );
	ENUM_OPTION( LL_Head );
	ENUM_OPTION( LL_Eyes );
	ENUM_OPTION( LL_Null );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EActorAnimState
{
	AAS_Default		= 0,
	AAS_Crossbow	= 1,
	AAS_Bow			= 2,
	AAS_Halberd		= 3,
	AAS_Drunk		= 4,
	AAS_Box			= 5,
	AAS_Broom		= 6,
	AAS_FishBasket	= 7,
	AAS_Torch		= 8,
	AAS_Bucket		= 9,
	AAS_Mage		= 10,
	AAS_Bowl		= 11,
	AAS_Sneak		= 12,
	AAS_Injured		= 13,
	AAS_Pickaxe		= 14,
	AAS_CrossbowAim = 15,
	AAS_Sword		= 16,
	AAS_RocheSword	= 17,
};

BEGIN_ENUM_RTTI( EActorAnimState )
	ENUM_OPTION( AAS_Default )
	ENUM_OPTION( AAS_Crossbow )
	ENUM_OPTION( AAS_Bow )
	ENUM_OPTION( AAS_Halberd )
	ENUM_OPTION( AAS_Drunk )
	ENUM_OPTION( AAS_Box )
	ENUM_OPTION( AAS_Broom )
	ENUM_OPTION( AAS_FishBasket )
	ENUM_OPTION( AAS_Torch )
	ENUM_OPTION( AAS_Bucket )
	ENUM_OPTION( AAS_Mage )
	ENUM_OPTION( AAS_Bowl )
	ENUM_OPTION( AAS_Sneak )
	ENUM_OPTION( AAS_Injured )
	ENUM_OPTION( AAS_Pickaxe )
	ENUM_OPTION( AAS_CrossbowAim )
	ENUM_OPTION( AAS_Sword )
	ENUM_OPTION( AAS_RocheSword )
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////////

enum ESkeletonType : CEnum::TValueType
{
	ST_Man,
	ST_Woman,
	ST_Witcher,
	ST_Dwarf,
	ST_Elf,
	ST_Child,
	ST_Monster,
};

BEGIN_ENUM_RTTI( ESkeletonType );
	ENUM_OPTION_DESC( TXT( "Man" ),		ST_Man );
	ENUM_OPTION_DESC( TXT( "Woman" ),	ST_Woman );
	ENUM_OPTION_DESC( TXT( "Witcher" ),	ST_Witcher );
	ENUM_OPTION_DESC( TXT( "Dwarf" ),	ST_Dwarf );
	ENUM_OPTION_DESC( TXT( "Elf" ),		ST_Elf );
	ENUM_OPTION_DESC( TXT( "Child" ),	ST_Child );
	ENUM_OPTION_DESC( TXT( "Monster" ),	ST_Monster );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

class IActorInterface
{
public:
	virtual ~IActorInterface() {};

	/************************************************************************/
	/* Mimic                                                                */
	/************************************************************************/

	//! Turn on/off mimic high
	virtual Bool MimicOn() =  0;
	virtual void MimicOff() = 0;

	//! Has actor high mimic
	virtual Bool HasMimic() const = 0;

	//! Set mimic variable
	virtual Bool SetMimicVariable( const CName varName, Float value ) = 0;

	//! Is actor in non gameplay scene
	virtual Bool IsInNonGameplayScene() const = 0;

	/************************************************************************/
	/* Head                                                                 */
	/************************************************************************/

	//! Get position of head
	virtual Vector GetHeadPosition() const = 0;

	//! Get head bone
	virtual Int32 GetHeadBone() const = 0;

	//! Get mimic component
	virtual CMimicComponent* GetMimicComponent() const = 0;

	/************************************************************************/
	/* Actor states                                                         */
	/************************************************************************/

	//! Actor's animation state
	virtual Int32 GetActorAnimState() const = 0;

	/************************************************************************/
	/* Look at                                                              */
	/************************************************************************/

	//! Is look at enabled
	virtual Bool IsLookAtEnabled() const = 0;

	//! Get look at level
	virtual ELookAtLevel GetLookAtLevel() const = 0;

	virtual void ActivateLookatFilter( CName key, Bool value ) = 0;

	//! Get look at target in world space
	virtual Vector GetLookAtTarget() const = 0;

	//! Get look at body part weights
	virtual Vector GetLookAtBodyPartsWeights() const = 0;

	//! Get look at data: speed damp, speed follow, autoLimit
	virtual Vector GetLookAtCompressedData() const = 0;

	//! Get eyes look at data: convergence weight, is additive
	virtual Vector GetEyesLookAtCompressedData() const = 0;

	/************************************************************************/
	/* Debug                                                                */
	/************************************************************************/
	
	//! Is actor working
	virtual Bool IsWorking() const = 0;

	//! Is actor in quest scene
	virtual Bool IsInQuestScene() const = 0;

	//! Visual debug
	virtual CVisualDebug* GetVisualDebug() const = 0;

	/************************************************************************/
	/* Hacks                                                                */
	/************************************************************************/

	virtual void Hack_SetSwordsHiddenInGame( Bool /*state*/, Float /*distanceToCamera*/, Float /*cameraYaw*/ ) {}

	virtual void Hack_SetSwordTrajectory( TDynArray< Vector >& /*dataWS*/, Float /*w*/ ) {}
};
