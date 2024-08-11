#pragma once

#include "previewItem.h"
#include "../../common/core/set.h"
struct SCurve3DData;

RED_DECLARE_NAME( CurvePointChanged )
RED_DECLARE_NAME( CurvePointRemoved )
RED_DECLARE_NAME( CurvePointAdded )
RED_DECLARE_NAME( CurveEditionEnded )

class ShapesPreviewContainer;

class QuadPreviewItem : public IEntityPreviewItem
{
public:
	~QuadPreviewItem() { Destroy(); }
	QuadPreviewItem( IPreviewItemContainer* container, Quad* quad, const Matrix& transform = Matrix::IDENTITY );

	virtual void	SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void	SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );
	virtual void	SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	virtual Bool	IsValid() const { return true; }

	Quad*			GetShape() const { return m_quad; }
	const Matrix&	GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	Quad*					m_quad;
	const Matrix&			m_transform;
};

class PlanePreviewItem : public IEntityPreviewItem
{
public:
	~PlanePreviewItem() { Destroy(); }
	PlanePreviewItem( IPreviewItemContainer* container, Plane* plane, const Matrix& transform = Matrix::IDENTITY );

	virtual void	SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void	SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}
	virtual void	SetScaleFromPreview( const Vector& prevScale, Vector& newScale ) {}

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	virtual Bool	IsValid() const { return true; }

	Plane*			GetShape() const { return m_plane; }
	const Matrix&	GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	Plane*					m_plane;
	const Matrix&			m_transform;
};

class SegmentPreviewItem : public IEntityPreviewItem
{
public:
	~SegmentPreviewItem() { Destroy(); }
	SegmentPreviewItem( ShapesPreviewContainer* container, Segment* segment, Bool isEnd, const Matrix& transform = Matrix::IDENTITY ) : m_isEnd( isEnd ), m_segment( segment ), m_transform( transform ), m_container( container )
	{
		Init( TXT("Segment") );
		m_component->CNode::SetPosition( transform.TransformPoint( segment->m_origin + ( isEnd ? segment->m_direction : Vector::ZEROS ) ) );
		m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const;

	Segment*				GetShape() const { return m_segment; }
	const Matrix&			GetTransform() const { return m_transform; }
	Bool					GetIsEnd() const { return m_isEnd; }

protected:
	ShapesPreviewContainer*	m_container;
	Segment*				m_segment;
	const Matrix&			m_transform;
	Bool					m_isEnd;
};

struct SCurveValueChanged
{
	Float		m_prevTime;
	Float		m_currentTime;
	Float		m_prevVal;
	Float		m_currentVal;
	SCurveData*	m_curve;
	void*		m_sender;

	SCurveValueChanged( void* sender, Float prevTime, Float currentTime, Float prevVal, Float currentVal, SCurveData* curve)
		: m_prevTime( prevTime )
		, m_currentTime( currentTime )
		, m_prevVal( prevVal )
		, m_currentVal( currentVal )
		, m_curve( curve )
		, m_sender( sender )
	{
	}
};
struct SCurvePointRemoved
{
	Float		m_time;
	SCurveData*	m_curve;
	void*		m_sender;

	SCurvePointRemoved( void* sender, Float time, SCurveData* curve )
		: m_time( time )
		, m_curve( curve )
		, m_sender( sender )
	{
	}
};
struct SCurvePointAdded
{
	Float		m_time;
	SCurveData*	m_curve;
	void*		m_sender;

	SCurvePointAdded( void* sender, Float time, SCurveData* curve )
		: m_time( time )
		, m_curve( curve )
		, m_sender( sender )
	{
	}
};
struct SCurveEditionEnded
{
	SCurveData*	m_curve;
	void*		m_sender;

