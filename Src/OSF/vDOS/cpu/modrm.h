
Bit8u  *lookupRMregb[] = {&reg_al, &reg_cl, &reg_dl, &reg_bl, &reg_ah, &reg_ch, &reg_dh, &reg_bh};

Bit16u *lookupRMregw[] = {&reg_ax, &reg_cx, &reg_dx, &reg_bx, &reg_sp, &reg_bp, &reg_si, &reg_di};

Bit32u *lookupRMregd[] = {&reg_eax, &reg_ecx, &reg_edx, &reg_ebx, &reg_esp, &reg_ebp, &reg_esi, &reg_edi};


#define GetRM	Bit8u rm = Fetchb();

#define Getrb	Bit8u  *rmrb = lookupRMregb[(rm>>3)&7];			
#define Getrw	Bit16u *rmrw = lookupRMregw[(rm>>3)&7];			
#define Getrd	Bit32u *rmrd = lookupRMregd[(rm>>3)&7];			

#define GetEArb	Bit8u  *earb = lookupRMregb[rm&7];
#define GetEArw	Bit16u *earw = lookupRMregw[rm&7];
#define GetEArd	Bit32u *eard = lookupRMregd[rm&7];


#define GetRMrb												\
	GetRM;													\
	Getrb;													

#define GetRMrw												\
	GetRM;													\
	Getrw;													

#define GetRMrd												\
	GetRM;													\
	Getrd;													



