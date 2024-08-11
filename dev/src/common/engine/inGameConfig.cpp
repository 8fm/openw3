#include "build.h"
#include "inGameConfig.h"
#include "inGameConfigSpecialization.h"
#include "InGameConfigParser.h"
#include "inputMappingInGameConfig.h"

namespace Config
{
	TConfigVar<String> cvInGameConfigMatrixPath( "InGameConfig", "MatrixPath", TXT(""), eConsoleVarFlag_ReadOnly );
}

namespace
{
	const Char* FILE_SEARCH_EXTENSION = TXT("*.xml");
}

namespace VisibilityByTagExpression
{
	class CTagExp : public BoolExpression::IExp
	{
	public:
		CTagExp( CName tag ) : m_tag( tag ) {}
		virtual Bool Evaluate() { return GInGameConfig::GetInstance().IsTagActive( m_tag ); }
		virtual void Destroy() { delete this; }

	private:
		CName m_tag;
	};

	//  !A&B|A&!B&C
	//  !A&B A&!B&C		Or Stack
	//  !A B A !B C		And Stack

	//	      or
	//	   /     \
	//	 and     and
	//	 / \     / \
	//	!A  B  and  C
	//	       / \
	//		  A  !B

	BoolExpression::IExp* Parse( const String& expressionStr )
	{
		// Split by '|' (or operations)
		TDynArray<String> orOperations = expressionStr.Split( TXT("|") );
		TDynArray<BoolExpression::IExp*> orIngrExpStack;
		for( auto& orOp : orOperations )
		{
			// Split by '&' (and operations)
			TDynArray<String> andOperations = orOp.Split( TXT("&") );
			TDynArray<BoolExpression::IExp*> andIngrExpStack;
			for( auto& andOp : andOperations )
			{
				// Not operation
				if( andOp.BeginsWith( TXT("!") ) )
				{
					String tagStr = andOp.RightString( andOp.GetLength()-1 );
					CName tag = CName( tagStr );
					andIngrExpStack.PushBack( new BoolExpression::CNotExp( new CTagExp( tag ) ) );
				}
				else	// regular expression
				{
					CName tag = CName( andOp );
					andIngrExpStack.PushBack( new CTagExp( tag ) );
				}

				if( andIngrExpStack.Size() > 1 )		// Create And operation
				{
					BoolExpression::IExp* left = andIngrExpStack[0];
					BoolExpression::IExp* right = andIngrExpStack[1];
					andIngrExpStack.Clear();
					andIngrExpStack.PushBack( new BoolExpression::CAndExp( left, right ) );
				}
			}

			RED_ASSERT( andIngrExpStack.Size() == 1, TXT("And-expression-stack size should be always 1 here") );
			orIngrExpStack.PushBack( andIngrExpStack[0] );
			andIngrExpStack.Clear();

			if( orIngrExpStack.Size() > 1 )		// Create Or operation
			{
				BoolExpression::IExp* left = orIngrExpStack[0];
				BoolExpression::IExp* right = orIngrExpStack[1];
				orIngrExpStack.Clear();
				orIngrExpStack.PushBack( new BoolExpression::COrExp( left, right ) );
			}
		}

		// Get root operation
		RED_ASSERT( orIngrExpStack.Size() == 1, TXT("Or-expression-stack size should be always 1 here") );
		BoolExpression::IExp* result = orIngrExpStack[0];
		return result;
	}

};

namespace InGameConfig
{

	CInGameConfig::~CInGameConfig()
	{
		for( auto group : m_configDynamicGroups )
		{
			group->Discard();
		}

		m_configDynamicGroups.ClearPtr();
	}

	void CInGameConfig::RegisterConfigGroup(IConfigGroup* group)
	{
		for( auto configGroup : m_configGroups )
		{
			RED_ASSERT( configGroup->GetDisplayName() != group->GetDisplayName(), TXT("Config group names collision for: %ls. Name should be unique."), configGroup->GetDisplayName().AsChar() );
		}
		m_configGroups.PushBack( group );
	}

