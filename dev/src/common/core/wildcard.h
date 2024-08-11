/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "string.h"

class CWildcard
{
	String			m_wildcard;
public:
	CWildcard( const String &wildcard );

	Bool	Matches( const String &str ) const;
};