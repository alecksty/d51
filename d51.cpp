//=====================================================================================
//
//	ģ��:	����� MCS-51ϵ�д���
//
//	�汾:	1.00
//
//	����:	2007-07-10
//
//	����:	ʩ̽��
//
//	˵��:	d51 -n rom.bin src.asm
//			d51 -Annn:nnn rom.hex src.asm
//			d51 -Annn,nnn rom.hex src.asm
//=====================================================================================
#include "stdafx.h"

//	�����Ÿ���
#define MAX_LABELS			(1024)

//	�����ַ����
#define MAX_ADDRESS			(0x10000)

//	������ֽ�(�ֽڶ���)
#define CODE_MAX_BYTES		4

//  ��ͨ���볤��(�ֽڶ���)
#define CODE_BYTES			1

//	����λ��(ʵ��ָ��λ�� 8 - 24)
#define CODE_BITS			8

//	Ĭ�ϵ����ݸ�ʽ
#define FMT_BIT				_T("0x%02x")
#define FMT_DAT				_T("#%03xH")
#define FMT_PTR				_T("0x%02x")
#define FMT_RN				_T("R%d")
#define FMT_RI				_T("@R%d")
#define FMT_LAB				_T("lab_%04x")
#define FMT_LABCR			FMT_LAB _T(":\n")

//	������β����
#define FMT_RET				_T("\n;--------------------------------------------------------------------------------------") \
							_T("\n; Function:") \
							_T("\n;--------------------------------------------------------------------------------------")

//-----------------------------------------------------------------------------------
//	������,����ָ�Ҫ�ĳ�,�����ĳɱ����Ծͷ���;
//-----------------------------------------------------------------------------------
typedef unsigned long		UINT64;
typedef unsigned int		UINT32;
typedef unsigned short		UINT16;
typedef unsigned char		UINT8;
typedef unsigned char		BYTE;

//-----------------------------------------------------------------------------------
//	�����ת����
//-----------------------------------------------------------------------------------
typedef struct tagDASM
{
	//	����ֵ
	UINT64	CodeData;
	//	����λ(�Ƚ�ƥ��)
	UINT64	CodeMask;
	//	�Ĵ���(ר�üĴ���)
	UINT64	RegsMask;
	//	����λ(�ڲ���ַ)
	UINT64	BitsMask;
	//	����λ(������)
	UINT64	DataMask;
	//	ָ��λ(����ָ��)
	UINT64	PtrsMask;
	//	���λ(�����ַ)
	UINT64	LabsMask;
	//	�ֽ���(���ֽ�ָ��)
	UINT8	nBytes;
	//	�����ʽ
	_TCHAR *pCodeFmt;
}DASM_TBL;

//-----------------------------------------------------------------------------------
//	�Ĵ������Ʊ�
//-----------------------------------------------------------------------------------
typedef struct tagREGS
{
	UINT8 uAddr;
	_TCHAR *pName;

}REGS_TBL;