	SCurveEditionEnded( void* sender, SCurveData* curve )
		: m_curve( curve )
		, m_sender( sender )
	{
	}
};

class Curve3DPointControlPointPreviewItem : public IEntityPreviewItem
{
public:
	~Curve3DPointControlPointPreviewItem() { Destroy(); }
	Curve3DPointControlPointPreviewItem( IPreviewItemContainer* container, SCurve3DData* curve, const Float& time, const Int32& index, const Int32& tangentIndex, const Matrix& transform = Matrix::IDENTITY );

	virtual void	SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void	SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}
	virtual void	SetScaleFromPreview( const Vector& prevScale, Vector& newScale ) {}

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	virtual Bool	IsValid() const { return true; }

	SCurve3DData*	GetCurve() const { return m_curve; }
	Int32				GetIndex() const { return m_index; }
	Float			GetTime() const { return m_time; }
	Int32				GetTangentIndex() const { return m_tangentIndex; }
	const Matrix&	GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	SCurve3DData*	m_curve;
	Float			m_time;
	Int32				m_index;
	Int32				m_tangentIndex;
	const Matrix&	m_transform;
};
class Curve3DPointPreviewItem : public IEntityPreviewItem
{
public:
	~Curve3DPointPreviewItem() { Destroy(); }
	Curve3DPointPreviewItem( ShapesPreviewContainer* container, SCurve3DData* curve, const Float& time, const Int32& index, const Matrix& transform = Matrix::IDENTITY );

	Vector			GetValue() const;
	Float			GetTime() const { return m_time; }
	void			SetTime( Float time ) { m_time = time; }
	void			SetValue( const Vector& value );

	virtual void	SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void	SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}
	virtual void	SetScaleFromPreview( const Vector& prevScale, Vector& newScale ) {}

	virtual IPreviewItemContainer* GetItemContainer() const;

	virtual Bool	IsValid() const { return true; }

	SCurve3DData*	GetCurve() const { return m_curve; }
	Int32				GetIndex() const { return m_index; }
	void			SetIndex( Int32 index ) { m_index = index; }
	const Matrix&	GetTransform() const { return m_transform; }

protected:
	ShapesPreviewContainer*	m_container;
	SCurve3DData*	m_curve;
	Float			m_time;
	Int32				m_index;
	const Matrix&	m_transform;
};

