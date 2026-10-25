/* opts: --cpu Cortex-M3 -O1 --loose_implicit_cast -DSTM32F10X_CL -I{root}/lib/stdperiph35/Libraries/STM32F10x_StdPeriph_Driver/inc -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/CoreSupport -I{root}/lib/stdperiph35/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x */
/* Serial queues and the BMS/STA receive parsers. */
/* at: sub_08014AC8 0x08014AC8, sub_08014AFA 0x08014AFA, sub_08014B2E 0x08014B2E, sub_08014B5C 0x08014B5C, sub_08014B92 0x08014B92, sub_08014BCC 0x08014BCC, sub_08014F36 0x08014F36, sub_08014F94 0x08014F94, sub_08014FD6 0x08014FD6, sub_08015034 0x08015034, sub_08015058 0x08015058, sub_0801508A 0x0801508A, sub_080150EE 0x080150EE, sub_0801512E 0x0801512E */
/* at: g_20000020 0x20000020, g_20000044 0x20000044, g_2000004D 0x2000004D, g_2000004E 0x2000004E, g_200000E1 0x200000E1, g_200000E2 0x200000E2, g_200000E4 0x200000E4, g_200000E6 0x200000E6, g_200000EA 0x200000EA, g_20000109 0x2000010D, g_20000163 0x20000167, g_20000CF7 0x20000D0F, g_20000F53 0x20000F6B, g_20000F77 0x20000F8F, g_20001637 0x2000164F */
#include "../../include/ghidra_types.h"
#include "stm32f10x_usart.h"
void OS_ENTER_CRITICAL(void);
void OS_EXIT_CRITICAL(void);
void sub_08014BCC(int param_1);
int sub_08014FD6(int param_1, int param_2);
void sub_08014B92(void);
uint sub_08014F36(int param_1, undefined1 * param_2);
void sub_08014AFA(void);
int sub_0801508A(int param_1);
int sub_08015058(int param_1);
void sub_08015034(int param_1, uchar *param_2);
void sub_080150EE(int param_1, int param_2);
void sub_08014AC8(int param_1, uint param_2);
void sub_08014B2E(void);
void sub_08014B5C(void);
void sub_0801512E(int param_1, int param_2);
void sub_08014F94(int param_1, uint param_2, ushort param_3);

typedef __packed struct {uchar busy;ushort len,idx;uchar *buf;} SerialTx;
typedef __packed struct {uchar busy,a,b,idx;uchar *buf;uchar size;} SerialRx;
extern uchar g_20000F53[],g_20000F77[];
#define serial_rx ((SerialRx*)g_20000F53)
#define serial_tx ((SerialTx*)g_20000F77)

typedef int T8014AC8_iVar1;
typedef uint T8014AC8_uVar2;
extern uchar g_20000CF7[];
typedef uchar T8014AC8_g_20000CF7_v;
#define g_20000CF7_v (*(T8014AC8_g_20000CF7_v *)(g_20000CF7 + 0))
extern uchar g_20000F53[];
typedef uchar T8014AC8_DAT_20000f54;
#define DAT_20000f54 (*(T8014AC8_DAT_20000f54 *)(g_20000F53 + 1))
typedef uchar T8014AC8_DAT_20000f55;
#define DAT_20000f55 (*(T8014AC8_DAT_20000f55 *)(g_20000F53 + 2))
typedef uchar T8014AC8_DAT_20000f56;
#define DAT_20000f56 (*(T8014AC8_DAT_20000f56 *)(g_20000F53 + 3))
typedef uchar T8014AC8_DAT_20000f57;
#define DAT_20000f57 (*(T8014AC8_DAT_20000f57 *)(g_20000F53 + 4))
typedef uchar T8014AC8_DAT_20000f5b;
#define DAT_20000f5b (*(T8014AC8_DAT_20000f5b *)(g_20000F53 + 8))
typedef uchar T8014AC8_g_20000F53_v;
#define g_20000F53_v (*(T8014AC8_g_20000F53_v *)(g_20000F53 + 0))

void sub_08014AC8(int param_1, uint param_2)
{
  SerialRx *p;
  uint i;
  for(i=0;i<param_2;i=(i+1)&0xffff) (g_20000CF7+param_1*151)[i]=0;
  p=&serial_rx[param_1];
  p->busy=1;p->a=0;p->b=0;p->idx=0;
  p->buf=g_20000CF7+param_1*151;p->size=param_2;
}

