/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_DEFERRED_DATA_BUFFER_EXTRACTOR_H_
#define _CORE_DEFERRED_DATA_BUFFER_EXTRACTOR_H_

#include "handleMap.h"
#include "uniquePtr.h"
#include "deferredDataBuffer.h"

class CResource;
class CFileManager;

class DeferredDataBufferExtractor
{
public:
	DeferredDataBufferExtractor( const CFileManager* fileManager, const String& rootPath, const String& resourcePath, const Uint32 sizeLimit );
	~DeferredDataBufferExtractor();

	// Extract the buffer data to external file
	// If successful it should return true and the size of the buffer's data on disk. It's allowed for those data to be smaller (compressed).
	bool Extract( const Uint32 bufferId, const Uint32 bufferSizeInMemory, const void* bufferData, Uint32& outBufferSizeOnDisk, Uint32& outBufferSavedDataCRC, String& outBufferFilePath ) const;

	// Create a buffer extractor for given resource, buffers will be written at given root path
	// If size limit for extracting is not specified than all buffers are extracted, empty buffer are never extracted regardless of this setting
	static Red::TUniquePtr< DeferredDataBufferExtractor > Create( const String& rootPath, const String & resourcePath, const Int32 sizeLimit = -1 );

	// Helper function: given the name of the current file generate a filename for the buffer with given ID
	static void GenerateFileName( const String& resourceFilename, const Uint32 id, String& outPath );

private:
	static const Char* TEMPLATE_NAME;

	String					m_rootPath;
	String					m_resourcePath;
	Uint32					m_sizeLimit;
	const CFileManager*		m_fileManager;
};

#endif
