#include "checksum.h"
unsigned int checksum_8(unsigned int cksum, char *data,unsigned int size){
	char num=0;
	unsigned char *p=(unsigned char *)data;
	if ((NULL == data) || (0==size))
		return cksum;
	while (size>1) {
		cksum+=(((unsigned short) p[num])<<8 & 0xff00) | ((unsigned short)p[num+1]) & 0xff; 
		size -=2;
		num +=2;
	}
	if (size>0)
	{
		cksum +=(((unsigned short)p[num]) <<8) & 0xffff;
		num+=1;
	}

	while (cksum >> 16)
		cksum=(cksum & 0xffff) +(cksum>>16);
	return ~cksum;
}
char checksum_odd(char *pkt)
{
	int checksum=0;
	for(int i=1;i<RDT_PKTSIZE;i+=2){
		checksum+=pkt[i];
	}
	return checksum & 0xFF;
}
char checksum_even(char *pkt)
{
	int checksum=0;
	for(int i=0;i<RDT_PKTSIZE;i+=2){
		checksum+=pkt[i];
	}
	return checksum&0xFF;
}
void checksum(struct packet *pkt){
	pkt->data[4]=0x11;
	pkt->data[5]=0x03;
	pkt->data[10]=0x0;
	pkt->data[11]=0x0;
	unsigned short check1= checksum_8(0,pkt->data,RDT_PKTSIZE);
	pkt->data[10]=check1>>8;
	pkt->data[11]=check1 & 0xFF;
	char odd=checksum_odd(pkt->data);
	char even=checksum_even(pkt->data);
	pkt->data[4]=odd;
	pkt->data[5]=even;
}
bool check(struct packet *pkt){
	char odd=pkt->data[4];
	char even=pkt->data[5];
	pkt->data[4]=0x11;
	pkt->data[5]=0x03;
	unsigned short check1=checksum_8(0,pkt->data,RDT_PKTSIZE);
	if(check1){
		return false;
	}else{
		return ((checksum_odd(pkt->data)&0xFF)==(odd&0xFF)) && ((checksum_even(pkt->data)&0xFF)==(even&0xFF)); 
	}
}
