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
//   float4 offset;                     // Offset:   16 Size:    16
//   float4 scolor;                     // Offset:   32 Size:    16
//   float4 scolor2;                    // Offset:   48 Size:    16
//   float4 srctexscale;                // Offset:   64 Size:    16
//   float4 texscale;                   // Offset:   80 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_srctex                    sampler      NA          NA    0        1
// sampler_tex                       sampler      NA          NA    1        1
// srctex                            texture  float4          2d    0        1
// tex                               texture  float4          2d    1        1
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
// First Precompiled Shader at offset:[340]
// Embedded Data:
//  0x00000154 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000014f - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[6], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 3
mov r0.xy, l(0,0,0,0)
mov r0.z, -cb0[0].x
loop 
  lt r0.w, cb0[0].x, r0.z
  breakc_nz r0.w
  add r1.x, r0.z, cb0[1].x
  mov r2.xy, r0.yxyy
  mov r2.z, -cb0[0].y
  loop 
    lt r0.w, cb0[0].y, r2.z
    breakc_nz r0.w
    add r1.y, r2.z, cb0[1].y
    mad r1.zw, r1.xxxy, cb0[5].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.zwzz, t1.xyzw, s1, l(0.000000)
    add r2.y, r0.w, r2.y
    mad r1.yz, -r1.xxyx, cb0[5].xxyx, v2.xxyx
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.yzyy, t1.xyzw, s1, l(0.000000)
    add r2.x, r0.w, r2.x
    add r2.z, r2.z, l(1.000000)
  endloop 
  mov r0.xy, r2.yxyy
  add r0.z, r0.z, l(1.000000)
endloop 
mul r0.xy, r0.xyxx, cb0[0].wwww
mad r0.xy, -r0.xyxx, cb0[0].zzzz, l(1.000000, 1.000000, 0.000000, 0.000000)
add_sat r0.xy, -r0.yxyy, r0.xyxx
mul r0.zw, v2.xxxy, cb0[4].xxxy
sample_l_indexable(texture2d)(float,float,float,float) r1.xyzw, r0.zwzz, t0.xyzw, s0, l(0.000000)
mov r2.x, cb0[2].w
mov r2.y, cb0[3].w
mul r0.xy, r0.xyxx, r2.xyxx
mul r2.xyzw, r0.yyyy, cb0[3].xyzw
mad r0.xyzw, cb0[2].xyzw, r0.xxxx, r2.xyzw
add r2.x, -r1.w, l(1.000000)
mad r0.xyzw, r0.xyzw, r2.xxxx, r1.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[340], bundle is:[225] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSColor2OuterBevelMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 492;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 5;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_SAMPLER, 0, offset 16:19 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 1, offset 20:23 dwords
;  extUserElements 1[4] = IMM_CONST_BUFFER, 0, offset 24:27 dwords
NumVgprs             = 15;
NumSgprs             = 26;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000003
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
  s_load_dwordx8  s[4:11], s[0:1], 0x14                 // 000000000004: C0C20114
  s_waitcnt     lgkmcnt(0)                              // 000000000008: BF8C007F
  s_buffer_load_dwordx4  s[12:15], s[8:11], 0x00        // 00000000000C: C2860900
  v_mov_b32     v12, 0                                  // 000000000010: 7E180280
  v_mov_b32     v3, 0                                   // 000000000014: 7E060280
  s_load_dwordx8  s[16:23], s[0:1], 0x08                // 000000000018: C0C80108
  s_waitcnt     lgkmcnt(0)                              // 00000000001C: BF8C007F
  v_max_f32     v4, -s12, -s12                          // 000000000020: D2200004 6000180C
label_000A:
  v_mov_b32     v5, s12                                 // 000000000028: 7E0A020C
  v_cmp_gt_f32  vcc, v4, v5                             // 00000000002C: 7C080B04
  s_cbranch_vccnz  label_0030                           // 000000000030: BF870023
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x04          // 000000000034: C2410904
  s_waitcnt     lgkmcnt(0)                              // 000000000038: BF8C007F
  v_add_f32     v5, s2, v4                              // 00000000003C: 060A0802
  v_max_f32     v6, -s13, -s13                          // 000000000040: D2200006 60001A0D
