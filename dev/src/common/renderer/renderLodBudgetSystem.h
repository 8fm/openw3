/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderProxy_Mesh;

enum ELodManagementPolicy
{
	LDP_None,				// Do nothing with LODs
	LDP_AgressiveDrop,		// Drop to lowest possible LOD
	LDP_GentleDrop,			// Drop only one level (if possible)
	LDP_AdaptiveDrop,		// Drop levels basing on heuristics
	LDP_AgressiveEnhance,	// Enhance to highest possible LOD (eg. for cutscenes)
	LDP_MAX,
};

class CRenderLodBudgetSystem
{
private:
	static const Uint32		DEFAULT_BUDGET_CHUNK		= 85;
	static const Uint32		DEFAULT_BUDGET_TRIANGLE		= 30000;
	static const Uint32		DEFAULT_BUDGET_PROXY		= 5;
	static const Uint32		DEFAULT_FRAME_DELAY			= 30;

	Uint32					m_chunkBudget;
	Uint32					m_triangleBudget;
	Uint32					m_proxyBudget;
	Uint32					m_frameDelay;
	
	Uint32					m_prevCharacterChunks;
	Uint32					m_prevCharacterTriangles;
	Uint32					m_prevCharacterProxies;
	Uint32					m_currCharacterChunks;
	Uint32					m_currCharacterTriangles;
	Uint32					m_currCharacterProxies;

	Bool					m_on;
	Bool					m_initialized;
	Uint32					m_currentFrame;

	ELodManagementPolicy	m_lodManagementPolicy;

public:
	CRenderLodBudgetSystem( Uint32 chunkBudget = DEFAULT_BUDGET_CHUNK, Uint32 triangleBudget = DEFAULT_BUDGET_TRIANGLE, Uint32 proxyBudget = DEFAULT_BUDGET_PROXY, Uint32 frameDelay = DEFAULT_FRAME_DELAY );

	RED_INLINE Bool IsInitialized() const									{ return m_initialized;				}
	RED_INLINE Bool IsOverBudget() const									{ return /*m_currCharacterProxies >= m_prevCharacterProxies &&*/ m_currCharacterProxies >= m_proxyBudget;	}
	
	RED_INLINE Uint32 GetPreviousCharacterChunksCount() const				{ return m_prevCharacterChunks;		}
	RED_INLINE Uint32 GetCurrentCharacterChunksCount() const				{ return m_currCharacterChunks;		}
	RED_INLINE Uint32 GetPreviousCharacterTrianglesCount() const			{ return m_prevCharacterTriangles;	}
	RED_INLINE Uint32 GetCurrentCharacterTrianglesCount() const				{ return m_currCharacterTriangles;	}
	RED_INLINE Uint32 GetPreviousCharacterProxiesCount() const				{ return m_prevCharacterProxies;	}
	RED_INLINE Uint32 GetCurrentCharacterProxiesCount() const				{ return m_currCharacterProxies;	}

	RED_INLINE Uint32 GetChunkBudget() const								{ return m_chunkBudget;				}
	RED_INLINE void SetChunkBudget( Uint32 chunkBudget )					{ m_chunkBudget = chunkBudget;		}
	RED_INLINE Uint32 GetTriangleBudget() const								{ return m_triangleBudget;			}
	RED_INLINE void SetTriangleBudget( Uint32 triangleBudget )				{ m_triangleBudget = triangleBudget;}
	RED_INLINE Uint32 GetProxyBudget() const								{ return m_proxyBudget;				}
	RED_INLINE void SetProxyBudget( Uint32 proxyBudget )					{ m_triangleBudget = proxyBudget;	}

	RED_INLINE ELodManagementPolicy GetLodManagementPolicy() const		{ return m_lodManagementPolicy;		}
	RED_INLINE void SetLodManagementPolicy( ELodManagementPolicy policy)	{ m_lodManagementPolicy = policy;	}
	
	RED_INLINE void DecreaseChunksCount( Uint32 val )						{ m_currCharacterChunks -= val;		}

	RED_INLINE void ToggleSwitch()										{ m_on = !m_on;						}
	RED_INLINE void Toggle( Bool turnedOn )								{ m_on = turnedOn;					}
	RED_INLINE Bool IsOn()												{ return m_on;						}

	void CollectVisibleCharactersInfo();
	void Tick( const CRenderCollector& collector );
	void CalculateLodCorrection( const CRenderProxy_Mesh* mesh, Int32& targetLodIndex );

	Bool				justTicked;
};