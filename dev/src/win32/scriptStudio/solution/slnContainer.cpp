/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "slnContainer.h"

#include "slnDummy.h"
#include "slnStandard.h"
#include "slnMod.h"
#include "slnDefinition.h"
#include "file.h"

Solution::Solution()
:	m_solution( nullptr )
{
	OpenDummy();
}

Solution::Solution( const wstring& root )
:	m_solution( nullptr )
{
	OpenStandard( root );
}

Solution::Solution( const wstring& name, const wstring& source, const wstring& local, const wstring& gameInstall )
:	m_solution( nullptr )
{
	OpenMod( name, source, local, gameInstall );
}

void Solution::OpenDummy()
{
	Clear();
	m_solution = new SolutionDummy();
}

void Solution::OpenStandard( const wstring& root )
{
	Clear();
	m_solution = new SolutionStandard( root );
}

void Solution::OpenMod( const wstring& name, const wstring& source, const wstring& local, const wstring& gameInstall )
{
	Clear();
	m_solution = new SolutionMod( name, source, local, gameInstall );
}

Solution::~Solution()
{
	Clear();
}

void Solution::Clear()
{
	delete m_solution;
	m_solution = nullptr;
}

ESolutionType Solution::FindRootContainingFile( SolutionFilePtr file ) const
{
	for( int i = 0; i < Solution_Max; ++i )
	{
		ESolutionType type = static_cast< ESolutionType >( i );
		wstring solutionPath = m_solution->GetPath( type );

		if( file->m_absolutePath.compare( 0, solutionPath.length(), solutionPath ) == 0 )
		{
			return type;
		}
	}

	return Solution_Dummy;
}

void Solution::Save( const wstring& path )
{
	SolutionDefinition definition;

	definition.Save( m_solution, path.c_str() );
}

bool Solution::Load( const wstring& path )
{
	SolutionDefinition definition;

	SolutionBase* newSolution = definition.Load( path.c_str() );

	if( newSolution )
	{
		Clear();

		m_solution = newSolution;
		return true;
	}

	return false;
}

wstring Solution::GetName() const
{
	if( m_solution->GetType() == Solution_Mod )
	{
		SolutionMod* mod = static_cast< SolutionMod* >( m_solution );
		return mod->GetName();
	}
	else if( m_solution->GetType() == Solution_Dummy )
	{
		return L"No open solution";
	}

	return m_solution->GetPath( Solution_Standard );
}

wstring Solution::GetInstall() const
{
	if( m_solution->GetType() == Solution_Mod )
	{
		SolutionMod* mod = static_cast< SolutionMod* >( m_solution );
		return mod->GetInstall();
	}

	return L"";
}
