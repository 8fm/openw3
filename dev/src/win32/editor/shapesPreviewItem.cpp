#include "build.h"
#include "shapesPreviewItem.h"
#include "../../common/core/math.h"
#include "../../common/core/curveData.h"
#include "../../common/core/curve3DData.h"
#include "../../common/engine/hitProxyObject.h"
#include "curvePropertyEditor.h"
#include "curveEditorWindow.h"
#include "propertyRecurrentBrowser.h"
#include "../../common/engine/renderFrame.h"

#define ID_REMOVE_POINT		3141
#define ID_ADD_POINT		3142
#define ID_EDIT_CURVE		3143

#define HANDLE_SCALE		0.1f
#define HANDLE_SCALE_REVERT	(1.0f/HANDLE_SCALE)

RED_DEFINE_NAME( CurvePointChanged )
RED_DEFINE_NAME( CurvePointRemoved )
RED_DEFINE_NAME( CurvePointAdded )
RED_DEFINE_NAME( CurveEditionEnded )

void QuadPreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	EulerAngles rot = newRot - prevRot;
	for ( Uint32 i = 1; i < 4; ++i )
	{
		m_quad->m_points[i] = rot.TransformPoint( m_quad->m_points[i] - m_quad->m_points[0] ) + m_quad->m_points[0];
	}
}
void QuadPreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_component->CNode::SetRotation( EulerAngles::ZEROS );
	for ( Uint32 i = 1; i < 4; ++i )
	{
		Vector v = m_quad->m_points[i] - m_quad->m_points[0];
		m_quad->m_points[i].X = v.X * newScale.X / prevScale.X;
		m_quad->m_points[i].Y = v.Y * newScale.Y / prevScale.Y;
		m_quad->m_points[i].Z = v.Z * newScale.Z / prevScale.Z;
		m_quad->m_points[i] += m_quad->m_points[0];
	}
}

QuadPreviewItem::QuadPreviewItem( IPreviewItemContainer* container, Quad* quad, const Matrix& transform/* = Matrix::IDENTITY*/ )
	: m_container( container )
	, m_quad( quad )
	, m_transform( transform )
{
	Init( TXT("Quad") );
	m_component->CNode::SetPosition( transform.TransformPoint( quad->GetPosition() ) );
	//m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
	m_component->CNode::SetScale( transform.GetScale33() );
}

void QuadPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Vector v = newPos - prevPos;
	*m_quad += v;
}

PlanePreviewItem::PlanePreviewItem( IPreviewItemContainer* container, Plane* plane, const Matrix& transform/* = Matrix::IDENTITY*/ )
	: m_container( container )
	, m_plane( plane )
	, m_transform( transform )
{
	Init( TXT("Plane") );
	m_component->CNode::SetPosition( transform.TransformPoint( plane->Project( plane->GetVectorRepresentation() ) ) );
	m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
	m_component->CNode::SetScale( transform.GetScale33() );
}

void PlanePreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Vector delta = newPos - prevPos;
	Float sum = delta.X * m_plane->GetVectorRepresentation().X + delta.Y * m_plane->GetVectorRepresentation().Y + delta.Z * m_plane->GetVectorRepresentation().Z;
	m_plane->SetPlane( m_plane->GetVectorRepresentation() + Vector( 0, 0, 0, - sum ) );
	sum = 0;
	InternalEditor_SetPosition( m_plane->GetVectorRepresentation() );
}

IPreviewItemContainer* SegmentPreviewItem::GetItemContainer() const { return m_container; }

void SegmentPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	if ( m_isEnd )
	{
		m_segment->m_direction += delta;
	}
	else
	{
		(*m_segment) += delta;
		const TDynArray< SegmentPreviewItem* >& segments = m_container->GetSegments();
		for ( Uint32 i = 0; i < segments.Size(); ++i )
		{
			if ( segments[i]->GetShape() == m_segment && this != segments[i] )
			{
				segments[i]->InternalEditor_SetPosition( GetComponent()->GetWorldPosition() + m_transform.TransformVector( m_segment->m_direction ) );
			}
		}
	}
}
void SegmentPreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	if ( !m_isEnd )
	{
		m_segment->m_direction.X *= newScale.X / prevScale.X;
		m_segment->m_direction.Y *= newScale.Y / prevScale.Y;
		m_segment->m_direction.Z *= newScale.Z / prevScale.Z;
	}
}
void SegmentPreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	if ( !m_isEnd )
	{
		EulerAngles rot = newRot - prevRot;
		m_segment->m_direction = rot.TransformVector( m_segment->m_direction );
	}
}

Curve3DPointControlPointPreviewItem::Curve3DPointControlPointPreviewItem( IPreviewItemContainer* container, SCurve3DData* curve, const Float& time, const Int32& index, const Int32& tangentIndex, const Matrix& transform /*= Matrix::IDENTITY*/ )
	: m_curve( curve )
	, m_container( container )
	, m_time( time )
	, m_index( index )
	, m_tangentIndex( tangentIndex )
	, m_transform( transform )
{
	Init( TXT("Curve Handle") );
	Vector pos = m_curve->GetValue( m_time );
	Vector control = m_curve->GetControlPoint( m_time, m_tangentIndex);
	//control.Normalize3();
	control *= HANDLE_SCALE;
	m_component->CNode::SetPosition( transform.TransformPoint( pos + control ) );
	m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
	m_component->CNode::SetScale( transform.GetScale33() );
}
void Curve3DPointControlPointPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector val = m_curve->GetValue( m_time );
	Vector newTransformed = inverted.TransformPoint( newPos );
	Vector dif = newTransformed - val;
	dif *= HANDLE_SCALE_REVERT;
	m_curve->SetControlPoint( m_time, dif, m_tangentIndex );
	Vector control =  m_curve->GetControlPoint( m_time, m_tangentIndex);
	Vector pos = control * HANDLE_SCALE  + val;
	ASSERT( !_isnan( pos.X ) || pos.X == NumericLimits<Float>::Infinity() || pos.X == -NumericLimits<Float>::Infinity() );
	ASSERT( !_isnan( pos.Y ) || pos.Y == NumericLimits<Float>::Infinity() || pos.Y == -NumericLimits<Float>::Infinity() );
	ASSERT( !_isnan( pos.Z ) || pos.Z == NumericLimits<Float>::Infinity() || pos.Z == -NumericLimits<Float>::Infinity() );
	InternalEditor_SetPosition( pos );
}
IPreviewItemContainer* Curve3DPointPreviewItem::GetItemContainer() const { return m_container; }

