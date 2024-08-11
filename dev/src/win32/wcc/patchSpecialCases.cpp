/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchUtils.h"
#include "../../common/core/fileSys.h"
#include "../../common/core/dependencyMapper.h"
#include "patchSpecialCases.h"

//-----------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_SpecialCases );

//-----------------------------------------------------------------------------

CPatchSpecialCases::~CPatchSpecialCases()
{
	m_tokens.ClearPtr();
}

void CPatchSpecialCases::GetTokens(TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens) const
{
	outTokens.Reserve( m_tokens.Size() );
	for ( auto* token : m_tokens )
	{
		outTokens.PushBack( token );
	}
}

const Uint64 CPatchSpecialCases::GetDataSize() const
{
	Uint64 totalSize = 0;
	for ( auto* token : m_tokens )
	{
		totalSize += token->GetDataSize();
	}
	return totalSize;
}

const String CPatchSpecialCases::GetInfo() const
{
	return TXT("SpecialCases");
}

void CPatchSpecialCases::AddToken(CPatchSpecialCasesFileToken* token)
{
	m_tokens.PushBack( token );
}

//-----------------------------------------------------------------------------

CPatchSpecialCasesFileToken::CPatchSpecialCasesFileToken( const String& fileRelativePath, Red::TSharedPtr<IFile> file, Uint64 crc, Uint64 hash )
	: m_fileRelativePath( fileRelativePath )
	, m_file( file )
	, m_hash( hash )
	, m_crc( crc )
{
	/* Intentionally empty */
}

const Uint64 CPatchSpecialCasesFileToken::GetTokenHash() const
{
	return m_hash;
}

const Uint64 CPatchSpecialCasesFileToken::GetDataCRC() const
{
	return m_crc;
}

const Uint64 CPatchSpecialCasesFileToken::GetDataSize() const
{
	return m_file->GetSize();
}

const String CPatchSpecialCasesFileToken::GetInfo() const
{
	return String::Printf( TXT("Special case file '%ls'"), m_fileRelativePath.AsChar() );
}

void CPatchSpecialCasesFileToken::DebugDump(const String& dumpPath, const Bool isBase) const
{
	String absoluePath = dumpPath;
	absoluePath += m_fileRelativePath;
	absoluePath += isBase ? TXT(".base") : TXT(".current");

	Red::TScopedPtr< IFile > destFile( GFileManager->CreateFileWriter( absoluePath.AsChar(), FOF_AbsolutePath ) );
	if ( m_file && destFile )
	{
		Uint8 copyBlock[ 64*1024 ];

		const Uint64 fileSize = m_file->GetSize();
		LOG_WCC( TXT("Dumping %d bytes to '%ls'"), fileSize, absoluePath.AsChar() );

		while ( m_file->GetOffset() < fileSize )
		{
			const Uint64 maxRead = Min< Uint64 >( sizeof(copyBlock), fileSize - m_file->GetOffset() );
			m_file->Serialize( copyBlock, maxRead );
			destFile->Serialize( copyBlock, maxRead );
		}
	}
}

//-----------------------------------------------------------------------------

String CPatchBuilder_SpecialCases::GetContentType() const
{
	return TXT("specialcases");
}

IBasePatchContentBuilder::IContentGroup* CPatchBuilder_SpecialCases::LoadContent(const ECookingPlatform platform, const String& absoluteBuildPath)
{
	TDynArray<String> specialCaseRelativePaths;
	EnumerateSpecialCasePaths( platform, absoluteBuildPath, specialCaseRelativePaths );
	return CreateContentGroup( absoluteBuildPath, specialCaseRelativePaths );
}

Bool CPatchBuilder_SpecialCases::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	if( patchContent.Empty() )
	{
		LOG_WCC( TXT("PatchSpecialCases: nothing to patch here. Move on.") );
		return true;
	}

	for( IContentToken* token : patchContent )
	{
		CPatchSpecialCasesFileToken* specialCaseToken = static_cast<CPatchSpecialCasesFileToken*>( token );
		
		String absoluteFilePath = absoluteBuildPath + specialCaseToken->GetFileRelativePath();
		Red::TSharedPtr<IFile> sourceFile = specialCaseToken->GetFile();

		Red::TScopedPtr< IFile > destFile( GFileManager->CreateFileWriter( absoluteFilePath.AsChar(), FOF_AbsolutePath ) );
		if ( sourceFile && destFile )
		{
			Uint8 copyBlock[ 64*1024 ];

			const Uint64 fileSize = sourceFile->GetSize();
			sourceFile->Seek( 0 );

			while ( sourceFile->GetOffset() < fileSize )
			{
				const Uint64 maxRead = Min< Uint64 >( sizeof(copyBlock), fileSize - sourceFile->GetOffset() );
				sourceFile->Serialize( copyBlock, maxRead );
				destFile->Serialize( copyBlock, maxRead );
			}
		}
	}

	return true;
}