#undef g_20000CF7_v
#undef DAT_20000f54
#undef DAT_20000f55
#undef DAT_20000f56
#undef DAT_20000f57
#undef DAT_20000f5b
#undef g_20000F53_v


extern uchar g_2000004E[];
typedef uchar T8014AFA_g_2000004E_v;
#define g_2000004E_v (*(T8014AFA_g_2000004E_v *)(g_2000004E + 0))
extern uchar g_20000F77[];
typedef ushort T8014AFA_DAT_20000f78;
#define DAT_20000f78 (*(T8014AFA_DAT_20000f78 *)(g_20000F77 + 1))
typedef ushort T8014AFA_DAT_20000f7a;
#define DAT_20000f7a (*(T8014AFA_DAT_20000f7a *)(g_20000F77 + 3))
typedef int T8014AFA_DAT_20000f7c;
#define DAT_20000f7c (*(T8014AFA_DAT_20000f7c *)(g_20000F77 + 5))
typedef uchar T8014AFA_g_20000F77_v;
#define g_20000F77_v (*(T8014AFA_g_20000F77_v *)(g_20000F77 + 0))

void sub_08014AFA(void)
{
  SerialTx *p=serial_tx;
  p+=0;
  if(p->idx>=p->len) {p->busy=0;g_2000004E_v=1;return;}
  USART_SendData(USART1,p->buf[p->idx]);p->idx++;p->busy=1;
}
#undef g_2000004E_v
#undef DAT_20000f78
#undef DAT_20000f7a
#undef DAT_20000f7c
#undef g_20000F77_v


typedef ushort T8014B2E_v0;
typedef int T8014B2E_v4;
extern uchar g_20000F77[];
typedef uchar T8014B2E_DAT_20000f77;
#define DAT_20000f77 (*(T8014B2E_DAT_20000f77 *)(g_20000F77 + 0))

void sub_08014B2E(void)
{
  SerialTx *p=serial_tx;
  p+=1;
  if(p->idx>=p->len) {p->busy=0;return;}
  USART_SendData(USART2,p->buf[p->idx]);p->idx++;p->busy=1;
}
#undef DAT_20000f77


typedef ushort T8014B5C_v0;
typedef int T8014B5C_v4;
typedef int T8014B5C_v5;
extern uchar g_2000004D[];
typedef uchar T8014B5C_DAT_2000004d;
#define DAT_2000004d (*(T8014B5C_DAT_2000004d *)(g_2000004D + 0))
extern uchar g_20000F77[];
typedef uchar T8014B5C_DAT_20000f77;
#define DAT_20000f77 (*(T8014B5C_DAT_20000f77 *)(g_20000F77 + 0))

void sub_08014B5C(void)
{
  SerialTx *p=serial_tx;
  p+=2;
  if(p->idx>=p->len) {p->busy=0;DAT_2000004d=1;return;}
  USART_SendData(USART3,p->buf[p->idx]);p->idx++;p->busy=1;
}
#undef DAT_2000004d
#undef DAT_20000f77


extern uchar g_20000F77[];
typedef uchar T8014B92_DAT_20000f92;
#define DAT_20000f92 (*(T8014B92_DAT_20000f92 *)(g_20000F77 + 27))
typedef ushort T8014B92_DAT_20000f93;
#define DAT_20000f93 (*(T8014B92_DAT_20000f93 *)(g_20000F77 + 28))
typedef ushort T8014B92_DAT_20000f95;
#define DAT_20000f95 (*(T8014B92_DAT_20000f95 *)(g_20000F77 + 30))
typedef int T8014B92_DAT_20000f97;
#define DAT_20000f97 (*(T8014B92_DAT_20000f97 *)(g_20000F77 + 32))

void sub_08014B92(void)
{
  SerialTx *p=serial_tx;
  p+=3;
  if(p->idx>=p->len) {p->busy=0;USART_ITConfig(UART4,USART_IT_TXE,DISABLE);return;}
  USART_SendData(UART4,p->buf[p->idx]);p->idx++;p->busy=1;
}
#undef DAT_20000f92
#undef DAT_20000f93
#undef DAT_20000f95
#undef DAT_20000f97


