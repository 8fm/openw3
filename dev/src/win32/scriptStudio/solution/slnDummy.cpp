/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "slnDummy.h"

SolutionDummy::SolutionDummy()
:	SolutionBase( Solution_Dummy )
{

}

SolutionDummy::~SolutionDummy()
{

}

SolutionFilePtr SolutionDummy::FindFile( const wstring& )
{
	return SolutionFilePtr();
}

void SolutionDummy::CheckFilesStatus()
{

}

wstring SolutionDummy::GetPath( ESolutionType ) const
{
	return L"";
}

SolutionDir* SolutionDummy::GetRoot( ESolutionType pathType )
{
	return nullptr;
}
