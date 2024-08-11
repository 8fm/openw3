/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

struct SOptionsXboxOne
{
	String				m_packageType;
	String				m_tempDir;
	String				m_inDir;
	String				m_outDir;
	String				m_langDir;
	String				m_exeName;
	String				m_releaseName;
	String				m_defaultSpeech;
	TDynArray< String > m_textLanguages;
	TDynArray< String > m_speechLanguages;
	Uint32				m_launchContentNumber;		//!< Launch chunk. E.g., if "bin, content0, content1, content3" then 3
	Bool				m_createPkg:1;
	Bool				m_skipCRC:1;

	// custom content name aka playgo chunk
	String				m_manifestContentName;

	SOptionsXboxOne()
		: m_launchContentNumber(0xFFFFFFFF) // everything in initial chunk by default
		, m_createPkg( false )
		, m_skipCRC( false )
	{}
};

Bool PackageMainXboxOne( const SOptionsXboxOne& options );