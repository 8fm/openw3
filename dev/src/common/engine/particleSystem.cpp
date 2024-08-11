/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "particleSystem.h"
#include "particleEmitter.h"
#include "particleComponent.h"
#include "baseEngine.h"
#include "materialInstance.h"

#include "../../common/core/objectIterator.h"

IMPLEMENT_ENGINE_CLASS( CParticleSystem );
IMPLEMENT_ENGINE_CLASS( SParticleSystemLODLevel );


CParticleSystem::CParticleSystem()
	: m_autoHideDistance( 100.0f )
	, m_autoHideRange( 0.0f )
	, m_previewBackgroundColor( Color::GRAY )
	, m_previewShowGrid( true )
	, m_visibleThroughWalls( false )
	, m_renderingPlane( RPl_Scene )
#ifndef NO_EDITOR
	, m_nextUniqueId( 0 )
#endif
{
	m_lods.Resize( 1 );	
}

void CParticleSystem::ResetInstances()
{
	// Reset all particle components using this system
	for ( ObjectIterator< CParticleComponent > it; it; ++it )
	{
		CParticleComponent* ps = (*it);
		if ( ps->GetParticleSystem() == this )
		{
			ps->Reset();
		}
	}
}

// ** ***********************
//
static Color RandomPastelColor()
{
	Uint8 r = GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 g = GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 b = GEngine->GetRandomNumberGenerator().Get< Uint8 >();
		
	r = ( 127 + r & 127 );
	g = ( 127 + g & 127 );
	b = ( 127 + b & 127 );

	return Color( r, g, b, 255 );
}

CParticleEmitter* CParticleSystem::AddEmitter( CClass* emitterClass, const String& emitterName /*=String::EMPTY*/ )
{
	// Create particle emitter
	CParticleEmitter* newEmitter = ::CreateObject< CParticleEmitter >( emitterClass, this );
	if ( !newEmitter )
	{
		WARN_ENGINE( TXT("Unable to add emitter") );
		return NULL;
	}

	// Set name and set random color
	newEmitter->SetEditorName( emitterName.Empty() ? TXT("Unnamed Emitter") : emitterName );
	newEmitter->SetEditorColor( RandomPastelColor() );

	newEmitter->SetLODCount( m_lods.Size() );

	// Add to list of emitters
	m_emitters.PushBack( newEmitter );

#ifndef NO_EDITOR
	// Assign unique ID for easy render side recognition
	AssignUniqueId( newEmitter );
#endif

	return newEmitter;
}

Bool CParticleSystem::AddEmitter( CParticleEmitter* emitter )
{
	ASSERT( emitter );
	if ( !emitter )
	{
		return false;
	}

	if ( emitter->GetLODCount() != m_lods.Size() )
	{
#ifdef RED_LOGGING_ENABLED
		if ( emitter->GetLODCount() < m_lods.Size() )
		{
			WARN_ENGINE( TXT("Emitter has fewer LODs than the particle system. Adding new ones.") );
		}
		else
		{
			WARN_ENGINE( TXT("Emitter has more LODs that the particle system. Removing the extras.") );
		}
#endif
		emitter->SetLODCount( m_lods.Size() );
	}

	m_emitters.PushBack( emitter );
#ifndef NO_EDITOR
	AssignUniqueId( emitter );
#endif
	return true;
}

CParticleEmitter* CParticleSystem::CloneEmitter( CParticleEmitter* emitterToClone, const String& emitterName /*=String::EMPTY*/ )
{
	// No emitter to clone given
	if ( !emitterToClone )
	{
		WARN_ENGINE( TXT("Unable to add emitter") );
		return NULL;
	}

	CMaterialInstance* newMatInst = nullptr;
	if ( CMaterialInstance* matInst = Cast< CMaterialInstance >( emitterToClone->GetMaterial() ) )
	{
		newMatInst = Cast< CMaterialInstance >( matInst->Clone( this ) );
	}

	// Create particle emitter
	CParticleEmitter* newEmitter = Cast< CParticleEmitter >( emitterToClone->Clone( this ) );
	if ( !newEmitter )
	{
		WARN_ENGINE( TXT("Unable to clone emitter") );
		return NULL;
	}

	if ( newMatInst )
	{
		newEmitter->SetMaterial( newMatInst );
	}

	// Set name and set random color
	newEmitter->SetEditorName( emitterName.Empty() ? TXT("Unnamed Emitter") : emitterName );
	newEmitter->SetEditorColor( RandomPastelColor() );


	if ( newEmitter->GetLODCount() != m_lods.Size() )
	{
#ifdef RED_LOGGING_ENABLED
		if ( newEmitter->GetLODCount() < m_lods.Size() )
		{
			WARN_ENGINE( TXT("Cloned emitter has fewer LODs than the particle system. Adding new ones.") );
		}
		else
		{
			WARN_ENGINE( TXT("Cloned emitter has more LODs that the particle system. Removing the extras.") );
		}
#endif
		newEmitter->SetLODCount( m_lods.Size() );
	}


	// Add to list of emitters
	m_emitters.PushBack( newEmitter );

#ifndef NO_EDITOR
	// Assign unique ID for easy render side recognition
	AssignUniqueId( newEmitter );
#endif

	return newEmitter;
}