	void CInGameConfig::UnregisterConfigGroup(IConfigGroup* group)
	{
		m_configGroups.Remove( group );
	}

	void CInGameConfig::RegisterDynamicConfigGroup(IConfigDynamicGroup* group)
	{
		for( auto configGroup : m_configGroups )
		{
			RED_ASSERT( configGroup->GetDisplayName() != group->GetDisplayName(), TXT("Config group names collision for: %ls. Name should be unique."), configGroup->GetDisplayName().AsChar() );
		}
		m_configGroups.PushBack( group );
		m_configDynamicGroups.PushBack( group );
	}

	void CInGameConfig::ListAllConfigGroups(TDynArray< IConfigGroup* >& output)
	{
		for( auto configGroup : m_configGroups )
		{
			output.PushBack( configGroup );
		}
	}

	Bool CInGameConfig::FindGroupByName(const CName& groupName, IConfigGroup** outGroup)
	{
		for( IConfigGroup* configGroup : m_configGroups )
		{
			if( configGroup->GetConfigId() == groupName )
			{
				(*outGroup) = configGroup;
				return true;
			}
		}

		return false;
	}

	void CInGameConfig::GenerateConfigs( const String& rootPath )
	{
		String path = rootPath + Config::cvInGameConfigMatrixPath.Get() + Config::GetPlatformString< Char >() + CFileManager::DIRECTORY_SEPARATOR_STRING;
		TDynArray< String > files;

#ifdef RED_PLATFORM_ORBIS
		String filelistString;
		GFileManager->LoadFileToString( path + TXT("filelist.txt"), filelistString, true );
		TDynArray<String> filelist = filelistString.Split( TXT(";") );
		for( String& filename : filelist )
		{
			filename.Trim();
			files.PushBack( path + filename );
		}
#else
		GFileManager->FindFiles( path.AsChar(), FILE_SEARCH_EXTENSION, files, false );
#endif
		LOG_CORE( TXT("Found %d engine configs in path %ls"), files.Size(), path.AsChar() );

		for ( const String& configFilePath : files )
			GenerateConfigsFromFile( configFilePath );
	}

	void CInGameConfig::GenerateConfigsFromFile(const String& filepath)
	{
		LOG_CORE( TXT("Loading in-game configs from file: %ls"), filepath.AsChar() );
		String xmlContent = TXT("");
		Bool result = GFileManager->LoadFileToString( filepath, xmlContent, true );
		RED_ASSERT( result, TXT("Can't load or find config matrix file in: %ls - so no configs will be visible in UI!"), filepath.AsChar() );
		CXMLReader reader( xmlContent );

		ParseUserConfig( &reader );
	}

	Bool CInGameConfig::ParseUserConfig(CXMLReader* reader)
	{
		if( reader->BeginNode( TXT("UserConfig") ) == true )
		{
			Bool result = ParseConfigGroups( reader );

			reader->EndNode( false );

			return result;
		}

		ERR_ENGINE( TXT("InGameConfig parser: file root should be UserConfig") );

		return false;
	}

	Bool CInGameConfig::ParseConfigGroups(CXMLReader* reader)
	{
		while( reader->BeginNode( TXT("Group") ) == true )
		{
			String builderTypeString = TXT("Standard");
			reader->Attribute( TXT("builder"), builderTypeString );			

			IConfigDynamicGroup* group = nullptr;

			if( builderTypeString == TXT("Standard") )
			{
				group = BuildStandardConfigGroup( reader, builderTypeString );
			}
			else if( builderTypeString == TXT("Input") )
			{
				group = BuildInputConfigGroup( reader, builderTypeString );
			}
			else	// Fallback is standard builder
			{
				group = BuildStandardConfigGroup( reader, builderTypeString );
			}

			if( group != nullptr )
			{
				RegisterDynamicConfigGroup( group );
			}

			reader->EndNode();
		}

		return true;
	}

