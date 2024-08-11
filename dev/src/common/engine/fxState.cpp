#include "build.h"
#include "fxSystem.h"
#include "fxTrackItemPause.h"
#include "entity.h"
#include "../core/events.h"
#include "entityTemplate.h"
#include "layer.h"
#include "dynamicLayer.h"
#include "baseEngine.h"
#include "tickManager.h"

CFXState::CFXState( const CFXDefinition* fxDefinition, Float showDistanceSqr, CName tag, CEntity* entity,
		Float startTime, Bool isPreview, const CName& boneName /* = CName::NONE */, const CNode* targetNode /* = NULL */, const CName& targetBone /* = CName::NONE */, Bool forAnim /* =false */ )
	: m_length( fxDefinition ? fxDefinition->GetLength() : 2.0f )
	, m_currentTime( startTime )
	, m_isPaused( false )
	, m_isAlive( true )
	, m_isStopping( false )
	, m_fxDefinition( fxDefinition )
	, m_entity( entity )
	, m_isInitialized( false )
	, m_isPreviewEffect( isPreview )
	, m_tag( tag )
	, m_boneName( boneName )
	, m_target( targetNode )
	, m_targetBone( targetBone ) 
	, m_stoppingNotified( false )
	, m_forAnim( forAnim )
	, m_tickEffectStartTime( 0.0f )
	, m_showDistanceSquared( showDistanceSqr )
{
	ASSERT( entity->IsAttached() );

	if( fxDefinition && fxDefinition->IsRandomStart() )
	{
		// Draw start time
		Float effectLength = fxDefinition->GetEndTime() - fxDefinition->GetStartTime();
		Float drawRange = 0.2f * effectLength;

		m_currentTime = Max( 0.0f, fxDefinition->GetStartTime() + GEngine->GetRandomNumberGenerator().Get< Float >( drawRange ) );
	}
}

CFXState::~CFXState()
{
}

void CFXState::Stop()
{
	// Mark as stopping
	m_isStopping = true;
}

void CFXState::Pause( Bool pause )
{
	// Pause effect
	m_isPaused = pause;
}

void CFXState::Cleanup()
{
	if ( m_isAlive )
	{
		// Mark as killed
		m_isAlive = false;

		// Inform editor listener
#ifndef NO_EDITOR_EVENT_SYSTEM
		if ( GIsEditor )
		{
			SEditorEffectMessage message;
			message.m_effectTag = m_tag;
			message.m_entity = m_entity;
			EDITOR_DISPATCH_EVENT( CNAME( EffectDestroyed ), CreateEventData( message ) );
		}
#endif
	}

	// Kill all tracks
	if ( !m_activeItems.Empty() )
	{
		// Delete play data
		for ( Uint32 i=0; i<m_activeItems.Size(); i++ )
		{
			IFXTrackItemPlayData* data = m_activeItems[i];
			delete data;
		}

		// Clear array
		m_activeItems.Clear();
	}
}

void CFXState::Destroy()
{
	Cleanup();

	// Suicide pointer pattern, this must be the last function called on the fxState EVER!
	// Listener (this entity), calls OnActiveEffectRemoved from inside of fxState when the effect is being destroyed. 
	// Thus fxState destroys itself on its own. This must be the last operation called on the effect instance, otherwise you'll get crash ;]
	GetEntity()->OnActiveEffectRemoved( this );
	delete this;
}

void CFXState::ClearLoopedPlayData()
{
	CFXDefinition* fxDefinition = m_fxDefinition.Get();
	if( fxDefinition && fxDefinition->IsLooped() )
	{
		// Delete play data
		for ( Uint32 i=0; i<m_activeItems.Size(); i++ )
		{
			IFXTrackItemPlayData* data = m_activeItems[i];
			delete data;
		}

		// Clear array
		m_activeItems.ClearFast();

		m_currentTime = 0.0f;
		m_isInitialized = false;
	}
}

