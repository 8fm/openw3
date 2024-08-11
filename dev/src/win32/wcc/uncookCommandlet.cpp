#include "build.h"
#include "../../common/core/asyncIO.h"
#include "../../common/core/bundleFileReaderDecompression.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundlePreambleParser.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/debugBundleHeaderParser.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/engine/baseTree.h"
#include "../../common/engine/bitmapDataBuffer.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/swfResource.h"
#include "../../common/engine/swfTexture.h"
#include "../../common/engine/textureArray.h"
#include "textureCacheProcessing.h"
#include "uncookDDSUtils.h"

namespace UncookingHelpers
{
	struct STextureData 
	{
		CBitmapTexture::MipMap	m_mipCopy;
		GpuApi::eTextureFormat	m_format;

		STextureData()
			: m_format(GpuApi::TEXFMT_NULL)
		{}
	};

	Bool GetTextureData( const CBitmapTexture* tex, STextureData& outTextureData )
	{
 		if ( ! tex || tex->GetMipCount() < 1 )
 		{
 			return false;
 		}

		GpuApi::eTextureFormat format = tex->GetPlatformSpecificCompression();
		outTextureData.m_mipCopy = tex->GetMips()[0];
		outTextureData.m_format = format;

		// Programming guide for DDS
		// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
		Uint32 ddsBlockSize = 0;
		switch (format)
		{
		case GpuApi::TEXFMT_BC1:
			ddsBlockSize = 8;
			break;
		case GpuApi::TEXFMT_BC3:
			ddsBlockSize = 16;
			break;
		default:
			break;
		}

		if ( ddsBlockSize > 0 )
		{
			outTextureData.m_mipCopy.m_pitch = Max<Uint32>( 1, (outTextureData.m_mipCopy.m_width + 3 )/4 ) * ddsBlockSize;
		}

		return true;
	}

	Bool GetExtForSaveFormat( GpuApi::eTextureSaveFormat saveFormat, const Char*& outExt )
	{
		struct SaveFormatExtEntry
		{
			const Char*					m_ext;
			GpuApi::eTextureSaveFormat	m_textureSaveFormat;
		};

		static const SaveFormatExtEntry SAVE_FORMAT_EXTS[] = {
			{ TXT("dds"), GpuApi::SAVE_FORMAT_DDS },
			{ TXT("bmp"), GpuApi::SAVE_FORMAT_BMP },
			{ TXT("png"), GpuApi::SAVE_FORMAT_PNG },
			{ TXT("jpg"), GpuApi::SAVE_FORMAT_JPG },
			{ TXT("tga"), GpuApi::SAVE_FORMAT_TGA },
		};

		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(SAVE_FORMAT_EXTS); ++i )
		{
			if ( saveFormat == SAVE_FORMAT_EXTS[i].m_textureSaveFormat )
			{
				outExt = SAVE_FORMAT_EXTS[i].m_ext;
				return true;
			}
		}

		return false;
	}

	Bool DumpTexture( CBitmapTexture* tex, const String& texSavePath, GpuApi::eTextureSaveFormat saveFormat )
	{
		STextureData textureData;
		if ( !GetTextureData( tex, textureData ) )
		{
			ERR_WCC(TXT("Failed to get texture data for '%ls'"), tex->GetDepotPath().AsChar());
			return false;
		}

		textureData.m_mipCopy.m_data.Load();
		Uint8* srcData = static_cast< Uint8* >( textureData.m_mipCopy.m_data.GetData() );
		const size_t srcDataSize = textureData.m_mipCopy.m_data.GetSize();
		const size_t width = textureData.m_mipCopy.m_width;
		const size_t height = textureData.m_mipCopy.m_height;
		const GpuApi::eTextureFormat format = textureData.m_format;
		const size_t pitch = textureData.m_mipCopy.m_pitch; // Already converted in GetTextureData if DDS

		GpuApi::TextureDataDesc textureToSave;
		textureToSave.data = &srcData;
		textureToSave.slicePitch = srcDataSize;
		textureToSave.width = width;
		textureToSave.height = height;
		textureToSave.format = format;
		textureToSave.rowPitch = pitch;

		CFilePath filePath( texSavePath );
 		const Char* ext = nullptr;
		if ( !GetExtForSaveFormat( saveFormat, ext ) )
		{
			ERR_WCC(TXT("Failed to get extension for saveFormat '%u'"), saveFormat);
			return false;
		}

		filePath.SetExtension(ext);
		const String absolutePath = filePath.ToString();
		if ( !GpuApi::SaveTextureToFile( textureToSave, absolutePath.AsChar(), saveFormat ) )
		{
			ERR_WCC(TXT("Failed to save texture '%ls' to disk"), absolutePath.AsChar());
			return false;
		}

		return true;
	}
}

class CUncookCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CUncookCommandlet, ICommandlet, 0 );

public:

	CUncookCommandlet( )
	{
		m_commandletName = CName( TXT( "uncook" ) );
	}
	~CUncookCommandlet( );

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;

