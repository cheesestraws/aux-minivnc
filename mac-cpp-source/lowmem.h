#ifndef MY_LOWMEM_H
#define MY_LOWMEM_H

void LMSetMBTicks(unsigned long val);
void LMSetMouseTemp(Point pt);
void LMSetRawMouseLocation(Point pt);
void LMSetMouseLocation(Point pt);
void LMSetCursorNew(Boolean val);
void LMSetMouseButtonState(unsigned char val);
Boolean LMGetCrsrCouple();

#endif