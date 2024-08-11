/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"


/// Material override
class CFXTrackItemMaterialOverride : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemMaterialOverride, CFXTrackItem, 0 );

private:
	THandle< IMaterial >	m_material;
	Bool					m_drawOriginal;
	CName					m_exclusionTag;
	TDynArray< CName >		m_includeList;
	TDynArray< CName >		m_excludeList;
	Bool					m_forceMeshAlternatives;

public:
	CFXTrackItemMaterialOverride();

	//! Change name of track item
	virtual void SetName( const String& name ){};

	//! Get name of track item
	virtual String GetName() const { return TXT("Material override"); };

	virtual String GetCurveName( Uint32 i /* = 0 */ ) const;

	virtual Uint32 GetCurvesCount() const { return 4; }

	RED_INLINE const TDynArray<CName>& GetIncludeList() const { return m_includeList; }
	RED_INLINE const TDynArray<CName>& GetExcludeList() const { return m_excludeList; }
	RED_INLINE Bool AreMeshAlternativesForced() const { return m_forceMeshAlternatives; }

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemMaterialOverride );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_EDIT( m_material, TXT( "Material for override" ) );
	PROPERTY_EDIT( m_exclusionTag, TXT( "Tag" ) );
	PROPERTY_EDIT( m_drawOriginal, TXT( "Draw original?" ) );
	PROPERTY_EDIT( m_includeList, TXT("Apply the material override only to these components") );
	PROPERTY_EDIT( m_excludeList, TXT("Apply the material override to all components except these") );
	PROPERTY_EDIT( m_forceMeshAlternatives, TXT("Force a mesh alternative for special visual components like fur") );
END_CLASS_RTTI();
