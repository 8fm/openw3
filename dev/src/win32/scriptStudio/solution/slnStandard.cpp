/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "slnStandard.h"

SolutionStandard::SolutionStandard( const wstring& root )
:	SolutionBase( Solution_Standard )
,	m_root( nullptr, L"", root, L"Solution" )
{
	CheckFilesStatus();
}

SolutionStandard::~SolutionStandard()
{

}

SolutionFilePtr SolutionStandard::FindFile( const wstring& searchPath )
{
	return m_root.FindFileRecurse( searchPath );
}

void SolutionStandard::CheckFilesStatus()
{
	m_root.RefreshFilesStatus();
}

wstring SolutionStandard::GetPath( ESolutionType pathType ) const
{
	if( pathType == Solution_Standard )
	{
		return m_root.GetAbsolutePath();
	}
	else
	{
		return L"";
	}
}

SolutionDir* SolutionStandard::GetRoot( ESolutionType pathType )
{
	if( pathType == Solution_Standard )
	{
		return &m_root;
	}
	else
	{
		return nullptr;
	}
}
