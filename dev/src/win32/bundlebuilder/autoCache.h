/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _RED_AUTO_CACHE_H_
#define _RED_AUTO_CACHE_H_

#include "../../common/core/core.h"
#include "../../common/core/bundleheader.h"

namespace Bundler
{
	namespace AutoCache
	{
		typedef Red::Core::Bundle::ECompressionType CompressionType;

		class Bundle
		{
		public:

			Bundle();
			~Bundle();

			void Initialize( const StringAnsi& bundleName );

			RED_INLINE const StringAnsi& GetBundleName() const { return m_bundleName; }

			void AddResult( const StringAnsi& id, CompressionType compression );
			Bool GetResult( const StringAnsi& id, CompressionType& compression );

			THashMap< StringAnsi, CompressionType >::iterator Begin() { return m_results.Begin(); }
			THashMap< StringAnsi, CompressionType >::iterator End() { return m_results.End(); }
			const THashMap< StringAnsi, CompressionType >::const_iterator Begin() const { return m_results.Begin(); }
			const THashMap< StringAnsi, CompressionType >::const_iterator End() const { return m_results.End(); }

		private:
			struct CompressionResult;

			StringAnsi m_bundleName;
			THashMap< StringAnsi, CompressionType > m_results;
		};

		//////////////////////////////////////////////////////////////////////////

		class Definition
		{
		public:
			Definition();
			~Definition();

			void Initialize( Uint32 numBundles );

			void Normalize();

			Bundle& GetResults( const StringAnsi& bundleName );

			void Load( const AnsiChar* filepath, const TDynArray< StringAnsi >& bundleNames );
			void Save( const AnsiChar* filepath ) const;

		private:
			void* Open( const AnsiChar* filepath );

			Bundle& MapResults( const StringAnsi& bundleName );

			TDynArray< Bundle > m_results;

			THashMap< StringAnsi, Uint32 > m_nameToBundleIndex;
			Uint32 m_firstFreeIndex;

			static const AnsiChar* HEADER;

		private:
			class Allocator;
		};
	}
}

#endif // _RED_AUTO_CACHE_H_
