
#pragma once

class Perlin
{
public:
	static const Uint32 SAMPLE_SIZE = 1024;

	Perlin( Int32 octaves, Float freq, Float amp );

	Float Get( Float x, Float y)
	{
		Float vec[2];
		vec[0] = x;
		vec[1] = y;
		return PerlinNoise2D(vec);
	};

	void Reinit()
	{
		Init();
	}

private:
	void InitPerlin( Int32 n, Float p );
	Float PerlinNoise2D( Float vec[2] );

	Float Noise1( Float arg );
	float Noise2( Float vec[2] );
	Float Noise3( Float vec[3] );

	void Normalize2( Float v[2] );
	void Normalize3( Float v[3] );

	void Init();
	void CheckStart();

	Int32		m_octaves;
	Float	m_frequency;
	Float	m_amplitude;

	Int32		p[SAMPLE_SIZE + SAMPLE_SIZE + 2];
	Float	g3[SAMPLE_SIZE + SAMPLE_SIZE + 2][3];
	Float	g2[SAMPLE_SIZE + SAMPLE_SIZE + 2][2];
	Float	g1[SAMPLE_SIZE + SAMPLE_SIZE + 2];
	Bool	m_start;
};
