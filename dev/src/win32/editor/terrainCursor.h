#pragma once

// parameters used while rebuilding brush buffer
enum EBrushType
{
	BT_SimpleCone		= FLAG( 0 ), //!< Create simple cone brush
	BT_UseFalloffCurve	= FLAG( 1 ), //!< Apply falloff curve to brush
};

enum EToolType
{
	TT_None,
	TT_Flatten,
	TT_Slope,
	TT_Smooth,
	TT_RiseLower,
	TT_PaintTexture,
	TT_PaintColor,
	TT_PaintTerrainHoles,
	TT_Stamp,
	TT_Melt,
	TT_CollForceOn,
	TT_CollForceOff,
	TT_CollReset,
	TT_Max
};

enum ECursorMode
{
	CM_Paint,
	CM_SlopeRotationChange,
	CM_SlopeAngleChange,
	CM_SlopeOffsetChange,
	CM_StampRotationChange,
};

class CBaseTerrainCursor : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CBaseTerrainCursor, CObject );

private:
	Vector								m_position;
	Float								m_size;
	Float								m_height;

	Float								m_texelsPerUnit;

	Uint32								m_texelsPerEdge;
	Float								m_intensity;

	Uint16*								m_dataBuffer;
	Uint32								m_dataBufferSize;
	Float*								m_falloffBuffer;
	Uint32								m_falloffBufferSize;

public:
	CBaseTerrainCursor();
	virtual ~CBaseTerrainCursor();


public:
	RED_INLINE const Vector			GetPosition() const		{ return m_position; }
	RED_INLINE const Float			GetSize() const			{ return m_size; }
	RED_INLINE const Float			GetHeight() const		{ return m_height; }
	RED_INLINE const Float			GetTexelSize() const	{ return m_size * m_texelsPerUnit; }
	RED_INLINE const Float			GetIntensity() const	{ return m_intensity; }

	RED_INLINE virtual ECursorMode	GetMode() const			{ return CM_Paint; }

	RED_INLINE virtual EToolType		GetType() const = 0;
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( CBaseTerrainCursor, CObject );

class CTerrainCursorNull : public CBaseTerrainCursor
{
	DECLARE_ENGINE_CLASS( CTerrainCursorNull, CBaseTerrainCursor, 0 );

public:
	CTerrainCursorNull();
	virtual ~CTerrainCursorNull();

	RED_INLINE virtual EToolType	GetType() const { return TT_None; }
};

BEGIN_CLASS_RTTI( CTerrainCursorNull );
PARENT_CLASS( CBaseTerrainCursor );
END_CLASS_RTTI();

class CTerrainCursorRiseLower : public CBaseTerrainCursor
{
	DECLARE_ENGINE_CLASS( CTerrainCursorRiseLower, CBaseTerrainCursor, 0 );

public:
	CTerrainCursorRiseLower();
	virtual ~CTerrainCursorRiseLower();

	RED_INLINE virtual EToolType	GetType() const { return TT_RiseLower; }
};

BEGIN_CLASS_RTTI( CTerrainCursorRiseLower );
PARENT_CLASS( CBaseTerrainCursor );
END_CLASS_RTTI();

class CTerrainCursorFlatten : public CBaseTerrainCursor
{
	DECLARE_ENGINE_CLASS( CTerrainCursorFlatten, CBaseTerrainCursor, 0 );

public:
	CTerrainCursorFlatten();
	virtual ~CTerrainCursorFlatten();

	RED_INLINE virtual EToolType	GetType() const { return TT_Flatten; }
};

BEGIN_CLASS_RTTI( CTerrainCursorFlatten );
PARENT_CLASS( CBaseTerrainCursor );
END_CLASS_RTTI();

class CTerrainCursorSlope : public CBaseTerrainCursor
{
	DECLARE_ENGINE_CLASS( CTerrainCursorSlope, CBaseTerrainCursor, 0 );

public:
	CTerrainCursorSlope();
	virtual ~CTerrainCursorSlope();

	RED_INLINE virtual EToolType	GetType() const { return TT_Slope; }
};

BEGIN_CLASS_RTTI( CTerrainCursorSlope );
PARENT_CLASS( CBaseTerrainCursor );
END_CLASS_RTTI();

class CTerrainCursorSmooth : public CBaseTerrainCursor
{
	DECLARE_ENGINE_CLASS( CTerrainCursorSmooth, CBaseTerrainCursor, 0 );

public:
	CTerrainCursorSmooth();
	virtual ~CTerrainCursorSmooth();

	RED_INLINE virtual EToolType	GetType() const { return TT_Smooth; }
};

BEGIN_CLASS_RTTI( CTerrainCursorSmooth );
PARENT_CLASS( CBaseTerrainCursor );
END_CLASS_RTTI();

class CTerrainCursorStamp : public CBaseTerrainCursor
{
	DECLARE_ENGINE_CLASS( CTerrainCursorStamp, CBaseTerrainCursor, 0 );

public:
	CTerrainCursorStamp();
	virtual ~CTerrainCursorStamp();

	RED_INLINE virtual EToolType	GetType() const { return TT_Stamp; }
};

BEGIN_CLASS_RTTI( CTerrainCursorStamp );
PARENT_CLASS( CBaseTerrainCursor );
END_CLASS_RTTI();
