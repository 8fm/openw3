/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_FORWARD_DECLARATIONS_H__
#define __SCRIPT_STUDIO_SOLUTION_FORWARD_DECLARATIONS_H__

#include <memory>

enum ESolutionType
{
	Solution_Dummy = -1,
	Solution_Standard = 0,
	Solution_Mod,

	Solution_Max
};

class Solution;
class SolutionDir;
class SolutionFile;

typedef std::shared_ptr< SolutionFile >	SolutionFilePtr;
typedef std::weak_ptr< SolutionFile >	SolutionFileWeakPtr;

#endif // __SCRIPT_STUDIO_SOLUTION_FORWARD_DECLARATIONS_H__
