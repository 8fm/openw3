/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/deferredDataBuffer.h"
#include "../../common/core/file.h"
#include "../../common/core/scopedPtr.h"
#include "../../common/core/fileLatentLoadingToken.h"
#include "../../common/core/loadingJobManager.h"
#include <functional>

// TODO: rewrite those mostly not very useful unit tests
#if 0 

using testing::Ref;
using testing::Return;
using testing::_;

class FileReaderMock : public CMemoryFileReader
{
public:

	FileReaderMock( const TDynArray< Uint8 > &inputBuffer )
		: CMemoryFileReader( inputBuffer, 0 )
	{}

	virtual void Serialize( void* buffer, size_t size )
	{
		CMemoryFileReader::Serialize( buffer, size );
		SerializeCalled();
	}

	MOCK_METHOD0_NO_OVERRIDE( SerializeCalled, void() );
	MOCK_CONST_METHOD0( GetOffset, Uint64 () );
	MOCK_CONST_METHOD0( GetSize, Uint64() );
	MOCK_METHOD1( Seek, void( Int64 position ) );
	MOCK_METHOD1( CreateLatentLoadingToken, IFileLatentLoadingToken* ( Uint64 currentOffset ) );
};

class FileMock : public IFile
{
public:
	FileMock( Uint32 flags )
		: IFile( flags )
	{}

	MOCK_METHOD2( Serialize, void( void *, size_t size ) );
	MOCK_CONST_METHOD0( GetOffset, Uint64 () );
	MOCK_CONST_METHOD0( GetSize, Uint64() );
	MOCK_METHOD1( Seek, void( Int64 position ) );
	MOCK_METHOD1( CreateLatentLoadingToken, IFileLatentLoadingToken* ( Uint64 currentOffset ) );
};

class FileLatentLoadingTokenMock : public IFileLatentLoadingToken
{
public:
	FileLatentLoadingTokenMock()
		: IFileLatentLoadingToken( 0 )
	{}

	MOCK_METHOD1( Resume, IFile* ( Uint64 relativeOffset ) );
	MOCK_CONST_METHOD0( Clone, IFileLatentLoadingToken* () );
	MOCK_CONST_METHOD0( GetDebugInfo, String () );

};

struct BufferDefinition
{
	Uint16 version;
	Uint16 memoryAlignment;
	Uint32 size;
	String filename;
	DeferredDataBuffer::FileOffsetType fileOffset;
	TDynArray< Uint8 > inputOutputBuffer;
};

BufferDefinition defaultBufferDefinition = 
{
	0,
	16,
	100,
	TXT( "" ),
	0xDEADBEEF
};

struct DeferredDataBufferFixture : testing::Test
{
	void SetupFileReader( BufferDefinition &definition )
	{
		CMemoryFileWriter writer( definition.inputOutputBuffer );
		writer << definition.size;

		fileReader.Reset( new FileReaderMock( definition.inputOutputBuffer ) );
		token = new FileLatentLoadingTokenMock();
		
		EXPECT_CALL( *fileReader, SerializeCalled() ).Times( 5 ); 
		EXPECT_CALL( *fileReader, CreateLatentLoadingToken( definition.fileOffset ) )
			.WillOnce( Return( token ) );
	}

	void SetupBufferFileReader( const BufferDefinition &definition )
	{
		bufferReader = new FileMock(FF_Reader);
		EXPECT_CALL( *token, Resume( 0 ) )
			.WillOnce( Return( bufferReader ) );
		EXPECT_CALL( *bufferReader, Serialize( _, definition.size ) ).Times( 1 );  
	}

	void SetupReadBufferSync( BufferDefinition &definition )
	{
		SetupFileReader( definition );
		SetupBufferFileReader( definition );
		buffer.SerializeDirectlyAsRawBuffer( *fileReader );
	}

	void SetupDefaultReadBufferSync( )
	{
		SetupReadBufferSync( defaultBufferDefinition );
	}

	void SetupDefaultReadBufferAsync()
	{
		SetupFileReader( defaultBufferDefinition );
		buffer.SerializeDirectlyAsRawBuffer( *fileReader );
	}

	DeferredDataBufferFixture()
	{
	}

	DeferredDataBuffer buffer;
	Red::TScopedPtr< FileReaderMock > fileReader;
	FileMock * bufferReader;
	FileLatentLoadingTokenMock * token; 
};

