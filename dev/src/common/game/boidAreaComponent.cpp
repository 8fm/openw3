#include "Build.h"
#include "boidAreaComponent.h"
#include "boidLairEntity.h"
#include "boidManager.h"

IMPLEMENT_ENGINE_CLASS( CBoidAreaComponent );

CBoidAreaComponent::CBoidAreaComponent()
	: m_boidAreaType( CName::NONE )
{
	m_color = Color( 97, 97, 97 );
}

void CBoidAreaComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Swarms );
}
void CBoidAreaComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Swarms );
}


void CBoidAreaComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	CAreaComponent::OnGenerateEditorFragments( frame, flag );
	
}
