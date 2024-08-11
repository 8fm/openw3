/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "hitProxyId.h"

CHitProxyID::CHitProxyID( const Color &color )
{
	m_index = color.R + (((Uint32)color.G) << 8) + (((Uint32)color.B) << 16) + (((Uint32)color.A) << 24);
}

CHitProxyID::CHitProxyID( const CHitProxyID &id )
	: m_index( id.m_index )
{
}