void Curve3DPointPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector pos = inverted.TransformPoint( newPos );
	SetValue( pos );
	Vector delta = newPos - prevPos;
	const TDynArray< Curve3DPointControlPointPreviewItem* >& handles = m_container->GetCurveHandles();
	for ( Uint32 i = 0; i < handles.Size(); ++i )
	{
		Curve3DPointControlPointPreviewItem* handle = handles[i];
		if ( handle->GetCurve() == m_curve && handle->GetIndex() == m_index )
		{
			handle->InternalEditor_SetPosition( handle->GetComponent()->GetWorldPosition() + delta );
		}
	}
}
Curve3DPointPreviewItem::Curve3DPointPreviewItem( ShapesPreviewContainer* container, SCurve3DData* curve, const Float& time, const Int32& index, const Matrix& transform /*= Matrix::IDENTITY*/ )
	: m_curve( curve )
	, m_container( container )
	, m_time( time )
	, m_index( index )
	, m_transform( transform )
{
	Init( TXT("3D Curve") );
	m_component->CNode::SetPosition( transform.TransformPoint( m_curve->GetValue( m_time ) ) );
	m_component->CNode::SetRotation( transform.ToEulerAnglesFull() );
	m_component->CNode::SetScale( transform.GetScale33() );
}
void Curve3DPointPreviewItem::SetValue( const Vector& value )
{
	for ( Uint32 i = 0; i < 3; ++i )
	{
		Int32 index = m_curve->v[i].GetIndex( m_time );
		if ( index != -1 )
		{
			SCurveValueChanged eventData( this, m_time, m_time, m_curve->v[i].GetFloatValueAtIndex(index), value.A[i], &m_curve->v[i] );
			SEvents::GetInstance().DispatchEvent( CNAME( CurvePointChanged ), CreateEventData( eventData ) );
		}
	}
	m_curve->SetValue( m_time, value );
}
Vector Curve3DPointPreviewItem::GetValue() const
{
	return m_curve->GetValue( m_time );
}

template< typename T >
void AddInstancesToSelection( CObject* obj, ShapesPreviewContainer& container )
{
	TDynArray< const CObject* > parents;
	TDynArray< void* > instances;
	TDynArray< const CClass* > filterOut;
	if ( !container.GetCanEditResources() )
	{
		filterOut.PushBack( CResource::GetStaticClass() );
	}
	SPropertyRecurrentBrowserConf conf( T::GetStaticClass(), &parents, &instances, &filterOut );
	CPropertyRecurrentBrowser::Find( obj, conf );
	for ( Uint32 i = 0; i < parents.Size(); ++i )
	{
		CNode* parent = parents[i]->FindParent<CNode>();
		container.AddShape( ( T* )instances[i], parent ? parent->GetLocalToWorld() : Matrix::IDENTITY );
	}
}
template< typename T >
Uint32 RemoveInstancesToSelection( CObject* obj, ShapesPreviewContainer& container )
{
	TDynArray< const CObject* > parents;
	TDynArray< void* > instances;
	TDynArray< const CClass* > filterOut;
	if ( !container.GetCanEditResources() )
	{
		filterOut.PushBack( CResource::GetStaticClass() );
	}
	SPropertyRecurrentBrowserConf conf( T::GetStaticClass(), &parents, &instances, &filterOut );
	CPropertyRecurrentBrowser::Find( obj, conf );
	Uint32 result = 0;
	for ( Uint32 i = 0; i < parents.Size(); ++i )
	{
		result += container.RemoveShape( ( T* )instances[i] ) ? 1 : 0;
	}
	return result;
}

void ShapesPreviewContainer::AddSelection( CObject* object )
{
	if ( object )
	{
		AddInstancesToSelection< Box >( object, *this );
		AddInstancesToSelection< Sphere >( object, *this );
		AddInstancesToSelection< AACylinder >( object, *this );
		AddInstancesToSelection< Cylinder >( object, *this );
		AddInstancesToSelection< FixedCapsule >( object, *this );
		AddInstancesToSelection< OrientedBox >( object, *this );
		AddInstancesToSelection< CutCone >( object, *this );
		AddInstancesToSelection< Tetrahedron >( object, *this );
		AddInstancesToSelection< SCurve3DData >( object, *this );
		AddInstancesToSelection< Segment >( object, *this );
		AddInstancesToSelection< Plane >( object, *this );
		AddInstancesToSelection< Quad >( object, *this );
	}
}

Uint32 ShapesPreviewContainer::RemoveSelection( CObject* object )
{
	Uint32 removed = 0;
	if ( object )
	{
		removed += RemoveInstancesToSelection< Box >( object, *this );
		removed += RemoveInstancesToSelection< Sphere >( object, *this );
		removed += RemoveInstancesToSelection< AACylinder >( object, *this );
		removed += RemoveInstancesToSelection< Cylinder >( object, *this );
		removed += RemoveInstancesToSelection< FixedCapsule >( object, *this );
		removed += RemoveInstancesToSelection< OrientedBox >( object, *this );
		removed += RemoveInstancesToSelection< CutCone >( object, *this );
		removed += RemoveInstancesToSelection< Tetrahedron >( object, *this );
		removed += RemoveInstancesToSelection< SCurve3DData >( object, *this );
		removed += RemoveInstancesToSelection< Segment >( object, *this );
		removed += RemoveInstancesToSelection< Plane >( object, *this );
		removed += RemoveInstancesToSelection< Quad >( object, *this );
	}
	return removed;
}

