/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/flashReference.h"
#include "../../common/engine/flashValue.h"
#include "../../common/engine/flashFunction.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovie;

//////////////////////////////////////////////////////////////////////////
// CFlashMovieAdapter
//////////////////////////////////////////////////////////////////////////
class CFlashMovieAdapter : public IFlashReference
{
public:
	typedef THashMap< const Char*, CFlashFunction* >			TFlashFunctionMap;

private:
	typedef	Flash::TFlashFuncPtr< CFlashMovieAdapter >::Type	TFlashFunctionExport;

private:
	struct SFlashFuncExportDesc
	{
		const Char*								m_memberName;
		TFlashFunctionExport					m_flashFuncExport;
		Bool									m_allowMissingExport;
	};

private:
	static SFlashFuncExportDesc					sm_flashFunctionExportTable[];

private:
	TFlashFunctionMap							m_flashFunctionMap;

	CFlashMovie*								m_flashMovie;

public:
												CFlashMovieAdapter( CFlashMovie* flashMovie );
	virtual										~CFlashMovieAdapter();
	virtual Bool								Init();
	virtual void								OnDestroy() override;

public:
	RED_INLINE CFlashMovie*					GetFlashMovie() const { return m_flashMovie; }
	RED_INLINE const TFlashFunctionMap&		GetFunctionMap() const { return m_flashFunctionMap; }

private:
	void										InitFlashFunctions();
	void										DiscardFlashFunctions();
	CFlashFunction*								CreateFlashFunction( TFlashFunctionExport flashFunc );

private:
	void										flFnRegisterDataBinding( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void										flFnUnregisterDataBinding( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void										flFnRegisterChild( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void										flFnUnregisterChild( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void										flFnCallGameEvent( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void										flFnRegisterRenderTarget( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
	void										flFnUnregisterRenderTarget( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args );
};
