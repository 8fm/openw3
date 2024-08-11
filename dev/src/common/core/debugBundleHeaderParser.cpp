#include "build.h"

#include "debugBundleHeaderParser.h"
#include "bundleheader.h"
#include "bundlePreamble.h"
#include "asyncIO.h"
#include "asyncLoadToken.h"
using namespace Red::Core::Bundle;

//////////////////////////////////////////////////////////////////////////
CDebugBundleHeaderParser::CDebugBundleHeaderParser( CDeprecatedIO& asyncIO, const SBundleHeaderPreamble& preamble )
	: m_asyncIO( asyncIO )
	, m_preamble( preamble )
	, m_headerToken( nullptr )
	
{

}

//////////////////////////////////////////////////////////////////////////
CDebugBundleHeaderParser::~CDebugBundleHeaderParser()
{
}


//////////////////////////////////////////////////////////////////////////
CAsyncLoadToken* CDebugBundleHeaderParser::CreateLoadToken( const String& absoluteBundlePath, void* dstBuffer )
{
	RED_FATAL_ASSERT( dstBuffer != nullptr, "Destination buffer is null" );
	m_headerToken = new CAsyncLoadToken( absoluteBundlePath, dstBuffer, m_preamble.m_headerSize, sizeof( SBundleHeaderPreamble ) );
	return m_headerToken;
}

//////////////////////////////////////////////////////////////////////////
void CDebugBundleHeaderParser::Parse()
{
	RED_FATAL_ASSERT( m_headerToken != nullptr, "You must create a load token first" );
	m_asyncIO.LoadAsync( m_headerToken );
	m_headerToken->Wait();
	ReadHeader();
	m_headerToken->Release();
}

//////////////////////////////////////////////////////////////////////////
CDebugBundleHeaderParser::HeaderCollection CDebugBundleHeaderParser::GetHeaderItems() const
{
	return m_bundleItems;
}

//////////////////////////////////////////////////////////////////////////
void CDebugBundleHeaderParser::ReadHeader()
{
	RED_FATAL_ASSERT( m_preamble.m_headerVersion == BUNDLE_HEADER_VERSION, "Header version mismatch!" );
	const Uint32 itemCount = m_preamble.m_headerSize / ALIGNED_DEBUG_BUNDLE_HEADER_SIZE;
	Uint8* currentItemPtr = reinterpret_cast< Uint8* >( m_headerToken->GetDestinationBuffer() );
	Uint8* headerEnd = currentItemPtr + ( ALIGNED_DEBUG_BUNDLE_HEADER_SIZE * itemCount );
	while( currentItemPtr != headerEnd ) 
	{
		m_bundleItems.PushBack( reinterpret_cast< const SBundleHeaderItem* >( currentItemPtr ) );
		currentItemPtr += ALIGNED_DEBUG_BUNDLE_HEADER_SIZE;
	}
}