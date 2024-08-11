/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "..\..\common\game\gameWorld.h"


/// World class used by default by the CEdPreviewPanel. Some editors might override with their own world, but the default one will
/// be CEdPreviewWorld.
class CEdPreviewWorld : public CGameWorld
{
	DECLARE_ENGINE_RESOURCE_CLASS( CEdPreviewWorld, CGameWorld, "w2ew", "Editor World" );

protected:
	Vector m_wind;

public:
	CEdPreviewWorld();
	virtual ~CEdPreviewWorld();

	/// CEdPreviewWorld has somewhat-constant wind at all points. The wind is roughly in a given direction and strength, but
	/// fluctuates with time to make cloth "blow". The fluctuation is the same as is used in CWitcherWorld.
	virtual Vector GetWindAtPointForVisuals( const Vector& point, Bool withTurbulence, Bool withForcefield = true ) const;
	virtual Vector GetWindAtPoint( const Vector& point ) const;

	/// Set wind direction and strength. The direction vector will be adjusted to have the given magnitude. If either direction or
	/// magnitude is 0, then thre is no wind. Default is no wind.
	void SetWind( const Vector& direction, Float magnitude );
};

BEGIN_CLASS_RTTI( CEdPreviewWorld );
PARENT_CLASS( CGameWorld );
END_CLASS_RTTI();
