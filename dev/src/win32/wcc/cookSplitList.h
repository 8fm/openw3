/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SPLITFILE_DATA_BASE_H__
#define __SPLITFILE_DATA_BASE_H__

#include "../../common/core/resourceid.h"
#include "../../common/core/hashmap.h"

#include "jsonFileHelper.h"

//////////////////////////
/// Entry in the cooker seed file
class CCookerSplitFileEntry
{
public:
	typedef Red::Core::ResourceManagement::CResourceId TResourceID;
	typedef CName TChunkID;

	RED_INLINE const TResourceID& GetFileId() const { return m_fileId; }
	RED_INLINE const StringAnsi& GetFilePath() const { return m_filePath; }
	RED_INLINE const TChunkID& GetFileChunkID() const { return m_chunkId; }

protected:
	CCookerSplitFileEntry() 
	{}

	TResourceID		m_fileId;
	StringAnsi		m_filePath;
	TChunkID		m_chunkId;
};

//////////////////////////
/// Split list represents a final mapping of where each resource file goes
/// All files have a valid, resolved chunk ID set
class CCookerSplitFile : public JSONFileHelper
{
public:
	CCookerSplitFile();
	~CCookerSplitFile();

	// Reset data base
	void Reset();

	// Load cooker data base from file
	Bool LoadFromFile( const String& absoluteFilePath );

	// Save cooker data base to file
	Bool SaveToFile( const String& absoluteFilePath );

	//////////////////////////
	// Data base building

	// Add file entry, returns false if entry already exists with a different bundle name
	Bool AddEntry( const StringAnsi& depotPath, const CCookerSplitFileEntry::TChunkID fileChunkID );

	//////////////////////////
	// Data base access

	// Get number of file entries
	const Uint32 GetNumEntries() const;

	// Get file entry
	const CCookerSplitFileEntry* GetEntry( const Uint32 index ) const;

	// Get entry for resource ID
	const CCookerSplitFileEntry* GetEntry( const Red::Core::ResourceManagement::CResourceId& id ) const;

	// Get entry for given depot path
	const CCookerSplitFileEntry* GetEntry( const StringAnsi& deptotPath ) const;

private:
	struct FileEntry : public CCookerSplitFileEntry
	{
		FileEntry(const StringAnsi& depotPath, const CCookerSplitFileEntry::TChunkID chunkID );

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

