#pragma once

enum EPlatforFeature
{
	PF_Kinect
};

class CPlatformFeature
{

public:
	CPlatformFeature( EPlatforFeature type ): m_type(type){}
	virtual ~CPlatformFeature(){}

private:
	EPlatforFeature m_type;
};

class CPlatformFeatureManager
{

public:
	CPlatformFeatureManager(){}
	virtual ~CPlatformFeatureManager()
	{
		THashMap<Uint32, CPlatformFeature*>::iterator end = m_platformFeature.End();
		for( THashMap<Uint32, CPlatformFeature*>::iterator iter = m_platformFeature.Begin(); iter != end; ++iter  )
		{
			delete iter->m_second;
		}
		m_platformFeature.Clear();
	}

public:

	void SetFeature( EPlatforFeature feature, CPlatformFeature* featurePtr )
	{
		m_platformFeature.Set( feature, featurePtr );
	}

	CPlatformFeature* GetFeature( EPlatforFeature feature )
	{
		THashMap<Uint32, CPlatformFeature*>::iterator foundFeature = m_platformFeature.Find(feature);
		if( foundFeature != m_platformFeature.End() )
		{
			return foundFeature->m_second;
		}
		return NULL;
	}

	Bool RemFeature( EPlatforFeature feature )
	{
		THashMap<Uint32, CPlatformFeature*>::iterator foundFeature = m_platformFeature.Find(feature);
		if( foundFeature != m_platformFeature.End() )
		{
			return m_platformFeature.Erase( feature );
		}
		return false;
	}

private:
	THashMap< Uint32, CPlatformFeature*> m_platformFeature;
};

typedef TSingleton< CPlatformFeatureManager, TDefaultLifetime, TCreateUsingNew > SPlatformFeatureManager;