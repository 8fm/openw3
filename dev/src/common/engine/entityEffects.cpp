/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entity.h"
#include "fxState.h"
#include "animatedComponent.h"
#include "drawableComponent.h"
#include "componentIterator.h"
#include "entityTemplate.h"
#include "layer.h"
#include "world.h"
#include "tickManager.h"
#include "fxDefinition.h"

void CEntity::OnActiveEffectRemoved( CFXState* fxState )
{
	ASSERT( fxState );
	ASSERT( m_activeEffects.Exist( fxState ) );
	CWorld* world = GetLayer()->GetWorld();	
	world->GetEffectManager().RemoveEffect( fxState );
	m_activeEffects.Remove( fxState );
}

Bool CEntity::IsAnyEffectAlive() const
{
	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->IsAlive() )
		{
			return true;
		}
	}

	// Not playing any effect
	return false;
}
Bool CEntity::IsPlayingAnyEffect() const
{
	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && !state->IsStopping() )
		{
			return true;
		}
	}

	// Not playing any effect
	return false;
}

Bool CEntity::IsPlayingEffect( const CName& effectTag, Bool treatStoppingAsPlaying ) const
{
	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetTag() == effectTag && ( treatStoppingAsPlaying || !state->IsStopping() ) )
		{
			return true;
		}
	}

	// Not playing given effect
	return false;
}

Bool CEntity::IsPlayingEffect( const CFXDefinition* def, Bool treatStoppingAsPlaying ) const
{
	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetDefinition() == def && ( treatStoppingAsPlaying || !state->IsStopping() ) )
		{
			return true;
		}
	}

	// Not playing given effect
	return false;
}

Bool CEntity::IsPausedEffect( const CName& effectTag ) const
{
	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetTag() == effectTag )
		{
			return state->IsPaused();
		}
	}

	// Not playing given effect
	return false;
}

Bool CEntity::IsPausedEffect( const CFXDefinition* def ) const
{
	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetDefinition() == def )
		{
			return state->IsPaused();
		}
	}

	// Not playing given effect
	return false;
}

Bool CEntity::PlayEffect( const CName& effectName, const CName& boneName /* = CName::NONE */, const CNode* targetNode /* = NULL */, const CName& targetBone /* = CName::NONE */ )
{
	// No entity template, no effects
	if ( !m_template )
	{
		LOG_ENGINE( TXT("Cannot play effect '%ls' on entity '%ls': no template"), effectName.AsString().AsChar(), GetFriendlyName().AsChar() );
		return false;
	}

	// Find effect by name
	const CFXDefinition* effectToPlay = m_template->FindEffect( effectName );
	if ( !effectToPlay )
	{
		// LOG_ENGINE( TXT("Cannot play effect '%ls' on entity '%ls': not found"), effectName.AsChar(), GetFriendlyName().AsChar() );

		if ( m_template->PreloadEffect( effectName ) )
		{
			// async 
			return PlayEffect( NULL, effectName, 0.0f, boneName, targetNode, targetBone );
		}
		else
		{
			return false;
		}
	}

	// Play effect
	return PlayEffect( effectToPlay, effectName, 0.0f, boneName, targetNode, targetBone );
}

Bool CEntity::HasEffect( const CName& effectName )
{
	// No entity template, no effects
	if ( !m_template )
	{
		return false;
	}

	// Find effect by name
	return m_template->HasEffect( effectName );
}

void CEntity::GetActiveEffects( TDynArray< CName >& effects )
{
	for ( CFXState* state : m_activeEffects )
	{
		if ( state->IsAlive() && !state->IsStopping() )
		{
			effects.PushBack( state->GetTag() );
		}
	}
}

void CEntity::PlayAllEffects()
{
	TDynArray<CName> names;
	
	if ( m_template )
	{
		m_template->GetAllEffectsNames( names );

		for( Uint32 i = 0; i < names.Size(); ++i )
		{
			PlayEffect( names[i] );
		}
	}
}

Bool CEntity::PlayEffect( const CFXDefinition* effectToPlay, const CName& effectTag, Float startTime,
	const CName& boneName /* = CName::NONE */, const CNode* targetNode /* = NULL */, const CName& targetBone /* = CName::NONE */, Bool isForAnim /*=false*/ )
{
	// Cannot play when not attached
	if ( !IsAttached() )
	{
		LOG_ENGINE( TXT( "Cannot play effect '%ls' on entity '%ls': not attached to world" ), effectTag.AsString().AsChar(), GetFriendlyName().AsChar() );
		return false;
	}

	// Attach to world
	if ( effectToPlay ) 
	{ 
		CWorld* world = GetLayer()->GetWorld();
		CEffectManager& effectManager = world->GetEffectManager();
		Float showDistanceSqr = effectToPlay->GetShowDistanceSqr();
		if( !effectToPlay->IsLooped() )
		{
			// Ignore the one shot (not looped) effects that are beyond their supposed show distance
			if ( GetWorldPosition().DistanceSquaredTo( effectManager.GetPosition() ) > showDistanceSqr )
			{
				// Too far away
				return false;
			}
		}

		CName usedEffectTag( effectTag );
		// Effect tag not given, use name of the effect
		if ( !usedEffectTag )
		{
			usedEffectTag = effectToPlay->GetName();
		}

		// Create FX state
		CFXState* fxState = new CFXState( effectToPlay, showDistanceSqr, usedEffectTag, this, startTime, false, boneName, targetNode, targetBone, isForAnim );
		if ( !fxState )
		{
			LOG_ENGINE( TXT( "Cannot play effect '%ls' on entity '%ls': unable to spawn FX state" ), usedEffectTag.AsString().AsChar(), GetFriendlyName().AsChar() );
			return false;
		}

		effectManager.AddEffect( fxState );

		// Remember
		m_activeEffects.PushBack( fxState );

		return true;
	}

	return false;
}

