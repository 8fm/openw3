/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/resource.h"

enum EVegetationAlignment
{
	VA_Terrain,
	VA_Collision,

	VA_Max
};

BEGIN_ENUM_RTTI( EVegetationAlignment );
ENUM_OPTION( VA_Terrain );
ENUM_OPTION( VA_Collision );
ENUM_OPTION( VA_Max );
END_ENUM_RTTI();

class CVegetationBrushEntry : public CObject
{
	DECLARE_ENGINE_CLASS( CVegetationBrushEntry, CObject, 0 );

public:
	THandle< CSRTBaseTree >				m_resource;					//!< A vegetation resource to be sowed
	Float								m_density;					//!< Seeding denstiy
	Float								m_size;						//!< Initial size
	Float								m_sizeVar;					//!< Size variation
	Float								m_radiusScale;				//!< Mesh radius scaling

public:
	CVegetationBrushEntry();

	CSRTBaseTree*		GetBaseTree() { return m_resource.Get(); }
	const CSRTBaseTree* GetBaseTree() const { return m_resource.Get(); }
};

BEGIN_CLASS_RTTI( CVegetationBrushEntry );
PARENT_CLASS( CObject );
PROPERTY( m_resource );
PROPERTY_EDIT( m_size, TXT("Scale of the tree") );
PROPERTY_EDIT( m_sizeVar, TXT("Scale variation") );
PROPERTY_EDIT( m_radiusScale, TXT("Scaling of the tree radius, for avoiding collision with other trees") );
PROPERTY_EDIT( m_density, TXT("Density") );
END_CLASS_RTTI();


class CVegetationBrush : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CVegetationBrush, CResource, "vbrush", "Vegetation Brush" );
	
	friend class CEdBrushEntriesPanel;

protected:
	TDynArray< CVegetationBrushEntry* >		m_entries;
	Bool									m_validEntries;

public:
	CVegetationBrush()
		: m_validEntries( true )
	{}

	virtual void OnPostLoad() override;

	Bool HasValidEntries() const
	{
		return m_validEntries;
	}

	void GetEntries( TDynArray< CVegetationBrushEntry* >& entries ) const;

	// Add entry for a base tree
	CVegetationBrushEntry* AddEntry( CSRTBaseTree* baseTree );

	CVegetationBrushEntry* FindEntry( CSRTBaseTree* baseTree );
};

BEGIN_CLASS_RTTI( CVegetationBrush );
PARENT_CLASS( CResource );
PROPERTY( m_entries );
END_CLASS_RTTI();
