/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameParams.h"

IMPLEMENT_ENGINE_CLASS( W3GameParams );

//////////////////////////////////////////////////////////////////////////

Float W3GameParams::GetFloatParam( CName paramName )
{
	return GetParam( paramName, m_floatParams );
}

Int32 W3GameParams::GetIntParam( CName paramName )
{
	return GetParam( paramName, m_intParams );
}

CName W3GameParams::GetNameParam( CName paramName )
{
	return GetParam( paramName, m_nameParams );
}
