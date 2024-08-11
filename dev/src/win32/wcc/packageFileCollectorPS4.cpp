/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/redSystem/crt.h"
#include "../../common/core/stringConversion.h"

#include "packageFileCollectorPS4.h"
#include "packageConstants.h"
#include "packageFiles.h"

static const Char FORBIDDEN_CHARS[] = { TXT('*'), TXT(':'), TXT(';'), TXT('?'), TXT('\"'), TXT('<'), TXT('>'), TXT('|'), TXT('/'), TXT('\\'), };

CPackageFileCollectorPS4::CPackageFileCollectorPS4( EPackageType packageType )
	: TBaseClass( packageType )
{
	const Char* EXTRA_EXTENSIONS_TO_IGNORE[] = { TXT("exe"), TXT("dll") };

	TDynArray< String > extraExtensionsToIngore;
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(EXTRA_EXTENSIONS_TO_IGNORE); ++i )
	{
		extraExtensionsToIngore.PushBack( EXTRA_EXTENSIONS_TO_IGNORE[i] );
	}

	AddExtensionsToIgnore( extraExtensionsToIngore );
}

Bool CPackageFileCollectorPS4::ProcessDirectory( const String& normalizedProjectAbsolutePath, const String& dirPath, SPackageFiles& outPackageFile ) const 
{
	if ( dirPath == TXT("sce_sys\\"))
	{
		return AddSystemDirectory( normalizedProjectAbsolutePath, dirPath, outPackageFile );
	}
	else if ( dirPath == TXT("sce_module\\"))
	{
		return AddGameBinaryDirectory( normalizedProjectAbsolutePath, dirPath, outPackageFile );
	}
	
	return TBaseClass::ProcessDirectory( normalizedProjectAbsolutePath, dirPath, outPackageFile );
}
