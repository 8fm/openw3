/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/commandlet.h"

/// Generalized commandlet for doing differential patches for different type of content
class CPatchCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CPatchCommandlet, ICommandlet, 0 );

public:
	CPatchCommandlet();
	~CPatchCommandlet();

	// interface
	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const { return TXT("Generalized commandlet for creating differential patches for specified type of content."); }
	virtual void PrintHelp() const;

private:
	struct Settings
	{
		// input
		String			m_baseBuildPath;
		String			m_currentBuildPath;
		Bool			m_isCurrentBuildAMod;

		// platform
		ECookingPlatform		m_platform;

		// output
		String			m_outputPath;

		// patch tool
		String			m_builderName;

		// name of the patch
		String			m_patchName;

		// debug dump of the differences
		String			m_dumpPath;

		// parse settings
		Bool Parse( const CommandletOptions& options );

		Settings();
	};

	Settings	m_settings;
};

BEGIN_CLASS_RTTI( CPatchCommandlet );
	PARENT_CLASS( ICommandlet );
END_CLASS_RTTI();