void CFXState::CollectActivePlayDataToKill( Float timeStart, Float timeEnd, TDynArray< IFXTrackItemPlayData* >& dataList )
{
	// Skip empty time ranges
	if ( timeEnd <= timeStart )
	{
		return;
	}

	// Process list
	for ( Uint32 i=0; i<m_activeItems.Size(); i++ )
	{
		IFXTrackItemPlayData* data = m_activeItems[i];
		if ( data )
		{
			const CFXTrackItem* item = data->GetTrackItem();

			// No node or track data, remove
			if ( !data->GetNode() || !item )
			{
				dataList.PushBackUnique( data );
				continue;
			}

			// Check time range
			if ( item )
			{
				// Should end within given time range
				const Float itemEndTime = item->GetTimeEnd();
				if ( itemEndTime >= timeStart && itemEndTime <= timeEnd )
				{
					ASSERT( !dataList.Exist( data ) );
					dataList.PushBackUnique( data );
				}
			}
		}
	}
}

void CFXState::Tick( Float timeDelta )
{
	// Do not tick dead effects
	if ( !m_isAlive )
	{
		return;
	}

	CFXDefinition* fxDefinition = m_fxDefinition.Get();
	if ( !fxDefinition )
	{
		if ( m_isStopping )
		{
			Destroy();
			return;
		}

		m_tickEffectStartTime += timeDelta;

		if ( m_tickEffectStartTime > 0.1f )
		{
			m_tickEffectStartTime = 0.0f; 

			CEntity* entity = m_entity;
			if ( entity )
			{
				CEntityTemplate* templ = entity->GetEntityTemplate();
				if ( templ )
				{
					CFXDefinition* def = NULL;

					if ( m_forAnim )
					{
						def = templ->FindEffectForAnimation( m_tag );
					}
					else
					{
						def = templ->FindEffect( m_tag );
					}

					if ( !def )
					{
						return;
					}

					FillFxDefinition( def );
					fxDefinition = def;
				}
			}
		}
		else
		{
			return;
		}
	}

	// Do not update if paused
	if ( !m_isPaused )
	{
		// Advance time
		Float prevTime = m_currentTime;
		m_currentTime += timeDelta;

		// First tick, initialize all tracks
		if ( !m_isInitialized )
		{
			prevTime = -FLT_MAX;
			m_isInitialized = true;
		}

Restart:

		Bool notifyStop = false;

		// Collect track items to activate
		m_playDataToRemove.ClearFast();
		m_trackItemsToActivate.ClearFast();

		if ( fxDefinition->IsLooped() )
		{
			// Crossed the loop border, restart
			const Float timePastLoopEnd = m_currentTime - fxDefinition->GetLoopEnd();
			if ( timePastLoopEnd > 0.0f )
			{
				if( ! m_isStopping )
				{
					const Float loopLength = ( fxDefinition->GetLoopEnd() - fxDefinition->GetLoopStart() );
					ASSERT( loopLength >= 0.0f );

					// Wrap time
					if ( loopLength > 0.f )
					{
						// Calculate the wrap length
						const Float loopFracTime = fmod( timePastLoopEnd, loopLength );
						ASSERT( loopFracTime < loopLength );

						// Collect from the previous time to the end of the loop to the end
						fxDefinition->CollectTracksToStart( prevTime, fxDefinition->GetLoopEnd(), m_trackItemsToActivate );
						CollectActivePlayDataToKill( prevTime, fxDefinition->GetLoopEnd(), m_playDataToRemove );

						// Restart at wrapped time
						m_currentTime = fxDefinition->GetLoopStart() + loopFracTime;
						fxDefinition->CollectTracksToStart( fxDefinition->GetLoopStart(), m_currentTime, m_trackItemsToActivate );
						CollectActivePlayDataToKill( fxDefinition->GetLoopStart(), m_currentTime, m_playDataToRemove );
					}
					else
					{
						// Collect from the previous time to the end of the loop to the end
						fxDefinition->CollectTracksToStart( prevTime, fxDefinition->GetLoopEnd(), m_trackItemsToActivate );
						CollectActivePlayDataToKill( prevTime, fxDefinition->GetLoopEnd(), m_playDataToRemove );

						// Restart at the loop start
						m_currentTime = fxDefinition->GetLoopStart();
					}
				}
				else
				{
					if( ! m_stoppingNotified )
					{
						notifyStop = true;
						m_stoppingNotified = true;
					}

					// Normal 
					fxDefinition->CollectTracksToStart( prevTime, m_currentTime, m_trackItemsToActivate );
					CollectActivePlayDataToKill( prevTime, m_currentTime, m_playDataToRemove );
				}
			}
			else
			{
				// Normal 
				fxDefinition->CollectTracksToStart( prevTime, m_currentTime, m_trackItemsToActivate );
				CollectActivePlayDataToKill( prevTime, m_currentTime, m_playDataToRemove );
			}
		}
		else
		{
			// Simple collect elements to start
			fxDefinition->CollectTracksToStart( prevTime, m_currentTime, m_trackItemsToActivate );
			CollectActivePlayDataToKill( prevTime, m_currentTime, m_playDataToRemove );
		}

		// Remove deleted tracks
		for ( Uint32 i=0; i<m_playDataToRemove.Size(); i++ )
		{
			IFXTrackItemPlayData* data = m_playDataToRemove[i];
			m_activeItems.RemoveFast( data );
			delete data;
		}
		m_playDataToRemove.ClearFast();

		// Pause if playing a normal effect
		if ( !fxDefinition->IsBoundToAnimation() && !m_isStopping && !m_isPaused )
		{
			// Get the first pause time
			Float pauseTime = FLT_MAX;
			for ( Uint32 i=0; i<m_trackItemsToActivate.Size(); i++ )
			{
				const CFXTrackItem* item = m_trackItemsToActivate[i];
				if ( item->IsA< CFXTrackItemPause >() )
				{
					pauseTime = Min< Float >( pauseTime, item->GetTimeBegin() );
				}
			}

			// Should pause ?
			if ( pauseTime != FLT_MAX )
			{
				// Pause at that tick
				m_currentTime = pauseTime + 0.00001f;	// HACK !
				m_isPaused = true;
				goto Restart;
			}
		}

		// Start new effects
		for ( Uint32 i=0; i<m_trackItemsToActivate.Size(); i++ )
		{
			const CFXTrackItem* item = m_trackItemsToActivate[i];
			
			// Start track item, keep the created track item
			IFXTrackItemPlayData* playData = item->OnStart( *this );
			if ( playData )
			{
				ASSERT( playData->GetNode() );
				ASSERT( playData->GetTrackItem() );
				m_activeItems.PushBack( playData );
			}
		}
		m_trackItemsToActivate.ClearFast();

		// Update track items
		for ( Uint32 i=0; i<m_activeItems.Size(); i++ )
		{
			// Get data to tick
			IFXTrackItemPlayData* data = m_activeItems[i];
			ASSERT( data->GetNode() );
			ASSERT( data->GetTrackItem() );

			// Update play data
			data->OnTick( *this, timeDelta );

			// Notify stop
			if( notifyStop )
			{
				data->OnStop();
			}
		}
	}

	// End of time reached
	if ( m_currentTime >= m_length )
	{
		Destroy();
	}

	// Inform editor listener
#ifndef NO_EDITOR_EVENT_SYSTEM
	else if ( GIsEditor )
	{
		SEditorEffectMessage message;
		message.m_definition = fxDefinition;
		message.m_effectTag = m_tag;
		message.m_entity = m_entity;
		message.m_effectTime = m_currentTime;
		EDITOR_DISPATCH_EVENT( CNAME( EffectTicked ), CreateEventData( message ) );
	}
#endif
}

