#include "build.h"
#include "bundlePreamble.h"
#include "bundlePreambleParser.h"
#include "asyncIO.h"

using namespace Red::Core::Bundle;

//////////////////////////////////////////////////////////////////////////
CBundlePreambleParser::CBundlePreambleParser( CDeprecatedIO& asyncIO )
	: m_asyncIO( asyncIO )
	, m_preambleToken( nullptr )
{

}

//////////////////////////////////////////////////////////////////////////
CBundlePreambleParser::~CBundlePreambleParser()
{
	if( m_preambleToken )
	{
		m_preambleToken->Release();
	}
}

//////////////////////////////////////////////////////////////////////////
 CAsyncLoadToken* CBundlePreambleParser::CreateLoadToken( const String& absoluteBundlePath, void* dstBuffer )
 {
	RED_FATAL_ASSERT( dstBuffer != nullptr, "No memory was allocated!");
	m_preambleToken = new CAsyncLoadToken( absoluteBundlePath, dstBuffer, sizeof( SBundleHeaderPreamble ), 0 );
	return m_preambleToken;
 }

//////////////////////////////////////////////////////////////////////////
void CBundlePreambleParser::Parse()
{
	RED_FATAL_ASSERT( m_preambleToken != nullptr, "You must create load token first.");
	m_asyncIO.LoadAsync( m_preambleToken );
	m_preambleToken->Wait();
	RED_ASSERT( GetPreamble()->m_bundleStamp == BUNDLE_STAMP, TXT("Bundle stamp is invalid!") );
}