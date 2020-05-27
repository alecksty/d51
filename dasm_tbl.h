
//	Ĭ�ϵ����ݸ�ʽ
#define FMT_NUL				_T("0x%02x")
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

//------------------------------------------------
//	��ʽ�б�
//------------------------------------------------
_TCHAR *arrFMT[]=
{
	FMT_NUL,
	FMT_BIT,
	FMT_DAT,
	FMT_PTR,
	FMT_RN,
	FMT_RI,
	FMT_LAB,
	FMT_LABCR,
};

//------------------------------------------------
//	��ʽ���
//------------------------------------------------
enum emFMT
{
	eFMT_NUL	= 0,
	eFMT_BIT	= 1,
	eFMT_DAT	= 2,
	eFMT_PTR	= 3,
	eFMT_RN		= 4,
	eFMT_RI		= 5,
	eFMT_LAB	= 6,
	eFMT_LABCR	= 7,
};
