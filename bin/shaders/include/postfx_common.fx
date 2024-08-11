#include "common.fx"
#include "globalConstantsVS.fx"
#include "commonPS.fx"

float2 ApplyTexCoordTransform( float2 pos_h, float4 transform )
{
    return pos_h.xy * transform.xy + transform.zw;
}

float3 ApplyViewSpaceVecTransform( float2 pos_h, float4 transform )
{
    return float3 ( pos_h.xy * transform.xy + transform.zw, 1.0 );
}

float2 ApplyTexCoordClamp( float2 coord, float4 _clamp )
{
    return clamp( coord, _clamp.xy, _clamp.zw );
}
