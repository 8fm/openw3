/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

// -------------------------------
struct SEntityWeight
{
	DECLARE_RTTI_STRUCT( SEntityWeight );

	THandle< CEntityTemplate >	m_template;
	Float						m_weight;

	SEntityWeight() : m_template( nullptr ), m_weight( 1.f ) {}
};

BEGIN_CLASS_RTTI( SEntityWeight );
	PROPERTY_EDIT( m_template, TXT("Templates to spawn") );
	PROPERTY_EDIT( m_weight, TXT("Probability/Weight") );
END_CLASS_RTTI();


class CCurveEntitySpawner : public CObject
{
	DECLARE_ENGINE_CLASS( CCurveEntitySpawner, CObject, 0 );

private:
	TDynArray< SEntityWeight >			m_templateWeights;
	TDynArray< EngineTransform >		m_transformsToSpawn;
	CLayer*								m_layer;
	TDynArray< THandle< CEntity > >		m_spawnedEntities;
	Uint32								m_density;
	Float								m_variation;
	THandle< CCurveEntity >				m_curveEntity;

public:
	CCurveEntitySpawner();
	void					SpawnEntities( CCurveEntity* curveEntity );
	void					SetCurveEntity( CCurveEntity* curveEntity );
private:
	CEntityTemplate*		GetEntityTemplateByWeight( Float val, TDynArray<Float>& ranges );
	void					PrepareSpawnData();
	void					InternalSpawnEntities();
	void					CalculateRanges( TDynArray<Float>& ranges );
	void					SetTransforms( TDynArray< EngineTransform >& matrices, Uint32 size );
	void					SetLayer( CLayer* layer );
};

BEGIN_CLASS_RTTI( CCurveEntitySpawner );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_density, TXT("Entities count") );
	PROPERTY_EDIT( m_variation, TXT("Density variation") );
	PROPERTY_EDIT( m_templateWeights, TXT("Templates to spawn") );
END_CLASS_RTTI();

