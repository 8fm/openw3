#include "build.h"
#include "spawnTreeSpawnStrategy.h"

#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/instanceDataLayoutCompiler.h"

#include "encounter.h"
#include "encounterCreaturePool.h"
#include "spawnTreeBaseEntry.h"
#include "entityPool.h"

IMPLEMENT_ENGINE_CLASS( SCompiledSpawnStrategyInitializer );
IMPLEMENT_ENGINE_CLASS( ISpawnTreeSpawnStrategy );
IMPLEMENT_ENGINE_CLASS( CSimpleSpawnStrategy );
IMPLEMENT_ENGINE_CLASS( SSpawnStrategyRange );
IMPLEMENT_ENGINE_CLASS( CMultiRangeSpawnStrategy );

namespace Config
{
	TConfigVar< Bool >			cvAllowSpawnInView( "SpawnTree", "AllowSpawnInView", false );
}

ISpawnTreeSpawnStrategy::ISpawnTreeSpawnStrategy()
	: m_enablePooling( false )
	, m_overflowPoolRange( 10.f )
{
}

Bool ISpawnTreeSpawnStrategy::IsConflicting( const ISpawnTreeInitializer* initializer ) const 
{
	return initializer->IsA< ISpawnTreeSpawnStrategy >();
}

void ISpawnTreeSpawnStrategy::Tick( CEncounterCreaturePool::SCreatureList& creatures, CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context ) const 
{
	PC_SCOPE_PIX( SpawnStrategy_Tick );

	const Matrix& referenceLtW = context.m_referenceTransform;
	const Vector& referencePos = referenceLtW.GetTranslationRef();

	CEncounterCreaturePool& creaturePool = instance.GetEncounter()->GetCreaturePool();
	CEncounterCreaturePool::CPartiesManager& parties = creaturePool.GetPartiesManager();

	TStaticArray< CActor*, 256 > actorsToDespawn;												// with actor limit around 100 we can safely assume that no single CEncounter entry will spawn more than 256 actors... yes?

	const Bool globalLimitExceeded = GCommonGame->GetEntityPool()->IsSpawnLimitExceeded();

	Bool isMarkingCreatures = false;

	for( auto it = creatures.Begin(), end = creatures.End(); it != end; ++it )
	{
		CEncounterCreaturePool::SCreature& creature = *it;
		CActor* actor = creature.m_actor;

		if ( creature.m_stateFlags 
			& ( CEncounterCreaturePool::SCreature::FLAG_IS_POOL_REQUESTED						// fist check if he is already requested to pool
			| CEncounterCreaturePool::SCreature::FLAG_PROCESSING_MARKING )						// don't process already processed party members
			)
		{
			continue;
		}

		Bool forcePool = globalLimitExceeded && creature.m_spawnGroup > 0;

		// special party processing branch
		if ( creature.m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_IS_IN_PARTY )
		{
			const CEncounterCreaturePool::Party* party = parties.GetParty( actor );
			if ( party )
			{
				isMarkingCreatures = true;
				Bool isPartyPoolable = true;
				
				for ( const CEncounterCreaturePool::PartyMember& partyMember : *party )
				{
					if ( !CheckPool( partyMember.m_actor, referencePos, referenceLtW, forcePool ) )
					{
						isPartyPoolable = false;
						break;
					}
				}

				for ( const CEncounterCreaturePool::PartyMember& partyMember : *party )
				{
					CEncounterCreaturePool::SCreature* partyCreature = creaturePool.GetCreatureEntry( partyMember.m_actor );
					// as creature is in party its 'guaranteed' to be in pool
					partyCreature->m_stateFlags |= CEncounterCreaturePool::SCreature::FLAG_PROCESSING_MARKING;			
					ASSERT( partyCreature->m_listId == creature.m_listId );											// its also guaranteed to be on same creature list. Otherwise marking would get fucked up.

					if ( isPartyPoolable )
					{
						partyCreature->m_stateFlags |= CEncounterCreaturePool::SCreature::FLAG_IS_POOL_REQUESTED;
						actorsToDespawn.PushBack( partyMember.m_actor );
					}
				}

				// end of special case
				continue;
			}
		}

		// common case
		if ( CheckPool( actor, referencePos, referenceLtW, forcePool ) )
		{
			creature.m_stateFlags |= CEncounterCreaturePool::SCreature::FLAG_IS_POOL_REQUESTED;
			actorsToDespawn.PushBack( actor );
		}
	}

	// clear marking (if needed) - used only for parties
	if ( isMarkingCreatures )
	{
		for( auto it = creatures.Begin(), end = creatures.End(); it != end; ++it )
		{
			CEncounterCreaturePool::SCreature& creature = *it;
			creature.m_stateFlags &= ~CEncounterCreaturePool::SCreature::FLAG_PROCESSING_MARKING;
		}
	}

	// kill kill kill
	if ( !actorsToDespawn.Empty() )
	{
		CEntityPool& pool = *GCommonGame->GetEntityPool();

		for ( auto it = actorsToDespawn.Begin(), end = actorsToDespawn.End(); it != end; ++it )
		{
			CActor* actor = *it;
			pool.AddEntity( actor );
		}
	}
}

