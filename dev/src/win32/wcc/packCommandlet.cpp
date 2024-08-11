/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundleFileReaderDecompression.h"
#include "../../common/core/compression/doboz.h"
#include "../../common/core/compression/zlib.h"
#include "../../common/core/compression/snappy.h"
#include "../../common/core/compression/lz4.h"
#include "../../common/core/compression/lz4hc.h"
#include "../../common/redSystem/crc.h"
#include "patchUtils.h"

/// Helper commandlet that packs all files from directory into single bundle WITHOUT using bundle builder
class CPackCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CPackCommandlet, ICommandlet, 0 );

public:
	CPackCommandlet();
	~CPackCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Packs file from given directory into a bundle"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CPackCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CPackCommandlet );

CPackCommandlet::CPackCommandlet()
{
	m_commandletName = CName( TXT("pack") );
}

CPackCommandlet::~CPackCommandlet()
{
}

namespace Helper
{
	/// Helper for writing a bundle files
	class CPackedBundleWriter
	{
	public:
		CPackedBundleWriter()
			: m_totalDataSize( 0 )
		{
			// allocate space for header
			m_currentOffset = sizeof( Red::Core::Bundle::SBundleHeaderPreamble );
		}

		/// Add file entry
		void AddEntry( const String& absoluteFilePath, const String& relativeFilePath, Red::Core::Bundle::ECompressionType compression )
		{
			// allocate space for the entry
			Entry info;
			info.m_compression = compression;
			info.m_absoluteFilePath = absoluteFilePath;
			info.m_bundleFilePath = UNICODE_TO_ANSI( relativeFilePath.AsChar() );
			info.m_entryOffset = m_currentOffset;
			info.m_dataSize = 0; // known after compression
			info.m_order = info.m_bundleFilePath.EndsWith( ".buffer" ) ? 1 : 0;
			m_entries.PushBack(info);

			// advance entry
			m_currentOffset += Red::Core::Bundle::ALIGNED_DEBUG_BUNDLE_HEADER_SIZE;
		}

		/// Save the final bundle
		Bool Save( const String& absolutePath )
		{
			// open file
			Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath ) );
			if ( !file )
			{
				ERR_WCC( TXT("Unable to open file '%ls' for writing"), absolutePath.AsChar() );
				return false;
			}

			// sort entries to put the buffers at the end
			::Sort( m_entries.Begin(), m_entries.End() );

			// write header
			Red::Core::Bundle::SBundleHeaderPreamble preamble;
			preamble.m_bundleStamp = Red::Core::Bundle::BUNDLE_STAMP;
			preamble.m_burstSize = 0; // no burst read for this bundle
			preamble.m_fileSize = 0; // known after all is saved (Uint32)( m_totalDataSize + m_currentOffset );
			preamble.m_headerSize = m_currentOffset - sizeof(preamble);
			file->Serialize( &preamble, sizeof(preamble) );

			// prepare a place for the data offset of the entries
			TDynArray< Uint64 > dataOffsets;
			dataOffsets.Reserve( m_entries.Size() );

			// total size of compressed data
			Uint64 totalCompressedSize = 0;
			Uint64 totalUncompressedSize = 0;

			// write the elements allocating the space in the file as we go
			const Uint64 fileAlignment = 4096;
			Uint64 currentDataOffset = AlignOffset( m_currentOffset, fileAlignment );
			for ( const Entry& info : m_entries )
			{
				// stats
				LOG_WCC( TXT("Compressing '%hs'..."), info.m_bundleFilePath.AsChar() );

				// Move to write position
				file->Seek( currentDataOffset );

				// bundle is to big
				if ( currentDataOffset > UINT_MAX )
				{
					ERR_WCC( TXT("Cannot add file '%ls' because the resulting bundle would be bigger than 4GB"), 
						info.m_absoluteFilePath.AsChar() );
					return false;
				}

				// Store file data
				auto compression = info.m_compression;
				Uint32 sizeInMemory = 0;
				Uint32 sizeOnDisk = 0;
				Uint32 crc = 0;
				if ( !StoreFile( file.Get(), info.m_absoluteFilePath, compression, sizeInMemory, sizeOnDisk, crc ) )
				{
					ERR_WCC( TXT("Unable to open and compress source file '%ls'"), 
						info.m_absoluteFilePath.AsChar() );
					return false;
				}

				// prepare the data to write, use the same settings as in the original file (compression especially)
				Red::Core::Bundle::SBundleHeaderItem bundleItem;
				Red::MemoryZero( &bundleItem, sizeof(bundleItem) );
				bundleItem.m_compressionType = compression;
				bundleItem.m_dataOffset = (Uint32) currentDataOffset;
				bundleItem.m_dataSize = sizeInMemory;
				bundleItem.m_compressedDataSize = sizeOnDisk;
				bundleItem.m_cookedResourceCRC = crc;

				// copy resource path
				Red::System::StringCopy( bundleItem.m_rawResourcePath, info.m_bundleFilePath.AsChar(), Red::Core::Bundle::SBundleHeaderItem::RAW_RESOURCE_PATH_MAX_SIZE );

				// store it
				file->Seek( info.m_entryOffset );
				file->Serialize( (void*)&bundleItem, sizeof(bundleItem) );

				// advance the write offset for the actual file data
				currentDataOffset += AlignOffset( bundleItem.m_compressedDataSize, fileAlignment );

				// stats
				totalCompressedSize += bundleItem.m_compressedDataSize;
				totalUncompressedSize += bundleItem.m_dataSize;
			}