ShapesPreviewContainer::ShapesPreviewContainer( CWorld *world, Bool canEditResources )
	: m_world( world )
	, m_canEditResources( canEditResources )
{
	InitItemContainer();

	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPreChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurvePointChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurvePointRemoved ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurvePointAdded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurveEditionEnded ), this );
}

ShapesPreviewContainer::~ShapesPreviewContainer()
{
	SEvents::GetInstance().UnregisterListener( this );

	DestroyItemContainer();

	m_boxItems.ClearPtr();
	m_sphereItems.ClearPtr();
	m_aacylinderItems.ClearPtr();
	m_cylinderItems.ClearPtr();
	m_fixedCapsuleItems.ClearPtr();
	m_orientedBoxItems.ClearPtr();
}

void ShapesPreviewContainer::OnAddPoint( wxCommandEvent& event )
{
	TDynArray< Curve3DPointPreviewItem* > points = GetSelectedPoints();
	for ( Uint32 i = 0; i < points.Size(); ++i )
	{
		TDynArray< Float > frames;
		SCurve3DData* curve = points[i]->GetCurve();
		curve->GetKeyframes( frames );
		Float time = points[i]->GetTime();
		Int32 index = frames.GetIndex( time );
		if ( index >= 0 )
		{
			Vector value;
			if ( index + 1 < (Int32)frames.Size() )
			{
				time += ( frames[ index + 1 ] - frames[ index ] ) * 0.5f;
				value = curve->GetValue( time );
			}
			else
			{
				value = curve->GetValue( time );
				if ( index > 0 )
				{
					Vector d = ( value - curve->GetValue( frames[ index - 1 ] ) );
					d.Normalize3();
					value += d;
				}
				else
				{
					value += Vector( 1.0f, 1.0f, 1.0f );
				}
				time += 1.0f;
			}
			curve->AddPoint( time, value );
			for ( Uint32 j = 0; j < 3; ++j )
			{
				SCurvePointAdded eventData( this, time, &curve->v[j] );
				SEvents::GetInstance().DispatchEvent( CNAME( CurvePointAdded ), CreateEventData( eventData ) );
			}
			for ( Uint32 j = 0; j < m_curveItems.Size(); ++j )
			{
				if ( m_curveItems[j]->GetCurve() == curve && m_curveItems[j]->GetIndex() > index )
				{
					m_curveItems[j]->SetIndex( m_curveItems[j]->GetIndex() + 1 );
				}
			}
			m_curveItems.PushBack( new Curve3DPointPreviewItem( this, points[i]->GetCurve(), time, index + 1, points[i]->GetTransform() ) );
		}
	}
}

void ShapesPreviewContainer::OnEditCurve( wxCommandEvent& event )
{
	TDynArray< Curve3DPointPreviewItem* > points = GetSelectedPoints();
	if ( points.Size() && event.GetEventObject())
	{
		TDynArray< SCurveData* > curves;
		curves.PushBack( &points[0]->GetCurve()->v[0] );
		curves.PushBack( &points[0]->GetCurve()->v[1] );
		curves.PushBack( &points[0]->GetCurve()->v[2] );
		wxWindow* window = wxDynamicCast( event.GetEventObject(), wxWindow );
		new CEdCurveEditorFrame( window, curves );
	}
}
void ShapesPreviewContainer::OnRemovePoint( wxCommandEvent& event )
{
	TDynArray< Curve3DPointPreviewItem* > points = GetSelectedPoints();
	for ( Uint32 i = 0; i < points.Size(); ++i )
	{
		Int32 index = points[i]->GetIndex();
		SCurve3DData* curve = points[i]->GetCurve();
		Float time = points[i]->GetTime();
		for ( Uint32 j = 0; j < m_curveItems.Size(); ++j )
		{
			if ( m_curveItems[j]->GetCurve() == curve && m_curveItems[j]->GetIndex() > index )
			{
				m_curveItems[j]->SetIndex( m_curveItems[j]->GetIndex() - 1 );
			}
		}
		for ( Uint32 j = 0; j < 3; ++j )
		{
			Int32 index = curve->v[j].GetIndex( time );
			if ( index != -1 )
			{
				SCurvePointRemoved eventData( this, time, &curve->v[j] );
				SEvents::GetInstance().DispatchEvent( CNAME( CurvePointRemoved ), CreateEventData( eventData ) );
			}
		}
		curve->RemovePoint( time );
		m_curveItems.Remove( points[i] );
		points[i]->Destroy();
	}
}
void ShapesPreviewContainer::AppendMenu( wxMenu* menu, wxWindow* parent )
{
	if ( GetSelectedPoints().Size() > 0 )
	{
		menu->Append( ID_REMOVE_POINT, TXT("Remove point") );
		menu->Connect( ID_REMOVE_POINT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ShapesPreviewContainer::OnRemovePoint ), NULL, this );
		menu->Append( ID_ADD_POINT, TXT("Add point") );
		menu->Connect( ID_ADD_POINT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ShapesPreviewContainer::OnAddPoint ), NULL, this );
		menu->Append( ID_EDIT_CURVE, TXT("Edit curve") );
		menu->Connect( ID_EDIT_CURVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ShapesPreviewContainer::OnEditCurve ), NULL, this );
	}
}

