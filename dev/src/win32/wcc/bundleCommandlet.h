/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __BUNDLE_COMMANDLET_H__
#define __BUNDLE_COMMANDLET_H__

#include "../../common/core/commandlet.h"
#include "../../common/engine/materialCompilerDefines.h"
#include "../../common/core/bundledefinition.h"

class CBundleCommandlet : public ICommandlet
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CBundleCommandlet, ICommandlet );

public:

	CBundleCommandlet();
	virtual ~CBundleCommandlet();

	// Interface
protected:

	typedef Red::Core::BundleDefinition::CBundleDefinitionWriter CBundleDefinitionWriter;

	// Implement this function in your class and add the entries for everything
	// you want to be bundled to the bundle definition writer
	// Return false to indicate error and exit
	virtual Bool PopulateDefinition( CBundleDefinitionWriter& definitionWriter ) const = 0;

	// If you do not override this function and specify a directory
	// bundle files will be output to the root folder of the bundles directory
	virtual String BundleOutputSubdirectory() const { return String::EMPTY; }

protected:
	String GetCommandLineHelp() const;
	void PrintCommandLineHelpDetails() const;

	ECookingPlatform GetPlatform( const CommandletOptions& options ) const;

	enum EDefinitionWriteError
	{
		DWE_Success = 0,
		DWE_DefinitionNotSpecified,
		DWE_PathNotSpecified,
		DWE_OpenFailed,
		DWE_PopulateFailed,
		DWE_WriteFailed,
		DWE_BuilderFailed,
	};

	EDefinitionWriteError WriteDefinition( const CommandletOptions& options, Bool append );

private:
	CBundleDefinitionWriter* CreateDefinition( const String& definitionPath );
	CBundleDefinitionWriter* AppendDefinition( const String& definitionPath );

	Bool BuildBundles( const CommandletOptions& options, const String& definitionPath );

private:

	static const String c_optionDefinition;
	static const String c_optionBuildBundles;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBundleCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI();


#endif // __BUNDLE_COMMANDLET_H_
