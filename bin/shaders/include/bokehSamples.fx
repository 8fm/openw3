//////////////////////////////////////////////////////////
// If you want to use higher quality samples 
// count (61) define BOKEH_SUPER_DUPER_SAMPLES_LVL as 1 . 
// 0 otherwise
//////////////////////////////////////////////////////////
// Samples are float4 due to xy the base position and zw
// that is an offset to get circle shape (hex is default)
//////////////////////////////////////////////////////////

// [[162.246 -27.9373 0.709058 1.50002|-360.399 7743.93 -0]] - SUNSET

// Lower quality samples
#if BOKEH_SUPER_DUPER_SAMPLES_LVL == 0

#define BOKEH_SAMPLE_COUNT 37

static const float4 samples [BOKEH_SAMPLE_COUNT] =
{
	float4( -0.25f, -0.433f, -1.847744E-6f, -3.1888485E-6f ),
	float4( -0.08333331f, -0.433f, -0.011157677f, -0.057975322f ),
	float4( 0.08333334f, -0.433f, 0.011157684f, -0.057975322f ),
	float4( 0.25f, -0.433f, 1.847744E-6f, -3.1888485E-6f ),
	float4( -0.3333333f, -0.28866664f, -0.044632226f, -0.038651496f ),
	float4( -0.16666664f, -0.28866664f, -1.2367964E-6f, -2.1457672E-6f ),
	float4( 1.4901161E-8f, -0.28866664f, 2.3052174E-9f, -0.044656903f ),
	float4( 0.16666667f, -0.28866664f, 1.2367964E-6f, -2.1457672E-6f ),
	float4( 0.33333337f, -0.28866664f, 0.044632226f, -0.038651496f ),
	float4( -0.41666666f, -0.14433332f, -0.05578807f, -0.019324988f ),
	float4( -0.24999997f, -0.14433332f, -0.03867513f, -0.022328451f ),
	float4( -0.08333332f, -0.14433332f, -6.183982E-7f, -1.0728836E-6f ),
	float4( 0.083333336f, -0.14433332f, 6.183982E-7f, -1.0728836E-6f ),
	float4( 0.25000003f, -0.14433332f, 0.03867516f, -0.022328451f ),
	float4( 0.41666672f, -0.14433332f, 0.0557881f, -0.019324988f ),
	float4( -0.5f, 0.0f, 0.0f, 0.0f ),
	float4( -0.3333333f, 0.0f, 0.0f, 0.0f ),
	float4( -0.16666666f, 0.0f, 0.0f, 0.0f ),
	float4( 0.0f, 0.0f, 0.0f, 0.0f ),
	float4( 0.16666669f, 0.0f, 0.0f, 0.0f ),
	float4( 0.33333337f, 0.0f, 0.0f, 0.0f ),
	float4( 0.5f, 0.0f, 0.0f, 0.0f ),
	float4( -0.41666666f, 0.14433335f, -0.05578807f, 0.019324988f ),
	float4( -0.24999997f, 0.14433335f, -0.03867513f, 0.022328451f ),
	float4( -0.08333332f, 0.14433335f, -6.03497E-7f, 1.0430813E-6f ),
	float4( 0.083333336f, 0.14433335f, 6.03497E-7f, 1.0430813E-6f ),
	float4( 0.25000003f, 0.14433335f, 0.03867516f, 0.022328451f ),
	float4( 0.41666672f, 0.14433335f, 0.0557881f, 0.019324988f ),
	float4( -0.3333333f, 0.28866664f, -0.044632196f, 0.038651496f ),
	float4( -0.16666664f, 0.28866664f, -1.2367964E-6f, 2.1457672E-6f ),
	float4( 1.4901161E-8f, 0.28866664f, 2.3052174E-9f, 0.044656903f ),
	float4( 0.16666667f, 0.28866664f, 1.2367964E-6f, 2.1457672E-6f ),
	float4( 0.33333337f, 0.28866664f, 0.044632196f, 0.038651496f ),
	float4( -0.25f, 0.433f, -1.847744E-6f, 3.1888485E-6f ),
	float4( -0.08333331f, 0.433f, -0.011157684f, 0.05797535f ),
	float4( 0.08333334f, 0.433f, 0.0111576915f, 0.05797535f ),
	float4( 0.25f, 0.433f, 1.847744E-6f, 3.1888485E-6f ),
};
#endif // BOKEH_SUPER_DUPER_SAMPLES_LVL == 0

