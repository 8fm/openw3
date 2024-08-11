/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef DEBUG_BUNDLE_HEADER_PARSER_H_
#define DEBUG_BUNDLE_HEADER_PARSER_H_
#include "bundleParser.h"


class CDeprecatedIO;
namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			struct SBundleHeaderPreamble;
			struct SBundleHeaderItem;
			class CDebugBundleHeaderParser : public IBundleParser, Red::System::NonCopyable
			{
			public:
				typedef TDynArray< const SBundleHeaderItem*, MC_BundleMetadata > HeaderCollection;
				CDebugBundleHeaderParser( CDeprecatedIO& asyncIO,  const SBundleHeaderPreamble& preamble );
				virtual ~CDebugBundleHeaderParser();

				virtual CAsyncLoadToken* CreateLoadToken( const String& absoluteBundlePath, void* dstBuffer ) RED_FINAL;
				virtual void Parse() RED_FINAL;

				HeaderCollection GetHeaderItems() const;
			private:
				void ReadHeader();

				CDeprecatedIO& m_asyncIO;
				const SBundleHeaderPreamble& m_preamble;
				CAsyncLoadToken* m_headerToken;
				HeaderCollection m_bundleItems;
			};
		}
	}
}

#endif