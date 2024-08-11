/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibSpecialZoneMap.h"

#include "areaComponent.h"
#include "pathlibBinTree.h"

namespace PathLib
{

typedef BinTree::TBinTreeNode< Uint16 > BinTreeNode;

CSpecialZonesMap::CSpecialZonesMap()
{

}
CSpecialZonesMap::~CSpecialZonesMap()
{

}

void CSpecialZonesMap::Collect( CAreaComponent* component, NodeFlags clearFlags, NodeFlags forceFlags )
{
	ASSERT( m_binTree.Empty() );

	// add new zone
	m_zones.PushBack( Zone() );
	Zone& zone = m_zones.Back();

	// set it up
	zone.m_bbox = component->GetBoundingBox();

	zone.m_clearFlags = clearFlags;
	zone.m_forceFlags = forceFlags;

	const auto& worldPoints = component->GetWorldPoints();
	zone.m_verts.Resize( worldPoints.Size() );

	for ( Uint32 i = 0, n = worldPoints.Size(); i < n; ++i )
	{
		zone.m_verts[ i ] = worldPoints[ i ].AsVector2();
	}
	
}

void CSpecialZonesMap::FinalizeCollection()
{
	struct RectInput : Red::System::NonCopyable
	{
		typedef Uint16 ElementType;

		RectInput( TDynArray< Uint8 >& dataBuffer )
			: m_dataBuffer( dataBuffer )											{}

		struct InputData
		{
			Uint16									m_element;
			Bool									m_ending;
			Vector2									m_bboxMin;
			Vector2									m_bboxMax;
		};
		TDynArray< InputData >					m_data;
		TDynArray< InputData >::iterator		m_itBegin;
		TDynArray< InputData >::iterator		m_itEnd;
		TDynArray< Uint8 >&						m_dataBuffer;

		enum { MIN_NODE_ELEMENTS = 4 };

		void SortInput( Uint32 dimm )
		{
			struct Comperator
			{
				Comperator( Uint32 dimm )
					: m_dimm( dimm ) {}
				RED_INLINE Bool operator()( const InputData& n1, const InputData& n2 ) const
				{
					Float f1 = n1.m_ending ? n1.m_bboxMax.A[ m_dimm ] : n1.m_bboxMin.A[ m_dimm ];
					Float f2 = n2.m_ending ? n2.m_bboxMax.A[ m_dimm ] : n2.m_bboxMin.A[ m_dimm ];
					if ( f1 == f2 )
						return n1.m_ending > n2.m_ending;
					return f1 < f2;
				}
				Uint32				m_dimm;
			};

			::Sort( m_itBegin, m_itEnd, Comperator( dimm ) );
		}

		RED_INLINE const Vector2& GetElementPosition( const InputData& d )
		{
			return d.m_ending ? d.m_bboxMax : d.m_bboxMin;
		}
	};

	m_binTree.ClearFast();

	RectInput input( m_binTree );
	input.m_data.Resize( m_zones.Size() * 2 );
	for( Uint16 i = 0, n = Uint16(m_zones.Size()); i < m_zones.Size(); ++i )
	{
		Zone& zone = m_zones[ i ];
		const Box& bbox = zone.m_bbox;
		RectInput::InputData zoneData;
		zoneData.m_element = i;
		zoneData.m_ending = false;
		zoneData.m_bboxMin = bbox.Min;
		zoneData.m_bboxMax = bbox.Max;
		input.m_data[ (Uint32(i) << 1U) + 0 ] = zoneData;
		zoneData.m_ending = true;
		input.m_data[ (Uint32(i) << 1U) + 1 ] = zoneData;

	}
	input.m_itBegin = input.m_data.Begin();
	input.m_itEnd = input.m_data.End();

	BinTree::ComputeBinTree( input );
}

Bool CSpecialZonesMap::QueryPosition( const Vector3& position, NodeFlags& clearFlags, NodeFlags& forceFlags )
{
	struct Functor
	{
		Vector3				m_position;
		NodeFlags			m_clearFlags;
		NodeFlags			m_forceFlags;
		Bool				m_retVal;
		const ZoneList&		m_zones;
		
		static Uint16 InvalidElement() { return 0xffff; }

		Functor( const Vector3& position, const ZoneList& zoneList )
			: m_position( position )
			, m_clearFlags( 0 )
			, m_forceFlags( 0 )
			, m_retVal( false )
			, m_zones( zoneList )
			
		{}

		RED_INLINE Bool operator()( Uint16 zoneId )
		{
			const Zone& zone = m_zones[ zoneId ];
			if ( !zone.m_bbox.Contains( m_position ) )
			{
				return false;
			}
			if ( !MathUtils::GeometryUtils::IsPointInPolygon2D( zone.m_verts, m_position.AsVector2() ) )
			{
				return false;
			}

			m_retVal = true;
			m_clearFlags |= zone.m_clearFlags;
			m_forceFlags |= zone.m_forceFlags;

			return false;
		}
		
	} functor( position, m_zones );
	
	const BinTreeNode* rootNode = reinterpret_cast< const BinTreeNode* >( m_binTree.Data() );
	BinTree::FindBinTreeElement( rootNode, position.AsVector2(), functor );

	clearFlags = functor.m_clearFlags;
	forceFlags = functor.m_forceFlags;

	return functor.m_retVal;
}
Bool CSpecialZonesMap::QueryLine( const Vector3& pos0, const Vector3& pos1, NodeFlags& clearFlags, NodeFlags& forceFlags )
{
	struct Functor
	{
		Vector3				m_pos0;
		Vector3				m_pos1;
		Box					m_testBox;
		NodeFlags			m_clearFlags;
		NodeFlags			m_forceFlags;
		Bool				m_retVal;
		const ZoneList&		m_zones;

		static Uint16 InvalidElement() { return 0xffff; }

		Functor( const Vector3& pos0, const Vector3& pos1, const ZoneList& zoneList )
			: m_pos0( pos0 )
			, m_pos1( pos1 )
			, m_testBox( Box::RESET_STATE )
			, m_clearFlags( 0 )
			, m_forceFlags( 0 )
			, m_retVal( false )
			, m_zones( zoneList )
		{
			m_testBox.AddPoint( m_pos0 );
			m_testBox.AddPoint( m_pos1 );
		}

		RED_INLINE void operator()( Uint16 zoneId )
		{
			const Zone& zone = m_zones[ zoneId ];
			if ( !zone.m_bbox.Touches( m_testBox ) )
			{
				return;
			}
			if ( !MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( zone.m_verts, m_pos0.AsVector2(), m_pos1.AsVector2() ) )
			{
				return;
			}

			m_retVal = true;
			m_clearFlags |= zone.m_clearFlags;
			m_forceFlags |= zone.m_forceFlags;

			return;
		}

	} functor( pos0, pos1, m_zones );

	const BinTreeNode* rootNode = reinterpret_cast< const BinTreeNode* >( m_binTree.Data() );
	BinTree::IterateElementsInBoundings( rootNode, functor.m_testBox.Min.AsVector2(), functor.m_testBox.Max.AsVector2(), functor );

	clearFlags = functor.m_clearFlags;
	forceFlags = functor.m_forceFlags;

	return functor.m_retVal;
}

};			// namespace PathLib