private:

	struct Settings
	{
		String						m_bundleDir;
		TDynArray< String >			m_bundleFiles;
		String						m_unbundleDir;
		Bool						m_skipErrors;
		String						m_targetDir;
		THashSet< String >			m_targetFiles;
		Bool						m_unbundleOnly;
		Bool						m_uncookOnly;
		Bool						m_dumpSwf;
		TDynArray< String >			m_uncookSearchPatterns;
		GpuApi::eTextureSaveFormat	m_textureSaveFormat;

		Settings( )
			: m_bundleDir( String::EMPTY )
			, m_unbundleDir( String::EMPTY )
			, m_skipErrors( false )
			, m_targetDir( String::EMPTY )
			, m_unbundleOnly( false )
			, m_uncookOnly( false )
			, m_dumpSwf( false )
			, m_textureSaveFormat( GpuApi::SAVE_FORMAT_TGA )
		{ }

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

private:

	Settings							m_settings;
	TDynArray< CTextureCacheLoader* >	m_textureCacheLoaders;
	TDynArray< String >					m_cookedFilePaths;
	TDynArray< String >					m_unbundlableFileTypes;

private:

	void			EnumerateUnbundlableFileTypes( );

	void			LoadTextureCaches( );
	void			UnloadTextureCaches( );

	Bool			UnbundleData( );
	Bool			UncookData( );

	Bool			ProcessBundleFile( const String& absoluteFilePath );
	Bool			ProcessCookedFile( const String& absoluteFilePath );

	CResource*		LoadAbsoluteFileAsObject( const String& absoluteFilePath ) const;
	BufferHandle	LoadAbsoluteFileAsBuffer( const String& absoluteFilePath ) const;
	Bool			SaveObjectToAbsoluteFile( CResource* object, const String& absoluteFilePath ) const;
	Bool			SaveBufferToAbsoluteFile( BufferHandle buffer, const String& absoluteFilePath ) const;

	Bool			IsFilePathTooLong( const String& filePath ) const;

private:

	template< typename T >
	class AutoDiscardedPtr
	{
		T* ptr;
	public:
		AutoDiscardedPtr( T* ptr ) { this->ptr = ptr; }
		~AutoDiscardedPtr( ) { ptr->Discard( ); }
		T* Get( ) const { return ptr; }
		T* operator -> ( ) const { return ptr; }
	};
};

