/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionCacheDataFormat.h"
#include "collisionCacheOptimizer.h"
#include "collisionCacheBuilder.h"

CCollisionCacheOptimizer::CCollisionCacheOptimizer()
{
}

Bool CCollisionCacheOptimizer::OptimizeFile( const String& inputCacheFile, const String& outputCacheFile )
{
	// Load input data
	Red::TScopedPtr< IFile > inputFile( GFileManager->CreateFileReader( inputCacheFile, FOF_AbsolutePath | FOF_Buffered ) );
	if ( !inputFile )
	{
		ERR_ENGINE( TXT("Failed to open input cache '%ls'"), inputCacheFile.AsChar() );
		return false;
	}

	// load file header
	Red::System::DateTime timeStamp;
	if ( !CCollisionCacheData::ValidateHeader( *inputFile, timeStamp ) )
	{
		ERR_ENGINE( TXT("Specified file is NOT a collision cache") );
		return false;
	}

	// load input data
	CCollisionCacheData inputData;
	if ( !inputData.Load( *inputFile ) )
	{
		return false;
	}

	// file is empty
	if ( inputData.m_tokens.Empty() )
	{
		WARN_ENGINE( TXT("No entries in the input cache file '%ls'"), inputCacheFile.AsChar() );
		return true;
	}

	// extract the tokens for sorting
	TDynArray< const CCollisionCacheData::CacheToken* > inputTokens;
	inputTokens.Reserve( inputData.m_tokens.Size() );
	for ( Uint32 i=0; i<inputData.m_tokens.Size(); ++i )
	{
		inputTokens.PushBack( &inputData.m_tokens[i] );
	}

	// sort the tokens based on their SIZE
	Sort( inputTokens.Begin(), inputTokens.End(), [](const CCollisionCacheData::CacheToken* a, const CCollisionCacheData::CacheToken* b) { return a->m_dataSizeOnDisk < b->m_dataSizeOnDisk; } );

	// open output file
	Red::TScopedPtr< IFile > outputFile( GFileManager->CreateFileWriter( outputCacheFile, FOF_Buffered | FOF_AbsolutePath ) );
	if ( !outputFile )
	{
		ERR_ENGINE( TXT("Failed to create output file '%ls'"), outputCacheFile.AsChar() );
		return false;
	}

	// write initial header - preserve time stamp
	CCollisionCacheData::WriteHeader( *outputFile, timeStamp );

	// skip to start of first data block
	const Uint32 writePos = sizeof( CCollisionCacheData::RawHeader ) + sizeof( CCollisionCacheData::IndexHeader );
	outputFile->Seek( writePos );

	// create the output file writer
	CCollisionCacheData writeData;
	CCollisionCacheDataBuilder writer( writeData );

	// preallocate read buffer
	TDynArray< Uint8 > readBuffer;
	readBuffer.Resize( 1 << 20 );

	// make sure reading buffer is large enough
	Uint32 index = 0;
	for ( auto* tokenToWrite : inputTokens )
	{
		const CCollisionCacheData::CacheToken& inToken = *tokenToWrite;
		if ( inToken.m_dataSizeOnDisk > readBuffer.Size() )
			readBuffer.Resize( inToken.m_dataSizeOnDisk );

		LOG_ENGINE( TXT("Collision %d, size %d, offset %d, path %hs"), 
			index++, inToken.m_dataSizeOnDisk, inToken.m_dataOffset, &inputData.m_strings[ inToken.m_name ] );

		// read source data from source file
		inputFile->Seek( inToken.m_dataOffset );
		inputFile->Serialize( readBuffer.Data(), inToken.m_dataSizeOnDisk );

		// write data to output file
		const Uint32 writePos = (Uint32) outputFile->GetOffset();
		outputFile->Serialize( readBuffer.Data(), inToken.m_dataSizeOnDisk );

		// create new token in output file
		CCollisionCacheData::CacheToken outToken = inToken;
		outToken.m_name = writer.AddString( &inputData.m_strings[ inToken.m_name ] );
		outToken.m_dataOffset = writePos;
		writer.AddToken( outToken );
	}

	// save the new tables
	const Uint32 endOfFilePos = (Uint32) outputFile->GetOffset();
	outputFile->Seek( sizeof( CCollisionCacheData::RawHeader ) );
	writeData.Save( *outputFile, endOfFilePos );

	// done
	return true;
}