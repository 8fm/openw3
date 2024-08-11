/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/redSystem/crt.h"
#include "../../common/core/stringConversion.h"

#include "packageFileCollector.h"
#include "packageConstants.h"
#include "packageFiles.h"

static const Char FORBIDDEN_CHARS[] = { TXT('*'), TXT(':'), TXT(';'), TXT('?'), TXT('\"'), TXT('<'), TXT('>'), TXT('|'), TXT('/'), TXT('\\'), };

CPackageFileCollector::CPackageFileCollector( EPackageType packageType )
	: m_packageType( packageType )
{
	const Char* const EXTENSIONS_TO_IGNORE[] = { TXT("fx"), TXT("pdb"), TXT("a"), TXT("lib"), TXT("pri"), TXT("lnk"), TXT("db"), TXT("gp4"), TXT("iso"), TXT("ws"), TXT("manifest"), TXT("log"), TXT("bat") };
	TDynArray< String > extensionsToIngore;
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(EXTENSIONS_TO_IGNORE); ++i )
	{
		m_extensionsToIgnore.PushBack( EXTENSIONS_TO_IGNORE[i] );
	}
}

void CPackageFileCollector::AddExtensionsToIgnore( const TDynArray< String >& extensions )
{
	m_extensionsToIgnore.PushBack( extensions );
}

Bool CPackageFileCollector::CollectPackageFiles( const String& projectAbsolutePath, SPackageFiles& outPackageFiles ) const
{
	String normalizedProjectAbsolutePath = projectAbsolutePath.ToLower();
	normalizedProjectAbsolutePath.ReplaceAll(TXT('/'), TXT('\\'));
	if ( !normalizedProjectAbsolutePath.EndsWith(TXT("\\")) )
	{
		normalizedProjectAbsolutePath += TXT('\\');
	}

	// TODO: Should validate contents aren't missing or extra unallowed stuff (not .prx in sce_module)
	// Or prx in bin and already in sce_sys. Shouldn't have DLLs or PRX files in game content dirs
	SPackageFiles newPackageFiles;
	TDynArray< String > dirs;
	GFileManager->FindDirectories( normalizedProjectAbsolutePath, dirs );

	for ( String& dir : dirs )
	{
		dir += TXT("\\");

		if ( ! ProcessDirectory( normalizedProjectAbsolutePath, dir, newPackageFiles ) )
		{
			return false;
		}
	}

	outPackageFiles = Move( newPackageFiles );

	return true;	
}

static Bool ParsePatchNumber( const Char* patchName, Uint32& outPatchNumber )
{
	const Char* patchPostfix = patchName + Red::System::StringLength(PATCH_NAME_PREFIX);
	if ( !GParseInteger( patchPostfix, outPatchNumber ) )
	{
		ERR_WCC(TXT("Patch dir '%ls' must end with a content number"), patchName);
		return false;
	}

	return true;
}

static Bool ParseContentNumber( const Char* contentName, Uint32& outContentNumber )
{
	const Char* contentPostfix = contentName + Red::System::StringLength(CONTENT_NAME_PREFIX);
	if ( !GParseInteger( contentPostfix, outContentNumber ) )
	{
		ERR_WCC(TXT("Content dir '%ls' must end with a content number"), contentName);
		return false;
	}

	return true;
}

