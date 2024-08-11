#include "build.h"
#include "areaConvex.h"
#include "areaShapeBuilder.h"
#include "polygonTriangulation.h"
#include "areaComponent.h"

const Float ClipDistTreshold = 0.01f;

//---------------------------------------------------------------------------

namespace
{
	static Bool IsStillOnPlane(const Plane& triPlane, const Vector& d)
	{
		// Next point is not on triangle plane
		const Float dist = triPlane.DistanceTo(d);
		if (Abs(dist) > 0.01f) return false;

		// Good
		return true;
	}

	template < class T >
	static void CleanupList(T*& listPtr)
	{
		T* cur = listPtr;
		while (cur != NULL)
		{
			T* next = cur->m_next;
			delete cur;
			cur = next;
		}
	}
}

//---------------------------------------------------------------------------

Bool CAreaShapeBuilder::Outline::CompileConvexList(Convex*& convexList) const
{
	// calculate general up vector for the whole shape
	const Vector upVector = m_localToShape.TransformVector(Vector(0,0,m_height));

	// create input polygon for triangulation
	CPartitionPoly inputPoly;
	inputPoly.Init(m_points.Size());
	for (Uint32 i=0; i<m_points.Size(); ++i)
	{
		inputPoly[i].x = m_points[i].X;
		inputPoly[i].y = m_points[i].Y;
		inputPoly[i].index = i;
	}

	// decompose concave stuff into convex pieces (2D shape outline only)
	// try the optimal triangulation first.
	// it has been observed that this can sometimes generate crappy results so we will add another pass
	CPartitionEngine partition;
	std::list<CPartitionPoly> tempPolygons;
	if (0 == partition.ConvexPartition_Optimal(&inputPoly, &tempPolygons))
	{
		WARN_ENGINE(TXT("Convex triangulation failed. Shape may be invalid"));
		return false;
	}

	// process the list again, now it contains many "almost convex" :) pieces
	// in this pass we will try to break down the remaining concave polygons
	// as a last resort if everything else fails we will triangulate them
	std::list<CPartitionPoly> outPolygons;
	for ( auto it = tempPolygons.begin(); it != tempPolygons.end(); ++it )
	{
		if (0 == partition.ConvexPartition_Forced(&*it, &outPolygons))
		{
			if (0 == partition.Triangulate(&*it, &outPolygons))
			{
				WARN_ENGINE(TXT("Convex triangulation failed. Shape may be invalid"));
				return false;
			}
		}
	}

	// create convex pieces for every 2D convex part
	// there may be more than one convex piece created from one 2D convex
	// becaues there may be differences in the vertex height
	for (auto it = outPolygons.begin();
		it != outPolygons.end(); ++it)
	{
		auto& poly = *it;

		TDynArray<Vector> localVertices;
		const Uint32 numPoints = poly.GetNumPoints();
		localVertices.Resize(numPoints);

		// Transform vertices to base space
		for (Uint32 i=0; i<numPoints; ++i)
		{
			const auto& point = poly[i];
			const Vector pos = m_points[point.index];
			localVertices[i] = m_localToShape.TransformPoint(pos);
		}

		// Start pulling parts
		TDynArray<Vector> buildVertices(numPoints);
		while (localVertices.Size() >= 3)
		{
			// Extract 3 first vertices (always valid since we start with a 2D convex)
			buildVertices.ClearFast();
			buildVertices.PushBack(localVertices[0]);
			buildVertices.PushBack(localVertices[1]);
			buildVertices.PushBack(localVertices[2]);

			// Calculate basic plane for the current triangle
			const Plane triPlane(localVertices[0], localVertices[1], localVertices[2]);

			// Get rid of the middle vertex
			localVertices.RemoveAt(1);

			// Try to add rest of the shape - as long as it lies on the same plane
			while (localVertices.Size() > 2)
			{
				if (IsStillOnPlane(triPlane, localVertices[2]))
				{
					buildVertices.PushBack(localVertices[2]);
					localVertices.RemoveAt(1);					
				}
				else
				{
					// done
					break;
				}
			}

			// Create and build convex geometry
			Convex* outConvex = new Convex();
			outConvex->Build(buildVertices.TypedData(), buildVertices.Size(), upVector);
			outConvex->m_next = convexList;
			convexList = outConvex;
		}
	}

	// Compiled
	return true;
}

//---------------------------------------------------------------------------

