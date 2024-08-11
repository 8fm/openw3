/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/gameSaveManager.h"

#ifndef NO_SAVE_IMPORT
class CW2PCGameFileLoader : public CGameStorageLoader
{
public:
	CW2PCGameFileLoader( CGameStorageReader* file, Uint32 saveVersion );
	~CW2PCGameFileLoader();

	Uint32 GetGameVersion() const override { return 1; }

	Uint32 GetSaveVersion() const override { return m_saveVersion; }
};


class CW2SaveImporter : public IObsoleteSaveImporter
{
protected:
	TDynArray< String > m_searchPaths;

public:
	CW2SaveImporter();

	static String FindStandardSavesPath();
	void AddSearchPath( const String& path ) { m_searchPaths.PushBack( path ); }

	// IObsoleteSaveImporter
	const TDynArray< String >& GetSearchPaths() const { return m_searchPaths; }
	virtual void GetSaveFiles( TDynArray< SSavegameInfo >& files ) const;
	virtual Bool ImportSave( const SSavegameInfo& file ) const;

private:
	IGameLoader* CreateLoader( const SSavegameInfo& file ) const;
};
	
IObsoleteSaveImporter* createSaveImporter();

#endif // NO_SAVE_IMPORT