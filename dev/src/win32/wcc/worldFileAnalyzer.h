/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __GENERIC_ANALYZER_H__
#define __GENERIC_ANALYZER_H__

#include "../../common/core/analyzer.h"

class CWorldAnalyzer;
/// Analyzer for world - creates the world_xxx bundle and hierarchical world_x_x_x bundles (quad tree)
class CWorldFileAnalyzer : public IAnalyzer
{
	DECLARE_RTTI_SIMPLE_CLASS(CWorldFileAnalyzer)

public:
	CWorldFileAnalyzer();
	~CWorldFileAnalyzer();

	// interface
	virtual bool DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList );
	virtual const Char* GetName() const { return TXT("world"); }
	virtual const Char* GetDescription() const { return TXT("Analyzes (hierarchicly) the world files"); }

private:
	static const AnsiChar* BUNLDE_NAME_COMMON;
	static const Char* BUNLDE_NAME_STREAMING;
	static const Char* WORLD_FILE_EXTENSION;
	
	struct Settings
	{
		Settings();
		Bool Parse( const ICommandlet::CommandletOptions& options );

		String m_worldDepotPath;
	}
	m_settings;
	
	String		m_depotBasePath;

	// Simple extraction of files to bundles.
	void ExtractToBundles( CWorld* world, CDirectory* worldDir, CAnalyzerOutputList& outputList );

	// file extraction helper
	void ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList );
};

BEGIN_CLASS_RTTI(CWorldFileAnalyzer);
	PARENT_CLASS(IAnalyzer);
END_CLASS_RTTI();

#endif __GENERIC_ANALYZER_H__
