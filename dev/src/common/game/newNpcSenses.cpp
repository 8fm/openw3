/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newNPCSense.h"
#include "actorsManager.h"

String CNewNPC::GetSensesInfo() const
{
	return m_senses.GetInfo();
}

void CNewNPC::InitializeSenses()
{
	CacheSenseParams( AIST_Vision );
	CacheSenseParams( AIST_Absolute );
	m_senses.Initialize();

#ifndef RED_FINAL_BUILD
	//String lst = GetSensesInfo();
	//AI_LOG( TXT("NPC '%ls' - senses found:%s"), GetName().AsChar(), lst.AsChar() );
#endif
}

CEntityTemplate* CNewNPC::CacheSenseParams( EAISenseType senseType )
{
	struct SSenseTypePred
	{
		EAISenseType	m_type;

		SSenseTypePred( EAISenseType type )
			: m_type( type )
		{}

		Bool operator()( CAIProfile* aiProfile ) const
		{
			return ( aiProfile->GetSenseParams( m_type ) != nullptr );
		}
	};

	m_senses.SetSenseParams( senseType, nullptr );
	CEntityTemplate* entityTemplate = GetEntityTemplate();
	if( entityTemplate )
	{
		CAIProfile* profile = entityTemplate->FindParameter< CAIProfile >( true, SSenseTypePred( senseType ) );
		if ( profile )
		{
			CAISenseParams* params = profile->GetSenseParams( senseType );
			m_senses.SetSenseParams( senseType, params );
			return SafeCast< CEntityTemplate >( profile->GetParent() );
		}
	}

	return nullptr;
}

CAISenseParams* CNewNPC::GetSenseParams( EAISenseType senseType )
{
	return m_senses.GetSenseParams( senseType );
}

namespace
{
	struct UpdateNpcFunctor
	{
		enum { SORT_OUTPUT = false };

		CNewNPC*					m_owner;
		CNewNPCSense*				m_sense;
		TNoticedObjects*			m_noticedObjects;
		NewNPCSenseUpdateFlags		m_changed;

		UpdateNpcFunctor( CNewNPC* owner, CNewNPCSense* sense, TNoticedObjects* noticedObjects )
			: m_owner( owner )
			, m_sense( sense )
			, m_noticedObjects( noticedObjects )
			, m_changed( CNewNPCSense::FLAG_NO_CHANGES )
		{}

		Bool operator()( const CActorsManagerMemberData& memberData )
		{
			RED_ASSERT( memberData.Get() != nullptr );
			CNewNPC* npc = Cast< CNewNPC >( memberData.Get() );
			if ( npc != nullptr && npc != m_owner )
			{
				m_changed = m_sense->UpdateNPC( npc, *m_noticedObjects ) | m_changed;
			}
			return true;
		}
	};
}

NewNPCSenseUpdateFlags CNewNPC::UpdateSenses( Float timeDelta )
{
	if ( !GGame->IsActive() )
	{
		return false;
	}

	PC_SCOPE_PIX( NewNPC_UpdateSenses );

	NewNPCSenseUpdateFlags changed = CNewNPCSense::FLAG_NO_CHANGES;
	{
		changed = m_senses.UpdateKnowledge( timeDelta, m_noticedObjects ) | changed;
	}

	// If object is not alive we still need to update (clear) knowledge
	// that is why we check IsAlive condition here.
	if ( IsAlive() && m_senses.ShouldUpdate( timeDelta ) )
	{
		m_senses.BeginUpdate( m_noticedObjects );
		if ( m_senses.ShouldUpdatePlayer( timeDelta ) )
		{
			changed = m_senses.UpdatePlayer( m_noticedObjects ) | changed;
		}
		if ( m_senses.ShouldUpdateNPCs( timeDelta ) )
		{
			RED_ASSERT( GCommonGame != nullptr && GCommonGame->GetActorsManager() != nullptr );
			CActorsManager* actorsStorage = GCommonGame->GetActorsManager();
			UpdateNpcFunctor functor( this, &m_senses, &m_noticedObjects );
			Box testBox ( Vector::ZERO_3D_POINT, m_senses.GetRangeMax() );
			actorsStorage->TQuery( this->GetWorldPosition(), functor, testBox, false, nullptr, 0 );
			changed |= functor.m_changed;
		}
	}

	changed = m_senses.EndUpdate( m_noticedObjects, changed, timeDelta ) | changed;
	return changed;
}
