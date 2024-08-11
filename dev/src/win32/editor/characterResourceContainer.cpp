#include "build.h"

#include "characterResourceContainer.h"
#include "../../common/game/characterResource.h"

IMPLEMENT_ENGINE_CLASS( CEdCharacterResourceContainer );

CEdCharacterResourceContainer::CEdCharacterResourceContainer(void)
{
}


CEdCharacterResourceContainer::~CEdCharacterResourceContainer(void)
{
}

CCharacterResource* CEdCharacterResourceContainer::CreateResouce()
{
	CCharacterResource* characterResource = ::CreateObject< CCharacterResource >( this );
	m_resources.PushBack( characterResource );
	return characterResource;
}

void CEdCharacterResourceContainer::AddResource( CCharacterResource* characterResource )
{
	characterResource->SetParent( this );
	m_resources.PushBack( characterResource );
}

void CEdCharacterResourceContainer::RemoveResource( CCharacterResource* characterResource )
{
	characterResource->SetParent( nullptr );
	RED_VERIFY( m_resources.Remove( characterResource ) );
}

CCharacter* CEdCharacterResourceContainer::FindFirstCharacterWithName( CName name )
{
	for ( auto resIt = m_resources.Begin(); resIt != m_resources.End(); ++resIt )
	{
		CCharacterResource* res = (*resIt).Get();
		CCharacter* character = res->FindFirstCharacterWithName( name );
		if ( character )
		{
			return character;
		}
	}
	return nullptr;
}

CCharacter* CEdCharacterResourceContainer::FindCharacterByGUID( CGUID guid )
{
	for ( auto resIt = m_resources.Begin(); resIt != m_resources.End(); ++resIt )
	{
		CCharacterResource* res = (*resIt).Get();
		CCharacter* character = res->FindCharacterByGUID( guid );
		if ( character )
		{
			return character;
		}
	}
	return nullptr;
}
