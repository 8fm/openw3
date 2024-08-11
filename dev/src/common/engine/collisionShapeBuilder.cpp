#include "build.h"
#include "collisionShapeBuilder.h"

//
// Some of the code below is based on stuff from the Apex libraries.
//
namespace CollisionShapeBuilder
{

	static Uint32 SubdivisionsForRadius( Float radius )
	{
		// TODO: Vary the number of subdivisions based on the radius?
		return 2;
	}


	static void SubdivideTriProjectToSphere( Uint32 subdivisions, const Vector& v0, const Vector& v1, const Vector& v2, Float radius, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices )
	{
		if ( subdivisions == 0 )
		{
			Uint32 baseIndex = vertices.Size();
			vertices.PushBack( v0.Normalized3() * radius );
			vertices.PushBack( v1.Normalized3() * radius );
			vertices.PushBack( v2.Normalized3() * radius );

			indices.PushBack( baseIndex + 0 );
			indices.PushBack( baseIndex + 1 );
			indices.PushBack( baseIndex + 2 );
		}
		else
		{
			Vector v01 = ( v0 + v1 ) * 0.5f;
			Vector v12 = ( v1 + v2 ) * 0.5f;
			Vector v20 = ( v2 + v0 ) * 0.5f;
			SubdivideTriProjectToSphere( subdivisions - 1, v0, v01, v20, radius, vertices, indices );
			SubdivideTriProjectToSphere( subdivisions - 1, v1, v12, v01, radius, vertices, indices );
			SubdivideTriProjectToSphere( subdivisions - 1, v2, v20, v12, radius, vertices, indices );
			SubdivideTriProjectToSphere( subdivisions - 1, v01, v12, v20, radius, vertices, indices );
		}
	}


	static void SubdivideLineProjectToSphere( Uint32 subdivisions, const Vector& v0, const Vector& v1, Float radius, TDynArray< Vector >& vertices )
	{
		if ( subdivisions == 0 )
		{
			vertices.PushBack( v0.Normalized3() * radius );
			vertices.PushBack( v1.Normalized3() * radius );
		}
		else
		{
			Vector v01 = ( v0 + v1 ) * 0.5f;
			SubdivideLineProjectToSphere( subdivisions - 1, v0, v01, radius, vertices );
			SubdivideLineProjectToSphere( subdivisions - 1, v01, v1, radius, vertices );
		}
	}

	void BuildBox( const Box& bbox, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices )
	{
		Uint32 baseVertex = vertices.Size();
		Uint32 baseIndex = indices.Size();

		vertices.Resize( vertices.Size() + 8 );
		vertices[baseVertex + 0] = Vector( bbox.Min.X, bbox.Min.Y, bbox.Max.Z );
		vertices[baseVertex + 1] = Vector( bbox.Max.X, bbox.Min.Y, bbox.Max.Z );
		vertices[baseVertex + 2] = Vector( bbox.Min.X, bbox.Max.Y, bbox.Max.Z );
		vertices[baseVertex + 3] = Vector( bbox.Max.X, bbox.Max.Y, bbox.Max.Z );
		vertices[baseVertex + 4] = Vector( bbox.Min.X, bbox.Min.Y, bbox.Min.Z );
		vertices[baseVertex + 5] = Vector( bbox.Max.X, bbox.Min.Y, bbox.Min.Z );
		vertices[baseVertex + 6] = Vector( bbox.Min.X, bbox.Max.Y, bbox.Min.Z );
		vertices[baseVertex + 7] = Vector( bbox.Max.X, bbox.Max.Y, bbox.Min.Z );

		const Uint32 numIdx = 36;
		const Uint16 idx[ numIdx ] = {
			0, 1, 2, 1, 3, 2,			// top
			5, 4, 6, 5, 6, 7,			// bottom
			0, 2, 4, 4, 2, 6,			// left
			1, 5, 3, 3, 5, 7,			// right
			2, 3, 6, 3, 7, 6,			// front
			0, 4, 1, 1, 4, 5			// back
		};

		indices.Resize( indices.Size() + numIdx );
		for ( Uint32 i = 0; i < numIdx; ++i )
		{
			indices[baseIndex + i] = idx[i] + baseVertex;
		}
	}
	void BuildBox( const Vector& size, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices )
	{
		Vector hs = size * 0.5f;
		BuildBox( Box( -hs, hs ), vertices, indices );
	}

	void GetBoxStats( Uint32& numVertices, Uint32& numIndices )
	{
		numVertices = 8;
		numIndices = 36;
	}


	void BuildSphere( Float radius, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices )
	{
		// Reserve space for mesh, to avoid extra re-allocations.
		Uint32 numVerts, numInds;
		GetSphereStats( radius, numVerts, numInds );
		vertices.Reserve( vertices.Size() + numVerts );
		indices.Reserve( indices.Size() + numInds );


		// Make the sphere by subdividing a cube.
		TDynArray< Vector > cubeVertices;
		TDynArray< Uint32 > cubeIndices;
		BuildBox( Vector( radius*2.0f, radius*2.0f, radius*2.0f ), cubeVertices, cubeIndices );

		Uint32 numSubdivisions = SubdivisionsForRadius( radius );

		for ( Uint32 i = 0; i < cubeIndices.Size(); i += 3 )
		{
			SubdivideTriProjectToSphere( numSubdivisions, cubeVertices[cubeIndices[i]], cubeVertices[cubeIndices[i+1]], cubeVertices[cubeIndices[i+2]], radius, vertices, indices );
		}
	}
	void GetSphereStats( Float radius, Uint32& numVertices, Uint32& numIndices )
	{
		Uint32 numSubdivisions = SubdivisionsForRadius( radius );
		// We start with a cube, 12 faces. Each subdivision subdivides 1 face into 4. Vertices are not shared
		// between adjacent triangles.
		numVertices = 36;
		numIndices = 36;
		for ( Uint32 i = 0; i < numSubdivisions; ++i )
		{
			numVertices *= 4;
			numIndices *= 4;
		}
	}


