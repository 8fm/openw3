/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "hitProxyMap.h"
#include "hitProxyObject.h"

CHitProxyMap::CHitProxyMap()
	: m_counter( 1 )
{
}

CHitProxyMap::~CHitProxyMap()
{
	// Delete proxies
	m_map.ClearPtr();
}

CHitProxyID CHitProxyMap::RegisterHitProxy( CHitProxyObject *proxy )
{
	ASSERT( proxy );
	ASSERT( !FindHitProxy( proxy->GetID() ) );

	// Allocate unique index
	Uint32 proxyIndex = m_counter++;

	// Add proxy to proxy map
	proxy->BindToMap( *this, CHitProxyID( proxyIndex ) );
	m_map.Insert( proxyIndex, proxy );

	// Return mapped
	return CHitProxyID( proxyIndex );
}

CHitProxyObject *CHitProxyMap::FindHitProxy( const CHitProxyID& id )
{
	CHitProxyObject *hitProxy = NULL;
	m_map.Find( id.GetID(), hitProxy );
	return hitProxy;
}