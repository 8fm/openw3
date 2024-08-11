/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "rangedWeapon.h"

IMPLEMENT_ENGINE_CLASS( RangedWeapon );

RED_DEFINE_STATIC_NAME( ClearDeployedEntity );

void RangedWeapon::OnDetached( CWorld* world )
{
	CallFunction( this, CNAME( ClearDeployedEntity ), true );
	TBaseClass::OnDetached( world );
}