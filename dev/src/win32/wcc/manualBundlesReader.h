/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/bundleMetadataStore.h"
#include "../../common/core/bundleMetadataStoreEntry.h"

/// Manual reader for bundle files
class CManualBundleReader
{
public:
	CManualBundleReader();
	~CManualBundleReader();

	/// Clear content
	void Clear();

	/// Add bundles from location
	void AddBundles( const String& rootDirectory );

	/// Resolve path hash to a depot path
	Bool ResolvePathHash( const Uint64 pathHash, String& outDepotPath ) const;

	/// Create reader for given file
	IFile* CreateReader( const String& depotPath ) const;

	/// Create raw reader for given file, this will read compressed data if necessary
	IFile* CreateRawReader( const String& depotPath ) const;

	/// Get compressed size of file, 0 if not there
	Uint32 GetCompressedFileSize( const String& depotPath ) const;

	/// Get size of the file in memory, 0 if not there
	Uint32 GetMemoryFileSize( const String& depotPath ) const;

	/// Get type of compression
	Uint32 GetCompressionType( const String& depotPath ) const;

private:
	struct BundleInfo;

	typedef Red::Core::Bundle::SMetadataFileInBundlePlacement					TFilePlacement;

	struct FileInfo
	{
		BundleInfo*			m_bundle;
		Uint32				m_fileIndex;
		String				m_depotPath;
		TFilePlacement		m_placement;

		// create file reader to read actuall file data
		IFile* CreateReader() const;

		// create file reader to read raw data (this will read compressed data)
		IFile* CreateRawReader() const;

	};

	struct BundleInfo
	{
		String					m_absolutePath;		// path to file (for reading)
		IFile*					m_file;				// opened handle to file
		TDynArray< FileInfo* >	m_files;			// files in the bundle

		~BundleInfo()
		{
			delete m_file;
			m_files.ClearPtr();
		}
	};

	TDynArray< BundleInfo* >		m_bundles;		// all bundles
	THashMap< String, FileInfo* >	m_files;		// only first is file mapped
	THashMap< Uint64, FileInfo* >	m_filesByHash;	// only first is file mapped, mapped by path hash
};