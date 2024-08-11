#pragma once

//-----------------------------------------------------------------------------

#include <..\..\..\external\FBX_SDK_2016_1_2\include\fbxsdk.h>
#include "..\..\common\core\xmlWriter.h"
#include "..\..\common\core\exporter.h"

//-----------------------------------------------------------------------------

enum EFBXFileVersion
{
	EFBX_2009,
	EFBX_2010,
	EFBX_2011,
	EFBX_2013,
	EFBX_2016
};

class FBXImportScene
{
public:
	FbxManager* m_manager;
	FbxScene* m_scene;

public:
	FBXImportScene();
	~FBXImportScene();

	static FBXImportScene* Load( const String& filename );
};


class FBXExportScene
{
public:
	FbxManager*				m_manager;
	FbxScene*				m_scene;
	TDynArray<Matrix>		m_refNodes;
	TDynArray<FbxNode*>		m_nodes;
	THashMap<String, Int32>	m_nodeMap;

	THashMap<String, FbxFileTexture*> m_textures;

	FbxArray<FbxNode*>		m_clusteredFbxNodes;

private:
	CXMLWriter				m_xmlWriter;
	TDynArray< Uint32 >		m_xmlSavedMaterials;

	Uint32 AppendPhongMaterial( FbxNode* lNode, const CName& name, const FbxDouble3& diffuse );

	void ExportXMLMeshData( const CMesh* srcModel );
	void ExportXMLCollisionData( const CCollisionMesh* collision );
	void ExportXMLMaterialData( const CMesh* srcModel, const IMaterial* material, Uint32 materialID, const FbxSurfacePhong* newMaterial );

public:
	FBXExportScene(const String& filePath);
	~FBXExportScene();

	Bool Save( const String& filePath, const EFBXFileVersion& version );

	void ExportMesh( const Matrix& modelLocalToWorld, Bool exportSkinning, const CMesh* srcModel, const String& name );

	void ExportCollisionMesh( const Matrix& modelLocalToWorld, const ICollisionShape* srcCollisionMesh, const String& name );

	Bool ExtractTexture( const IMaterial* srcMaterial, const String& paramName, ITexture*& outTexture );

	void ExtractTextures( FbxSurfacePhong* material, const IMaterial* srcMaterial, const Bool bMaterialNameSet );

	Bool ExtractTextureData( const CBitmapTexture* bitmapTexture, String& outFileName );

	void ExportSkeleton( const CSkeleton* skeleton );

	static void FillExportFormat( TDynArray<CFileFormat>& fileFormatsToFill );

	static EFBXFileVersion GetFBXExportVersion( const IExporter::ExportOptions& options );
};

//-----------------------------------------------------------------------------

template<const Uint32 NUM_BUCKETS = 1024>
class FBXPointCache
{
protected:
	struct Entry
	{
		Vector3	m_point;
		Int32	m_next;

		RED_INLINE Entry()
		{}

		RED_INLINE Entry(const Vector3& point)
			: m_point(point)
			, m_next(-1)
		{}
	};

protected:
	Int32				m_cacheHeads[NUM_BUCKETS];
	TDynArray< Entry >	m_entries;

public:
	RED_INLINE const Uint32 GetNumPoints() const
	{
		return m_entries.Size();
	}

	RED_INLINE const Vector3& GetPoint(const Uint32 index) const
	{
		return m_entries[index].m_point;
	}

	RED_INLINE FBXPointCache()
	{
		m_entries.Reserve( NUM_BUCKETS * 4 );

		for (Uint32 i=0; i<NUM_BUCKETS; ++i)
		{
			m_cacheHeads[i] = -1;
		}
	}

	RED_INLINE void Clear()
	{
		m_entries.ClearFast();

		for (Uint32 i=0; i<NUM_BUCKETS; ++i)
		{
			m_cacheHeads[i] = -1;
		}
	}

	RED_INLINE Uint32 MapPoint(const Vector3& point)
	{
		const Uint32 hash = CalcHash(point);

		Int32 entryIndex = m_cacheHeads[ hash ];
		while (entryIndex != -1)
		{
			const Entry& entry = m_entries[ entryIndex ];
			if ( entry.m_point.X == point.X && entry.m_point.Y == point.Y && entry.m_point.Z == point.Z )
			{
				return entryIndex;
			}

			entryIndex = entry.m_next;
		}

		const Uint32 newEntryIndex = m_entries.Size();
		Entry* newEntry = new (m_entries) Entry(point);

		newEntry->m_next = m_cacheHeads[hash];
		m_cacheHeads[hash] = newEntryIndex;

		return newEntryIndex;
	}

protected:
	RED_INLINE static const Uint32 CalcHash(const Vector3& point)
	{
		// works good for integer ranges
		const Uint32 x = ( ( Uint32& ) point.X );
		const Uint32 y = ( ( Uint32& ) point.Y );
		const Uint32 z = ( ( Uint32& ) point.Z );
		return ( x ^ y ^ z ) % NUM_BUCKETS;
	}
};