
#pragma once

class IPreviewGhost
{
public:
	virtual void UpdatePose( const CAnimatedComponent* component ) {}
	virtual void Update( Float dt ) {}
	virtual void Draw( CRenderFrame* frame, const CAnimatedComponent* component ) {}
	virtual void SetActivation( Float val ) {}
	virtual ~IPreviewGhost() {}
};

class PreviewGhostSkeleton : public IPreviewGhost
{
	Float				m_activation;
	Color				m_color;
	TDynArray< Matrix > m_pose;

public:
	PreviewGhostSkeleton();

	virtual void UpdatePose( const CAnimatedComponent* component );
	virtual void Update( Float dt );
	virtual void Draw( CRenderFrame* frame, const CAnimatedComponent* component );
	virtual void SetActivation( Float val );
};

class PreviewGhostEntity : public IPreviewGhost
{
	Bool							m_isReady;
	Bool							m_visibility;
	THandle< CAnimatedComponent >	m_componentH;
	const CAnimatedComponent*		m_template;

public:
	PreviewGhostEntity();
	~PreviewGhostEntity();

	virtual void UpdatePose( const CAnimatedComponent* component );
	virtual void SetActivation( Float val );

private:
	void RecreateEntity( const CAnimatedComponent* component );
	void SyncEntityTo( const CAnimatedComponent* component );
};

//////////////////////////////////////////////////////////////////////////

class PreviewGhostContainer
{
public:
	enum EGhostType
	{
		GhostType_Entity = 0,
		GhostType_Skeleton,

		GhostType_Max
	};

private:
	Float						m_timeStep;

	Uint32						m_currGhost;
	Float						m_accTime;

	TDynArray< IPreviewGhost* > m_ghosts;

	EGhostType					m_currentType;

public:
	PreviewGhostContainer();

	void InitGhosts( Uint32 numberOfGhosts, EGhostType ghostType );
	void DestroyGhosts();
	Bool HasGhosts() const;

	void UpdateGhosts( Float dt, const CAnimatedComponent* component );

	void Reset();

	void Draw( CRenderFrame *frame, const CAnimatedComponent* component );

	void SetTimeStep( Float time );
};