Bool ShapesPreviewContainer::HandleItemSelection( const TDynArray< CHitProxyObject* >& objects )
{
	m_selectedItems.ClearFast();
	for ( Uint32 i = 0; i < objects.Size(); ++i )
	{
		m_selectedItems.PushBack( objects[i]->GetHitObject() );
	}

	TDynArray< Curve3DPointPreviewItem* > selected = GetSelectedPoints();
	TDynArray< Curve3DPointPreviewItem* > selectedPointsThatDontHaveHandlers;
	TDynArray< Curve3DPointControlPointPreviewItem* > handlersNeeded;
	for ( Uint32 i = 0; i < selected.Size(); ++i )
	{
		Curve3DPointPreviewItem* point = selected[i];
		Bool hasHandlers = false;
		for ( Uint32 j = 0; j < m_curveHandles.Size(); ++j )
		{
			if ( m_curveHandles[j]->GetCurve() == point->GetCurve() && m_curveHandles[j]->GetIndex() == point->GetIndex() )
			{
				handlersNeeded.PushBackUnique( m_curveHandles[j] );
				hasHandlers = true;
				break;
			}
		}
		if ( !hasHandlers )
		{
			selectedPointsThatDontHaveHandlers.PushBack( point );
		}
	}
	if ( selected.Size() > 0 )
	{
		for ( Uint32 i = 0; i < m_curveHandles.Size(); ++i )
		{
			Bool isNeeded = false;
			Curve3DPointControlPointPreviewItem* handle = m_curveHandles[i];

			for ( Uint32 j = 0; j < selected.Size(); ++j )
			{
				if ( selected[j]->GetCurve() == handle->GetCurve() && selected[j]->GetIndex() == handle->GetIndex() )
				{
					isNeeded = true;
					break;
				}
			}
			if ( !isNeeded )
			{
				m_curveHandles.Erase( m_curveHandles.Begin() + i );
				handle->Destroy();
				--i;
			}
		}
	}
	for ( Uint32 i = 0; i < selectedPointsThatDontHaveHandlers.Size(); ++i )
	{
		m_curveHandles.PushBack( new Curve3DPointControlPointPreviewItem( this, selectedPointsThatDontHaveHandlers[i]->GetCurve(), selectedPointsThatDontHaveHandlers[i]->GetTime(), selectedPointsThatDontHaveHandlers[i]->GetIndex(), 0, selectedPointsThatDontHaveHandlers[i]->GetTransform() ) );
		m_curveHandles.PushBack( new Curve3DPointControlPointPreviewItem( this, selectedPointsThatDontHaveHandlers[i]->GetCurve(), selectedPointsThatDontHaveHandlers[i]->GetTime(), selectedPointsThatDontHaveHandlers[i]->GetIndex(), 1, selectedPointsThatDontHaveHandlers[i]->GetTransform() ) );
	}
	return IPreviewItemContainer::HandleItemSelection( objects );
}

