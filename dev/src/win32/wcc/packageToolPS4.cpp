/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "packageToolPS4.h"
#include "processRunner.h"

CPackageToolPS4::CPackageToolPS4(const String& exeAbsolutePath )
	: TBaseClass( exeAbsolutePath )
{
}

Bool CPackageToolPS4::CompressFile( const String& sourceAbsolutePath, const String& destAbsolutePath ) const
{
	const String commandLine = String::Printf(TXT("file_compress \"%ls\" \"%ls\""), sourceAbsolutePath.AsChar(), destAbsolutePath.AsChar() );
	return RunExe( commandLine );
}

Bool CPackageToolPS4::Version() const
{
	const String commandLine(TXT("version"));
	return RunExe( commandLine );
}

Bool CPackageToolPS4::CreateProject( const String& appRootAbsolutePath, const String& batchFileAbsolutePath, const String& gp4projectAbsolutePath ) const
{
	const String commandLine = String::Printf(TXT("gp4_batch \"%ls\" \"%ls\""), batchFileAbsolutePath.AsChar(), gp4projectAbsolutePath.AsChar());
	return RunExe( commandLine, appRootAbsolutePath );
}

Bool CPackageToolPS4::CreatePackage( const String& gp4projectAbsolutePath, const String& destAbsolutePath, Uint32 packageFormatFlags /*= ePackageFormatFlags_Package*/, Uint32 packageFormatOptions /*= ePackageFormatOptions_None*/ ) const
{
	// The img_create tool isn't very smart
	String normalizedDestAbsolutePath = destAbsolutePath;
	if ( normalizedDestAbsolutePath.EndsWith(TXT("\\")) )
	{
		normalizedDestAbsolutePath[ normalizedDestAbsolutePath.GetLength()-1] = TXT('\0');
	}

	// Valid format combinations
	const Uint32 packageOnly = ePackageFormatFlags_Package;
	const Uint32 packageSubmissionMaterials = ePackageFormatFlags_Package|ePackageFormatFlags_SubmissionMaterials;
	const Uint32 packageIso = ePackageFormatFlags_Package|ePackageFormatFlags_ISO;
	const Uint32 packageIsoSubmissionMaterials = ePackageFormatFlags_Package|ePackageFormatFlags_ISO|ePackageFormatFlags_SubmissionMaterials;
	
	String pkgFormat;
	if ( (packageFormatFlags & packageIsoSubmissionMaterials) == packageIsoSubmissionMaterials )
	{
		pkgFormat = TXT("pkg+iso+subitem");
	}
	else if ( (packageFormatFlags & packageSubmissionMaterials) == packageSubmissionMaterials )
	{
		pkgFormat = TXT("pkg+subitem");
	}
	else if ( (packageFormatFlags & packageIso) == packageIso )
	{
		pkgFormat = TXT("pkg+iso");
	}
	else if ( (packageFormatFlags & packageOnly) == packageOnly )
	{
		pkgFormat = TXT("pkg");
	}
	else
	{
		ERR_WCC(TXT("CPackageToolPS4::CreatePackage: Invalid EPackageFormatFlags combination 0x%08X. Valid combinations:"), packageFormatFlags );
		ERR_WCC(TXT("\tPackageFormatFlags_Package"));
		ERR_WCC(TXT("\tPackageFormatFlags_Package|ePackageFormatFlags_ISO"));
		ERR_WCC(TXT("\tPackageFormatFlags_Package|ePackageFormatFlags_SubmissionMaterials"));
		ERR_WCC(TXT("\tPackageFormatFlags_Package|ePackageFormatFlags_ISO|ePackageFormatFlags_SubmissionMaterials"));
		return false;
	}

	String pkgOpts;
	if ( (packageFormatFlags & ePackageFormatFlags_ISO) && (packageFormatOptions & ePackageFormatOptions_MoveOuter) )
	{
		pkgOpts += TXT("--move_outer ");
	}
	if ( !(packageFormatFlags & ePackageFormatFlags_SubmissionMaterials) && (packageFormatOptions & ePackageFormatOptions_SkipDigest) )
	{
		pkgOpts += TXT("--skip_digest ");
	}

	const String commandLine = String::Printf(TXT("img_create --oformat %ls %ls \"%ls\" \"%ls\""), pkgFormat.AsChar(), pkgOpts.AsChar(), gp4projectAbsolutePath.AsChar(), normalizedDestAbsolutePath.AsChar());
	LOG_WCC(TXT("Running command '%ls'"), commandLine.AsChar() );
	return RunExe( commandLine );
}


Bool CPackageToolPS4::ExtractFile( const String& imgAbsolutePath, const String& packageFilePath, const String& destAbsolutePath, const String& passcode )
{
	String normalizedPackageFilePath = packageFilePath;
	normalizedPackageFilePath.ReplaceAll(TXT('\\'),TXT('/'));
	const Char* roots[] = { TXT("Disc0"), TXT("Image0") };

	Bool ret = false;
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(roots); ++i )
	{
		const Char* imgRoot = roots[i];
		const String commandLine = String::Printf(TXT("img_extract --passcode %ls \"%ls:%ls/%ls\" \"%ls\""), 
			passcode.AsChar(), imgAbsolutePath.AsChar(), imgRoot, normalizedPackageFilePath.AsChar(), destAbsolutePath.AsChar() );
		if ( RunExe( commandLine ) )
		{
			LOG_WCC(TXT("Extracted %ls under root %ls"), normalizedPackageFilePath.AsChar(), imgRoot);
			ret = true;
			break;
		}
	}

	return ret;
}