/*TEST_F( DeferredDataBufferFixture, Serialize_serialize_in_correct_order_and_file_offset_at_the_end )
{
	SetupFileReader( defaultBufferDefinition );
	buffer.SerializeDirectlyAsRawBuffer( *fileReader );

	//EXPECT_EQ( defaultBufferDefinition.memoryAlignment, buffer.GetMemoryAlignment() );
	EXPECT_EQ( defaultBufferDefinition.size, buffer.GetSize() );
	//EXPECT_EQ( defaultBufferDefinition.fileOffset, buffer.GetFileOffset() );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}*/

TEST_F( DeferredDataBufferFixture, GetBufferHandle_return_null_buffer_if_not_previously_acquired )
{
	BufferHandle handle = buffer.GetBufferHandle();
	EXPECT_FALSE( handle );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleSync_return_null_buffer_if_size_is_0 )
{
	BufferHandle handle = buffer.AcquireBufferHandleSync();
	EXPECT_FALSE( handle );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleSync_create_and_read_buffer )
{
	SetupReadBufferSync( defaultBufferDefinition );

	BufferHandle handle = buffer.AcquireBufferHandleSync();

	ASSERT_TRUE( handle );
	EXPECT_EQ( defaultBufferDefinition.size, handle->GetSize() );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleSync_do_not_recreate_buffer_if_buffer_is_alive )
{
	SetupDefaultReadBufferSync();

	BufferHandle handle1 = buffer.AcquireBufferHandleSync();
	BufferHandle handle2 = buffer.AcquireBufferHandleSync();

	EXPECT_EQ( handle1, handle2 );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}

TEST_F( DeferredDataBufferFixture, GetBufferHandle_return_currently_loaded_buffer )
{
	SetupDefaultReadBufferSync();

	BufferHandle handle1 = buffer.AcquireBufferHandleSync();
	BufferHandle handle2 = buffer.GetBufferHandle();

	EXPECT_EQ( handle1, handle2 );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}

TEST_F( DeferredDataBufferFixture, GetBufferHandle_return_null_handle_no_one_needs_buffer_anymore )
{
	SetupDefaultReadBufferSync();
	
	{
		BufferHandle handle1 = buffer.AcquireBufferHandleSync();
	}
	
	BufferHandle handle2 = buffer.GetBufferHandle();

	EXPECT_FALSE( handle2 );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleForWritingSync_return_null_handle_if_size_is_0 )
{
	BufferHandle handle = buffer.AcquireBufferHandleForWritingSync();
	EXPECT_FALSE( handle );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleForWritingSync_create_and_read_buffer )
{
	SetupReadBufferSync( defaultBufferDefinition );

	BufferHandle handle = buffer.AcquireBufferHandleForWritingSync();

	ASSERT_TRUE( handle );
	EXPECT_EQ( defaultBufferDefinition.size, handle->GetSize() );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleForWritingSync_do_not_recreate_buffer_if_buffer_is_alive )
{
	SetupDefaultReadBufferSync();

	BufferHandle handle1 = buffer.AcquireBufferHandleForWritingSync();
	BufferHandle handle2 = buffer.AcquireBufferHandleForWritingSync();

	EXPECT_EQ( handle1, handle2 );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleForWriting_buffer_is_not_unloaded_even_if_no_one_needs_it )
{
	SetupDefaultReadBufferSync();

	BufferHandle handle1 = buffer.AcquireBufferHandleSync();
	handle1.Reset();
	BufferHandle handle2 = buffer.GetBufferHandle();

	EXPECT_EQ( handle1, handle2 );
	EXPECT_TRUE( testing::Mock::VerifyAndClearExpectations( fileReader.Get() ) );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleAsync_call_callback_if_already_loaded )
{
	BufferHandle async;

	SetupDefaultReadBufferSync();
	BufferHandle sync = buffer.AcquireBufferHandleSync();
	buffer.AcquireBufferHandleAsync( [&]( BufferHandle result ){ async = result; } );

	EXPECT_EQ( sync, async );
}

TEST_F( DeferredDataBufferFixture, AcquireBufferHandleAsync_wont_call_callback_if_not_already_loaded )
{
	SetupDefaultReadBufferAsync();

	bool callbackCalled = false;

	BufferAsyncDataHandle handle = buffer.AcquireBufferHandleAsync( [&](BufferHandle){ callbackCalled = true; } );
	EXPECT_FALSE( callbackCalled );
}



#endif