Bool CPackageFileCollector::ProcessDirectory( const String& normalizedProjectAbsolutePath, const String& dirPath, SPackageFiles& outPackageFile ) const
{
	if ( dirPath.BeginsWith(GAME_BIN_DIR) )
	{
		if ( ! AddGameBinaryDirectory( normalizedProjectAbsolutePath, dirPath, outPackageFile ) )
		{
			return false;
		}
	}
	else if ( dirPath.BeginsWith(CONTENT_DIR) )
	{
		TDynArray< String > contentDirs;
		const String& contentAbsolutePath = normalizedProjectAbsolutePath + dirPath;

		// Kind of a hack
		String metadataStoreFilePath = contentAbsolutePath + TXT("metadata.store");
		if ( GFileManager->FileExist( metadataStoreFilePath ) )
		{
			String localPath;
			if ( ConvertToLocalProjectPath( normalizedProjectAbsolutePath, metadataStoreFilePath, localPath ) )
			{
				String tempPath;
				metadataStoreFilePath = Move( CFilePath::ConformPath( localPath, tempPath ) );
				outPackageFile.m_gameBinFiles.PushBack( Move( metadataStoreFilePath ) );
			}
		}
		
		// Allow patch directories to replace content files, for those ones that are unaware of the patch mechanism. E.g., static shader cache, scripts, etc
		if ( m_packageType == ePackageType_App || m_packageType == ePackageType_Patch )
		{
			GFileManager->FindDirectories( contentAbsolutePath, contentDirs );
			for ( const String& contentDir : contentDirs )
			{
				const String& contentName = contentDir;
				const String contentDepotPath = dirPath + contentDir + TXT("\\");
				if ( contentName.BeginsWith( CONTENT_NAME_PREFIX ) )
				{
					Uint32 contentNumber = 0;
					if ( !ParseContentNumber(contentName.AsChar(), contentNumber) )
					{
						return false;
					}
					if ( ! AddGameContentDirectory( contentNumber, normalizedProjectAbsolutePath, contentDepotPath, outPackageFile ) )
					{
						return false;
					}
				}
				else if ( m_packageType == ePackageType_Patch && contentName.BeginsWith( PATCH_NAME_PREFIX ) )
				{
					Uint32 patchNumber = 0;
					if ( !ParsePatchNumber(contentName.AsChar(), patchNumber) )
					{
						return false;
					}
					if ( ! AddGamePatchDirectory( patchNumber, normalizedProjectAbsolutePath, contentDepotPath, outPackageFile ) )
					{
						return false;
					}
				}
				else
				{
					ERR_WCC(TXT("Content dir '%ls' {%ls} must start with prefix '%ls'"), contentDir.AsChar(), (normalizedProjectAbsolutePath+contentDir).AsChar(), CONTENT_NAME_PREFIX);
					return false;
				}
			}
		}
		else if ( m_packageType == ePackageType_Dlc )
		{
			// Almost like an app layout, only there's no content0, content1 etc, all data starts in content
			// E.g.,
			/*
			    [addcont0] <- mount point
				\sce_sys\*
				\content
				\content\metadata.store
				\content\bundles\*.bundle
				\content\*.cache
				\content\*.w3speech
				etc...
			*/
			Uint32 contentNumber = 0;
			const String contentDepotPath = dirPath;
			if ( ! AddGameContentDirectory( contentNumber, normalizedProjectAbsolutePath, contentDepotPath, outPackageFile ) )
			{
				return false;
			}
		}
		else
		{
			ERR_WCC(TXT("Unhandled package type %u"), m_packageType );
			return false;
		}
	}
	else if ( !dirPath.BeginsWith(TXT("sce_") ) )
	{
		WARN_WCC(TXT("Adding unknown dir '%ls' {%ls} to default content%u"), dirPath.AsChar(), (normalizedProjectAbsolutePath + dirPath).AsChar(), DEFAULT_CONTENT_NAME_NUMBER );
		if ( ! AddGameContentDirectory( DEFAULT_CONTENT_NAME_NUMBER, normalizedProjectAbsolutePath, dirPath, outPackageFile ) )
		{
			return false;
		}
		return true;
	}
	else
	{
		ERR_WCC(TXT("Unhandled directory '%ls'"), dirPath.AsChar() );
		return false;
	}

	return true;
}

Bool CPackageFileCollector::AddGamePatchDirectory( Uint32 patchNumber, const String& projectAbsolutePath, const String& depotPath, SPackageFiles& outPackageFiles ) const
{
	if ( depotPath.BeginsWith(TXT("sce_")) || depotPath.BeginsWith(TXT("eboot")) || depotPath.BeginsWith(TXT("standby_screen")) )
	{
		ERR_WCC(TXT("Cannot add reserved SCE directory as patch: patch%u: '%ls'"), patchNumber, depotPath.AsChar());
		return false;
	}

	TDynArray< String > newFiles;
	if ( ! PopulateFilesFromDirectory( projectAbsolutePath, depotPath, newFiles ) )
	{
		return false;
	}

	for ( Int32 j = newFiles.SizeInt()-1; j >= 0; --j )
	{
		const String& file = newFiles[ j ];
		if ( file.EndsWith( SYS_PRX_EXT ) )
		{
			ERR_WCC(TXT("PRX '%ls' can't go in game patch patch%u"), file.AsChar(), patchNumber );
			return false;
		}
		else if ( file.EndsWith( SYS_ELF_EXT ) )
		{
			ERR_WCC(TXT("ELF '%ls' can't go in game patch patch%u"), file.AsChar(), patchNumber );
			return false;
		}
	}

	TDynArray<String>& patchFiles = outPackageFiles.m_gamePatchFilesMap.GetRef( patchNumber );
	patchFiles.PushBack( newFiles );

	return true;
}

