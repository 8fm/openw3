/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "packageTool.h"

enum EPackageFormatFlags
{
	ePackageFormatFlags_None				= 0,
	ePackageFormatFlags_Package				= FLAG(0),
	ePackageFormatFlags_ISO					= FLAG(1),
	ePackageFormatFlags_SubmissionMaterials	= FLAG(2),
};

enum EPackageFormatOptions
{
	ePackageFormatOptions_None					= 0,
	ePackageFormatOptions_MoveOuter				= FLAG(0),
	ePackageFormatOptions_SkipDigest			= FLAG(1),
};

class CPackageToolPS4 : public CPackageTool
{
private:
	typedef CPackageTool TBaseClass;

public:
	Bool Version() const;
	Bool CompressFile( const String& sourceAbsolutePath, const String& destAbsolutePath ) const;
	Bool CreateProject( const String& appRootAbsolutePath, const String& batchFileAbsolutePath, const String& gp4projectAbsolutePath ) const;
	Bool CreatePackage( const String& gp4projectAbsolutePath, const String& destAbsolutePath, Uint32 packageFormatFlags = ePackageFormatFlags_Package, Uint32 packageFormatOptions = ePackageFormatOptions_None ) const;
	Bool ExtractFile( const String& imgAbsolutePath, const String& packageFilePath, const String& destAbsolutePath, const String& passcode );

public:
	CPackageToolPS4( const String& exeAbsolutePath );
};