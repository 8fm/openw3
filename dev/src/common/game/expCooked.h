/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CCookedExploration : public IExploration
{
private:
	Matrix				m_mat;				// hack, to be removed
	Matrix				m_parentMat;		// hack, to be removed
	Vector3				m_start;			// temp, should be kept in WS
	Vector3				m_end;				// temp, should be kept in WS
	Int8				m_explorationId;	

public:
	CCookedExploration() {}
	CCookedExploration( CExplorationComponent* component );

	Uint32 ComputeDataSize() const; 
	void Serialize( IFile& file );

	virtual void GetMatWS( Matrix & mat ) const;			// hack, to be removed
	virtual void GetParentMatWS( Matrix& mat ) const;		// hack, to be removed
	virtual void GetEdgeWS( Vector& p1, Vector& p2 ) const;
	virtual void GetNormal( Vector& n ) const; // WS
	virtual Int32 GetId() const;
	virtual CObject* GetObjectForEvents() const;
};

class CCookedExplorations : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CCookedExplorations, CResource, "redexp", "Cooked explorations" );

public:
	DataBuffer* m_data;

	CCookedExplorations() : m_data( nullptr ) {}
	~CCookedExplorations() { delete m_data; }
	virtual void OnSerialize( class IFile& file );
};

BEGIN_CLASS_RTTI( CCookedExplorations )
	PARENT_CLASS( CResource )
END_CLASS_RTTI()