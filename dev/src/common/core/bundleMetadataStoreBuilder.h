/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef BUNDLE_METADATA_CACHE_BUILDER_H_
#define BUNDLE_METADATA_CACHE_BUILDER_H_

#include "string.h"
#include "hashmap.h"
#include "bundleMetadataStore.h"
#include "bundlePreambleParser.h"
#include "debugBundleHeaderParser.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CDeprecatedIO;

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			struct SMetadataBundleEntry;
			struct SMetadataFileEntry;
			class CBundleMetadataStore;			

			// CBundleMetadataCacheBuilder
			class CBundleMetadataStoreBuilder : public CBundleMetadataStore
			{
			public:
				CBundleMetadataStoreBuilder( CDeprecatedIO& asyncIO );
				~CBundleMetadataStoreBuilder();

				struct PatchGroup
				{
					String						m_patchRootPath;
					TDynArray< String >			m_patchBundlesPaths; 
				};

				struct Settings
				{
					String						m_rootPath;
					TDynArray< String >			m_bundlesPaths; 

					TDynArray< PatchGroup >		m_patchGroups;

					Bool						m_isCensored;

					Settings();
				};

				// Generate meta data store from given ABSOLUTE bundle paths
				// Inside the meta data store we only store bundle paths relative to the root path
				Bool ProcessBundles( const Settings& settings );

			private:
				static const String BUNDLE_FILE_EXTENTION;

				// entry information
				struct EntryInfo
				{
					Int32									m_fileId;		// mapped final path id

					StringAnsi								m_path;			// file path
					StringAnsi								m_originalPath;	// original file path (censored files will have different one)
					Bool									m_ignore;		// do not add this entry to the final meta store					

					Int32									m_bufferId;		// id of this buffer (if not a buffer than -1)
					EntryInfo*								m_bufferOwner;	// owner of the buffer data
					TDynArray< EntryInfo* >					m_buffers;		// related buffers
					StringAnsi								m_bufferPrefix;
					Int32									m_maxBufferId;
				};

				struct RawBundleData;

				// raw bundle entry
				struct RawBundleEntry
				{
					RawBundleData*							m_bundle;
					StringAnsi								m_path;
					Uint8									m_compressionType;
					Uint32									m_compressedDataSize;
					Uint32									m_dataSize;
					Uint32									m_dataOffset;
					Bool									m_skip;
					Bool									m_censored;
					Int32									m_finalPatchGroup;
				};

				// raw bundle data
				struct RawBundleData
				{
					String									m_path;
					StringAnsi								m_name;
					TDynArray< RawBundleEntry* >			m_entries;
					Uint32									m_burstSize;
					Uint32									m_dataBlockSize;
					Uint32									m_dataBlockOffset;
					Int32									m_patchGroup;
				};
				
				//////////////////////////////////////////////////////////////////////////
				// Private Methods
				//////////////////////////////////////////////////////////////////////////
				CDebugBundleHeaderParser* CreateDebugBundleParser( const SBundleHeaderPreamble& preamble );
				Bool ProcessBundle( const String& rootPath, const String& relativeBundleFilePath, const Int32 patchGroupID ); 
				CAsyncLoadToken* ReadPreamble( const String& absoluteBundleFilePath, SBundleHeaderPreamble* preamble );
				CDebugBundleHeaderParser::HeaderCollection ReadDebugBundleHeader( const String& absoluteFilePath, const SBundleHeaderPreamble& preamble, void* dstBuffer );

				Uint32 AddString( const AnsiChar* txt );
				Uint32 AddFile( const RawBundleEntry& fileHeaderInfo );

				Bool ApplyCensorship();
				Bool ApplyPatch();
				Bool DiscardCensorFiles();
				Bool CreateFinalLayout();
				Bool CreateFinalBundleLayout( const RawBundleData& bundle );
				Bool LinkBuffers(bool buildingPatch);
				Bool BuildInitData();

				//////////////////////////////////////////////////////////////////////////
				// Private Data
				//////////////////////////////////////////////////////////////////////////
				CBundlePreambleParser m_preambleParser;
				CDeprecatedIO& m_asyncIO;

				typedef THashMap< StringAnsi, Uint32 >		TPathToFileIDMap;
				typedef TDynArray< RawBundleData* >			TRawBundleData;
				typedef THashMap< StringAnsi, StringAnsi >	TAppliedCensorship;

				TPathToFileIDMap	m_fileMap;
				TRawBundleData		m_rawBundles;
			};
		}
	}
}

#endif // BUNDLE_METADATA_CACHE_BUILDER_H_