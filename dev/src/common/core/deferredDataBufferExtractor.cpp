/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "deferredDataBufferExtractor.h"
#include "dependencySaver.h"
#include "resource.h"
#include "configVar.h"
#include "fileSys.h"
#include "../redSystem/crc.h"

namespace Config
{
	TConfigVar< Int32, Validation::IntRange<0, 65536> > cvDeferedBufferExtractionSizeLimit( "Cooking", "DeferedBufferExtractionSizeLimit", eConsoleVarFlag_ReadOnly );
} // Config

const Char* DeferredDataBufferExtractor::TEMPLATE_NAME = TXT( "%ls.%d.buffer" );

DeferredDataBufferExtractor::DeferredDataBufferExtractor( const CFileManager* fileManager, const String& rootPath, const String& resourcePath, const Uint32 sizeLimit )
	: m_fileManager( fileManager )
	, m_resourcePath( resourcePath )
	, m_rootPath( rootPath )
	, m_sizeLimit( sizeLimit )
{}

DeferredDataBufferExtractor::~DeferredDataBufferExtractor()
{}

bool DeferredDataBufferExtractor::Extract( const Uint32 bufferId, const Uint32 bufferSizeInMemory, const void* bufferData, Uint32& outBufferSizeOnDisk, Uint32& outBufferSavedDataCRC, String& outBufferFilePath ) const
{
	RED_FATAL_ASSERT( bufferSizeInMemory != 0, "Trying to extract buffer with no data - it should be filtered earlier" );
	RED_FATAL_ASSERT( bufferData != nullptr, "Trying to extract buffer with no data - it should be filtered earlier" );

	// TODO: do not extract small buffers

	// generate extraction path
	String extractedFilePath;
	GenerateFileName( m_resourcePath, bufferId, /*out*/ extractedFilePath );

	// open target file
	Red::TScopedPtr< IFile > file( m_fileManager->CreateFileWriter( m_rootPath + extractedFilePath, FOF_AbsolutePath ) );
	if ( !file )
	{
		ERR_CORE( TXT("Failed to open target file for buffer %d from resource '%ls' into '%ls'. Buffer will stay embedded."), 
			bufferId, m_resourcePath.AsChar(), extractedFilePath.AsChar() );
		return false;
	}

	// TODO: we can compress the data here, it's much better than relay on the bundle builder to do it as we can know the format of the data much better
	// NOTE: if we add compression the CRC should be calculated from the data ON DISK

	// no compression case
	{
		// save the data
		file->Serialize( (void*)bufferData, bufferSizeInMemory );
		outBufferSizeOnDisk = bufferSizeInMemory; // no compression for now
		outBufferFilePath = extractedFilePath;

		// compute the buffer CRC
		Red::System::CRC32 crcCalculator;
		outBufferSavedDataCRC = crcCalculator.Calculate( bufferData, bufferSizeInMemory );
	}

	// buffer was extracted
	return true;
}

Red::TUniquePtr< DeferredDataBufferExtractor > DeferredDataBufferExtractor::Create( const String& rootPath, const String & resourcePath, const Int32 sizeLimit /*= -1*/ )
{
	const Uint32 size = (sizeLimit > 0) ? (Uint32)sizeLimit : Config::cvDeferedBufferExtractionSizeLimit.Get();
	Red::TUniquePtr< DeferredDataBufferExtractor > ret( new DeferredDataBufferExtractor( GFileManager, rootPath, resourcePath, size ) );
	return ret;
}

void DeferredDataBufferExtractor::GenerateFileName( const String& resourceFilename, const Uint32 id, String& outPath )
{
	outPath = String::Printf( TEMPLATE_NAME, resourceFilename.AsChar(), id );
}