typedef uchar T8014BCC_cVar1;
typedef uchar T8014BCC_bVar2;
typedef ushort T8014BCC_uVar3;
typedef uint T8014BCC_uVar4;
typedef uint T8014BCC_uVar5;
typedef ushort T8014BCC_sVar6;
typedef uint T8014BCC_uVar7;
extern uchar g_20000020[];
typedef volatile uchar T8014BCC_DAT_20000021;
#define DAT_20000021 (*(T8014BCC_DAT_20000021 *)(g_20000020 + 1))
typedef volatile ushort T8014BCC_DAT_20000022;
#define DAT_20000022 (*(T8014BCC_DAT_20000022 *)(g_20000020 + 2))
typedef volatile uchar T8014BCC_g_20000020_v;
#define g_20000020_v (*(T8014BCC_g_20000020_v *)(g_20000020 + 0))
extern uchar g_20000044[];
typedef uint T8014BCC_g_20000044_v;
#define g_20000044_v (*(T8014BCC_g_20000044_v *)(g_20000044 + 0))
extern uchar g_200000E1[];
typedef uchar T8014BCC_g_200000E1_v;
#define g_200000E1_v (*(T8014BCC_g_200000E1_v *)(g_200000E1 + 0))
extern uchar g_200000E2[];
typedef uchar T8014BCC_g_200000E2_v;
#define g_200000E2_v (*(T8014BCC_g_200000E2_v *)(g_200000E2 + 0))
extern uchar g_200000E4[];
typedef uchar T8014BCC_g_200000E4_v;
#define g_200000E4_v (*(T8014BCC_g_200000E4_v *)(g_200000E4 + 0))
extern uchar g_200000E6[];
typedef uchar T8014BCC_g_200000E6_v;
#define g_200000E6_v (*(T8014BCC_g_200000E6_v *)(g_200000E6 + 0))
extern uchar g_200000EA[];
typedef ushort T8014BCC_g_200000EA_v;
#define g_200000EA_v (*(T8014BCC_g_200000EA_v *)(g_200000EA + 0))
extern uchar g_20000163;
typedef uchar T8014BCC_g_20000163_v;
#define g_20000163_v (g_20000163)
extern uchar g_20000F53[];
typedef sbyte T8014BCC_DAT_20000f54;
#define DAT_20000f54 (*(T8014BCC_DAT_20000f54 *)(g_20000F53 + 1))
typedef uchar T8014BCC_DAT_20000f55;
#define DAT_20000f55 (*(T8014BCC_DAT_20000f55 *)(g_20000F53 + 2))
typedef uchar T8014BCC_DAT_20000f5b;
#define DAT_20000f5b (*(T8014BCC_DAT_20000f5b *)(g_20000F53 + 8))
typedef uchar T8014BCC_DAT_20001635;
#define DAT_20001635 (*(T8014BCC_DAT_20001635 *)(g_20000F53 + 1762))
typedef uchar T8014BCC_DAT_20001636;
#define DAT_20001636 (*(T8014BCC_DAT_20001636 *)(g_20000F53 + 1763))
typedef uchar T8014BCC_bRam20000f5e;
#define bRam20000f5e (*(T8014BCC_bRam20000f5e *)(g_20000F53 + 11))
typedef uchar T8014BCC_bRam20000f64;
#define bRam20000f64 (*(T8014BCC_bRam20000f64 *)(g_20000F53 + 17))
typedef uchar T8014BCC_bRam20000f67;
#define bRam20000f67 (*(T8014BCC_bRam20000f67 *)(g_20000F53 + 20))
typedef uchar T8014BCC_bRam20000f6d;
#define bRam20000f6d (*(T8014BCC_bRam20000f6d *)(g_20000F53 + 26))
typedef uchar T8014BCC_bRam20000f70;
#define bRam20000f70 (*(T8014BCC_bRam20000f70 *)(g_20000F53 + 29))
typedef uchar T8014BCC_bRam20000f76;
#define bRam20000f76 (*(T8014BCC_bRam20000f76 *)(g_20000F53 + 35))
typedef sbyte T8014BCC_cRam20000f5d;
#define cRam20000f5d (*(T8014BCC_cRam20000f5d *)(g_20000F53 + 10))
typedef sbyte T8014BCC_cRam20000f66;
#define cRam20000f66 (*(T8014BCC_cRam20000f66 *)(g_20000F53 + 19))
typedef sbyte T8014BCC_cRam20000f6f;
#define cRam20000f6f (*(T8014BCC_cRam20000f6f *)(g_20000F53 + 28))
typedef uchar T8014BCC_g_20000F53_v;
#define g_20000F53_v (*(T8014BCC_g_20000F53_v *)(g_20000F53 + 0))
typedef int T8014BCC_iRam20000f60;
#define iRam20000f60 (*(T8014BCC_iRam20000f60 *)(g_20000F53 + 13))
typedef int T8014BCC_iRam20000f69;
#define iRam20000f69 (*(T8014BCC_iRam20000f69 *)(g_20000F53 + 22))
typedef int T8014BCC_iRam20000f72;
#define iRam20000f72 (*(T8014BCC_iRam20000f72 *)(g_20000F53 + 31))
typedef uint T8014BCC_uRam20000f5c;
#define uRam20000f5c (*(T8014BCC_uRam20000f5c *)(g_20000F53 + 9))
typedef uint T8014BCC_uRam20000f65;
#define uRam20000f65 (*(T8014BCC_uRam20000f65 *)(g_20000F53 + 18))
typedef uint T8014BCC_uRam20000f6e;
#define uRam20000f6e (*(T8014BCC_uRam20000f6e *)(g_20000F53 + 27))
extern uchar g_20001637[];
typedef uchar T8014BCC_DAT_20001638;
#define DAT_20001638 (*(T8014BCC_DAT_20001638 *)(g_20001637 + 1))
typedef uchar T8014BCC_DAT_20001639;
#define DAT_20001639 (*(T8014BCC_DAT_20001639 *)(g_20001637 + 2))
typedef uchar T8014BCC_DAT_2000163a;
#define DAT_2000163a (*(T8014BCC_DAT_2000163a *)(g_20001637 + 3))
typedef uchar T8014BCC_DAT_2000163b;
#define DAT_2000163b (*(T8014BCC_DAT_2000163b *)(g_20001637 + 4))
typedef uchar T8014BCC_DAT_2000163c;
#define DAT_2000163c (*(T8014BCC_DAT_2000163c *)(g_20001637 + 5))
typedef uchar T8014BCC_DAT_2000163d;
#define DAT_2000163d (*(T8014BCC_DAT_2000163d *)(g_20001637 + 6))
typedef uchar T8014BCC_DAT_2000163e;
#define DAT_2000163e (*(T8014BCC_DAT_2000163e *)(g_20001637 + 7))
typedef uchar T8014BCC_g_20001637_v;
#define g_20001637_v (*(T8014BCC_g_20001637_v *)(g_20001637 + 0))

