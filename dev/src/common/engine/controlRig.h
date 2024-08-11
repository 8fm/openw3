
#pragma once

#include "controlRigIncludes.h"
#include "controlRigSolver.h"

class TCrPropertySet;

class TCrInstance
{
	friend class TCrDefinition;
	friend class TCrSolver;

	RedQsTransform			m_bonesLS[ TCrB_Last ];
	RedQsTransform			m_bonesMS[ TCrB_Last ];
	RedQsTransform			m_bonesWS[ TCrB_Last ];

	Bool					m_dirtyBonesLS[ TCrB_Last ];
	Bool					m_dirtyBonesMS[ TCrB_Last ];
	Bool					m_dirtyBonesWS[ TCrB_Last ];

	RedQsTransform			m_effectors[ TCrEffector_Last ];

	Float					m_effectorsPos[ TCrEffector_Last ];
	Float					m_effectorsRot[ TCrEffector_Last ];

	RedQsTransform			m_localToWorld;
	RedQsTransform			m_worldToLocal;

	const TCrPropertySet*	m_propertySet;

	Bool					m_poseSyncLS;
	Bool					m_poseSyncMS;
	Bool					m_poseSyncWS;

	Float					m_weaponOffsetForHandL;
	Float					m_weaponOffsetForHandR;

	TCrSolver				m_solver;

public:
	TCrInstance( const TCrPropertySet* propertySet );
	~TCrInstance();

	void Solve();

public:
	void SetLocalToWorld( const Matrix& l2w );

public:
	void SetBoneLS( ETCrBoneId bone, const RedQsTransform& transformLS );

	void GetBoneLS( ETCrBoneId bone, RedQsTransform& transformLS ) const;
	void GetBoneMS( ETCrBoneId bone, RedQsTransform& transformMS ) const;
	void GetBoneWS( ETCrBoneId bone, RedQsTransform& transformWS ) const;

public:
	void SetTranslationActive( ETCrEffectorId id, Float weight );
	void SetRotationActive( ETCrEffectorId id, Float weight );

	void SetEffectorWS( ETCrEffectorId id, const RedQsTransform& transformWS );
	void SetEffectorWS( ETCrEffectorId id, const Vector& positionWS );

	void GetEffectorWS( ETCrEffectorId id, RedQsTransform& transformWS ) const;
	Bool IsAnyEffectorSet() const;

#ifndef NO_EDITOR
	void GetEffectorDefaultWS( Int32 id, RedQsTransform& transformWS ) const;

	void DrawSkeleton( CRenderFrame *frame, Bool overlay );
#endif

	void ResetAllEffectors();

public:
	void SetWeaponOffsetForHandLeft( Float weight );
	void SetWeaponOffsetForHandRight( Float weight );

public:
	Bool IsSyncLS() const;
	Bool IsSyncMS() const;
	Bool IsSyncWS() const;

	void MarkSyncLS();
	void SyncLSFromMS();
	void SyncMSFromWS();
	void SyncMSFromLS();
	void SyncWSFromMS();

	const RedQsTransform* GetBonesLS() const;
	const RedQsTransform* GetBonesMS() const;
	const RedQsTransform* GetBonesWS() const;

public:
	void GenerateFragments( CRenderFrame* frame ) const;

private:
	RedQsTransform* AccessBonesLS();
	RedQsTransform* AccessBonesMS();
	RedQsTransform* AccessBonesWS();
};
