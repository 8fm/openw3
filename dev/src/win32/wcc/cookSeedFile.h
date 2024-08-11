/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SEED_DATA_BASE_H__
#define __SEED_DATA_BASE_H__

#include "../../common/core/resourceid.h"
#include "../../common/core/hashmap.h"

#include "jsonFileHelper.h"

//////////////////////////
/// Entry in the cooker seed file
class CCookerSeedFileEntry
{
public:
	typedef Red::Core::ResourceManagement::CResourceId TResourceID;
	typedef TDynArray< CName > TChunkIDs;

	RED_INLINE const StringAnsi& GetFilePath() const { return m_filePath; }
	RED_INLINE const StringAnsi& GetBundleName() const { return m_bundleName; }
	RED_INLINE const TChunkIDs& GetFileChunkIDs() const { return m_chunkIDs; }
	
protected:
	CCookerSeedFileEntry() 
	{}

	StringAnsi		m_filePath;
	StringAnsi		m_bundleName;
	TChunkIDs		m_chunkIDs;
};

//////////////////////////
/// Seed files are simplified JSON bundle definition files
class CCookerSeedFile : public JSONFileHelper
{
public:
	CCookerSeedFile();
	~CCookerSeedFile();

	// Reset data base
	void Reset();

	// Load cooker data base from file
	Bool LoadFromFile( const String& absoluteFilePath );

	// Save cooker data base to file
	Bool SaveToFile( const String& absoluteFilePath );

	// Initialize from analyzer output list
	Bool Initialize( const class CAnalyzerOutputList& fileList );

	//////////////////////////
	// Data base building

	// Add file entry, returns false if entry already exists with a different bundle name
	Bool AddEntry( const StringAnsi& depotPath, const StringAnsi& bundleName, const CCookerSeedFileEntry::TChunkIDs* fileChunkIDs = nullptr );

	// Remove file entry, returns false if entry does not exist
	Bool RemoveEntry( const StringAnsi& depotPath, const StringAnsi& bundleName );

	//////////////////////////
	// Data base access

	// Get number of file entries
	const Uint32 GetNumEntries() const;

	// Get file entry
	const CCookerSeedFileEntry* GetEntry( const Uint32 index ) const;

private:
	struct FileEntry : public CCookerSeedFileEntry
	{
		FileEntry(const StringAnsi& depotPath, const StringAnsi& bundleName, const CCookerSeedFileEntry::TChunkIDs* chunkIDs );

		static FileEntry* Parse( const JSONValue& reader );
		void Write(JSONWriter& writer) const;
	};

	// list of file entries
	typedef TDynArray< FileEntry* >		TEntries;
	TEntries		m_entries;

	// file map (for fast lookups)
	typedef THashMap< Red::Core::ResourceManagement::CResourceId, FileEntry* >		TEntriesMap;
	TEntriesMap		m_entriesMap;
};

#endif

