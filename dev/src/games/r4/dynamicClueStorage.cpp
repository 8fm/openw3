/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dynamicClueStorage.h"
#include "../../common/core/gatheredResource.h"

//////////////////////////////////////////////////////////////////////////

CGatheredResource resDynamicClueRanges( TXT("gameplay\\globals\\dynamic_clue_ranges.csv"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////

SDynamicClue::SDynamicClue()
	: m_template( nullptr )
	, m_layer( nullptr )
	, m_entity( nullptr )
{
}

SDynamicClue::SDynamicClue( const THandle<CEntityTemplate>& templ, CLayer* layer, const Vector& position, const EulerAngles& rotation )
	: m_template( templ )
	, m_layer( layer )
	, m_position( position )
	, m_rotation( rotation )
	, m_entity( nullptr )
{
}

Bool SDynamicClue::ShouldBeSpawned() const
{
	if ( m_entity.Get() != nullptr || !m_template.IsValid() || m_layer == nullptr )
	{
		return false;
	}
	CPlayer* player = GCommonGame->GetPlayer();
	if ( player == nullptr )
	{
		return false;
	}
	Vector diff = m_position - player->GetWorldPosition();
	if ( Abs( diff.X ) > CDynamicClueStorage::SPAWN_MIN_RANGE && Abs( diff.X ) < CDynamicClueStorage::SPAWN_MAX_RANGE &&
		 Abs( diff.Y ) > CDynamicClueStorage::SPAWN_MIN_RANGE && Abs( diff.Y ) < CDynamicClueStorage::SPAWN_MAX_RANGE )
	{
		Float distSqr = diff.X * diff.X + diff.Y * diff.Y;
		return ( distSqr > CDynamicClueStorage::SPAWN_MIN_RANGE_SQR && distSqr < CDynamicClueStorage::SPAWN_MAX_RANGE_SQR );
	}
	return false;
}

Bool SDynamicClue::ShouldBeDestroyed() const
{
	if ( m_entity.Get() == nullptr )
	{
		return false;
	}
	CPlayer* player = GCommonGame->GetPlayer();
	if ( player == nullptr )
	{
		return false;
	}
	Vector diff = m_position - player->GetWorldPosition();
	if ( Abs( diff.X ) > CDynamicClueStorage::DESTROY_RANGE || Abs( diff.Y ) > CDynamicClueStorage::DESTROY_RANGE_SQR )
	{
		return true;
	}
	return ( diff.X * diff.X + diff.Y * diff.Y > CDynamicClueStorage::DESTROY_RANGE_SQR );
}

void SDynamicClue::Spawn()
{
	if ( m_template.IsValid() && m_layer != nullptr )
	{
		EntitySpawnInfo spawnInfo;
		spawnInfo.m_template = m_template;
		spawnInfo.m_spawnPosition = m_position;
		spawnInfo.m_spawnRotation = m_rotation;
		m_entity = m_layer->CreateEntitySync( spawnInfo );
	}
}

void SDynamicClue::Destroy()
{
	if ( m_entity.Get() != nullptr )
	{
		m_entity.Get()->Destroy();
		m_entity = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////

Float CDynamicClueStorage::UPDATE_RANGE = 1.0f;
Float CDynamicClueStorage::SPAWN_MIN_RANGE = 1.0f;
Float CDynamicClueStorage::SPAWN_MIN_RANGE_SQR = CDynamicClueStorage::SPAWN_MIN_RANGE * CDynamicClueStorage::SPAWN_MIN_RANGE;
Float CDynamicClueStorage::SPAWN_MAX_RANGE = 30.0f;
Float CDynamicClueStorage::SPAWN_MAX_RANGE_SQR = CDynamicClueStorage::SPAWN_MAX_RANGE * CDynamicClueStorage::SPAWN_MAX_RANGE;
Float CDynamicClueStorage::DESTROY_RANGE = 5.0f;
Float CDynamicClueStorage::DESTROY_RANGE_SQR = CDynamicClueStorage::DESTROY_RANGE * CDynamicClueStorage::DESTROY_RANGE;

CDynamicClueStorage::CDynamicClueStorage()
{
}

CDynamicClueStorage::~CDynamicClueStorage()
{
	Clear();
}

void CDynamicClueStorage::OnGameStart()
{
	LoadConfig();
	m_distanceChecker.Init( UPDATE_RANGE );
}

void CDynamicClueStorage::OnGameEnd()
{
	Clear();
}

void CDynamicClueStorage::Clear()
{
	m_cluesToDestroy.PushBack( m_clues );
	m_clues.Clear();
	DestroyClues( true ); // force destroy entities and clear m_cluesToDestroy
}

void CDynamicClueStorage::LoadConfig()
{
	C2dArray* rangesArray = resDynamicClueRanges.LoadAndGet< C2dArray >();
	RED_ASSERT( rangesArray != nullptr, TXT( "Unable to open '%ls'" ), resDynamicClueRanges.GetPath().ToString().AsChar() );
	if ( rangesArray == nullptr )
	{
		return;
	}

	Uint32 size = static_cast< Uint32 >( rangesArray->GetNumberOfRows() );
	for ( Uint32 i = 0; i < size; i++ )
	{
		String rangeName = rangesArray->GetValue( TXT( "Name" ), i );
		Float range = 0.0f;
		if ( C2dArray::ConvertValue( rangesArray->GetValue( TXT( "Value" ), i ), range ) )
		{
			SetRange( rangeName, range );
		}
	}
}

void CDynamicClueStorage::SetRange( const String& rangeName, Float value )
{
	if ( rangeName == TXT( "update" ) )
	{
		UPDATE_RANGE = value;
	}
	else if ( rangeName == TXT( "spawn_min" ) )
	{
		SPAWN_MIN_RANGE = value;
		SPAWN_MIN_RANGE_SQR = value * value;
	}
	else if ( rangeName == TXT( "spawn_max" ) )
	{
		SPAWN_MAX_RANGE = value;
		SPAWN_MAX_RANGE_SQR = value * value;
	}
	else if ( rangeName == TXT( "destroy" ) )
	{
		DESTROY_RANGE = value;
		DESTROY_RANGE_SQR = value * value;
	}
	else
	{
		RED_ASSERT( false, TXT( "Unknown dynamic clue range name: '%ls'" ), rangeName.AsChar() );
	}
}

SDynamicClue* CDynamicClueStorage::Add( const THandle< CEntityTemplate >& templ, CLayer* layer, const Vector& position, const EulerAngles& rotation )
{
	SDynamicClue* clue = new SDynamicClue( templ, layer, position, rotation );
	if ( clue->ShouldBeSpawned() )
	{
		clue->Spawn();
	}
	m_clues.PushBack( clue );
	return clue;
}

Bool CDynamicClueStorage::Remove( SDynamicClue* clue )
{
	ASSERT( clue != nullptr );
	if ( m_clues.Remove( clue ) )
	{
		if ( clue->m_entity.Get() != nullptr )
		{
			if ( clue->ShouldBeDestroyed() )
			{
				clue->Destroy();
				delete clue;
			}
			else
			{
				m_cluesToDestroy.PushBack( clue );
			}
		}
		else
		{
			delete clue;
		}
		return true;
	}
	return false;
}

void CDynamicClueStorage::Update()
{
	if ( !m_distanceChecker.ShouldUpdate() )
	{
		return;
	}
	DestroyClues( false );
	TDynArray< SDynamicClue * >::iterator it = m_clues.Begin();
	TDynArray< SDynamicClue * >::iterator itEnd = m_clues.End();
	for ( ; it != itEnd; ++it )
	{
		SDynamicClue* clue = *it;
		if ( clue->ShouldBeSpawned() )
		{
			clue->Spawn();
		}
	}
}

void CDynamicClueStorage::DestroyClues( Bool force /* = false */ )
{
	Uint32 index = 0;
	while ( index < m_cluesToDestroy.Size() )
	{
		SDynamicClue* clue = m_cluesToDestroy[ index ];
		if ( force || clue->m_entity.Get() == nullptr || clue->ShouldBeDestroyed() )
		{
			clue->Destroy();
			delete clue;
			m_cluesToDestroy.RemoveAt( index );
		}
		else
		{
			index++;
		}
	}
}