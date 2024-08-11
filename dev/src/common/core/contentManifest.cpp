/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../redSystem/crt.h"
#include "fileSys.h"
#include "scopedPtr.h"
#include "filePath.h"
#include "depot.h"
#include "contentManifest.h"
#include "2darray.h"

namespace Helper
{

static RuntimePackageID PackageIDPool = BASE_RUNTIME_PACKAGE_ID;

RuntimePackageID AllocRuntimePackageID()
{
	return Red::Threads::AtomicOps::Increment32( &PackageIDPool );
}

Bool CalcCRC64( const String& absolutePath, Uint64& outCRC64, Uint64& outFileSize )
{
	const Uint32 BUFSZ = 0x10000;
	Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( absolutePath, FOF_Buffered | FOF_AbsolutePath, BUFSZ ) );
	if ( ! reader )
	{
		return false;
	}

	outFileSize = GFileManager->GetFileSize( absolutePath );

	outCRC64 = RED_FNV_OFFSET_BASIS64;

	Uint8 buf[BUFSZ];
	Uint64 numBytesLeft = outFileSize;
	while ( numBytesLeft > 0 )
	{
		const Uint32 readSize = numBytesLeft >= BUFSZ ? BUFSZ : (Uint32)numBytesLeft;
		numBytesLeft -= readSize;
		reader->Serialize( buf, readSize );
		outCRC64 = Red::System::CalculateHash64( buf, readSize, outCRC64 );
	}

	if ( outFileSize == 0 )
	{
		outCRC64 = 0;
		WARN_CORE(TXT("CalcCRC64: file '%ls' has zero size"), absolutePath.AsChar() );
	}

	return true;
}

Bool CreateContentChunk( SContentChunk& outContentChunk, Red::System::DateTime& outMaxFileTimestamp, CName chunkName, const TDynArray< String >& absolutePaths, const String& baseDirectory, Bool allFilesMustSucceed /*= true*/, Bool skipCrc /*= false */ )
{
	TDynArray< String > manifestPaths;
	
	manifestPaths.Reserve( absolutePaths.Size() );
	for ( const auto& path : absolutePaths )
		manifestPaths.PushBackUnique( path );

	String pathRoot = baseDirectory.ToLower();
	pathRoot.ReplaceAll(TXT("/"), TXT("\\"));
	if ( ! pathRoot.EndsWith( TXT("\\") ) )
	{
		pathRoot += TXT("\\");
	}

	// Remove invalid paths and trim off prefix now since we'll preallocate the strings and entries in bulk
	// Not allowed for a file to contain non-ASCII characters (after the prefix)
	for ( Int32 j = manifestPaths.SizeInt()-1; j >= 0; --j )
	{
		String& path = manifestPaths[j];
		path.ReplaceAll( TXT("/"), TXT("\\") );

		CUpperToLower conv( path.TypedData(), path.Size() );
		if ( path.BeginsWith( pathRoot ) )
		{
			path = path.RightString( path.GetLength() - pathRoot.Size() + 1 );
		}
		else
		{
			if ( allFilesMustSucceed )
			{
				ERR_CORE(TXT("CreateContentChunk: skipping file '%ls' not under root '%ls'"), path.AsChar(), pathRoot.AsChar() );
				return false;
			}

			WARN_CORE(TXT("CreateContentChunk: skipping file '%ls' not under root '%ls'"), path.AsChar(), pathRoot.AsChar() );
			manifestPaths.RemoveAt( j );
		}
	}

	if ( manifestPaths.Empty() )
	{
		ERR_CORE( TXT("CreateContentChunk chunkName '%ls' has no files after validation"), chunkName.AsChar() );
		return false;
	}

	outContentChunk.m_chunkLabel = chunkName;
	outContentChunk.m_contentFiles.Reserve( manifestPaths.Size() );

	outMaxFileTimestamp.Clear();
	for ( Uint32 i = 0 ; i < manifestPaths.Size(); ++i )
	{
		const String& manifestPath = manifestPaths[i];

		// TBD: FIXME, need to pass in isPatch instead of just filenames
		Bool isPatch = false;

		// Reconstruct abs path
		const String absolutePath = pathRoot + manifestPath;
		const Red::System::DateTime fileTimestamp = GFileManager->GetFileTime( absolutePath );
		if ( fileTimestamp > outMaxFileTimestamp )
		{
			outMaxFileTimestamp = fileTimestamp;
		}

		Uint64 fileCrc = 0;
		Uint64 fileSize = 0;

		if ( ! skipCrc )
		{
			if ( !CalcCRC64( absolutePath, fileCrc, fileSize ) )
			{
				if ( allFilesMustSucceed )
				{
					ERR_CORE(TXT("CreateContentChunk: failed to open file '%ls' for reading"), absolutePath.AsChar() );
					outContentChunk.m_contentFiles.Clear();
					return false;
				}
				else
				{
					WARN_CORE(TXT("CreateContentChunk: failed to open file '%ls' for reading"), absolutePath.AsChar() );
				}
			}
		}

		const SContentFile contentFile( UNICODE_TO_ANSI(manifestPath.AsChar()), fileSize, fileCrc, isPatch );
		outContentChunk.m_contentFiles.PushBack( Move( contentFile ) );
	}

	SortFilesForFilter( outContentChunk );

	return true;
}

void SortFilesForFilter( SContentChunk& outContentChunk )
{
	TDynArray< SContentFile >& files = outContentChunk.m_contentFiles;
	
	struct SFileExt
	{
		static const AnsiChar* Get( const StringAnsi& a )
		{
			const AnsiChar* ch = Red::System::StringSearchLast( a.AsChar(), '.' );
			return ch ? ch : "";
		}
	};

	::Sort( files.Begin(), files.End(), [](const SContentFile& a, const SContentFile& b){
		const AnsiChar* const extA = SFileExt::Get( a.m_path );
		const AnsiChar* const extB = SFileExt::Get( b.m_path );
		const Int32 cmp = Red::System::StringCompareNoCase( extA, extB );
		if ( cmp != 0 )
		{
			return cmp < 0;
		}
	
		return Red::System::StringCompareNoCase( a.m_path.AsChar(), b.m_path.AsChar() ) < 0;
	});
}

} // namespace Helper
