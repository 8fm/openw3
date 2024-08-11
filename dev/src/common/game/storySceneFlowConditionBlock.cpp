#include "build.h"
#include "storySceneFlowConditionBlock.h"
#include "storySceneFlowCondition.h"
#include "storySceneGraphSocket.h"
#include "storyScene.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/graphConnectionRebuilder.h"

CGatheredResource resExpresionList( TXT("gameplay\\globals\\scenes\\scene_flow_conditions.csv"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CStorySceneFlowConditionBlock )

void CStorySceneFlowConditionBlock::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneFlowConditionBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_condition )
	{
		// Every control flow has an input and two outputs - true and false
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( In ), m_condition, LSD_Input ) );
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( False ), m_condition->GetFalseLink(), LSD_Output ) );
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( True ), m_condition->GetTrueLink(), LSD_Output ) );
	}
}

EGraphBlockShape CStorySceneFlowConditionBlock::GetBlockShape() const
{
	return GBS_Triangle;
}

Color CStorySceneFlowConditionBlock::GetClientColor() const
{
	return Color( 76, 130, 191 );
}

String CStorySceneFlowConditionBlock::GetCaption() const
{
	return m_description;
}

#endif

void CStorySceneFlowConditionBlock::SetCondition( CStorySceneFlowCondition* condition )
{
	ASSERT(!m_condition);
	ASSERT(condition);

	m_condition = condition;
}

void CStorySceneFlowConditionBlock::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	// as there is only one property with 2da value selection editor, it is ok to ignore the property parameter.
	valueProperties.m_array = resExpresionList.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Id");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneFlowConditionBlock::OnPropertyPostChange( IProperty* property )
{
}

void CStorySceneFlowConditionBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	if ( m_condition != NULL )
	{
		//m_condition->SetParent( this );
	}
}

void CStorySceneFlowConditionBlock::OnDestroyed()
{
	if ( m_condition != NULL )
	{
		m_condition->GetScene()->RemoveControlPart( m_condition );
	}
}
#endif