#pragma once

/// These functions are provided to assist in generating meshes for basic collision shapes.
/// Each takes an array of vertices and an array of indices. Any geometry created by the function will be
/// appended to the arrays, allowing for multiple shapes to be built up into a single mesh.
/// The generated meshes are not guaranteed to be optimal, may have duplicated vertices and such.
namespace CollisionShapeBuilder
{

	// Build a box centered on the origin with width/height/length given by size.
	void BuildBox( const Box& bbox, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices );
	void BuildBox( const Vector& size, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices );
	void GetBoxStats( Uint32& numVertices, Uint32& numIndices );

	// Build a sphere centered on the origin.
	void BuildSphere( Float radius, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices );
	void GetSphereStats( Float radius, Uint32& numVertices, Uint32& numIndices );

	// Build a cylinder centered on the origin, oriented so it runs along the X axis. halfHeight is the length from the origin to the end.
	void BuildCylinder( Float radius, Float halfHeight, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices );
	void GetCylinderStats( Float radius, Uint32& numVertices, Uint32& numIndices );

	// Build a capsule centered on the origin, oriented so it runs along the X axis. halfHeight is the length from the origin to the center of a cap hemisphere.
	// Basically, same as PhysX's collision capsules.
	void BuildCapsule( Float radius, Float halfHeight, TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices );
	void GetCapsuleStats( Float radius, Uint32& numVertices, Uint32& numIndices );

}

