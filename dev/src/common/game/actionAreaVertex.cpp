/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actionAreaVertex.h"

#include "../../common/core/gatheredResource.h"
#include "../engine/renderFrame.h"
#include "../engine/bitmapTexture.h"

CGatheredResource resVertexIcon( TXT("engine\\textures\\icons\\vertexicon.xbm"), RGF_NotCooked );

#ifndef NO_EDITOR
Bool GVertexEditorEntityOverlay = true;
#endif

IMPLEMENT_ENGINE_CLASS( CVertexEditorEntity );
IMPLEMENT_ENGINE_CLASS( CVertexComponent );

void CVertexEditorEntity::OnUpdateTransformEntity()
{
	TBaseClass::OnUpdateTransformEntity();

	m_oldPosition = GetPosition();
	m_oldRotation = GetRotation();
	m_oldScale    = GetScale();
}

void CVertexEditorEntity::OnPropertyPostChange( IProperty* property )
{
	// Propagate to base class
	TBaseClass::OnPropertyPostChange( property );

#ifndef NO_EDITOR
	// Position changed
	if ( property->GetName() == TXT("transform") )
	{
		if ( m_owner )
		{
			Vector newPosition = GetPosition();
			if ( newPosition != m_oldPosition )
			{
				m_owner->OnEditorNodeMoved( m_index, m_oldPosition, GetPosition(), newPosition );
				m_transform.SetPosition( newPosition );
			}

			EulerAngles newRotation = GetRotation();
			if ( m_rotatable && newRotation != m_oldRotation )
			{
				m_owner->OnEditorNodeRotated( m_index, m_oldRotation, GetRotation(), newRotation );
				m_transform.SetRotation( newRotation );
			}

			Vector newScale = GetScale();
			if ( m_scalable && newScale != m_oldScale )
			{
				m_owner->OnEditorNodeScaled( m_index, m_oldScale, GetScale(), newScale );
				m_transform.SetScale( newScale );
			}
		}
		m_oldPosition = GetPosition();
		m_oldRotation = GetRotation();
		m_oldScale    = GetScale();
	}
#endif
}

void CVertexEditorEntity::SetPosition( const Vector& position )
{
	TBaseClass::SetPosition( position );

	// Inform owner
	Vector newPosition = GetPosition();
	if ( m_owner && newPosition != m_oldPosition )
	{
		m_owner->OnEditorNodeMoved( m_index, m_oldPosition, GetPosition(), newPosition );
		m_transform.SetPosition( newPosition );
	}

	m_oldPosition = GetPosition();
}

void CVertexEditorEntity::SetRotation( const EulerAngles& rotation )
{
	TBaseClass::SetRotation( rotation );
	
	// Inform owner
	EulerAngles newRotation = GetRotation();
	if ( m_owner && m_rotatable && newRotation != m_oldRotation )
	{
		m_owner->OnEditorNodeRotated( m_index, m_oldRotation, GetRotation(), newRotation );
		m_transform.SetRotation( newRotation );
	}

	m_oldRotation = GetRotation();
}

void CVertexEditorEntity::SetScale( const Vector& scale )
{
	//uniform scale only
	const Vector& oldScale = GetScale();
	Float s = scale.Y;
	if (scale.X!=oldScale.X)
		s = scale.X;
	else if (scale.Z!=oldScale.Z)
		s = scale.Z;

	TBaseClass::SetScale( Vector(s,s,s) );

	// Inform owner
	if ( m_owner )
	{
		Vector newPosition = GetPosition();
		if ( newPosition != m_oldPosition )
		{
			m_owner->OnEditorNodeMoved( m_index, m_oldPosition, GetPosition(), newPosition );
			m_transform.SetPosition( newPosition );
		}

		EulerAngles newRotation = GetRotation();
		if ( m_rotatable && newRotation != m_oldRotation )
		{
			m_owner->OnEditorNodeRotated( m_index, m_oldRotation, GetRotation(), newRotation );
			m_transform.SetRotation( newRotation );
		}

		Vector newScale = GetScale();
		if ( m_scalable && newScale != m_oldScale )
		{
			m_owner->OnEditorNodeScaled( m_index, m_oldScale, GetScale(), newScale );
			m_transform.SetScale( newScale );
		}
	}

	m_oldScale = GetScale();
}

void CVertexEditorEntity::EditorOnTransformChangeStart()
{
	if ( m_owner )
	{
		m_owner->OnEditorNodeTransformStart( m_index );
	}
}

void CVertexEditorEntity::EditorOnTransformChangeStop()
{
	if ( m_owner )
	{
		m_owner->OnEditorNodeTransformStop( m_index );
	}
}

void CVertexEditorEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sprites );
}

void CVertexEditorEntity::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Sprites );
}

void CVertexEditorEntity::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

 	if ( m_drawSmallBox )
 	{
 		Box box( Vector::ZERO_3D_POINT, 0.25f );
 		frame->AddDebugBox( box, GetLocalToWorld(), Color::LIGHT_YELLOW, true );
 	}
}

CBitmapTexture* CVertexComponent::GetSpriteIcon() const
{
	return resVertexIcon.LoadAndGet< CBitmapTexture >();
}

Color CVertexComponent::CalcSpriteColor() const
{
	CVertexEditorEntity * vertex = Cast< CVertexEditorEntity >( GetParent() );
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

Bool CVertexComponent::IsOverlay() const
{
#ifndef NO_EDITOR
	return GVertexEditorEntityOverlay;
#else
	return true;
#endif
}
