#include "build.h"
#include "../../common/core/memory.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundleheader.h"
#include "../../common/core/bundlePreambleParser.h"
#include "../../common/core/debugBundleHeaderParser.h"
#include "bundleTestMocks.h"
#include "testBundleData.h"

using namespace Red::Core::Bundle;

class DebugBundleHeaderParserFixture : public ::testing::Test
{
public:
	DebugBundleHeaderParserFixture()
	{
	}

	virtual void SetUp()
	{
		test_bundleName = String( TXT("TestUncompressed.bundle") );
		test_asyncIO.SetReadData( TestUncompressed );
		test_preambleParser = new CBundlePreambleParser( test_asyncIO );
		test_preamble = new SBundleHeaderPreamble();
		LoadPreamble( static_cast< void* >( test_preamble ) );
		test_debugHeaderParser = new CDebugBundleHeaderParser( test_asyncIO, *test_preamble );
	}

	virtual void TearDown()
	{
		delete test_preamble;
		delete test_preambleParser;
		delete test_debugHeaderParser;
	}

	void LoadPreamble( void* preambleBuffer )
	{
		String uncompressedBundleName( TXT("TestUncompressed.bundle") );
		CAsyncLoadToken* preambleLoadToken = test_preambleParser->CreateLoadToken( uncompressedBundleName, preambleBuffer );
		test_asyncIO.LoadAsync( preambleLoadToken );
		preambleLoadToken->Wait();
	}

	String test_bundleName;
	SBundleHeaderPreamble* test_preamble;
	CBundlePreambleParser* test_preambleParser;
	CDebugBundleHeaderParser* test_debugHeaderParser;
	CAsyncIOMock test_asyncIO;
};


TEST_F( DebugBundleHeaderParserFixture, Parse_parse_bundle_header_into_buffer_check_header_items )
{
	void* debugHeaderBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_BundleMetadata, test_preamble->m_headerSize );
	CAsyncLoadToken* loadToken = test_debugHeaderParser->CreateLoadToken( test_bundleName, debugHeaderBuffer );
	EXPECT_TRUE( loadToken != nullptr );
	test_debugHeaderParser->Parse();
	const CDebugBundleHeaderParser::HeaderCollection& bundleHeaderItems = test_debugHeaderParser->GetHeaderItems();
	ASSERT_EQ( 3, bundleHeaderItems.Size() );
	const Uint32 headerOffset = sizeof( Red::Core::Bundle::SBundleHeaderPreamble );
	const Uint8* compareItemPtr = TestUncompressed + headerOffset;
	for( Uint32 itemIndex = 0; itemIndex < 3; ++itemIndex )
	{
		const SBundleHeaderItem* currentItem = reinterpret_cast< const SBundleHeaderItem * >( compareItemPtr );
		compareItemPtr += ALIGNED_DEBUG_BUNDLE_HEADER_SIZE;
		EXPECT_STREQ( currentItem->m_rawResourcePath, bundleHeaderItems[itemIndex]->m_rawResourcePath );
		EXPECT_EQ( currentItem->m_compressedDataSize, bundleHeaderItems[itemIndex]->m_compressedDataSize );
		EXPECT_EQ( currentItem->m_compressionType, bundleHeaderItems[itemIndex]->m_compressionType );
		EXPECT_EQ( currentItem->m_dataOffset, bundleHeaderItems[itemIndex]->m_dataOffset );
		EXPECT_EQ( currentItem->m_dataSize, bundleHeaderItems[itemIndex]->m_dataSize );
		EXPECT_EQ( currentItem->m_resourceId, bundleHeaderItems[itemIndex]->m_resourceId );
	}
	RED_MEMORY_FREE( MemoryPool_Default, MC_BundleMetadata, debugHeaderBuffer );
}