label_0012:
  v_mov_b32     v7, s13                                 // 000000000048: 7E0E020D
  v_cmp_gt_f32  vcc, v6, v7                             // 00000000004C: 7C080F06
  s_cbranch_vccnz  label_002E                           // 000000000050: BF870019
  s_buffer_load_dwordx2  s[24:25], s[8:11], 0x14        // 000000000054: C24C0914
  v_interp_p1_f32  v7, v0, attr2.x                      // 000000000058: C81C0800
  v_interp_p1_f32  v8, v0, attr2.y                      // 00000000005C: C8200900
  v_interp_p2_f32  v7, v1, attr2.x                      // 000000000060: C81D0801
  v_interp_p2_f32  v8, v1, attr2.y                      // 000000000064: C8210901
  v_add_f32     v9, s3, v6                              // 000000000068: 06120C03
  s_waitcnt     lgkmcnt(0)                              // 00000000006C: BF8C007F
  v_mad_legacy_f32  v10, v5, s24, v7                    // 000000000070: D280000A 041C3105
  v_mad_legacy_f32  v11, v9, s25, v8                    // 000000000078: D280000B 04203309
  v_mad_legacy_f32  v7, -v5, s24, v7                    // 000000000080: D2800007 241C3105
  v_mad_legacy_f32  v8, -v9, s25, v8                    // 000000000088: D2800008 24203309
  image_sample_lz  v[9:12], v[10:11], s[16:23], s[4:7] dmask:0x8 // 000000000090: F09C0800 0024090A
  image_sample_lz  v[7:10], v[7:8], s[16:23], s[4:7] dmask:0x8 // 000000000098: F09C0800 00240707
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000000A0: BF8C0F71
  v_add_f32     v12, v12, v9                            // 0000000000A4: 0618130C
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000A8: BF8C0F70
  v_add_f32     v3, v3, v7                              // 0000000000AC: 06060F03
  v_add_f32     v6, 1.0, v6                             // 0000000000B0: 060C0CF2
  s_branch      label_0012                              // 0000000000B4: BF82FFE4
label_002E:
  v_add_f32     v4, 1.0, v4                             // 0000000000B8: 060808F2
  s_branch      label_000A                              // 0000000000BC: BF82FFDA
