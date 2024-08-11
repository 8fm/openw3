
#pragma once

#include "../engine/behaviorGraphPointCloudLookAtNode.h"
#include "../engine/behaviorGraphMimicConverter.h"

namespace StorySceneEventsCollector
{
	struct ActorLookAt;
	struct ActorGameplayLookAt;
}

class CStorySceneLookAtController
{
	static const CName& LOOK_AT_NODE_NAME_A;
	static const CName& LOOK_AT_NODE_NAME_B;

	static const CName& LOOK_AT_ON_GMPL_ANIMAL;		// EP1 support
	static const CName& LOOK_AT_TARGET_GMPL_ANIMAL; // EP1 support

	struct TargetPoint
	{
		THandle< CNode >	m_node;
		Int32				m_bone;
		Vector				m_point;
		Bool				m_pointWasChanged;
		Bool				m_enabled;

		TargetPoint() 
		{ 
			Reset(); 
		}

		void Reset()
		{
			m_enabled = false;
			m_pointWasChanged = false;
			ResetTarget();
		}

		void ResetTarget()
		{
			m_bone = -1;
			m_node = nullptr;
			m_point = Vector::ZERO_3D_POINT;
		}

		Bool IsEqual( const TargetPoint& rhs ) const
		{
			const CNode* nodeA = m_node.Get();
			const CNode* nodeB = rhs.m_node.Get();

			if ( nodeA && nodeB )
			{
				if ( m_bone != -1 )
				{
					return nodeA == nodeB && m_bone == rhs.m_bone;
				}
				else
				{
					return nodeA == nodeB;
				}
			}

			return Vector::Near3( m_point, rhs.m_point );
		}
	};

	struct Target
	{
		const void*			m_id;
		TargetPoint			m_targetPoint;
		Bool				m_headLevel;
		Float				m_bodyWeight;

		Target()
		{ 
			Reset();
			ResetId();
		}

		void Reset()
		{
			m_headLevel = false;
			m_bodyWeight = 0.f;
			m_targetPoint.Reset();
		}

		void ResetId()
		{
			m_id = nullptr;
		}
	};

	enum EMode
	{
		M_PointCloud,
		M_Gameplay,
	};

private:
	EMode								m_mode;

	CBehaviorPointCloudLookAtInterface	m_lookAtNodeA;
	CBehaviorPointCloudLookAtInterface	m_lookAtNodeB;
	Bool								m_lookAtNodesAvailable;
	CBehaviorGraphMimicsLookAtMediator	m_lookAtEyesMediator;

	Target								m_bodyTargetA;
	Target								m_bodyTargetB;
	Bool								m_bodyDataWasChanged;
	Float								m_progress;
	Float								m_duration;
	Bool								m_useDeformationMS;

	Float								m_eyesLookAtConvergenceWeight;
	Bool								m_eyesLookAtIsAdditive;
	Float								m_eyesLookAtDampScale;
	Float								m_eyesTransitionFactor;

	CName								m_behaviorVarWeight;
	CName								m_behaviorVarTarget;

	THandle< CEntity >					m_entityLookAtOwner;
	CActor*								m_actorLookAtOwner;

	Bool								m_resetRequestMarker;
	Bool								m_resetRequestConsumed;

	Bool								m_blockBlink;
	CGUID								m_blockBlinkId;

public:
	CStorySceneLookAtController();

	void Init( CEntity* e );
	void Deinit();

public:
	void Update();

	Bool SetPointCloud( const StorySceneEventsCollector::ActorLookAt& evt, const CEntity* bodyTarget, const CEntity* eyesTarget, const Vector& bodyStaticPointWS );
	void SetGameplay( const StorySceneEventsCollector::ActorGameplayLookAt& evt, const CEntity* bodyTarget, const Vector& bodyStaticPointWS );

	void SyncWithIdle( const CName& lookAtBodyPrev, const CName& lookAtHeadPrev, const CName& lookAtBodyCurr, const CName& lookAtHeadCurr );

	void NotifyTargetWasChanged();

public: // customs
	void SetLowerBodyPartsWeight( Float w );

	Vector GetApproximateTargetWS() const;
	Float GetApproximateHorAngleDegToTargetLS() const;

	void BlockBlink( const CGUID& id );
	void UnblockBlink();
	Bool IsBlinkBlocked( const CGUID& id ) const;

private:
	void ChangeMode( EMode mode );
	void CacheSlots();
	void CheckSlots();

	void Reset();
	void MarkReset( Bool flag );
	void ConsumeReset();

	void SetupTarget( CStorySceneLookAtController::Target& target, const StorySceneEventsCollector::ActorLookAt& evt, const CEntity* targetEntity, const Vector& bodyStaticPointWS ) const;
	void SetupTarget( CStorySceneLookAtController::Target& target, const StorySceneEventsCollector::ActorGameplayLookAt& evt, const CEntity* targetEntity, const Vector& bodyStaticPointWS ) const;
	void SetupTargetPoint( TargetPoint& target, const Vector& point, const CEntity* targetEntity ) const;
	Bool UpdateTarget( TargetPoint& target);
	void UpdateTarget( TargetPoint& target, CBehaviorPointCloudLookAtInterface& lookAt, Bool targetB );
	void SendDataToLookAt( CBehaviorPointCloudLookAtInterface& lookAt );
	void SendDataToGameplayLookAt( Float weight, const Vector& targetWS );
	void SendDataToGameplayLookAt();
	void SendDataToEyesLookAt();

	void ResetPointCloudLookAtNode();
	void ResetGameplayLookAtNode();

	Bool ShouldBlendParams( const Target& targetA, const Target& targetB ) const;
	Float CalcBodyWeight( const Target& targetA, const Target& targetB, Float progress ) const;
	Float CalcHeadBlendWeight( const Target& targetA, const Target& targetB, Float progress ) const;
	Float CalcTargetBlendWeight( const Target& targetA, const Target& targetB, Float progress ) const;
	void CalcApproximateWeightAndTarget( Float& w, Vector& vec ) const;
	void CalcApproximateWeightAndTargetForGameplay( Float& w, Vector& vec ) const;
};
