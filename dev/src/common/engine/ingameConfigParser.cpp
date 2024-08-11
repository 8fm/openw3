/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inGameConfigParser.h"
#include "../core/xmlReader.h"

namespace InGameConfig
{
	/***** Common Parser *****/

	CCommonParserBase::CCommonParserBase(CXMLReader* reader)
		: m_reader( reader )
	{
		/* Intentionally empty */
	}

	Bool CCommonParserBase::ParseName(CName& name)
	{
		String nameString;
		Bool result = m_reader->Attribute( TXT("id"), nameString );
		if( result == true )
		{
			name = CName( nameString );
			return true;
		}
		
		return false;
	}

	Bool CCommonParserBase::ParseDisplayName(String& name)
	{
		return m_reader->Attribute( TXT("displayName"), name );
	}

	Bool CCommonParserBase::ParseTags(THashSet<CName>& tags)
	{
		String groupTagsString;
		if( m_reader->Attribute( TXT("tags"), groupTagsString ) )
		{
			auto groupTagsStrTable = groupTagsString.Split(TXT(";"));
			for( auto& tagStr : groupTagsStrTable )
			{
				tags.Insert( CName(tagStr) );
			}

			return true;
		}

		return false;
	}

	/***** Standard Variable Parser *****/

	CStandardVariableParser::CStandardVariableParser(CXMLReader* reader )
		: CCommonParserBase( reader )
	{
		/* Intentionally empty */
	}

	Bool CStandardVariableParser::ParseDisplayType(String& displayType)
	{
		return m_reader->Attribute( TXT("displayType"), displayType );
	}

	Bool CStandardVariableParser::ParseOverrideGroupName(CName& overrideGroupName)
	{
		String nameString;
		Bool result = m_reader->Attribute( TXT("overrideGroup"), nameString );
		if( result == true )
		{
			overrideGroupName = CName( nameString );
			return true;
		}

		return false;
	}

	Bool CStandardVariableParser::ParseOptions(TDynArray< SConfigPresetOption >& options, TDynArray< SConfigPresetEntry >& optionEntries, CName& listingFunction, const CName& groupName)
	{
		if( m_reader->BeginNode( TXT("OptionsArray") ) == true )
		{
			while( m_reader->BeginNode( TXT("Option") ) == true )
			{
				String optionIdStr;
				Int32 optionId;
				String optionDisplayName;

				m_reader->Attribute( TXT("id"), optionIdStr );
				m_reader->Attribute( TXT("displayName"), optionDisplayName );

				FromString( optionIdStr, optionId );
				options.PushBack( SConfigPresetOption( optionId, optionDisplayName ) );

				while( m_reader->BeginNode( TXT("Entry") ) == true )
				{
					String optionVarGroup;
					String optionVarIdStr;
					String optionVarValue;

					CName optionVarGroupcname = groupName;

					if( m_reader->Attribute( TXT("overrideGroup"), optionVarGroup ) == true )
					{
						optionVarGroupcname = CName( optionVarGroup );
					}
					m_reader->Attribute( TXT("varId"), optionVarIdStr );
					m_reader->Attribute( TXT("value"), optionVarValue );

					CName optionVarIdStrcname = CName( optionVarIdStr );

					optionEntries.PushBack( SConfigPresetEntry( optionId, optionVarGroupcname, optionVarIdStrcname, optionVarValue ) );

					m_reader->EndNode();
				}

				m_reader->EndNode();
			}

			while( m_reader->BeginNode( TXT("ListingFunction") ) == true )
			{
				String funcName;
				m_reader->Attribute( TXT("name"), funcName );

				listingFunction = CName( funcName );

				m_reader->EndNode();
			}

			m_reader->EndNode();

			return true;
		}

		return false;
	}

	Bool CStandardVariableParser::ParseListingFunction(CName& listingFunction)
	{
		String nameString;
		Bool result = m_reader->Attribute( TXT("ListingFunction"), nameString );
		if( result == true )
		{
			listingFunction = CName( nameString );
			return true;
		}

		return false;
	}

	/***** Standard Group Parser *****/

	CStandardGroupParser::CStandardGroupParser(CXMLReader* reader)
		: CCommonParserBase( reader )
	{
		/* Intentionally empty */
	}

	Bool CStandardGroupParser::ParsePresets(TDynArray< SConfigPresetOption >& presets, TDynArray< SConfigPresetEntry >& presetEntries, const CName& groupName)
	{
		if( m_reader->BeginNode( TXT("PresetsArray") ) == true )
		{
			while( m_reader->BeginNode( TXT("Preset") ) == true )
			{
				/***** Initialize preset *****/
				Int32 presetId;
				String presetIdStr;
				String presetDisplayName;

				m_reader->Attribute( TXT("id"), presetIdStr );
				FromString( presetIdStr, presetId );
				m_reader->Attribute( TXT("displayName"), presetDisplayName );
				presets.PushBack( SConfigPresetOption( presetId, presetDisplayName ) );

				while( m_reader->BeginNode( TXT("Entry") ) == true )
				{
					/***** Initialize preset entry *****/
					String presetEntryGroup;
					String presetEntryVarId;
					String presetEntryValue;

					CName presetEntryGroupcname = groupName;

					// If no overrides for group, then apply user config group
					if( m_reader->Attribute( TXT("overrideGroup"), presetEntryGroup ) == true )
					{
						presetEntryGroupcname = CName( presetEntryGroup );
					}
					m_reader->Attribute( TXT("varId"), presetEntryVarId );
					m_reader->Attribute( TXT("value"), presetEntryValue );

					CName presetEntryVarIdcname = CName( presetEntryVarId );
					presetEntries.PushBack( SConfigPresetEntry( presetId, presetEntryGroupcname, presetEntryVarIdcname, presetEntryValue ) );

					m_reader->EndNode();
				}

				m_reader->EndNode();
			}

			m_reader->EndNode();

			return true;
		}

		return false;
	}

}
