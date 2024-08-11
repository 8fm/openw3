/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "slnMod.h"

SolutionMod::SolutionMod( const wstring& name, const wstring& source, const wstring& local, const wstring& gameInstall )
:	SolutionBase( Solution_Mod )
,	m_name( name )
,	m_gameInstall( gameInstall )
,	m_source( nullptr, L"", source, L"Source" )
,	m_local( nullptr, L"", local, L"Local" )
{
	CheckFilesStatus();
}

SolutionMod::~SolutionMod()
{

}

wstring SolutionMod::GetName() const
{
	return m_name;
}

wstring SolutionMod::GetInstall() const
{
	return m_gameInstall;
}

SolutionFilePtr SolutionMod::FindFile( const wstring& searchPath )
{
	SolutionFilePtr file = m_local.FindFileRecurse( searchPath );

	if( !file )
	{
		file = m_source.FindFileRecurse( searchPath );
	}

	return file;
}

void SolutionMod::CheckFilesStatus()
{
	m_source.RefreshFilesStatus();
	m_local.RefreshFilesStatus();
}

wstring SolutionMod::GetPath( ESolutionType pathType ) const
{
	if( pathType == Solution_Standard )
	{
		return m_source.GetAbsolutePath();
	}
	else if( pathType == Solution_Mod )
	{
		return m_local.GetAbsolutePath();
	}

	return L"";
}

SolutionDir* SolutionMod::GetRoot( ESolutionType pathType )
{
	if( pathType == Solution_Standard )
	{
		return &m_source;
	}
	else if( pathType == Solution_Mod )
	{
		return &m_local;
	}

	return nullptr;
}
