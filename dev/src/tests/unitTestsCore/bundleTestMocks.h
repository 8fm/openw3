#pragma once
#ifndef BUNDLE_FILE_MOCKS_H_
#define BUNDLE_FILE_MOCKS_H_

#include "../../common/core/asyncIO.h"
#include "../../common/core/asyncLoadToken.h"
#include "../../common/core/directory.h"
//#include "../../common/core/bundleBufferIOMapper.h"

const static String BUNDLENAME = String( TXT( "UncompressedBundle.Bundle" ) );

class CAsyncIOMock : public CDeprecatedIO
{
public:
	virtual void LoadAsync( CAsyncLoadToken* asyncLoadToken )
	{
		asyncLoadToken->Signal( CAsyncLoadToken::EAsyncResult::eAsyncResult_Success );
		void* dst = asyncLoadToken->GetDestinationBuffer();
		Red::System::MemoryCopy( dst, ( m_readData + asyncLoadToken->GetFileOffset() ), asyncLoadToken->GetFileReadSize() );
	}

	void SetReadData( const Uint8* readData )
	{
		m_readData = readData;
	}

private:
	const Uint8* m_readData;
};

class CDirectoryMock : public CDirectory
{
public:
	virtual void FindResourcesByExtension( const String& extension, TDynArray< String >& resourcesPaths, Bool recursive = true, Bool fullFilePath = true )
	{
		RED_UNUSED( extension );
		RED_UNUSED( recursive );
		RED_UNUSED( fullFilePath );
		resourcesPaths.PushBack( BUNDLENAME );
	}
};

#endif // BUNDLE_FILE_MOCKS_H_