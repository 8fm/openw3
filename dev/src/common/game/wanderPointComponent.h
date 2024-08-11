#pragma once

#include "wayPointComponent.h"

class CWanderPointSelector;
class CWanderData;


struct SWanderPointConnection
{
	DECLARE_RTTI_STRUCT( SWanderPointConnection );

	SWanderPointConnection()
		: m_forcePathfinding( false )														{}

	EntityHandle				m_destination;
	Bool						m_forcePathfinding;
};

BEGIN_CLASS_RTTI( SWanderPointConnection )
	PROPERTY_EDIT( m_destination, TXT("Destination wander point") )
	PROPERTY_EDIT( m_forcePathfinding, TXT("Force pathfinding between points. Enables road use.") )
END_CLASS_RTTI()


class CWanderPointComponent : public CWayPointComponent
{
	DECLARE_ENGINE_CLASS( CWanderPointComponent, CWayPointComponent, 0 )

protected:
	TDynArray< SWanderPointConnection >	m_connectedPoints;
	Float								m_wanderPointRadius;
#ifndef NO_EDITOR
	enum EDebugLazyStatus
	{
		DS_NOT_CALCULATED,
		DS_OK,
		DS_INVALID
	};
	EDebugLazyStatus				m_wanderAreaIsAvailable;
	TDynArray< EDebugLazyStatus >	m_connectionsAvailable;
#endif

public:
	CWanderPointComponent()
		: CWayPointComponent()
		, m_wanderPointRadius( 0.f )
#ifndef NO_EDITOR
		, m_wanderAreaIsAvailable( DS_NOT_CALCULATED )
#endif
	{ 
	}

#ifndef NO_EDITOR
	virtual void WaypointGenerateEditorFragments( CRenderFrame* frame ) override;
	virtual void EditorOnTransformChanged() override;
	virtual void EditorPreDeletion() override;
	virtual void EditorPostDuplication( CNode* originalNode ) override;

	void MarkCachedNavtestsDirty();
#endif

	virtual void ChangeLinks( Bool generate, Bool twoSideRemoval = false );

	virtual void OnPropertyPreChange( IProperty* property ) override;
	virtual void OnPropertyPostChange( IProperty* property ) override;

	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;


	Bool IsConnectedTo( CEntity* wanderPoint, Int32& index );
	Bool AddConnection( CEntity* wanderPoint, Bool forcePathfind );
	Bool RemoveConnection( CEntity* wanderPoint, Bool bothSides );
	void RemoveAllConnections();
	Uint32 GetConnectionsSize() const														{ return m_connectedPoints.Size(); }
	CEntity* GetConnection( Uint32 index )													{ return m_connectedPoints[ index ].m_destination.Get(); }
	Bool GetConnectionForcePathfinding( Uint32 index ) const								{ return m_connectedPoints[ index ].m_forcePathfinding; }

	Float GetWanderpointRadius() const														{ return m_wanderPointRadius; }

protected:
	static IRenderResource*		s_markerValid;
	static IRenderResource*		s_markerInvalid;
	static IRenderResource*		s_markerNoMesh; 
	static IRenderResource*		s_markerSelection;

	void InitializePointMarkers();

	virtual IRenderResource* GetMarkerValid() override;
	virtual IRenderResource* GetMarkerInvalid() override;
	virtual IRenderResource* GetMarkerNoMesh() override;
	virtual IRenderResource* GetMarkerSelection() override;
};

BEGIN_CLASS_RTTI( CWanderPointComponent )
	PARENT_CLASS( CWayPointComponent )
	PROPERTY_EDIT( m_connectedPoints, TXT("Connected points that can be traversed") )
	PROPERTY_EDIT( m_wanderPointRadius, TXT("Wander point logical radius") )
END_CLASS_RTTI()
