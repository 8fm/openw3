/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "videoSubtitles.h"
#include "../core/depot.h"
#include "../core/diskFile.h"
#include "../core/scopedPtr.h"

namespace cs602_ceremony_ears_tr
{
#include "cs602_ceremony_ears_tr.subs.h"
}
namespace cs602_ceremony_tr
{
#include "cs602_ceremony_tr.subs.h"
}
namespace cs702_vision_1_tr
{
#include "cs702_vision_1_tr.subs.h"
}
namespace cs702_vision_2_tr
{
#include "cs702_vision_2_tr.subs.h"
}
namespace cs702_vision_3_tr
{
#include "cs702_vision_3_tr.subs.h"
}
namespace cs704_sister_lives_teleport_tr
{
#include "cs704_sister_lives_teleport_tr.subs.h"
}

namespace
{
	struct BlobMapEntry
	{
		const Char* m_fileName;
		const Uint8* m_blob;
		Uint32 m_blobSize;
	};

	#define BLOBDEF(x) { MACRO_TXT(RED_STRINGIFY(x)) MACRO_TXT(".subs"), x::Data, sizeof(x::Data) }
	static BlobMapEntry BlobMap[] = {
		BLOBDEF( cs602_ceremony_ears_tr ),
		BLOBDEF( cs602_ceremony_tr ),
		BLOBDEF( cs702_vision_1_tr ),
		BLOBDEF( cs702_vision_2_tr ),
		BLOBDEF( cs702_vision_3_tr ),
		BLOBDEF( cs704_sister_lives_teleport_tr ),
	};
	#undef BLOBDEF

	static Bool FindBlob( const String& depotPath, BlobMapEntry& outEntry )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32( BlobMap ); ++i )
		{
			if ( depotPath.EndsWith( BlobMap[i].m_fileName ) )
			{
				outEntry = BlobMap[i];
				return true;
			}
		}
		return false;
	}
}

CVideoSubtitles::CVideoSubtitles( const String& depotPath )
	: m_timeInterval( 0 )
	, m_depotPath( depotPath )
{
	m_isValid = LoadFromFile();
}

Bool CVideoSubtitles::LoadFromFile()
{
	Red::TScopedPtr< IFile > reader;
	{
		CDiskFile* diskFile = GDepot->FindFile( m_depotPath );
		if ( diskFile )
		{
			reader.Reset( diskFile->CreateReader() );
		}
		else
		{
			BlobMapEntry blob;
			if ( FindBlob( m_depotPath, blob ) )
			{
				WARN_ENGINE( TXT("CVideoSubtitles had to load binarized subs for '%ls"), m_depotPath.AsChar() );
				reader.Reset( new CMemoryFileReaderExternalBuffer( blob.m_blob, blob.m_blobSize ) );
			}
		}
	}

	if ( !reader )
	{
		ERR_ENGINE(TXT("CVideoSubtitles failed to load '%ls'"), m_depotPath.AsChar() );
		return false;
	}

	String fileData;
	if ( !GFileManager->LoadFileToString( reader.Get(), fileData ) )
	{
		ERR_ENGINE(TXT("CVideoSubtitles failed to load '%ls'"), m_depotPath.AsChar());
		return false;
	}

	return ParseSubtitlesFromString( fileData );
}

// Note: they must be UCS2-LE to be read in correctly by our engine!
Bool CVideoSubtitles::ParseSubtitlesFromString( const String& fileData )
{
	TDynArray< String > lines = fileData.Split( TXT("\r\n") );
	if ( lines.Empty() )
	{
		ERR_ENGINE(TXT("Subtitles '%ls' has no lines for parsing"), m_depotPath.AsChar() );
		return false;
	}

	Uint32 timeInterval = 0;
	if ( !::FromString( lines[0], m_timeInterval ) )
	{
		ERR_ENGINE(TXT("Subtitles '%ls' has no time interval for parsing"), m_depotPath.AsChar() );
		return false;
	}

	lines.RemoveAt( 0 );

	for ( const String& textLine : lines )
	{
		const Char* ch = textLine.AsChar();
		SVideoSubtitleLine subsLine;

		if ( !GParseInteger( ch, subsLine.m_startTime ) || !GParseKeyword( ch, TXT(",") ) )
		{
			ERR_ENGINE(TXT("Subtitles '%ls' has no start time for parsing"), m_depotPath.AsChar() );
			return false;
		}
		if ( !GParseInteger( ch, subsLine.m_endTime ) || !GParseKeyword( ch, TXT(",") ) )
		{
			ERR_ENGINE(TXT("Subtitles '%ls' has no end time for parsing"), m_depotPath.AsChar() );
			return false;
		}

		subsLine.m_text = ch;
		subsLine.m_text.Trim();

		if ( subsLine.m_text.Empty() )
		{
			ERR_ENGINE(TXT("Subtitles '%ls' has no text for parsing"), m_depotPath.AsChar() );
			return false;
		}

		m_lines.PushBack( subsLine );
	}

	return true;
}
