#include "build.h"
#include "baseEngine.h"
#include "layer.h"
#include "layerInfo.h"
#include "world.h"
#include "selectionManager.h"
#include "entity.h"
#include "curveEntity.h"
#include "curveControlPointEntity.h"
#include "curveEntitySpawner.h"
#include "../../common/core/feedback.h"

IMPLEMENT_ENGINE_CLASS( CCurveEntitySpawner );
IMPLEMENT_ENGINE_CLASS( SEntityWeight );

CCurveEntitySpawner::CCurveEntitySpawner()
	: m_layer( nullptr )
	, m_density( 10 )
	, m_variation( 0.f )
	, m_curveEntity( nullptr )
{
}

void CCurveEntitySpawner::SpawnEntities( CCurveEntity* curEnt )
{
	if ( m_templateWeights.Empty() || curEnt == nullptr ) return;
	SetCurveEntity( curEnt );
	PrepareSpawnData();
	if ( m_density >= 100 )
	{
		if ( GFeedback->AskYesNo(TXT("Are you sure to spawn more than 100 entities?") ) )
		{
			InternalSpawnEntities();
		}
	}
	else
	{
		InternalSpawnEntities();
	}

}

void CCurveEntitySpawner::CalculateRanges( TDynArray< Float >& ranges )
{
	Float weight = 0.f;
	for ( Uint32 i=0; i<m_templateWeights.Size(); ++i )
	{
		ranges.PushBack( weight );
		weight += m_templateWeights[ i ].m_weight;
	}
}

void CCurveEntitySpawner::SetTransforms(TDynArray< EngineTransform >& matrices, Uint32 size)
{
	if ( matrices.Size() != size )
	{
		return;
	}
	m_transformsToSpawn = matrices;
}

void CCurveEntitySpawner::SetLayer(CLayer* layer)
{
	m_layer = layer;
}

CEntityTemplate* CCurveEntitySpawner::GetEntityTemplateByWeight(Float val, TDynArray<Float>& ranges)
{
	CEntityTemplate* retEt = nullptr;
	for ( Int32 i = ranges.SizeInt()-1; i >= 0; --i )
	{
		if ( val > ranges[ i ] )
		{
			retEt = m_templateWeights[ i ].m_template;
			break;
		}
	}
	return retEt;
}

void CCurveEntitySpawner::PrepareSpawnData()
{
	SMultiCurve* multiCurve = m_curveEntity->GetCurve();
	if ( m_density != 0 && multiCurve )
	{
		Float curveTime = multiCurve->GetTotalTime();
		if ( curveTime <= 0.f ) return;

		CLayer* layerToSpawn = m_curveEntity->GetLayer();
		if ( !layerToSpawn ) return;
		CWorld* w = layerToSpawn->GetWorld();
		if( !w ) return;
		CSelectionManager* selMgr = w->GetSelectionManager();
		if( !selMgr ) return;
		CLayerInfo* activeLayer = selMgr->GetActiveLayer();
		if( !activeLayer ) return;
		layerToSpawn = activeLayer->GetLayer();
		if ( !layerToSpawn ) return;
		SetLayer( layerToSpawn );

		TDynArray< EngineTransform > matrices;
		matrices.ClearFast();
		matrices.Reserve( m_density );

		Float fixedSubstep = (curveTime/(Float)m_density);
		Float epsilon = fixedSubstep/1000.f;
		Float currentTime = 0.f;
		
		EngineTransform transform;
		multiCurve->GetAbsoluteTransform( currentTime, transform );
		Vector previousPos = transform.GetPosition();

		CStandardRand& r = GEngine->GetRandomNumberGenerator();

		for ( Uint32 i = 0; i<m_density; ++i )
		{
			// add some variation to substep
			Float randomVariation = r.Get< Float >( -m_variation, m_variation );
			randomVariation /= (curveTime*100.f);
			currentTime = Abs<Float>( currentTime+randomVariation );
			// clamp in case
			currentTime = Clamp<Float>( currentTime, 0.f, curveTime );

			// calc orientation
			multiCurve->GetAbsoluteTransform( currentTime-epsilon, transform );
			previousPos = transform.GetPosition();
			multiCurve->GetAbsoluteTransform( currentTime+epsilon, transform );
			Vector currentPos = transform.GetPosition();
			Vector direction = currentPos-previousPos;
			EulerAngles rotation = direction.ToEulerAngles();
			
			// get correct position
			multiCurve->GetAbsoluteTransform( currentTime, transform );
			
			// set calculated rotation
			transform.SetRotation( rotation );

			// cache current position as previous one
			previousPos = currentPos;

			matrices.PushBack( transform );

			// add subsept for next loop
			currentTime += fixedSubstep;
		}
		SetTransforms( matrices, m_density );
	}
}

void CCurveEntitySpawner::SetCurveEntity(CCurveEntity* curveEntity)
{
	if( curveEntity != nullptr )
	{
		m_curveEntity = curveEntity;
	}
}

void CCurveEntitySpawner::InternalSpawnEntities()
{
	if ( m_layer )
	{
		if( !m_spawnedEntities.Empty() )
		{
			for ( CEntity* e : m_spawnedEntities )
			{
				if ( e )
				{
					m_layer->DestroyEntity( e );
				}
			}
		}
		m_spawnedEntities.ClearFast();

		CStandardRand& r = GEngine->GetRandomNumberGenerator();
		TDynArray<Float> rangesToRand;
		Uint32 rangesSize = m_templateWeights.Size();
		rangesToRand.Resize( rangesSize );
		rangesToRand.ClearFast();
		CalculateRanges( rangesToRand );

		Float maxRangeValue = rangesToRand[ rangesToRand.Size()-1 ]+m_templateWeights[ m_templateWeights.Size()-1 ].m_weight;

		for ( Uint32 i=0; i<m_transformsToSpawn.Size(); ++i )
		{
			Float randomWeight = r.Get< Float >( 0.f, maxRangeValue );
			EntitySpawnInfo spawnInfo;
			spawnInfo.m_template = GetEntityTemplateByWeight( randomWeight, rangesToRand );
			spawnInfo.m_spawnPosition = m_transformsToSpawn[i].GetPosition();
			spawnInfo.m_spawnRotation = m_transformsToSpawn[i].GetRotation();
			CEntity* entity = m_layer->CreateEntitySync( spawnInfo );
			if ( entity )
			{
				m_spawnedEntities.PushBack( entity );
			}
		}
	}
}
