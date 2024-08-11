/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_DUMMY_H__
#define __SCRIPT_STUDIO_SOLUTION_DUMMY_H__

#include "slnBase.h"

//////////////////////////////////////////////////////////////////////////
// Dummy solution used when no solution is currently loaded
class SolutionDummy : public SolutionBase
{
public:
	SolutionDummy();
	virtual ~SolutionDummy() override final;

	virtual SolutionFilePtr FindFile( const wstring& searchPath ) override final;
	virtual void CheckFilesStatus() override final;
	virtual wstring GetPath( ESolutionType pathType = Solution_Standard ) const override final;
	virtual SolutionDir* GetRoot( ESolutionType pathType = Solution_Standard ) override final;
};

#endif // __SCRIPT_STUDIO_SOLUTION_BASE_H__
