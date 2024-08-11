/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorCluePath.h"
#include "../../common/game/behTreeInstance.h"

RED_DEFINE_NAME( clueTemplate );
RED_DEFINE_NAME( maxClues );
RED_DEFINE_NAME( cluesOffset );

CBehTreeDecoratorCluePathInstance::CBehTreeDecoratorCluePathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /* = NULL */ )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
	, m_clueTemplate( context.GetVal< THandle< CEntityTemplate > >( def.m_clueTemplate_var, def.m_clueTemplate ) )
	, m_distanceChecker( owner->GetActor(), def.m_cluesOffset.GetVal( context ) )
	, m_cluePathData( owner )
{
	m_cluePathData.TGet< CBehTreeCluePathData >()->SetMaxClues( def.m_maxClues.GetVal( context ) );
	if ( m_clueTemplate.IsValid() )
	{
		owner->AddDependentObject( Cast< IScriptable >( m_clueTemplate ) );
	}
}

void CBehTreeDecoratorCluePathInstance::OnDestruction()
{
	if ( m_clueTemplate.IsValid() )
	{
		m_owner->RemoveDependentObject( Cast< IScriptable >( m_clueTemplate ) );
	}
	Super::OnDestruction();
}

void CBehTreeDecoratorCluePathInstance::Update()
{
	if ( ShouldLeaveClue() )
	{
		LeaveClue();
	}
	return Super::Update();
}

Bool CBehTreeDecoratorCluePathInstance::ShouldLeaveClue()
{
	if ( !m_clueTemplate )
	{
		return false;
	}
	return m_distanceChecker.ShouldUpdate();
}

void CBehTreeDecoratorCluePathInstance::LeaveClue()
{
	if ( !m_distanceChecker.WasFirstUpdate() )
	{
		Vector diff = m_distanceChecker.GetMovementDelta();
		diff.Z = 0.0f;
		EulerAngles rotation = diff.ToEulerAngles();		
		m_cluePathData.TGet< CBehTreeCluePathData >()->LeaveClue( m_clueTemplate, GetOwner()->GetActor()->GetLayer(), m_distanceChecker.GetUpdatePosition(), rotation );
	}
}

//////////////////////////////////////////////////////////////////////////

CBehTreeDecoratorCluePathDefinition::CBehTreeDecoratorCluePathDefinition()
	: m_clueTemplate( nullptr )
	, m_clueTemplate_var( CNAME( clueTemplate ) )
	, m_maxClues( CNAME( maxClues ), 5 )
	, m_cluesOffset( CNAME( cluesOffset ), 2.0f )
{
}

IBehTreeNodeDecoratorInstance* CBehTreeDecoratorCluePathDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}
