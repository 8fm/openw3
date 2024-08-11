/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/xmlFile.h"
#include "../core/xmlFileWriter.h"
#include "../core/xmlWriter.h"
#include "../core/xmlreader.h"
#include "../core/configFileManager.h"
#include "../core/filePath.h"
#include "../core/filesys.h"
#include "../core/depot.h"

#include "../core/configVar.h"
#include "inGameConfigInterface.h"

#include <functional>
#include "inGameConfigSpecialization.h"

namespace InGameConfig
{
	/*********************** Config Register ***********************/
	class CInGameConfig
	{
	public:
		typedef std::function<void(TDynArray< SConfigPresetOption >&, TDynArray<SConfigPresetEntry>&)> ListingFunc;

	public:
		~CInGameConfig();

		void GenerateConfigs( const String& rootPath );		// Generates configs from all config matrices placed in cvInGameConfigMatrixPath
		void RegisterConfigGroup( IConfigGroup* group );
		void UnregisterConfigGroup( IConfigGroup* group );
		void RegisterDynamicConfigGroup( IConfigDynamicGroup* group );		// group will be deleted at exit
		void ListAllConfigGroups( TDynArray< IConfigGroup* >& output );
		Bool FindGroupByName( const CName& groupName, IConfigGroup** outGroup );

		void ActivateTag( CName tag ) { m_activeTags.Insert( tag ); }
		void DeactivateTag( CName tag ) { m_activeTags.Erase( tag ); }
		Bool IsTagActive( CName tag ) { return m_activeTags.Exist( tag ); }
		const THashSet<CName>& GetTags() { return m_activeTags; }

	private:
		void GenerateConfigsFromFile( const String& filepath );

		Bool ParseUserConfig(CXMLReader* reader);
		Bool ParseConfigGroups(CXMLReader* reader);

		IConfigDynamicGroup* BuildStandardConfigGroup(CXMLReader* reader, const String& builderType);
		IConfigDynamicGroup* BuildInputConfigGroup(CXMLReader* reader, const String& builderType);

		void ParseVariables(CXMLReader* reader, TDynArray< IConfigVar* >& vars, const String& parentGroupBuilderType, const CName& parentGroupName);

		IConfigVar* BuildStandardVariable(CXMLReader* reader, const CName& parentGroupName);
		IConfigVar* BuildInputVariable(CXMLReader* reader, const CName& parentGroupName);

	private:
		TDynArray< IConfigGroup* > m_configGroups;
		TDynArray< IConfigDynamicGroup* > m_configDynamicGroups;
		THashSet<CName> m_activeTags;

	};

}

typedef TSingleton< InGameConfig::CInGameConfig > GInGameConfig;
