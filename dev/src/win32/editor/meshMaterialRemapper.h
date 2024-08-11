
#pragma once


class CEdMeshMaterialRemapper
{
public:

	CEdMeshMaterialRemapper( wxWindow* parent, CEdUndoManager* undoManager = nullptr );

	//! Do remapping for single, already loaded mesh
	Bool Execute( CMeshTypeResource* mesh );

	//! Do remapping for all given files
	Bool Execute( const TDynArray< CDiskFile* >& meshFiles );

	//! Do remapping for all meshes in given directories
	Bool Execute( const TDynArray< CDirectory* >& dirs );

private:
	void CollectMeshMaterialInfo( const CMeshTypeResource* mesh );
	Bool DoShit();
	Bool FindMaterialMatch( const TDynArray< String >& allMaterialPaths, const String& matchName, Int32& idxFound );
	String FindDiffuseTextureName( const CMaterialInstance* matInstance );
	String GenerateMappingLabel( const String& meshMatName, const String& diffuseTexName );
	void ApplyTemplate( class CEdRemappingDialog* reamppingDlg );

	struct MaterialInfo
	{
		String m_materialName;
		String m_diffuseTexture;
		String m_shaderName;
		Bool m_localInstance;

		MaterialInfo()
		{
			m_materialName = String::EMPTY;
			m_diffuseTexture = String::EMPTY;
			m_shaderName = String::EMPTY;
			m_localInstance = false;
		}

		MaterialInfo( const String& materialName, const String& diffuseTex, const String& shaderName, Bool localInstance )
		{
			m_materialName = materialName;
			m_diffuseTexture = diffuseTex;
			m_shaderName = shaderName;
			m_localInstance = localInstance;
		}
	};

	struct MeshDesc
	{
		String m_meshName;
		String m_baseMaterial;

		MeshDesc( const String& meshName, const String& baseMaterial )
		{
			m_meshName = meshName;
			m_baseMaterial = baseMaterial;
		}
	};

	struct StringNoCaseCompare
	{
		static Bool Less( const MaterialInfo& s1, const MaterialInfo& s2 ) 
		{ 
			Int32 ret = Red::System::StringCompareNoCase( s1.m_materialName.AsChar(), s2.m_materialName.AsChar() );
			if ( ret == 0 )
			{
				ret = Red::System::StringCompareNoCase( s1.m_diffuseTexture.AsChar(), s2.m_diffuseTexture.AsChar() );
			}
			if ( ret == 0 )
			{
				ret = Red::System::StringCompareNoCase( s1.m_shaderName.AsChar(), s2.m_shaderName.AsChar() );
			}
			if ( ret == 0 )
			{
				return s1.m_localInstance != s2.m_localInstance && !s1.m_localInstance;
			}
			return ret < 0;
		}	
	};

	typedef TSortedMap< MaterialInfo/*mat name*/, TDynArray< MeshDesc >/*meshes using this mat*/, StringNoCaseCompare > 
		MeshMaterialMap;

	wxWindow*               m_parent;
	CEdUndoManager*         m_undoManager;
	MeshMaterialMap         m_allMeshMaterials;
	TDynArray< CDiskFile* > m_meshFiles;
	String                  m_initialMatPath;
	String					m_initialTemplatePath;
	TDynArray< CDiskFile* > m_allMatFiles;				// stores all possible saved material instances (from chosen directory and template)
	TDynArray< IMaterial* > m_templateLocalMaterials;	// stores local material instances (from templates)
};