BEGIN_CLASS_RTTI( CUncookCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI( )

IMPLEMENT_ENGINE_CLASS( CUncookCommandlet );

CUncookCommandlet::~CUncookCommandlet( )
{
	UnloadTextureCaches( );
}

bool CUncookCommandlet::Execute( const CommandletOptions& options )
{
	if ( !m_settings.Parse( options ) )
		return false;

	// enable depot fallback
	GDepot->EnableFallbackResources( true );

	EnumerateUnbundlableFileTypes( );

	LoadTextureCaches( );

	if( !m_settings.m_uncookOnly )
	{
		if( !UnbundleData( ) && !m_settings.m_skipErrors )
			return false;
	}

	if( !m_settings.m_unbundleOnly )
	{
		if( !UncookData( ) && !m_settings.m_skipErrors )
			return false;
	}

	UnloadTextureCaches( );

	return true;
}

const Char* CUncookCommandlet::GetOneLiner( ) const
{
	return TXT( "Uncooks resources from a given bundle set." );
}

void CUncookCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  uncook -indir=<dirpath> -outdir=<dirpath> [options]" ) );
	LOG_WCC( TXT( "    -indir    - Path to the bundled directory." ) );
	LOG_WCC( TXT( "    -outdir   - Path to the unbundled directory." ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Options:" ) );
	LOG_WCC( TXT( "  -infile=<filepath>     - Path to bundle file." ) );
	LOG_WCC( TXT( "  -skiperrors            - Upon failure, skips to the next file." ) );
	LOG_WCC( TXT( "  -targetdir=<dirpath>   - Relative inner path to be extracted." ) );
	LOG_WCC( TXT( "  -targetfile=<filepath> - Relative inner file path to be extracted." ) );
	LOG_WCC( TXT( "  -unbundleonly          - Unbundles data without uncooking it." ) );
	LOG_WCC( TXT( "  -uncookonly            - Assumes data in unbundledir is unbundled already." ) );
	LOG_WCC( TXT( "  -uncookext=<...>       - Comma delineated list of file extensions to uncook. If options missing will uncook all available. E.g., -extensions=xbm,w2mesh" ) );
	LOG_WCC( TXT( "  -imgfmt=<imgopt>       - Image format for XBM files. Choose one of bmp, png, jpg, or tga. Default is tga.") );
	LOG_WCC( TXT( "  -dumpswf               - Dump redswf files out.") );
}

Bool CUncookCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	if( !options.GetSingleOptionValue( TXT( "indir" ), m_bundleDir ) )
	{
		ERR_WCC( TXT( "No input directory has been specified!" ) );
		return false;
	}

	if( !m_bundleDir.EndsWith( TXT( "\\" ) ) )
		m_bundleDir += TXT( "\\" );

	if( options.HasOption( TXT( "infile" ) ) )
	{
		TList< String > files = options.GetOptionValues( TXT( "infile" ) );
		for( const String& file : files )
		{
			CFilePath path( file );
			if( path.IsRelative( ) )
			{
				ERR_WCC( TXT( "File path '%ls' is not absolute! Ignoring..." ), file.AsChar( ) );
				continue;
			}
			if( !GFileManager->FileExist( file ) )
			{
				ERR_WCC( TXT( "File '%ls' does not exist! Ignoring..." ), file.AsChar( ) );
				continue;
			}
			m_bundleFiles.PushBackUnique( file );
		}
	}

	if( !options.GetSingleOptionValue( TXT( "outdir" ), m_unbundleDir ) )
	{
		ERR_WCC( TXT( "No output directory has been specified!" ) );
		return false;
	}

	if( !m_unbundleDir.EndsWith( TXT( "\\" ) ) )
		m_unbundleDir += TXT( "\\" );

	m_skipErrors = options.HasOption( TXT( "skiperrors" ) );

	if( options.GetSingleOptionValue( TXT( "targetdir" ), m_targetDir ) )
	{
		m_targetDir.MakeLower( );
		if( !m_targetDir.EndsWith( TXT( "\\" ) ) )
			m_targetDir += TXT( "\\" );
	}

	if( options.HasOption( TXT( "targetfile" ) ) )
	{
		TList< String > files = options.GetOptionValues( TXT( "infile" ) );
		for( const String& file : files )
		{
			m_targetFiles.Insert( file );
		}
	}

	String imgFormat;
	if ( options.GetSingleOptionValue( TXT("imgfmt"), imgFormat ) )
	{
		const Char* pch = imgFormat.AsChar();
		if ( *pch == TXT('.') )
			++pch;

		if ( Red::System::StringCompareNoCase( pch, TXT("dds") ) == 0 )
			m_textureSaveFormat = GpuApi::SAVE_FORMAT_DDS;
		else if ( Red::System::StringCompareNoCase( pch, TXT("bmp") ) == 0 )
			m_textureSaveFormat = GpuApi::SAVE_FORMAT_BMP;
		else if ( Red::System::StringCompareNoCase( pch, TXT("png") ) == 0 )
			m_textureSaveFormat = GpuApi::SAVE_FORMAT_PNG;
		else if ( Red::System::StringCompareNoCase( pch, TXT("jpg") ) == 0 )
			m_textureSaveFormat = GpuApi::SAVE_FORMAT_JPG;
		else if ( Red::System::StringCompareNoCase( pch, TXT("tga") ) == 0 )
			m_textureSaveFormat = GpuApi::SAVE_FORMAT_TGA;
		else
		{
			ERR_WCC(TXT("Unrecognized imgfmt argument '%ls'"), imgFormat.AsChar());
			return false;
		}
	}

	m_dumpSwf = options.HasOption(TXT("dumpswf"));
	m_unbundleOnly = options.HasOption( TXT( "unbundleonly" ) );
	m_uncookOnly = options.HasOption( TXT( "uncookonly" ) );

	if ( options.HasOption( TXT("uncookext") ) )
	{
		const auto& opts = options.GetOptionValues( TXT("uncookext") );
		for ( auto it = opts.Begin(); it != opts.End(); ++it )
		{
			const Char* pch = it->AsChar();
			for(;;)
			{
				if ( *pch && *pch == TXT('*') )
				{
					++pch;
					continue;
				}
				if ( *pch && *pch == TXT('.') )
				{
					++pch;
					continue;
				}
				break;
			}

			if ( !*pch )
				continue;

			const String pattern = String::Printf(TXT("*.%ls"), pch );
			m_uncookSearchPatterns.PushBackUnique( pattern.ToLower() );

			// Extract texture arrays altogether with regular textures.
			if( pattern.EqualsNC( TXT( "*.xbm" ) ) )
				m_uncookSearchPatterns.PushBackUnique( TXT( "*.texarray" ) );
		}
	}

	if ( m_uncookSearchPatterns.Empty() )
	{
		m_uncookSearchPatterns.PushBack(TXT("*.*"));
	}

	return true;
}

void CUncookCommandlet::EnumerateUnbundlableFileTypes( )
{
	m_unbundlableFileTypes.PushBack( TXT( ".w2mesh" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".w2mesh.1.buffer" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".xbm" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".texarray" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".xml" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".w2mg" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".w2mi" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".csv" ) );
	m_unbundlableFileTypes.PushBack( TXT( ".srt" ) );

	if ( m_settings.m_dumpSwf )
		m_unbundlableFileTypes.PushBack( TXT( ".redswf" ) );
}

void CUncookCommandlet::LoadTextureCaches( )
{
	TDynArray< String > filePaths;
	GFileManager->FindFiles( m_settings.m_bundleDir, TXT( "texture.cache" ), filePaths, true );

	if( filePaths.Empty( ) )
	{
		WARN_WCC( TXT( "Failed to find texture caches in '%ls'!" ), m_settings.m_bundleDir.AsChar( ) );
		return;
	}

	for( String file : filePaths )
	{
		// For some reason, the loader hangs with empty files.
		if( GFileManager->GetFileSize( file ) == 0 )
			continue;

		CTextureCacheLoader* loader = new CTextureCacheLoader( );
		loader->StartLoading( file );
		m_textureCacheLoaders.PushBack( loader );
		while( !loader->IsLoaded( ) ) { } // Ugly!
	}
}

void CUncookCommandlet::UnloadTextureCaches( )
{
	m_textureCacheLoaders.ClearPtr( );
}

Bool CUncookCommandlet::UnbundleData( )
{
	TDynArray< String > filePaths;

	// Specific files have been requested, unbundle only those.
	if( !m_settings.m_bundleFiles.Empty( ) )
	{
		filePaths = m_settings.m_bundleFiles;
	}

	// Unbundle all files in the base directory.
	else
	{
		GFileManager->FindFiles( m_settings.m_bundleDir, TXT( "*.bundle" ), filePaths, true );
		if( filePaths.Empty( ) )
		{
			ERR_WCC( TXT( "No bundle files found in '%ls'!" ), m_settings.m_bundleDir.AsChar() );
			return false;
		}
	}

	for( String file : filePaths )
	{
		if( !ProcessBundleFile( file ) )
		{
			ERR_WCC( TXT( "Failed to unbundle file '%ls'!" ), file.AsChar( ) );
			if( !m_settings.m_skipErrors )
			{
				return false;
			}
		}
	}

	return true;
}

Bool CUncookCommandlet::UncookData( )
{
	if( m_cookedFilePaths.Empty( ) && m_settings.m_uncookOnly )
	{
		String searchPath = m_settings.m_unbundleDir;
		if( !m_settings.m_targetFiles.Empty( ) )
		{
			for( const String& file : m_settings.m_targetFiles )
			{
				GFileManager->FindFiles( m_settings.m_unbundleDir, file, m_cookedFilePaths, true );
			}
		}
		else
		{
			if( m_settings.m_targetDir != String::EMPTY )
			{
				searchPath += m_settings.m_targetDir;
			}
			for ( const String& pattern : m_settings.m_uncookSearchPatterns )
			{
				GFileManager->FindFiles( searchPath, pattern, m_cookedFilePaths, true );
			}
		}
		Sort( m_cookedFilePaths.Begin(), m_cookedFilePaths.End() );
		for ( Int32 j = m_cookedFilePaths.SizeInt()-1; j >= 1 ; --j )
		{
			if ( m_cookedFilePaths[j] == m_cookedFilePaths[j-1] )
				m_cookedFilePaths.RemoveAt(j);
		}
	}
	else if ( !m_settings.m_uncookSearchPatterns.Empty() && m_settings.m_targetFiles.Empty( ) && !m_settings.m_uncookSearchPatterns.Exist(TXT("*.*") ) )
	{
		for ( Int32 j = m_cookedFilePaths.SizeInt()-1; j >= 0; --j )
		{
			Bool matchFound = false;
			for ( const String& pattern : m_settings.m_uncookSearchPatterns )
			{
				if ( StringHelpers::WildcardMatch( m_cookedFilePaths[j].AsChar(), pattern.AsChar() ) )
				{
					matchFound = true;
					break;
				}
			}
			if ( !matchFound )
				m_cookedFilePaths.RemoveAt(j);
		}
	}

	if( m_cookedFilePaths.Empty( ) )
	{
		ERR_WCC( TXT( "No unbundled files found in '%ls'!" ), m_settings.m_bundleDir.AsChar() );
		return false;
	}

	for( String file : m_cookedFilePaths )
	{
		if( !ProcessCookedFile( file ) )
		{
			ERR_WCC( TXT( "Failed to uncook file '%ls'!" ), file.AsChar( ) );
		}
		else
		{
			// Free discarded CResource's every once in a while (?).
			static Uint32 gc_cycles = 0;
			if( ++gc_cycles > 500 )
			{
				gc_cycles = 0;
				GObjectsDiscardList->ProcessList( true );
				SGarbageCollector::GetInstance( ).CollectNow( );
			}
		}
	}

	return true;
}

Bool CUncookCommandlet::ProcessBundleFile( const String& absoluteFilePath )
{
	// Parse bundle preamble.
	Red::Core::Bundle::SBundleHeaderPreamble preamble;
	Red::Core::Bundle::CBundlePreambleParser preambleParser( *GDeprecatedIO );
	preambleParser.CreateLoadToken( absoluteFilePath, &preamble );
	preambleParser.Parse( );
	if( preamble.m_configuration != Red::Core::Bundle::EBundleHeaderConfiguration::BHC_Debug )
	{
		return false;
	}

	// Open bundle file reader.
	Red::TScopedPtr< IFile > bundleFile( GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath | FOF_Buffered ) );
	if( !bundleFile.Get( ) )
	{
		ERR_WCC( TXT( "Failed to open bundle file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return false;
	}

	// Read bundle debug header.
	void* headerBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_BundleMetadata, preamble.m_headerSize );
	Red::Core::Bundle::CDebugBundleHeaderParser headerParser( *GDeprecatedIO, preamble );
	headerParser.CreateLoadToken( absoluteFilePath, headerBuffer );
	headerParser.Parse( );
	const Red::Core::Bundle::CDebugBundleHeaderParser::HeaderCollection headerItemCollection = headerParser.GetHeaderItems( );

	// Allocate decompression buffers.
	TDynArray< Uint8 > compressedData( 64 * 1024 * 1024 );
	TDynArray< Uint8 > uncompressedData( 64 * 1024 * 1024 );

	// Process every bundled file.
	const Uint32 headerItemCount = headerItemCollection.Size( );
	for( Uint32 itemIndex = 0; itemIndex < headerItemCount; ++itemIndex )
	{
		const auto& fileHeaderInfo = *headerItemCollection[ itemIndex ];

		// Get bundled file path.
		StringAnsi filePath( fileHeaderInfo.m_rawResourcePath );
		filePath.MakeLower( );

		// Skip files not present in the target list.
		if( !m_settings.m_targetFiles.Empty( ) && m_settings.m_targetFiles.FindPtr( ANSI_TO_UNICODE( filePath.AsChar( ) ) ) == nullptr )
			continue;

		// Skip files out of scope.
		else if( m_settings.m_targetDir != String::EMPTY && !filePath.BeginsWith( UNICODE_TO_ANSI( m_settings.m_targetDir.AsChar( ) ) ) )
			continue;

		// Build output unbundled file path.
		const String fullPath = m_settings.m_unbundleDir + ANSI_TO_UNICODE( filePath.AsChar( ) );

		// Check for too long file path names.
		if( IsFilePathTooLong( fullPath ) )
		{
			continue;
		}

		// Skip unsupported files (can't use a THashSet because of special types like ".w2mesh.1.buffer").
		Bool validFileType = false;
		for( const String& type : m_unbundlableFileTypes )
		{
			if( fullPath.EndsWith( type ) )
			{
				validFileType = true;
				break;
			}
		}
		if( !validFileType )
			continue;

		// Skip files that have already been processed.
		const Uint64 existingSize = GFileManager->GetFileSize( fullPath );
		if( existingSize == fileHeaderInfo.m_dataSize )
			continue;

		// Create output file.
		Red::TScopedPtr< IFile > outputFile( GFileManager->CreateFileWriter( fullPath, FOF_AbsolutePath | FOF_Buffered ) );
		if( !outputFile.Get( ) )
		{
			ERR_WCC( TXT( "Failed to create output unbundled file '%ls'!" ), fullPath.AsChar( ) );
			if( m_settings.m_skipErrors )
				continue;
			RED_MEMORY_FREE( MemoryPool_Default, MC_BundleMetadata, headerBuffer );
			return false;
		}

		LOG_WCC( TXT( "Extracting '%ls' (%1.2f KB)..." ), ANSI_TO_UNICODE( filePath.AsChar( ) ), fileHeaderInfo.m_dataSize / 1024.0f );

		// Direct output for not compressed files.
		if( fileHeaderInfo.m_compressionType == Red::Core::Bundle::CT_Uncompressed )
		{
			// Move to file start.
			bundleFile->Seek( fileHeaderInfo.m_dataOffset );

			// Copy data.
			Uint8 buffer[ 64 * 1024 ];
			Uint64 sizeLeft = fileHeaderInfo.m_dataSize;
			while( sizeLeft > 0 )
			{
				const Uint64 readBlock = Min< Uint64 >( sizeof( buffer ), sizeLeft );
				bundleFile->Serialize( buffer, readBlock );
				outputFile->Serialize( buffer, readBlock );
				sizeLeft -= readBlock;
			}

			// Keep track of unbundled files.
			if( existingSize == 0 )
			{
				m_cookedFilePaths.PushBack( fullPath );
			}

			continue;
		}

		// Prepare source buffer.
		if( fileHeaderInfo.m_compressedDataSize > compressedData.Size( ) )
			compressedData.Resize( fileHeaderInfo.m_compressedDataSize  );

		// Prepare target buffer.
		if( fileHeaderInfo.m_dataSize > uncompressedData.Size( ) )
			uncompressedData.Resize( fileHeaderInfo.m_dataSize );

		// Read compressed data.
		bundleFile->Seek( fileHeaderInfo.m_dataOffset );
		bundleFile->Serialize( compressedData.Data( ), fileHeaderInfo.m_compressedDataSize );

		// Decompress file data.
		if( !BundleFileReaderDecompression::DecompressFileBufferSynch(
			( Red::Core::Bundle::ECompressionType )fileHeaderInfo.m_compressionType,
			compressedData.Data( ),
			fileHeaderInfo.m_compressedDataSize,
			uncompressedData.Data( ),
			fileHeaderInfo.m_dataSize
		) )
		{
			ERR_WCC( TXT( "Failed to decompress bundled file '%ls'!" ), filePath.AsChar( ) );
			if( m_settings.m_skipErrors )
				continue;
			RED_MEMORY_FREE( MemoryPool_Default, MC_BundleMetadata, headerBuffer );
			return false;
		}

		// Write output data.
		outputFile->Serialize( uncompressedData.Data( ), fileHeaderInfo.m_dataSize );

		// Keep track of uncooked files.
		if( existingSize == 0 )
		{
			m_cookedFilePaths.PushBack( fullPath );
		}
	}

	// Free resources.
	RED_MEMORY_FREE( MemoryPool_Default, MC_BundleMetadata, headerBuffer );

	return true;
}

Bool CUncookCommandlet::ProcessCookedFile( const String& absoluteFilePath )
{
	if( absoluteFilePath.EndsWith( TXT( "w2mesh" ) ) )
	{
		LOG_WCC( TXT( "Uncooking file '%ls'..." ), absoluteFilePath.AsChar( ) );

		AutoDiscardedPtr< CMesh > mesh( Cast< CMesh >( LoadAbsoluteFileAsObject( absoluteFilePath ) ) );
		if( !mesh.Get( ) )
			return false;

		// Load the cooked buffer with mesh data.
		const String meshBufferAbsolutePath = absoluteFilePath + TXT( ".1.buffer" );
		BufferHandle meshBuffer = LoadAbsoluteFileAsBuffer( meshBufferAbsolutePath );
		if( meshBuffer == nullptr )
			return false;

		// Convert the mesh.
		mesh->UncookData( meshBuffer );

		// Save the uncooked mesh.
		if( !SaveObjectToAbsoluteFile( mesh.Get( ), absoluteFilePath ) )
			return false;

		// DELETE the cooked mesh buffer.
		GFileManager->DeleteFile( meshBufferAbsolutePath );

		return true;
	}
	else if( absoluteFilePath.EndsWith( TXT( "xbm" ) ) )
	{
		LOG_WCC( TXT( "Uncooking file '%ls'..." ), absoluteFilePath.AsChar( ) );

		size_t depotPathIndex = 0;
		if( !absoluteFilePath.FindSubstring( m_settings.m_unbundleDir, depotPathIndex, true ) || depotPathIndex != 0 )
		{
			WARN_WCC( TXT( "Failed to find unbundled path for file '%ls'!" ), absoluteFilePath.AsChar( ) );
			return false;
		}

		String depotPath = ( absoluteFilePath.RightString( absoluteFilePath.Size( ) - m_settings.m_unbundleDir.Size( ) ) ).ToLower( );
		Uint32 fileHash = GetHash( depotPath );

		// Find texture entry in the caches.
		CTextureCacheQuery query;
		for( CTextureCacheLoader* loader : m_textureCacheLoaders )
		{
			query = loader->FindEntry( fileHash );
			if( query ) break;
		}
		if( !query )
		{
			WARN_WCC( TXT( "Failed to find cached texture for file '%ls'!" ), absoluteFilePath.AsChar( ) );

			// Flat one-color textures don't seem to be cached, so they have to be treated as false positives.
			// They all happen to have ( tex->GetResidentMipIndex( ) == 0 ), but I have no confirmation for this.
			//return false;
			return true;
		}

		// Allocate memory.
		const Uint32 memSize = static_cast< Uint32 >( query.GetEntry( ).m_info.m_uncompressedSize );
		void* mem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, memSize );
		BufferHandle textureCacheData( new BufferProxy( mem, memSize, [ ] ( void* ptr ) { RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, ptr ); } ) );

		// Load cached data.
		ETextureCacheLoadResult result = query.LoadData( 0, mem, memSize );
		if( result != Red::Core::Decompressor::Status_Success )
		{
			WARN_WCC( TXT( "Failed to decompress cached texture for file '%ls'!" ), absoluteFilePath.AsChar( ) );
			return false;
		}

		// Load the cooked resource.
		AutoDiscardedPtr< CBitmapTexture > tex( Cast< CBitmapTexture >( LoadAbsoluteFileAsObject( absoluteFilePath ) ) );
		if( !tex.Get( ) )
			return false;

		// Convert the texture.
		if( !tex->UncookData( query.GetEntry( ), textureCacheData ) )
			return false;

		// Dump something people can edit
		if ( !UncookingHelpers::DumpTexture( tex.Get(), absoluteFilePath, m_settings.m_textureSaveFormat ) )
			return false;

		// Save the uncooked texture.
		if( !SaveObjectToAbsoluteFile( tex.Get( ), absoluteFilePath ) )
			return false;

		return true;
	}
	else if( absoluteFilePath.EndsWith( TXT( "srt" ) ) )
	{
		LOG_WCC( TXT( "Uncooking file '%ls'..." ), absoluteFilePath.AsChar( ) );

		BufferHandle treeBuffer = LoadAbsoluteFileAsBuffer( absoluteFilePath );
		if( treeBuffer == nullptr )
			return false;

		// Parse used textures.
		TDynArray< String > treeTextures;
		CSRTBaseTree::ReportUsedTextures( static_cast< const AnsiChar* >( treeBuffer->GetData( ) ), treeBuffer->GetSize( ), treeTextures );

		// Extract textures from caches.
		if( !treeTextures.Empty( ) )
		{
			size_t depotPathIndex = 0;
			if( !absoluteFilePath.FindSubstring( m_settings.m_unbundleDir, depotPathIndex, true ) || depotPathIndex != 0 )
			{
				WARN_WCC( TXT( "Failed to find unbundled path for file '%ls'!" ), absoluteFilePath.AsChar( ) );
				return false;
			}

			const String depotPath = absoluteFilePath.RightString( absoluteFilePath.Size( ) - m_settings.m_unbundleDir.Size( ) );
			const String depotPathDirectories = CFilePath( depotPath ).GetPathString( true );

			for( const String& texture : treeTextures )
			{
				const String outFilePath = m_settings.m_unbundleDir + depotPathDirectories + texture;

				// Skip existing files (used repeatedly in several *.srt files).
				if( GFileManager->FileExist( outFilePath ) )
					continue;

				const String entryPath = ( depotPathDirectories + texture ).ToLower( );
				Uint32 fileHash = GetHash( entryPath );

				// Find texture entry in the caches.
				CTextureCacheQuery query;
				for( CTextureCacheLoader* loader : m_textureCacheLoaders )
				{
					query = loader->FindEntry( fileHash );
					if( query ) break;
				}
				if( !query )
				{
					WARN_WCC( TXT( "Failed to find cached texture '%ls'!" ), entryPath.AsChar( ) );
					continue;
				}

				const TextureCacheEntryBase& entry = query.GetEntry( ).m_info;

				// Build DDS header.
				UncookDDSUtils::DDSFileHeader ddsFileHeader;
				UncookDDSUtils::CreateDDSHeader( ddsFileHeader, entry.m_baseWidth, entry.m_baseHeight, entry.m_mipCount, query.GetEntry( ).m_encodedFormat );

				const Uint32 headerSize = ddsFileHeader.m_size;
				const Uint32 bufferSize = static_cast< Uint32 >( entry.m_uncompressedSize );

				// Allocate memory.
				const Uint32 memSize = headerSize + bufferSize;
				void* mem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, memSize );
				BufferHandle textureCacheData( new BufferProxy( mem, memSize, [ ] ( void* ptr ) { RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, ptr ); } ) );

				// Copy header into final buffer.
				Red::MemoryCopy( mem, &ddsFileHeader.m_header, ddsFileHeader.m_size );

				// Load cached data.
				ETextureCacheLoadResult result = query.LoadData( 0, static_cast< Uint8* >( mem ) + headerSize, bufferSize );
				if( result != Red::Core::Decompressor::Status_Success )
				{
					WARN_WCC( TXT( "Failed to decompress cached texture file '%ls'!" ), entryPath.AsChar( ) );
					return false;
				}

				// Copy texture data.
				SaveBufferToAbsoluteFile( textureCacheData, outFilePath );
			}
		}

		return true;
	}
	else if( absoluteFilePath.EndsWith( TXT( "redswf" ) ) )
	{
		if ( !m_settings.m_dumpSwf )
			return true;

		LOG_WCC( TXT( "Uncooking file '%ls'..." ), absoluteFilePath.AsChar( ) );

		AutoDiscardedPtr< CSwfResource > swfResource( Cast< CSwfResource>( LoadAbsoluteFileAsObject( absoluteFilePath ) ) );
		if( !swfResource.Get( ) )
			return false;

		// Save GFX file
		{
			const DataBuffer& gfxBuffer = swfResource->GetDataBuffer();
			CFilePath baseFilePath( absoluteFilePath );

			baseFilePath.SetExtension(TXT("gfx"));
			const String gfxFilePath = baseFilePath.ToString();

			Red::TScopedPtr< IFile > writer( GFileManager->CreateFileWriter( gfxFilePath, FOF_AbsolutePath | FOF_Buffered ) );
			if( !writer.Get( ) )
			{
				WARN_WCC( TXT( "Failed to create writer for file '%ls'!" ), gfxFilePath.AsChar( ) );
				return false;
			}
			writer->Serialize( gfxBuffer.GetData(), gfxBuffer.GetSize() );
		}

		// Save embedded textures
		{
			CFilePath baseFilePath( absoluteFilePath );

			Bool allOk = true;
			const auto& swfTextures = swfResource->GetTextures();
			for ( Uint32 i = 0; i < swfTextures.Size(); ++i )
			{
				CSwfTexture* swfTexture = swfTextures[ i ];
				const String& linkageName = swfTexture->GetLinkageName();

				if ( !swfTexture->UncookData() )
					return false;

				String name = swfTexture->GetLinkageName();
				name.ReplaceAll(TXT(".dds"),TXT(""));
				baseFilePath.SetFileName(name);
				baseFilePath.SetExtension(TXT("dds"));
				const String texSavePath = baseFilePath.ToString();

				if ( !UncookingHelpers::DumpTexture( swfTexture, texSavePath, GpuApi::SAVE_FORMAT_DDS ) )
				{
					WARN_WCC( TXT( "Failed to dump swfTexture '%ls' in redswf '%ls'"), swfTexture->GetLinkageName().AsChar(), absoluteFilePath.AsChar() );
					allOk = false;
				}
			}

			if ( !allOk )
				return false;
		}

		return true;
	}
	else if( absoluteFilePath.EndsWith( TXT( "texarray" ) ) )
	{
		LOG_WCC( TXT( "Uncooking file '%ls'..." ), absoluteFilePath.AsChar( ) );

		// Load the cooked resource.
		AutoDiscardedPtr< CTextureArray > texarray( Cast< CTextureArray >( LoadAbsoluteFileAsObject( absoluteFilePath ) ) );
		if( !texarray.Get( ) )
			return false;

		// Find texture entry in the caches.
		CTextureCacheQuery query;
		for( CTextureCacheLoader* loader : m_textureCacheLoaders )
		{
			query = loader->FindEntry( texarray->GetCookedData().m_header.m_textureCacheKey );
			if( query ) break;
		}
		if( !query )
		{
			WARN_WCC( TXT( "Failed to find cached data for texture array file '%ls'!" ), absoluteFilePath.AsChar( ) );
			return true;
		}

		// Allocate memory.
		const Uint32 memSize = static_cast< Uint32 >( query.GetEntry( ).m_info.m_uncompressedSize );
		void* mem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, memSize );
		BufferHandle textureCacheData( new BufferProxy( mem, memSize, [ ] ( void* ptr ) { RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, ptr ); } ) );

		// Load cached data.
		ETextureCacheLoadResult result = query.LoadData( 0, mem, memSize );
		if( result != Red::Core::Decompressor::Status_Success )
		{
			WARN_WCC( TXT( "Failed to decompress cached data for texture array file '%ls'!" ), absoluteFilePath.AsChar( ) );
			return false;
		}

		// Extract cached data.
		texarray->UncookData( query.GetEntry( ), textureCacheData, absoluteFilePath, m_settings.m_unbundleDir );

		// Process each entry in the array.
		TDynArray< CBitmapTexture* > textures;
		texarray->GetTextures( textures );
		for( auto tex : textures )
		{
			String outPath = m_settings.m_unbundleDir + tex->GetDepotPath( );

			// Create writer.
			Red::TScopedPtr< IFile > writer( GFileManager->CreateFileWriter( outPath, FOF_AbsolutePath | FOF_Buffered ) );
			if( !writer.Get( ) )
			{
				return false;
			}

			// Save single object.
			CDependencySaver saver( *writer, NULL );
			DependencySavingContext context( tex );
			if( !saver.SaveObjects( context ) )
			{
				return false;
			}

			// Output editable file.
			if( !UncookingHelpers::DumpTexture( tex, outPath, m_settings.m_textureSaveFormat ) )
				return false;
		}

		// Save the uncooked array.
		if( !SaveObjectToAbsoluteFile( texarray.Get( ), absoluteFilePath ) )
			return false;

		return true;
	}

	// File is OK, keep it.
	return true;
}