//-----------------------------------------------------------------------------------
//	����ת��������˳��:��ʽΪ printf("CODE %S,%S,%S",REG,BIT,DAT)
//-----------------------------------------------------------------------------------
//	MCS-51 ָ��
//	��256ָ��.
//	����ָ������Ҫת��Ϊ64λ��.
//
// Lab11 ->code_addr(11λ��ַ)
// Lab   ->code_addr(16λ��ַ)
// Deret ->code_addr(08λ��ַ)
// Ptr   ->data_addr(16λָ��)
// Bit   ->data_addr(8λָ��)
//
//-----------------------------------------------------------------------------------
DASM_TBL tblCode[]=
{
	//	������		��Чλ		�Ĵ���		����λ		����λ		ָ��λ		���λ		�ֽ�	��ʽ
	//------------------------------------------------------------------------------------------------------------------------------------------------
	//  CODE-ID		CODE-MASK	REGS-MASK	BITS-MASK	DATS-MASK	PTRS-MASK	LABS-MASK	BYTES,	FORMAT
	//------------------------------------------------------------------------------------------------------------------------------------------------
	{	0x000000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("NOP")},				// NOP
	{	0x010000,	0x1f0000,	0x000000,	0x000000,	0x000000,	0x000000,	0xe0ff00,	0x2,	_T("AJMP    %s")},		// AJMP Lab11
	{	0x020000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ffff,	0x3,	_T("LJMP    %s")},		// LJMP Lab16
	{	0x030000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RR      A")},		// RR	A
	{	0x040000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     A")},		// INC	A
	{	0x050000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("INC     %s")},		// INC	Ptr
	{	0x060000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     %s")},		// INC	@Ri
	{	0x080000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     %s")},		// INC	Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x100000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x0000ff,	0x3,	_T("JBC     %s,%s")},	// JBC	Bit,Lab8
	{	0x110000,	0x1f0000,	0x000000,	0x000000,	0x000000,	0x000000,	0xe0ff00,	0x2,	_T("ACALL   %s")},		// ACALL Lab11
	{	0x120000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ffff,	0x3,	_T("LCALL   %s")},		// LCALL Lab
	{	0x130000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RRC     A")},		// RRC	A
	{	0x140000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DEC     A")},		// DEC	A
	{	0x150000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("DEC     %s")},		// DEC	Ptr
	{	0x160000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DEC     %s")},		// DEC	@Ri
	{	0x180000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DEC     %s")},		// DEC	Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x200000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x0000ff,	0x3,	_T("JB      %s,%s")},	// JB	Bit,Lab8
	{	0x220000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RET") FMT_RET},		// RET
	{	0x230000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RL      A")},		// RL	A
	{	0x240000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ADD     A,%s")},	// ADD	A,#data
	{	0x250000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ADD     A,%s")},	// ADD	A,Ptr
	{	0x260000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADD     A,%s")},	// ADD	A,@Ri
	{	0x280000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADD     A,%s")},	// ADD	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x300000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x0000ff,	0x3,	_T("JNB     %s,%s")},	// JNB	bit,Lab8
	{	0x320000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RETI") FMT_RET},	// RETI
	{	0x330000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("RLC     A")},		// RLC	A
	{	0x340000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ADC     A,%s")},	// ADC	A,#data
	{	0x350000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ADC     A,%s")},	// ADC	A,Ptr
	{	0x360000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADC     A,%s")},	// ADC	A,@Ri
	{	0x380000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ADC     A,%s")},	// ADC	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x400000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JC      %s")},		// JC	Lab
	{	0x420000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ORL     %s,A")},	// ORL	Ptr,A
	{	0x430000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("ORL     %s,%s")},	// ORL	Ptr,#data
	{	0x440000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ORL     A,%s")},	// ORL	A,#data
	{	0x450000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ORL     A,%s")},	// ORL	A,Ptr
	{	0x460000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ORL     A,%s")},	// ORL	A,@Ri
	{	0x480000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ORL     A,%s")},	// ORL	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x500000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JNC     %s")},		// JNC	Lab
	{	0x520000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ANL     %s,A")},	// ANL	Ptr,A
	{	0x530000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("ANL     %s,%s")},	// ANL	Ptr,#data
	{	0x540000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("ANL     A,%s")},	// ANL	A,#data
	{	0x550000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("ANL     A,%s")},	// ANL	A,Ptr
	{	0x560000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ANL     A,%s")},	// ANL	A,@Ri
	{	0x580000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("ANL     A,%s")},	// ANL	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x600000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JZ      %s")},		// JZ	Lab
	{	0x620000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("XRL     %s,A")},	// XRL	Ptr,A
	{	0x630000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("XRL     %s,%s")},	// XRL	Ptr,#data
	{	0x640000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("XRL     A,%s")},	// XRL	A,#data
	{	0x650000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("XRL     A,%s")},	// XRL	A,Ptr
	{	0x660000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XRL     A,%s")},	// XRL	A,@Ri
	{	0x680000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XRL     A,%s")},	// XRL	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x700000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("JNZ     %s")},		// JNZ	Lab
	{	0x720000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("ORL     C,%s")},	// ORL	C,Bit
	{	0x730000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("JMP     @A + DPTR")},// JMP	@A+DPTR
	{	0x740000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("MOV     A,%s")},	// MOV	A,#data
	{	0x750000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("MOV     %s,%s")},	// MOV	Ptr,#data
	{	0x760000,	0xfe0000,	0x010000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("MOV     %s,A")},	// MOV	@Ri,#data
	{	0x780000,	0xf80000,	0x070000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Rn,#data
//----------------------------------------------------------------------------------------------------------------------------------	
	{	0x800000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("SJMP    %s")},		// SJMP Lab
	{	0x820000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("ANL     C,%s")},	// ANL	C,bit
	{	0x830000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVC    A,@A + PC")},// MOVC A,@A+PC
	{	0x840000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DIV     AB")},		// DIV	AB
	{	0x850000,	0xff0000,	0x000000,	0x000000,	0x0000ff,	0x00ff00,	0x000000,	0x3,	_T("MOV     %s,%s")},	// MOV	Ptr,Ptr // ����ָ��
	{	0x860000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Ptr,@Ri //
	{	0x880000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Ptr,Rn  // 
//----------------------------------------------------------------------------------------------------------------------------------
	{	0x900000,	0xff0000,	0x000000,	0x000000,	0x00ffff,	0x000000,	0x000000,	0x3,	_T("MOV     DPTR,%s")},	// MOV	DPTR,#data
	{	0x920000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("MOV     %s,C")},	// MOV	Bit,C
	{	0x930000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVC    A,@A + DPTR")},//MOVC	A,@A+DPTR
	{	0x940000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x2,	_T("SUBB    A,%s")},	// SUBB A,#data
	{	0x950000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("SUBB    A,%s")},	// SUBB A,Ptr
	{	0x960000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SUBB    A,%s")},	// SUBB A,@Ri
	{	0x980000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SUBB    A,%s")},	// SUBB A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xa00000,	0xff0000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x000000,	0x2,	_T("ORL     C,%s")},	// ORL	C,bit
	{	0xa20000,	0xff0000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x000000,	0x2,	_T("MOV     C,%s")},	// MOV	C,bit
	{	0xa30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("INC     DPTR")},	// INC	DPTR
	{	0xa40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MUL     AB")},		// INC	DPTR
	//  0xa50000 -- ����ָ��
	{	0xa60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	@Ri,Ptr	// ע���86����
	{	0xa80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,%s")},	// MOV	Rn,Ptr	// ע���88����
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xb00000,	0xff0000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x000000,	0x2,	_T("ANL     C,%s")},	// MOV	C,bit
	{	0xb20000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("CPL     %s")},		// CPL	bit
	{	0xb30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CPL     C")},		// CPL	C
	{	0xb40000,	0xff0000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x0000ff,	0x3,	_T("CJNE    A,%s,%s")}, // CJNE A,#data,Lab
	{	0xb50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x0000ff,	0x3,	_T("CJNE    A,%s,%s")},	// CJNE A,Ptr,Lab
	{	0xb60000,	0xfe0000,	0x010000,	0x000000,	0x00ff00,	0x000000,	0x0000ff,	0x3,	_T("CJNE    %s,%s,%s")},// CJNE @Ri,#data,Lab
	{	0xb80000,	0xf80000,	0x070000,	0x000000,	0x00ff00,	0x000000,	0x0000ff,	0x3,	_T("CJNE    %s,%s,%s")},// CJNE Rn,#data,Lab
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xc00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("PUSH    %s")},		// PUSH	Ptr
	{	0xc20000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("CLR     %s")},		// CLR	Ptr
	{	0xc30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CLR     C")},		// CLR	C
	{	0xc40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SWAP    A")},		// SWAP	A
	{	0xc50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("XCH     A,%s")},	// XCH	A,Ptr
	{	0xc60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XCH     A,%s")},	// XCH	A,@Ri
	{	0xc80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XCH     A,%s")},	// XCH	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xd00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("POP     %s")},		// POP	Ptr
	{	0xd20000,	0xff0000,	0x000000,	0x00ff00,	0x000000,	0x000000,	0x000000,	0x2,	_T("SETB    %s")},		// SETB	Bi
	{	0xd30000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("SETB    C")},		// SETB	C
	{	0xd40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("DA      A")},		// DA	A
	{	0xd50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x0000ff,	0x3,	_T("DJNZ    %s,%s")},	// DJNZ	Ptr,Lab
	{	0xd60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("XCHD    A,%s")},	// XCHD	A,@Ri
	{	0xd80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x2,	_T("DJNZ    %s,%s")},	// DJNZ	Rn,Lab
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xe00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    A,@DPTR")},	// MOVX	A,@DPTR
	{	0xe20000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    A,%s")},	// MOVX	A,@Ri
	{	0xe40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CLR     A")},		// CLR	A
	{	0xe50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     A,%s")},	// MOV	A,Ptr
	{	0xe60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     A,%s")},	// MOV	A,@Ri
	{	0xe80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     A,%s")},	// MOV	A,Rn
//----------------------------------------------------------------------------------------------------------------------------------
	{	0xf00000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    @DPTR,A")},	// MOV	A,Rn
	{	0xf20000,	0xff0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOVX    %s,A")},	// MOV	@Ri,A
	{	0xf40000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("CPL     A")},		// CPL	A
	{	0xf50000,	0xff0000,	0x000000,	0x000000,	0x000000,	0x00ff00,	0x000000,	0x2,	_T("MOV     %s,A")},	// MOV	Ptr,A
	{	0xf60000,	0xfe0000,	0x010000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     %s,A")},	// MOV	@Ri,A
	{	0xf80000,	0xf80000,	0x070000,	0x000000,	0x000000,	0x000000,	0x000000,	0x1,	_T("MOV     %s,A")},	// MOV	Rn,A
};

//-----------------------------------------------------------------------------------
//	�Ĵ�������,�͵�ַ
//-----------------------------------------------------------------------------------
REGS_TBL arrRegs[]=
{
	{0xE0,_T("ACC")},
	{0xF0,_T("B")},
	{0xD0,_T("PSW")},
	{0xF0,_T("SP")},
	{0x82,_T("DPL")},
	{0x83,_T("DPH")},
	{0x80,_T("P0")},
	{0x90,_T("P1")},
	{0xA0,_T("P2")},
	{0xB0,_T("P3")},
	{0xB8,_T("IP")},
	{0xA8,_T("IE")},
	{0x89,_T("TMOD")},
	{0x88,_T("TCON")},
	{0xA8,_T("T2CON")},
	{0x8C,_T("TH0")},
	{0x8A,_T("TL0")},
	{0x8D,_T("TH1")},
	{0x8B,_T("TL1")},
	{0xCD,_T("TH2")},
	{0xCC,_T("TL2")},
	{0xCB,_T("RCAP2H")},
	{0xCA,_T("RCAP2L")},
	{0x98,_T("SCON")},
	{0x99,_T("SBUF")},
	{0x87,_T("TR1")},
	{0x8e,_T("PCON")},
};

//-----------------------------------------------------------------------------------
//	Ĭ���ļ�����
//-----------------------------------------------------------------------------------
_TCHAR *pStart = _T("");
_TCHAR *pBinFile = _T("rom.bin");
_TCHAR *pSrcFile = _T("src.asm");
_TCHAR buff[]=_T("0x0000");

//	�ļ�ָ��
FILE *fpBin;
FILE *fpSrc;

//	Ĭ����ʼ��ַ:
int iCodeStart = 0;

//	�������
int idArgv = 1;

//	ָ�����
BYTE pCodeBuff[CODE_MAX_BYTES];

//	�洢��źͺ�����ַ
UINT64 useLabels[MAX_LABELS];
UINT64 posLabels = 0;

//	��ʾ����
void ShowHelp();

//	�Ĵ�����
_TCHAR *GetRegname(UINT64 nReg);

int GetCodeSize(UINT64 lCode);

//	�ǼǱ��
int RegistLabel(UINT64 lCode,UINT64 lLine);

//	�������
int ParseCode(UINT64 lLine,UINT64 lCode,FILE *fpOut);

//	�����ѵǼǱ��
int FindLabel(UINT64 lLine);

//	�ļ�����
int ParseFile(int iStart, FILE *pIn,FILE *pOut);

//=====================================================================================
//	��ʼ����
//=====================================================================================
int main(int argc, _TCHAR* argv[])
{
	//	�жϲ���
	if(argc < 2 || argc > 4)
	{
		ShowHelp();
#ifdef _DEBUG
		getchar();
#endif
		return -1;
	}

	//	Ĭ��ָ���һ������
	idArgv = 1;
	pStart = argv[idArgv];

	//	�Ƿ��е�ַ����?
	if(pStart[0] == '-')
	{
		iCodeStart = atoi(&pStart[1]);
		idArgv ++;
	}

	if(argc > idArgv )
	{
		pBinFile = argv[idArgv ];
	}

	if(argc > idArgv + 1)
	{
		pSrcFile = argv[idArgv +1];
	}
	
	fpBin = fopen(pBinFile,"rb");
	if(!fpBin)
	{
		printf("Can't open bin file: %s!\n",pBinFile);
#ifdef _DEBUG
		getchar();
#endif
		return -1;
	}

	fpSrc = fopen(pSrcFile,"w+a");
	if(!fpSrc)
	{
		fclose(fpBin);
		printf("Can open src file: %s!\n",pSrcFile);
#ifdef _DEBUG
		getchar();
#endif
		return -1;
	}

	//	��ʾ̧ͷ
	printf(_T(" MCS-51 Disasm tools(C) Version 1.00\n"));
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" Code start:  %x.\n"),iCodeStart);
	printf(_T(" Input file:  %s.\n"),pBinFile);
	printf(_T(" Output file: %s.\n"),pSrcFile);
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" QQ:190376601,TEL:13751152175              --Aleck.Shi\n"));

	//	�����ļ�
	ParseFile(iCodeStart,fpBin,fpSrc);

	fclose(fpBin);
	fclose(fpSrc);

	//	�������!
	printf(_T(" Processed done!\n"));

#ifdef _DEBUG
	getchar();
#endif
	return 0;
}

//=====================================================================================
//	��ʾ����
//=====================================================================================
void ShowHelp()
{
	printf(_T(" MCS-51 Disasm tools(C) Version:1.00\n"));
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" usage:d51 [-code address] <bin file> [src file]\n"));
	printf(_T("   exp:d51 rom.bin\n"));
	printf(_T("       d51 -2 rom.bin\n"));
	printf(_T("       d51 -10 rom.bin src.asm\n"));
	printf(_T("---------------------------------------------------------------\n"));
	printf(_T(" QQ:190376601,TEL:13751152175                     ----Aleck.Shi\n"));
}

//=====================================================================================
//	�ļ�����
//=====================================================================================
int ParseFile(int iStart, FILE *fpIn,FILE *fpOut)
{
	UINT64 lCodePos = 0;
	UINT64 lCode = 0;
	int iSize = 0;
	int i = 0;

	fprintf(fpOut,_T(";//=====================================================================================\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	ģ��:	<ģ������>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	�汾:	<�汾��>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	����:	<����>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	����:	<����>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//	˵��:	<ģ��˵��>\n"));
	fprintf(fpOut,_T(";//\n"));
	fprintf(fpOut,_T(";//=====================================================================================\n"));
	fprintf(fpOut,_T("\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T(";// ��������:\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T("\n"));

	for(i = 0; i < (sizeof(arrRegs) / sizeof(arrRegs[0])); i++)
	{
		fprintf(fpOut,_T(";%s\t\tEQU\t0x%02x\n"),arrRegs[i].pName,arrRegs[i].uAddr);
	}

	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T(";// ��������:\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T("\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T(";// ���뿪ʼ:\n"));
	fprintf(fpOut,_T(";//-------------------------------------------------------------------------------------\n"));
	fprintf(fpOut,_T("\t\tORG     0x00\n"));

	//	������ʼ��ַ
	fseek(fpIn,iStart,SEEK_SET);

	//----------------------------------------------------
	//	�ǼǱ��!
	//----------------------------------------------------
	posLabels = 0;
	lCodePos = 0;
	while(!feof(fpIn))
	{
		fread(pCodeBuff,1,CODE_BYTES,fpIn);
		lCode = pCodeBuff[0] << 16;
		iSize = GetCodeSize(lCode);
		if(iSize > 1)
		{
			for(i = 0;i < CODE_MAX_BYTES; i ++ )
				pCodeBuff[i] = 0;

			fread(pCodeBuff,(iSize - 1),CODE_BYTES,fpIn);
			lCode |= pCodeBuff[0] << 8;
			lCode |= pCodeBuff[1] << 0;
		}

		//lCodePos += iSize;
		//	�ǼǱ��
		RegistLabel(lCode,lCodePos);
		lCodePos += iSize;

		//	��ַ���޼��
		if(lCodePos >= MAX_ADDRESS)
			break;
	}

	//----------------------------------------------------
	//	������ʼ��ַ
	//----------------------------------------------------
	fseek(fpIn,iStart,SEEK_SET);
	lCodePos = 0;
	while(!feof(fpIn))
	{
		//	��ȡ����
		fread(pCodeBuff,1,CODE_BYTES,fpIn);
		lCode = pCodeBuff[0] << 16;
		iSize = GetCodeSize(lCode);

		if(iSize > 1)
		{
			for(i = 0;i < CODE_MAX_BYTES; i++)
				pCodeBuff[i] = 0;

			fread(pCodeBuff,(iSize - 1),CODE_BYTES,fpIn);
			lCode |= pCodeBuff[0] << 8;
			lCode |= pCodeBuff[1] << 0;
		}
	
		//lCodePos += iSize;

		//	��������
		ParseCode(lCodePos,lCode,fpOut);
		lCodePos += iSize;

		//	��ַ���޼��
		if(lCodePos >= MAX_ADDRESS)
			break;
	}

	fprintf(fpOut,_T("\t\tEND\n"));
	fprintf(fpOut,_T(";//=====================================================================================\n"));
	fprintf(fpOut,_T(";// �ļ�����. <END OF FILE> \n"));
	fprintf(fpOut,_T(";//=====================================================================================\n"));
	return 0;
}

//=====================================================================================
//	��ѯָ���ֽ���
//=====================================================================================
int GetCodeSize(UINT64 lCode)
{
	UINT64 i = 0;	
	UINT64 lCount =  sizeof(tblCode) / sizeof(tblCode[0]);
	UINT64 lLine = 0;
	
	//	��ʼ��
	for( i = 0; i < lCount; i++ )
	{
		//	ƥ��ָ�,�����ֽ���
		if(((tblCode[i].CodeMask & lCode) == tblCode[i].CodeData))
		{
			return tblCode[i].nBytes;
		}
	}
	printf("Unknow Code's Size!\n");
	return 1;
}

//=====================================================================================
//	�����к�
//=====================================================================================
UINT64 GetLabel(UINT64 lCode,UINT64 lLine,UINT64 lMask)
{
	UINT64 lCurLine = 0;

	//	��ͬ�ı��,����ͬ,��������Ե�ַ,�����õǼ�!
	switch(lMask)
	{
	//	���Գ���ַ
	case 0x00ffff:
		lCurLine = lCode & lMask;
		break;

	//	ҳ�ھ��Ե�ַ
	case 0xe0ff00:
		lCurLine  = (lCode & 0xe00000) >> 21;
		lCurLine |= (lCode & 0x00ff00) >> 8;
		lCurLine |= (lLine & 0x00f800);
		break;

	//	��Զ̵�ַ
	case 0x00ff00:
		lCurLine = (lCode & lMask) >> 8;
		lCurLine = lLine + (char)lCurLine + GetCodeSize(lCode);
		break;

	//	��Զ̵�ַ
	case 0x0000ff:
		lCurLine = lCode & lMask;
		lCurLine = lLine + (char)lCurLine + GetCodeSize(lCode);
		break;

	//	������ʽ����
	default:
		lCurLine = 0;
		lCurLine += lLine;
		printf("Label Format Error!\n");
		break;
	}
	return lCurLine;
}

//=====================================================================================
//	�к�,��ŷ���(Ҫ���ֶ���ת!�ͳ���ת!)
//=====================================================================================
int RegistLabel(UINT64 lCode,UINT64 lLine)
{
	//
	// 16λΪȫ��ַ,��5λΪҳ��ַ
	// lLine->0000,0111,1111,1111
	//
	//	lab16 = abs_address
	//  lab11 = lLine & 0xf800 | lcode.line
	//	lab08 = lLine + lcode.line
	//
	UINT64 i = 0;	
	UINT64 lCount =  (sizeof(tblCode) / sizeof(tblCode[0]));
	UINT64 lCurLine = 0;

	//	��ʼ��
	for( i = 0; i < lCount; i++ )
	{
		//	ƥ��ָ�
		if(((tblCode[i].CodeMask & lCode) == tblCode[i].CodeData))
		{
			//	���б��
			if(tblCode[i].LabsMask)
			{
				//	ȡ���Ե�ַ
				lCurLine = GetLabel(lCode,lLine,tblCode[i].LabsMask);

				//	�鿴�Ƿ�Ǽ�!����Ǽ�
				if(!FindLabel(lCurLine))
				{
					useLabels[posLabels] = lCurLine;
#ifdef _DEBUG
					printf("Labels[%d] = %04x\n",posLabels,lCurLine);
#endif
					posLabels++;
					return 1;
				}
			}
		}
	}
	return 0;
}

//=====================================================================================
//	�Ƿ�Ǽ��к�
//=====================================================================================
int FindLabel(UINT64 lLine)
{
	UINT64 i = 0;
	for( i = 0; i < posLabels; i++ )
	{
		//	�ǼǱ��
		if(useLabels[i] == lLine)
			return 1;
	}
	return 0;
}

//=====================================================================================
//	ȡ�üĴ�������!
//=====================================================================================
_TCHAR *GetRegName(UINT64 nReg)
{
	int i = 0;
	for(i = 0; i < (sizeof(arrRegs) / sizeof(arrRegs[0])); i++)
	{
		if(arrRegs[i].uAddr == nReg)
		{
			return arrRegs[i].pName;
		}
	}
	sprintf(buff,FMT_PTR,nReg);
	return buff;
}

//=====================================================================================
//	�������
//=====================================================================================
int ParseCode(UINT64 lLine,UINT64 lCode,FILE *fpOut)
{
	UINT64 i = 0;	
	UINT64 lCount =  sizeof(tblCode) / sizeof(DASM_TBL);
	UINT64 id = 0;

	_TCHAR buff1[64];
	_TCHAR buff2[64];
	_TCHAR buff3[64];

	UINT64 R,B,D,P,L;
	R = B = D = P = L = 0;

#ifdef _DEBUG
	//printf("Code:%06x\n",lCode);
#endif

	for( i = 0; i < lCount; i++ )
	{
		id = tblCode[i].CodeMask & lCode;

		//	ƥ��ָ�
		if( id == tblCode[i].CodeData)
		{
			//	����Ҫ�Ĳű��,��Ҫ���
			if(FindLabel(lLine))
			{
				//	��Ÿ�ʽ
				fprintf(fpOut,FMT_LABCR,lLine);
			}
			//	��ʼ�յ�λ��!
			fprintf(fpOut,_T("\t\t"));
			
			R = tblCode[i].RegsMask;
			B = tblCode[i].BitsMask;
			D = tblCode[i].DataMask;
			P = tblCode[i].PtrsMask;
			L = tblCode[i].LabsMask;

			//-------------------------------------------------------------------------
			// ����:CJNE Rn,#data,Lab
			//-------------------------------------------------------------------------
			if( R && !B && D && !P && L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				sprintf(buff2,FMT_DAT, (tblCode[i].DataMask & lCode) >> 8);
				sprintf(buff3,FMT_LAB, GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2,buff3);
			}
			//-------------------------------------------------------------------------
			// ����:MOV Rn,#data
			//-------------------------------------------------------------------------
			else if( R && !B && D && !P && !L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				sprintf(buff2,FMT_DAT,(tblCode[i].DataMask & lCode) >> 8);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			// ����:MOV Rn,Ptr(86-A6,88-A8�Ƿ���)
			//-------------------------------------------------------------------------
			else if( R && !B && !D &&  P && !L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(R & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(R & lCode) >> 16);
				strcpy(buff2,GetRegName((tblCode[i].PtrsMask & lCode) >> 8));
				
				if(id == 0x00a60000 || id == 0x00a80000)
				{
					fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff2,buff1);
				}
				else
				{
					fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
				}
			}
			//-------------------------------------------------------------------------
			// ����:MOV Rn,Lab
			//-------------------------------------------------------------------------
			else if( R && !B && !D && !P && L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			// ����:INC Rn
			//-------------------------------------------------------------------------
			else if( R && !B && !D && !P && !L)
			{
				if(tblCode[i].RegsMask == 0x010000)
					sprintf(buff1,FMT_RI,(tblCode[i].RegsMask & lCode) >> 16);
				else
					sprintf(buff1,FMT_RN,(tblCode[i].RegsMask & lCode) >> 16);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	���� JB Bit,Lab
			//-------------------------------------------------------------------------
			else if(!R &&  B && !D && !P && L)
			{
				strcpy(buff1,GetRegName((B & lCode) >> 8));
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	���� CJNE A,#data,Lab (3BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && D && !P && L)
			{
				sprintf(buff1,FMT_DAT,(lCode & tblCode[i].DataMask) >> 8);
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	���� CJNE A,Ptr,Lab (3BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && P && L)
			{
				strcpy(buff1,GetRegName((P & lCode) >> 8));
				sprintf(buff2,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	���� MOV Ptr,#data (3BYTE) //ע��!��85,������ָ��,��Ϊ�ô���Ϊ˫ָ��!����������λ����!
			//-------------------------------------------------------------------------
			else if(!R && !B && D && P && !L)
			{
				strcpy(buff1,GetRegName((P & lCode) >> 8));
				
				//	����ָ��
				if(id == 0x00850000)
					strcpy(buff2,GetRegName((D & lCode)));
				else
					sprintf(buff2,FMT_DAT,(D & lCode));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1,buff2);
			}
			//-------------------------------------------------------------------------
			//	���� STEB Bi ( 2 BYTE )
			//-------------------------------------------------------------------------
			else if(!R && B && !D && !P && !L)
			{
				strcpy(buff1,GetRegName((B & lCode) >> 8));
				//sprintf(buff1,FMT_BIT,(lCode & tblCode[i].BitsMask) >> 8);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	���� AJMP Lab (2~3BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && !P && L)
			{
				sprintf(buff1,FMT_LAB,GetLabel(lCode,lLine,tblCode[i].LabsMask));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	���� MOV  A,#data (2BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && D && !P && !L)
			{
				sprintf(buff1,FMT_DAT,(lCode & tblCode[i].DataMask) >> 8);
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	���� PUSH [Ptr] (2BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && P && !L)
			{
				strcpy(buff1,GetRegName((tblCode[i].PtrsMask & lCode) >> 8));
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt,buff1);
			}
			//-------------------------------------------------------------------------
			//	���� WDTC,CLRA ( 1 BYTE)
			//-------------------------------------------------------------------------
			else if(!R && !B && !D && !P && !L)
			{
				fprintf(fpOut,(const char*)tblCode[i].pCodeFmt);
			}
			//	�����ƥ��!
			else
			{
				fprintf(fpOut,_T(";Error Code:%04x\n"), lCode);
				printf(_T("Error Code:%04x\n"),lCode);
			}
			//	����
			fprintf(fpOut,_T("\n"));
			break;
		}
	}
	
	//	�����ָ��
	if(i >= lCount)
	{
		fprintf(fpOut,_T(";Unknow Code:%04x\n"),lCode);
		printf(_T("Unknow Code:%04x\n"),lCode);
	}

	return 0;
}