void ShapesPreviewContainer::ClearSelection()
{
	m_selectedItems.ClearFast();
	m_curveHandles.ClearPtr();
}
TDynArray< Curve3DPointPreviewItem* > ShapesPreviewContainer::GetSelectedPoints() const
{
	TSet< const CPreviewHelperComponent* > selected;
	TDynArray< Curve3DPointPreviewItem* > result;
	for ( Uint32 i = 0; i < m_selectedItems.Size(); ++i )
	{
		CPreviewHelperComponent* item = Cast< CPreviewHelperComponent >( m_selectedItems[i] );
		if ( item )
		{
			selected.Insert( item );
		}
	}
	for ( Uint32 i = 0; i < m_curveItems.Size(); ++i )
	{
		const CPreviewHelperComponent* comp = m_curveItems[i]->GetComponent();
		if ( selected.Exist( comp ) )
		{
			result.PushBack( m_curveItems[i] );
		}
	}
	return result;
}
void ShapesPreviewContainer::AddToEdit( CObject* object )
{
	TDynArray< CObject* >::iterator it = m_editedItems.FindPtr( object );
	if ( it == NULL )
	{
		m_editedItems.PushBack( object );
		AddSelection( object );
	}
	else
	{
		m_editedItems.Erase( it );
		RemoveSelection( object );
	}
}
void ShapesPreviewContainer::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPropertyPreChange ) || name == CNAME( EditorPropertyPostChange ) )
	{
		typedef CEdPropertiesPage::SPropertyEventData SEventData;
		const SEventData& eventData = GetEventData< SEventData >( data );

		CObject* object = eventData.m_typedObject.AsObject();

		if ( name == CNAME( EditorPropertyPreChange ) && m_editedItems.Exist( object ) == true )
		{
			if ( RemoveSelection( object ) )
			{
				m_willHavePropertiesChanged.Insert( object );
			}
		}
		else if ( name == CNAME( EditorPropertyPostChange ) && m_editedItems.Exist( object ) == true  )
		{
			if ( m_willHavePropertiesChanged.Exist( object ) )
			{
				AddSelection( object );
				m_willHavePropertiesChanged.Erase( object );
			}
		}
	}
	else if ( name == CNAME( CurvePointChanged )  )
	{
		const SCurveValueChanged& eventData = GetEventData< SCurveValueChanged >( data );
		if ( eventData.m_sender == this || m_curveItems.FindPtr( (Curve3DPointPreviewItem*)eventData.m_sender ) ) return;
		for ( Uint32 i = 0; i < m_curveItems.Size(); ++i )
		{
			SCurve3DData* curve = m_curveItems[i]->GetCurve();
			if ( curve->Contains( eventData.m_curve ) )
			{
				TDynArray< Float > frames;
				curve->GetKeyframes( frames );
				Int32 prevIndex = LowerBoundIndex( frames.Begin(), frames.End(), eventData.m_prevTime ) - frames.Begin();
				Int32 actualIndex = LowerBoundIndex( frames.Begin(), frames.End(), eventData.m_currentTime ) - frames.Begin();
				Float min = Min( eventData.m_prevTime, eventData.m_currentTime );
				Float max = Max( eventData.m_prevTime, eventData.m_currentTime );
				Int32 diff = eventData.m_currentTime - eventData.m_prevTime > 0.0f ? -1 : 1;
				Int32 changedIndex = -1;
				Int32 previousWithIndex = -1;
				for ( Uint32 j = 0; j < m_curveItems.Size(); ++j )
				{
					SCurve3DData* curve2 = m_curveItems[j]->GetCurve();
					if ( curve == curve2)
					{
						if ( m_curveItems[j]->GetTime() == eventData.m_currentTime && m_curveItems[j]->GetTime() != eventData.m_prevTime )
						{
							previousWithIndex = j;
						}
						if ( m_curveItems[j]->GetTime() > min && m_curveItems[j]->GetTime() < max )
						{
							m_curveItems[j]->SetIndex( m_curveItems[j]->GetIndex() + diff );
						}
						if ( m_curveItems[j]->GetTime() == eventData.m_prevTime )
						{
							changedIndex = j;
						}
						m_curveItems[j]->InternalEditor_SetPosition( curve->GetValue( m_curveItems[j]->GetTime() ) );
					}
				}
				ASSERT( changedIndex != -1 );
				diff = 0;
				if ( previousWithIndex != -1 && !( prevIndex != -1 && frames[ prevIndex ] == eventData.m_prevTime ) )
				{
					Curve3DPointPreviewItem* item = *( m_curveItems.Begin() + changedIndex );
					m_curveItems.Erase( m_curveItems.Begin() + changedIndex );
					diff = -1;
					delete item;
				}
				if ( prevIndex != -1 && frames[ prevIndex ] == eventData.m_prevTime && eventData.m_currentTime != eventData.m_prevTime && previousWithIndex == -1 )
				{
					m_curveItems.PushBack( new Curve3DPointPreviewItem( this, curve, eventData.m_prevTime, prevIndex ) );
					diff = 1;
				}
				if ( changedIndex != -1 && previousWithIndex == -1 )
				{
					m_curveItems[changedIndex]->SetIndex( actualIndex );
					m_curveItems[changedIndex]->SetTime( eventData.m_currentTime );
					m_curveItems[changedIndex]->InternalEditor_SetPosition( curve->GetValue( m_curveItems[changedIndex]->GetTime() ) );
				}
				if ( diff != 0 )
				{
					for ( Uint32 j = 0; j < m_curveItems.Size(); ++j )
					{
						SCurve3DData* curve2 = m_curveItems[j]->GetCurve();
						if ( curve == curve2)
						{
							if ( m_curveItems[j]->GetTime() > max )
							{
								m_curveItems[j]->SetIndex( m_curveItems[j]->GetIndex() + diff );
							}
						}
					}
				}
				ClearSelection();
				break;
			}
		}
	}
	else if ( name == CNAME( CurvePointRemoved )  )
	{
		const SCurvePointRemoved& eventData = GetEventData< SCurvePointRemoved >( data );
		if ( eventData.m_sender == this || m_curveItems.FindPtr( (Curve3DPointPreviewItem*)eventData.m_sender ) ) return;
		for ( Uint32 i = 0; i < m_curveItems.Size(); ++i )
		{
			SCurve3DData* curve = m_curveItems[i]->GetCurve();
			if ( eventData.m_time == m_curveItems[i]->GetTime() && curve->Contains( eventData.m_curve ) )
			{
				TDynArray< Float > frames;
				curve->GetKeyframes( frames );
				if ( !frames.Exist( eventData.m_time ) )
				{
					Curve3DPointPreviewItem* item = *( m_curveItems.Begin() + i );
					m_curveItems.Erase( m_curveItems.Begin() + i );
					delete item;
					for ( Uint32 j = 0; j < m_curveItems.Size(); ++j )
					{
						SCurve3DData* curve2 = m_curveItems[j]->GetCurve();
						if ( curve == curve2)
						{
							if ( m_curveItems[j]->GetTime() > eventData.m_time )
							{
								m_curveItems[j]->SetIndex( m_curveItems[j]->GetIndex() - 1 );
							}
						}
					}
				}
				else
				{
					m_curveItems[i]->InternalEditor_SetPosition( curve->GetValue( eventData.m_time ) );
				}
				ClearSelection();
				break;
			}
		}
	}
	else if ( name == CNAME( CurvePointAdded )  )
	{
		const SCurvePointAdded& eventData = GetEventData< SCurvePointAdded >( data );
		if ( eventData.m_sender == this || m_curveItems.FindPtr( (Curve3DPointPreviewItem*)eventData.m_sender ) ) return;
		for ( Uint32 i = 0; i < m_curveItems.Size(); ++i )
		{
			SCurve3DData* curve = m_curveItems[i]->GetCurve();
			if ( curve->Contains( eventData.m_curve ) )
			{
				TDynArray< Float > frames;
				curve->GetKeyframes( frames );
				ASSERT( frames.FindPtr( eventData.m_time ) );
				m_curveItems.PushBack( new Curve3DPointPreviewItem( this, curve, eventData.m_time, frames.FindPtr( eventData.m_time ) - frames.Begin() ) );
				for ( Uint32 j = 0; j < m_curveItems.Size(); ++j )
				{
					SCurve3DData* curve2 = m_curveItems[j]->GetCurve();
					if ( curve == curve2)
					{
						if ( m_curveItems[j]->GetTime() > eventData.m_time )
						{
							m_curveItems[j]->SetIndex( m_curveItems[j]->GetIndex() + 1 );
						}
					}
				}
				ClearSelection();
				break;
			}
		}
	}
	else if ( name == CNAME( CurveEditionEnded ) )
	{
		const SCurveEditionEnded& eventData = GetEventData< SCurveEditionEnded >( data );
		for ( Uint32 i = 0; i < m_curveItems.Size(); ++i )
		{
			SCurve3DData* curve = m_curveItems[i]->GetCurve();
			if ( curve->Contains( eventData.m_curve ) )
			{
				m_curveItems[i]->SetValue( curve->GetValue( m_curveItems[i]->GetTime() ) );
			}
		}
		ClearSelection();
	}
	
}
void ShapesPreviewContainer::OnGenerateFragments( IViewport *view, CRenderFrame *frame ) const
{
	for ( TDynArray< QuadPreviewItem* >::const_iterator it = m_quadItems.Begin(); it != m_quadItems.End(); ++it )
	{
		Vector p[4];
		for ( Uint32 i = 0; i < 4; ++i )
		{
			p[i] = (*it)->GetTransform().TransformPoint( (*it)->GetShape()->m_points[i] );
		}
		frame->AddDebugLine( p[0], p[1], Color( 255, 255, 255 ), false );
		frame->AddDebugLine( p[1], p[2], Color( 255, 255, 255 ), false );
		frame->AddDebugLine( p[2], p[3], Color( 255, 255, 255 ), false );
		frame->AddDebugLine( p[3], p[0], Color( 255, 255, 255 ), false );
	}
	for ( TDynArray< PlanePreviewItem* >::const_iterator it = m_planeItems.Begin(); it != m_planeItems.End(); ++it )
	{
		frame->AddDebugPlane( *(*it)->GetShape(), (*it)->GetTransform(), Color( 255, 255, 255 ), 10, false );
	}
	for ( TDynArray< SegmentPreviewItem* >::const_iterator it = m_segmentItems.Begin(); it != m_segmentItems.End(); ++it )
	{
		if ( !(*it)->GetIsEnd() ) frame->AddDebugLine( (*it)->GetTransform().TransformPoint( (*it)->GetShape()->m_origin ), (*it)->GetTransform().TransformPoint( (*it)->GetShape()->m_origin + (*it)->GetShape()->m_direction ), Color( 255, 255, 255 ), false );
	}
	for ( TDynArray< Curve3DPointControlPointPreviewItem* >::const_iterator it = m_curveHandles.Begin(); it != m_curveHandles.End(); ++it )
	{
		frame->AddDebugLine( (*it)->GetTransform().TransformPoint( (*it)->GetCurve()->GetValue( (*it)->GetTime() ) ), (*it)->GetComponent()->GetWorldPosition(), Color( 255, 255, 255 ), false );
	}
	for ( TDynArray< SCurve3DData* >::const_iterator it = m_curves.Begin(); it != m_curves.End(); ++it )
	{
		SCurve3DData* b = *it;
		frame->AddDebug3DCurve( *b, Color::CYAN, m_curveItemsTransforms[(Int32)(it - m_curves.Begin())], 12, false );
	}
	for ( TDynArray< BoxPreviewItem* >::const_iterator it = m_boxItems.Begin(); it != m_boxItems.End(); ++it )
	{
		BoxPreviewItem* b = *it;
		frame->AddDebugBox( *(b->GetShape()), b->GetTransform(), Color::GREEN, false );
	}
	for ( TDynArray< SpherePreviewItem* >::const_iterator it = m_sphereItems.Begin(); it != m_sphereItems.End(); ++it )
	{
		SpherePreviewItem* b = *it;
		frame->AddDebugSphere( b->GetShape()->GetCenter(), b->GetShape()->GetRadius(), b->GetTransform(), Color::GREEN, false );
	}
	for ( TDynArray< AACylinderPreviewItem* >::const_iterator it = m_aacylinderItems.Begin(); it != m_aacylinderItems.End(); ++it )
	{
		AACylinderPreviewItem* b = *it;
		frame->AddDebugWireframeTube( b->GetShape()->GetPosition(), b->GetShape()->GetPosition() + Vector( 0, 0, b->GetShape()->GetHeight() ), b->GetShape()->GetRadius(), b->GetShape()->GetRadius(), b->GetTransform(), Color::GREEN, Color::GREEN );
	}
	for ( TDynArray< CylinderPreviewItem* >::const_iterator it = m_cylinderItems.Begin(); it != m_cylinderItems.End(); ++it )
	{
		CylinderPreviewItem* b = *it;
		frame->AddDebugWireframeTube( b->GetShape()->GetPosition(), b->GetShape()->GetPosition2(), b->GetShape()->GetRadius(), b->GetShape()->GetRadius(), b->GetTransform(), Color::GREEN, Color::GREEN );
	}
	for ( TDynArray< FixedCapsulePreviewItem* >::const_iterator it = m_fixedCapsuleItems.Begin(); it != m_fixedCapsuleItems.End(); ++it )
	{
		FixedCapsulePreviewItem* b = *it;
		frame->AddDebugCapsule( *b->GetShape(), b->GetTransform(), Color::GREEN, false );
	}
	for ( TDynArray< OrientedBoxPreviewItem* >::const_iterator it = m_orientedBoxItems.Begin(); it != m_orientedBoxItems.End(); ++it )
	{
		OrientedBoxPreviewItem* b = *it;
		frame->AddDebugOrientedBox( *b->GetShape(), b->GetTransform(), Color::GREEN, false );
	}
	for ( TDynArray< CutConePreviewItem* >::const_iterator it = m_cutConesItems.Begin(); it != m_cutConesItems.End(); ++it )
	{
		CutConePreviewItem* b = *it;
		frame->AddDebugWireframeTube( b->GetShape()->GetPosition(), b->GetShape()->GetPosition2(), b->GetShape()->GetRadius1(), b->GetShape()->GetRadius2(), b->GetTransform(), Color::RED, Color::RED );
	}
	for ( TDynArray< TetrahedronPreviewItem* >::const_iterator it = m_tetraItems.Begin(); it != m_tetraItems.End(); ++it )
	{
		TetrahedronPreviewItem* b = *it;
		frame->AddDebugTetrahedron( *b->GetShape(), b->GetTransform(), Color::RED );
	}
}
void ShapesPreviewContainer::AddShape( AACylinder* s, const Matrix& transform )
{
	for ( TDynArray< AACylinderPreviewItem* >::iterator it = m_aacylinderItems.Begin(); it != m_aacylinderItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_aacylinderItems.PushBack( new AACylinderPreviewItem( this, s, transform ) );
}
void ShapesPreviewContainer::AddShape( SCurve3DData* s, const Matrix& transform )
{
	for ( TDynArray< SCurve3DData* >::iterator it = m_curves.Begin(); it != m_curves.End(); ++it )
	{
		if ( (*it) == s )
		{
			return;
		}
	}
	m_curves.PushBack( s );
	m_curveItemsTransforms.PushBack( transform );
	TDynArray< Float > frames;
	s->GetKeyframes( frames );
	for ( TDynArray< Float >::iterator it = frames.Begin(); it != frames.End(); ++it )
	{
		s->SetValue( frames[*it], s->GetValue( frames[*it] ) );
		m_curveItems.PushBack( new Curve3DPointPreviewItem( this, s, *it, frames.GetIndex( it ), m_curveItemsTransforms[ m_curveItemsTransforms.Size() - 1 ] ) );
	}
}
void ShapesPreviewContainer::AddShape( Cylinder* s, const Matrix& transform )
{
	for ( TDynArray<CylinderPreviewItem* >::iterator it = m_cylinderItems.Begin(); it != m_cylinderItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_cylinderItems.PushBack( new CylinderPreviewItem( this, s, transform ) );
}
void ShapesPreviewContainer::AddShape( Box* box, const Matrix& transform )
{
	for ( TDynArray<BoxPreviewItem* >::iterator it = m_boxItems.Begin(); it != m_boxItems.End(); ++it )
	{
		if ( (*it)->GetShape() == box )
		{
			return;
		}
	}
	m_boxItems.PushBack( new BoxPreviewItem( this, box, transform ) );
}
void ShapesPreviewContainer::AddShape( FixedCapsule* s, const Matrix& transform )
{
	for ( TDynArray<FixedCapsulePreviewItem* >::iterator it = m_fixedCapsuleItems.Begin(); it != m_fixedCapsuleItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_fixedCapsuleItems.PushBack( new FixedCapsulePreviewItem( this, s, transform ) );
}
void ShapesPreviewContainer::AddShape( OrientedBox* s, const Matrix& transform )
{
	for ( TDynArray<OrientedBoxPreviewItem* >::iterator it = m_orientedBoxItems.Begin(); it != m_orientedBoxItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_orientedBoxItems.PushBack( new OrientedBoxPreviewItem( this, s, transform ) );
}
void ShapesPreviewContainer::AddShape( CutCone* s, const Matrix& transform )
{
	for ( TDynArray<CutConePreviewItem* >::iterator it = m_cutConesItems.Begin(); it != m_cutConesItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_cutConesItems.PushBack( new CutConePreviewItem( this, s, transform ) );
}
void ShapesPreviewContainer::AddShape( Tetrahedron* s, const Matrix& transform )
{
	for ( TDynArray<TetrahedronPreviewItem* >::iterator it = m_tetraItems.Begin(); it != m_tetraItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_tetraItems.PushBack( new TetrahedronPreviewItem( this, s, transform ) );
}
void ShapesPreviewContainer::AddShape( Segment* s, const Matrix& transform )
{
	for ( TDynArray<SegmentPreviewItem* >::iterator it = m_segmentItems.Begin(); it != m_segmentItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_segmentItems.PushBack( new SegmentPreviewItem( this, s, true, transform ) );
	m_segmentItems.PushBack( new SegmentPreviewItem( this, s, false, transform ) );
}
void ShapesPreviewContainer::AddShape( Plane* s, const Matrix& transform )
{
	for ( TDynArray<PlanePreviewItem* >::iterator it = m_planeItems.Begin(); it != m_planeItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_planeItems.PushBack( new PlanePreviewItem( this, s, transform ) );
}
void ShapesPreviewContainer::AddShape( Quad* s, const Matrix& transform )
{
	for ( TDynArray<QuadPreviewItem* >::iterator it = m_quadItems.Begin(); it != m_quadItems.End(); ++it )
	{
		if ( (*it)->GetShape() == s )
		{
			return;
		}
	}
	m_quadItems.PushBack( new QuadPreviewItem( this, s, transform ) );
}
Bool ShapesPreviewContainer::RemoveShape( Quad* p )
{
	for ( TDynArray< QuadPreviewItem* >::iterator it = m_quadItems.Begin(); it != m_quadItems.End(); ++it )
	{
		if ( p == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_quadItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( Plane* p )
{
	for ( TDynArray< PlanePreviewItem* >::iterator it = m_planeItems.Begin(); it != m_planeItems.End(); ++it )
	{
		if ( p == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_planeItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( OrientedBox* p )
{
	for ( TDynArray< OrientedBoxPreviewItem* >::iterator it = m_orientedBoxItems.Begin(); it != m_orientedBoxItems.End(); ++it )
	{
		if ( p == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_orientedBoxItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( Segment* p )
{
	Bool result = false;
	for ( TDynArray< SegmentPreviewItem* >::iterator it = m_segmentItems.Begin(); it != m_segmentItems.End(); ++it )
	{
		if ( p == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_segmentItems.Erase( it );
			result = true;
		}
	}
	return result;
}
Bool ShapesPreviewContainer::RemoveShape( FixedCapsule* p )
{
	for ( TDynArray< FixedCapsulePreviewItem* >::iterator it = m_fixedCapsuleItems.Begin(); it != m_fixedCapsuleItems.End(); ++it )
	{
		if ( p == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_fixedCapsuleItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( SCurve3DData* p )
{
	for ( TDynArray< SCurve3DData* >::iterator it = m_curves.Begin(); it != m_curves.End(); ++it )
	{
		if ( p == (*it) )
		{
			for ( Uint32 i = 0; i < m_curveItems.Size(); ++i )
			{
				if ( m_curveItems[i]->GetCurve() == p )
				{
					m_curveItems[i]->Destroy();
					m_curveItems.Erase( m_curveItems.Begin() + i );
					m_curveItemsTransforms.Erase( m_curveItemsTransforms.Begin() + i );
					--i;
				}
			}
			m_curves.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( AACylinder* p )
{
	for ( TDynArray< AACylinderPreviewItem* >::iterator it = m_aacylinderItems.Begin(); it != m_aacylinderItems.End(); ++it )
	{
		if ( p == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_aacylinderItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( Cylinder* p )
{
	for ( TDynArray< CylinderPreviewItem* >::iterator it = m_cylinderItems.Begin(); it != m_cylinderItems.End(); ++it )
	{
		if ( p == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_cylinderItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( Box* box )
{
	for ( TDynArray< BoxPreviewItem* >::iterator it = m_boxItems.Begin(); it != m_boxItems.End(); ++it )
	{
		if ( box == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_boxItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( CutCone* s )
{
	for ( TDynArray< CutConePreviewItem* >::iterator it = m_cutConesItems.Begin(); it != m_cutConesItems.End(); ++it )
	{
		if ( s == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_cutConesItems.Erase( it );
			return true;
		}
	}
	return false;
}
void ShapesPreviewContainer::AddShape( Sphere* s, const Matrix& transform )
{
	m_sphereItems.PushBack( new SpherePreviewItem( this, s, transform ) );
}
Bool ShapesPreviewContainer::RemoveShape( Sphere* s )
{
	for ( TDynArray< SpherePreviewItem* >::iterator it = m_sphereItems.Begin(); it != m_sphereItems.End(); ++it )
	{
		if ( s == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_sphereItems.Erase( it );
			return true;
		}
	}
	return false;
}
Bool ShapesPreviewContainer::RemoveShape( Tetrahedron* s )
{
	for ( TDynArray< TetrahedronPreviewItem* >::iterator it = m_tetraItems.Begin(); it != m_tetraItems.End(); ++it )
	{
		if ( s == (*it)->GetShape() )
		{
			(*it)->Destroy();
			m_tetraItems.Erase( it );
			return true;
		}
	}
	return false;
}

void SpherePreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	(*m_sphere) += delta;
}
void SpherePreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	Float s = newScale.X / prevScale.X * newScale.Y / prevScale.Y * newScale.Z / prevScale.Z;
	m_sphere->CenterRadius2.W *= s;
}
void FixedCapsulePreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	(*m_capsule) += delta;
}
void FixedCapsulePreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_capsule->Height *= newScale.Z / prevScale.Z;
	m_capsule->PointRadius.W *= newScale.X / prevScale.X * newScale.Y / prevScale.Y;
}
void BoxPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) 
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	(*m_box) += delta;
}
void BoxPreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_box->Max.X *= newScale.X / prevScale.X;
	m_box->Max.Y *= newScale.Y / prevScale.Y;
	m_box->Max.Z *= newScale.Z / prevScale.Z;
}
void AACylinderPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) 
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	(*m_cylinder) += delta;
}
void AACylinderPreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_cylinder->m_height *= newScale.Z / prevScale.Z;
	m_cylinder->m_positionAndRadius.W *= newScale.X / prevScale.X * newScale.Y / prevScale.Y;
}
void CylinderPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	(*m_cylinder) += delta;
}
void CylinderPreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_cylinder->m_normalAndHeight.W*= newScale.Z / prevScale.Z;
	m_cylinder->m_positionAndRadius.W *= newScale.X / prevScale.X * newScale.Y / prevScale.Y;
}
void CylinderPreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	EulerAngles rot = newRot - prevRot;
	Float h = m_cylinder->m_normalAndHeight.W;
	m_cylinder->m_normalAndHeight = rot.TransformVector( Vector( 0, 0, 1 ) );
	m_cylinder->m_normalAndHeight.W = h;
}
void OrientedBoxPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	(*m_box) += delta;
}
void OrientedBoxPreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	EulerAngles rot = newRot - prevRot;
	Float e1 = m_box->m_edge1.W;
	Float e2 = m_box->m_edge2.W;
	m_box->m_edge1 = rot.TransformVector( m_box->m_edge1 );
	m_box->m_edge2 = rot.TransformVector( m_box->m_edge2 );
	m_box->m_edge1.W = e1;
	m_box->m_edge2.W = e2;
}
void OrientedBoxPreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_box->m_edge2.W *= newScale.X / prevScale.X;
	m_box->m_edge1.W *= newScale.Y / prevScale.Y;
	m_box->m_position.W *= newScale.Z / prevScale.Z;
}

void CutConePreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	delta.W = 0;
	(*m_cone) += delta;
}
void CutConePreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_cone->m_height *= newScale.Z / prevScale.Z;
	m_cone->m_positionAndRadius1.W *= newScale.X / prevScale.X * newScale.Y / prevScale.Y;
	m_cone->m_normalAndRadius2.W *= newScale.X / prevScale.X * newScale.Y / prevScale.Y;
}
void CutConePreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	EulerAngles rot = newRot - prevRot;
	Float t = m_cone->m_normalAndRadius2.W;
	m_cone->m_normalAndRadius2 = rot.TransformVector(  Vector( 0, 0, 1 ) );
	m_cone->m_normalAndRadius2.W = t;
}


void TetrahedronPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	Matrix inverted = m_transform.Inverted();
	Vector delta = inverted.TransformPoint( newPos - prevPos );
	m_tetra->m_points[0] += delta;
	m_tetra->m_points[1] += delta;
	m_tetra->m_points[2] += delta;
	m_tetra->m_points[3] += delta;
}
void TetrahedronPreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	EulerAngles rot = newRot - prevRot;
	Vector d;

	d = m_tetra->m_points[0] - m_tetra->GetPosition();
	m_tetra->m_points[0] = m_tetra->GetPosition() + rot.TransformVector( d );

	d = m_tetra->m_points[1] - m_tetra->GetPosition();
	m_tetra->m_points[1] = m_tetra->GetPosition() + rot.TransformVector( d );

	d = m_tetra->m_points[2] - m_tetra->GetPosition();
	m_tetra->m_points[2] = m_tetra->GetPosition() + rot.TransformVector( d );

	d = m_tetra->m_points[3] - m_tetra->GetPosition();
	m_tetra->m_points[3] = m_tetra->GetPosition() + rot.TransformVector( d );
}
void TetrahedronPreviewItem::SetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	Vector mult( newScale.X / prevScale.X, newScale.Y / prevScale.Y, newScale.Z / prevScale.Z );
	Vector d;

	d = m_tetra->m_points[0] - m_tetra->GetPosition();
	m_tetra->m_points[0] = m_tetra->GetPosition() + Vector( d.X * mult.X, d.Y * mult.Y, d.Z * mult.Z );

	d = m_tetra->m_points[1] - m_tetra->GetPosition();
	m_tetra->m_points[1] = m_tetra->GetPosition() + Vector( d.X * mult.X, d.Y * mult.Y, d.Z * mult.Z );

	d = m_tetra->m_points[2] - m_tetra->GetPosition();
	m_tetra->m_points[2] = m_tetra->GetPosition() + Vector( d.X * mult.X, d.Y * mult.Y, d.Z * mult.Z );

	d = m_tetra->m_points[3] - m_tetra->GetPosition();
	m_tetra->m_points[3] = m_tetra->GetPosition() + Vector( d.X * mult.X, d.Y * mult.Y, d.Z * mult.Z );
}