Bool CAreaShapeBuilder::Face::BuildPlanarFace( const Plane& plane, const Float size /*= 1000.0f*/ )
{
	// Set plane
	m_plane = plane;

#if 1
	// get plane directions
	Vector u,v;
	if ( Abs(plane.NormalDistance.Z) > 0.7f )
	{
		const Vector temp  = Vector::Cross(plane.NormalDistance, Vector::EX);

		u = Vector::Cross(plane.NormalDistance, temp).Normalized3();
		v = Vector::Cross(plane.NormalDistance, u).Normalized3();
	}
	else
	{
		const Vector temp  = Vector::Cross(plane.NormalDistance, Vector::EZ);

		u = Vector::Cross(plane.NormalDistance, temp).Normalized3();
		v = Vector::Cross(plane.NormalDistance, u).Normalized3();
	}

	// scale
	u *= size;
	v *= size;

	// build plane
	const Vector base = plane.NormalDistance * (-plane.NormalDistance.W);

	// build vertices
	m_vertices.Resize(4);
	m_vertices[0] = base - u + v;
	m_vertices[1] = base - u - v;
	m_vertices[2] = base + u - v;
	m_vertices[3] = base + u + v;
#else
	// Vectors
	static Vector worldNormals[6];
	static Vector worldVertices[6][4];
	static Bool init = true;

	// Initialize internal tables
	if ( init )
	{
		init = false;

		// Init normals
		worldNormals[0] = Vector( 1.0f, 0.0f, 0.0f );
		worldNormals[1] = Vector( -1.0f, 0.0f, 0.0f );
		worldNormals[2] = Vector( 0.0f, 1.0f, 0.0f );
		worldNormals[3] = Vector( 0.0f, -1.0f, 0.0f );
		worldNormals[4] = Vector( 0.0f, 0.0f, 1.0f );
		worldNormals[5] = Vector( 0.0f, 0.0f, -1.0f );

		// Initialize faces
		for ( Uint32 i=0; i<6; i++ )
		{
			Vector up = (i<4) ? Vector(0,0,1) : Vector(1,0,0);
			Vector right = Vector::Cross( up, worldNormals[i] );
			worldVertices[i][0] = up + right;
			worldVertices[i][1] = up - right;
			worldVertices[i][2] = -up - right;
			worldVertices[i][3] = -up + right;
		}
	}

	// Find best axis
	Float bestDot=-2.0f;
	Int32 best = -1;
	for ( Uint32 i=0; i<6; i++ )
	{
		Float dot = Vector::Dot3( plane.NormalDistance, worldNormals[i] );
		if ( dot > bestDot )
		{
			bestDot = dot;
			best = i;
		}
	}

	// Project vertices
	m_vertices.Resize(4);
	m_vertices[0] = plane.Project( worldVertices[best][0] * size );
	m_vertices[1] = plane.Project( worldVertices[best][1] * size );
	m_vertices[2] = plane.Project( worldVertices[best][2] * size );
	m_vertices[3] = plane.Project( worldVertices[best][3] * size );
#endif
	return true;
}

void CAreaShapeBuilder::Face::SplitFace( const Plane& clipPlane, Face& outFrontFace, Face& outBackFace, const Bool removePlanar ) const
{
	// create temporary buffers
	const Uint32 numVertices = m_vertices.Size();
	const Uint32 numSegments = numVertices + 1;
	Float* vertexDistances = (Float*) alloca(sizeof(Float) * numSegments);
	Int8* vertexSides = (Int8*) alloca(sizeof(Int8) * numSegments);

	// calculate distance of vertices to clipping plane
	Uint32 numPositive = 0;
	Uint32 numNegative = 0;
	for (Uint32 i=0; i<numVertices; ++i)
	{
		const Vector& a = m_vertices[i];
		const Float dist = clipPlane.DistanceTo(a);
		vertexDistances[i] = dist;

		if (dist > ClipDistTreshold)
		{
			vertexSides[i] = 1;
			numPositive += 1;
		}
		else if (dist < -ClipDistTreshold)
		{
			vertexSides[i] = -1;
			numNegative += 1;
		}
		else
		{
			vertexSides[i] = 0;
		}
	}

	// wrap
	vertexDistances[numVertices] = vertexDistances[0];
	vertexSides[numVertices] = vertexSides[0];

	// planar case
	if (numNegative == 0 && numPositive == 0)
	{
		const Float dot = Vector::Dot3( clipPlane.NormalDistance, m_plane.NormalDistance );
		if ( dot >= 0.0f && !removePlanar )
		{
			outFrontFace = *this;
			return;
		}
		else
		{
			outBackFace = *this;
			return;
		}
	}

	// no negative part - face is in front of the plane
	if (numNegative == 0)
	{
		outFrontFace = *this;
		return;
	}
	else if (numPositive == 0)
	{
		outBackFace = *this;
		return;
	}

	// copy plane settings
	outFrontFace.m_plane = m_plane;
	outBackFace.m_plane = m_plane;

	// distribute vertices
	for (Uint32 i=0; i<numVertices; ++i)
	{
		const Vector& srcPos = m_vertices[i];

		// point is both points
		if (vertexSides[i] == 0)
		{
			outFrontFace.m_vertices.PushBack(srcPos);
			outBackFace.m_vertices.PushBack(srcPos);
			continue;
		}

		// point is front of the plane
		if (vertexSides[i] == 1)
		{
			outFrontFace.m_vertices.PushBack(srcPos);
		}

		// point is on the back side of the plane
		if (vertexSides[i] == -1)
		{
			outBackFace.m_vertices.PushBack(srcPos);
		}

		// ignore edges that do not intersect the plane
		if ( vertexSides[i+1] == 0 || ( vertexSides[i+1] == vertexSides[i] ) )
		{
			continue;
		}

		// we have an intersecting edge
		const Float frac = vertexDistances[i] / ( vertexDistances[i] - vertexDistances[i+1] );
		ASSERT( frac >= 0.0f && frac <= 1.0f );
		const Vector pos = Lerp<Vector>( frac, m_vertices[ i ], m_vertices[ (i+1)%m_vertices.Size() ] );

		outFrontFace.m_vertices.PushBack(pos);
		outBackFace.m_vertices.PushBack(pos);
	}
}

