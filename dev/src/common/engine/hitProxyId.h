/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/math.h"

/// RGB encoded hit ID
class CHitProxyID
{
protected:
	Uint32	m_index;		// Object ID

public:
	RED_INLINE explicit CHitProxyID( Uint32 id = 0):	m_index( id ) {}
	CHitProxyID( const Color &color );
	CHitProxyID( const CHitProxyID &id );

	// Assignment
	RED_INLINE CHitProxyID& operator=( const CHitProxyID& other )
	{
		m_index = other.m_index;
		return *this;
	}

	// Comparison
	RED_INLINE Bool operator==( const CHitProxyID& other ) const
	{
		return m_index == other.m_index;
	}

	// Get hit proxy id index
	RED_INLINE Uint32 GetID() const { return m_index; }

	// Get RGB code
	RED_INLINE Color GetColor() const { return Color( (Uint8)(m_index & 0xFF), (Uint8)((m_index >> 8) & 0xFF), (Uint8)(( m_index >> 16 ) & 0xFF), (Uint8)(( m_index >> 24 ) & 0xFF) ); }

	RED_FORCE_INLINE Uint32 CalcHash() const { return m_index; }
};