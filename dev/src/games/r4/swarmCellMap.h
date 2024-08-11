#pragma once
#include "../../common/core/intsBitField.h"
#include "r4SwarmUtils.h"

//////////////////////////////////////////////////////
// CSwarmCellMap
//
class CSwarmCellMap : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSwarmCellMap, CResource, "cellmap", "SwarmCellMap" );

	typedef TTypedIntsBitField< 0, Int32 >					CBitField;

	Float		m_cellSize;

	CBitField	m_data;
	

	// Cached useful variables 
	Vector3		m_cornerPosition;

	Int32		m_dataSizeX;
	Int32		m_dataSizeY;
	Int32		m_dataSizeZ;
	Int32		m_dataSize;
	Float		m_sizeInKbytes;

public:
	CSwarmCellMap();
	~CSwarmCellMap();

	void DebugDisplay( CRenderFrame* frame )const;
	void ClearDebug();

	// Return the value of the cell map at the givent position
	Int32 Get( const Vector3 &Position )const;

	// sets collision to a vector pointing away from collision data that opposes velocity
	// if the blocked cell is one cell away from position its magnitude is 1
	// then gradually descends to reach 0 when the distance is equal to radius
	// Also returns true if the cell at position is a valid one
	Bool	ComputeCollisionForceAtPosition( const Vector3 & position, const Vector3 & velocity, Float radius, Vector3 & collision, const Color& color = Color( 0,0,0,0 ) )const;
	void	GetCellPositionFromIndex( Int32 indexX, Int32 indexY, Int32 indexZ, Vector3 &cellPosition )const;
	void	GetCellIndexFromPosition( const Vector3 &cellPosition, Int32 &indexX, Int32 &indexY, Int32 &indexZ )const;
	
	// Returns the next available cell on a specified axis and direction
	// return false if no available cell found
	// direction.X > 0 searches on the positive X axis 
	// direction.X < 0 searches on the negative X axis 
	// etc.
	Bool	GetNextFreeCellPosition_AxisAligned( const Vector3 & position, Vector3 & cellPosition, const Vector3 & direction, const Color& color = Color( 0,0,0,0 ) )const;
	// Returns true if there is a blocjed cell in the direction and range disired
	Bool	GetNextBlockedCellPosition_AxisAligned( const Vector3 & position, Vector3 & blockedCellPosition, const Vector3 & direction, Float radius = -1.0f, Vector3 *lastFreeCellPosition = nullptr, const Color& color = Color( 0,0,0,0 ) )const;
	Bool	GetFreeCellAtDistance_AxisAligned( const Vector3 & position, Vector3 & freeCellPosition, const Vector3 & direction, Float distance, const Color& color = Color( 0,0,0,0 ) )const;
	Bool	LineTest( const Vector3 &position, const Vector3 &direction, const Color& color = Color( 0,0,0,0 ) )const;

	const Float & GetCellSize()const{ return m_cellSize; }
	const Float & GetSizeInKByte()const{ return m_sizeInKbytes; }

	///////////////////////////
	// Tool interface

	// Creates a new CSwarmCellMap Resource
	static CSwarmCellMap* Create();

	// Returns the disk file of the cell map if it exists
	static CDiskFile *const  FindFileInDepot( const String &m_resourceName );

	// Generates the cell map from the area
	void Generate( CAreaComponent *const areaComponent, Float cellSize );

	Box ComputeCellmapArea( const Box& movingZoneArea, Float cellSize );

	void OnSerialize( IFile& file )override;

#ifdef SWARM_ADVANCED_DEBUG
	// Debug stuff only :
private:
	class CDebugTestCellData
	{
	public :
		Vector3	m_position;
		Color	m_color;

		CDebugTestCellData( const Vector3 &position, const Color &color )
			: m_position( position )
			, m_color( color )
		{}
	};
	// debug only 
	TDynArray< CDebugTestCellData >	m_debugTestCells;
#endif 
};


BEGIN_CLASS_RTTI( CSwarmCellMap );
	PARENT_CLASS( CResource );
	PROPERTY( m_cellSize );
END_CLASS_RTTI();

