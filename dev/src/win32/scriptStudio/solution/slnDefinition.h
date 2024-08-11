/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_DEFINITION_H__
#define __SCRIPT_STUDIO_SOLUTION_DEFINITION_H__

class SolutionBase;

class SolutionDefinition
{
public:
	SolutionDefinition();
	~SolutionDefinition();

	void Save( SolutionBase* solution, const wchar_t* path );
	SolutionBase* Load( const wchar_t* path );
};

#endif // __SCRIPT_STUDIO_SOLUTION_DEFINITION_H__
