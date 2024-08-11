/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_STANDARD_H__
#define __SCRIPT_STUDIO_SOLUTION_STANDARD_H__

#include "slnBase.h"
#include "dir.h"

//////////////////////////////////////////////////////////////////////////
// Base class for all solution types
class SolutionStandard : public SolutionBase
{
public:
	SolutionStandard( const wstring& root );
	virtual ~SolutionStandard() override final;

	virtual SolutionFilePtr FindFile( const wstring& searchPath ) override final;
	virtual void CheckFilesStatus() override final;

	virtual wstring GetPath( ESolutionType pathType ) const override final;
	virtual SolutionDir* GetRoot( ESolutionType pathType ) override final;

private:
	SolutionDir m_root;
};

#endif // __SCRIPT_STUDIO_SOLUTION_STANDARD_H__
