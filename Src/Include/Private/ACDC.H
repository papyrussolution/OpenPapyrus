#ifndef __ACDC_H
#define __ACDC_H

#ifdef __cplusplus
extern "C" {
#endif

char far pascal InitChannel (int Channel);
char far pascal SendToPrn (int Channel, char Len, char far* S);
char far pascal SendToDisplay (int Channel, char Len, char far* S);
char far pascal AddLongToMemory (int Channel, int Adress, long Value);
char far pascal AddIntToMemory (int Channel, int Adress, int Value);
char far pascal ReceivFromMemory (int Channel, int Adress, char Len, char far* S);

#ifdef __cplusplus
}
#endif

#endif /* __ACDC_H */