			// update header
			preamble.m_fileSize = (Uint32) currentDataOffset;
			file->Seek( 0 );
			file->Serialize( &preamble, sizeof(preamble) );

			// bundle saved
			LOG_WCC( TXT("Bundle '%ls' saved, %d files, %1.2f KB of data"), 
				absolutePath.AsChar(), m_entries.Size(), currentDataOffset / 1024.0f );
			return true;
		}

	private:
		struct Entry
		{
			String									m_absoluteFilePath;
			StringAnsi								m_bundleFilePath;
			Uint64									m_entryOffset;
			Uint64									m_dataSize; // on disk
			Uint32									m_order;
			Red::Core::Bundle::ECompressionType		m_compression;

			RED_INLINE const Bool operator<( const Entry& other ) const
			{
				return m_order < other.m_order;
			}
		};

		template< typename T >
		Bool CompressToFile( const void* sourceData, const Uint32 sourceDataSize, IFile& target, Uint32& outCompressedDataSize )
		{
			T compressor;
			if ( !compressor.Compress( sourceData, sourceDataSize ) )
				return false;

			// compression is not successful
			if ( compressor.GetResultSize() >= sourceDataSize )
				return false; // no need to store file if it's bigger than compressed version

			target.Serialize( (void*)compressor.GetResult(), compressor.GetResultSize() );
			outCompressedDataSize = compressor.GetResultSize();
			return true;
		}

		Bool StoreFile( IFile* target, const String& absoluteFilePath, Red::Core::Bundle::ECompressionType& inOutCompression, Uint32& outMemorySize, Uint32& outDiskSize, Uint32& outCrc )
		{
			// Open source file
			Red::TScopedPtr< IFile > srcFile( GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath ) );
			if ( !srcFile )
				return false;

			// Source data size
			const Uint64 dataSize = (Uint64) srcFile->GetSize();
			if ( dataSize > UINT_MAX )
				return false; // file to big to store

			// Remember source size
			outMemorySize = (Uint32) dataSize;

			// file to big to be compressed
			if ( dataSize > 32*1024*1024 )
				inOutCompression = Red::Core::Bundle::CT_Uncompressed;

			outCrc = 0;

			// uncompressed file - copy raw data 
			if ( inOutCompression == Red::Core::Bundle::CT_Uncompressed )
			{
				outDiskSize = (Uint32) dataSize;
				PatchUtils::CopyFileContent( *srcFile, *target, outCrc );
				return true;
			}

			// load file data into the memory
			void* tempBuffer = PatchUtils::AllocateTempMemory();
			srcFile->Serialize( tempBuffer, dataSize );

			Red::System::CRC32 crcCalculator;
			outCrc = crcCalculator.Calculate( tempBuffer, static_cast< Uint32 >( dataSize ), outCrc );

			// compress file
			bool compressed = false;
			switch ( inOutCompression )
			{
				case Red::Core::Bundle::CT_Doboz:
					compressed = CompressToFile< Red::Core::Compressor::CDoboz >(tempBuffer, (Uint32)dataSize, *target, outDiskSize );
					break;

				case Red::Core::Bundle::CT_Snappy:
					compressed = CompressToFile< Red::Core::Compressor::CSnappy >(tempBuffer, (Uint32)dataSize, *target, outDiskSize );
					break;

				case Red::Core::Bundle::CT_Zlib:
					compressed = CompressToFile< Red::Core::Compressor::CZLib >(tempBuffer, (Uint32)dataSize, *target, outDiskSize );
					break;

				case Red::Core::Bundle::CT_LZ4HC:
					compressed = CompressToFile< Red::Core::Compressor::CLZ4HC >(tempBuffer, (Uint32)dataSize, *target, outDiskSize );
					break;

				case Red::Core::Bundle::CT_LZ4:
					compressed = CompressToFile< Red::Core::Compressor::CLZ4 >(tempBuffer, (Uint32)dataSize, *target, outDiskSize );
					break;
			}

			// compression failed for some reason
			if ( !compressed )
			{
				ERR_WCC( TXT("Compression of file '%ls' failed for some reason. Storing as uncompressed."), absoluteFilePath.AsChar() );
				outDiskSize = (Uint32) dataSize;
				inOutCompression = Red::Core::Bundle::CT_Uncompressed;
				return true;
			}

			// not compressed
			return true;
		}

		Uint32							m_currentOffset;
		Uint64							m_totalDataSize;
		TDynArray< Entry >				m_entries;
	};

	class CFileGrouping
	{
	public:
		struct Entry
		{
			String									m_relativePath;
			String									m_absolutePath;
			Red::Core::Bundle::ECompressionType		m_compression;
		};

		struct Bundle
		{
			String					m_name;
			TDynArray< Entry >		m_sources;
			Uint64					m_totalSize; // so far

			RED_INLINE Bundle( const String&  name )
				: m_name( name )
				, m_totalSize( 0 )
			{}

			Bool SaveToDirectory( const String& absoluteOutputPath ) const
			{
				CPackedBundleWriter writer;

				// add entries
				for ( const auto& source : m_sources )
				{
					writer.AddEntry( source.m_absolutePath, source.m_relativePath, source.m_compression );
				}

				// format bundle name
				const String bundleOutputFilePath = String::Printf( TXT("%ls%ls.bundle"), 
					absoluteOutputPath.AsChar(), m_name.AsChar() );
				return writer.Save( bundleOutputFilePath );
			}
		};

		struct ContentGroup
		{
			String				m_name;
			Bundle*				m_current;
			TDynArray< Bundle >	m_bundles;

			RED_INLINE ContentGroup( const Char* name )
				: m_current(nullptr)
				, m_name( name )
			{}

			void AddFile( const Uint64 dataSize, const String& relativeSourcePath, const String& absoluteSourcePath, Red::Core::Bundle::ECompressionType compression )
			{
				// will fit ?
				const Uint64 maxBundleSize = (Uint64)3950 * (Uint64)1024 * (Uint64)1024;
				if ( !m_current || (m_current->m_totalSize + dataSize) >= maxBundleSize )
				{
					// format bundle name
					const String bundleName = String ::Printf( TXT("%ls%d"), m_name.AsChar(), m_bundles.Size() );
					m_bundles.PushBack( Bundle( bundleName ) );
					m_current = &m_bundles.Back();
				}

				// add file
				m_current->m_totalSize += dataSize;

				// add entry
				Entry info;
				info.m_absolutePath = absoluteSourcePath;
				info.m_relativePath = relativeSourcePath;
				info.m_compression = compression;
				m_current->m_sources.PushBack( info );					
			}

			Bool SaveToDirectory( const String& absoluteOutputPath ) const
			{
				for ( const Bundle& bundle : m_bundles )
				{
					if ( !bundle.SaveToDirectory( absoluteOutputPath ) )
					{
						ERR_WCC( TXT("Failed to save bundle '%ls' from group '%ls'"), 
							bundle.m_name.AsChar(), m_name.AsChar() );
						return false;
					}
				}

				return true;
			}

			void RemoveBundles( const String& absoluteOutputPath ) const
			{
				for ( const Bundle& bundle : m_bundles )
				{
					const String bundleOutputFilePath = String::Printf( TXT("%ls%ls.bundle"), 
						absoluteOutputPath.AsChar(), bundle.m_name.AsChar() );

					GFileManager->DeleteFile( bundleOutputFilePath );
				}
			}
		};

		ContentGroup			m_buffers;
		ContentGroup			m_content;

		CFileGrouping()
			: m_buffers( TXT("buffers") )
			, m_content( TXT("blob") )
		{}

		void AddEntry( const String& relativeSourcePath, const String& absoluteFilePath, Red::Core::Bundle::ECompressionType compression )
		{
			const Uint64 dataSize = GFileManager->GetFileSize( absoluteFilePath );

			// skip empty file
			if ( dataSize == 0 )
				return;

			// is this file a buffer ?
			const Bool isBuffer = relativeSourcePath.ContainsSubstring( TXT(".buffer") );
			ContentGroup& content = isBuffer ? m_buffers : m_content;

			// add to content group
			content.AddFile( dataSize, relativeSourcePath, absoluteFilePath, compression );
		}

		void RemoveBundles( const String& absoluteOutputPath ) const		
		{
			m_buffers.RemoveBundles( absoluteOutputPath );
			m_content.RemoveBundles( absoluteOutputPath );
		}


		Bool SaveToDirectory( const String& absoluteOutputPath ) const
		{
			if ( !m_content.SaveToDirectory( absoluteOutputPath ) )
				return false;

			if ( !m_buffers.SaveToDirectory( absoluteOutputPath ) )
				return false;

			return true;
		}

	};

	void FixPath( String& path )
	{
		// append path ends
		if ( !path.EndsWith(TXT("\\")) && !path.EndsWith(TXT("/")) )
			path += TXT("\\");
	}
}

