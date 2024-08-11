/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "cutsceneEffectsPropertyEditors.h"
#include "cutsceneTimeline.h"
#include "../../common/engine/extAnimCutsceneEffectEvent.h"
#include "../../common/engine/fxDefinition.h"

void CEdCutsceneEffectPropertyEditor::FillChoices()
{	
	CExtAnimCutsceneEffectEvent* effect = m_propertyItem->GetParent()->GetParentObject( 0 ).As< CExtAnimCutsceneEffectEvent >();
	
	TDynArray< CFXDefinition* > effects;

	if ( CEntityTemplate* entityTempl = effect->LoadAndGetEntityTemplate() )
	{
		entityTempl->GetAllEffects( effects );
	}
	else
	{
		CCutsceneTemplate* cutscene = CEdCutsceneTimeline::GetEditedCutscene();
		effects = cutscene->GetEffects();
	}

	m_ctrlChoice->AppendString( TXT("") );

	for ( auto effectIter = effects.Begin(); effectIter != effects.End(); ++effectIter )
	{
		CFXDefinition* effect = *effectIter;

		m_ctrlChoice->AppendString( effect->GetName().AsString().AsChar() );
	}
}

void CEdCutsceneActorEffectPropertyEditor::FillChoices()
{	
	CCutsceneTemplate* cutscene = CEdCutsceneTimeline::GetEditedCutscene();
	String actor = CEdCutsceneTimeline::GetCurrentEntity().AsString().StringBefore( TXT( ":" ) );
	SCutsceneActorDef* actorDef = cutscene->GetActorDefinition( actor );
	ASSERT( actorDef != NULL );

	TDynArray< CFXDefinition* > effects;
	actorDef->GetEntityTemplate()->GetAllEffects( effects );

	effects.PushBackUnique( cutscene->GetEffects() );

	m_ctrlChoice->AppendString( TXT("") );

	for ( auto effectIter = effects.Begin(); effectIter != effects.End(); ++effectIter )
	{
		CFXDefinition* effect = *effectIter;

		m_ctrlChoice->AppendString( effect->GetName().AsString().AsChar() );
	}
}
