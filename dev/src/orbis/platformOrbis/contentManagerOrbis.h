/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/contentManager.h"

#include <app_content.h>

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CDlcInstallerOrbis;

//////////////////////////////////////////////////////////////////////////
// CContentManagerOrbis
//////////////////////////////////////////////////////////////////////////
class CContentManagerOrbis : public CContentManager
{
private:
	typedef CContentManager TBaseClass;

public:
											CContentManagerOrbis();
	virtual									~CContentManagerOrbis();
	virtual Bool							Init() override;

private:
	void									Shutdown();
	Bool									InitAppContentLib();
};
