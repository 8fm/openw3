/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "hitProxyId.h"

class CHitProxyObject;

/// Hit proxy object map
class CHitProxyMap
{
protected:
	THashMap< Uint32, CHitProxyObject* >		m_map;		// ID->hit proxy map
	Uint32									m_counter;	// ID counter

public:
	CHitProxyMap();
	~CHitProxyMap();

	// Register hit proxy in map
	CHitProxyID RegisterHitProxy( CHitProxyObject *proxy );

	// Get hit proxy for ID
	CHitProxyObject *FindHitProxy( const CHitProxyID& id );
};
