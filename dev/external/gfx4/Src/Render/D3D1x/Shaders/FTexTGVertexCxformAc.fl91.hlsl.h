#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 9.30.9200.20499
//
//
///
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// tex                               sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// COLOR                    1   xyzw        1     NONE   float   x   
// TEXCOORD                 0   xyzw        2     NONE   float   xyzw
// TEXCOORD                 1   xyzw        3     NONE   float   xyzw
// TEXCOORD                 2   xy          4     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
//
// Sampler/Resource to DX9 shader sampler mappings:
//
// Target Sampler Source Sampler  Source Resource
// -------------- --------------- ----------------
// s0             s0              t0               
//
//
// Level9 shader bytecode:
//
    ps_2_0
    def c0, 1, 0, 0, 0
    dcl t0
    dcl t1
    dcl t2
    dcl t3
    dcl_pp t4.xy
    dcl_2d s0
    texld r0, t4, s0
    add r0, r0, -t0
    mov r1, t0
    mad r0, t1.x, r0, r1
    mov r1.xyz, t3
    mov r1.w, c0.x
    mul r0, r0, r1
    mul r0, r0, t3.w
    mad r0, t2, r0.w, r0
    mov oC0, r0

// approximately 10 instruction slots used (1 texture, 9 arithmetic)
ps_4_0
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.x
dcl_input_ps linear v2.xyzw
dcl_input_ps linear v3.xyzw
dcl_input_ps linear v4.xy
dcl_output o0.xyzw
dcl_temps 2
sample r0.xyzw, v4.xyxx, t0.xyzw, s0
add r0.xyzw, r0.xyzw, -v0.xyzw
mad r0.xyzw, v1.xxxx, r0.xyzw, v0.xyzw
mov r1.xyz, v3.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v3.wwww
mad o0.xyzw, v2.xyzw, r0.wwww, r0.xyzw
ret 
// Approximately 9 instruction slots used
#endif

const BYTE pBinary_D3D1xFL91_FTexTGVertexCxformAc[] =
{
     68,  88,  66,  67, 233, 230, 
    252, 192,  23, 183, 247, 113, 
     64, 238, 186, 234,  76, 219, 
    158, 106,   1,   0,   0,   0, 
    184,   4,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
    104,   1,   0,   0, 212,   2, 
      0,   0,  80,   3,   0,   0, 
    236,   3,   0,   0, 132,   4, 
      0,   0,  65, 111, 110,  57, 
     40,   1,   0,   0,  40,   1, 
      0,   0,   0,   2, 255, 255, 
      0,   1,   0,   0,  40,   0, 
      0,   0,   0,   0,  40,   0, 
      0,   0,  40,   0,   0,   0, 
     40,   0,   1,   0,  36,   0, 
      0,   0,  40,   0,   0,   0, 
      0,   0,   0,   2, 255, 255, 
     81,   0,   0,   5,   0,   0, 
     15, 160,   0,   0, 128,  63, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   0,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   1,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   2,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   3,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   4,   0,  35, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   0,   8,  15, 160, 
     66,   0,   0,   3,   0,   0, 
     15, 128,   4,   0, 228, 176, 
      0,   8, 228, 160,   2,   0, 
      0,   3,   0,   0,  15, 128, 
      0,   0, 228, 128,   0,   0, 
    228, 177,   1,   0,   0,   2, 
      1,   0,  15, 128,   0,   0, 
    228, 176,   4,   0,   0,   4, 
      0,   0,  15, 128,   1,   0, 
      0, 176,   0,   0, 228, 128, 
      1,   0, 228, 128,   1,   0, 
      0,   2,   1,   0,   7, 128, 
      3,   0, 228, 176,   1,   0, 
      0,   2,   1,   0,   8, 128, 
      0,   0,   0, 160,   5,   0, 
      0,   3,   0,   0,  15, 128, 
      0,   0, 228, 128,   1,   0, 
    228, 128,   5,   0,   0,   3, 
      0,   0,  15, 128,   0,   0, 
    228, 128,   3,   0, 255, 176, 
      4,   0,   0,   4,   0,   0, 
     15, 128,   2,   0, 228, 176, 
      0,   0, 255, 128,   0,   0, 
    228, 128,   1,   0,   0,   2, 
      0,   8,  15, 128,   0,   0, 
    228, 128, 255, 255,   0,   0, 
     83,  72,  68,  82, 100,   1, 
      0,   0,  64,   0,   0,   0, 
     89,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      0,   0,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      0,   0,   0,   0,  85,  85, 
      0,   0,  98,  16,   0,   3, 
    242,  16,  16,   0,   0,   0, 
      0,   0,  98,  16,   0,   3, 
     18,  16,  16,   0,   1,   0, 
      0,   0,  98,  16,   0,   3, 
    242,  16,  16,   0,   2,   0, 
      0,   0,  98,  16,   0,   3, 
    242,  16,  16,   0,   3,   0, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   4,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 104,   0,   0,   2, 
      2,   0,   0,   0,  69,   0, 
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   4,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   8, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
      6,  16,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  18,  16,   0,   3,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0, 246,  31, 
     16,   0,   3,   0,   0,   0, 
     50,   0,   0,   9, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   2,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 116,   0,   0,   0, 
      9,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  82,  68,  69,  70, 
    148,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  28,   0, 
      0,   0,   0,   4, 255, 255, 
      0, 145,   0,   0,  96,   0, 
      0,   0,  92,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  92,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  12,   0,   0,   0, 
    116, 101, 120,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  57,  46,  51, 
     48,  46,  57,  50,  48,  48, 
     46,  50,  48,  52,  57,  57, 
      0, 171,  73,  83,  71,  78, 
    144,   0,   0,   0,   5,   0, 
      0,   0,   8,   0,   0,   0, 
    128,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
    128,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   1,   0,   0, 
    134,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,  15,   0,   0, 
    134,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,  15,  15,   0,   0, 
    134,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   4,   0, 
      0,   0,   3,   3,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171,  79,  83, 
     71,  78,  44,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  83,  86,  95,  84, 
     97, 114, 103, 101, 116,   0, 
    171, 171
};