label_0030:
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x10          // 0000000000C0: C2410910
  v_interp_p1_f32  v4, v0, attr2.x                      // 0000000000C4: C8100800
  v_interp_p1_f32  v5, v0, attr2.y                      // 0000000000C8: C8140900
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 0000000000CC: C0C80100
  s_load_dwordx4  s[4:7], s[0:1], 0x10                  // 0000000000D0: C0820110
  v_interp_p2_f32  v4, v1, attr2.x                      // 0000000000D4: C8110801
  v_interp_p2_f32  v5, v1, attr2.y                      // 0000000000D8: C8150901
  s_waitcnt     lgkmcnt(0)                              // 0000000000DC: BF8C007F
  v_mul_legacy_f32  v4, s2, v4                          // 0000000000E0: 0E080802
  v_mul_legacy_f32  v5, s3, v5                          // 0000000000E4: 0E0A0A03
  image_sample_lz  v[4:7], v[4:5], s[16:23], s[4:7] dmask:0xf // 0000000000E8: F09C0F00 00240404
  s_buffer_load_dwordx8  s[0:7], s[8:11], 0x08          // 0000000000F0: C2C00908
  v_mul_legacy_f32  v2, s15, v12                        // 0000000000F4: 0E04180F
  v_mul_legacy_f32  v3, s15, v3                         // 0000000000F8: 0E06060F
  v_mad_legacy_f32  v2, -v2, s14, 1.0                   // 0000000000FC: D2800002 23C81D02
  v_mad_legacy_f32  v3, -v3, s14, 1.0                   // 000000000104: D2800003 23C81D03
  v_add_f32     v8, v3, -v2 clamp                       // 00000000010C: D2060808 40020503
  v_add_f32     v2, v2, -v3 clamp                       // 000000000114: D2060802 40020702
  s_waitcnt     lgkmcnt(0)                              // 00000000011C: BF8C007F
  v_mul_legacy_f32  v3, s7, v8                          // 000000000120: 0E061007
  v_mul_legacy_f32  v8, s4, v3                          // 000000000124: 0E100604
  v_mul_legacy_f32  v9, s5, v3                          // 000000000128: 0E120605
  v_mul_legacy_f32  v10, s6, v3                         // 00000000012C: 0E140606
  v_interp_p1_f32  v12, v0, attr1.x                     // 000000000130: C8300400
  v_mul_legacy_f32  v2, s3, v2                          // 000000000134: 0E040403
  v_mul_legacy_f32  v3, s7, v3                          // 000000000138: 0E060607
  v_interp_p1_f32  v13, v0, attr1.y                     // 00000000013C: C8340500
  v_mac_legacy_f32  v8, s0, v2                          // 000000000140: 0C100400
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000144: BF8C0F70
  v_sub_f32     v11, 1.0, v7                            // 000000000148: 08160EF2
  v_interp_p1_f32  v14, v0, attr1.z                     // 00000000014C: C8380600
  v_mac_legacy_f32  v3, s3, v2                          // 000000000150: 0C060403
  v_interp_p2_f32  v12, v1, attr1.x                     // 000000000154: C8310401
  v_mac_legacy_f32  v9, s1, v2                          // 000000000158: 0C120401
  v_mac_legacy_f32  v4, v8, v11                         // 00000000015C: 0C081708
  v_interp_p2_f32  v13, v1, attr1.y                     // 000000000160: C8350501
  v_mac_legacy_f32  v7, v3, v11                         // 000000000164: 0C0E1703
  v_mac_legacy_f32  v10, s2, v2                         // 000000000168: 0C140402
  v_mac_legacy_f32  v5, v9, v11                         // 00000000016C: 0C0A1709
  v_interp_p2_f32  v14, v1, attr1.z                     // 000000000170: C8390601
  v_mul_legacy_f32  v3, v4, v12                         // 000000000174: 0E061904
  v_mac_legacy_f32  v6, v10, v11                        // 000000000178: 0C0C170A
  v_interp_p1_f32  v2, v0, attr1.w                      // 00000000017C: C8080700
  v_mul_legacy_f32  v4, v5, v13                         // 000000000180: 0E081B05
  v_mul_legacy_f32  v5, v6, v14                         // 000000000184: 0E0A1D06
  v_interp_p2_f32  v2, v1, attr1.w                      // 000000000188: C8090701
  v_interp_p1_f32  v6, v0, attr0.x                      // 00000000018C: C8180000
  v_interp_p1_f32  v8, v0, attr0.y                      // 000000000190: C8200100
  v_interp_p1_f32  v9, v0, attr0.z                      // 000000000194: C8240200
  v_interp_p1_f32  v0, v0, attr0.w                      // 000000000198: C8000300
  v_mul_legacy_f32  v3, v3, v2                          // 00000000019C: 0E060503
  v_mul_legacy_f32  v4, v4, v2                          // 0000000001A0: 0E080504
  v_mul_legacy_f32  v5, v5, v2                          // 0000000001A4: 0E0A0505
  v_interp_p2_f32  v6, v1, attr0.x                      // 0000000001A8: C8190001
  v_interp_p2_f32  v8, v1, attr0.y                      // 0000000001AC: C8210101
  v_interp_p2_f32  v9, v1, attr0.z                      // 0000000001B0: C8250201
  v_mul_legacy_f32  v2, v7, v2                          // 0000000001B4: 0E040507
  v_interp_p2_f32  v0, v1, attr0.w                      // 0000000001B8: C8010301
  v_mac_legacy_f32  v3, v6, v2                          // 0000000001BC: 0C060506
  v_mac_legacy_f32  v4, v8, v2                          // 0000000001C0: 0C080508
  v_mac_legacy_f32  v5, v9, v2                          // 0000000001C4: 0C0A0509
  v_mac_legacy_f32  v2, v0, v2                          // 0000000001C8: 0C040500
  v_mul_legacy_f32  v0, v3, v2                          // 0000000001CC: 0E000503
  v_mul_legacy_f32  v1, v4, v2                          // 0000000001D0: 0E020504
  v_mul_legacy_f32  v3, v5, v2                          // 0000000001D4: 0E060505
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 0000000001D8: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v3, v2                       // 0000000001DC: 5E020503
  exp           mrt0, v0, v0, v1, v1 compr vm           // 0000000001E0: F800140F 00000100
  s_endpgm                                              // 0000000001E8: BF810000
end


