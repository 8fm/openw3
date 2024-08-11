/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "hitProxyObject.h"
#include "hitProxyMap.h"

CHitProxyObject::CHitProxyObject( CObject* object )
	: m_object( object )
{
}

CHitProxyObject::~CHitProxyObject()
{
}

void CHitProxyObject::BindToMap( CHitProxyMap &map, const CHitProxyID& id )
{
	ASSERT( m_id.GetID() == 0 );
	m_id = id;
}