void sub_08014BCC(int param_1)
{
  SerialRx *p;
  uint address,mode,state;
  uchar *end;
  ushort expected;
  T8014BCC_cVar1 cVar1;
  T8014BCC_bVar2 bVar2;
  T8014BCC_uVar4 uVar4;
  T8014BCC_uVar5 uVar5;
  T8014BCC_sVar6 sVar6;
  T8014BCC_uVar7 uVar7;

  sVar6 = 0;
  uVar7 = 0;
  p=&serial_rx[param_1];
  if (param_1 == 0) {
    const uchar *address_ptr=&g_20000163;const uchar *mode_ptr=g_200000E1;uint mode_address_diagnostic=0;
    /* ARMCC deletes these initialized pointer comparisons after allocation.
       Their lifetimes preserve the matched literal-load schedule. */
    mode_address_diagnostic=(uint)(g_20000020!=(const uchar*)p);
    address=*address_ptr;
    mode=*mode_ptr;
    mode_address_diagnostic=(uint)(mode_ptr!=(const uchar*)p);
    mode_address_diagnostic=(uint)(address_ptr!=(const uchar*)p);
    (void)mode_address_diagnostic;
    if (address == 0) {
      if (mode == 1) {
        cVar1 = (uchar)(USART_ReceiveData(USART1));
        state=g_20000020_v;
        if(state==0) {if(cVar1=='O'||cVar1=='N')g_20000020_v=1;else g_20000020_v=0;}
        else if(state==1) {if(cVar1=='K'||cVar1=='O')g_20000020_v=state+1;else g_20000020_v=0;}
        if (g_20000020_v >= 2) {
          g_20000020_v = 0;
          g_200000E4_v = 1;
          g_200000EA_v = 0;
        }
      }
      else {
        if (p->b < p->size) {
          *(char *)(((uint)p->buf) + (uint)p->b) = (char)(USART_ReceiveData(USART1));
          p->b = p->b + 1;
          p->a = p->a + '\x01';
        }
        else {
          USART_ReceiveData(USART1);
        }
        p->busy = 0;
      }
    }
    else if (((mode == 1) && (g_200000E2_v == '\0')) && (g_200000E6_v == '\0')) {
      bVar2 = (byte)(USART_ReceiveData(USART1));
      uVar5 = (uint)DAT_20000022;
      if (uVar5 == 0) {
        if ((bVar2 == 0x42) || (bVar2 == 0x45)) {
          g_20001637_v = bVar2;
          DAT_20000022 = 1;
        }
        else {
          DAT_20000022 = 0;
        }
      }
      else if (uVar5 == 1) {
        if ((bVar2 == 0x4d) || (bVar2 == 0x4e)) {
          DAT_20001638 = bVar2;
          DAT_20000022 = 2;
        }
        else {
          DAT_20000022 = 0;
        }
      }
      else if (uVar5 == 2) {
        if ((bVar2 == 0x53) || (bVar2 == 0x44)) {
          DAT_20001639 = bVar2;
          DAT_20000022 = 3;
        }
        else {
          DAT_20000022 = 0;
        }
      }
      else if (uVar5 == 3) {
        if (((g_20001637_v == 0x45) && (DAT_20001638 == 0x4e)) && (DAT_20001639 == 0x44)) {
          DAT_2000163a = bVar2;
          DAT_20000022 = uVar5+1;
        }
        else if (bVar2 == g_20000163_v) {
          if (((g_20001637_v == 0x42) && (DAT_20001638 == 0x4d)) && (DAT_20001639 == 0x53)) {
            DAT_2000163a = bVar2;
            DAT_20000022 = uVar5+1;
          }
        }
        else {
          DAT_20000022 = 0;
        }
        if (bVar2 < 6) {
          g_200000EA_v = 0;
        }
      }
      else {
        (&g_20001637_v)[uVar5] = bVar2;
        uVar4 = uVar5 + 1 & 0xffff;
        DAT_20000022 = uVar4;
        if (((g_20001637_v == 0x45) && (DAT_20001638 == 0x4e)) && (DAT_20001639 == 0x44)) {
          if (uVar4 >= 9) {
            DAT_20000022 = 0;
            g_200000E6_v = '\x01';
            g_20000044_v = (uint)DAT_2000163e + (uint)DAT_2000163b * 0x1000000 +
                         (uint)DAT_2000163c * 0x10000 + (uint)DAT_2000163d * 0x100;
          }
        }
        else if (uVar4 >= 0x406) {
          do {
            sVar6 = (ushort)(&g_20001637_v)[uVar7] + sVar6;
            uVar7 = uVar7 + 1 & 0xffff;
          } while (uVar7 < 0x404);
          end=g_20001637+uVar4;
          expected=(ushort)((int)(sbyte)end[-2]*256);
          expected+=end[-1];
          if(expected==sVar6)g_200000E2_v=1;else g_200000E2_v=0;
          DAT_20000022 = 0;
        }
      }
    }
    else {
      bVar2 = (byte)(USART_ReceiveData(USART1));
      state=DAT_20000021;
      if (state == 0) {
        if (bVar2 == 0x53) {
          DAT_20000021 = '\x01';
        }
        else {
          DAT_20000021 = '\0';
        }
      }
      else if(state==1) {
        if (bVar2 == 0x54) {
          DAT_20000021 = 2;
        }
        else {
          DAT_20000021 = '\0';
        }
      }
      else if(state==2) {
        if (bVar2 == 0x41) {
          DAT_20000021 = '\x03';
          g_200000E1_v = '\x01';
        }
        else {
          DAT_20000021 = '\0';
        }
      }
      else if(state==3) {
        if (bVar2 == g_20000163_v) {
          DAT_20000021 = 4;
        }
        else {
          DAT_20000021 = '\0';
        }
      }
      else {
        DAT_20000021 = '\0';
      }
      if ((uint)p->b < (uint)p->size) {
        *(byte *)(((uint)p->buf) + (uint)p->b) = bVar2;
        p->b = p->b + 1;
        p->a = p->a + '\x01';
      }
      else {
        USART_ReceiveData(USART1);
      }
      p->busy = 0;
    }
  }
  else if (param_1 == 1) {
    if (p->b < p->size) {
      *(char *)(((int)p->buf) + (uint)p->b) = (char)(USART_ReceiveData(USART2));
      p->b = p->b + 1;
      p->a = p->a + '\x01';
    }
    else {
      USART_ReceiveData(USART2);
    }
    p->busy = 0;
  }
  else if (param_1 == 2) {
    if (p->b < p->size) {
      *(char *)(((int)p->buf) + (uint)p->b) = (char)(USART_ReceiveData(USART3));
      p->b = p->b + 1;
      p->a = p->a + '\x01';
    }
    else {
      USART_ReceiveData(USART3);
    }
    p->busy = 0;
  }
  else if (param_1 == 3) {
    if (p->b < p->size) {
      *(char *)(((int)p->buf) + (uint)p->b) = (char)(USART_ReceiveData(UART4));
      p->b = p->b + 1;
      p->a = p->a + '\x01';
    }
    else {
      USART_ReceiveData(UART4);
    }
    p->busy = 0;
  }
  return;
}
#undef DAT_20000021
#undef DAT_20000022
#undef g_20000020_v
#undef g_20000044_v
#undef g_200000E1_v
#undef g_200000E2_v
#undef g_200000E4_v
#undef g_200000E6_v
#undef g_200000EA_v
#undef g_20000163_v
#undef DAT_20000f54
#undef DAT_20000f55
#undef DAT_20000f5b
#undef DAT_20001635
#undef DAT_20001636
#undef bRam20000f5e
#undef bRam20000f64
#undef bRam20000f67
#undef bRam20000f6d
#undef bRam20000f70
#undef bRam20000f76
#undef cRam20000f5d
#undef cRam20000f66
#undef cRam20000f6f
#undef g_20000F53_v
#undef iRam20000f60
#undef iRam20000f69
#undef iRam20000f72
#undef uRam20000f5c
#undef uRam20000f65
#undef uRam20000f6e
#undef DAT_20001638
#undef DAT_20001639
#undef DAT_2000163a
#undef DAT_2000163b
#undef DAT_2000163c
#undef DAT_2000163d
#undef DAT_2000163e
#undef g_20001637_v