Bool ISpawnTreeSpawnStrategy::CheckPool( CActor* actor, const Vector3& referencePos, const Matrix& referenceLtW, Bool isOverflow ) const
{
	return false;
}

void ISpawnTreeSpawnStrategy::LoadFromConfig()
{
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("EnablePooling"), m_enablePooling );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("OverflowRange"), m_overflowPoolRange );
}

void ISpawnTreeSpawnStrategy::SaveToConfig()
{
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("EnablePooling"), m_enablePooling );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("OverflowRange"), m_overflowPoolRange );
}

//////////////////////////////////////////////////////////////////////////

CSimpleSpawnStrategy::CSimpleSpawnStrategy()
	: m_minSpawnRange( 10.f )
	, m_visibilityTestRange( 40.f )
	, m_maxSpawnRange( 100.f )
	, m_canPoolOnSight( false )
	, m_minPoolRange( 110.f )
	, m_forcePoolRange( 200.f )
{
	m_maxSqrSpawnRange = ( m_maxSpawnRange * m_maxSpawnRange );
	m_minSqrSpawnRange = ( m_minSpawnRange * m_minSpawnRange );
	m_sqrVisibilityTestRange = ( m_visibilityTestRange * m_visibilityTestRange );
}

String CSimpleSpawnStrategy::GetEditorFriendlyName() const
{
	static String STR( TXT("Spawn Strategy - Simple") );
	return STR;
}

CSimpleSpawnStrategy::ESpawnPointTestResult CSimpleSpawnStrategy::CheckSpawnPoint( SSpawnTreeUpdateSpawnContext& context, const Vector3& point ) const
{
	const Float distSq = (point - context.m_referenceTransform.GetTranslationRef().AsVector3()).SquareMag();
	if ( distSq > m_maxSqrSpawnRange )
	{
		return VR_Reject;
	}

	if ( context.m_ignoreMinDistance )
	{
		const Float MIN_DISTANCE = 2.f;
		if ( distSq < MIN_DISTANCE*MIN_DISTANCE )
		{
			return VR_Reject;
		}
	}
	else
	{
		if ( distSq < m_minSqrSpawnRange )
		{
			return VR_Reject;
		}
	}

	if ( distSq > m_sqrVisibilityTestRange || context.m_ignoreVision )
	{
		return VR_Accept_NoVisibilityTest;
	}

	if ( Config::cvAllowSpawnInView.Get() )
		return VR_Accept_NoVisibilityTest;

	return VR_Accept_VisibilityTest;
}

Bool CSimpleSpawnStrategy::CheckPool( CActor* actor, const Vector3& referencePos, const Matrix& referenceLtW, Bool isOverflow ) const
{
	const Float squareDist = (actor->GetWorldPositionRef() - referencePos).SquareMag3();
	Bool shouldPool = isOverflow ? squareDist > m_overflowPoolRange * m_overflowPoolRange : squareDist > m_minPoolRange * m_minPoolRange;
	
	return shouldPool && ( squareDist > m_forcePoolRange * m_forcePoolRange || !actor->WasVisibleLastFrame() );		
}

void CSimpleSpawnStrategy::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	const CName& name = property->GetName();
	if( name == TXT("minSpawnRange") || name == TXT("maxSpawnRange")
		|| name == TXT("minPoolRange") || name == TXT("forcePoolRange") )
	{
		if( m_minSpawnRange >= m_maxSpawnRange )
		{
			m_maxSpawnRange = m_minSpawnRange + 10.f;
			m_maxSqrSpawnRange = m_maxSpawnRange * m_maxSpawnRange;
		}

		if( m_maxSpawnRange >= m_minPoolRange )
		{
			m_minPoolRange = m_maxSpawnRange + 10.f;
		}

		if( m_minPoolRange >= m_forcePoolRange )
		{
			m_forcePoolRange = m_minPoolRange + 10.f;
		}
	}
}

void CSimpleSpawnStrategy::LoadFromConfig()
{
	TBaseClass::LoadFromConfig();

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("MinSpawnRange"), m_minSpawnRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("MaxSpawnRange"), m_maxSpawnRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("CanPoolOnSight"), m_canPoolOnSight );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("MinPoolRange"), m_minPoolRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("ForcePoolRange"), m_forcePoolRange );
}

