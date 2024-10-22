#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Buffer Definitions: 
//
// cbuffer Constants
// {
//
//   float4 fsize;                      // Offset:    0 Size:    16
//   float4 texscale;                   // Offset:   16 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex                       sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
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
// First Precompiled Shader at offset:[189]
// Embedded Data:
//  0x000000bd - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x000000b8 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[2], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 4
mov r0.xyzw, l(0,0,0,0)
mov r1.x, -cb0[0].x
loop 
  lt r1.z, cb0[0].x, r1.x
  breakc_nz r1.z
  mov r2.xyzw, r0.xyzw
  mov r1.y, -cb0[0].y
  loop 
    lt r1.z, cb0[0].y, r1.y
    breakc_nz r1.z
    mad r1.zw, r1.xxxy, cb0[1].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r3.xyzw, r1.zwzz, t0.xyzw, s0, l(0.000000)
    add r2.xyzw, r2.xyzw, r3.xyzw
    add r1.y, r1.y, l(1.000000)
  endloop 
  mov r0.xyzw, r2.xyzw
  add r1.x, r1.x, l(1.000000)
endloop 
mul r0.xyzw, r0.xyzw, cb0[0].wwww
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[189], bundle is:[187] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FBox2BlurMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 312;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 3;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
;  extUserElements 1[2] = IMM_CONST_BUFFER, 0, offset 12:15 dwords
NumVgprs             = 13;
NumSgprs             = 24;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000001
; constBufUsage           = 0x00000001

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000003
SPIC:NUM_INTERP             = 3
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
  s_mov_b32     m0, s2                                  // 000000000000: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x08                 // 000000000004: C0C20108
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx4  s[12:15], s[8:11], 0x00        // 00000000000C: C2860900
  v_mov_b32     v12, 0                                  // 000000000010: 7E180280
  v_mov_b32     v3, 0                                   // 000000000014: 7E060280
  v_mov_b32     v4, 0                                   // 000000000018: 7E080280
  v_mov_b32     v5, 0                                   // 00000000001C: 7E0A0280
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 000000000020: C0C80100
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  v_max_f32     v6, -s12, -s12                          // 000000000028: D2200006 6000180C
label_000C:
  v_mov_b32     v7, s12                                 // 000000000030: 7E0E020C
  v_cmp_gt_f32  vcc, v6, v7                             // 000000000034: 7C080F06
  s_cbranch_vccnz  label_0027                           // 000000000038: BF870018
  v_max_f32     v2, -s13, -s13                          // 00000000003C: D2200002 60001A0D
label_0011:
  v_mov_b32     v8, s13                                 // 000000000044: 7E10020D
  v_cmp_gt_f32  vcc, v2, v8                             // 000000000048: 7C081102
  s_cbranch_vccnz  label_0025                           // 00000000004C: BF870011
  s_buffer_load_dwordx2  s[0:1], s[8:11], 0x04          // 000000000050: C2400904
  v_interp_p1_f32  v8, v0, attr2.x                      // 000000000054: C8200800
  v_interp_p1_f32  v9, v0, attr2.y                      // 000000000058: C8240900
  v_interp_p2_f32  v8, v1, attr2.x                      // 00000000005C: C8210801
  v_interp_p2_f32  v9, v1, attr2.y                      // 000000000060: C8250901
  s_waitcnt     lgkmcnt(0)                              // 000000000064: BF8C007F
  v_mac_legacy_f32  v8, s0, v6                          // 000000000068: 0C100C00
  v_mac_legacy_f32  v9, s1, v2                          // 00000000006C: 0C120401
  image_sample_lz  v[8:11], v[8:9], s[16:23], s[4:7] dmask:0xf // 000000000070: F09C0F00 00240808
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000078: BF8C0F70
  v_add_f32     v12, v12, v8                            // 00000000007C: 0618110C
  v_add_f32     v3, v3, v9                              // 000000000080: 06061303
  v_add_f32     v4, v4, v10                             // 000000000084: 06081504
  v_add_f32     v5, v5, v11                             // 000000000088: 060A1705
  v_add_f32     v2, 1.0, v2                             // 00000000008C: 060404F2
  s_branch      label_0011                              // 000000000090: BF82FFEC
label_0025:
  v_add_f32     v6, 1.0, v6                             // 000000000094: 060C0CF2
  s_branch      label_000C                              // 000000000098: BF82FFE5
