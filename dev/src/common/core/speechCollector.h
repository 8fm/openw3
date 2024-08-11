/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CSpeechCollector
{
public:

	typedef TDynArray< Uint32 > TSpeechIDs;
	typedef TDynArray< String > TStringKeys;

	TSpeechIDs	speechIds;
	TStringKeys	stringKeys;

public:

	CSpeechCollector( ) { }

	void ReportStringID( Uint32 id )
	{
		speechIds.PushBackUnique( id );
	}

	void ReportStringKey( const String& string )
	{
		stringKeys.PushBackUnique( string );
	}
};