void CAreaShapeBuilder::Face::Swap()
{
	// flip plane
	m_plane.NormalDistance.Negate();

	// swap the vertices
	const Uint32 count = m_vertices.Size();
	for (Uint32 i=0; i<(count/2); ++i)
	{
		::Swap(m_vertices[i], m_vertices[(count-1)-i]);
	}
}

//---------------------------------------------------------------------------

void CAreaShapeBuilder::Convex::Build(const Vector* baseVertices, const Uint32 numVertices, const Vector& height)
{
	// Initialize face table
	const Uint32 numFaces = numVertices + 2;
	m_faces.Resize(numFaces);

	// Fill down and up faces
	{
		Face& faceB = m_faces[0];
		Face& faceT = m_faces[1];

		faceB.m_vertices.Resize(numVertices);
		faceT.m_vertices.Resize(numVertices);

		for (Uint32 i=0; i<numVertices; ++i)
		{
			faceB.m_vertices[i] = baseVertices[(numVertices-1)-i];
			faceT.m_vertices[i] = baseVertices[i] + height;
		}

		faceB.UpdatePlane();
		faceT.UpdatePlane();
	}

	// Create side faces
	for (Uint32 i=0; i<numVertices; ++i)
	{
		const Vector b0 = baseVertices[i];
		const Vector b1 = baseVertices[(i+1) % numVertices];
		const Vector t0 = b0 + height;
		const Vector t1 = b1 + height;

		Face& face = m_faces[i+2];

		face.m_vertices.Resize(4);
		face.m_vertices[0] = b1;
		face.m_vertices[1] = t1;
		face.m_vertices[2] = t0;
		face.m_vertices[3] = b0;

		face.UpdatePlane();
	}

	// Calculate final bounding box
	CalcBoundingBox();
}

Bool CAreaShapeBuilder::Convex::GetClipFace(const Plane& plane, Face& outFace, Uint32& outNumFrontVertices, Uint32& outNumBackVertices) const
{
	// Generate big face
	Face bigFace;
	bigFace.BuildPlanarFace( plane );

	// Clip to planes
	for (Uint32 i=0; i<m_faces.Size(); ++i)
	{
		const Plane& facePlane = m_faces[i].m_plane;

		Face frontFace, backFace;
		bigFace.SplitFace( facePlane, frontFace, backFace, false );

		if (backFace.m_vertices.Empty())
		{
			for (Uint32 i=0; i<m_faces.Size(); ++i)
			{
				const Face& face = m_faces[i];

				for (Uint32 j=0; j<face.m_vertices.Size(); ++j)
				{
					const Float dist = plane.DistanceTo(face.m_vertices[j]);
					if (dist < -ClipDistTreshold)
					{
						outNumBackVertices += 1;
					}
					else if (dist > ClipDistTreshold)
					{
						outNumFrontVertices += 1;
					}
				}
			}

			return false;
		}

		Swap(bigFace.m_vertices, backFace.m_vertices);
	}

	// Whatever survived is the out face
	outFace = bigFace;

	// Finalize with calculating the plane - should match
	const Plane facePlane(outFace.m_vertices[0], outFace.m_vertices[1], outFace.m_vertices[2]);
	const Float planeDot = Vector::Dot3(facePlane.NormalDistance, plane.NormalDistance);
	if (Abs(planeDot) < 0.999f)
	{
		WARN_ENGINE(TXT("Polygon clipping - not aligned to plane"));
		return false;
	}

	// Make sure that the face plane is the same as the clipping plane
	outFace.m_plane = plane;
	if (planeDot < 0.0f)
	{
		outFace.Swap();
	}

	// Done
	return true;
}