typedef int T8014F36_iVar1;
extern uchar g_20000F53[];
typedef uchar T8014F36_DAT_20000f54;
#define DAT_20000f54 (*(T8014F36_DAT_20000f54 *)(p_g_20000F53 + 1))
typedef uchar T8014F36_DAT_20000f55;
#define DAT_20000f55 (*(T8014F36_DAT_20000f55 *)(p_g_20000F53 + 2))
typedef uchar T8014F36_DAT_20000f56;
#define DAT_20000f56 (*(T8014F36_DAT_20000f56 *)(p_g_20000F53 + 3))
typedef uchar T8014F36_DAT_20000f57;
#define DAT_20000f57 (*(T8014F36_DAT_20000f57 *)(p_g_20000F53 + 4))
typedef uchar T8014F36_DAT_20000f5b;
#define DAT_20000f5b (*(T8014F36_DAT_20000f5b *)(p_g_20000F53 + 8))
typedef uchar T8014F36_g_20000F53_v;
#define g_20000F53_v (*(T8014F36_g_20000F53_v *)(p_g_20000F53 + 0))

uint sub_08014F36(int param_1, undefined1 * param_2)
{
  uchar *p_g_20000F53 = g_20000F53;
  uchar *pcVar2;

  pcVar2 = &g_20000F53_v + (param_1 * 9);
  if (*pcVar2 == '\x01') {
    return 1;
  }
  if ((uint)(byte)(&DAT_20000f56)[(param_1 * 9)] < (uint)(byte)(&DAT_20000f5b)[(param_1 * 9)]) {
    *param_2 = *(undefined1 *)(*(int *)(&DAT_20000f57 + (param_1 * 9)) + (uint)(byte)(&DAT_20000f56)[(param_1 * 9)])
    ;
    (&DAT_20000f56)[(param_1 * 9)] = (&DAT_20000f56)[(param_1 * 9)] + '\x01';
    OS_ENTER_CRITICAL();
    if ((&DAT_20000f54)[(param_1 * 9)] != '\0') {
      (&DAT_20000f54)[(param_1 * 9)] = (&DAT_20000f54)[(param_1 * 9)] + -1;
    }
    if ((&DAT_20000f54)[(param_1 * 9)] == '\0') {
      *pcVar2 = '\x01';
      (&DAT_20000f55)[(param_1 * 9)] = 0;
      (&DAT_20000f56)[(param_1 * 9)] = 0;
    }
    OS_EXIT_CRITICAL();
  }
  else {
    OS_ENTER_CRITICAL();
    *pcVar2 = '\x01';
    (&DAT_20000f55)[(param_1 * 9)] = 0;
    (&DAT_20000f56)[(param_1 * 9)] = 0;
    (&DAT_20000f54)[(param_1 * 9)] = 0;
    OS_EXIT_CRITICAL();
  }
  return 0;
}
#undef DAT_20000f54
#undef DAT_20000f55
#undef DAT_20000f56
#undef DAT_20000f57
#undef DAT_20000f5b
#undef g_20000F53_v