bool CPackCommandlet::Execute( const CommandletOptions& options )
{
	// get out path
	String outPath;
	if ( !options.GetSingleOptionValue( TXT("outdir"), outPath ) )
	{
		ERR_WCC( TXT("Missing output file path") );
		return false;
	}

	// get input directory
	String baseDir;
	if ( !options.GetSingleOptionValue( TXT("dir"), baseDir ) )
	{
		ERR_WCC( TXT("Missing intput directory") );
		return false;
	}

	// fix paths
	Helper::FixPath( outPath );
	Helper::FixPath( baseDir );

	// get compression type
	Red::Core::Bundle::ECompressionType compression = Red::Core::Bundle::ECompressionType::CT_LZ4HC;
	{
		String compName;
		if ( options.GetSingleOptionValue( TXT("compression"), compName ) )
		{
			if ( compName.EqualsNC( TXT("lz4") ) )
			{
				compression = Red::Core::Bundle::ECompressionType::CT_LZ4;
			}
			else if ( compName.EqualsNC( TXT("lz4hc") ) )
			{
				compression = Red::Core::Bundle::ECompressionType::CT_LZ4HC;
			}
			else if ( compName.EqualsNC( TXT("zlib") ) )
			{
				compression = Red::Core::Bundle::ECompressionType::CT_Zlib;
			}
			else if ( compName.EqualsNC( TXT("doboz") ) )
			{
				compression = Red::Core::Bundle::ECompressionType::CT_Doboz;
			}
			else if ( compName.EqualsNC( TXT("snappy") ) )
			{
				compression = Red::Core::Bundle::ECompressionType::CT_Snappy;
			}
			else if ( compName.EqualsNC( TXT("none") ) )
			{
				compression = Red::Core::Bundle::ECompressionType::CT_Uncompressed;
			}
			else
			{
				ERR_WCC( TXT("Unknown compression type: '%ls'"), compName.AsChar() );
				return false;
			}
		}
	}

	// get files from folder (with local paths)
	TDynArray< String > localFiles;
	GFileManager->FindFilesRelative( baseDir, String::EMPTY, TXT("*.*"), localFiles, true );
	if ( localFiles.Empty() )
	{
		ERR_WCC( TXT("There are no file in directory '%ls'"), baseDir.AsChar() );
		return false;
	}

	// add files to writer, NOTE: we may have more data that fits in one bundle (4GB) if this happens than we split 
	Helper::CFileGrouping grouping;
	for ( const String& localPath : localFiles )
	{
		// do not store the .db files
		if ( localPath.EndsWith( TXT(".db") ) )
			continue;

		// do not store the .cache files
		if ( localPath.EndsWith( TXT(".cache") ) )
			continue;

		const String absolutePath = baseDir + localPath;
		grouping.AddEntry( localPath, absolutePath, compression ); // use suggested compression for all files
	}

	// save the grouped files into bundles
	if ( !grouping.SaveToDirectory( outPath ) )
	{
		ERR_WCC( TXT("There were errors saving files") );
		grouping.RemoveBundles( outPath );
		return false;
	}

	// bundles saved
	return true;
}

void CPackCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  pack -dir=<directory> -outdir=<mod.bundle> [-compression=LZ4|LZ4HC|ZLIB]" ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "  -dir = Input directory to pack" ) );
	LOG_WCC( TXT( "  -outdir = Output directory with bundles (note: bundles bigger than 4GB are split)" ) );
	LOG_WCC( TXT( "  -compression = Compression type to use (default: LZ4HC)" ) );
}
