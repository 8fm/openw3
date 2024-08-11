/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

struct SOptionsPS4
{
	String				m_packageType;
	String				m_tempDir;
	String				m_inDir;
	String				m_outDir;
	String				m_langDir;
	String				m_prefetchFile;
	String				m_contentID;
	String				m_passcode;
	String				m_elfName;
	String				m_defaultSpeech;

	// custom content name aka playgo chunk
	String				m_manifestContentName;

	// DLC options, blech needs some refactoring mixing everything
	String				m_dlcEntitlementKey;

	// Patch options, double blech
	String				m_appPkgPath;
	String				m_latestPatchPath;
	Bool				m_isDayOne;

	// Patch/content options
	TDynArray< String > m_speechLanguages;
	TDynArray< String > m_textLanguages;
	Bool				m_createPkg:1;
	Bool				m_createIso:1;
	Bool				m_createSubmissionMaterials:1;
	Bool				m_moveOuter:1;
	Bool				m_skipDigest:1;
	Bool				m_skipCRC:1;
	Uint32				m_launchContentNumber; // e.g., 0 for content0. Not PlayGo chunk directly.

	SOptionsPS4()
		: m_isDayOne( false )
		, m_createPkg( false )
		, m_createIso( false )
		, m_createSubmissionMaterials( false )
		, m_moveOuter( false )
		, m_skipDigest( false )
		, m_skipCRC( false )
		, m_launchContentNumber( 0xFFFFFFFF )
	{}
};

Bool PackageMainPS4( const SOptionsPS4& options );
