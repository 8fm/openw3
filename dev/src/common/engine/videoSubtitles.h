/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/resource.h"

class CVideoSubtitles
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CVideoSubtitles( const String& depotPath );

public:
	Uint32 GetSize() const { return m_lines.Size(); }
	const String& operator[](Int32 index) const { return m_lines[index].m_text; }

public:
	Bool IsValid() const { return m_isValid; }

private:
	Bool LoadFromFile();

private:
	Bool ParseSubtitlesFromString( const String& fileData );

private:
	struct SVideoSubtitleLine
	{
		String m_text;
		Uint32 m_startTime;
		Uint32 m_endTime;

		SVideoSubtitleLine()
			: m_startTime( 0 )
			, m_endTime( 0 )
		{}
	};

private:
	Uint32							m_timeInterval;
	TDynArray< SVideoSubtitleLine > m_lines;
	String							m_depotPath;
	Bool							m_isValid;
};
