#include "build.h"
#include "characterResource.h"
#include "character.h"

IMPLEMENT_ENGINE_CLASS( CCharacterResource );

CCharacterResource::CCharacterResource()
{

}
CCharacterResource::~CCharacterResource()
{

}

CCharacter* CCharacterResource::FindFirstCharacterWithName( CName name )
{
	for ( auto it = m_charactersData.Begin(); it != m_charactersData.End(); ++it )
	{
		if ( (*it)->GetName() == name )
		{
			return *it;
		}
	}
	return nullptr;
}

CCharacter* CCharacterResource::FindCharacterByGUID( CGUID guid )
{
	for ( auto it = m_charactersData.Begin(); it != m_charactersData.End(); ++it )
	{
		if ( (*it)->GetGUID() == guid )
		{
			return *it;
		}
	}
	return nullptr;
}