// Higher quality samples
#if BOKEH_SUPER_DUPER_SAMPLES_LVL == 1

#define BOKEH_SAMPLE_COUNT 61

static const float4 samples [BOKEH_SAMPLE_COUNT] =
{
	float4( -0.25f, -0.433f, -1.847744E-6f, -3.1888485E-6f ),
	float4( -0.125f, -0.433f, -0.013674736f, -0.0473693f ),
	float4( 0.0f, -0.433f, 0.0f, -0.06698534f ),
	float4( 0.125f, -0.433f, 0.013674736f, -0.0473693f ),
	float4( 0.25f, -0.433f, 1.847744E-6f, -3.1888485E-6f ),
	float4( -0.3125f, -0.32475f, -0.034189105f, -0.035529315f ),
	float4( -0.1875f, -0.32475f, -1.385808E-6f, -2.413988E-6f ),
	float4( -0.0625f, -0.32475f, -0.008368261f, -0.0434815f ),
	float4( 0.0625f, -0.32475f, 0.008368261f, -0.0434815f ),
	float4( 0.1875f, -0.32475f, 1.385808E-6f, -2.413988E-6f ),
	float4( 0.3125f, -0.32475f, 0.034189105f, -0.035529315f ),
	float4( -0.375f, -0.2165f, -0.058012724f, -0.03349267f ),
	float4( -0.25f, -0.2165f, -0.033474177f, -0.02898863f ),
	float4( -0.125f, -0.2165f, -9.23872E-7f, -1.5944242E-6f ),
	float4( 0.0f, -0.2165f, 0.0f, -0.03349267f ),
	float4( 0.125f, -0.2165f, 9.23872E-7f, -1.5944242E-6f ),
	float4( 0.25f, -0.2165f, 0.033474177f, -0.02898863f ),
	float4( 0.375f, -0.2165f, 0.058012724f, -0.03349267f ),
	float4( -0.4375f, -0.10825f, -0.047861725f, -0.011842355f ),
	float4( -0.3125f, -0.10825f, -0.04184106f, -0.014493741f ),
	float4( -0.1875f, -0.10825f, -0.029006362f, -0.016746335f ),
	float4( -0.0625f, -0.10825f, -4.61936E-7f, -7.972121E-7f ),
	float4( 0.0625f, -0.10825f, 4.61936E-7f, -7.972121E-7f ),
	float4( 0.1875f, -0.10825f, 0.029006362f, -0.016746335f ),
	float4( 0.3125f, -0.10825f, 0.04184106f, -0.014493741f ),
	float4( 0.4375f, -0.10825f, 0.047861725f, -0.011842355f ),
	float4( -0.5f, 0.0f, 0.0f, 0.0f ),
	float4( -0.375f, 0.0f, 0.0f, 0.0f ),
	float4( -0.25f, 0.0f, 0.0f, 0.0f ),
	float4( -0.125f, 0.0f, 0.0f, 0.0f ),
	float4( 0.0f, 0.0f, 0.0f, 0.0f ),
	float4( 0.125f, 0.0f, 0.0f, 0.0f ),
	float4( 0.25f, 0.0f, 0.0f, 0.0f ),
	float4( 0.375f, 0.0f, 0.0f, 0.0f ),
	float4( 0.5f, 0.0f, 0.0f, 0.0f ),
	float4( -0.4375f, 0.10825f, -0.047861725f, 0.011842355f ),
	float4( -0.3125f, 0.10825f, -0.04184106f, 0.014493741f ),
	float4( -0.1875f, 0.10825f, -0.029006362f, 0.016746335f ),
	float4( -0.0625f, 0.10825f, -4.61936E-7f, 7.972121E-7f ),
	float4( 0.0625f, 0.10825f, 4.61936E-7f, 7.972121E-7f ),
	float4( 0.1875f, 0.10825f, 0.029006362f, 0.016746335f ),
	float4( 0.3125f, 0.10825f, 0.04184106f, 0.014493741f ),
	float4( 0.4375f, 0.10825f, 0.047861725f, 0.011842355f ),
	float4( -0.375f, 0.2165f, -0.058012724f, 0.03349267f ),
	float4( -0.25f, 0.2165f, -0.033474147f, 0.028988615f ),
	float4( -0.125f, 0.2165f, -9.23872E-7f, 1.5944242E-6f ),
	float4( 0.0f, 0.2165f, 0.0f, 0.03349267f ),
	float4( 0.125f, 0.2165f, 9.23872E-7f, 1.5944242E-6f ),
	float4( 0.25f, 0.2165f, 0.033474147f, 0.028988615f ),
	float4( 0.375f, 0.2165f, 0.058012724f, 0.03349267f ),
	float4( -0.3125f, 0.32475f, -0.034189105f, 0.035529315f ),
	float4( -0.1875f, 0.32475f, -1.385808E-6f, 2.413988E-6f ),
	float4( -0.0625f, 0.32475f, -0.008368269f, 0.0434815f ),
	float4( 0.0625f, 0.32475f, 0.008368269f, 0.0434815f ),
	float4( 0.1875f, 0.32475f, 1.385808E-6f, 2.413988E-6f ),
	float4( 0.3125f, 0.32475f, 0.034189105f, 0.035529315f ),
	float4( -0.25f, 0.433f, -1.847744E-6f, 3.1888485E-6f ),
	float4( -0.125f, 0.433f, -0.013674736f, 0.0473693f ),
	float4( 0.0f, 0.433f, 0.0f, 0.06698534f ),
	float4( 0.125f, 0.433f, 0.013674736f, 0.0473693f ),
	float4( 0.25f, 0.433f, 1.847744E-6f, 3.1888485E-6f ),
};

