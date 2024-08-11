/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Build a trigger shape geometry from shape of area
class CAreaShapeBuilder
{
public:
	// full face
	struct Face
	{
		Plane m_plane;

		TDynArray<Vector> m_vertices;

		RED_FORCE_INLINE void UpdatePlane()
		{
			m_plane.SetPlane(m_vertices[0], m_vertices[1], m_vertices[2]);
		}

		// build a planar face
		Bool BuildPlanarFace( const Plane& plane, const Float size = 1000.0f );

		// split the face into two parts
		void SplitFace( const Plane& clipPlane, Face& outFrontFace, Face& outBackFace, const Bool removePlanar ) const;

		// flip the face (flips the plane to)
		void Swap();
	};

	struct Convex : public IAreaConvexDataSource
	{
		// Convex faces (renderable)
		TDynArray<Face> m_faces;

		// Next convex on list
		Convex* m_next;


		// Bounding box
		Box m_box;

		Convex()
			: m_next(NULL)
		{};

		// Create convex from list of base vertices
		void Build( const Vector* baseVertices, const Uint32 numVertices, const Vector& height );

		// Generate clip face for given plane, returns false if plane is not intersecting the convex)
		Bool GetClipFace( const Plane& plane, Face& outFace, Uint32& outNumFrontVertices, Uint32& outNumBackVertices ) const;

		// Split convex in two parts by plane, if convex is not split it will be reasigned
		// Returns true if the source convex should be deleted
		Bool Split( const Plane& plane, Convex*& frontPartList, Convex*& backPartList );

		// Clip convex by removing part included in given clipping convex (negative area)
		// Returns true if the source convex should be deleted
		Bool Clip( const Convex& clipConvex, Convex*& outList );

		// Returns true if this convex is fully in front of given plane
		Bool IsInFrontOf( const Plane& plane ) const;

		// Update bounding box
		void CalcBoundingBox();

		// IAreaConvexDataSource interface
		virtual const Uint32 GetNumFaces() const;
		virtual const Uint32 GetNumFaceVertices(const Uint32 faceIndex) const;
		virtual const Vector GetFaceVertex(const Uint32 faceIndex, const Uint32 vertexIndex) const;
	};

	struct Outline
	{
		// Outline points
		TDynArray< Vector > m_points;

		// Height of the area
		Float m_height;

		// Reference matrix
		Matrix m_localToShape;

		// CSG add/subtract flag
		Bool m_subtract;

		// Generate convex list (in reference space)
		Bool CompileConvexList(Convex*& convexList) const;
	};

	// Collected outlines
	TDynArray<Outline> m_outlines;

	// Reference local space
	Matrix m_referenceSpace;

	// Inverse of reference local space
	Matrix m_referenceSpaceInv;

public:
	CAreaShapeBuilder();

	//! Set reference space (usually of the first area)
	void SetReferenceSpace(const Matrix& referenceSpace);

	//! Add generic outline to builder
	void AddOutline(const Matrix& localSpace, const Vector* points, const Uint32 numPoints, const Float height, Bool subtract = false);

	//! Add outline from area component
	void AddOutline(const class CAreaComponent& area, Bool subtract = false);

	//! Compile trigger shape (list of convex shapes)
	CAreaShape* Compile() const;
};
