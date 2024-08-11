/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibConst.h"

namespace PathLib
{

class CSpecialZonesMap
{
protected:
	struct Zone
	{
		Box							m_bbox;
		TDynArray< Vector2 >		m_verts;
		NodeFlags					m_clearFlags;
		NodeFlags					m_forceFlags;
	};
	typedef TDynArray< Zone > ZoneList;
	ZoneList							m_zones;
	TDynArray< Uint8 >					m_binTree;
public:
	CSpecialZonesMap();
	~CSpecialZonesMap();

	void Collect( CAreaComponent* component, NodeFlags clearFlags, NodeFlags forceFlags );

	void FinalizeCollection();
	Bool QueryPosition( const Vector3& position, NodeFlags& clearFlags, NodeFlags& forceFlags );
	Bool QueryLine( const Vector3& pos0, const Vector3& pos1, NodeFlags& clearFlags, NodeFlags& forceFlags );

};

};				// namespace PathLib