Bool CPackageFileCollector::AddGameContentDirectory( Uint32 contentNumber, const String& projectAbsolutePath, const String& depotPath, SPackageFiles& outPackageFiles ) const
{
	if ( depotPath.BeginsWith(TXT("sce_")) || depotPath.BeginsWith(TXT("eboot")) || depotPath.BeginsWith(TXT("standby_screen")) )
	{
		ERR_WCC(TXT("Cannot add reserved SCE directory as content: content%u: '%ls'"), contentNumber, depotPath.AsChar());
		return false;
	}

	TDynArray< String > newFiles;
	if ( ! PopulateFilesFromDirectory( projectAbsolutePath, depotPath, newFiles ) )
	{
		return false;
	}

	for ( Int32 j = newFiles.SizeInt()-1; j >= 0; --j )
	{
		const String& file = newFiles[ j ];
		if ( file.EndsWith( SYS_PRX_EXT ) )
		{
			ERR_WCC(TXT("PRX '%ls' can't go in game content content%u"), file.AsChar(), contentNumber );
			return false;
		}
		else if ( file.EndsWith( SYS_ELF_EXT ) )
		{
			ERR_WCC(TXT("ELF '%ls' can't go in game content content%u"), file.AsChar(), contentNumber );
			return false;
		}
	}

	SPackageGameFiles& gameFiles = outPackageFiles.m_gameContentFilesMap.GetRef( contentNumber );
	MoveGameFilesByExtension( newFiles, gameFiles );

	return true;
}

Bool CPackageFileCollector::AddGameBinaryDirectory( const String& projectAbsolutePath, const String& depotPath, SPackageFiles& outPackageFiles ) const
{
// 	if ( depotPath.BeginsWith(TXT("sce_")) || depotPath.BeginsWith(TXT("eboot")) || depotPath.BeginsWith(TXT("standby_screen")) )
// 	{
// 		ERR_WCC(TXT("Cannot add reserved SCE directory as bin: '%ls'"), depotPath.AsChar());
// 		return false;
// 	}

	TDynArray< String > newFiles;
	if ( ! PopulateFilesFromDirectory( projectAbsolutePath, depotPath, newFiles ) )
	{
		return false;
	}

	for ( Int32 j = newFiles.SizeInt()-1; j >= 0; --j )
	{
		const String& file = newFiles[ j ];
		if ( file.EndsWith( SYS_PRX_EXT ) || file.EndsWith( SYS_DLL_EXT ) )
		{
			outPackageFiles.m_dynLibFiles.PushBack( Move( file ) );
			newFiles.RemoveAt( j );
		}
		else if ( file.EndsWith( SYS_ELF_EXT ) || file.EndsWith( SYS_EXE_EXT ) )
		{
			outPackageFiles.m_exeFiles.PushBack( Move( file ) );
			newFiles.RemoveAt( j );
		}
	}

	outPackageFiles.m_gameBinFiles.PushBack( Move( newFiles ) );

	return true;
}

Bool CPackageFileCollector::AddSystemDirectory( const String& projectAbsolutePath, const String& depotPath, SPackageFiles& outPackageFiles ) const
{
	TDynArray< String > newFiles;
	if ( ! PopulateFilesFromDirectory( projectAbsolutePath, depotPath, newFiles ) )
	{
		return false;
	}

	for ( Uint32 i = 0 ; i < newFiles.Size(); ++i )
	{
		const String& file = newFiles[ i ];
		if ( file.EndsWith( SYS_PRX_EXT ) )
		{
			ERR_WCC(TXT("PRX '%ls' can't go in system directory"), file.AsChar() );
			return false;
		}
		else if ( file.EndsWith( SYS_ELF_EXT ) )
		{
			ERR_WCC(TXT("ELF '%ls' can't go in system directory"), file.AsChar() );
			return false;
		}
	}

	outPackageFiles.m_sysFiles.PushBack( Move( newFiles ) );

	return true;
}

Bool CPackageFileCollector::PopulateFilesFromDirectory( const String& projectAbsolutePath, const String& depotPath, TDynArray< String >& outFiles ) const
{
	const String absolutePath = projectAbsolutePath + depotPath;
	TDynArray< String > files;
	GFileManager->FindFiles( absolutePath, TXT("*.*"), files, true );
	for ( Int32 j = files.SizeInt()-1; j >= 0; --j )
	{
		String& file = files[j];
		String localPath;
		if ( ! ConvertToLocalProjectPath( projectAbsolutePath, file, localPath ) )
		{
			return false;
		}

		String tempPath;
		file = Move( CFilePath::ConformPath( localPath, tempPath ) );

		// Remove before validation
		const String ext = CFilePath( file ).GetExtension();
		if ( m_extensionsToIgnore.Exist( ext ) )
		{
			files.RemoveAt( j );
			continue;
		}
		
		if ( !ValidateLocalAppPath(file) )
		{
			return false;
		}
	}
	outFiles.PushBack( Move(files) );

	return true;
}