typedef int T8014F94_iVar1;
extern uchar g_20000F77[];
typedef ushort T8014F94_DAT_20000f78;
#define DAT_20000f78 (*(T8014F94_DAT_20000f78 *)(g_20000F77 + 1))
typedef ushort T8014F94_DAT_20000f7a;
#define DAT_20000f7a (*(T8014F94_DAT_20000f7a *)(g_20000F77 + 3))
typedef uint T8014F94_DAT_20000f7c;
#define DAT_20000f7c (*(T8014F94_DAT_20000f7c *)(g_20000F77 + 5))
typedef uchar T8014F94_g_20000F77_v;
#define g_20000F77_v (*(T8014F94_g_20000F77_v *)(g_20000F77 + 0))

void sub_08014F94(int param_1, uint param_2, ushort param_3)
{
  SerialTx *p=&serial_tx[param_1];
  while(p->busy==1){}
  p->busy=1;p->len=param_3;p->idx=0;p->buf=(uchar*)param_2;
  if(param_1==0) sub_08014AFA();
  else if(param_1==1) sub_08014B2E();
  else if(param_1==2) sub_08014B5C();
  else if(param_1==3) sub_08014B92();
}

#undef DAT_20000f78
#undef DAT_20000f7a
#undef DAT_20000f7c
#undef g_20000F77_v


