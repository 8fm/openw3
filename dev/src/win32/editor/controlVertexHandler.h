
#pragma once

class IEditorNodeMovementHook;

class CEdControlVertexHandler 
{
public:
	CEdControlVertexHandler( Bool rotatable, Bool scalable );

	void DestroyVertexEntities();

	void RebuildVertexEntities();

protected:

	struct ControlVertex
	{
		ControlVertex( IEditorNodeMovementHook* controlledObject, Uint32 id,
					   const Vector& position, const EulerAngles& rotation, Float scale = 1.0f );

		IEditorNodeMovementHook* m_controlledObject;
		Uint32      m_id;
		Vector      m_position;
		EulerAngles m_rotation;
		Float       m_scale;
	};

	virtual void GetControlVertices( TDynArray< ControlVertex >& vertices ) =0;

private:
	Bool    m_rotatable;
	Bool    m_scalable;
	TDynArray< CVertexEditorEntity* > m_verticies;
};
