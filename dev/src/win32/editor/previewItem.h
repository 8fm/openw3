/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CPreviewHelperComponent;
class IPreviewItem;


class IPreviewItemContainer
{
protected:
	TDynArray< IPreviewItem* >	m_items;
	CEntity*					m_itemContainer;

public:
	virtual CWorld* GetPreviewItemWorld() const = 0;
	virtual Bool IsSelectionBoxDragging() { return false; }

public:
	virtual Bool HandleItemSelection( const TDynArray< CHitProxyObject* >& objects );

public:
	void InitItemContainer();
	void DestroyItemContainer();

	void RefreshItems();

	void SetItemPosition( const String& itemName, const Vector& newPos, const EulerAngles& rot = EulerAngles::ZEROS );

	Bool HasItem( const String& itemName ) const;
	Bool IsEmpty() const;
	void AddItem( IPreviewItem* item );
	void ClearItem( IPreviewItem* item );
	void ClearItems();

	CEntity* GetItemEntityContainer() const;

	virtual void OnItemTransformChangedFromPreview( IPreviewItem* item ) {}
	virtual void OnSelectItem( IPreviewItem* item ) {}
	virtual void OnDeselectAllItem() {}
};


//////////////////////////////////////////////////////////////////////////


class IPreviewItem
{
public:
	enum PreviewSize
	{
		PS_Tiny,
		PS_Small,
		PS_Normal
	};

public:
	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) = 0;
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) = 0;
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale ) {}

	virtual IPreviewItemContainer* GetItemContainer() const = 0;

	virtual Bool IsValid() const { return true; }
	virtual void Refresh() {}
	virtual Bool HasCaption() const { return true; }

public:
	virtual void Init( const String& name );
	virtual void Destroy();

	void SetPosition( const Vector& newPos );
	void SetRotation( const EulerAngles& newRot );
	void RefreshTransform( const Vector& newPos, const EulerAngles& newRot );
	void RefreshTransform( const EngineTransform& newTransform );

	void InternalEditor_SetPosition( const Vector& newPos );
	void InternalEditor_SetRotation( const EulerAngles& newRot );

	void SetSize( PreviewSize size );
	void SetColor( const Color& color );
	void SetVisible( Bool flag );

	const CPreviewHelperComponent* GetComponent() const;
	const CEntity* GetEntity() const;

	const String& GetName() const;

	virtual void DrawGizmo( CRenderFrame* frame ) {}
	void Select();

protected:
	CPreviewHelperComponent*	m_component;
};


//////////////////////////////////////////////////////////////////////////


class IEntityPreviewItem : public IPreviewItem
{
private:
	CEntity*					m_entity;
public:
	IEntityPreviewItem() : m_entity( NULL ) {}

	virtual void Init( const String& name );
	virtual void Destroy();

	CEntity* GetEntity() const;
};


//////////////////////////////////////////////////////////////////////////


class CPreviewHelperComponent: public CSpriteComponent
{
	DECLARE_ENGINE_CLASS( CPreviewHelperComponent, CSpriteComponent, 0 );

	const IPreviewItemContainer*	m_container;
public:
	CPreviewHelperComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual Bool IsManualCreationAllowed() const { return false; }
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	virtual Color CalcSpriteColor() const;
	virtual Float CalcSpriteSize() const;
	virtual CBitmapTexture* GetSpriteIcon() const;
	virtual Bool IsOverlay() const;

	void SetOverlay( Bool value ) { m_overlay = value; }
	void SetDrawArrows( Bool value ) { m_drawArrows = value; }

	RED_FORCE_INLINE void SetContainer( const IPreviewItemContainer* container ) { m_container = container; }
	RED_FORCE_INLINE const IPreviewItemContainer* GetContainer() const { return m_container; }

	virtual void SetPosition( const Vector& position );
	virtual void SetRotation( const EulerAngles& rotation );
	virtual void SetScale( const Vector& scale );

	void RefreshTransform( const Vector& position, const EulerAngles& rotation );
	void RefreshTransform( const EngineTransform& newTransform );

	void SetItem( IPreviewItem* item );

	Color RandColor() const;
	void SetSize( IPreviewItem::PreviewSize size );
	void SetColor( const Color& color );

	void SetManualControl( Bool flag );
	Bool HasManualControl() const;

	IPreviewItem* GetItem() const;

protected:
	IPreviewItem*				m_item;
	Color						m_color;
	Bool						m_manualControl;
	Bool						m_overlay;
	Bool						m_drawArrows;
	IPreviewItem::PreviewSize	m_size;
};

BEGIN_CLASS_RTTI( CPreviewHelperComponent );
	PARENT_CLASS( CSpriteComponent );
END_CLASS_RTTI();
