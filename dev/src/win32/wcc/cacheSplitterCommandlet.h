/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __CACHEBUILDER_COMMANDLET_H__
#define __CACHEBUILDER_COMMANDLET_H__

#include "../../common/core/commandlet.h"

#include "baseCacheSplitter.h"
#include "cookDataBase.h"
#include "cookSplitList.h"

//---------------------------------------------------------

/// Generic cached data splitter (for shaders, textures, etc) 
/// This is used for PlayGo/StreamInstall stuff
class CCacheSplitterCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CCacheSplitterCommandlet, ICommandlet, 0 );

public:
	CCacheSplitterCommandlet();
	~CCacheSplitterCommandlet();

	// Interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Split cache file into multiple files"); }
	virtual void PrintHelp() const;

private:
	struct Settings
	{
		String					m_rootOutputDirectory;
		String					m_inputFile;
		String					m_splitFilePath;
		String					m_builderName;

		CName					m_fallBackChunk;
		Bool					m_stripNotCooked;

		Settings();

		Bool Parse( const CommandletOptions& options );
	};

	struct OutputCache
	{
		CName							m_name;
		TDynArray< IBaseCacheEntry* >	m_entries;

		OutputCache( CName name )
			: m_name( name )
		{}
	};

	Settings				m_settings;

	CCookerSplitFile		m_splitFile;
};

BEGIN_CLASS_RTTI( CCacheSplitterCommandlet )
	PARENT_CLASS( ICommandlet );
END_CLASS_RTTI()

#endif

//---------------------------------------------------------
