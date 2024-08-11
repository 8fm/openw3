/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "packageTool.h"

class CPackageToolXboxOne : public CPackageTool
{
private:
	typedef CPackageTool TBaseClass;

public:
	CPackageToolXboxOne( const String& exeAbsolutePath );
};