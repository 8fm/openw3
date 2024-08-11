/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wildcard.h"

CWildcard::CWildcard( const String &wildcard )
	: m_wildcard( wildcard )
{
}

Bool CWildcard::Matches( const String &str ) const
{
	unsigned int i = 0;		// wildcard iterator
	unsigned int j = 0;		// string iterator

	Bool inAnyChar = false;

	while( i < m_wildcard.GetLength() && j < str.GetLength() )
	{
		if ( m_wildcard[i] == TXT('?') )
		{
			// ? matches any single character
			++i;
			++j;
		} else
		if ( m_wildcard[i] == TXT('*') )
		{
			// * matches multiple characters
			if ( inAnyChar )
			{
				if ( ( i + 1 < m_wildcard.GetLength() ) &&
					 ( m_wildcard[ i + 1 ] == str[ j ] ) )
				{
					inAnyChar = false;
					i += 2;
					++j;
				}
				else
				{
					++j;
				}
			}
			else
			{
				inAnyChar = true;
				++j;					// advance in string
			}
		} else
		{
			// regular character, must match
			if ( m_wildcard[i] != str[j] )
				return false;

			++i;
			++j;
		}
	}

	// return true if we processed both strings or we reached end of string while processing *
	return ( ( i == m_wildcard.GetLength() ) && ( j == str.GetLength() ) ) ||
		   ( ( j == str.GetLength() ) && inAnyChar );
}