void CAreaShapeBuilder::Convex::CalcBoundingBox()
{
	m_box.Clear();

	for (Uint32 i=0; i<m_faces.Size(); ++i)
	{
		const Face& face = m_faces[i];
		for (Uint32 j=0; j<face.m_vertices.Size(); ++j)
		{
			m_box.AddPoint(face.m_vertices[j]);
		}
	}
}

Bool CAreaShapeBuilder::Convex::Split(const Plane& plane, Convex*& frontPartList, Convex*& backPartList)
{
	// Create clip face - if the face will be empty than there's on intersection
	Face clipFace;
	Uint32 numFrontVertices = 0;
	Uint32 numBackVertices = 0;
	if (!GetClipFace(plane, clipFace, numFrontVertices, numBackVertices))
	{
		// decide on which side of the plane this convex lies
		if (numFrontVertices == 0)
		{
			m_next = backPartList;
			backPartList = this;

			return false; // do not delete
		}
		else if (numBackVertices == 0)
		{
			m_next = frontPartList;
			frontPartList = this;

			return false; // do not delete
		}
		else
		{
			m_next = backPartList;
			backPartList = this;

			return false; // do not delete
		}
	}

	// Generate output convex pieces
	Convex* frontConvex = new Convex();
	Convex* backConvex = new Convex();

	// Split faces
	for (Uint32 i=0; i<m_faces.Size(); ++i)
	{
		Face frontFace, backFace;
		m_faces[i].SplitFace( plane, frontFace, backFace, false );

		if (!frontFace.m_vertices.Empty())
		{
			frontConvex->m_faces.PushBack(frontFace);
		}

		if (!backFace.m_vertices.Empty())
		{
			backConvex->m_faces.PushBack(backFace);
		}
	}

	// Add front convex to the list
	{
		frontConvex->m_faces.PushBack(clipFace);		
		frontConvex->m_faces.Back().Swap();

		frontConvex->CalcBoundingBox();

		frontConvex->m_next = frontPartList;
		frontPartList = frontConvex;
	}

	// Add back convex to the list
	{
		backConvex->m_faces.PushBack(clipFace);

		backConvex->CalcBoundingBox();

		backConvex->m_next = backPartList;
		backPartList = backConvex;
	}

	// Return true to indicate that the original can be deleted
	return true;
}

Bool CAreaShapeBuilder::Convex::IsInFrontOf(const Plane& plane) const
{
	for (Uint32 i=0; i<m_faces.Size(); ++i)
	{
		const Face& face = m_faces[i];

		const Uint32 numVertices = face.m_vertices.Size();
		for (Uint32 j=0; j<numVertices; ++j)
		{
			const Vector& pos = face.m_vertices[j];
			if (plane.GetSide(pos) == Plane::PS_Back)
			{
				return false;
			}
		}
	}

	return true;
}

Bool CAreaShapeBuilder::Convex::Clip(const Convex& clipConvex, Convex*& outList)
{
	// fast bounding box overlap
	if (!m_box.Touches(clipConvex.m_box))
	{
		m_next = outList;
		outList = this;

		return false; // Do not delete original
	}

	// overlap test using planes - prevents unnecesarry splitting
	const Uint32 numClipPlanes = clipConvex.m_faces.Size();
	for (Uint32 i=0; i<numClipPlanes; ++i)
	{
		const Plane& clipPlane = clipConvex.m_faces[i].m_plane;

		if (IsInFrontOf(clipPlane))
		{
			m_next = outList;
			outList = this;

			return false; // Do not delete original
		}
	}

	// start with one convex in the back list and empty front list
	Convex* frontList = NULL;
	Convex* backList = new Convex(*this);
	m_next = NULL;

	// clip to the plane list
	for (Uint32 i=0; i<numClipPlanes; ++i)
	{
		const Plane& clipPlane = clipConvex.m_faces[i].m_plane;

		// new back list
		Convex* newBackList = NULL;

		// process all convexes from back list
		Convex* cur = backList;
		while (NULL != cur)
		{
			Convex* next = cur->m_next;
			cur->m_next = NULL;

			if (cur->Split(clipPlane, frontList, newBackList))
			{
				// discard part
				delete cur;
			}

			cur = next;
		}

		// substitute the list
		backList = newBackList;
	}

	// no real splits - restore original
	if (NULL == backList)
	{
		CleanupList(frontList);

		m_next = outList;
		outList = this;

		return false; // Do not delete original
	}

	// what ever is left in the back list should be deleted - it's the part that is clipped away
	CleanupList(backList);

	// whatever is in the front list is the left over part and should be preserved
	{
		Convex* cur = frontList;
		while (NULL != cur)
		{
			Convex* next = cur->m_next;

			cur->m_next = outList;
			outList = cur;

			cur = next;
		}
	}

	// delete original (it was clipped)
	return true;
}