label_0027:
  v_interp_p1_f32  v6, v0, attr1.x                      // 00000000009C: C8180400
  v_interp_p2_f32  v6, v1, attr1.x                      // 0000000000A0: C8190401
  v_mul_legacy_f32  v2, s15, v12                        // 0000000000A4: 0E04180F
  v_interp_p1_f32  v7, v0, attr1.y                      // 0000000000A8: C81C0500
  v_interp_p1_f32  v8, v0, attr1.z                      // 0000000000AC: C8200600
  v_mul_legacy_f32  v2, v2, v6                          // 0000000000B0: 0E040D02
  v_interp_p1_f32  v6, v0, attr1.w                      // 0000000000B4: C8180700
  v_interp_p2_f32  v7, v1, attr1.y                      // 0000000000B8: C81D0501
  v_interp_p2_f32  v8, v1, attr1.z                      // 0000000000BC: C8210601
  v_mul_legacy_f32  v3, s15, v3                         // 0000000000C0: 0E06060F
  v_mul_legacy_f32  v4, s15, v4                         // 0000000000C4: 0E08080F
  v_interp_p2_f32  v6, v1, attr1.w                      // 0000000000C8: C8190701
  v_mul_legacy_f32  v5, s15, v5                         // 0000000000CC: 0E0A0A0F
  v_mul_legacy_f32  v3, v3, v7                          // 0000000000D0: 0E060F03
  v_mul_legacy_f32  v4, v4, v8                          // 0000000000D4: 0E081104
  v_mul_legacy_f32  v2, v2, v6                          // 0000000000D8: 0E040D02
  v_mul_legacy_f32  v3, v3, v6                          // 0000000000DC: 0E060D03
  v_mul_legacy_f32  v4, v4, v6                          // 0000000000E0: 0E080D04
  v_mul_legacy_f32  v5, v5, v6                          // 0000000000E4: 0E0A0D05
  v_interp_p1_f32  v6, v0, attr0.x                      // 0000000000E8: C8180000
  v_interp_p1_f32  v7, v0, attr0.y                      // 0000000000EC: C81C0100
  v_interp_p1_f32  v8, v0, attr0.z                      // 0000000000F0: C8200200
  v_interp_p1_f32  v0, v0, attr0.w                      // 0000000000F4: C8000300
  v_interp_p2_f32  v6, v1, attr0.x                      // 0000000000F8: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 0000000000FC: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 000000000100: C8210201
  v_interp_p2_f32  v0, v1, attr0.w                      // 000000000104: C8010301
  v_mac_legacy_f32  v2, v6, v5                          // 000000000108: 0C040B06
  v_mac_legacy_f32  v3, v7, v5                          // 00000000010C: 0C060B07
  v_mac_legacy_f32  v4, v8, v5                          // 000000000110: 0C080B08
  v_mac_legacy_f32  v5, v0, v5                          // 000000000114: 0C0A0B00
  v_mul_legacy_f32  v0, v2, v5                          // 000000000118: 0E000B02
  v_mul_legacy_f32  v1, v3, v5                          // 00000000011C: 0E020B03
  v_mul_legacy_f32  v2, v4, v5                          // 000000000120: 0E040B04
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 000000000124: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v5                       // 000000000128: 5E020B02
  exp           mrt0, v0, v0, v1, v1 compr vm           // 00000000012C: F800140F 00000100
  s_endpgm                                              // 000000000134: BF810000
end


