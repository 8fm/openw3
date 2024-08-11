
#pragma once

#include "../../common/engine/visualDebug.h"

class CWaterDebug : public CObject, IVisualDebugInterface
{
	DECLARE_ENGINE_CLASS( CWaterDebug, CObject, 0 );

	CActor*		m_center;
	Float		m_length;
	Int32			m_divides;
	Bool		m_isAdded;

public:
	CWaterDebug();

	void Init( Float length, Int32 divides, CActor* center );
	void Reset();

	virtual void Render( CRenderFrame* frame, const Matrix& matrix );

private:
	Float GetWaterLevel( Float x, Float y ) const;

private:
	void funcReset( CScriptStackFrame& stack, void* result );
	void funcInit( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CWaterDebug );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "Init", funcInit );
	NATIVE_FUNCTION( "Reset", funcReset );
END_CLASS_RTTI();
