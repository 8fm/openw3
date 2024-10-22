#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex[0]                    sampler      NA          NA    0        1
// sampler_tex[1]                    sampler      NA          NA    1        1
// sampler_tex[2]                    sampler      NA          NA    2        1
// tex[0]                            texture  float4          2d    0        1
// tex[1]                            texture  float4          2d    1        1
// tex[2]                            texture  float4          2d    2        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float      w
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[134]
// Embedded Data:
//  0x00000086 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000081 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_input_ps linear v0.w
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v1.xyxx, t2.xyzw, s2
add r0.x, r0.x, l(-0.501961)
mul r0.xyz, r0.xxxx, l(1.596000, -0.813000, 0.000000, 0.000000)
sample_indexable(texture2d)(float,float,float,float) r0.w, v1.xyxx, t0.yzwx, s0
add r0.w, r0.w, l(-0.062745)
mad r0.xyz, r0.wwww, l(1.164000, 1.164000, 1.164000, 0.000000), r0.xyzx
sample_indexable(texture2d)(float,float,float,float) r0.w, v1.xyxx, t1.yzwx, s1
add r0.w, r0.w, l(-0.501961)
mad o0.xyz, r0.wwww, l(0.000000, -0.392000, 2.017000, 0.000000), r0.xyzx
mov o0.w, v0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[134], bundle is:[173] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FYUVEAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask  3, param1, paramSlot 1, DefaultVal={0,0,0,0}

codeLenInByte        = 212;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 6;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_RESOURCE, 2, offset 16:23 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 0, offset 24:27 dwords
;  extUserElements 1[4] = IMM_SAMPLER, 1, offset 28:31 dwords
;  extUserElements 1[5] = IMM_SAMPLER, 2, offset 32:35 dwords
NumVgprs             = 7;
NumSgprs             = 42;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000007
; constBufUsage           = 0x00000000

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000002
SPIC:NUM_INTERP             = 2
SPIC:PARAM_GEN              = 0
SPIC:FOG_ADDR               = 0
SPIC:BC_OPTIMIZE_DISABLE    = 0
SPIC:PASS_FOG_THROUGH_PS    = 0
; SPI_PS_INPUT_ADDR       = 0x00000002
SPIA:PERSP_CENTER_ENA       = 1
; DB_SHADER_CONTROL       = 0x00000010
DB:Z_ORDER                  = 1
DB:CONSERVATIVE_Z_EXPORT    = 0; EXPORT_ANY_Z
; CB_SHADER_MASK          = 0x0000000F
CB:OUTPUT0_ENABLE           = 15

// Shader Instructions:
shader main
  asic(CI)
  type(PS)
                                                            // s_ps_state in s0
  s_mov_b64     s[40:41], exec                          // 000000000000: BEA8047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 00000000000C: C0C80100
  s_load_dwordx8  s[4:11], s[0:1], 0x10                 // 000000000010: C0C20110
  s_load_dwordx8  s[24:31], s[0:1], 0x18                // 000000000014: C0CC0118
  s_load_dwordx4  s[12:15], s[0:1], 0x20                // 000000000018: C0860120
  v_interp_p1_f32  v2, v0, attr1.x                      // 00000000001C: C8080400
  v_interp_p1_f32  v3, v0, attr1.y                      // 000000000020: C80C0500
  v_interp_p2_f32  v2, v1, attr1.x                      // 000000000024: C8090401
  v_interp_p2_f32  v3, v1, attr1.y                      // 000000000028: C80D0501
  s_waitcnt     lgkmcnt(0)                              // 00000000002C: BF8C007F
  image_sample  v[4:7], v[2:3], s[4:11], s[12:15]       // 000000000030: F0800100 00610402
  image_sample  v[5:8], v[2:3], s[16:23], s[24:27]      // 000000000038: F0800100 00C40502
  s_load_dwordx8  s[32:39], s[0:1], 0x08                // 000000000040: C0D00108
  s_waitcnt     lgkmcnt(0)                              // 000000000044: BF8C007F
  image_sample  v[2:5], v[2:3], s[32:39], s[28:31]      // 000000000048: F0800100 00E80202
  s_waitcnt     vmcnt(2) & lgkmcnt(15)                  // 000000000050: BF8C0F72
  v_add_f32     v3, 0xbf008081, v4                      // 000000000054: 060608FF BF008081
  v_mul_legacy_f32  v4, 0xbf5020c5, v3                  // 00000000005C: 0E0806FF BF5020C5
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 000000000064: BF8C0F71
  v_add_f32     v5, 0xbd808081, v5                      // 000000000068: 060A0AFF BD808081
  s_mov_b32     s0, 0x3f94fdf4                          // 000000000070: BE8003FF 3F94FDF4
  v_mul_legacy_f32  v3, 0x3fcc49ba, v3                  // 000000000078: 0E0606FF 3FCC49BA
  v_mul_legacy_f32  v6, 0x3f94fdf4, v5                  // 000000000080: 0E0C0AFF 3F94FDF4
  v_mac_legacy_f32  v4, s0, v5                          // 000000000088: 0C080A00
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000008C: BF8C0F70
  v_add_f32     v2, 0xbf008081, v2                      // 000000000090: 060404FF BF008081
  s_mov_b32     s1, 0xbec8b439                          // 000000000098: BE8103FF BEC8B439
  s_mov_b32     s2, 0x40011687                          // 0000000000A0: BE8203FF 40011687
  v_mac_legacy_f32  v3, s0, v5                          // 0000000000A8: 0C060A00
  v_mac_legacy_f32  v4, s1, v2                          // 0000000000AC: 0C080401
  v_mac_legacy_f32  v6, s2, v2                          // 0000000000B0: 0C0C0402
  v_interp_p1_f32  v0, v0, attr0.w                      // 0000000000B4: C8000300
  v_interp_p2_f32  v0, v1, attr0.w                      // 0000000000B8: C8010301
  s_mov_b64     exec, s[40:41]                          // 0000000000BC: BEFE0428
  v_cvt_pkrtz_f16_f32  v1, v3, v4                       // 0000000000C0: 5E020903
  v_cvt_pkrtz_f16_f32  v0, v6, v0                       // 0000000000C4: 5E000106
  exp           mrt0, v1, v1, v0, v0 compr vm           // 0000000000C8: F800140F 00000001
  s_endpgm                                              // 0000000000D0: BF810000
