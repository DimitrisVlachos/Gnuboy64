#ifndef _umem_
#define _umem_
/*Some mem copy hacks ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/

void umemcpy(void* d,const void* s,int cnt);
void umemset(void* d,const unsigned char v,int cnt);
#endif

