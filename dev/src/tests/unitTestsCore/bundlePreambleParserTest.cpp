#include "build.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundlePreambleParser.h"
#include "bundleTestMocks.h"
#include "testBundleData.h"

using namespace Red::Core::Bundle;

class BundlePreambleParserFixture : public ::testing::Test
{
public:
	BundlePreambleParserFixture()
	{
	}
	
	virtual void SetUp()
	{
		test_preambleParser = new CBundlePreambleParser( test_asyncIO );
		test_preamble_uncompressed = reinterpret_cast< const Red::Core::Bundle::SBundleHeaderPreamble* >( TestUncompressed );
		test_preamble_compressed = reinterpret_cast< const Red::Core::Bundle::SBundleHeaderPreamble* >( TestCompressed );
	}

	virtual void TearDown()
	{
		delete test_preambleParser;
	}

	const Red::Core::Bundle::SBundleHeaderPreamble* test_preamble_uncompressed;
	const Red::Core::Bundle::SBundleHeaderPreamble* test_preamble_compressed;
	CAsyncIOMock test_asyncIO;
	CBundlePreambleParser* test_preambleParser;
};

TEST_F( BundlePreambleParserFixture, Parse_parse_preamble_into_buffer_uncompressed )
{
	String uncompressedBundleName( TXT( "TestUncompressed.bundle" ) );
	test_asyncIO.SetReadData( TestUncompressed );
	SBundleHeaderPreamble preamble;
	CAsyncLoadToken* preambleLoadToken = test_preambleParser->CreateLoadToken( uncompressedBundleName, &preamble );
	test_preambleParser->Parse();
	RED_UNUSED( preambleLoadToken );
	EXPECT_EQ( CAsyncLoadToken::EAsyncResult::eAsyncResult_Success, preambleLoadToken->GetAsyncResult() );
	EXPECT_EQ( test_preamble_uncompressed->m_configuration, preamble.m_configuration );
	EXPECT_EQ( test_preamble_uncompressed->m_fileSize, preamble.m_fileSize );
	EXPECT_EQ( test_preamble_uncompressed->m_burstSize, preamble.m_burstSize );
	EXPECT_EQ( test_preamble_uncompressed->m_fileVersion, preamble.m_fileVersion );
	EXPECT_EQ( test_preamble_uncompressed->m_headerSize, preamble.m_headerSize );
	EXPECT_EQ( test_preamble_uncompressed->m_headerVersion, preamble.m_headerVersion );
	EXPECT_EQ( test_preamble_uncompressed->m_bundleStamp, preamble.m_bundleStamp );
}

TEST_F( BundlePreambleParserFixture, Parse_parse_preamble_into_buffer_compressed )
{
	String compressedBundleName( TXT( "TestCompressed.bundle" ) );
	test_asyncIO.SetReadData( TestCompressed );
	SBundleHeaderPreamble preamble;
	CAsyncLoadToken* preambleLoadToken = test_preambleParser->CreateLoadToken( compressedBundleName, &preamble );
	test_preambleParser->Parse();
	RED_UNUSED( preambleLoadToken );
	EXPECT_EQ( CAsyncLoadToken::EAsyncResult::eAsyncResult_Success, preambleLoadToken->GetAsyncResult() );
	EXPECT_EQ( test_preamble_compressed->m_configuration, preamble.m_configuration );
	EXPECT_EQ( test_preamble_compressed->m_fileSize, preamble.m_fileSize );
	EXPECT_EQ( test_preamble_compressed->m_burstSize, preamble.m_burstSize );
	EXPECT_EQ( test_preamble_compressed->m_fileVersion, preamble.m_fileVersion );
	EXPECT_EQ( test_preamble_compressed->m_headerSize, preamble.m_headerSize );
	EXPECT_EQ( test_preamble_compressed->m_headerVersion, preamble.m_headerVersion );
	EXPECT_EQ( test_preamble_compressed->m_bundleStamp, preamble.m_bundleStamp );
}