	IConfigDynamicGroup* CInGameConfig::BuildStandardConfigGroup(CXMLReader* reader, const String& builderType)
	{
		CStandardGroupParser groupParser( reader );

		// Obligatory parameters
		CName name;
		if( groupParser.ParseName( name ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config group name. Name is obligatory!") );
			return nullptr;
		}

		String displayName;
		if( groupParser.ParseDisplayName( displayName ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config group display name. Display name is obligatory!") );
			return nullptr;
		}

		// Optional parameters
		THashSet<CName> tags;
		groupParser.ParseTags( tags );

		BoolExpression::IExp* visibilityCondition = nullptr;
		String visibilityConditionString;
		if( reader->Attribute( TXT("visibilityCondition"), visibilityConditionString ) )
		{
			visibilityCondition = VisibilityByTagExpression::Parse( visibilityConditionString );
		}

		TDynArray< SConfigPresetOption > presets;
		TDynArray< SConfigPresetEntry > presetEntries;
		groupParser.ParsePresets( presets, presetEntries, name );

		// Variables
		TDynArray< IConfigVar* > vars;
		ParseVariables( reader, vars, builderType, name );

		return new CConfigGroupEngineConfig( name, displayName, presets, presetEntries, vars, tags, visibilityCondition );
	}

	IConfigDynamicGroup* CInGameConfig::BuildInputConfigGroup(CXMLReader* reader, const String& builderType)
	{
		CStandardGroupParser groupParser( reader );

		// Obligatory parameters
		CName name;
		if( groupParser.ParseName( name ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config group name. Name is obligatory!") );
			return nullptr;
		}

		String displayName;
		if( groupParser.ParseDisplayName( displayName ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config group display name. Display name is obligatory!") );
			return nullptr;
		}

		// Optional parameters
		THashSet<CName> tags;
		groupParser.ParseTags( tags );

		BoolExpression::IExp* visibilityCondition = nullptr;
		String visibilityConditionString;
		if( reader->Attribute( TXT("visibilityCondition"), visibilityConditionString ) )
		{
			visibilityCondition = VisibilityByTagExpression::Parse( visibilityConditionString );
		}

		// Variables
		TDynArray< IConfigVar* > vars;
		ParseVariables( reader, vars, builderType, name );

		CInputMappingInGameConfigGroup::ConstructParams inputGroupParams;

		inputGroupParams.basicParams.name = name;
		inputGroupParams.basicParams.displayName = displayName;
		inputGroupParams.basicParams.tags = tags;
		inputGroupParams.basicParams.visibilityCondition = visibilityCondition;

		inputGroupParams.vars = vars;

		return new CInputMappingInGameConfigGroup( inputGroupParams );
	}

	void CInGameConfig::ParseVariables(CXMLReader* reader, TDynArray< IConfigVar* >& vars, const String& parentGroupBuilderType, const CName& parentGroupName )
	{
		if( reader->BeginNode( TXT("VisibleVars") ) == true )
		{
			while( reader->BeginNode( TXT("Var") ) == true )
			{
				String builderTypeString = TXT("Standard");
				reader->Attribute( TXT("builder"), builderTypeString );

				IConfigVar* variable = nullptr;

				if( builderTypeString == TXT( "Standard" ) && parentGroupBuilderType == TXT( "Standard" ) )
				{
					variable = BuildStandardVariable( reader, parentGroupName );
				}
				else if( builderTypeString == TXT( "Input" ) && parentGroupBuilderType == TXT( "Input" ) )
				{
					variable = BuildInputVariable( reader, parentGroupName );
				}
				else	// Fallback is standard builder
				{
					variable = BuildStandardVariable( reader, parentGroupName );
				}

				vars.PushBack( variable );
				
				reader->EndNode();
			}

			reader->EndNode();
		}
	}

	IConfigVar* CInGameConfig::BuildStandardVariable(CXMLReader* reader, const CName& parentGroupName)
	{
		CStandardVariableParser variableParser( reader );

		// Obligatory parameters
		CName name;
		if( variableParser.ParseName( name ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config variable name. Name is obligatory!") );
			return nullptr;
		}

		String displayName;
		if( variableParser.ParseDisplayName( displayName ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config variable display name. Display name is obligatory!") );
			return nullptr;
		}

		String displayType;
		if( variableParser.ParseDisplayType( displayType ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config variable display name. Display type is obligatory!") );
			return nullptr;
		}

		// Optional parameters
		THashSet<CName> tags;
		variableParser.ParseTags( tags );

		BoolExpression::IExp* visibilityCondition = nullptr;
		String visibilityConditionString;
		if( reader->Attribute( TXT("visibilityCondition"), visibilityConditionString ) )
		{
			visibilityCondition = VisibilityByTagExpression::Parse( visibilityConditionString );
		}

		TDynArray< SConfigPresetOption > options;
		TDynArray< SConfigPresetEntry > optionEntries;
		CName listingFunction;
		variableParser.ParseOptions( options, optionEntries, listingFunction, parentGroupName );
		variableParser.ParseListingFunction( listingFunction );		// listing function parsed by function above this one is deprecated, but we still have to support it for old configs

		CName groupName;
		if( variableParser.ParseOverrideGroupName( groupName ) == false )
		{
			groupName = parentGroupName;
		}

		SConfigVarCreationDesc varDesc;
		varDesc.groupName = groupName;
		varDesc.name = name;
		varDesc.displayName = displayName;
		varDesc.displayType = displayType;
		varDesc.options = options;
		varDesc.optionEntries = optionEntries;
		varDesc.tags = tags;
		varDesc.visibilityCondition = visibilityCondition;
		varDesc.listingFunction = listingFunction;

		return new CConfigVarEngineConfig( varDesc );
	}

	IConfigVar* CInGameConfig::BuildInputVariable(CXMLReader* reader, const CName& parentGroupName)
	{
		CStandardVariableParser variableParser( reader );

		// Obligatory parameters
		CName name;
		if( variableParser.ParseName( name ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config variable name. Name is obligatory!") );
			return nullptr;
		}

		String displayName;
		if( variableParser.ParseDisplayName( displayName ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config variable display name. Display name is obligatory!") );
			return nullptr;
		}

		String displayType;
		if( variableParser.ParseDisplayType( displayType ) == false )
		{
			ERR_ENGINE( TXT("InGameConfig parser: Can't parse config variable display name. Display type is obligatory!") );
			return nullptr;
		}

		Bool isPadInput = false;
		if( displayType == TXT("INPUTPC") )
		{
			isPadInput = false;
		}
		else
		{
			isPadInput = true;
		}

		// Optional parameters
		THashSet<CName> tags;
		variableParser.ParseTags( tags );

		BoolExpression::IExp* visibilityCondition = nullptr;
		String visibilityConditionString;
		if( reader->Attribute( TXT("visibilityCondition"), visibilityConditionString ) )
		{
			visibilityCondition = VisibilityByTagExpression::Parse( visibilityConditionString );
		}

		CName groupName;
		if( variableParser.ParseOverrideGroupName( groupName ) == false )
		{
			groupName = parentGroupName;
		}

		String actionsString;
		reader->Attribute( TXT("actions"), actionsString );
		TDynArray<String> allActions = actionsString.Split( TXT(";") );
		TDynArray<CName> actions;

		CInputMappingInGameConfigVar::ConstructParams params;
		params.basicParams.name = name;
		params.basicParams.displayName = displayName;
		
		params.basicParams.tags = tags;
		params.basicParams.visibilityCondition = visibilityCondition;

		params.isPadInput = isPadInput;

		for( String& action : allActions )
		{
			params.actions.PushBack( CName( action ) );
		}

		return new CInputMappingInGameConfigVar( params );
	}

}
