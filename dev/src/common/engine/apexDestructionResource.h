/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "apexResource.h"

namespace NxParameterized
{
	class Interface;
}

class CApexDestructionResource : public CApexResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CApexDestructionResource, CApexResource, "redapex", "Apex destruction resource" );

#ifndef NO_EDITOR
protected:
	friend class CEdMeshEditor;
	Bool VerifyChunkMaterials() const;
#endif

protected:
	virtual void RestoreDefaults();
	virtual void ConfigureNewAsset( NxParameterized::Interface* params ) const;

	Uint32 m_maxDepth;
	Uint32 m_originalMaxDepth;
	Uint32 m_supportDepth;
	Float m_neighborPadding;
	Uint32 m_initialAllowance;
	Bool m_formExtendedStructures;
	Bool m_useAssetSupport;
	Bool m_useWorldSupport;
	TDynArray< CName > m_chunkDepthMaterials;

	Float m_unfracturedDensityScaler;
	Float m_fracturedDensityScaler;

	StringAnsi m_fractureSoundEvent;
	CName m_fxName;

public:
	CApexDestructionResource();
	virtual ~CApexDestructionResource();

	CName GetMaterialForChunkDepth( Uint32 depth ) const;
	void SetMaterialForChunkDepth( Uint32 depth, const CName& materialName );

	void SetDensityUnfractured( Float unfractured ) { m_unfracturedDensityScaler = unfractured; }
	void SetDensityFractured( Float fractured ) { m_fracturedDensityScaler = fractured; }
	void GetDensities( Float& unfractured, Float& fractured ) { fractured = m_fracturedDensityScaler, unfractured = m_unfracturedDensityScaler; }
	const char* GetFractureEventName() const { return m_fractureSoundEvent.Size() ? m_fractureSoundEvent.AsChar() : 0; }
	CName GetFractureFxName() const { return m_fxName; }

	virtual void OnSerialize( IFile& file );

	const char* GetAssetTypeName() { return "DestructibleAsset"; }

#ifndef NO_EDITOR
	virtual void FillStatistics( C2dArray* array );
	virtual void FillLODStatistics( C2dArray* array );
#endif

};

BEGIN_CLASS_RTTI( CApexDestructionResource );
PARENT_CLASS( CApexResource );
PROPERTY( m_maxDepth );
PROPERTY( m_originalMaxDepth );
PROPERTY( m_supportDepth );
PROPERTY( m_neighborPadding );
PROPERTY( m_initialAllowance );
PROPERTY( m_formExtendedStructures );
PROPERTY( m_useAssetSupport );
PROPERTY( m_useWorldSupport );
PROPERTY( m_chunkDepthMaterials );
PROPERTY( m_unfracturedDensityScaler );
PROPERTY( m_fracturedDensityScaler );
PROPERTY_CUSTOM_EDIT( m_fractureSoundEvent, TXT( "fracture sound event" ), TXT( "AudioEventBrowser" ) )
PROPERTY_EDIT( m_fxName, TXT( "fx Name" ) )
END_CLASS_RTTI();