// Approximately 27 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FBox2BlurMul[] =
{
     68,  88,  66,  67, 136, 249, 
     86, 200, 173,  21, 122, 124, 
    108,  96, 170, 118, 134,  41, 
      9, 183,   1,   0,   0,   0, 
    252,   8,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    216,   1,   0,   0,  60,   2, 
      0,   0, 112,   2,   0,   0, 
     96,   8,   0,   0,  82,  68, 
     69,  70, 156,   1,   0,   0, 
      1,   0,   0,   0, 184,   0, 
      0,   0,   3,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
     93,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    156,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 168,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 172,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 116, 101, 120,   0, 
    116, 101, 120,   0,  67, 111, 
    110, 115, 116,  97, 110, 116, 
    115,   0, 171, 171, 172,   0, 
      0,   0,   2,   0,   0,   0, 
    208,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  32,   1, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  48,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  84,   1,   0,   0, 
     16,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     48,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    102, 115, 105, 122, 101,   0, 
    102, 108, 111,  97, 116,  52, 
      0, 171, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     38,   1,   0,   0, 116, 101, 
    120, 115,  99,  97, 108, 101, 
      0,  77, 105,  99, 114, 111, 
    115, 111, 102, 116,  32,  40, 
     82,  41,  32,  72,  76,  83, 
     76,  32,  83, 104,  97, 100, 
    101, 114,  32,  67, 111, 109, 
    112, 105, 108, 101, 114,  32, 
     70, 111, 114,  32,  68, 117, 
    114,  97, 110, 103, 111,  32, 
     57,  46,  51,  48,  46,  49, 
     50,  48,  57,  56,  46,  48, 
      0, 171, 171, 171,  73,  83, 
     71,  78,  92,   0,   0,   0, 
      3,   0,   0,   0,   8,   0, 
      0,   0,  80,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   3,   3, 
      0,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
    171, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171, 
     83,  72,  69,  88, 232,   5, 
      0,   0,  80,   0,   0,   0, 
    122,   1,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
    189,   0,   0,   0,  80,   0, 
      0,   0, 184,   0,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   4,   0, 
      0,   0,  54,   0,   0,   8, 
    242,   0,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  18,   0,  16,   0, 
      1,   0,   0,   0,  10, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8,  66,   0, 
     16,   0,   1,   0,   0,   0, 
     10, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,   3,   0,   4,   3, 
     42,   0,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    242,   0,  16,   0,   2,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  34,   0,  16,   0, 
      1,   0,   0,   0,  26, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8,  66,   0, 
     16,   0,   1,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   1,   0, 
      0,   0,   3,   0,   4,   3, 
     42,   0,  16,   0,   1,   0, 
      0,   0,  50,   0,   0,  10, 
    194,   0,  16,   0,   1,   0, 
      0,   0,   6,   4,  16,   0, 
      1,   0,   0,   0,   6, 132, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   6,  20, 
     16,   0,   2,   0,   0,   0, 
     72,   0,   0, 141, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   3,   0, 
      0,   0, 230,  10,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   7, 242,   0,  16,   0, 
      2,   0,   0,   0,  70,  14, 
     16,   0,   2,   0,   0,   0, 
     70,  14,  16,   0,   3,   0, 
      0,   0,   0,   0,   0,   7, 
     34,   0,  16,   0,   1,   0, 
      0,   0,  26,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     22,   0,   0,   1,  54,   0, 
      0,   5, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   2,   0,   0,   0, 
      0,   0,   0,   7,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  22,   0, 
      0,   1,  56,   0,   0,   8, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0, 246, 143, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 114,   0,  16,   0, 
      1,   0,   0,   0,  70,  18, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    189,   0,   0,   0,  21,   0, 
      1,   0,  41,  11,  11,   0, 
    120,   1, 237,  86,  65, 107, 
     19,  65,  20, 126,  51, 187, 
    153,  76, 118, 103, 119,  27, 
    173, 184, 149, 138,  41, 109, 
     65,  60,   4,  91,  61, 148, 
    226, 161, 148,  82, 168,  32, 
     72, 235,  41,  46,  37,  73, 
    155,  90, 113, 219,  74, 210, 
     74, 148,  18, 171, 136,  94, 
    122, 240,   7, 248,   3,  60, 
    122, 246,  96, 106,  15,  50, 
     39,  17, 207,  34,  94, 122, 
    243,  80, 122, 242,  32, 212, 
    247,  54, 187, 186,  21, 123, 
     43,  40, 216,  15, 190, 247, 
    230, 123,  51, 251, 230, 101, 
    102,  55,  51, 182,  13,  17, 
    174, 124, 234, 223,  33, 127, 
    255, 201, 250, 203,  47,  28, 
     32, 131, 109, 134,  36,  79, 
    104,  74, 178,   0, 187,  57, 
    178,   0, 231, 201, 208,   0, 
     28, 235, 160, 243, 145,  35, 
    168, 199, 208,  83,  95, 194, 
     14, 244, 219, 247, 104, 105, 
     76,   1,  89,  50,   0, 166, 
    209,  79,  34,  77, 228,  40, 
    210,  69,  82,  58,  66, 226, 
      9, 241, 116,   7,  98,   4, 
    129, 236,  69, 226, 244,  17, 
    187, 145, 148, 139,  64, 250, 
    192, 248,  65,  50, 127, 198, 
     85,  36, 213, 127,  24, 168, 
    182,  36,  23, 121, 170, 147, 
     64, 191, 229, 111, 163,  83, 
    215,  47,  75,  76, 246, 137, 
     64, 251, 145,  70, 122, 141, 
     95, 197,  11, 139,  91,  17, 
     97,  43, 214, 201,  94, 110, 
     33,  15, 195,  62, 162, 139, 
     26,  41,  36, 154, 242,   9, 
    240, 208, 162, 103, 231, 162, 
    253,  22, 188,  63, 122,  47, 
    254,  23,  36, 239,  33, 173, 
      5, 189, 139, 180,  54, 157, 
     21, 233, 128, 250, 167, 144, 
      0,  27, 109, 226, 115,  31, 
    190, 145,  74,  52, 251,  77, 
     27, 164, 163, 141,  75, 105, 
    250,  48,  13, 152, 229,  54, 
    159, 245, 122, 227, 126,   3, 
      2, 212,  65,  74, 151,  80, 
    151,  82, 186, 140, 186, 156, 
    210,  85, 212, 213, 159,  58, 
    206, 239,  69, 243, 241,  12, 
    237,  51,   6,  91, 255,  50, 
    143, 107,  60,  26,  30, 215, 
    120,  52,  60, 174, 241, 200, 
     24, 129,  27, 223, 223,  72, 
    182, 189, 245,  16,  54, 219, 
    144, 123, 186, 189, 193, 253, 
    214,   6,  23,  72, 137, 180, 
     90, 192, 116, 212,  39, 160, 
    240,  81, 249,  80,  86, 220, 
    109,   9,  79, 174, 251, 240, 
    172, 205,  49, 230, 156, 129, 
    178, 195, 187,  90,  60,  47, 
    215, 243,  24,  51, 115,  99, 
    219,  32,  11,  26, 114,   3, 
    154, 201,  62, 205, 114, 131, 
     58, 202, 173, 186,  20,  51, 
     79,  40, 240,  94, 236,  74, 
     57,   0, 119, 189, 205, 182, 
    202, 251, 194,  56,  41, 132, 
    121,  74, 138, 204, 105,  75, 
    236, 153, 166, 248, 186, 255, 
    184, 189, 167, 148, 216,  65, 
     15, 166, 175, 153, 217, 163, 
     61, 223, 116,  33, 211, 171, 
     65,  20,  52, 119, 176, 157, 
    197, 120, 230, 172, 102, 162, 
     79, 123,  66, 184, 158, 148, 
     46, 203, 226,  56, 203, 114, 
     13,  79, 184, 102,  94, 186, 
     52, 206, 112, 176, 237,  72, 
     55, 227,  88, 120,  52, 251, 
     26,  24, 230, 224,  88, 155, 
      1, 154,  65, 143, 102,  12, 
    115, 112, 172, 209,  96,  90, 
    216, 166, 202, 218,  66,  73, 
     91,  42, 176,  45, 197, 109, 
    112,  13, 155, 187, 166, 141, 
    243,  37, 231,  64, 119, 242, 
     63, 254, 168, 221, 131,  54, 
    106,  34, 232,  12,  74,   3, 
    175,   8,  44, 233, 163, 235, 
      2, 158,  71,  44,  25, 163, 
    144, 164, 125,  11,  27,   8, 
     25, 123,  58, 195,  48, 142, 
    119, 136, 228, 201,  14,  70, 
    226, 254, 139, 177, 167, 124, 
    116, 165, 124,  61, 113,  97, 
    234, 102, 245, 195, 216, 141, 
     56, 126,  61, 246, 148, 131, 
    154, 202, 232,  83, 239, 226, 
     88,  24, 251,  18, 146, 158, 
     95,  24,  13, 170, 149, 176, 
    242, 160,  81,  92,  93, 169, 
    175, 220,  41,  54,  86, 235, 
    181, 202, 210, 112,  48,  95, 
    187,  23, 212, 154, 171, 181, 
    250, 114,  37,  12, 110,  45, 
     52,  47,   7,  51, 245, 185, 
     96, 186, 182,  60,  95, 171, 
      7,  19, 151,  38, 134, 154, 
    193, 204,  98,   5,  69,  35, 
    152,  28,  95, 105,  14, 143, 
    135, 107, 245, 107, 107,  97, 
    113,  33,  28,  26,  42,  46, 
    134, 141,  16, 115, 119, 240, 
     57, 158, 145,  14, 209,  44, 
    114, 169, 114, 123,  25, 126, 
      0, 232,  13,  64,   0,   0, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,  27,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  11,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
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
