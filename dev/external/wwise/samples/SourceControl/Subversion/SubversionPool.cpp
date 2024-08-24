//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "SubversionPool.h"

SvnPool::SvnPool( apr_pool_t* in_pParentPool )
{
	m_pPool = svn_pool_create( in_pParentPool );
}

SvnPool::~SvnPool()
{
	svn_pool_destroy( m_pPool );
}

void SvnPool::Clear()
{
	svn_pool_clear( m_pPool );
}

SvnPool::operator apr_pool_t*()
{
	return m_pPool;
}