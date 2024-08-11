/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "brushCSG.h"
#include "brushComponent.h"
#include "brushVertex.h"
#include "brushFace.h"
#include "brushRenderData.h"

CCSGFace::CCSGFace( CBrushFace *face, Bool flag, CBrushPolygon *poly )
	: m_face( face )
	, m_polygon( poly )
	, m_flag( flag )
{
}

CCSGFace::CCSGFace( CCSGFace *face, Bool flag, CBrushPolygon *newPoly )
	: m_face( face->m_face )
	, m_polygon( newPoly )
	, m_flag( flag )
{
}

CCSGFace::~CCSGFace()
{
	// Delete brush polygon
	if ( m_polygon )
	{
		delete m_polygon;
		m_polygon = NULL;
	}
}

CCSGBrush::CCSGBrush( CBrushComponent* brush, Bool generateFaces )
	: m_brush( brush )
	, m_box( brush->GetBoundingBox() )
//	, m_clipBPS( brush->GetLocalBSP() )
	, m_generateFaces( generateFaces )
	, m_subtractive( brush->GetCSGType() == CSG_Subtractive )
	, m_detail( brush->GetCSGType() == CSG_Detail )
{
	// Extrude bounds a little
	m_box.Extrude( 0.01f );

	// Get matrices
	brush->GetLocalToWorld( m_localToWorld );
	brush->GetWorldToLocal( m_worldToLocal );

	// Generate faces
	const TDynArray< CBrushFace* >& brushFaces = brush->GetFaces();
	for ( Uint32 i=0; i<brushFaces.Size(); i++ )
	{
		CBrushFace* face = brushFaces[i];
		if ( face )
		{
			// Generate face polygons
			TDynArray< CBrushPolygon* > polygons;
			face->GeneratePolygons( polygons );

			// Create CSG faces
			for ( Uint32 j=0; j<polygons.Size(); j++ )
			{
				CBrushPolygon* poly = polygons[j];
				m_faces.PushBack( new CCSGFace( face, !m_subtractive, poly ) );
			}
		}
	}
}

CCSGBrush::~CCSGBrush()
{
	// Delete faces
	for ( Uint32 i=0; i<m_faces.Size(); i++ )
	{
		delete m_faces[i];
	}
}

void CCSGCompiler::CompileCSG( const TDynArray< CBrushComponent* > &brushes, BrushRenderData& renderData )
{
	// Generate CSG brushes
	TDynArray< CCSGBrush* > csgBrushes;
	for ( Uint32 i=0; i<brushes.Size(); i++ )
	{	
		CCSGBrush* brush = new CCSGBrush( brushes[i], true );
		csgBrushes.PushBack( brush );
	}

	// Compile
	CompileCSG( csgBrushes, true, renderData );
}

