#if !defined (CRPE2_H)
#define CRPE2_H

#if defined (__cplusplus)
extern "C"
{
#endif

// Set 1-byte structure alignment
#if defined (__BORLANDC__)               // Borland C/C++
  #pragma option -a-
#elif defined (_MSC_VER)                 // Microsoft Visual C++
  #if _MSC_VER >= 900                    // MSVC 2.x and later
    #pragma pack (push)
  #endif
  #pragma pack (1)
#endif 

#define DEFAULT_COLUMN_WIDTH    10

#define UXFWordWinType          0
#define UXFRichTextFormatType   0
#define UXFCommaSeparatedType   0
#define UXFTabSeparatedType     1
#define UXFCharSeparatedType    2
#define UXFTextType             0
#define UXFXls7ExtType          6
#define UXFPdfType              0

#define UXDDiskType             0
#define UXDApplicationType      0
#define UXDMailType             2

typedef struct UXFCommaTabSeparatedOptions
{
    WORD structSize;

    BOOL useReportNumberFormat;
    BOOL useReportDateFormat;
} UXFCommaTabSeparatedOptions;

#define UXFCommaTabSeparatedOptionsSize (sizeof (UXFCommaTabSeparatedOptions))

typedef struct UXFCharSeparatedOptions {
    WORD structSize;

    BOOL useReportNumberFormat;
    BOOL useReportDateFormat;
    char stringDelimiter;
    char FAR *fieldDelimiter;
} UXFCharSeparatedOptions;

#define UXFCharSeparatedOptionsSize (sizeof (UXFCharSeparatedOptions))

typedef struct UXFXlsOptions { 
	WORD structSize;
	BOOL bColumnHeadings;   //TRUE -- has column headings, which come from
	                        //       "Page Header" and "Report Header" ares.
	                        //FALSE -- no clolumn headings. 
	                        //The default value is FALSE.
	BOOL bUseConstColWidth; //TRUE -- use constant column width
	                        //FALSE -- set column width based on an area
	                        //The default value is FALSE.
	double fConstColWidth;	//Column width, when bUseConstColWidth is TRUE
	                        //The default value is 9.
	BOOL bTabularFormat;	//TRUE -- tabular format (flatten an area into a row)
	                        //FALSE -- non-tabular format
	                        //The default value is FALSE.
	WORD baseAreaType;      //One of the 7 Section types defined in "Crpe.h".
	                        //The default value is PE_SECT_DETAIL.
	WORD baseAreaGroupNum;  //If baseAreaType is either GroupHeader or
	                        //GroupFooter and there are more than one groups, 
	                        //we need to give the group number. 
	                        //The default value is 1. 
	
 	
#if defined (__cplusplus)
public:
	UXFXlsOptions()
	{
		structSize = sizeof(UXFXlsOptions);
		bColumnHeadings=FALSE;
 		bUseConstColWidth = FALSE;
		fConstColWidth = DEFAULT_COLUMN_WIDTH;
		bTabularFormat = FALSE;
		baseAreaType = 4; // PE_SECT_DETAIL
		baseAreaGroupNum = 1;
	};
#endif
}
    UXFXlsOptions;

#define UXFXlsOptionsSize (sizeof (UXFXlsOptions))


typedef struct UXFPdfOptions { 
	WORD structSize;
	long    FirstPage;
	long    LastPage;
	boolean UsePageRange;
#if defined (__cplusplus)
public:
	UXFPdfOptions()
	{
		structSize = sizeof(UXFPdfOptions);
		FirstPage = LastPage = 0;
		UsePageRange = false;
	};
#endif
} UXFPdfOptions;

#define UXFPdfOptionsSize (sizeof (UXFPdfOptions))

typedef struct UXDApplicationOptions
{
    WORD structSize;

    char FAR *fileName;
}
    UXDApplicationOptions;

#define UXDApplicationOptionsSize      (sizeof (UXDApplicationOptions))

typedef struct UXDDiskOptions
{
    WORD structSize;

    char FAR *fileName;
}
    UXDDiskOptions;
#define UXDDiskOptionsSize      (sizeof (UXDDiskOptions))

// Reset structure alignment
#if defined (__BORLANDC__)
  #pragma option -a.
#elif defined (_MSC_VER)
  #if _MSC_VER >= 900
    #pragma pack (pop)
  #else
    #pragma pack ()
  #endif
#endif    

#if defined (__cplusplus)
}
#endif

#endif // CRPE2_H
