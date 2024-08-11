/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/itemEntity.h"

// PB: sorry for the class name but it was moved directly from the scripts and we cannot change it now due to backward data compatibility

class RangedWeapon : public CItemEntity
{
	DECLARE_ENGINE_CLASS( RangedWeapon, CItemEntity, 0 );

public:

	// Entity was detached from world
	virtual void OnDetached( CWorld* world ) override;
};

BEGIN_CLASS_RTTI( RangedWeapon );
	PARENT_CLASS( CItemEntity );
END_CLASS_RTTI();
