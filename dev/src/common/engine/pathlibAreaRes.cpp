/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibAreaRes.h"

namespace PathLib
{
///////////////////////////////////////////////////////////////////////////////
// CAreaRes
///////////////////////////////////////////////////////////////////////////////
Bool CAreaRes::VHasChanged() const
{
	return true;
}
Bool CAreaRes::VSave( const String& depotPath ) const
{
	return false;
}

void CAreaRes::VOnPreLoad( CAreaDescription* area )
{

}

void CAreaRes::VOnPreUnload( CAreaDescription* area )
{

}

Bool CAreaRes::VLoad( const String& depotPath, CAreaDescription* area )
{
	return false;
}

void CAreaRes::VOnPostLoad( CAreaDescription* area )
{

}



};			// namespace PathLib