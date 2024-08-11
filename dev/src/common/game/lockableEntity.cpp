/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "lockableEntity.h"

IMPLEMENT_ENGINE_CLASS( W3LockableEntity );

//////////////////////////////////////////////////////////////////////////////////////////////
W3LockableEntity::W3LockableEntity()
	: m_lockedByKey( false )
	, m_isEnabledOnSpawn( true )
{
}

void W3LockableEntity::OnStreamIn()
{
	TBaseClass::OnStreamIn();
	CallEvent( CNAME( OnStreamIn ) );
}