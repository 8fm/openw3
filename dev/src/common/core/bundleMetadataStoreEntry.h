/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef BUNDLE_METADATA_CACHE_ENTRY_H_
#define BUNDLE_METADATA_CACHE_ENTRY_H_

#include "resourceid.h"
#include "bundleheader.h"
#include "datetime.h"

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			//////////////////////////////////////////////////////////////////////////
			// SBundleMetadataFileEntry
			// -----------------------------------------------------------------------
			// Storage object for each file from a bundle to be stored within 
			// CBundleMetadataCache
			//////////////////////////////////////////////////////////////////////////
			struct SMetadataFileEntry
			{
				DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_BundleMetadata );

				//////////////////////////////////////////////////////////////////////////
				// Constructors
				//////////////////////////////////////////////////////////////////////////
				SMetadataFileEntry()
					: m_filename( String::EMPTY )
					, m_sizeInBundle( 0 )
					, m_sizeInMemory( 0 )
					, m_internalBundleOffset( 0 )
					, m_compressionType( Red::Core::Bundle::CT_Uncompressed )
				{
				}

				// Copy constructor
				SMetadataFileEntry( const SMetadataFileEntry& fileEntry )
					: m_filename( fileEntry.m_filename )
					, m_sizeInBundle( fileEntry.m_sizeInBundle )
					, m_sizeInMemory( fileEntry.m_sizeInMemory )
					, m_internalBundleOffset( fileEntry.m_internalBundleOffset )
					, m_lastModified( fileEntry.m_lastModified )
					, m_compressionType( fileEntry.m_compressionType )
				{

				}

				// Move constructor
				SMetadataFileEntry( SMetadataFileEntry&& fileEntry )
					: m_filename( std::move( fileEntry.m_filename ) )
					, m_sizeInBundle( std::move( fileEntry.m_sizeInBundle ) )
					, m_sizeInMemory( std::move( fileEntry.m_sizeInMemory ) )
					, m_internalBundleOffset( std::move( fileEntry.m_internalBundleOffset ) )
					, m_lastModified( std::move( fileEntry.m_lastModified ) )
					, m_compressionType( std::move( fileEntry.m_compressionType ) )
				{
					// Not strictly speaking in keeping with the C++ standard, but lets make this class safe to use afterwards
					fileEntry.m_sizeInBundle			= 0;
					fileEntry.m_sizeInMemory			= 0;
					fileEntry.m_internalBundleOffset	= 0;
					fileEntry.m_lastModified.SetRaw( 0 );
					fileEntry.m_compressionType			= CT_Default;
				}

				SMetadataFileEntry( const SBundleHeaderItem& headerItem )
					: m_filename( ANSI_TO_UNICODE( headerItem.m_rawResourcePath ) )
					, m_sizeInBundle( headerItem.m_compressedDataSize )
					, m_sizeInMemory( headerItem.m_dataSize )
					, m_internalBundleOffset( headerItem.m_dataOffset )
					, m_lastModified( headerItem.m_lastModified )
					, m_compressionType( headerItem.m_compressionType )
				{

				}

				//////////////////////////////////////////////////////////////////////////
				// Data
				//////////////////////////////////////////////////////////////////////////
				String m_filename;
				Uint32 m_sizeInBundle;
				Uint32 m_sizeInMemory;
				Uint32 m_internalBundleOffset;
				CDateTime m_lastModified;
				ECompressionType m_compressionType;
			};

			//////////////////////////////////////////////////////////////////////////
			// SMetadataBundleEntry
			// -----------------------------------------------------------------------
			// Storage object for bundles which are contained in the metadata store.
			//////////////////////////////////////////////////////////////////////////
			struct SMetadataBundleEntry
			{
				DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_BundleMetadata );
				//////////////////////////////////////////////////////////////////////////
				// Constructors
				//////////////////////////////////////////////////////////////////////////
				SMetadataBundleEntry( const String& path, const Uint32 elementCount, const Uint32 dataBlockSize, const Uint32 dataBlockOffset, const Uint32 burstDataBlockSize )
					: m_resourceId( path.ToLower() )
					, m_dataBlockSize( dataBlockSize )
					, m_dataBlockOffset( dataBlockOffset )
					, m_burstDataBlockSize( burstDataBlockSize )
					, m_filename( path.ToLower() )
					, m_elementCount( elementCount )
				{}

				SMetadataBundleEntry( const SMetadataBundleEntry& bundleEntry )
					: m_resourceId( bundleEntry.m_resourceId )
					, m_dataBlockSize( bundleEntry.m_dataBlockSize )
					, m_dataBlockOffset( bundleEntry.m_dataBlockOffset )
					, m_burstDataBlockSize( bundleEntry.m_burstDataBlockSize )
					, m_filename( bundleEntry.m_filename )
					, m_elementCount( bundleEntry.m_elementCount )
				{}

				SMetadataBundleEntry()
					: m_dataBlockSize(0)
					, m_dataBlockOffset(0)
					, m_burstDataBlockSize(0)
					, m_elementCount(0)
				{}

				//////////////////////////////////////////////////////////////////////////
				// Data
				//////////////////////////////////////////////////////////////////////////
				ResourceManagement::CResourceId m_resourceId;
				Uint32 m_dataBlockSize;
				Uint32 m_dataBlockOffset;
				Uint32 m_burstDataBlockSize;
				String m_filename;
				Uint32 m_elementCount;
			};

		}
	}
}

#endif // BUNDLE_METADATA_CACHE_ENTRY_H_
