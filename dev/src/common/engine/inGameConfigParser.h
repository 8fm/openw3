/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inGameConfigInterface.h"

class CXMLReader;

namespace BoolExpression
{
	class IExp;
}

namespace InGameConfig
{
	class CCommonParserBase
	{
	public:
		CCommonParserBase( CXMLReader* reader );

		Bool ParseName( CName& name );
		Bool ParseDisplayName( String& name );
		Bool ParseTags( THashSet<CName>& tags );

	protected:
		CXMLReader* m_reader;
	};

	class CStandardVariableParser : public CCommonParserBase
	{
	public:
		CStandardVariableParser( CXMLReader* reader );

		Bool ParseDisplayType( String& displayType );
		Bool ParseOverrideGroupName( CName& overrideGroupName );
		Bool ParseListingFunction( CName& listingFunction );

		// Deprecated
		Bool ParseOptions(TDynArray< SConfigPresetOption >& options, TDynArray< SConfigPresetEntry >& optionEntries, CName& listingFunction, const CName& groupName);

	};

	class CStandardGroupParser : public CCommonParserBase
	{
	public:
		CStandardGroupParser( CXMLReader* reader );

		Bool ParsePresets(TDynArray< SConfigPresetOption >& presets, TDynArray< SConfigPresetEntry >& presetEntries, const CName& groupName);
	};
}
