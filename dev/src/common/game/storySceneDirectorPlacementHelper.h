/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CStoryScenePlayer;

// DIALOG_TOMSIN_TODO - remove this class
class CStorySceneDirectorPlacementHelper
{
private:
	THashMap< CName, EngineTransform >				m_safePlacements;	//<! setting name , safe placement transform

	const THashMap< CName, THandle< CEntity > >*	m_pivotActors;
	CNode*										m_initialPlacementFallbackNode;

	const CStoryScenePlayer*					m_directorParent;

public:
	CStorySceneDirectorPlacementHelper();

	void Init( const CStoryScenePlayer* directorParent );
	Bool FindUnsafePlacementFallback( const TDynArray< Vector > & convexHull, const Vector & initialPosition, Vector & retPosition, const CWorld* gameWorld, Float maxRadius, Float radiusStep , Float slotExtent   );
	Float	SnapPlacement( EngineTransform& placement, const CWorld* world );

public:
	void	FindDialogsetPlacement( const CStorySceneDialogsetInstance* dialogset, EngineTransform& placement ) const;
	void	GetSlotPlacement( const CStorySceneDialogsetSlot* slot, const CStorySceneDialogsetInstance* dialogset, EngineTransform& placement ) const;

public:
	void CleanupPlacements();
	void SetPivotActors( const THashMap< CName, THandle< CEntity > >* pivotActors ) { m_pivotActors = pivotActors; }
	void SetInitialPlacementFallbackNode( CNode* fallbackNode ) { m_initialPlacementFallbackNode = fallbackNode; }
};
