/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __CACHEBUILDER_COMMANDLET_H__
#define __CACHEBUILDER_COMMANDLET_H__

#include "../../common/core/commandlet.h"

#include "cookDataBase.h"

//---------------------------------------------------------

/// Generic cached data compiler (for shaders, textures, etc) 
class CCacheBuilderCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CCacheBuilderCommandlet, ICommandlet, 0 );

public:
	CCacheBuilderCommandlet();
	~CCacheBuilderCommandlet();

	// Interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Build data cache from cooked assets"); }
	virtual void PrintHelp() const;

private:
	typedef TDynArray< CCookerDataBase::TCookerDataBaseID > TFiles;		
	typedef TDynArray< const CClass* > TClasses;

	struct Settings
	{
		String					m_dataBasePath;
		String					m_outputFilePath;
		ECookingPlatform		m_platform;

		String					m_builderName;

		Uint32					m_distributeModulo;
		Uint32					m_distributeOffset;

		Settings();

		Bool Parse( const CommandletOptions& options );
	};

	Settings				m_settings;

	CCookerDataBase			m_dataBase;
	TClasses				m_classes;
	TFiles					m_files;	
};

BEGIN_CLASS_RTTI(CCacheBuilderCommandlet)
	PARENT_CLASS( ICommandlet );
END_CLASS_RTTI()

#endif