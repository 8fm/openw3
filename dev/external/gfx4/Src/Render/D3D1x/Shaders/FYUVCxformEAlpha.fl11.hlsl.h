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
// TEXCOORD                 0   xyzw        1     NONE   float   xyzw
// TEXCOORD                 1   xyzw        2     NONE   float   xyzw
// TEXCOORD                 2   xy          3     NONE   float   xy  
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
// First Precompiled Shader at offset:[161]
// Embedded Data:
//  0x000000a1 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000009c - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_input_ps linear v0.w
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xyzw
dcl_input_ps linear v3.xy
dcl_output o0.xyzw
dcl_temps 1
sample_indexable(texture2d)(float,float,float,float) r0.x, v3.xyxx, t2.xyzw, s2
add r0.x, r0.x, l(-0.501961)
mul r0.xyz, r0.xxxx, l(1.596000, -0.813000, 0.000000, 0.000000)
sample_indexable(texture2d)(float,float,float,float) r0.w, v3.xyxx, t0.yzwx, s0
add r0.w, r0.w, l(-0.062745)
mad r0.xyz, r0.wwww, l(1.164000, 1.164000, 1.164000, 0.000000), r0.xyzx
sample_indexable(texture2d)(float,float,float,float) r0.w, v3.xyxx, t1.yzwx, s1
add r0.w, r0.w, l(-0.501961)
mad r0.xyz, r0.wwww, l(0.000000, -0.392000, 2.017000, 0.000000), r0.xyzx
mov r0.w, l(1.000000)
mad r0.xyzw, r0.xyzw, v2.xyzw, v1.xyzw
mul o0.w, r0.w, v0.w
mov o0.xyz, r0.xyzx
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[161], bundle is:[195] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FYUVCxformEAlpha.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  8, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask 15, param2, paramSlot 2, DefaultVal={0,0,0,0};   [3] generic, usageIdx  3, channelMask  3, param3, paramSlot 3, DefaultVal={0,0,0,0}

codeLenInByte        = 296;Bytes

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
NumVgprs             = 15;
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
; SPI_PS_IN_CONTROL       = 0x00000004
SPIC:NUM_INTERP             = 4
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
  v_interp_p1_f32  v2, v0, attr3.x                      // 00000000001C: C8080C00
  v_interp_p1_f32  v3, v0, attr3.y                      // 000000000020: C80C0D00
  v_interp_p2_f32  v2, v1, attr3.x                      // 000000000024: C8090C01
  v_interp_p2_f32  v3, v1, attr3.y                      // 000000000028: C80D0D01
  s_waitcnt     lgkmcnt(0)                              // 00000000002C: BF8C007F
  image_sample  v[4:7], v[2:3], s[4:11], s[12:15]       // 000000000030: F0800100 00610402
  image_sample  v[5:8], v[2:3], s[16:23], s[24:27]      // 000000000038: F0800100 00C40502
  s_load_dwordx8  s[32:39], s[0:1], 0x08                // 000000000040: C0D00108
  s_waitcnt     lgkmcnt(0)                              // 000000000044: BF8C007F
  image_sample  v[2:5], v[2:3], s[32:39], s[28:31]      // 000000000048: F0800100 00E80202
  s_waitcnt     vmcnt(2) & lgkmcnt(15)                  // 000000000050: BF8C0F72
  v_add_f32     v3, 0xbf008081, v4                      // 000000000054: 060608FF BF008081
  v_interp_p1_f32  v6, v0, attr1.w                      // 00000000005C: C8180700
  v_interp_p1_f32  v7, v0, attr2.w                      // 000000000060: C81C0B00
  v_interp_p1_f32  v9, v0, attr1.x                      // 000000000064: C8240400
  v_interp_p1_f32  v10, v0, attr1.y                     // 000000000068: C8280500
  v_interp_p1_f32  v11, v0, attr1.z                     // 00000000006C: C82C0600
  v_interp_p1_f32  v12, v0, attr2.x                     // 000000000070: C8300800
  v_interp_p1_f32  v13, v0, attr2.y                     // 000000000074: C8340900
  v_interp_p1_f32  v14, v0, attr2.z                     // 000000000078: C8380A00
  v_mul_legacy_f32  v4, 0xbf5020c5, v3                  // 00000000007C: 0E0806FF BF5020C5
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 000000000084: BF8C0F71
  v_add_f32     v5, 0xbd808081, v5                      // 000000000088: 060A0AFF BD808081
  s_mov_b32     s0, 0x3f94fdf4                          // 000000000090: BE8003FF 3F94FDF4
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000098: C8000300
  v_mul_legacy_f32  v8, 0x3f94fdf4, v5                  // 00000000009C: 0E100AFF 3F94FDF4
  v_mac_legacy_f32  v4, s0, v5                          // 0000000000A4: 0C080A00
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000A8: BF8C0F70
  v_add_f32     v2, 0xbf008081, v2                      // 0000000000AC: 060404FF BF008081
  s_mov_b32     s1, 0xbec8b439                          // 0000000000B4: BE8103FF BEC8B439
  s_mov_b32     s2, 0x40011687                          // 0000000000BC: BE8203FF 40011687
  v_interp_p2_f32  v6, v1, attr1.w                      // 0000000000C4: C8190701
  v_interp_p2_f32  v7, v1, attr2.w                      // 0000000000C8: C81D0B01
  v_mul_legacy_f32  v3, 0x3fcc49ba, v3                  // 0000000000CC: 0E0606FF 3FCC49BA
  v_mac_legacy_f32  v4, s1, v2                          // 0000000000D4: 0C080401
  v_mac_legacy_f32  v8, s2, v2                          // 0000000000D8: 0C100402
  v_interp_p2_f32  v9, v1, attr1.x                      // 0000000000DC: C8250401
  v_interp_p2_f32  v10, v1, attr1.y                     // 0000000000E0: C8290501
  v_interp_p2_f32  v11, v1, attr1.z                     // 0000000000E4: C82D0601
  v_interp_p2_f32  v12, v1, attr2.x                     // 0000000000E8: C8310801
  v_interp_p2_f32  v13, v1, attr2.y                     // 0000000000EC: C8350901
  v_interp_p2_f32  v14, v1, attr2.z                     // 0000000000F0: C8390A01
  v_interp_p2_f32  v0, v1, attr0.w                      // 0000000000F4: C8010301
  v_mac_legacy_f32  v3, s0, v5                          // 0000000000F8: 0C060A00
  v_add_f32     v2, v6, v7                              // 0000000000FC: 06040F06
  v_mac_legacy_f32  v9, v3, v12                         // 000000000100: 0C121903
  v_mac_legacy_f32  v10, v4, v13                        // 000000000104: 0C141B04
  v_mac_legacy_f32  v11, v8, v14                        // 000000000108: 0C161D08
  v_mul_legacy_f32  v0, v2, v0                          // 00000000010C: 0E000102
  s_mov_b64     exec, s[40:41]                          // 000000000110: BEFE0428
  v_cvt_pkrtz_f16_f32  v1, v9, v10                      // 000000000114: 5E021509
  v_cvt_pkrtz_f16_f32  v0, v11, v0                      // 000000000118: 5E00010B
  exp           mrt0, v1, v1, v0, v0 compr vm           // 00000000011C: F800140F 00000001
  s_endpgm                                              // 000000000124: BF810000