Bool CEntity::PlayEffectPreview( const CFXDefinition* effectToPlay )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		const CName& animationName = effectToPlay->GetAnimationName();
		if ( ! animationName.Empty() )
		{
			ac->PlayAnimationOnSkeleton( animationName );
			return true;
		}
	}

	// Play normal effect
	return PlayEffect( effectToPlay, CName::NONE, 0.0f );
}

Bool CEntity::PlayEffectForAnimation( const CName& animationName, Float startTime/*=0.0f*/ )
{
	if ( m_template )
	{
		// Find effect by name
		const CFXDefinition* effectToPlay = m_template->FindEffectForAnimation( animationName );
		if ( effectToPlay )
		{
			return PlayEffect( effectToPlay, CName::NONE, startTime );
		}
		else
		{
			if ( m_template->PreloadEffectForAnimation( animationName ) )
			{
				// async 
				return PlayEffect( NULL, animationName, startTime, CName::NONE, NULL, CName::NONE, true );
			}
		}
	}

	// Not played
	return false;
}

Bool CEntity::StopEffect( const CName& effectTag )
{
	// Search for effect
	Bool flag = false;
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetTag() == effectTag )
		{
			state->Stop();
			flag = true;
		}
	}

	// Return status flag
	return flag;
}

Bool CEntity::DestroyEffect( const CName& effectTag )
{
	CWorld* world = GetLayer()->GetWorld();	
	ASSERT( world, TXT("Entity was detached from world prior to destroying effects!") );

	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetTag() == effectTag )
		{
			state->Destroy();
			return true;
		}
	}

	// Return status flag
	return false;
}

Bool CEntity::StopEffect( const CFXDefinition* def )
{
	// Search for effect
	Bool flag = false;
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetDefinition() == def )
		{
			state->Stop();
			flag = true;
		}
	}

	// Return status flag
	return flag;
}

Bool CEntity::PauseEffect( const CName& effectTag, Bool isPaused )
{
	// Search for effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetTag() == effectTag )
		{
			state->Pause( isPaused );
			return true;
		}
	}

	// Not playing given effect
	return false;
}

Bool CEntity::PauseEffect( const CFXDefinition* def, Bool isPaused )
{
	// Search for effect
	Bool flag = false;
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetDefinition() == def )
		{
			state->Pause( isPaused );
			flag = true;
		}
	}

	// Return status flag
	return flag;
}

void CEntity::PauseAllEffects( Bool isPaused )
{
	// Stop effects
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		state->Pause( isPaused );
	}
}

void CEntity::StopAllEffects()
{
	// Stop effects
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if( state )
		{
			state->Stop();
		}
	}
}

void CEntity::DestroyAllEffects()
{
	CWorld* world = GetLayer()->GetWorld();	
	ASSERT( world, TXT("Entity was detached from world prior to destroying effects!") );

	// Destroy effects
	/* copy */ TDynArray< CFXState*, MC_EntityFxState > effects = m_activeEffects;
	for ( Uint32 i=0; i<effects.Size(); i++ )
	{
		CFXState* state = effects[i];
		world->GetEffectManager().RemoveEffect( effects[i] );
		state->Cleanup();
		delete effects[i];
	}

	// Clear array
	m_activeEffects.Clear();
}

void CEntity::SetEffectIntensity( const CName& effectName, Float intensity, const CName& specificComponentName /*= CName::NONE */,
																			const CName& effectParameterName /*= CName::NONE */)
{
	// Due to backward-compatibility issues, if effectParameterName
	// was not specified (or set to NONE), it's assumed to be 'MeshScalar3'
	CName realEffectParamName = effectParameterName;
	if ( realEffectParamName == CName::NONE )
	{
		realEffectParamName = CNAME( MeshEffectScalar3 );
	}

	Bool isEffectPlayed = IsPlayingEffect( effectName, false );
	if ( intensity == 0.0f )
	{
		if ( isEffectPlayed )
		{
			StopEffect( effectName );
		}
	}
	else
	{
		if ( !isEffectPlayed )
		{
			PlayEffect( effectName );
		}
		SetEffectsParameterValue( intensity, specificComponentName, realEffectParamName );
		// pass intensity value to effect entity
		CFXState* fxState = GetActiveEffect( effectName );
		if ( fxState != NULL )
		{
			fxState->SetEffectsParameterValue( intensity, specificComponentName, realEffectParamName );
		}
	}
}

void CEntity::SetEffectsParameterValue( Float intensity, const CName &specificComponentName, CName effectIntensityParameterName )
{
	EffectParameterValue effectValue;
	effectValue.SetFloat( intensity );

	for ( ComponentIterator< CDrawableComponent > componentIter( this ); componentIter; ++componentIter )
	{
		CDrawableComponent* drawableComponent = *componentIter;
		if ( specificComponentName != CName::NONE && drawableComponent->GetName() != specificComponentName.AsString() )
		{
			continue;
		}

		drawableComponent->SetEffectParameterValue( effectIntensityParameterName, effectValue );
	}
}

CFXState* CEntity::GetActiveEffect( const CName& effectName )
{
	for ( auto it = m_activeEffects.Begin(); it != m_activeEffects.End(); ++it )
	{
		// effect is tagged by its name
		if ( (*it)->GetTag() == effectName )
		{
			return *it;
		}
	}
	return NULL;
}