#endif // BOKEH_SUPER_DUPER_SAMPLES_LVL == 1

// Samples used to blur CoC
// Saved as float3 - need XY for sampling and 
/*
#if 0

	math.randomseed( os.time() )

	local  numSamples = 16

	io.write("#define COC_SAMPLE_BLUR_COUNT " .. numSamples .. "\n\n")
	io.write("static const float3 samplesCoCBlur[COC_SAMPLE_BLUR_COUNT] = {\n")

	local x = 0.0
	local y = 0.0
	local s = 0.0
	local l = 0.0

	for i = 1,numSamples do

		repeat

			x = math.random() * 2 - 1
			y = math.random() * 2 - 1

			l = math.sqrt( x*x + y*y )

		until l > 0.001

		s = math.random() * 0.9 + 0.1 -- i / numSamples 
		x = x * s / l
		y = y * s / l

		io.write("\tfloat3(" .. x .. "," .. y .. "," .. (1.0-s*s) .. "),\n")

	end

	io.write("};")

#endif


#define COC_SAMPLE_BLUR_COUNT 16

static const float3 samplesCoCBlur[COC_SAMPLE_BLUR_COUNT] = {
	float3(0.30842767230369,0.28629584692157,0.82290705899279),
	float3(-0.31332065171261,-0.58817662246439,0.55587842999677),
	float3(-0.68659247545372,0.15604011548928,0.50424225500843),
	float3(-0.79972043679232,-0.47093427233436,0.13866813411761),
	float3(-0.039029088550756,-0.20829193093826,0.95509120175291),
	float3(-0.19022191620985,-0.12568085366024,0.94801994561669),
	float3(0.75409773889214,0.5866511456722,0.087177033479257),
	float3(-0.651887665864,-0.62205674826198,0.18808787303612),
	float3(0.61570268489824,0.41420203515037,0.44934687788639),
	float3(0.16397233007906,-0.084953330927861,0.9658960065327),
	float3(-0.2177041664839,-0.89234210776142,0.15633045861146),
	float3(0.37211588471587,0.17869037865979,0.82959951691655),
	float3(-0.28904365940834,0.88595274547084,0.13154149574853),
	float3(-0.18355390451874,0.13692278289866,0.94756011565921),
	float3(0.40078239351928,-0.18801103879305,0.80402532233691),
	float3(-0.58150067278742,-0.25430295995188,0.59718697210749),
};
*/