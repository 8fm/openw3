/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CVertexEditorEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CVertexEditorEntity, CEntity, 0 );

public:
	IEditorNodeMovementHook*	m_owner;
	Uint32						m_index;
	Bool						m_hovered;
	Vector						m_oldPosition;
	EulerAngles					m_oldRotation;
	Vector						m_oldScale;
	Bool						m_rotatable;
	Bool						m_scalable;
	Bool						m_drawSmallBox;

	CVertexEditorEntity() : m_owner( NULL ), m_hovered( false ), m_rotatable( false ), m_scalable( false ), m_drawSmallBox( false ) {}

	virtual void OnPropertyPostChange( IProperty* property ) override;

	virtual void OnUpdateTransformEntity() override;

	virtual void EditorOnTransformChangeStart();
	virtual void SetPosition( const Vector& position ) override;
	virtual void SetRotation( const EulerAngles& rotation ) override;
	virtual void SetScale( const Vector& scale ) override;
	virtual void EditorOnTransformChangeStop();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;
};

DEFINE_SIMPLE_RTTI_CLASS( CVertexEditorEntity, CEntity );

class CVertexComponent : public CSpriteComponent
{
	DECLARE_ENGINE_CLASS( CVertexComponent, CSpriteComponent, 0 );

protected:
	virtual CBitmapTexture* GetSpriteIcon() const;

	virtual Color CalcSpriteColor() const;

	// Check if object can be created manually in the editor
	virtual Bool IsManualCreationAllowed() const { return false; }

	virtual Bool IsOverlay() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CVertexComponent, CSpriteComponent );
