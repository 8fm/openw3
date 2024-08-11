/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/communityData.h"

class CCommunityValidator
{
public:
	// Validates community resource, returns true if everything is OK.
	// If returned value is false, than error message will be put into 'errorMsg' variable.
	Bool Validate( const CCommunity *community, String &errorMsg /* out */ );

private:
	void AddErrorMsg( const String &errorMsg );
	String GetErrorMsg();

	Bool                m_isValid;
	TDynArray< String > m_errorMessages;
};
