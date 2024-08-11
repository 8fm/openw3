#include "build.h"
#include "GameplayFXMedalion.h"
#include "../../common/engine/tickManager.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CGameplayFXMedalion );

RED_DEFINE_STATIC_NAME( isHighlightedByMedallion );

CGameplayFXMedalion::CGameplayFXMedalion()
	: m_beginRadius ( 0.05f )
	, m_endRadius ( 40.0f )
	, m_debugLoop ( false )
	, m_distPerSec ( 6.0f )
	, m_sustainTime ( 5.0f )
	, m_ringRadiusTolerance ( 0.5f )
	, m_highlightTag( CNAME( HighlightedByMedalionFX ) )
{
	m_entitiesCollector.Reserve( 32 );
	m_highlightList.Reserve( 32 );
	m_toRemoveList.Reserve( 16 );
}

void CGameplayFXMedalion::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Areas );

	m_currentRadius = m_beginRadius;
	m_endTime = 0.0f;
	m_isStarted = false;

	// Add entity to tick manager	
	world->GetTickManager()->AddEntity( this );	
}

void CGameplayFXMedalion::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Areas );

	// Remove entity from tick manager	
	world->GetTickManager()->RemoveEntity( this );

	// Fade out
	for ( Uint32 j = 0; j < m_highlightList.Size(); ++j )
	{
		CEntity* entity = m_highlightList[j].m_entity.Get();

		if ( entity )
		{
			entity->StopEffect( CNAME( medalion_detection_fx ) );
		}
	}
}


void CGameplayFXMedalion::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Base fragments
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Draw only if visible
	if ( flag == SHOW_Areas )
	{
		frame->AddDebugSphere( GetWorldPosition(), m_currentRadius, Matrix::IDENTITY, Color::RED );
	}
}

void CGameplayFXMedalion::OnTick( Float timeDelta )
{
	PC_SCOPE( CGameplayFXMedalion );

	TBaseClass::OnTick( timeDelta );

	if ( !GGame->IsActive() || GGame->IsPaused() )
	{
		return;
	}

	if ( m_endTime != 0.0f )
	{
		m_endTime += timeDelta;


		m_toRemoveList.ClearFast();

		// First, simulate highlighted items
		for ( Uint32 j = 0; j < m_highlightList.Size(); ++j )
		{
			m_highlightList[j].m_timeSinceStart += timeDelta;

			if ( m_highlightList[j].m_timeSinceStart > (m_sustainTime ) )
			{
				if ( !m_highlightList[j].m_effectStopped )
				{
					CEntity* entity = m_highlightList[j].m_entity.Get();
					m_highlightList[j].m_effectStopped = true;

					if ( entity )
					{
						entity->StopEffect( CNAME( medalion_detection_fx ) );
					}

					// remove
					m_toRemoveList.PushBack( m_highlightList[j] );
				}
			}
		}

		// Remove not needed
		for ( Uint32 j = 0; j < m_toRemoveList.Size(); ++j )
		{
			for ( TDynArray< SGameplayFXMedalionEntityHighlightInfo >::iterator it = m_highlightList.Begin(); it != m_highlightList.End(); ++it )
			{
				if ( it->m_entity == m_toRemoveList[j].m_entity )
				{
					m_highlightList.EraseFast( it );
					break;
				}
			}
		}

		if ( m_endTime > m_sustainTime )
		{
			if ( m_debugLoop )
			{
				m_endTime = 0.0f;
				m_currentRadius = m_beginRadius;
			}
			else
			{
				Destroy();
			}
		}

		return;
	}

	if ( !m_isStarted )
	{
		m_isStarted = true;

		PlayAllEffects();
	}

	m_currentRadius += timeDelta * m_distPerSec;
	SetScale( Vector( m_currentRadius, m_currentRadius, m_currentRadius ) );

	const Float radiusSquared = Max( m_currentRadius - m_ringRadiusTolerance, 0.0f ) * Max( m_currentRadius - m_ringRadiusTolerance, 0.0f );
	const Float radiusAndToleranceSquared = (m_currentRadius + m_ringRadiusTolerance) * (m_currentRadius + m_ringRadiusTolerance);

	m_toRemoveList.ClearFast();

	// First, simulate highlighted items
	for ( Uint32 j = 0; j < m_highlightList.Size(); ++j )
	{
		m_highlightList[j].m_timeSinceStart += timeDelta;
		
		if ( m_highlightList[j].m_timeSinceStart > m_sustainTime )
		{
			if ( !m_highlightList[j].m_effectStopped )
			{
				m_highlightList[j].m_effectStopped = true;

				CEntity* entity = m_highlightList[j].m_entity.Get();
				if ( entity )
				{
					entity->StopEffect( CNAME( medalion_detection_fx ) );
				}

				// remove
				m_toRemoveList.PushBack( m_highlightList[j] );
			}
		}
	}

	// Remove not needed
	for ( Uint32 j = 0; j < m_toRemoveList.Size(); ++j )
	{
		for ( TDynArray< SGameplayFXMedalionEntityHighlightInfo >::iterator it = m_highlightList.Begin(); it != m_highlightList.End(); ++it )
		{
			if ( it->m_entity == m_toRemoveList[j].m_entity )
			{
				m_highlightList.EraseFast( it );
				break;
			}
		}
	}


	// Second, find new items to highlight
	m_entitiesCollector.ClearFast();
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( m_highlightTag, m_entitiesCollector );

	CPlayer * player = GCommonGame->GetPlayer();
	if( player )
	{
		for ( Uint32 i = 0; i < m_entitiesCollector.Size(); ++i )
		{
			Float distance = GetWorldPosition().DistanceSquaredTo( m_entitiesCollector[ i ]->GetWorldPosition() );
			if ( (distance >= radiusSquared) && ( distance <= radiusAndToleranceSquared ) )
			{
				CEntity* entity = m_entitiesCollector[ i ];
				CGameplayEntity* gameplayEntity = Cast < CGameplayEntity >( entity );

				if ( gameplayEntity )
				{
					CProperty* isHighlightedByMedallion = gameplayEntity->GetClass()->FindProperty( CNAME( isHighlightedByMedallion ) );

					if ( isHighlightedByMedallion )
					{
						Bool val;
						isHighlightedByMedallion->Get( gameplayEntity, &val );

						if ( !val )
						{
							continue;
						}
					}
				}

				if ( entity->IsA< CNewNPC >() && ((Cast< CNewNPC >( entity ))->GetAttitude( player ) != AIA_Hostile) )
				{
					continue;
				}

				Bool found = false;
				for ( Uint32 j = 0; j < m_highlightList.Size(); ++j )
				{
					CEntity* oldEntity = m_highlightList[j].m_entity.Get();
					if ( entity && ( entity == oldEntity ) )
					{
						found = true;
						break;
					}
				}

				if ( !found )
				{
					SGameplayFXMedalionEntityHighlightInfo info;
					info.m_entity = entity;
					info.m_timeSinceStart = 0.0f;
					info.m_effectStopped = false;

					m_highlightList.PushBack( info );

					m_entitiesCollector[ i ]->PlayEffect( CNAME( medalion_detection_fx ) );
				}
			}
		}
	}

	// Check if should end

	if ( m_currentRadius > m_endRadius )
	{
		StopAllEffects();
		m_endTime = 0.001f;
		m_currentRadius = 0.0f;
		SetScale( Vector( m_currentRadius, m_currentRadius, m_currentRadius ) );
	}
}