void CCSGCompiler::CompileCSG( const TDynArray< CCSGBrush* > &csgBrushes, Bool useDetailBrushes, BrushRenderData& renderData )
{
	// For all brushes
	Uint32 totalFaces = 0;
	for ( Uint32 i=0; i<csgBrushes.Size(); i++ )
	{
		CCSGBrush &clipBrush = *csgBrushes[i];
		if ( !clipBrush.m_generateFaces )
		{
			// Nothing to do
			continue;
		}

		// Get source faces
		TDynArray< CCSGFace*> inputFaces;
		inputFaces = clipBrush.m_faces;
		clipBrush.m_faces.Clear();

		// Clip to all brushes
		for ( Uint32 j=0; j<csgBrushes.Size(); j++ )
		{
			const CCSGBrush &clipToBrush = *csgBrushes[j];
			TDynArray< CCSGFace* > newFrags;

			// Pre->Post clip transistion
			if ( i == j )
			{
				// Keep only faces with positive sign info
				for ( Uint32 k=0; k<inputFaces.Size(); k++ )
				{     
					CCSGFace *face = inputFaces[ k ];
					if ( face->m_flag )
					{
						newFrags.PushBack( face );
					}
					else
					{
						delete face;
					}
				}

				// Swap arrays
				inputFaces = newFrags;
				continue;
			}

			// Check if brushes are touching
			if ( !clipToBrush.m_box.Touches( clipBrush.m_box ))
			{
				continue;
			}

			// No more faces to clip    
			if ( !inputFaces.Size() )
			{
				break;
			}

			// Setup clipping matrix
			Matrix clipToSrc = clipToBrush.m_localToWorld * clipBrush.m_worldToLocal;
			const_cast< CLocalBSP& >( clipToBrush.m_clipBPS ).Transform( clipToSrc );

			// Clip faces
			Box localClipToBox = clipToSrc.TransformBox( clipToBrush.m_box );
			for ( Uint32 k=0; k<inputFaces.Size(); k++ )
			{
				// Check bounding box
				Box faceBox = inputFaces[k]->m_polygon->CalcBox();
				//if ( faceBox.Touches( localClipToBox ) )
				{
					// Clip the face
					ClipFace( (j<i), clipBrush, clipToBrush, inputFaces[k], newFrags, useDetailBrushes );
				}
			}

			// Swap arrays
			inputFaces = newFrags;
		} // Clip to

		// If this is a subtractive brush swap final polygons
		if ( clipBrush.m_subtractive )
		{
			for ( Uint32 i=0; i<inputFaces.Size(); i++ )
			{
				CCSGFace *face = inputFaces[i];
				face->m_polygon->Swap();

				// Swap normals
				for ( Uint32 j=0; j<face->m_polygon->m_vertices.Size(); j++ )
				{
					face->m_polygon->m_vertices[j].m_normal = -face->m_polygon->m_vertices[j].m_normal;
				}
			}
		}

		// Remember fragments that survived
		totalFaces += inputFaces.Size();
		clipBrush.m_faces = inputFaces;
		inputFaces.Clear();
	}

	// Feed brushes with new render data
	Uint32 numRealBrushes = 0;
	for ( Uint32 i=0; i<csgBrushes.Size(); i++ )
	{
		const CCSGBrush &brush = *csgBrushes[i];
		if ( brush.m_generateFaces )
		{
			GenerateRenderData( brush, renderData );
			numRealBrushes++;
		}
	}

	// Delete temporary CSG brushes
	for ( Uint32 i=0; i<csgBrushes.Size(); i++ )
	{
		delete csgBrushes[i];
	}

	// Info
	LOG_ENGINE( TXT("%i vertices, %i indices in %i faces generated from %i brushes ( %i with geometry )"), renderData.m_vertices.Size(), renderData.m_indices.Size(), renderData.m_faces.Size(), csgBrushes.Size(), numRealBrushes );
}

/// Sign planar solver
class CSignPlanarSolver : public IPlanarSolver
{
private:
	Plane::ESide	m_pos;	//!< Response to positive case
	Plane::ESide	m_neg;	//!< Response to negative case

public:
	//! Constructor
	CSignPlanarSolver( Plane::ESide pos, Plane::ESide neg ) 
		: m_pos( pos )
		, m_neg( neg )
	{};

	//! Solver function
	virtual Plane::ESide Solve( const Plane& polygonPlane, const Plane &plane )
	{
		Float dot = Vector::Dot3( polygonPlane.NormalDistance, plane.NormalDistance );
		return dot > 0.0f ? m_pos : m_neg;
	}
};

void CCSGCompiler::ClipFace( Bool preClip, const CCSGBrush &srcBrush, const CCSGBrush &clipBrush, CCSGFace *face, TDynArray< CCSGFace* > &faces, Bool useDetailBrushes )
{
	// Determine brushes type
	Bool srcSub = srcBrush.m_subtractive;
	Bool clipSub = clipBrush.m_subtractive;

	// Opposite brush types (ADD-SUB or SUB-ADD)
	Bool crossClip = srcSub ^ clipSub;

	// Source faces plane
	ASSERT( face->m_polygon );
	Bool sourceSign = face->m_flag;
	Plane sourcePlane = face->m_polygon->m_plane;

	// When we are clipping brush to a brush of the same type then we can only 
	// get negative sign for face, thus if face already has negative sign there's no reason to clip it.
	if ( preClip && ( !crossClip == !face->m_flag ) )
	{
		faces.PushBack( face );
		return;
	}

	// In detail brush mode we can only clip to non detail brushes
	if ( useDetailBrushes && !srcBrush.m_detail && clipBrush.m_detail )
	{
		faces.PushBack( face );
		return;
	}

	// Initialize planar solver
	Plane::ESide pos, neg;
	if ( preClip )
	{
		pos = Plane::PS_Front;
		neg = Plane::PS_Back;
	}
	else
	{
		pos = Plane::PS_Back;
		neg = Plane::PS_Back;
	}

	// Slice input poly so we get fragments that are inside and fragments that are outside brush
	TDynArray< CBrushPolygon* > inside, outside;    
	CSignPlanarSolver solver( pos, neg );
	clipBrush.m_clipBPS.SplitPoly< CBrushPolygon >( face->m_polygon, inside, outside, &solver );

	// Process inside fragments
	for ( Uint32 i=0; i<inside.Size(); i++ )
	{
		CBrushPolygon* polygon = inside[i];

		// In post clip we delete inner fragments
		if ( !preClip )
		{
			delete polygon;
			continue;
		}

		// Create new face with inverted sign
		CCSGFace *newFace = new CCSGFace( face, !sourceSign, polygon );
		faces.PushBack( newFace );
	}

	// Fragments that are outside are not modified
	for ( Uint32 i=0; i<outside.Size(); i++ )
	{
		faces.PushBack( new CCSGFace( face, sourceSign, outside[i] ));
	}

	// Delete source face
	face->m_polygon = NULL;
	delete face;
}