end


// Approximately 14 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FYUVCxformEAlpha[] =
{
     68,  88,  66,  67,  56, 103, 
     84, 131,  93,  54, 152, 154, 
     40,  26, 126, 208,  84,  56, 
     99, 205,   1,   0,   0,   0, 
    168,   8,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    184,   1,   0,   0,  56,   2, 
      0,   0, 108,   2,   0,   0, 
     12,   8,   0,   0,  82,  68, 
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
    120,   0,   0,   0,   4,   0, 
      0,   0,   8,   0,   0,   0, 
    104,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   8,   0,   0, 
    110,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
    110,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,  15,   0,   0, 
    110,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
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
    152,   5,   0,   0,  80,   0, 
      0,   0, 102,   1,   0,   0, 
     53,  16,   0,   0,   5,   0, 
      0,   0, 161,   0,   0,   0, 
     80,   0,   0,   0, 156,   0, 
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
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   2,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   3,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0,  18,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   3,   0,   0,   0, 
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
      3,   0,   0,   0, 150, 115, 
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
      3,   0,   0,   0, 150, 115, 
     16,   0,   1,   0,   0,   0, 
      0,  96,  16,   0,   1,   0, 
      0,   0,   0,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 129, 128,   0, 191, 
     50,   0,   0,  12, 114,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,  57, 180, 
    200, 190, 135,  22,   1,  64, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  58,  16, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 114,  32, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 197,   0, 
      0,   0,  21,   0,   1,   0, 
     41,  47,  11,   0, 120,   1, 
    237,  86, 207,  79,  19,  65, 
     20, 126, 179, 187,  93, 182, 
    219, 237,  15,   9, 144,  18, 
     32, 114, 128,  88, 141,  22, 
     43, 160,  96,  66, 192,   8, 
     38,  30, 140,  68, 130,  81, 
    178, 105,  88, 164,  21,  99, 
    129, 218,  54, 166, 241, 128, 
    197,  68, 189, 112, 240, 224, 
    217, 120,  52, 241,  98, 140, 
    241, 160, 113, 219,  24,  77, 
    230, 192, 129, 147, 127, 131, 
    241, 228, 193,   3,   7,  97, 
    125, 111, 119,  71,  54,  68, 
    110,  36,  26, 195, 151, 124, 
    239, 205, 247, 118, 230, 237, 
    235, 204, 116, 103, 250,  34, 
    224, 226, 253, 150,  51,  72, 
     62, 218,  49, 189,  81, 148, 
      0,  66, 216, 102,  72, 242, 
    132, 170,  70,  22,  32, 161, 
    147,   5,  72, 145, 161,  14, 
    216,  55, 142, 238,  24,  50, 
    133, 122, 140, 124, 128,  30, 
    198,  26,  47, 209, 118,  35, 
     19, 200, 103,  56, 230,  18, 
    250,  97, 164, 130,  60, 130, 
    108,  70,  82,  58, 130,  76, 
    198, 135,  74,   6, 209,  68, 
    102,  23,  14, 145, 241, 209, 
    134,  20, 181,  82,  78, 124, 
    197,  14, 122, 201, 252,  25, 
     35,  72, 170, 127,  47,  80, 
    109, 162,  30, 170, 143, 234, 
     36, 208, 239, 253, 219, 240, 
    230, 203, 171,  78, 204, 143, 
     88,  39,   2, 173,  71,  16, 
     49, 164,   8,  61,  15, 147, 
    221, 153, 223,  87, 190,  22, 
    107, 217,  64, 238,   5,   7, 
    209,  67, 141,   0, 132, 166, 
    185,  87, 193,  43,  66, 101, 
    135, 221,  53,  87, 165,  62, 
    119,  95, 168, 114,  63, 204, 
    162, 255, 223,  65, 115,  64, 
     32,  79, 251, 144, 246, 124, 
    112, 143,  81, 124,   2,   9, 
     80, 171,  19, 159,  36,  97, 
    147, 148, 208,  12, 181, 183, 
     28, 158, 150,  73, 187, 255, 
    187, 128,  70,  21, 110, 149, 
    178,  17,   6, 217, 120,  39, 
    108, 210, 186, 162,  54,  81, 
    155,   1,  61, 131, 122,  38, 
    160, 103,  81, 207,   6, 244, 
     28, 234,  57, 161,  69, 254, 
    184, 251, 190, 136, 187, 206, 
     24,  92, 249, 151, 121,  80, 
    227, 254, 240, 160, 198, 253, 
    225,  65, 141, 251,  70,  23, 
     43, 202,  11, 123,  69, 223, 
    182,  37, 249, 167,  13, 140, 
     55,  18, 236,  99,  35, 201, 
    214,  27, 221, 236,  81,   3, 
     12, 141,  67, 212, 224, 204, 
      8, 115,  22, 141, 242, 251, 
    176,  86,   7,  86, 251,  46, 
     41,  22,  30, 116, 232,  67, 
    159,  64,  99,  27, 141, 223, 
    113, 233,  43, 148, 226, 107, 
    117,  71,  83, 213, 213,  26, 
    212, 161,  41, 201,  33, 210, 
    201,  65, 233, 225,  16,  74, 
    113,  80, 143, 115, 208,  78, 
    114,   8,  15, 112, 208, 135, 
    184, 163, 106, 177, 207, 221, 
    147, 245,  59,  52,  70, 215, 
    113,  76, 237, 131,  35, 215, 
    236,  31,  91,  79,  71, 241, 
    216, 229, 142, 158, 136, 185, 
    109,  93,  51, 138, 212,  71, 
     81, 220, 188, 142, 188, 106, 
     15, 191, 225, 182,  35,  63, 
    176,  31, 183, 177,  49, 214, 
    212, 206,  89, 164,  11, 243, 
    169, 177, 119,  23, 215,  71, 
    153, 162,  25, 146, 146,  48, 
    152, 210, 203,  89, 232,  40, 
    103, 234,   9, 206, 180,  12, 
    103, 225,  65, 206, 244,  97, 
    206, 100, 134, 239,  87,  13, 
     53, 174, 168, 114, 123, 179, 
    161, 116, 180,  24,  90,  87, 
    155,  33,  49, 136, 165, 148, 
    109,  27, 191, 219, 222, 119, 
    191,  69, 124, 183,  87, 235, 
    237, 104, 189, 246, 174, 187, 
     15,   2,  79,  99, 247,  17, 
     25,  58, 153, 133,  22, 103, 
     19, 105,  55, 128,  72,  34, 
    241, 124, 114,  53, 177,  19, 
     41,  52, 245, 167, 243, 156, 
    244, 144, 184,   7, 250, 158, 
    242,  96,  28, 239,  24, 212, 
    115,   7, 215, 252, 231, 147, 
    190, 199, 119,   1,  93,  57, 
     31,  94,  30, 129, 183, 173, 
    167, 205, 138,  31,  47, 250, 
    158, 114,  80, 243, 204, 235, 
    234, 151, 111, 126, 108, 205, 
    247,  89,  36, 141, 207, 159, 
     53, 231, 172, 130, 117, 175, 
    156, 174,  44, 151, 150, 111, 
    167, 203, 149,  82, 206,  90, 
     60, 101, 206, 231, 238, 154, 
    185, 106,  37,  87,  90, 178, 
     10, 230, 205, 124, 117, 192, 
    156,  42, 221,  48, 175, 228, 
    150, 230, 115,  37, 115, 188, 
    127,  60,  83,  53, 167,  22, 
     44,  20, 101, 243, 194, 245, 
    233, 171, 231, 171, 249, 229, 
    210, 226, 196, 185,  66, 113, 
    193,  74, 231,  11, 153,  76, 
    122, 161,  80,  46,  96, 126, 
     15, 146, 127,  51, 166,  91, 
     37, 221, 174,  22, 173,  91, 
     75, 240,  11, 134, 235,  93, 
    113,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,  14,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,   8,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
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
      0,   0
};