void CSimpleSpawnStrategy::SaveToConfig()
{
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("SpawnStrategy"), String(TXT("Simple")) );

	TBaseClass::SaveToConfig();

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("MinSpawnRange"), m_minSpawnRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("MaxSpawnRange"), m_maxSpawnRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("CanPoolOnSight"), m_canPoolOnSight );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("MinPoolRange"), m_minPoolRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("ForcePoolRange"), m_forcePoolRange );
}

void CSimpleSpawnStrategy::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	m_maxSqrSpawnRange = ( m_maxSpawnRange * m_maxSpawnRange );
	m_minSqrSpawnRange = ( m_minSpawnRange * m_minSpawnRange );
	m_sqrVisibilityTestRange = ( m_visibilityTestRange * m_visibilityTestRange );
}

//////////////////////////////////////////////////////////////////////////

CMultiRangeSpawnStrategy::CMultiRangeSpawnStrategy()
	: m_primaryMinSpawnRange( 5.f )
	, m_primaryMaxSpawnRange( 100.f )
	, m_visibilityTestRange( 40.f )
	, m_primaryMinPoolRange( 110.f )
	, m_canPoolOnSight( false )
	, m_forcePoolRange( 200.f )
	, m_poolDelay( 3.f )
{
	m_primaryMaxSqrSpawnRange = ( m_primaryMaxSpawnRange * m_primaryMaxSpawnRange );
	m_primaryMinSqrSpawnRange = ( m_primaryMinSpawnRange * m_primaryMinSpawnRange );
	m_sqrVisibilityTestRange = ( m_visibilityTestRange * m_visibilityTestRange );
}

String CMultiRangeSpawnStrategy::GetEditorFriendlyName() const
{
	static String STR( TXT("Spawn Strategy - Multi Range") );
	return STR;
}

CMultiRangeSpawnStrategy::ESpawnPointTestResult CMultiRangeSpawnStrategy::CheckSpawnPoint( SSpawnTreeUpdateSpawnContext& context, const Vector3& point ) const
{
	PC_SCOPE_PIX( MultiRange_CheckSpawnPoint );

	const Matrix& referenceLtW = context.m_referenceTransform;
	const Vector& referencePos = referenceLtW.GetTranslationRef();
	const Float distSq = (point - referencePos.AsVector3()).SquareMag();

	if ( context.m_ignoreMinDistance )
	{
		const Float CLOSE_RANGE = 2.f;
		if ( distSq <= m_primaryMaxSqrSpawnRange && distSq >= CLOSE_RANGE*CLOSE_RANGE )
		{
			if ( distSq > m_sqrVisibilityTestRange || context.m_ignoreVision )
			{
				return VR_Accept_NoVisibilityTest;
			}

			if ( Config::cvAllowSpawnInView.Get() )
				return VR_Accept_NoVisibilityTest;

			return VR_Accept_VisibilityTest;
		}
	}
	
	if( (distSq <= m_primaryMaxSqrSpawnRange) && (distSq >= m_primaryMinSqrSpawnRange) )
	{
		if ( distSq > m_sqrVisibilityTestRange || context.m_ignoreVision )
		{
			return VR_Accept_NoVisibilityTest;
		}

		if ( Config::cvAllowSpawnInView.Get() )
			return VR_Accept_NoVisibilityTest;

		return VR_Accept_VisibilityTest;
	}
	else
	{
		for( auto it = m_orientedRanges.Begin(), end = m_orientedRanges.End(); it != end; ++it )
		{
			const SSpawnStrategyRange& range = *it;
			const Float squareORange = range.m_spawnRange * range.m_spawnRange;
			const Vector rangePos = referenceLtW.TransformPoint( range.m_offset );
			const Float squareODist = (point - rangePos.AsVector3()).SquareMag();

			if( squareODist <= squareORange )
			{
				if ( distSq > m_sqrVisibilityTestRange || context.m_ignoreVision )
				{
					return VR_Accept_NoVisibilityTest;
				}

				if ( Config::cvAllowSpawnInView.Get() )
					return VR_Accept_NoVisibilityTest;

				return VR_Accept_VisibilityTest;
			}
		}
	}

	return VR_Reject;
}

