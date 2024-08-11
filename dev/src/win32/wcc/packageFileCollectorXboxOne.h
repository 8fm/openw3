/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "packageFileCollector.h"

struct SPackageFiles;
struct SPackageGameFiles;

class CPackageFileCollectorXboxOne : public CPackageFileCollector
{
private:
	typedef CPackageFileCollector  TBaseClass;
public:
	CPackageFileCollectorXboxOne( EPackageType packageType );

public:
	virtual Bool ProcessDirectory( const String& normalizedProjectAbsolutePath, const String& dirPath, SPackageFiles& outPackageFile ) const override;
};