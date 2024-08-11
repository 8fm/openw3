/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_MOD_H__
#define __SCRIPT_STUDIO_SOLUTION_MOD_H__

#include "slnBase.h"
#include "dir.h"

//////////////////////////////////////////////////////////////////////////
// Base class for all solution types
class SolutionMod : public SolutionBase
{
public:
	SolutionMod( const wstring& name, const wstring& source, const wstring& local, const wstring& gameInstall );
	virtual ~SolutionMod() override final;

	// Mod only functions
	wstring GetName() const;
	wstring GetInstall() const;

	// Common solution functions
	virtual SolutionFilePtr FindFile( const wstring& searchPath ) override final;
	virtual void CheckFilesStatus() override final;

	virtual wstring GetPath( ESolutionType pathType ) const override final;
	virtual SolutionDir* GetRoot( ESolutionType pathType ) override final;

private:
	wstring m_name;
	wstring m_gameInstall;

	// Copy of original game scripts
	SolutionDir m_source;

	// Files entirely new created for this mod
	SolutionDir m_local;
};

#endif // __SCRIPT_STUDIO_SOLUTION_STANDARD_H__
