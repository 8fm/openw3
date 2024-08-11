/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_CONTAINER_H__
#define __SCRIPT_STUDIO_SOLUTION_CONTAINER_H__

#include "slnBase.h"

class Solution
{
public:
	Solution();
	Solution( const wstring& root );
	Solution( const wstring& name, const wstring& source, const wstring& local, const wstring& gameInstall );

	void OpenDummy();
	void OpenStandard( const wstring& root );
	void OpenMod( const wstring& name, const wstring& source, const wstring& local, const wstring& gameInstall );

	~Solution();
	void Clear();

	ESolutionType FindRootContainingFile( SolutionFilePtr file ) const;

	void Save( const wstring& path );
	bool Load( const wstring& path );

	RED_INLINE ESolutionType GetType() const { return m_solution->GetType(); }
	RED_INLINE SolutionFilePtr FindFile( const wstring& searchPath ) { return m_solution->FindFile( searchPath ); }
	RED_INLINE void CheckFilesStatus() { m_solution->CheckFilesStatus(); }
	RED_INLINE wstring GetPath( ESolutionType pathType = Solution_Standard ) const { return m_solution->GetPath( pathType ); }
	RED_INLINE SolutionDir* GetRoot( ESolutionType pathType = Solution_Standard ) { return m_solution->GetRoot( pathType ); }

	// Mod only functions
	wstring GetName() const;
	wstring GetInstall() const;

private:
	SolutionBase* m_solution;
};

#endif // __SCRIPT_STUDIO_SOLUTION_CONTAINER_H__
