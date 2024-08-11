/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "apexResource.h"


namespace NxParameterized
{
	class Interface;
}


class CApexClothResource : public CApexResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CApexClothResource, CApexResource, "redcloth", "Apex cloth resource" );

#ifndef NO_EDITOR
	friend class CEdMeshEditor;
#endif
	friend class CApexImporter;

public:
	typedef TDynArray< SMeshTypeResourceLODLevel > TGraphicalLODLevelArray;


protected:
	Uint32 m_materialIndex;

	virtual void RestoreDefaults();
	virtual void ConfigureNewAsset( NxParameterized::Interface* params ) const;


	Float	m_simThickness;
	Float	m_simVirtualParticleDensity;
	Bool	m_simDisableCCD;
	
	Float	m_mtlBendingStiffness;
	Float	m_mtlShearingStiffness;
	Float	m_mtlTetherStiffness;
	Float	m_mtlTetherLimit;
	Float	m_mtlDamping;
	Float	m_mtlDrag;
	Bool	m_mtlComDamping;
	Float	m_mtlFriction;
	Float	m_mtlGravityScale;
	Float	m_mtlInertiaScale;
	Float	m_mtlHardStretchLimitation;
	Float	m_mtlMaxDistanceBias;
	Float	m_mtlSelfcollisionThickness;
	Float	m_mtlSelfcollisionStiffness;
	Float	m_mtlMassScale;

#ifndef NO_EDITOR
	String	m_materialPresetName;							//<! The name of the material preset that was last used on this cloth.
#endif

	TGraphicalLODLevelArray m_graphicalLodLevelInfo;		// LOD levels

	// fill in importer
	TDynArray< CName >	m_boneNames;
	TDynArray< Matrix > m_boneMatrices;
	Uint32				m_boneCount;

public:
	CApexClothResource();
	virtual ~CApexClothResource();

#ifndef NO_RESOURCE_IMPORT

	virtual Uint32 GetNumLODLevels() const { return m_graphicalLodLevelInfo.Size(); }
	virtual const SMeshTypeResourceLODLevel& GetLODLevel( Uint32 level ) const;

	virtual Bool UpdateLODSettings( Uint32 level, const SMeshTypeResourceLODLevel& lodSettings );

	virtual Uint32 CountLODTriangles( Uint32 level ) const;
	virtual Uint32 CountLODVertices( Uint32 level ) const;
	virtual Uint32 CountLODChunks( Uint32 level ) const;
	virtual Uint32 CountLODMaterials( Uint32 level ) const;

#endif

	virtual void AddRef();

	void Prepare();

	//! Return the number of bones used by this mesh.
	virtual Uint32 GetBoneCount() const { return m_boneCount; }

	//! Get an array containing the names of each bone. Must have GetBoneCount() elements, or can be NULL if 0 bones.
	virtual const CName* GetBoneNames() const { return m_boneNames.TypedData(); }

	//! Get the matrices for bones. Must have GetBoneCount() elements, or can be NULL if 0 bones.
	virtual const Matrix* GetBoneRigMatrices() const { return m_boneMatrices.TypedData(); }

	Float GetAverageEdgeLength() const;

	const char* GetAssetTypeName() { return "NxClothingAsset"; }

#ifndef NO_EDITOR
	virtual void FillStatistics( C2dArray* array );
	virtual void FillLODStatistics( C2dArray* array );
#endif

	virtual void OnPreSave() override;
};


BEGIN_CLASS_RTTI( CApexClothResource );
PARENT_CLASS( CApexResource );

PROPERTY( m_boneCount );
PROPERTY( m_boneNames );
PROPERTY( m_boneMatrices );
PROPERTY( m_simThickness );
PROPERTY( m_simVirtualParticleDensity );
PROPERTY( m_simDisableCCD );
PROPERTY( m_mtlMassScale );
PROPERTY( m_mtlFriction );
PROPERTY( m_mtlGravityScale );
PROPERTY( m_mtlBendingStiffness );
PROPERTY( m_mtlShearingStiffness );
PROPERTY( m_mtlTetherStiffness );
PROPERTY( m_mtlTetherLimit );
PROPERTY( m_mtlDamping );
PROPERTY( m_mtlDrag );
PROPERTY( m_mtlInertiaScale );
PROPERTY( m_mtlMaxDistanceBias );
PROPERTY( m_mtlSelfcollisionThickness );
PROPERTY( m_mtlSelfcollisionStiffness );
PROPERTY( m_mtlHardStretchLimitation );
PROPERTY( m_mtlComDamping );

#ifndef NO_EDITOR
PROPERTY_NOT_COOKED( m_materialPresetName );
#endif
PROPERTY( m_graphicalLodLevelInfo );
END_CLASS_RTTI();