const Uint32 CAreaShapeBuilder::Convex::GetNumFaces() const
{
	return m_faces.Size();
}

const Uint32 CAreaShapeBuilder::Convex::GetNumFaceVertices(const Uint32 faceIndex) const
{
	return m_faces[faceIndex].m_vertices.Size();
}

const Vector CAreaShapeBuilder::Convex::GetFaceVertex(const Uint32 faceIndex, const Uint32 vertexIndex) const
{
	return m_faces[faceIndex].m_vertices[vertexIndex];
}

//---------------------------------------------------------------------------

CAreaShapeBuilder::CAreaShapeBuilder()
{
	m_referenceSpace = Matrix::IDENTITY;
	m_referenceSpaceInv = Matrix::IDENTITY;
}

void CAreaShapeBuilder::SetReferenceSpace(const Matrix& referenceSpace)
{
	m_referenceSpace = referenceSpace;
	m_referenceSpaceInv = referenceSpace.FullInverted();
}

void CAreaShapeBuilder::AddOutline(const Matrix& localSpace, const Vector* points, const Uint32 numPoints, const Float height, Bool subtract /*= false*/)
{
	// unable to add outline with less than 3 points
	if (numPoints < 3)
	{
		return;
	}

	// create outline data
	Outline* newOutline = new ( m_outlines ) Outline();
	newOutline->m_height = height;
	newOutline->m_subtract = subtract;
	newOutline->m_points.Resize(numPoints);

	// Copy points
	for (Uint32 i=0; i<numPoints; ++i)
	{
		newOutline->m_points[i] = points[i];
	}

	// calculate the transformation matrix
	const Bool isInRefSpace = (0 == Red::System::MemoryCompare(&localSpace, &m_referenceSpace, sizeof(Matrix)));
	if (isInRefSpace)
	{
		newOutline->m_localToShape = Matrix::IDENTITY;
	}
	else
	{
		newOutline->m_localToShape = localSpace * m_referenceSpaceInv;
	}
}

void CAreaShapeBuilder::AddOutline(const class CAreaComponent& area, Bool subtract/*= false*/)
{
	AddOutline(
		area.GetLocalToWorld(), 
		area.GetLocalPoints().TypedData(), 
		area.GetLocalPoints().Size(), 
		area.GetHeight(),
		subtract );
}

CAreaShape* CAreaShapeBuilder::Compile() const
{
	// Not enought outlines
	if (m_outlines.Empty())
	{
		return NULL;
	}

	// Generate convex pieces for each outline
	Convex* convexList = NULL;
	Convex* subConvexList = NULL;
	for (Uint32 i=0; i<m_outlines.Size(); ++i)
	{
		const Bool subtract = m_outlines[i].m_subtract;
		if (!m_outlines[i].CompileConvexList(subtract ? subConvexList : convexList))
		{
			CleanupList(convexList);
			CleanupList(subConvexList);
			return NULL;
		}
	}

	// CSG clipping
	if (NULL != subConvexList)
	{
		for (const Convex* curClip = subConvexList; NULL != curClip; curClip = curClip->m_next)
		{
			Convex* newList = NULL;

			// process all existing convex parts and clip them to current clip convex
			Convex* cur = convexList;
			while (NULL != cur)
			{
				Convex* next = cur->m_next;
				cur->m_next = NULL;

				if (cur->Clip(*curClip, newList))
				{
					delete cur;
				}

				cur = next;
			}

			// Restart with new list
			convexList = newList;
		}

		// we don't need the clip list anymore
		CleanupList(subConvexList);
	}

	// Everything was clipped away - should not happen under normal conditions
	if (NULL == convexList)
	{
		return NULL;
	}

	// Emit final convex list
	CAreaShape* outShape = new CAreaShape();
	for (Convex* cur = convexList; cur != NULL; cur = cur->m_next)
	{
		outShape->AddConvex(*cur);
	}		

	// Cleanup source convex list
	CleanupList(convexList);

	// Done, return compiled shape
	return outShape;
}