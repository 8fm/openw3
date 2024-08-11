/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __DEPOT_BUNDLE_ENTRY_H__
#define __DEPOT_BUNDLE_ENTRY_H__

#include "diskFile.h"
#include "fileHandleCache.h" // sync
#include "../redIO/redIOAsyncFileHandleCache.h" // async

class CBundleDataCache;
class CDiskBundleIOCache;
class CDiskBundleContent;

/// A mapped depot bundle
class CDiskBundle : public IDepotObject, public Red::System::NonCopyable
{
public:
	CDiskBundle( const StringAnsi& shortName, const StringAnsi& fullPath, const String& absoluteFilePath, const Red::Core::Bundle::BundleID bundleId );
	~CDiskBundle();
	void OnAttached();

	//! Get bundle ID
	RED_INLINE const Red::Core::Bundle::BundleID GetBundleID() const { return m_bundleId; }

	//! Get bundle short name (no path) NOTE: does not have to be unique
	RED_INLINE const StringAnsi& GetShortName() const { return m_shortName; }

	//! Get full bundle path
	RED_INLINE const StringAnsi& GetFullPath() const { return m_fullPath; }

	//! Preload bundle resource content, returns all loaded resources
	THandle< CDiskBundleContent > Preload();

	//! Create a bundle file reader for a single file access, not used directly, called from CDepotBundles
	IFile* CreateReader( CBundleDataCache& dataCache, CDiskBundleIOCache & ioCache, const Red::Core::Bundle::SMetadataFileInBundlePlacement& placement );

	//! Create asynchronous reader
	virtual EAsyncReaderResult CreateAsyncReader( const Red::Core::Bundle::SMetadataFileInBundlePlacement& placement, const Uint8 ioTag, IAsyncFile*& outAsyncReader ) const;

	//! (synchronous) Read raw bundle data
	Bool ReadRawData( void* ptr, const Uint32 offset, const Uint32 size, Uint32& outNumBytesRead );

private:
	typedef Red::IO::CAsyncFileHandleCache::TFileHandle	AsyncFileHandle;

	StringAnsi						m_shortName;
	StringAnsi						m_fullPath;
	Red::Core::Bundle::BundleID		m_bundleId;
	CNativeFileHandleWrapper		m_syncFileHandle;
	AsyncFileHandle					m_asyncFileHandle;
};

#endif