void CPatchBuilder_SpecialCases::EnumerateSpecialCasePaths(const ECookingPlatform platform, const String& absoluteBuildPath, TDynArray<String>& specialCaseRelativePaths)
{
	// generic stuff
	GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\"), TXT("Init.bnk"), specialCaseRelativePaths, true );
	GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\config\\"), TXT("*.ini"), specialCaseRelativePaths, true );
	GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\config\\"), TXT("*.xml"), specialCaseRelativePaths, true );

	// shaders & includes
	if ( platform == PLATFORM_PC )
	{
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\shaders\\speedtree\\shaders_directx11\\"), TXT("*.fx11obj"), specialCaseRelativePaths, true );
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_PS4 )
	{
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\shaders\\speedtree\\shaders_orbis\\"), TXT("*.sb"), specialCaseRelativePaths, true );
	}
	else if ( platform == PLATFORM_XboxOne )
	{
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\shaders\\speedtree\\shaders_durango\\"), TXT("*.durobj"), specialCaseRelativePaths, true );
	}
#endif

	// scripts
	GFileManager->FindFilesRelative( absoluteBuildPath, TXT("content\\content0\\"), TXT("*.redscripts"), specialCaseRelativePaths, true );

	// dep cache
	GFileManager->FindFilesRelative( absoluteBuildPath, TXT("content\\content0\\"), TXT("dep.cache"), specialCaseRelativePaths, true );

	// streaming cache
	GFileManager->FindFilesRelative( absoluteBuildPath, TXT("content\\"), TXT("streaming.cache"), specialCaseRelativePaths, true );

	// videoplayer SWF
	GFileManager->FindFilesRelative( absoluteBuildPath, TXT("content\\content0\\engine\\swf\\"), TXT("videoplayer.redswf"), specialCaseRelativePaths, false );

	// executables
	if ( platform == PLATFORM_PC )
	{
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\x64\\"), TXT("*.exe"), specialCaseRelativePaths, true );
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\x64\\"), TXT("*.dll"), specialCaseRelativePaths, true );
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_PS4 )
	{
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\PS4\\"), TXT("*.elf"), specialCaseRelativePaths, true );
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\config\\r4game\\user_config_matrix\\ps4\\"), TXT("filelist.txt"), specialCaseRelativePaths, true );
	}
	else if ( platform == PLATFORM_XboxOne )
	{
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\XboxOne\\"), TXT("*.*"), specialCaseRelativePaths, true );
		GFileManager->FindFilesRelative( absoluteBuildPath, TXT("bin\\xdkconfig\\"), TXT("*.*"), specialCaseRelativePaths, true );
	}
#endif

	LOG_WCC( TXT("Found %d special cases for all patterns in build '%ls'"), specialCaseRelativePaths.Size(), absoluteBuildPath.AsChar() );
}

IBasePatchContentBuilder::IContentGroup* CPatchBuilder_SpecialCases::CreateContentGroup(const String& absoluteBuildPath, const TDynArray<String>& specialCaseRelativePaths)
{
	CPatchSpecialCases* result = new CPatchSpecialCases();

	for( const String& relativePath : specialCaseRelativePaths )
	{
		if( CheckTokenExistance( relativePath ) == true )
			continue;

		CPatchSpecialCasesFileToken* token = CreateToken( absoluteBuildPath, relativePath );
		result->AddToken( token );
	}

	return result;
}

CPatchSpecialCasesFileToken* CPatchBuilder_SpecialCases::CreateToken(const String& absoluteBuildPath, const String& relativePath)
{
	String absoluteFilePath = absoluteBuildPath + relativePath;
	Red::TSharedPtr<IFile> file = Red::TSharedPtr<IFile>( GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath ) );

	const Uint64 size = file->GetSize();
	if ( size > (1024*1024) )
	{
		LOG_WCC( TXT("Calculating CRC for '%ls' (%1.2f MB)..."), relativePath.AsChar(), size / (1024.0*1024.0) );
	}

	Uint64 hash = CalculateSpecialCaseFileHash( relativePath );
	Uint64 crc = CalculateSpecialCaseFileCRC( file );

	return new CPatchSpecialCasesFileToken( relativePath, file, crc, hash );
}

bool CPatchBuilder_SpecialCases::CheckTokenExistance(const String& relativePath)
{
	Uint64 hash = CalculateSpecialCaseFileHash( relativePath );
	return m_fileTokensMap.KeyExist( hash );
}

Uint64 CPatchBuilder_SpecialCases::CalculateSpecialCaseFileHash(const String& relativePath)
{
	return Red::System::CalculateHash64( UNICODE_TO_ANSI( relativePath.AsChar() ) );
}

Uint64 CPatchBuilder_SpecialCases::CalculateSpecialCaseFileCRC(Red::TSharedPtr<IFile> file)
{
	return PatchUtils::CalcFileBlockCRC( file.Get(), file->GetSize(), RED_FNV_OFFSET_BASIS64 );
}

//-----------------------------------------------------------------------------
