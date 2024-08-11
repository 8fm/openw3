/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "pivot.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/bitmapTexture.h"

CGatheredResource resPivotIcon( TXT("engine\\textures\\icons\\vertexicon.xbm"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CPivotEditorEntity );
IMPLEMENT_ENGINE_CLASS( CPivotComponent );

void CPivotEditorEntity::OnUpdateTransformEntity()
{
	TBaseClass::OnUpdateTransformEntity();

	m_oldPosition = GetPosition();
}

void CPivotEditorEntity::OnPropertyPostChange( IProperty* property )
{
	// Propagate to base class
	TBaseClass::OnPropertyPostChange( property );

	// Position changed
	if ( property->GetName() == TXT("transform") )
	{
		if ( m_owner )
		{
			Vector newPosition = GetPosition();
			m_owner->OnEditorNodeMoved( m_index, m_oldPosition, GetPosition(), newPosition );
			m_transform.SetPosition( newPosition );
		}

		m_oldPosition = GetPosition();
	}
}

void CPivotEditorEntity::SetPosition( const Vector& position )
{
	TBaseClass::SetPosition( position );

	// Inform owner
	if ( m_owner )
	{
		Vector newPosition = GetPosition();
		m_owner->OnEditorNodeMoved( m_index, m_oldPosition, GetPosition(), newPosition );
		m_transform.SetPosition( newPosition );
	}

	m_oldPosition = GetPosition();
}

CBitmapTexture* CPivotComponent::GetSpriteIcon() const
{
	return resPivotIcon.LoadAndGet< CBitmapTexture >();
}

Color CPivotComponent::CalcSpriteColor() const
{
	CPivotEditorEntity * vertex = Cast< CPivotEditorEntity >( GetParent() );
	if ( vertex && vertex->m_hovered )
	{
		return Color::YELLOW;
	}
	else if ( IsSelected() )
	{
		return Color::GREEN;
	}
	else
	{
		return Color::WHITE;
	}
}
