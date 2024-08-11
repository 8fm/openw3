/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "enableComponentsPerfomableAction.h"

#include "factsDB.h"
#include "../engine/drawableComponent.h"

IMPLEMENT_ENGINE_CLASS( CEnableComponentsPerformableAction )


CEnableComponentsPerformableAction::CEnableComponentsPerformableAction()
	: m_enable( true )	
{
}

void CEnableComponentsPerformableAction::PerformOnComponent( CComponent* component )
{			
	if( CDrawableComponent* drawableCmp = Cast< CDrawableComponent >( component ) )
	{
		drawableCmp->SetVisible( m_enable );
	}
	else if( CAnimatedComponent* animatedCmp = Cast< CAnimatedComponent >( component ) )
	{
		if( m_enable )
		{
			animatedCmp->Unfreeze();
		}
		else
		{
			animatedCmp->Freeze();
		}
	}
	else
	{
		if( m_enable != component->IsEnabled() )
		{
			component->SetEnabled( m_enable );
		}
	}				
}