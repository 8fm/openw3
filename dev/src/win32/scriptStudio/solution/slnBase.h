/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_BASE_H__
#define __SCRIPT_STUDIO_SOLUTION_BASE_H__

#include "slnDeclarations.h"

//////////////////////////////////////////////////////////////////////////
// Base class for all solution types
class SolutionBase
{
public:
	SolutionBase( ESolutionType type );
	virtual ~SolutionBase();

	RED_INLINE ESolutionType GetType() const { return m_type; }

	//! Find file by path
	virtual SolutionFilePtr FindFile( const wstring& searchPath ) = 0;

	//! Update files source control status
	virtual void CheckFilesStatus() = 0;

	virtual wstring GetPath( ESolutionType pathType ) const = 0;
	virtual SolutionDir* GetRoot( ESolutionType pathType ) = 0;

private:
	ESolutionType m_type;
};


#endif // __SCRIPT_STUDIO_SOLUTION_BASE_H__
