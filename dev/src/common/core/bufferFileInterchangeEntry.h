/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef BUFFER_FILE_INTERCHANGE_ENTRY_H_
#define BUFFER_FILE_INTERCHANGE_ENTRY_H_

#include "bundleheader.h"
#include "bundleMetadataStore.h"
#include "bundleMetadataStoreEntry.h"

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			class CBufferFileInterchangeEntry
			{
				DECLARE_CLASS_MEMORY_ALLOCATOR( MC_BundleMetadata );
			public:
				RED_INLINE CBufferFileInterchangeEntry( const CBundleMetadataStore* store, const SMetadataFileInBundleRange& data, const Uint32 dataBlockOffset );
				RED_INLINE CBufferFileInterchangeEntry( const CBufferFileInterchangeEntry& entry );
				
				RED_INLINE const String GetPath() const;
				RED_INLINE const AnsiChar* GetPathPtr() const;
				RED_INLINE Uint32 GetSizeInBundle() const;
				RED_INLINE Uint32 GetSizeInMemory() const;
				RED_INLINE Uint32 GetAbsoluteOffset() const;
				RED_INLINE Uint32 GetRelativeOffset() const;
				RED_INLINE ECompressionType GetCompressionType() const;

			private:
				RED_INLINE CBufferFileInterchangeEntry& operator=( const CBufferFileInterchangeEntry& entry );

				static const Uint32 DEFERRED_DATA_BUFFER_TYPE = 1717990754;

				const SMetadataFileInBundleRange m_data;
				const CBundleMetadataStore* m_store;
				const Uint32 m_dataBlockOffset;
			};

			class CBundleDataBlockInterchangeEntry
			{
				DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_BundleMetadata );
			public:
				RED_INLINE CBundleDataBlockInterchangeEntry();
				RED_INLINE CBundleDataBlockInterchangeEntry( const SMetadataBundleEntry& entry );

				RED_INLINE Uint32 GetDataBlockSize() const;
				RED_INLINE Uint32 GetDataBlockOffset() const;
				RED_INLINE Uint32 GetBurstReadSize() const;

			private:
				Uint32 m_dataBlockSize;
				Uint32 m_dataBlockOffset;
				Uint32 m_burstDataBlockSize;
			};
		}
	}
}

#include "bufferFileInterchangeEntry.inl"

#endif // BUFFER_FILE_INTERCHANGE_ENTRY_H_