typedef int T8014FD6_v0;
typedef int T8014FD6_v2;

int sub_08014FD6(int param_1, int param_2)
{
  T8014FD6_v0 v0;
  T8014FD6_v2 v2;
  v2 = param_1 - 0x30;
  if ((uint)v2 <= 9) {
    v0 = (uchar)v2;
  }
  else {
    if ((uint)(param_1 - 0x41) <= 5) {
      v0 = (uchar)(param_1 - 0x37);
    }
    else {
      if ((uint)(param_1 - 0x61) <= 5) {
        v0 = (uchar)(param_1 - 0x57);
      }
      else {
        v0 = 0;
      }
    }
  }
  v0 = ((uint)v0 << 0x1c) >> 0x18;
  if ((uint)(param_2 - 0x30) <= 9) {
    return (uchar)(v0 + param_2 - 0x30);
  }
  if ((uint)(param_2 - 0x41) <= 5) {
    return (uchar)(v0 + param_2 - 0x37);
  }
  if ((uint)(param_2 - 0x61) > 5) return v0;
  return (uchar)(v0 + param_2 - 0x57);
}


typedef int T8015034_v0;
typedef uint T8015034_v2;

void sub_08015034(int param_1, uchar *param_2)
{
  T8015034_v0 v0;
  T8015034_v2 v2;
  v2 = (uint)param_1 >> 4;
  if (v2 < 0xa) {
    *(uchar *)param_2 = v2 + 0x30;
  }
  else {
    *(uchar *)param_2 = v2 + 0x37;
  }
  v0 = param_1 & 0xf;
  if ((uint)v0 < 0xa) {
    *(uchar *)(param_2 + 1) = v0 + 0x30;
    return;
  }
  *(uchar *)(param_2 + 1) = v0 + 0x37;
}