class TetrahedronPreviewItem : public IEntityPreviewItem
{
public:
	~TetrahedronPreviewItem() { Destroy(); }
	TetrahedronPreviewItem( IPreviewItemContainer* container, Tetrahedron* tetra, const Matrix& transform = Matrix::IDENTITY ) : m_tetra( tetra ), m_transform( transform ), m_container( container )
	{
		Init( TXT("Tetrahedron") );
		m_component->CNode::SetPosition( transform.TransformPoint( tetra->GetPosition() ) );
		m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	Tetrahedron*			GetShape() const { return m_tetra; }
	const Matrix&			GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	Tetrahedron*			m_tetra;
	const Matrix&			m_transform;
};
class CutConePreviewItem : public IEntityPreviewItem
{
public:
	~CutConePreviewItem() { Destroy(); }
	CutConePreviewItem( IPreviewItemContainer* container, CutCone* cone, const Matrix& transform = Matrix::IDENTITY ) : m_cone( cone ), m_transform( transform ), m_container( container )
	{
		Init( TXT("Cylinder") );
		m_component->CNode::SetPosition( transform.TransformPoint( cone->GetPosition() ) );
		m_component->CNode::SetRotation( cone->GetOrientation() + transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	CutCone*				GetShape() const { return m_cone; }
	const Matrix&			GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	CutCone*				m_cone;
	const Matrix&			m_transform;
};

class CylinderPreviewItem : public IEntityPreviewItem
{
public:
	~CylinderPreviewItem() { Destroy(); }
	CylinderPreviewItem( IPreviewItemContainer* container, Cylinder* cylinder, const Matrix& transform = Matrix::IDENTITY ) : m_cylinder( cylinder ), m_transform( transform ), m_container( container )
	{
		Init( TXT("Cylinder") );
		m_component->CNode::SetPosition( transform.TransformPoint( cylinder->GetPosition() ) );
		m_component->CNode::SetRotation( cylinder->GetOrientation() + transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	Cylinder*				GetShape() const { return m_cylinder; }
	const Matrix&			GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	Cylinder*				m_cylinder;
	const Matrix&			m_transform;
};
class AACylinderPreviewItem : public IEntityPreviewItem
{
public:
	~AACylinderPreviewItem() { Destroy(); }
	AACylinderPreviewItem( IPreviewItemContainer* container, AACylinder* cylinder, const Matrix& transform = Matrix::IDENTITY ) : m_cylinder( cylinder ), m_transform( transform ), m_container( container )
	{
		Init( TXT("AACylinder") );
		m_component->CNode::SetPosition( transform.TransformPoint( cylinder->GetPosition() ) );
		m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) { newRot = prevRot; }
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	AACylinder*				GetShape() const { return m_cylinder; }
	const Matrix&			GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	AACylinder*				m_cylinder;
	const Matrix&			m_transform;
};
class BoxPreviewItem : public IEntityPreviewItem
{
public:
	~BoxPreviewItem() { Destroy(); }
	BoxPreviewItem( IPreviewItemContainer* container, Box* box, const Matrix& transform = Matrix::IDENTITY ) : m_box( box ), m_container( container ), m_transform( transform )
	{
		Init( TXT("Box") );
		m_component->CNode::SetPosition( transform.TransformPoint( box->Min ) );
		m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) { newRot = prevRot; }
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	Box*					GetShape() const { return m_box; }
	const Matrix&			GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	Box*					m_box;
	const Matrix&			m_transform;
};
class SpherePreviewItem : public IEntityPreviewItem
{
public:
	~SpherePreviewItem() { Destroy(); }
	SpherePreviewItem( IPreviewItemContainer* container, Sphere* sphere, const Matrix& transform = Matrix::IDENTITY ) : m_sphere( sphere ), m_transform( transform ), m_container( container )
	{
		Init( TXT("Sphere") );
		m_component->CNode::SetPosition( transform.TransformPoint( sphere->GetCenter() ) );
		m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) { newRot = prevRot; }
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	Sphere*			GetShape() const { return m_sphere; }
	const Matrix&	GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	Sphere*					m_sphere;
	const Matrix&			m_transform;
};
class FixedCapsulePreviewItem : public IEntityPreviewItem
{
public:
	~FixedCapsulePreviewItem() { Destroy(); }
	FixedCapsulePreviewItem( IPreviewItemContainer* container, FixedCapsule* capsule, const Matrix& transform = Matrix::IDENTITY ) : m_capsule( capsule ), m_transform( transform ), m_container( container )
	{
		Init( TXT("FixedCapsule") );
		m_component->CNode::SetPosition( transform.TransformPoint( capsule->GetPosition() ) );
		m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) { newRot = prevRot; }
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	FixedCapsule*			GetShape() const { return m_capsule; }
	const Matrix&			GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	FixedCapsule*			m_capsule;
	const Matrix&			m_transform;
};
class OrientedBoxPreviewItem : public IEntityPreviewItem
{
public:
	~OrientedBoxPreviewItem() { Destroy(); }
	OrientedBoxPreviewItem( IPreviewItemContainer* container, OrientedBox* box, const Matrix& transform = Matrix::IDENTITY ) : m_box( box ), m_transform( transform ), m_container( container )
	{
		Init( TXT("OrientedBox") );
		m_component->CNode::SetPosition( transform.TransformPoint( transform.TransformVector( box->m_position ) ) );
		m_component->CNode::SetRotation( box->GetOrientation() + transform.ToEulerAnglesFull() );
		m_component->CNode::SetScale( transform.GetScale33() );
	}

	virtual Bool IsValid() const { return true; }

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const { return m_container; }

	OrientedBox*			GetShape() const { return m_box; }
	const Matrix&			GetTransform() const { return m_transform; }

protected:
	IPreviewItemContainer*	m_container;
	OrientedBox*			m_box;
	const Matrix&			m_transform;
};

class ShapesPreviewContainer : public IPreviewItemContainer, public IEdEventListener, public wxEvtHandler
{
private:
	TDynArray< BoxPreviewItem* >			m_boxItems;
	TDynArray< SpherePreviewItem* >			m_sphereItems;
	TDynArray< AACylinderPreviewItem* >		m_aacylinderItems;
	TDynArray< CylinderPreviewItem* >		m_cylinderItems;
	TDynArray< FixedCapsulePreviewItem* >	m_fixedCapsuleItems;
	TDynArray< OrientedBoxPreviewItem* >	m_orientedBoxItems;
	TDynArray< CutConePreviewItem* >		m_cutConesItems;
	TDynArray< SegmentPreviewItem* >		m_segmentItems;
	TDynArray< TetrahedronPreviewItem* >	m_tetraItems;
	TDynArray< PlanePreviewItem* >			m_planeItems;
	TDynArray< QuadPreviewItem* >			m_quadItems;


	TDynArray< Curve3DPointPreviewItem* >	m_curveItems;
	TDynArray< Curve3DPointControlPointPreviewItem* >	m_curveHandles;
	
	TDynArray< SCurve3DData* >				m_curves;
	TDynArray< Matrix >						m_curveItemsTransforms;


	CWorld*									m_world;
	TSet < CObject* >						m_willHavePropertiesChanged;

	TDynArray< CObject* >					m_editedItems;
	Bool									m_canEditResources;
	TDynArray< CObject* >					m_selectedItems;

	TDynArray< Curve3DPointPreviewItem* >	GetSelectedPoints() const;
	void									ClearSelection();
public:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	ShapesPreviewContainer( CWorld *world, Bool canEditResources );
	~ShapesPreviewContainer();

	virtual CWorld* GetPreviewItemWorld() const { return m_world; }
	void			AddShape( Box* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( Sphere* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( AACylinder* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( Cylinder* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( FixedCapsule* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( OrientedBox* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( CutCone* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( Tetrahedron* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( SCurve3DData* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( Segment* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( Plane* s, const Matrix& transform = Matrix::IDENTITY );
	void			AddShape( Quad* s, const Matrix& transform = Matrix::IDENTITY );
	Bool			RemoveShape( Box* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( Sphere* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( AACylinder* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( Cylinder* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( FixedCapsule* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( OrientedBox* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( CutCone* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( Tetrahedron* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( SCurve3DData* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( Segment* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( Plane* s ); // MUST be called when given object is about to be deleted
	Bool			RemoveShape( Quad* s ); // MUST be called when given object is about to be deleted
	void			OnGenerateFragments( IViewport *view, CRenderFrame *frame ) const; // MUST be called to display visualization

	void			AddSelection( CObject* object ); // ads all shapes from given object and it's children to edition
	Uint32			RemoveSelection( CObject* object ); // removes all shapes from given object and it's children from edition

	void			AddToEdit( CObject* object ); // ads object to edit or removes it if it's already added

	Bool			GetCanEditResources() const { return m_canEditResources; }

	virtual Bool	HandleItemSelection( const TDynArray< CHitProxyObject* >& objects );
	void			AppendMenu( wxMenu* menu, wxWindow* parent );
	void			OnRemovePoint( wxCommandEvent& event );
	void			OnAddPoint( wxCommandEvent& event );
	void			OnEditCurve( wxCommandEvent& event );

	const TDynArray< Curve3DPointControlPointPreviewItem* >&	GetCurveHandles() const { return m_curveHandles; }
	const TDynArray< SegmentPreviewItem* >&						GetSegments() const { return m_segmentItems; }
};
