/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

struct CWWiseStreamedAudioEntry
{
	String id;
	String name;
	String audioSourceFile;
	String generatedAudioFile;
	String wWiseObjectPath;
};

class CWWiseDescriptionParser
{
public:
	static Bool GetStreamedAudioEntries( const String& content, TDynArray<CWWiseStreamedAudioEntry>& output );

};