CEntity* CFXState::CreateDynamicEntity( const EntitySpawnInfo& info )
{
	CEntity *entity = m_entity;

	// No entity
	if ( !entity )
	{
		return NULL;
	}

	// Entity is not attached
	if ( !entity->IsAttached() )
	{
		return NULL;
	}

	// Get dynamic layer
	CLayer* dynamicLayer = entity->GetLayer()->GetWorld()->GetDynamicLayer();
	return dynamicLayer->CreateEntitySync( info );
}

void CFXState::FillFxDefinition( CFXDefinition* fxDefinition )
{
	m_fxDefinition = fxDefinition; 
	m_length = fxDefinition->GetLength();

	if( fxDefinition->IsRandomStart() )
	{
		// Draw start time
		Float effectLength = fxDefinition->GetEndTime() - fxDefinition->GetStartTime();
		Float drawRange = 0.2f * effectLength;

		m_currentTime = Max( 0.0f, fxDefinition->GetStartTime() + GEngine->GetRandomNumberGenerator().Get< Float >( drawRange ) );
	}
}

void CFXState::OnComponentStreamOut( CComponent* component )
{
	ASSERT( component );

	if ( m_activeItems.Empty() )
	{
		// Nothing to notify
		return;
	}

	for ( Int32 i=m_activeItems.Size()-1; i>=0; --i )
	{
		IFXTrackItemPlayData* playData = m_activeItems[i];
		ASSERT( playData );

		CFXTrackItem* trackItem = playData->GetTrackItem();
		ASSERT( trackItem );

		playData->OnPreComponentStreamOut( component );
	}
}

