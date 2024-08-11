/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/object.h"

class CR6TelemetryScriptProxy : public CObject
{
	DECLARE_ENGINE_CLASS( CR6TelemetryScriptProxy, CObject, 0 );
};

BEGIN_CLASS_RTTI( CR6TelemetryScriptProxy )
	PARENT_CLASS( CObject )
END_CLASS_RTTI();