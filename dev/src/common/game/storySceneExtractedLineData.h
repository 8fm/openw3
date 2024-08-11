/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

struct SExtractedSceneLineData
{
	CName		m_voicetag;
	Uint32		m_stringIndex;
	StringAnsi	m_eventName;
	Int32			m_modeFlags;
	TagList		m_actorTags;
};