#pragma once

#include "fbxCommon.h"
//-----------------------------------------------------------------------------

class FBXMeshBuilder
{
public:
	struct Material
	{
		String m_name;

		inline Material()
		{}
	};

	struct Vertex
	{
		Uint32 m_point;
		Vector3 m_pos;
		Vector3 m_normal;
		Vector3 m_tangent;
		Vector3 m_bitangent;
		Vector2 m_uv0;
		Vector2 m_uv1;
		Vector m_color;
		Float m_weights[4];
		Uint32 m_indices[4]; // global bone table

		inline Vertex()
			: m_pos(0,0,0)
			, m_normal(0,0,0)
			, m_tangent(0,0,0)
			, m_bitangent(0,0,0)
			, m_uv0(0,0)
			, m_uv1(0,0)
			, m_color(1,1,1,1)
		{
			m_weights[0] = 0;
			m_weights[1] = 0;
			m_weights[0] = 0;
			m_weights[3] = 0;
			m_indices[0] = 0;
			m_indices[1] = 0;
			m_indices[2] = 0;
			m_indices[3] = 0;
		}
	};

	struct Triangle
	{
		Uint32 m_material;
		Vertex m_v[3];
		Vector3 m_normal;
		Vector3 m_tangent;
		Vector3 m_bitangent;
		Triangle* m_next;
		Uint8 m_validTangents:1;
		Uint8 m_validNormals:1;
		Uint8 m_validColor:1;
		Uint8 m_validUV0:1;
		Uint8 m_validUV1:1;

		inline Triangle(const Uint32 materialIndex)
			: m_material(materialIndex)
			, m_normal(0,0,0)
			, m_tangent(0,0,0)
			, m_bitangent(0,0,0)
			, m_next(NULL)
			, m_validTangents(0)
			, m_validNormals(0)
			, m_validColor(0)
			, m_validUV0(0)
			, m_validUV1(0)
		{}
	};

	struct LODLevel
	{
		Uint32 m_lodIndex;
		TDynArray<Triangle>	m_triangles;
		FBXPointCache<2048> m_points;		
		TDynArray<TDynArray<Uint32>> m_shared;

		TDynArray<Triangle*> m_materialTriangles;

		LODLevel(const Uint32 index);
		~LODLevel();

		void GenerateConnectivity();

		void CalculateNormals();

		void CalculateTangentSpace();

		void BuildTriangleLists();

		FBXMeshBuilder::Triangle* AddTriangle(Uint32 materialIndex, const Vertex& a, const Vertex& b, const Vertex& c);
	};

	struct CollisionMesh
	{
		enum ECollisionType
		{
			CT_Convex,
			CT_Trimesh
		};

		TDynArray< Vector > m_vertices;
		TDynArray< Uint32 > m_indices;
		ECollisionType m_collisionType;
		TDynArray<CName> m_materialNames;
		TDynArray<unsigned short> m_materialIndexes;
	};

	struct Bone
	{
		const FbxNode* m_source;
		String m_name;
		Int32 m_parent;
		Matrix m_localToWorld;

		inline Bone(const Int32 parent, const String& name)
			: m_parent(parent)
			, m_name(name)
			, m_source(NULL)
		{};
	};

public:
	TDynArray<Material> m_materials;
	TDynArray<LODLevel*> m_lods;
	TDynArray<CollisionMesh*> m_collision;
	TDynArray<Bone> m_bones;

	const FbxPose* m_bindPose;

	FBXMeshBuilder();
	~FBXMeshBuilder();

	LODLevel* GetLodLevel(const Uint32 lodIndex);

	Int32 AddBone(const FbxNode* node);

	Int32 AddBone(const FbxNode* node, const FbxAMatrix& matrix);

	Uint32 AddMaterial(const FbxSurfaceMaterial* material, const FbxNode* owner);

	void GetLocalToWorld(const FbxNode* node, Matrix& localToWorld);

	Bool GetBindPoseLocalToWorld(const FbxNode* node, Matrix& localToWorld);

	void ExtractMesh(const Uint32 lodIndex, const FbxMesh* mesh, const Matrix& localToWorld, const FbxNode* forceSkinNode=NULL);

	void ExtractCollisionMesh(const FbxMesh* mesh, const Matrix& localToWorld, CollisionMesh::ECollisionType collisionType);

	static Matrix MakeRotation( const FbxDouble3& rot, EFbxRotationOrder rotOrder );

	static Matrix MakeTranslation( const FbxDouble3& pos );
};

//-----------------------------------------------------------------------------