void CFXState::SetEffectsParameterValue( Float intensity, const CName &specificComponentName, CName effectIntensityParameterName )
{
	Uint32 size = m_activeItems.Size();
	for ( Uint32 i = 0; i < size; i++ )
	{
		IFXTrackItemPlayData* playData = m_activeItems[i];
		ASSERT( playData );
		CEntity* effectEntity = playData->GetEffectEntity();
		if ( effectEntity != NULL )
		{
			effectEntity->SetEffectsParameterValue( intensity, specificComponentName, effectIntensityParameterName );
		}
	}
}

#ifndef NO_DEBUG_PAGES
Box CFXState::GetDebugBoundingBox() const
{
	Box retBox;
	retBox.Clear();
	for ( Uint32 i=0; i<m_activeItems.Size(); ++i )
	{
		m_activeItems[i]->CollectBoundingBoxes( retBox );
	}

	return retBox;
}

#endif

ILODable::LOD CFXState::ComputeLOD( CLODableManager* manager ) const
{
	const Vector& worldPos = m_entity->GetWorldPosition();
	const Float distSqr = manager->GetPosition().DistanceSquaredTo2D( worldPos );

	if ( distSqr < m_showDistanceSquared ) 
	{
		if( distSqr < manager->GetBudgetableDistanceSqr() )
		{
			return ILODable::LOD_0;
		}

		return ILODable::LOD_1;
	}

	return ILODable::LOD_2;
}

void CFXState::UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager )
{
	ASSERT( m_currentLOD != newLOD );

	CTickManager* tickManager = manager->GetTickManager();

	switch ( m_currentLOD )
	{
		// Enabled tick
		case ILODable::LOD_0:
			if ( newLOD == ILODable::LOD_1 )
			{
				tickManager->SetBudgeted( this, true );
			}
			else
			{
				ClearLoopedPlayData();
				tickManager->RemoveEffect( this, false );
			}
			break;

		// Budgeted tick
		case ILODable::LOD_1:
			if ( newLOD == ILODable::LOD_0 )
			{
				tickManager->SetBudgeted( this, false );
			}
			else
			{
				ClearLoopedPlayData();
				tickManager->RemoveEffect( this, true );
			}
			break;

		// Disabled tick
		case ILODable::LOD_2:
			tickManager->AddEffect( this, newLOD == ILODable::LOD_1 );
			break;
	}

	m_currentLOD = newLOD;
}

// CEffectManager

CEffectManager::CEffectManager()
{}

void CEffectManager::AddEffect( CFXState* effect )
{
	Register( effect );

	// Make it disabled at startup

	effect->SetCurrentLOD( ILODable::LOD_2 );

	// Update it to an appropriate LOD
	const ILODable::LOD initialLOD = effect->ComputeLOD( this );
	if ( initialLOD != effect->GetCurrentLOD() )
	{
		effect->UpdateLOD( initialLOD, this );
	}
}

void CEffectManager::RemoveEffect( CFXState* effect )
{
	Unregister( effect );

	if ( !effect->IsTickDisabled() )
	{
		RED_FATAL_ASSERT( m_tickManager, "No tickManager set");
		m_tickManager->RemoveEffect( effect, effect->IsTickBudgeted() );
	}
}

