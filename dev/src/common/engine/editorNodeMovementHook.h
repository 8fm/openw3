/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class IEditorNodeMovementHook
{
public:
	virtual ~IEditorNodeMovementHook(){}

	//! called before a batch of transformations is about to start (like after starting to drag a node with a mouse)
	virtual void OnEditorNodeTransformStart( Int32 /*vertexIndex*/ ) {}

	virtual void OnEditorNodeMoved  ( Int32 /*vertexIndex*/, const Vector& /*oldPosition*/, const Vector& /*wishedPosition*/, Vector& /*allowedPosition*/ ) {}

	virtual void OnEditorNodeRotated( Int32 /*vertexIndex*/, const EulerAngles& /*oldRotation*/, const EulerAngles& /*wishedRotation*/, EulerAngles& /*allowedRotation*/ ) {}

	virtual void OnEditorNodeScaled ( Int32 /*vertexIndex*/, const Vector& /*oldScale*/, const Vector& /*wishedScale*/, Vector& /*allowedScale*/ ) {}
	
	//! called after a batch of transformations is ended (like after releasing a mouse button after dragging)
	virtual void OnEditorNodeTransformStop( Int32 /*vertexIndex*/ ) {}
};
