#pragma once

#include "../../common/game/swarmLairEntity.h"
#include "../../common/game/swarmUpdateJob.h"


class CFlyingCrittersAlgorithmData;
class CFlyingCritterLairParams;
class CFlyingSwarmScriptInput;

////////////////////////////////////////////////////////////////
/// Boid class for all creatures that move on the ground 
class CFlyingCrittersLairEntity : public CSwarmLairEntity
{
	DECLARE_ENGINE_CLASS( CFlyingCrittersLairEntity, CSwarmLairEntity, 0 );

	friend class CFlyingCrittersAlgorithmData;
public:



	CFlyingCrittersLairEntity();

	// CSwarmLairEntity virtual functions
	void	OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )override;
	void	OnAttachFinished( CWorld* world )override;
	// Called pre deletion from editor
#ifndef NO_EDITOR
	void	EditorPreDeletion() override;
#endif

	void NoticeFireInCone( const Vector& position, const Vector2& coneDir, Float coneHalfAngle, Float coneRange );

	volatile CFlyingCrittersAlgorithmData* GetAlgorithmData() const volatile;

	// Script :
	void OnBoidPointOfInterestReached( Uint32 count, CEntity *const entity, Float deltaTime )override{}

protected:
	// CSwarmLairEntity Interface
	void					OnTick( Float timeDelta ) override;
	CSwarmUpdateJob*		NewUpdateJob() override;
	CSwarmAlgorithmData*	NewAlgorithmData() override;
	void					DeactivateLair() override;
	void					SynchroniseData()override;

	// my interface
	virtual void OnScriptTick( CFlyingSwarmScriptInput & flyingGroupList, Bool isActive, Float deltaTime ){}

	Uint32					m_breakCounter;

	// soft handle on the cellmap resource file
	TSoftHandle< CSwarmCellMap >	m_cellMapResourceFile;
	// When generating the cell map we need to know what is the cell size
	Float							m_cellMapCellSize;
	

	////////////////////////////////////////////////////////////
	// Here more custom, and less readable shit starts.
public:
	struct SFireInConeInfo
	{
		SFireInConeInfo()
			: m_isPending( false )								{}
		Bool				m_isPending;
		Bool				m_isPointOfInterestActive;
		Vector3				m_origin;
		Vector2				m_dir;
		Float				m_halfAngle;
		Float				m_range;
	};

	const SFireInConeInfo& GetFireInConeInfo() const			{ return m_fireInConeInfo; }
	void DisposePendingFireInConeInfo()							{ m_fireInConeInfo.m_isPending = false; }
	
#ifndef NO_EDITOR
	Bool OnGenerateSwarmCollisions();
#endif
protected:
	SFireInConeInfo					m_fireInConeInfo;
	CFlyingSwarmScriptInput *		m_scriptInput;
	Bool							m_cellMapSet;
};

BEGIN_CLASS_RTTI( CFlyingCrittersLairEntity )
	PARENT_CLASS( CSwarmLairEntity )
	PROPERTY_NOSERIALIZE( m_scriptInput )
	PROPERTY( m_cellMapResourceFile )
	PROPERTY_EDIT_RANGE( m_cellMapCellSize, TXT("The lower the more precise the collisions will be but the more memory it will take [ 1.0f, 5.0f ]"), 1.0f, 5.0f )
END_CLASS_RTTI()