void CParticleSystem::RemoveEmitter( CParticleEmitter* emitter )
{
	// Find emitter on the list
	if ( !m_emitters.Remove( emitter ) )
	{
		WARN_ENGINE( TXT("Emitter not found in particle system") );
		return;
	}

	// Remove from list of emitters
	m_emitters.Remove( emitter );

	// Discard emitter
	emitter->Discard();
}

void CParticleSystem::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

void CParticleSystem::RecompileEmitters()
{
	for ( Uint32 i=0; i<m_emitters.Size(); ++i )
	{
		m_emitters[i]->CreateRenderResource();
	}
}

#ifndef NO_EDITOR
void CParticleSystem::AssignUniqueId( CParticleEmitter* emitter )
{
	emitter->SetUniqueId( m_nextUniqueId );
	++m_nextUniqueId;
}
#endif

void CParticleSystem::OnPostLoad()
{
	//Pass to base class
	TBaseClass::OnPostLoad();

#ifndef NO_EDITOR
	// Assign unique ids, for easy render side recognition
	for ( Uint32 i=0; i<m_emitters.Size(); ++i )
	{
		ASSERT( m_emitters[i] );
		AssignUniqueId( m_emitters[i] );
	}
#endif

	// Make sure we have at least one LOD defined.
	if ( m_lods.Size() == 0 )
	{
		AddLOD();
	}

	// Make sure all emitters have the right number of LODs...
	for ( CParticleEmitter* emitter : m_emitters )
	{
		if ( emitter->GetLODCount() != m_lods.Size() )
		{
			emitter->SetLODCount( m_lods.Size() );
		}
	}
}

void CParticleSystem::AddLOD()
{
	m_lods.Grow( 1 );

	// Copy distance from existing LOD
	if ( m_lods.Size() > 1 )
	{
		Uint32 lastIdx = m_lods.Size() - 1;
		m_lods[lastIdx].m_distance = m_lods[lastIdx - 1].m_distance;
	}

	for ( CParticleEmitter* emitter : m_emitters )
	{
		if ( emitter->GetLODCount() < m_lods.Size() )
		{
			emitter->AddLOD();
		}
	}
}

void CParticleSystem::RemoveLOD( Uint32 level )
{
	RED_ASSERT( level < m_lods.Size(), TXT("Trying to remove out-of-range LOD %u"), level );
	RED_ASSERT( m_lods.Size() > 1, TXT("Must have at least one LOD") );

	if ( level >= m_lods.Size() )
	{
		return;
	}

	m_lods.RemoveAt( level );
	for ( CParticleEmitter* emitter : m_emitters )
	{
		emitter->RemoveLOD( level );
	}
}

#ifndef NO_RESOURCE_COOKING

void CParticleSystem::OnCook(class ICookerFramework& cooker)
{
	TBaseClass::OnCook( cooker );

	for( Int32 i = 0; i < m_emitters.SizeInt(); ++i )
	{
		CParticleEmitter* emitter = m_emitters[i];
		Bool used = false;

		if ( emitter->IsEnabled() )
		{
			const Uint32 emitterLODCount = emitter->GetLODCount();
			for( Uint32 j = 0; j < emitterLODCount ; ++j )
			{
				if ( emitter->GetLOD(j).m_isEnabled )
				{
					used = true;
					break;
				}
			}
		}

		if ( !used )
		{
			m_emitters.RemoveAt( i-- );
			emitter->Discard();
		}
	}
}

#endif