class CSGFaceGrouper
{
protected:
	struct Bucket
	{
		CBrushFace*						m_sourceFace;
		TDynArray< const CCSGFace* >	m_faces;
	};

protected:
	TDynArray< Bucket* >		m_buckets;
	
public:
	~CSGFaceGrouper()
	{
		m_buckets.ClearPtr();
	}

	void AddFace( const CCSGFace& face )
	{
		if ( face.m_polygon )
		{
			CBrushFace* sourceFace = face.m_face;

			// Find existing bucket
			for ( Uint32 i=0; i<m_buckets.Size(); i++ )
			{
				Bucket* bucket = m_buckets[i];
				if ( bucket->m_sourceFace == sourceFace )
				{
					bucket->m_faces.PushBack( &face );
					return;
				}
			}

			// Add new bucket
			Bucket* bucket = new Bucket;
			bucket->m_sourceFace = sourceFace;
			bucket->m_faces.PushBack( &face );
			m_buckets.PushBack( bucket );
		}
	}

	Uint32 GetNumBuckets() const
	{
		return m_buckets.Size();
	}

	CBrushFace* GetSourceFace( Uint32 bucketIndex ) const
	{
		return m_buckets[ bucketIndex ]->m_sourceFace;
	}

	const TDynArray< const CCSGFace* >& GetFaces( Uint32 bucketIndex )
	{
		return m_buckets[ bucketIndex ]->m_faces;
	}
};

void CCSGCompiler::GenerateRenderData( const CCSGBrush &brush, BrushRenderData& renderData )
{
	// Get faces
	CBrushComponent* bc = brush.m_brush;
	const TDynArray< CBrushFace* >& brushFaces = bc->GetFaces();

	// Reset rendering data
	for ( Uint32 i=0; i<brushFaces.Size(); i++ )
	{
		CBrushFace* face = brushFaces[i];
		face->m_renderFaceID = -1;
	}

	// Group CSG faces by source Brush Face
	CSGFaceGrouper faceGrouper;
	for ( Uint32 i=0; i<brush.m_faces.Size(); i++ )
	{
		const CCSGFace& csgFace = *brush.m_faces[i];
		faceGrouper.AddFace( csgFace );
	}

	// Process face buckets
	const Uint32 numBuckets = faceGrouper.GetNumBuckets();
	for ( Uint32 i=0; i<numBuckets; i++ )
	{
		// Create render face
		CBrushFace* sourceFace = faceGrouper.GetSourceFace( i );
		sourceFace->m_renderFaceID = renderData.BeginFace( sourceFace );

		// Add polygons form CSG faces
		const TDynArray< const CCSGFace* >& csgFaces = faceGrouper.GetFaces( i );
		for ( Uint32 j=0; j<csgFaces.Size(); j++ )
		{
			// Face should have valid polygon
			const CCSGFace* face = csgFaces[j];
			ASSERT( face->m_polygon );

			// Generate vertices
			TDynArray< Uint16 > mappedVertices;
			CBrushPolygon* poly = face->m_polygon;
			for ( Uint32 k=0; k<poly->m_vertices.Size(); k++ )
			{
				const BrushVertex& v = poly->m_vertices[k];

				// Map vertex
				const Uint16 index = renderData.AddVertex( sourceFace->m_renderFaceID, v.m_position, v.m_normal, v.m_mapping.X, v.m_mapping.Y );
				mappedVertices.PushBack( index );
			}

			// Create indices
			for ( Uint32 k=2; k<poly->m_vertices.Size(); k++ )
			{
				renderData.AddIndex( sourceFace->m_renderFaceID, mappedVertices[ k ] );
				renderData.AddIndex( sourceFace->m_renderFaceID, mappedVertices[ k-1 ] );
				renderData.AddIndex( sourceFace->m_renderFaceID, mappedVertices[ 0 ] );
			}
		}

		// Update face mapping
		renderData.UpdateMapping( sourceFace->m_renderFaceID, bc->GetLocalToWorld(), sourceFace->GetMapping() );
	}
}