	void BuildCylinder( Float radius, Float halfHeight, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices )
	{
		// Reserve space for mesh, to avoid extra re-allocations.
		Uint32 numVerts, numInds;
		GetCylinderStats( radius, numVerts, numInds );
		vertices.Reserve( vertices.Size() + numVerts );
		indices.Reserve( indices.Size() + numInds );

		Uint32 numSubdivisions = SubdivisionsForRadius( radius );

		Uint32 baseVertex = vertices.Size();
		Uint32 baseIndex = indices.Size();
		RED_UNUSED( baseIndex );

		// Similar to the sphere, we'll subdivide a square and project onto a circle. This should end up giving the same points as the center of the sphere,
		// which will work nicely.
		SubdivideLineProjectToSphere( numSubdivisions, Vector( 0,-1,-1 ), Vector( 0, 1,-1 ), radius, vertices );
		SubdivideLineProjectToSphere( numSubdivisions, Vector( 0, 1,-1 ), Vector( 0, 1, 1 ), radius, vertices );
		SubdivideLineProjectToSphere( numSubdivisions, Vector( 0, 1, 1 ), Vector( 0,-1, 1 ), radius, vertices );
		SubdivideLineProjectToSphere( numSubdivisions, Vector( 0,-1, 1 ), Vector( 0,-1,-1 ), radius, vertices );

		// Now push the circle to one end of the cylinder, and duplicate on the other end.
		Uint32 numVertices = vertices.Size() - baseVertex;
		for ( Uint32 i = 0; i < numVertices; ++i )
		{
			Vector& v = vertices[i + baseVertex];
			v.X = halfHeight;
			vertices.PushBack( Vector( -v.X, v.Y, v.Z ) );

			// Also generate two triangles here.
			Uint32 i0 = baseVertex + i;
			Uint32 i1 = baseVertex + ( ( i + 1 ) % numVertices );
			indices.PushBack( i0 );
			indices.PushBack( i0 + numVertices );
			indices.PushBack( i1 );

			indices.PushBack( i1 );
			indices.PushBack( i0 + numVertices );
			indices.PushBack( i1 + numVertices );
		}
	}
	void GetCylinderStats( Float radius, Uint32& numVertices, Uint32& numIndices )
	{
		Uint32 numSubdivisions = SubdivisionsForRadius( radius );
		// We start with 2 loops of 4 lines each. Each subdivision splits 1 line into 2 lines. 2 triangles for each pair
		// of lines. Each pair of tris shares two vertices, but vertices are not shared between separate pairs.
		numVertices = 16;
		for ( Uint32 i = 0; i < numSubdivisions; ++i )
		{
			numVertices *= 2;
		}
		numIndices = numVertices * 3;
	}


	void BuildCapsule( Float radius, Float halfHeight, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices )
	{
		// Reserve space for mesh, to avoid extra re-allocations.
		Uint32 numVerts, numInds;
		GetCapsuleStats( radius, numVerts, numInds );
		vertices.Reserve( vertices.Size() + numVerts );
		indices.Reserve( indices.Size() + numInds );

		Uint32 baseVertex = vertices.Size();

		// First, build a sphere. This is used to make the caps.
		BuildSphere( radius, vertices, indices );

		// Split the sphere into two hemispheres. Because of how the subdivision works, we know that the sphere is split evenly through the middle,
		// so we just check which side of the YZ plane each triangle center is and push it out by the halfHeight. We know that each triangle was made
		// with its own vertices (they're not shared between adjacent triangles), and also that every three vertices form a single triangle, so it's easy.
		for ( Uint32 i = baseVertex; i < vertices.Size(); i += 3 )
		{
			Vector& v0 = vertices[i + 0];
			Vector& v1 = vertices[i + 1];
			Vector& v2 = vertices[i + 2];
			Vector c = ( v0 + v1 + v2 ) / 3.0f;
			Float sgn = MSign( c.X );
			v0.X += sgn * halfHeight;
			v1.X += sgn * halfHeight;
			v2.X += sgn * halfHeight;
		}

		// Put a cylinder between the caps. We assume that the cylinder and sphere have the same subdivision, so that they connect nicely. But, we know
		// how they're made, so that's okay.
		BuildCylinder( radius, halfHeight, vertices, indices );
	}
	void GetCapsuleStats( Float radius, Uint32& numVertices, Uint32& numIndices )
	{
		// We build a cylinder and a sphere independently.
		Uint32 cylVerts, cylIndices, sphVerts, sphIndices;
		GetCylinderStats( radius, cylVerts, cylIndices );
		GetSphereStats( radius, sphVerts, sphIndices );
		numVertices = cylVerts + sphVerts;
		numIndices = cylIndices + sphIndices;
	}

}