// Approximately 43 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSColor2OuterBevelMul[] =
{
     68,  88,  66,  67,  32,  47, 
    179, 155, 188,  45, 139, 223, 
    142,  38,  96, 211,  43, 214, 
    206,  71,   1,   0,   0,   0, 
      4,  13,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    236,   2,   0,   0,  80,   3, 
      0,   0, 132,   3,   0,   0, 
    104,  12,   0,   0,  82,  68, 
     69,  70, 176,   2,   0,   0, 
      1,   0,   0,   0,  12,   1, 
      0,   0,   5,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    115,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    220,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 235,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 247,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    254,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   1,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,   2,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    115, 114,  99, 116, 101, 120, 
      0, 115,  97, 109, 112, 108, 
    101, 114,  95, 116, 101, 120, 
      0, 115, 114,  99, 116, 101, 
    120,   0, 116, 101, 120,   0, 
     67, 111, 110, 115, 116,  97, 
    110, 116, 115,   0,   2,   1, 
      0,   0,   6,   0,   0,   0, 
     36,   1,   0,   0,  96,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  20,   2, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  36,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  72,   2,   0,   0, 
     16,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     36,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     79,   2,   0,   0,  32,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  36,   2, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  86,   2, 
      0,   0,  48,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  36,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  94,   2,   0,   0, 
     64,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     36,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    106,   2,   0,   0,  80,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  36,   2, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 102, 115, 
    105, 122, 101,   0, 102, 108, 
    111,  97, 116,  52,   0, 171, 
    171, 171,   1,   0,   3,   0, 
      1,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  26,   2, 
      0,   0, 111, 102, 102, 115, 
    101, 116,   0, 115,  99, 111, 
    108, 111, 114,   0, 115,  99, 
    111, 108, 111, 114,  50,   0, 
    115, 114,  99, 116, 101, 120, 
    115,  99,  97, 108, 101,   0, 
    116, 101, 120, 115,  99,  97, 
    108, 101,   0,  77, 105,  99, 
    114, 111, 115, 111, 102, 116, 
     32,  40,  82,  41,  32,  72, 
     76,  83,  76,  32,  83, 104, 
     97, 100, 101, 114,  32,  67, 
    111, 109, 112, 105, 108, 101, 
    114,  32,  70, 111, 114,  32, 
     68, 117, 114,  97, 110, 103, 
    111,  32,  57,  46,  51,  48, 
     46,  49,  50,  48,  57,  56, 
     46,  48,   0, 171,  73,  83, 
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
     83,  72,  69,  88, 220,   8, 
      0,   0,  80,   0,   0,   0, 
     55,   2,   0,   0,  53,  16, 
      0,   0,   5,   0,   0,   0, 
     84,   1,   0,   0,  80,   0, 
      0,   0,  79,   1,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   6,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   1,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      2,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   3,   0,   0,   0, 
     54,   0,   0,   8,  50,   0, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   7, 
     66,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     48,   0,   0,   1,  49,   0, 
      0,   8, 130,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  42,   0, 
     16,   0,   0,   0,   0,   0, 
      3,   0,   4,   3,  58,   0, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  10, 128,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
     50,   0,  16,   0,   2,   0, 
      0,   0,  22,   5,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  66,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   2,   0, 
      0,   0,   3,   0,   4,   3, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     34,   0,  16,   0,   1,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0,  26, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  10, 194,   0,  16,   0, 
      1,   0,   0,   0,   6,   4, 
     16,   0,   1,   0,   0,   0, 
      6, 132,  32,   0,   0,   0, 
      0,   0,   5,   0,   0,   0, 
      6,  20,  16,   0,   2,   0, 
      0,   0,  72,   0,   0, 141, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 130,   0,  16,   0, 
      0,   0,   0,   0, 230,  10, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      2,   0,   0,   0,  50,   0, 
      0,  11,  98,   0,  16,   0, 
      1,   0,   0,   0,   6,   1, 
     16, 128,  65,   0,   0,   0, 
      1,   0,   0,   0,   6, 129, 
     32,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,   6,  17, 
     16,   0,   2,   0,   0,   0, 
     72,   0,   0, 141, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    130,   0,  16,   0,   0,   0, 
      0,   0, 150,   5,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   1,   0,   0,   0, 
      0,  96,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   7,  18,   0,  16,   0, 
      2,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     66,   0,  16,   0,   2,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     22,   0,   0,   1,  54,   0, 
      0,   5,  50,   0,  16,   0, 
      0,   0,   0,   0,  22,   5, 
     16,   0,   2,   0,   0,   0, 
      0,   0,   0,   7,  66,   0, 
     16,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  22,   0, 
      0,   1,  56,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0, 246, 143, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  50,   0, 
      0,  14,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0, 166, 138, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,  32,   0,   8,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     22,   5,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
    194,   0,  16,   0,   0,   0, 
      0,   0,   6,  20,  16,   0, 
      2,   0,   0,   0,   6, 132, 
     32,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0, 242,   0, 
     16,   0,   1,   0,   0,   0, 
    230,  10,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   6, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  54,   0,   0,   6, 
     34,   0,  16,   0,   2,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  56,   0,   0,   7, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   2,   0,   0,   0, 
     56,   0,   0,   8, 242,   0, 
     16,   0,   2,   0,   0,   0, 
     86,   5,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  50,   0,   0,  10, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   6,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   2,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16, 128,  65,   0, 
      0,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    114,   0,  16,   0,   1,   0, 
      0,   0,  70,  18,  16,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    114,  32,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     53,  16,   0,   0, 227,   0, 
      0,   0,  21,   0,   1,   0, 
     41, 232,  11,   0, 120,   1, 
    237,  86, 207, 111,  27,  69, 
     20, 126,  51,  59,  59,  59, 
    187, 153, 142,  77,  48, 141, 
     83,  18, 112, 137,  11, 229, 
     98, 225, 166,  72, 128,  16, 
     10,  80, 113,   2,  85, 144, 
     91, 181, 180, 113,  26, 135, 
     34, 156,   4, 156,  52,  10, 
    168,  50, 105,  14, 112, 233, 
    129,  63, 160,  55, 254,   0, 
    254, 128,  74,  56,  88,  72, 
    204,  57,  55,  16,  71,  14, 
     28,  42,  81, 133,  11,   7, 
    164, 242, 189, 253,  81, 172, 
    138, 222,  34, 129,  68,  62, 
    249, 155, 183, 223, 188, 153, 
    247, 222, 206, 172,  60, 243, 
    235,   4, 101, 248,  97, 126, 
    239,  55, 182, 179, 253, 231, 
    190, 121,  53,  36, 194, 143, 
      4, 200, 150, 177,  99, 184, 
     37, 250,  62, 225, 150, 232, 
     44,  55,  60,  64,  18,  85, 
     96,  78, 129, 119, 161,  23, 
     96, 217,  87,  50, 135, 255, 
    174,   5, 223, 179, 120, 122, 
     10,  60, 137, 160, 235, 176, 
     87,  64,   7, 190,  11, 214, 
     65,  12, 201,  16, 112,  83, 
    128, 253, 140, 210,  87,  66, 
    129, 207, 131,  92,  14,  74, 
    200, 230, 199,  32, 131, 125, 
    220, 247,   0, 103, 184, 249, 
    103, 172, 129,  92, 255, 163, 
    192, 181, 149, 245, 112,  13, 
    156, 135, 193, 239, 251, 111, 
     35,  95, 147, 252,  77, 185, 
     70, 214, 229,  62,  49, 120, 
     63, 198, 193, 107,  89, 118, 
    173,  20, 251,  88, 238, 239, 
     86, 161, 203, 189, 220,   7, 
     31, 133, 251, 192,  12,  63, 
    140, 161, 212,  92, 135,  46, 
     86,  84, 139, 167, 169, 193, 
     86, 206, 101, 223, 197, 255, 
      5, 252, 253,  49, 120,  45, 
    120, 119, 170,  96, 190,  34, 
     57, 216, 191,  13,  18, 237, 
     14, 153,  95, 213, 233,  15, 
     86, 165,  22,  15, 233, 128, 
    117, 182, 113,  99, 154,   3, 
      7, 116,  57,   8, 229, 229, 
    202,  76, 225,  15,  40, 133, 
     78, 199, 244,  37, 232,  75, 
     99, 122,   9, 122, 105,  76, 
     47,  67,  47,  63, 208,  69, 
    252,  74, 150,  47, 144, 188, 
    207, 232,  28, 252, 151, 121, 
     92, 227, 209, 240, 184, 198, 
    163, 225, 113, 141,  71, 198, 
     12,  50, 248, 243, 219, 154, 
     24, 237, 127,  78, 183, 134, 
     20, 127,  49, 218, 149, 245, 
    193, 174, 212,   3,  35, 124, 
    214, 167, 168, 113,  96, 235, 
    180, 100, 101,  50,  80,  19, 
    230, 198,  28, 125,  57,  84, 
    241, 235,  35, 246,  73, 147, 
    104,  13, 255, 137,  83, 180, 
    116,  66, 186, 129, 174, 152, 
     27, 211, 240, 215, 226, 183, 
     71, 100, 102,  60, 197,  13, 
     47, 204, 172,  23, 241, 105, 
     31, 216,  73, 205, 115,  18, 
    218,  61,   8, 219,  51, 106, 
      2,  54, 158, 111, 168,  40, 
    215,  77, 147, 235,  38, 153, 
    219, 247, 146, 184,  73, 108, 
    163, 168,  73, 159,  84, 110, 
     13, 237, 227, 117, 253,  49, 
    108,  80, 209, 250, 208,  90, 
    253, 203, 253, 189, 225, 161, 
     49, 250,  39, 216,  42, 106, 
     33,  83,  69, 174, 154,  39, 
    212,  92,  21, 123, 251, 194, 
     60, 134, 156,  79, 248, 188, 
     70, 227, 130,  36, 113,  84, 
    185, 125,  79,  41, 132, 143, 
    247,  71, 149, 186, 114,   8, 
    229,  36, 114, 202,  89,  63, 
     23, 192,   6, 176,   8, 121, 
    128, 255, 235,   5,   9,  43, 
     35, 185, 192, 243, 163, 170, 
    118,  74,  87,  93, 168,  39, 
    157, 214,  53,  71, 234,   5, 
     31,  40, 229,  34, 204, 167, 
    240, 188,  39,  85, 181,  92, 
    219, 161,  59, 105,  72, 191, 
      4, 159, 182,  66, 181, 189, 
     80, 147, 214,  76,  25,  43, 
    194,  23, 125,  48, 229, 172, 
     84,  53,  27,  79,  37,  86, 
    232, 151, 189, 154, 214,  46, 
    153, 178, 150,  34, 227, 195, 
     39, 141, 211, 179, 137,  19, 
     81, 236, 113, 251, 193,  59, 
     52,  60, 201, 166, 199, 217, 
    225, 131,  16, 185,  67, 227, 
    194,  16, 126, 154, 246,  66, 
    156, 246,  66, 158, 241,  81, 
    168, 156,   8, 132, 215, 161, 
    182,  38,  52,  54,  14,  19, 
     75, 161, 178,  65,  72,  24, 
     47,  49,  30, 181, 149, 103, 
     87, 173,  60, 123, 110,  14, 
    167, 209, 102, 143,   0,  31, 
    111, 227, 192, 181,  38, 115, 
    113, 195,  87,  28, 214,  89, 
      7, 192, 231,  43, 206, 212, 
     76,  51, 107,  32, 235,  50, 
      6, 223, 217,  88, 223,  45, 
    238,  56,  63,  23, 182, 152, 
    135, 123,  17, 207, 250,  27, 
    182, 184,  19, 227,  35, 200, 
    192, 249, 248, 209, 126, 250, 
    251, 143, 175, 221, 236, 202, 
    179,  69, 127, 179, 176,  28, 
    131,  67, 190, 255, 204, 197, 
     59,  95,  23, 125,  11, 133, 
    189,  10, 242, 252, 213,  87, 
    210, 229,  78, 175, 243, 217, 
    102, 107, 107, 163, 191, 241, 
     81, 107, 115, 171, 223, 237, 
    172, 157,  75,  87, 186, 219, 
    105, 119, 103, 171, 219,  95, 
    239, 244, 210,  15,  86, 119, 
    206, 167, 139, 253, 171, 233, 
    123, 221, 245, 149, 110,  63, 
    189,  48, 127, 161, 189, 147, 
     46,  94, 235,  64, 108, 166, 
    111,  45, 190, 185, 209, 219, 
    232, 159, 187, 120,  29, 195, 
    223, 232, 110, 119, 123, 239, 
     92, 239, 181,  86, 123, 237, 
    118, 235,  90, 111, 179, 135, 
     36,  57, 238,  20, 169, 249, 
    106,  23, 129, 107, 157,  15, 
    215, 233,  47,  50, 235, 108, 
    200,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,  43,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  23,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  10,   0,   0,   0, 
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
