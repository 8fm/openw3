/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "bundle.h"

#include "bundleBuilderMemory.h"

#include "../../common/core/bundleHeader.h"
#include "../../common/core/bundledefinition.h"

#include "../../common/core/compression/zlib.h"
#include "../../common/core/compression/snappy.h"
#include "../../common/core/compression/doboz.h"
#include "../../common/core/compression/lz4.h"
#include "../../common/core/compression/lz4hc.h"

#include "compressionProfiler.h"
#include "autoCache.h"

namespace Bundler {

using Red::Core::Bundle::SBundleHeaderPreamble;
using Red::Core::Bundle::SBundleHeaderItem;

#define CALCULATE_PADDING_NOTEST( size, alignment )	( alignment - ( size & ( alignment - 1 ) ) )
#define CALCULATE_PADDING( size, alignment )		( ( size % alignment )? CALCULATE_PADDING_NOTEST( size, alignment ) : 0 )
#define DEFERRED_DATA_BUFFER_TYPE 1717990754

//////////////////////////////////////////////////////////////////////////
// Bundle Header

//////////////////////////////////////////////////////////////////////////
CBundleHeader::CBundleHeader()
{

}

//////////////////////////////////////////////////////////////////////////
CBundleHeader::~CBundleHeader()
{
	m_bundleItems.ClearPtr();
	m_dataPaths.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CBundleHeader::SetPreamble( const SBundleHeaderPreamble& preamble )
{
	m_preamble = preamble;
}

//////////////////////////////////////////////////////////////////////////
void CBundleHeader::AddItem( const SBundleHeaderItem& bundleHeaderItem, const StringAnsi& dataPath )
{
	SBundleHeaderItem* headerItem = new SBundleHeaderItem( bundleHeaderItem );
	m_bundleItems.PushBack(headerItem);

	m_dataPaths.PushBack( dataPath );

	RED_FATAL_ASSERT( m_bundleItems.Size() == m_dataPaths.Size(), "Arrays are not in sync" );
}

//////////////////////////////////////////////////////////////////////////
Uint32 CBundleHeader::GetBundleDataSize() const
{
	Uint32 totalSize = 0;

	const Uint32 numItems = m_bundleItems.Size();
	for( Uint32 i = 0; i < numItems; ++i )
	{
		totalSize += m_bundleItems[ i ]->m_dataSize; 
	}

	return totalSize;
}

//////////////////////////////////////////////////////////////////////////
// Bundle the bundler will create.

//////////////////////////////////////////////////////////////////////////
CBundle::CBundle( const BundleCreationParameters& creationParameters )
:	m_feedback( creationParameters.m_feedback )
,	m_autoCache( creationParameters.m_autoCache )
,	m_options( creationParameters.m_options )
,	m_writer( creationParameters.m_writer )
,	m_crcCalculator( creationParameters.m_crcCalculator )
{
	const AnsiChar* name = creationParameters.m_bundleDataContainer->GetBundleName();
	Red::System::StringCopy( m_bundleName, name, BUNDLE_NAME_MAX_SIZE );
}

//////////////////////////////////////////////////////////////////////////
CBundle::~CBundle()
{
}

//////////////////////////////////////////////////////////////////////////
void CBundle::AddItem( const SBundleHeaderItem& fileDesc, const StringAnsi& dataPath )
{
	m_bundleHeader.AddItem( fileDesc, dataPath );
}

void CBundle::Construct( Uint8* buffer, Uint32 size )
{
	if( CreateFile() )
	{
		WriteBuffer( buffer, size );

		m_writer.Close();
	}
}

//////////////////////////////////////////////////////////////////////////
Bool CBundle::CreateFile()
{
	StringAnsi path = m_options.GetOutputDirectory() + m_bundleName;

	GFileManager->CreatePath( ANSI_TO_UNICODE( path.AsChar() ) );

	if( !m_writer.Open( path.AsChar() ) )
	{
		BUNDLE_ERROR( m_feedback, TXT( "Failed to open bundle file for writing: %hs" ), path.AsChar() );

		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundle::WriteBuffer( Uint8* buffer, Uint32 bufferSize )
{
	// Grab the data size.
	const Uint32 bundleDataSize = m_bundleHeader.GetBundleDataSize(); 
	if( bundleDataSize == 0 )
	{
		return false;
	}

	// Grab the header size.
	const Uint32 preambleOffset		= 0;
	const Uint32 headerOffset		= preambleOffset + sizeof( SBundleHeaderPreamble );
	const Uint32 headerItemPadding	= CALCULATE_PADDING( sizeof( SBundleHeaderItem ), 16 );
	const Uint32 headerItemCount	= m_bundleHeader.GetHeaderCount();
	const Uint32 headerSize			= headerItemCount * ( sizeof( SBundleHeaderItem ) + headerItemPadding );
	      Uint32 fileSize			= headerOffset + headerSize;
		  Uint32 burstSize			= 0; // Burst read is always offset from the start of the data block.

	FeedbackHelper feedbackHelper( bundleDataSize + ( headerItemCount * sizeof( SBundleHeaderItem ) ) + sizeof( SBundleHeaderPreamble ), m_feedback );

	// Go to data position
	RED_VERIFY( m_writer.Seek( fileSize ), TXT( "Unable to seek to the data section of the bundle" ) );

	// Once we get to the deferred buffer files, we no longer want to increment the burst read size.
	Bool deferredBufferFiles = false;

	if( m_autoCache )
	{
		m_autoCache->Initialize( m_bundleName );
	}

	Uint8 alignmentZeroedBuffer[ 4096 ];
	Red::MemoryZero( &alignmentZeroedBuffer[0], sizeof(alignmentZeroedBuffer) );

	// HACK: align non startup bundles to 4K
	const Bool genericBundle = (Red::StringSearch( m_bundleName, "startup" ) == nullptr);
	const Uint32 dataAlignment = genericBundle ? 4096 : 1;

	// Fill in data
	for( Uint32 i = 0; i < headerItemCount; ++i )
	{
		// add padding/alignment data
		if ( dataAlignment > 1 )
		{
			const Uint64 alignmentOffset = (fileSize + (dataAlignment-1)) & ~(dataAlignment-1);
			const Uint64 toWrite = alignmentOffset - fileSize;

			RED_VERIFY( m_writer.Write( alignmentZeroedBuffer, (Uint32)toWrite ), TXT( "Unable to write data item padding: %u" ), i );
			fileSize = (Uint32) alignmentOffset;
		}

		m_bundleHeader[ i ]->m_dataOffset = fileSize;

		Uint32 expectedSize = m_bundleHeader[ i ]->m_dataSize;

		Bool success = false;

		const AnsiChar* dataPath = m_bundleHeader.GetDataPath( i ).AsChar();

		if( expectedSize > bufferSize )
		{
			success = WriteLargeBufferItem( dataPath, m_bundleHeader[ i ], m_writer, buffer, bufferSize );
		}
		else
		{
			if( WriteBufferItem( dataPath, m_bundleHeader[ i ], buffer, bufferSize ) )
			{
				success = true;

				RED_VERIFY( m_writer.Write( buffer, m_bundleHeader[ i ]->m_compressedDataSize ), TXT( "Unable to write data item %u" ), i );

				if( m_autoCache )
				{
					m_autoCache->AddResult( m_bundleHeader[ i ]->m_rawResourcePath, m_bundleHeader[ i ]->m_compressionType );
				}
			}
		}

		if( success )
		{
			fileSize += m_bundleHeader[ i ]->m_compressedDataSize;
			
			// We've hit a deferred buffer? Now it's time to stop incrementing burst read size. Switch the flag to turn it off.
			if( m_bundleHeader[ i ]->m_resourceType == DEFERRED_DATA_BUFFER_TYPE )
			{
				deferredBufferFiles = true;
			}

			if( !deferredBufferFiles )
			{
				burstSize += m_bundleHeader[ i ]->m_compressedDataSize;
			}

			if( m_bundleHeader[ i ]->m_compressedDataSize % 16 != 0 )
			{
				const AnsiChar* alignmentBuffer = "AlignmentUnused";
				const Uint16 alignmentAdjust	= CALCULATE_PADDING_NOTEST( m_bundleHeader[ i ]->m_compressedDataSize, 16 );

				RED_VERIFY( m_writer.Write( alignmentBuffer, alignmentAdjust ), TXT( "Unable to write data item padding: %u" ), i );
				
				fileSize += alignmentAdjust;

				if( !deferredBufferFiles )
				{
					burstSize += alignmentAdjust;
				}
			}

			// If the file-size has changed, we need to update the total we're counting towards
			Uint32 actualSize = m_bundleHeader[ i ]->m_dataSize;
			if( expectedSize != actualSize )
			{
				feedbackHelper.AdjustTotal( actualSize - expectedSize );
			}

			feedbackHelper.Update( actualSize );
		}
		else
		{
			return false;
		}
	}

	// Fill in preamble
	SBundleHeaderPreamble preamble	= m_bundleHeader.GetPreamble();

	preamble.m_fileSize				= fileSize;
	preamble.m_burstSize			= burstSize;
	preamble.m_headerSize			= headerSize;
	preamble.m_headerVersion		= Red::Core::Bundle::BUNDLE_HEADER_VERSION;
	// These need to be at least configured for the preamble to be useful.
	// Set to some defaults for now - we a view to being configurable later.
	preamble.m_configuration		= EBundleHeaderConfiguration::BHC_Debug;
	preamble.m_bundleStamp			= BUNDLE_STAMP;

	RED_VERIFY( m_writer.Seek( 0 ), TXT( "Unable to seek to beginning of file" ) );
	RED_VERIFY( m_writer.Write( &preamble, sizeof( SBundleHeaderPreamble ) ), TXT( "Unable to write Preamble" ) );

	feedbackHelper.Update( sizeof( SBundleHeaderPreamble ) );

	// Fill in header
	for( Uint32 i = 0; i < headerItemCount; ++i )
	{
		SBundleHeaderItem* itemHeader = m_bundleHeader[ i ];
		RED_VERIFY( m_writer.Write( itemHeader, sizeof( SBundleHeaderItem ) ), TXT( "Unable to write header item: %u" ), i );

		if( headerItemPadding > 0 )
		{
			RED_ASSERT( headerItemPadding < 16 );

			const AnsiChar* alignmentBuffer = "AlignmentUnused";
			RED_VERIFY( m_writer.Write( alignmentBuffer, headerItemPadding ), TXT( "Unable to write header item padding: %u" ), i );

			fileSize += headerItemPadding;
		}

		feedbackHelper.Update( sizeof( SBundleHeaderItem ) );
	}

	if( m_feedback )
	{
		m_feedback->MarkCompleted( Feedback::CS_Success );
	}

	return true;
}

RED_INLINE Bool CBundle::MeetsSizeThreshold( Uint32 size ) const
{
	const Uint32 MAXIMUM_SIZE_THRESHOLD = c_BundleBuilderPoolGranularity - ( 128U * c_oneMegabyte );
	return size >= MINIMUM_SIZE_THRESHOLD && size < MAXIMUM_SIZE_THRESHOLD;
}

Bool CBundle::WriteLargeBufferItem( const AnsiChar* dataPath, SBundleHeaderItem* item, CBufferedFileWriter& writer, Uint8* buffer, Uint32 bufferSize ) const
{
	Bool retVal = false;

	RED_FATAL_ASSERT( item->m_dataSize > bufferSize, "Wrong path taken, this file should have gone through WriteBufferItem()" );

	FILE* file = fopen( dataPath, "rb" );

	Uint32 totalAmountRead = 0;

	if( file )
	{
		// Ensure the compression type is valid
		if( item->m_compressionType != CT_Uncompressed )
		{
			BUNDLE_WARNING( m_feedback, TXT( "Cannot compress files larger than %u (%u): %hs" ), bufferSize, item->m_dataSize, dataPath );
			item->m_compressionType = CT_Uncompressed;
		}

		Uint32 crc = 0;

		// Primary read and write loop
		while( feof( file ) == 0 )
		{
			Uint32 amountRead = static_cast< Uint32 >( fread( buffer, sizeof( Uint8 ), bufferSize, file ) );
			totalAmountRead += amountRead;

			crc = m_crcCalculator.Calculate( buffer, amountRead, crc );

			writer.Write( buffer, amountRead );
		}

		item->m_cookedResourceCRC = crc;
		item->m_compressedDataSize = totalAmountRead;

		fclose( file );

		if( item->m_compressedDataSize != item->m_dataSize )
		{
			BUNDLE_WARNING( m_feedback, TXT( "File on disk (%hs) has inconsistent filesize: %u (Expected %u)" ), dataPath, item->m_compressedDataSize, item->m_dataSize );
		}

		retVal = true;
	}
	else
	{
		BUNDLE_ERROR( m_feedback, TXT( "File is missing: %hs" ), dataPath );
	}

	return retVal;
}

Bool CBundle::WriteBufferItem( const AnsiChar* dataPath, SBundleHeaderItem* item, Uint8* buffer, Uint32 bufferSize ) const
{
	using namespace Red::Core;

	const Uint32 readInBufferSize	= bufferSize;
	const Uint32 expectedReadSize	= item->m_dataSize;
	Uint32& actualReadSize			= item->m_dataSize;
	Uint32& sizeAfterCompression	= item->m_compressedDataSize;

	if( readInBufferSize < expectedReadSize )
	{
		BUNDLE_ERROR( m_feedback, TXT( "File is too large for read in buffer! Expected file size: %u, Buffer size: %u" ), expectedReadSize, readInBufferSize );

		return false;
	}

	FILE* file = fopen( dataPath, "rb" );
	if( file )
	{
		void* uncompressedDataBuffer = nullptr;

		uncompressedDataBuffer = AllocateUncompressedBuffer( item->m_compressionType, buffer, expectedReadSize );

		Uint32 amountRead = static_cast< Uint32 >( fread( uncompressedDataBuffer, sizeof( Uint8 ), expectedReadSize, file ) );

		// Force EOF (We need to read off the end of the buffer, which should only be 1 byte away
		Uint8 temporaryStorage;
		fread( &temporaryStorage, sizeof( Uint8 ), 1, file );

		if( feof( file ) == 0 )
		{
			// File on disk is larger than was specified in the definition

			// Move to end end of the file
			RED_VERIFY( fseek( file, 0, SEEK_END ) == 0, TXT( "Unable to seek to end of file" ) );

			// Find out how big the file actually is
			actualReadSize = ftell( file );

			// Move back to where we stopped reading
			RED_VERIFY( fseek( file, amountRead, SEEK_SET ) == 0, TXT( "Unable to seek back to where we were" ) );

			// Re-adjust the overall data buffer
			if( readInBufferSize < expectedReadSize )
			{
				BUNDLE_ERROR( m_feedback, TXT( "File is too large for read in buffer! Actual file size: %u, Buffer size: %u" ), expectedReadSize, readInBufferSize );

				fclose( file );

				return false;
			}

			if( item->m_compressionType != Bundle::CT_Uncompressed )
			{
				// Increase the size of the buffer so it can safely contain all the information
				uncompressedDataBuffer = BUNDLER_MEMORY_REALLOCATE( uncompressedDataBuffer, MC_BundlerInputBuffer, actualReadSize );
			}

			// Read the rest of the file into the buffer
			Uint32 sizeDifference = actualReadSize - amountRead;
			Uint8* typedBuffer = static_cast< Uint8* >( uncompressedDataBuffer );
			amountRead += static_cast< Uint32 >( fread( typedBuffer + amountRead, sizeof( Uint8 ), sizeDifference, file ) );

			// 
			RED_FATAL_ASSERT( amountRead == actualReadSize, "Our size adjustment has failed, amountRead (%u) does not match what we calculated to be on disk (%u)", amountRead, actualReadSize );
			BUNDLE_WARNING( m_feedback, TXT( "File on disk (%hs) is larger (%u) than expected (%u)" ), item->m_rawResourcePath, actualReadSize, expectedReadSize );
		}
		else if( amountRead < item->m_dataSize )
		{
			BUNDLE_WARNING( m_feedback, TXT( "File on disk (%hs) is smaller (%u) than expected (%u)" ), item->m_rawResourcePath, amountRead, expectedReadSize );

			// Update header
			actualReadSize = amountRead;
		}

		fclose( file );

		CalculateCRC( item, uncompressedDataBuffer, actualReadSize );

		// Force to uncompressed if it's too small
		if( !MeetsSizeThreshold( actualReadSize ) )
		{
			item->m_compressionType = Bundle::CT_Uncompressed;
		}
		else  if( item->m_compressionType == Bundle::CT_Auto )
		{
			if( !m_autoCache || !m_autoCache->GetResult( item->m_rawResourcePath, item->m_compressionType ) )
			{
				item->m_compressionType = CT_Zlib;
			}
		}

		// Now that we've sorted out which compression type to use, let's 
		if( item->m_compressionType == Bundle::CT_Uncompressed )
		{
			//Since we've fallen back to uncompressed, we need to copy the data directly into the output buffer
			Red::System::MemoryCopy( buffer, uncompressedDataBuffer, actualReadSize );

			sizeAfterCompression = actualReadSize;
		}
		else
		{
			// Compression time!
			Compressor::Base* compressor = CreateCompressor( item );

			RED_VERIFY( compressor->Compress( uncompressedDataBuffer, actualReadSize ), TXT( "Could not compress %hs" ), item->m_rawResourcePath );

			sizeAfterCompression = compressor->GetResultSize();

			if( sizeAfterCompression < actualReadSize )
			{
				// This is what we expect to happen after compression
				Red::System::MemoryCopy( buffer, compressor->GetResult(), sizeAfterCompression );
			}
			else
			{
				// The file has emerged from decompression larger than it went in
				// So we need to fall back to uncompressed instead
				BUNDLE_WARNING( m_feedback, TXT( "File (%hs) is larger after compression: %u (compressed using %u) vs %u (uncompressed)" ), item->m_rawResourcePath, sizeAfterCompression, item->m_compressionType, actualReadSize );

				sizeAfterCompression = actualReadSize;
				item->m_compressionType = Bundle::CT_Uncompressed;

				Red::System::MemoryCopy( buffer, uncompressedDataBuffer, actualReadSize );
			}

			// Release temporary allocation used to store the uncompressed data
			BUNDLER_MEMORY_FREE( MC_BundlerInputBuffer, uncompressedDataBuffer );
			uncompressedDataBuffer = nullptr;

			delete compressor;
		}

		return true;
	}
	else
	{
		BUNDLE_ERROR( m_feedback, TXT( "File is missing: %hs" ), dataPath );
	}

	sizeAfterCompression = 0;

	return false;
}

RED_INLINE Bool CBundle::MeetsCompressionRatioThreshold( Uint32 uncompressedSize, const SProfilerItem& profilingData ) const
{
	// -1 because we don't profile CT_Uncompressed
	Float compressedSize = static_cast< Float >( profilingData.m_averages[ profilingData.m_best - 1 ].m_compressedSize );
	Float compressionRatio = compressedSize / uncompressedSize;

	return compressionRatio < m_options.GetMinimumCompressionRatio();
}

void CBundle::SelectCompressionType( Red::Core::Bundle::ECompressionType& compressionType, const void* buffer, Uint32 size ) const
{
	using namespace Red::Core;

	RED_FATAL_ASSERT( MeetsSizeThreshold( size ), "File is too small for compression" );
	RED_FATAL_ASSERT( compressionType == Bundle::CT_Auto, "Compression type was already specified explicitly" );

	compressionType = Bundle::CT_Uncompressed;

	// Run the profiler to determine which compression type to use
	SProfilerItem profilingData;
	CProfilerCommon profiler;

	profiler.Profile( buffer, size, profilingData, m_options.GetCompressionIterations() );

	if( MeetsCompressionRatioThreshold( size, profilingData ) )
	{
		// Acceptable amount of compression
		compressionType = profilingData.m_best;
	}
}

Red::Core::Compressor::Base* CBundle::CreateCompressor( const SBundleHeaderItem* item ) const
{
	using namespace Red::Core;
	Compressor::Base* compressor = nullptr;

	switch( item->m_compressionType )
	{
	case Bundle::CT_Zlib:
		compressor = new Compressor::CZLib;
		break;

	case Bundle::CT_Snappy:
		compressor = new Compressor::CSnappy;
		break;

	case Bundle::CT_Doboz:
		compressor = new Compressor::CDoboz;
		break;

	case Bundle::CT_LZ4:
		compressor = new Compressor::CLZ4;
		break;

	case Bundle::CT_LZ4HC:
		compressor = new Compressor::CLZ4HC;
		break;

	default:
		RED_HALT( "Unknown compressor type: %i", item->m_compressionType );
	}

	return compressor;
}

void* CBundle::AllocateUncompressedBuffer( Red::Core::Bundle::ECompressionType compressionType, void* compressedBuffer, Uint32 size ) const
{
	using namespace Red::Core;

	if( compressionType == Bundle::CT_Uncompressed )
	{
		// No compression selected - read the file directly into the bundle buffer
		return compressedBuffer;
	}
	else
	{
		// First read the uncompressed data into a temporary buffer
		return BUNDLER_MEMORY_ALLOCATE( MC_BundlerInputBuffer, size );
	}
}

void CBundle::CalculateCRC( SBundleHeaderItem* item, const void* buffer, Uint32 size ) const
{
	item->m_cookedResourceCRC = m_crcCalculator.Calculate( buffer, size );
}

CBundle::FeedbackHelper::FeedbackHelper( Uint32 total, Feedback* feedback )
:	m_total( static_cast< Float >( total ) )
,	m_progress( 0 )
,	m_feedback( feedback )
{
}

void CBundle::FeedbackHelper::Update( Uint32 increment )
{
	m_progress += increment;

	if( m_feedback )
	{
		m_feedback->SetProgress( m_progress / m_total );
	}
}

void CBundle::FeedbackHelper::AdjustTotal( Int32 adjustment )
{
	m_total += adjustment;
}

} // namespace Bundler {