typedef int T8015058_v0;
typedef int T8015058_v4;

int sub_08015058(int param_1)
{
  T8015058_v0 v0;
  T8015058_v4 v4;
  v4 = sub_08014FD6(*(uchar *)(param_1 + 9), *(uchar *)(param_1 + 0xa));
  v0 = sub_08014FD6(*(uchar *)(param_1 + 0xb), *(uchar *)(param_1 + 0xc));
  if ((~(v0 + (v4 + ((uint)v0 >> 4))) + 1 & 0xf) == (uint)v4 >> 4) {
    return 1;
  }
  return 0;
}


typedef int T801508A_v0;
typedef ushort T801508A_v1;
typedef int T801508A_v3;
typedef ushort T801508A_v4;
typedef int T801508A_v5;

static __inline ushort serial_ascii_word(int p,uint mask){uint hi=(sub_08014FD6(*(uchar*)(p+1),*(uchar*)(p+2))<<8)&mask;uint lo=sub_08014FD6(*(uchar*)(p+3),*(uchar*)(p+4));return hi+lo;}
static __inline ushort serial_negate_sum(ushort sum){return (ushort)(~sum+1);}
int sub_0801508A(int param_1)
{
  uint length,high;ushort received;
  ushort sum=0,checksum,i;
  uint mask;
  uint pointer_diagnostic=0;

  high=sub_08014FD6(*(uchar*)(param_1+9),*(uchar*)(param_1+10));
  mask=0xffff;
  high=(high<<8)&mask;
  length=((high+sub_08014FD6(*(uchar*)(param_1+11),*(uchar*)(param_1+12)))&0xfff)+12;
  for(i=1;i<=length;i++)sum+=*(uchar*)(param_1+i);
  checksum=serial_negate_sum(sum);
  /* The existing reads require at least 17 frame bytes; sum&1 is in bounds.
     This unread equality preserves the accumulator's ARMCC lifetime. */
  pointer_diagnostic=(uint)((((uchar*)param_1)+(sum&1))!=g_200000E1);
  (void)pointer_diagnostic;
  received=serial_ascii_word(param_1+length,mask);
  if(checksum==received)return 1;
  return 0;
}


typedef ushort T80150EE_v4;
typedef uchar * T80150EE_v5;
extern uchar g_20000109[];
typedef uchar T80150EE_DAT_20000109;
#define DAT_20000109 (*(T80150EE_DAT_20000109 *)(g_20000109 + 0))

void sub_080150EE(int param_1, int param_2)
{
  T80150EE_v4 v4;
  v4 = param_2 & 0xfff | (uchar)(~(param_2 + ((uint)param_2 >> 4) + ((uint)param_2 >> 8) & 0xf) + 1) << 0xc;
  sub_08015034((uint)v4 >> 8, &DAT_20000109);
  *(uchar *)(param_1 + 9) = DAT_20000109;
  *(uchar *)(param_1 + 0xa) = g_20000109[1];
  sub_08015034((uchar)v4, &DAT_20000109);
  *(uchar *)(param_1 + 0xb) = DAT_20000109;
  *(uchar *)(param_1 + 0xc) = g_20000109[1];
}
#undef DAT_20000109


extern uchar g_20000109[];
typedef uchar T801512E_DAT_20000109;
#define DAT_20000109 (*(T801512E_DAT_20000109 *)(g_20000109 + 0))
typedef uchar T801512E_DAT_2000010a;
#define DAT_2000010a (*(T801512E_DAT_2000010a *)(g_20000109 + 1))

void sub_0801512E(int param_1, int param_2)
{

  sub_08015034(param_2, &DAT_20000109);
  *(uchar *)(param_1 + 7) = DAT_20000109;
  *(uchar *)(param_1 + 8) = DAT_2000010a;
}
#undef DAT_20000109
#undef DAT_2000010a

