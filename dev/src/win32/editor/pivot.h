/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CPivotEditorEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CPivotEditorEntity, CEntity, 0 );

public:
	IEditorNodeMovementHook*	m_owner;
	Uint32						m_index;
	Bool						m_hovered;
	Vector						m_oldPosition;

	CPivotEditorEntity() : m_owner( NULL ), m_hovered( false ) {}

	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnUpdateTransformEntity() override;

	virtual void SetPosition( const Vector& position );
};

DEFINE_SIMPLE_RTTI_CLASS( CPivotEditorEntity, CEntity );

class CPivotComponent : public CSpriteComponent
{
	DECLARE_ENGINE_CLASS( CPivotComponent, CSpriteComponent, 0 );

protected:
	virtual CBitmapTexture* GetSpriteIcon() const;

	virtual Color CalcSpriteColor() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CPivotComponent, CSpriteComponent );
