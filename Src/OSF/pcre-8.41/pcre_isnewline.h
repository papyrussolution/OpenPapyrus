// PCRE_ISNEWLINE.H
// @sobolev 
//
// This macro checks for a newline at the given position 
//
#define IS_NEWLINE(p) ((NLBLOCK->nltype != NLTYPE_FIXED) ? ((p) < NLBLOCK->PSEND && PRIV(is_newline) ((p), NLBLOCK->nltype, NLBLOCK->PSEND, \
	&(NLBLOCK->nllen), utf)) : ((p) <= NLBLOCK->PSEND - NLBLOCK->nllen && UCHAR21TEST(p) == NLBLOCK->nl[0] && (NLBLOCK->nllen == 1 || UCHAR21TEST(p+1) == NLBLOCK->nl[1])))
//
// This macro checks for a newline immediately preceding the given position 
//
#define WAS_NEWLINE(p) ((NLBLOCK->nltype != NLTYPE_FIXED) ? ((p) > NLBLOCK->PSSTART && PRIV(was_newline) ((p), NLBLOCK->nltype, NLBLOCK->PSSTART, &(NLBLOCK->nllen), utf)) \
	: ((p) >= NLBLOCK->PSSTART + NLBLOCK->nllen && UCHAR21TEST(p - NLBLOCK->nllen) == NLBLOCK->nl[0] && (NLBLOCK->nllen == 1 || \
	UCHAR21TEST(p - NLBLOCK->nllen + 1) == NLBLOCK->nl[1])))