CResource* CUncookCommandlet::LoadAbsoluteFileAsObject( const String& absoluteFilePath ) const
{
	// Check for too long file path names.
	if( IsFilePathTooLong( absoluteFilePath ) )
		return false;

	// Setup loader context.
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = NULL; // Resources from files are never parented.
	loadingContext.m_validateHeader = true; // Do header validation - at this stage we can safely handle missing assets.
	loadingContext.m_getAllLoadedObjects = true;

	// Reader.
	Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath | FOF_Buffered ) );
	if( !reader.Get( ) )
	{
		WARN_WCC( TXT( "Failed to create reader for file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return nullptr;
	}

	// Hacky disk file.
	Red::TScopedPtr< CDiskFile > diskFile( new CDiskFile( GDepot, TXT( "fakeDiskFile" ) ) );

	// Load data from file.
	CDependencyLoader loader( *reader, diskFile.Get( ) );
	if( !loader.LoadObjects( loadingContext ) )
	{
		WARN_WCC( TXT( "Failed to load contents from file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return nullptr;
	}

	// Nothing loaded.
	if ( loadingContext.m_loadedObjects.Empty( ) )
	{
		WARN_WCC( TXT( "No objects loaded from file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return nullptr;
	}

	// Resource should be loaded.
	CResource* res = Cast< CResource >( loadingContext.m_loadedObjects[ 0 ].GetObjectPtr( ) );
	if( !res )
	{
		WARN_WCC( TXT( "Invalid objects loaded from file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return nullptr;
	}

	// Unbind from the fake file.
	res->UnbindFile( );

	return res;
}

BufferHandle CUncookCommandlet::LoadAbsoluteFileAsBuffer( const String& absoluteFilePath ) const
{
	// Check for too long file path names.
	if( IsFilePathTooLong( absoluteFilePath ) )
		return BufferHandle( );

	// Create reader.
	Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath | FOF_Buffered ) );
	if( !reader.Get() )
	{
		WARN_WCC( TXT( "Failed to load contents from file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return BufferHandle( );
	}

	// Allocate memory.
	const Uint32 memSize = static_cast< Uint32 >( reader->GetSize( ) );
	void* mem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, memSize );
	reader->Serialize( mem, memSize );

	return BufferHandle( new BufferProxy( mem, memSize, [ ] ( void* ptr ) { RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, ptr ); } ) );
}

Bool CUncookCommandlet::SaveObjectToAbsoluteFile( CResource* object, const String& absoluteFilePath ) const
{
	// Check for too long file path names.
	if( IsFilePathTooLong( absoluteFilePath ) )
		return false;

	// Create writer.
	Red::TScopedPtr< IFile > writer( GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath | FOF_Buffered ) );
	if( !writer.Get( ) )
	{
		WARN_WCC( TXT( "Failed to create writer for file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return false;
	}

	// Save single object.
	CDependencySaver saver( *writer, NULL );
	DependencySavingContext context( object );
	if( !saver.SaveObjects( context ) )
	{
		WARN_WCC( TXT( "Failed to save contents for file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return false;
	}

	return true;
}

Bool CUncookCommandlet::SaveBufferToAbsoluteFile( BufferHandle buffer, const String& absoluteFilePath ) const
{
	// Check for too long file path names.
	if( IsFilePathTooLong( absoluteFilePath ) )
		return false;

	// Create reader.
	Red::TScopedPtr< IFile > writer( GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath | FOF_Buffered ) );
	if( !writer.Get( ) )
	{
		WARN_WCC( TXT( "Failed to create writer for file '%ls'!" ), absoluteFilePath.AsChar( ) );
		return false;
	}

	// Save buffer data.
	writer->Serialize( buffer->GetData( ), buffer->GetSize( ) );

	return true;
}

Bool CUncookCommandlet::IsFilePathTooLong( const String& filePath ) const
{
	// Check for too long file path names.
	if( filePath.Size( ) > MAX_PATH )
	{
		WARN_WCC( TXT( "File path is too long (%d characters, limit is %d)! '%ls'" ), filePath.Size( ), MAX_PATH, filePath.AsChar( ) );
		return true;
	}

	return false;
}