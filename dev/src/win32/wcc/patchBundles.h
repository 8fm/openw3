/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "patchBuilder.h"

#include "../../common/core/bundleMetadataStore.h"
#include "../../common/core/bundleMetadataStoreEntry.h"

//-----------------------------------------------------------------------------

typedef Red::Core::Bundle::SMetadataFileInBundlePlacement					TFilePlacement;
typedef TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >		TFileEntries;
typedef TDynArray< StringAnsi >												TFileNames;

class CPatchBundleFileToken;

//-----------------------------------------------------------------------------

/// Bundle information
class CPatchBundleFile
{
public:
	~CPatchBundleFile();

	// Load single bundle from given path
	static CPatchBundleFile* LoadBundle( const String& absolutePath );

	// Is this a DLC bundle ?
	RED_INLINE const Bool IsDLC() const { return m_isDLC; }

	// Get bundle path
	RED_INLINE const String& GetAbsolutePath() const { return m_absoluteFilePath; }

	// Get bundle relative path (relative to content\ dir in the build)
	RED_INLINE const String& GetRelativePath() const { return m_relativePath; }	

	// Get short bundle name (the file name)
	RED_INLINE const String& GetShortName() const { return m_shortName; }

	// Get bundle entries
	RED_INLINE const TFileEntries& GetFileEntries() const { return m_entries; }

	// Get bundle file names
	RED_INLINE const TFileNames& GetFileNames() const { return m_names; }

	// Is atomic ?
	RED_INLINE const Bool IsAtomic() const { return m_isAtomic; }

	// Load file data
	IFile* LoadFileData( const TFilePlacement& placement ) const;

	// Load raw file data (as it is in the bundle)
	IFile* LoadRawFileData( const TFilePlacement& placement ) const;

	// Copy bundle to remote location
	Bool Copy( const String& absolutePath ) const;

private:
	String			m_absoluteFilePath;
	String			m_shortName;
	String			m_relativePath;

	TFileEntries	m_entries;
	TFileNames		m_names;

	IFile*			m_file;

	Bool			m_isAtomic; // bundle should be replaced as a whole
	Bool			m_isDLC; // bundle is coming from a DLC
};

//-----------------------------------------------------------------------------

/// Group of bundles (the build)
class CPatchBundles : public IBasePatchContentBuilder::IContentGroup
{
public:
	/// Interface
	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;

	/// Load all of the bundles from given directory
	static CPatchBundles* LoadBundles( const String& baseDirectory );

private:
	String									m_basePath;
	TDynArray< CPatchBundleFile* >			m_bundles;
	TDynArray< CPatchBundleFileToken* >		m_tokens;
};

//-----------------------------------------------------------------------------

/// Information about single patchable file in the bundles
class CPatchBundleFileToken : public IBasePatchContentBuilder::IContentToken
{
public:
	CPatchBundleFileToken( const StringAnsi& filePath, CPatchBundleFile* bundleFile, const TFilePlacement& placement );
	~CPatchBundleFileToken();

	/// Refresh data CRC
	Bool RefreshCRC();

	/// Create data reader (uncompressed file data)
	IFile* CreateReader() const;

	/// Create data reader (raw data directly form bundle, can be compressed)
	IFile* CreateRawReader() const;

	/// Access
	RED_INLINE const StringAnsi& GetFilePath() const { return m_filePath; }
	RED_INLINE const CPatchBundleFile* GetBundleFile() const { return m_bundleFile; }
	RED_INLINE const TFilePlacement& GetPlacement() const { return m_bundlePlacement; }

	/// IContentToken interface
	virtual const Uint64 GetTokenHash() const override;
	virtual const Uint64 GetDataCRC() const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;
	virtual const String GetAdditionalInfo() const override;
	virtual void DebugDump( const String& dumpPath, const Bool isBase ) const override;

public:
	StringAnsi			m_filePath;
	Uint64				m_fileHash;

	Uint64				m_dataCRC;

	CPatchBundleFile*	m_bundleFile;
	TFilePlacement		m_bundlePlacement;

	TDynArray< CPatchBundleFile* >		m_owningBundles;
};

//-----------------------------------------------------------------------------

/// Patch builder for bundles
class CPatchBuilder_Bundles : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_Bundles );

public:
	CPatchBuilder_Bundles();
	~CPatchBuilder_Bundles();

	/// Interface
	virtual Bool CanUseWithMods() const override { return true; }
	virtual String GetContentType() const override;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;
};

BEGIN_CLASS_RTTI( CPatchBuilder_Bundles );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
