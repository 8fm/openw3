/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef _RED_BUNDLE_HEADER_H_
#define _RED_BUNDLE_HEADER_H_

#include "resourceid.h"

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			enum ECompressionType
			{
				CT_Uncompressed = 0,
				CT_Zlib,
				CT_Snappy,
				CT_Doboz,
				CT_LZ4,
				CT_LZ4HC,

				CT_ChainedZlib,		// Special purpose compression, shouldn't be considered by bundler or anything.

				CT_Max,

				CT_Auto, // Figure out which compression type to use

				CT_Default = CT_Auto,
			};

			struct SBundleHeaderItem
			{
				typedef Red::Core::ResourceManagement::CResourceId CResourceId;
				typedef Red::System::DateTime DateTime;

				static const Uint32 RAW_RESOURCE_PATH_MAX_SIZE = 256u;
				static const Uint32 MAX_DEPENDENCY_ITEMS = 1u;
				DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_ResourceBuffer );

				SBundleHeaderItem()
				:	m_resourceType( Uint32( -1 ) )
				,	m_dataSize( Uint32( -1 ) )
				,	m_compressedDataSize( Uint32( -1 ) )
				,	m_dataOffset( Uint32( -1 ) )
				,	m_cookedResourceCRC( Uint32( -1 ) )
				,	m_compressionType( CT_Uncompressed )
				{
				}

				SBundleHeaderItem( const SBundleHeaderItem& other )
				:	m_resourceId( other.m_resourceId )
				,	m_resourceType( other.m_resourceType )
				,	m_dataSize( other.m_dataSize )
				,	m_compressedDataSize( other.m_compressedDataSize )
				,	m_dataOffset( other.m_dataOffset )
				,	m_lastModified( other.m_lastModified )
				,	m_cookedResourceCRC( other.m_cookedResourceCRC )
				,	m_compressionType( other.m_compressionType )
				{
					Red::System::StringCopy( m_rawResourcePath, other.m_rawResourcePath, RAW_RESOURCE_PATH_MAX_SIZE );
					Red::System::MemoryCopy( m_resourceDependencies, other.m_resourceDependencies, ( sizeof( CResourceId ) * MAX_DEPENDENCY_ITEMS ) );
				}

				AnsiChar			m_rawResourcePath[ RAW_RESOURCE_PATH_MAX_SIZE ];
				CResourceId			m_resourceId;
				Uint32				m_resourceType;
				Uint32				m_dataSize;
				Uint32				m_compressedDataSize;
				Uint32				m_dataOffset;
				DateTime			m_lastModified;
				CResourceId			m_resourceDependencies[ MAX_DEPENDENCY_ITEMS ];
				Uint32				m_cookedResourceCRC;
				ECompressionType	m_compressionType;
			};

			// Aligned to 16 in the file.
			static const Uint32 PADDING = 16 - ( sizeof( SBundleHeaderItem ) & 15 );
			static const Uint32 ALIGNED_DEBUG_BUNDLE_HEADER_SIZE = sizeof( SBundleHeaderItem ) + ( ( PADDING < 16 )? PADDING : 0 );
		}
	}
}

#endif // _RED_BUNDLE_HEADER_H_