Bool CMultiRangeSpawnStrategy::CheckPool( CActor* actor, const Vector3& referencePos, const Matrix& referenceLtW, Bool isOverflow ) const
{
	PC_SCOPE_PIX( MultiRange_CheckPool );

	const Vector& actorPos = actor->GetWorldPositionRef();
	const Float squareDist = (actorPos - referencePos).SquareMag3();

	if( isOverflow && squareDist > (m_overflowPoolRange * m_overflowPoolRange) )
	{
		return squareDist > m_forcePoolRange * m_forcePoolRange || !actor->WasVisibleLastFrame();
	}

	const Float squareRange = m_primaryMinPoolRange * m_primaryMinPoolRange;

	Bool isWithinRanges = squareDist <= squareRange;

	if( !isWithinRanges )
	{
		for( auto rit = m_orientedRanges.Begin(), end = m_orientedRanges.End(); rit != end; ++rit )
		{
			const SSpawnStrategyRange& range = *rit;
			const Float squareORange = range.m_poolRange * range.m_poolRange;
			const Vector rangePos = referenceLtW.TransformPoint( range.m_offset );
			const Float squareODist = (actorPos - rangePos).SquareMag3();

			if( squareODist <= squareORange )
			{
				isWithinRanges = true;
				break;
			}
		}
	}

	if( isWithinRanges )
	{
		return false;
	}
	else
	{
		return ( squareDist > m_forcePoolRange * m_forcePoolRange || !actor->WasVisibleLastFrame() );
	}
}

void CMultiRangeSpawnStrategy::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	const CName& name = property->GetName();

	Float maxPoolRange = 0.f;

	if( name == TXT("orientedRanges") )
	{
		for( auto it = m_orientedRanges.Begin(), end = m_orientedRanges.End(); it != end; ++it )
		{
			SSpawnStrategyRange& range = *it;
			
			if( range.m_spawnRange >= range.m_poolRange )
			{
				range.m_poolRange = range.m_spawnRange + 10.f;
			}

			const Float realPoolRange = range.m_offset.Mag() + range.m_poolRange;
			maxPoolRange = Max( maxPoolRange, realPoolRange );
		}
	}

	if( name == TXT("primaryMinSpawnRange") || name == TXT("primaryMaxSpawnRange")
		|| name == TXT("primaryMinPoolRange") || name == TXT("forcePoolRange") )
	{
		if( m_primaryMinSpawnRange >= m_primaryMaxSpawnRange )
		{
			m_primaryMaxSpawnRange = m_primaryMinSpawnRange + 10.f;
			m_primaryMaxSqrSpawnRange = m_primaryMaxSpawnRange * m_primaryMaxSpawnRange;
		}

		if( m_primaryMaxSpawnRange >= m_primaryMinPoolRange )
		{
			m_primaryMinPoolRange = m_primaryMaxSpawnRange + 10.f;
		}

		if( m_primaryMinPoolRange >= m_forcePoolRange )
		{
			m_forcePoolRange = m_primaryMinPoolRange + 10.f;
		}
	}

	m_forcePoolRange = Max( m_forcePoolRange, maxPoolRange );
}

void CMultiRangeSpawnStrategy::LoadFromConfig()
{
	TBaseClass::LoadFromConfig();

	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("MinSpawnRange"), m_primaryMinSpawnRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("MaxSpawnRange"), m_primaryMaxSpawnRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("MinPoolRange"), m_primaryMinPoolRange );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("CanPoolOnSight"), m_canPoolOnSight );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("ForcePoolRange"), m_forcePoolRange );

	// This does not seem to be used in game
	//SConfig::GetInstance().GetLegacy().ReadParamArray( TXT("Gameplay"), TXT("Encounter"), TXT("OrientedRanges"), m_orientedRanges );

	m_primaryMaxSqrSpawnRange = ( m_primaryMaxSpawnRange * m_primaryMaxSpawnRange );
	m_primaryMinSqrSpawnRange = ( m_primaryMinSpawnRange * m_primaryMinSpawnRange );
}

void CMultiRangeSpawnStrategy::SaveToConfig()
{
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("SpawnStrategy"), String(TXT("MultiRange")) );

	TBaseClass::SaveToConfig();

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("MinSpawnRange"), m_primaryMinSpawnRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("MaxSpawnRange"), m_primaryMaxSpawnRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("MinPoolRange"), m_primaryMinPoolRange );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("CanPoolOnSight"), m_canPoolOnSight );
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("gameplay"), TXT("Encounter"), TXT("ForcePoolRange"), m_forcePoolRange );

	// This does not seem to be used in game
	//SConfig::GetInstance().GetLegacy().WriteParamArray( TXT("gameplay"), TXT("Encounter"), TXT("OrientedRanges"), m_orientedRanges );
}

void CMultiRangeSpawnStrategy::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	m_primaryMaxSqrSpawnRange = ( m_primaryMaxSpawnRange * m_primaryMaxSpawnRange );
	m_primaryMinSqrSpawnRange = ( m_primaryMinSpawnRange * m_primaryMinSpawnRange );
	m_sqrVisibilityTestRange = ( m_visibilityTestRange * m_visibilityTestRange );
}
