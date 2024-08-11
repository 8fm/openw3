/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "undoStep.h"
#include "../../common/engine/terrainTypes.h"
#include "../../common/engine/terrainTile.h"

class CTerrainTile;

// Undo Step holding terrain data (optionally height map, control map, color map)
class CUndoTerrain : public IUndoStep
{
	CUndoTerrain() : m_initFinished(FALSE) {}
	DECLARE_ENGINE_CLASS( CUndoTerrain, IUndoStep, 0 );

private:
	struct SToolHistory
	{
		THandle< CTerrainTile >	m_subjectTile;
		Uint32					m_tileGridRow;
		Uint32					m_tileGridCol;

		Uint32					m_tileRes;				// Size of HM and CM data. Assumed to be square
		Uint32					m_colorRes;				// Size of Color data. Assumed to be square
		Uint32					m_colorMip;				// Mip level where data was taken from.

		// These buffers will hold either the pre- or post- data, depending on whether the UndoStep can be undone or redone.
		// Or NULL if that data channel was not included.
		Uint16*					m_hmData;
		TControlMapType*		m_cmData;
		TColorMapType*			m_colorData;

		Rect					m_strokesExtents;
		Rect					m_strokesExtentsColor;

		Bool					m_isCropped;

		ETerrainTileCollision	m_collisionType;
		Bool					m_haveCollisionType;

		Bool					m_valid;

		SToolHistory( Uint32 gridRow, Uint32 gridCol, CTerrainTile* tile );
		SToolHistory( SToolHistory&& other );
		~SToolHistory();

		void AddStroke( const Rect& strokeExtents, const Rect& strokeExtentsColor );

		void InitFromTile( Bool hm, Bool cm, Bool color, Bool collisionType );
		void CropData();
		void SwapWithTile();
	};

private:
	TDynArray<SToolHistory>	m_history;
	Bool					m_initFinished;

	String					m_toolName;

	Bool					m_useHM;
	Bool					m_useCM;
	Bool					m_useColor;
	Bool					m_useCollisionType;

protected:
	virtual void DoUndo();
	virtual void DoRedo();

	CUndoTerrain( CEdUndoManager& undoManager, const String& toolName );

public:
	virtual String GetName();

	// Create a new empty step. Only the data channels specified will be stored.
	static void CreateStep( CEdUndoManager* undoManager, const String& toolName, CClipMap* terrain, Bool hm, Bool cm, Bool color, Bool collisionType );
	// Crop data buffers to only include the stroke extents.
	static void FinishStep( CEdUndoManager* undoManager );

	// Add a stroke to the given tile. The first time this tile is added, its starting data will be grabbed into an internal buffer.
	static void AddStroke( CEdUndoManager* undoManager, CTerrainTile* tile, Uint32 gridRow, Uint32 gridCol, const Rect& strokeExtents, const Rect& strokeExtentsColor );

	virtual void OnSerialize( IFile& file );
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoTerrain, IUndoStep );

