/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/redSystem/crt.h"
#include "../../common/core/stringConversion.h"

#include "packageFileCollectorXboxOne.h"
#include "packageConstants.h"
#include "packageFiles.h"

static const Char FORBIDDEN_CHARS[] = { TXT('*'), TXT(':'), TXT(';'), TXT('?'), TXT('\"'), TXT('<'), TXT('>'), TXT('|'), TXT('/'), TXT('\\'), };

CPackageFileCollectorXboxOne::CPackageFileCollectorXboxOne( EPackageType packageType )
	: TBaseClass( packageType )
{
	// Keep resources.pri
	const Char* EXTRA_EXTENSIONS_TO_IGNORE[] = { TXT("elf"), TXT("prx"), TXT("appxrecipe"), TXT("winmd"), TXT("exp") };

	TDynArray< String > extraExtensionsToIngore;
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(EXTRA_EXTENSIONS_TO_IGNORE); ++i )
	{
		extraExtensionsToIngore.PushBack( EXTRA_EXTENSIONS_TO_IGNORE[i] );
	}

	AddExtensionsToIgnore( extraExtensionsToIngore );
}

Bool CPackageFileCollectorXboxOne::ProcessDirectory( const String& normalizedProjectAbsolutePath, const String& dirPath, SPackageFiles& outPackageFile ) const 
{
	if ( !dirPath.BeginsWith( TXT("Manifest") ) )
	{
		return TBaseClass::ProcessDirectory( normalizedProjectAbsolutePath, dirPath, outPackageFile );
	}
	else
	{
		return AddGameBinaryDirectory( normalizedProjectAbsolutePath, dirPath, outPackageFile );
	}
}