end


// Approximately 11 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FYUVEAlpha[] =
{
     68,  88,  66,  67, 249, 250, 
     67,   0, 189, 107, 227,  72, 
     60,   0, 113, 144, 182,  44, 
    165, 173,   1,   0,   0,   0, 
    180,   7,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    184,   1,   0,   0,   8,   2, 
      0,   0,  60,   2,   0,   0, 
     24,   7,   0,   0,  82,  68, 
     69,  70, 124,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
     62,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    252,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  11,   1,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  26,   1, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     41,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,  48,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0,  55,   1, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      2,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 116, 101, 120,  91, 
     48,  93,   0, 115,  97, 109, 
    112, 108, 101, 114,  95, 116, 
    101, 120,  91,  49,  93,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 116, 101, 120,  91, 
     50,  93,   0, 116, 101, 120, 
     91,  48,  93,   0, 116, 101, 
    120,  91,  49,  93,   0, 116, 
    101, 120,  91,  50,  93,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  70, 
    111, 114,  32,  68, 117, 114, 
     97, 110, 103, 111,  32,  57, 
     46,  51,  48,  46,  49,  50, 
     48,  57,  56,  46,  48,   0, 
    171, 171,  73,  83,  71,  78, 
     72,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   8,   0,   0, 
     62,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
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
    171, 171,  83,  72,  69,  88, 
    212,   4,   0,   0,  80,   0, 
      0,   0,  53,   1,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0, 134,   0,   0,   0, 
     80,   0,   0,   0, 129,   0, 
      0,   0, 106,   8,   0,   1, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   2,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   2,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  18,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   2,   0, 
      0,   0,   0,  96,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0, 129, 128, 
      0, 191,  56,   0,   0,  10, 
    114,   0,  16,   0,   0,   0, 
      0,   0,   6,   0,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0, 186,  73, 204,  63, 
    197,  32,  80, 191,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      1,   0,   0,   0, 150, 115, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 129, 128, 128, 189, 
     50,   0,   0,  12, 114,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    244, 253, 148,  63, 244, 253, 
    148,  63, 244, 253, 148,  63, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      1,   0,   0,   0, 150, 115, 
     16,   0,   1,   0,   0,   0, 
      0,  96,  16,   0,   1,   0, 
      0,   0,   0,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 129, 128,   0, 191, 
     50,   0,   0,  12, 114,  32, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,  57, 180, 
    200, 190, 135,  22,   1,  64, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     58,  16,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 175,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 213,  10,   0, 120,   1, 
    237,  86,  77, 107,  19,  81, 
     20, 189,  47,  51,  25,  39, 
    227, 152,  20,  21, 169, 208, 
     69,  22,  10, 197,  69, 176, 
    234,  66, 221, 180, 133,  90, 
     20,  93, 136, 197, 162, 101, 
    168, 157, 182, 169,  45,  77, 
    211, 154,  20,   9,  34,  49, 
     17, 212,  77, 193,  46, 252, 
      5, 174, 250,   3, 116,  35, 
     58,  25, 252, 128,  89, 116, 
    225, 162,  46, 220, 187, 112, 
    169, 160, 208,  69, 237, 120, 
    207, 204, 123, 118, 144, 186, 
     11,  40, 152,   3, 231, 190, 
     57, 239, 227, 190,  59, 239, 
     38, 115, 223,   7, 139,  34, 
    140, 154,  55, 166, 208, 126, 
    239,  31, 126, 160, 167, 136, 
    210, 252,  44, 152, 104, 129, 
    154,   9,  75, 244,  50,   3, 
     75, 212,  11, 131,   9,  60, 
    119,  15,  55, 199, 152,  27, 
    204,   1,  38, 198,  20,  99, 
     12, 248, 117, 182, 121,  38, 
    230,  22, 120, 205, 121, 110, 
    143,  48, 117, 230, 126,  73, 
    184,   3,  52,  24,   9,   3, 
    134, 129, 117, 191, 195, 134, 
    145, 192,  60,  21,  43, 124, 
    242,  22,  59,  56,  10, 179, 
     59,  16, 227, 110, 190,  21, 
     16, 151, 138,   7, 241,  65, 
      3, 120, 223, 191, 141, 248, 
    188, 226, 232, 212,  59, 168, 
     60,   1, 200,  71,  18,  89, 
    102, 188, 134, 232, 146, 204, 
    163,  58,  95,  71, 106, 248, 
    193, 187, 249, 204,  63,  33, 
    100,  32, 119,  73,  40, 141, 
    115,  55,  40,  14, 194,  16, 
      7, 162, 156, 255, 111, 192, 
    239,  15, 192,  89, 128,  93, 
    204,  28,  83,   1, 227, 199, 
    153,  68, 141,  22, 184, 218, 
     77, 155,  80,  74,  11, 214, 
    113,  86,  99, 173,  65,  71, 
     78,  19, 154, 149, 150,  73, 
    141,  27, 130, 198, 115,  61, 
    180, 137, 188, 178, 118,  88, 
     59,   9,  61, 198, 122,  44, 
    161,  39,  88,  79,  36, 244, 
     36, 235,  73, 165, 149, 255, 
     92, 180, 159,  17, 229, 153, 
     59, 241, 215, 253, 103, 217, 
    137, 177,  61, 236, 196, 216, 
     30, 118,  98, 108,  27,  35, 
    212, 245,  53, 175, 110, 109, 
    123,  41, 109, 203,  35,  17, 
    248,  93, 226, 181, 223,  45, 
    214, 253, 188, 120, 232, 147, 
    110,   6, 148, 182,   3, 161, 
    103,   2, 145, 222,  23, 220, 
    163, 149,  22, 137, 198, 151, 
    148, 238, 114, 161, 227,  54, 
    253, 150,  76, 241, 222, 255, 
    213, 159, 250,  76, 149, 220, 
     74,  43,  52,  13, 163, 217, 
    160,  86, 104, 152, 217, 119, 
    249, 203, 173,  91, 232, 179, 
     44, 238, 107, 188,  10, 181, 
    134, 247, 237, 199, 147, 254, 
    208,  48, 178,  47,  46, 172, 
    247, 135, 150, 157, 133,  38, 
    203, 180, 151,  48,  79, 215, 
    227, 181,  90, 211,  59, 243, 
     44, 240,  66, 237, 190, 247, 
    232, 144,  24,  32, 203, 176, 
    133, 110, 218,  41, 221, 182, 
    185,  36,   7,  66,  19,  65, 
    175, 190, 237, 241,  55,  54, 
    254,  70,  31,  84, 223, 216, 
    102, 235,  48, 219, 248,  57, 
    174,  15,  73, 112, 229, 140, 
    134,  96,  80,  69, 149,  86, 
    117,   4,  58, 234,  96, 116, 
     51, 185,  44,  68,  26, 236, 
     97,  42, 141, 249, 168, 189, 
    208, 159, 100, 109, 223, 144, 
     45, 252, 112,  63, 223,   7, 
     48, 115,   7, 186, 188,  11, 
    110, 201, 121, 188,  23, 237, 
    101,  94, 156,  95, 124, 254, 
    245, 233, 224, 199, 188,  28, 
    239, 145,  45, 124, 224, 241, 
    238, 227, 185,  55, 171, 178, 
    239, 180, 108, 175,  49, 177, 
    126, 230, 172,  51, 233, 150, 
    220,  59, 213, 194, 242,  98, 
    101, 113, 190,  80,  93, 174, 
     20, 221, 133,  19, 206, 116, 
    241, 182,  83, 172,  45,  23, 
     43, 101, 183, 228, 220, 156, 
    169, 157, 114,  70,  42,  83, 
    206, 149,  98, 121, 186,  88, 
    113, 134,  78,  14, 245, 213, 
    156, 145,  89, 151,  69, 213, 
     25, 190, 126, 117, 244, 220, 
     96, 105, 105, 214,  45, 204, 
    148, 250, 250,  10, 179, 165, 
    106, 137,  61, 199,  88, 147, 
    251, 225, 238, 135, 187, 203, 
    130,  59,  87, 166, 159, 197, 
    173,  74, 194,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     11,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   6,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0
};
