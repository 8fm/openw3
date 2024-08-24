//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "svn_pools.h"

class SvnPool
{
public:
	SvnPool( apr_pool_t* in_pParentPool );
	~SvnPool();

	void Clear();

	operator apr_pool_t*();

private:

	apr_pool_t* m_pPool;
};