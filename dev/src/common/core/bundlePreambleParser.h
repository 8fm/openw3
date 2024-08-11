/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef BUNDLE_PREAMBLE_PARSER_H_
#define BUNDLE_PREAMBLE_PARSER_H_
#include "bundleParser.h"
#include "asyncLoadToken.h"

class CDeprecatedIO;

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			struct SBundleHeaderPreamble;
			//////////////////////////////////////////////////////////////////////////
			// CBundlePreambleParser
			// -----------------------------------------------------------------------
			// Parses and returns the data from a bundles preamble.
			//////////////////////////////////////////////////////////////////////////
			class CBundlePreambleParser : public IBundleParser, Red::System::NonCopyable
			{
			public:
				CBundlePreambleParser( CDeprecatedIO& asyncIO );
				virtual ~CBundlePreambleParser();

				virtual CAsyncLoadToken* CreateLoadToken( const String& absoluteBundlePath, void* dstBuffer ) RED_FINAL;
				virtual void Parse() RED_FINAL;
				
				RED_INLINE const SBundleHeaderPreamble* GetPreamble() const
				{
					return reinterpret_cast< const SBundleHeaderPreamble* >( m_preambleToken->GetDestinationBuffer() );
				}

			private:
				CDeprecatedIO& m_asyncIO;
				CAsyncLoadToken* m_preambleToken;
				void* m_preambleBuffer;
			};
		}
	}
}
#endif // BUNDLE_PREAMBLE_PARSER_H_