Bool CPackageFileCollector::ConvertToLocalProjectPath( const String& projectAbsolutePath, const String& absolutePath, String& localPath ) const
{
	// Strip absolute path of this directory
	StringBuffer<512> absolutePathBuf( absolutePath );

	StringBuffer<512> directoryAbsolutePathBuf( projectAbsolutePath.AsChar() );

	absolutePathBuf.ToLower();
	directoryAbsolutePathBuf.ToLower();
	absolutePathBuf[ directoryAbsolutePathBuf.Size() ] = 0;

	// find if it begins with 
	if ( absolutePathBuf == directoryAbsolutePathBuf )
	{
		localPath = absolutePath.ToLower().StringAfter( projectAbsolutePath.ToLower() );
		return true;
	}

	// Not a local path
	ERR_WCC(TXT("Failed to convert path '%ls' to local app path"), absolutePath.AsChar() );
	return false;
}

Bool CPackageFileCollector::ValidateLocalAppPath( const String& path ) const
{
	if ( path.GetLength() > MAX_PATH_LENGTH )
	{
		ERR_WCC(TXT("Entire path '%ls' is too long for the PS4 (length %u, max %u)'"), path.AsChar(), path.GetLength(), MAX_PATH_LENGTH );
		return false;
	}

	if ( path.Empty() )
	{
		ERR_WCC(TXT("Entire path is empty"));
		return false;
	}

	CFilePath filePath( path );
	// Not sure why this doesn't return a const ref
	TDynArray< String > dirs = filePath.GetDirectories();
	const Uint32 dirDepth = dirs.Size() + (filePath.HasFilename() ? 1 : 0);
	if ( dirDepth > MAX_DIR_DEPTH )
	{
		ERR_WCC(TXT("Path '%ls' with depth %u exceeds the %u level directory depth"), path.AsChar(), dirDepth, MAX_DIR_DEPTH );
		return false;
	}

	if ( filePath.HasFilename() )
	{
		if ( ! ValidatePathComponent( filePath.GetFileNameWithExt() ) )
		{
			return false;
		}
	}

	for ( const String& dir : dirs )
	{
		if ( dir.GetLength() > MAX_ENTRY_LENGTH )
		{
			ERR_WCC(TXT("Dir '%ls' is too long for the PS4 (length %u, max %u) in path '%ls'"), dir.AsChar(), dir.GetLength(), MAX_ENTRY_LENGTH );
			return false;
		}
		if ( ! ValidatePathComponent( dir ) )
		{
			return false;
		}
	}

	const String fileNameWithExt = filePath.GetFileNameWithExt();
	if ( fileNameWithExt.GetLength() > MAX_ENTRY_LENGTH )
	{
		ERR_WCC(TXT("File '%ls' is too long for the PS4 (length %u, max %u) in path '%ls'"), fileNameWithExt.AsChar(), fileNameWithExt.GetLength(), MAX_ENTRY_LENGTH );
		return false;
	}

	return true;
}

Bool CPackageFileCollector::ValidatePathComponent( const String& pathComponent ) const
{
	for ( const Char* pch = pathComponent.AsChar(); *pch; ++pch )
	{
		const Char ch = *pch;
		if ( (Uint32)ch >= 0x7F || ((Uint32)ch >= 0x0 && (Uint32)ch <= 0x1F) )
		{
			ERR_WCC(TXT("Path component '%ls' contains illegal character '%lc'"), pathComponent.AsChar(), ch );
			return false;
		}
		for ( Uint32 j = 0; j < ARRAY_COUNT_U32(FORBIDDEN_CHARS); ++j )
		{
			if ( ch == FORBIDDEN_CHARS[j] )
			{
				ERR_WCC(TXT("Path component contains illegal character '%lc'"), pathComponent.AsChar(), ch );
				return false;
			}
		}
	}

	return true;
}

void CPackageFileCollector::MoveGameFilesByExtension( TDynArray< String >& inFiles, SPackageGameFiles& outGameFiles ) const
{
	for ( Int32 j = inFiles.SizeInt()-1; j >= 0; --j )
	{
		const String& file = inFiles[ j ];

		// FIXME: inconsistent but in non-split cook .store goes into game content directory vs bin
		if ( file.EndsWith(TXT(".bundle")) || file.EndsWith(TXT(".store")) )
		{
			outGameFiles.m_bundleFiles.PushBack( Move( file ) );
		}
		else if ( file.EndsWith(TXT(".redscripts")) )
		{
			outGameFiles.m_scriptFiles.PushBack( Move( file ) );
		}
		else if ( file.EndsWith(TXT(".cache")) )
		{
			outGameFiles.m_cacheFiles.PushBack( Move( file ) );
		}
		else if ( file.EndsWith(TXT(".w3speech")) )
		{
			outGameFiles.m_speechFiles.PushBack( Move( file ) );
		}
		else if ( file.EndsWith(TXT(".w3strings")) )
		{
			outGameFiles.m_stringsFiles.PushBack( Move( file ) );
		}
		else
		{
			outGameFiles.m_miscFiles.PushBack( Move( file ) );
		}
		inFiles.RemoveAt( j );
	}

	RED_FATAL_ASSERT( inFiles.Empty(), "Missed processing some files!" );
}
