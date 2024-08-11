/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/gameSave.h"

// This is an implementation of save files which use skip-blocks, rather than seek-on-write to handle
// skipping bad data on load.
class CGameFileSaverNoSeeks : public IGameSaver
{
public:
	CGameFileSaverNoSeeks( IFileEx* saveDataFile );
	virtual ~CGameFileSaverNoSeeks();
	virtual void BeginBlock( CName name );
	virtual void EndBlock( CName name );
	virtual void WriteRawAnsiValue( CName name, const AnsiChar* data, Uint32 size );
	virtual void WriteValue( CName name, IRTTIType* type, const void* data );
	virtual void WriteProperty( void* object, CProperty* prop );
	virtual void AddStorageStream( IGameDataStorage* storageStream );
	virtual void Finalize();
	virtual void Close();
	virtual const void* GetData() const;
	virtual Uint32 GetDataSize() const;
	virtual Uint32 GetDataCapacity() const;
private:
	struct BlockData
	{
		BlockData() { }
		BlockData( CName name, Uint32 skipOffset ) : m_name( name ), m_skipOffset( skipOffset ) { }
		CName m_name;
		Uint32 m_skipOffset;
	};
	TDynArray<BlockData> m_openBlockStack;		// Track open block stack
	TDynArray<BlockData> m_rootBlocks;			// Track 'root' block offsets

	IFileEx* m_saveDataFile;
};

class CGameFileLoaderNoSeeks : public IGameLoader
{
public:
	CGameFileLoaderNoSeeks( IFile* saveDataFile, Uint32 gameVersion, Uint32 saveVersion );
	virtual ~CGameFileLoaderNoSeeks();

	virtual Uint32 GetGameVersion() const;
	virtual Uint32 GetSaveVersion() const;
	virtual void BeginBlock( CName blockName );
	virtual void EndBlock( CName blockName );
	virtual void ReadValue( CName name, IRTTIType* type, const void* data, CObject* defaultParent );
	virtual void ReadProperty( void* object, CClass* theClass, CObject* defaultParent );
	virtual IGameDataStorage* ExtractDataStorage();
	virtual void SkipDataStorage();

private:
	struct OpenBlock
	{
		OpenBlock() { }
		OpenBlock( CName name, Uint32 dataEnd )
			: m_name( name ), m_dataEndOffset( dataEnd ) { }
		CName m_name;
		Uint32 m_dataEndOffset;
	};

	Bool ValueTypeIsCompatible( IRTTIType* requestedType, CName actualTypeName );
	Bool LoadCNameRemapping( Uint32 fileOffset );
	Bool LoadRootblockData( Uint32 fileOffset );

	IFile* m_saveDataFile;
	Uint32 m_saveVersion;
	Uint32 m_gameVersion;
	THashMap< CName, Uint32 > m_rootBlockOffsets;	// Root-block lookup
	TDynArray< OpenBlock > m_openBlockStack;
};