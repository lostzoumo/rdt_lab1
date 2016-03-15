/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "rdt_struct.h"
#include "checksum.h"
#include "rdt_receiver.h"
using namespace std;
FILE *rlog;
FILE *rstruc;
FILE *rcor;
#define header_size 14
int maxpayload_size=RDT_PKTSIZE-header_size;
/* receiver initialization, called once at the very beginning */
struct mypacket{
		int payload_size;
		unsigned char msgnum;
		int seqnum;
		int globalcnt;
		int tot;
		char data[RDT_PKTSIZE-header_size];
};
#define window 100
struct message * bucket[window];
int mark[window][20];
int lmr=-1;//last message received
int lpr=0;//last packet received
int lam=lmr+window;//last acceptable message
int recbuf[window];
void Receiver_Init()
{
		fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
		rlog=fopen("rlog.txt","w");
		rstruc=fopen("rstruc.txt","w");
		rcor=fopen("rcor.txt","w");
		for(int i=0;i<window;i++){
				bucket[i]=(struct message *)malloc(sizeof(struct message));
				bucket[i]->size=0;
		}
		for(int i=0;i<window;i++){
				for(int j=0;j<20;j++){
						mark[i][j]=0;
				}
		}
		for(int i=0;i<window;i++){
				recbuf[i]=-1;
		}
}
/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
		fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
		fclose(rlog);
		fclose(rstruc);
		fclose(rcor);
}

void Receiver_MergePkt(struct mypacket *mpkt)
{
		int bucketnum=(mpkt->msgnum-lmr-1+256)%256;
		int seqnum=mpkt->seqnum;
		fflush(rstruc);
		if(bucketnum>=window){
				return;
		}
		if(mark[bucketnum][seqnum]){
				return;
		}
		fprintf(rstruc,"lmr%d bucketnum%d seqnum%d msgnum%d max%d tot%d \n",lmr,bucketnum,seqnum,mpkt->msgnum,mark[bucketnum][0],mpkt->tot);
		struct message * pmsg=bucket[bucketnum];
		//mark[?][0] =>first not 1

		if(pmsg->size > mpkt->payload_size+(seqnum-1)*maxpayload_size){
				memcpy(pmsg->data+(seqnum-1)*maxpayload_size,mpkt->data,maxpayload_size);
		}
		else{
				pmsg->size=mpkt->payload_size+(seqnum-1)*maxpayload_size;
				if(mark[bucketnum][0]==0)    
						pmsg->data=(char *)malloc(pmsg->size);
				else	
						pmsg->data=(char *)realloc(pmsg->data,pmsg->size);
				memcpy(pmsg->data+(seqnum-1)*maxpayload_size,mpkt->data,mpkt->payload_size);
		}
		mark[bucketnum][seqnum]=1;
		for(int i=1;i<20;i++){
				if(mark[bucketnum][i]==0)	{mark[bucketnum][0]=i-1;break;}
		}
		fprintf(rstruc,"mark[0]%d tot%d\n",mark[bucketnum][0],mpkt->tot);
		if( mark[bucketnum][0]==mpkt->tot ){
				mark[bucketnum][0]=-1;
				if(bucketnum!=0)	return;
				while(mark[0][0]==-1){
						Receiver_ToUpperLayer(bucket[0]);
						if((bucket[0])->data!=NULL) free((bucket[0])->data);
						struct message *tmp=bucket[0];
						tmp->size=0;
						for(int i=0;i<window-1;i++){
								bucket[i]=bucket[i+1];
						}
						bucket[window-1]=tmp;
						lmr=(lmr+1)%256;
						for(int i=0;i<window-1;i++){
								for(int j=0;j<20;j++){
										mark[i][j]=mark[i+1][j];
								}
						}
						for(int i=0;i<20;i++)
								mark[window-1][i]=0;
				}
				return;
		}
}
void pktFormat(struct packet *pkt,struct mypacket *mpkt)
{
		mpkt->payload_size=pkt->data[0];
		mpkt->msgnum=pkt->data[1] & 0xFF;
		mpkt->seqnum=pkt->data[2] & 0xFF;
		mpkt->globalcnt=(pkt->data[6] & 0xFF)+
						((pkt->data[7] &0xFF)<<8)+
						((pkt->data[8] &0xFF)<<16)+
						((pkt->data[9] &0xFF)<<24);
		mpkt->tot=pkt->data[3];
		memcpy(mpkt->data,pkt->data+header_size,maxpayload_size);
		fprintf(rlog,"pktfor size%d msgnum%d seqnum%d tot%d glcnt%d\n",mpkt->payload_size,mpkt->msgnum,mpkt->seqnum,mpkt->tot,mpkt->globalcnt);
		fflush(rlog);
}
unsigned int ack(int *vec,int num)
{
		int ret=lpr;
		int i=0;
		while(i<window){
				for(int j=0;j<window;j++){
						if(recbuf[j]==lpr+i+1)	{ret=lpr+i+1;break;}
				}
				if(ret!=lpr+i+1)	break;
				i++;
		}
		return ret;
}

bool contain(int *vec,int num)
{
		for(int i=0;i<window;i++){
				if(vec[i]==num) return true;
		}
		return false;
}
/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
		struct mypacket *mpkt=(struct mypacket*)malloc(sizeof(struct mypacket));
		bool check1=check(pkt);
		if(check1==false){
				fprintf(rlog,"corrupt\n");
				return;
		}
		pktFormat(pkt,mpkt);
		fprintf(rlog,"msgnum%d seqnum%d\n",mpkt->msgnum,mpkt->seqnum);
		Receiver_MergePkt(mpkt);

		//next handle returning msg
		//first fill the glcnt in the recbuf
		int glcnt=mpkt->globalcnt;
		int pos=glcnt-lpr-1;
		if(mpkt!=NULL) free(mpkt);
		if(!contain(recbuf,glcnt)&&glcnt>lpr){
				if(glcnt>window+lpr)	fprintf(rcor,"allert!!!!\n");
				else	{fprintf(rcor,"not contain%d\n",glcnt);recbuf[pos]=glcnt;}
		}	
		fflush(rcor);
		//construct the ack pkt
		packet *anspkt=(packet *)malloc(sizeof(packet));

		unsigned int acknum=ack(recbuf,lpr);
		anspkt->data[0] = acknum&0xFF;
		anspkt->data[1] = (acknum>>8)&0xFF;
		anspkt->data[2] = (acknum>>16)&0xFF;
		anspkt->data[3] = (acknum>>24)&0xFF;
		checksum(anspkt);
		Receiver_ToLowerLayer(anspkt);
		fprintf(rcor,"lpr%d receive%d ack%d\n",lpr,glcnt,acknum);
		fprintf(rcor,"clear %dbuf ",acknum-lpr);
		fflush(rcor);
		if(acknum!=lpr){
				int clear=acknum-lpr;
				for(int i=0;i<clear;i++){
						fprintf(rcor," %d",recbuf[i]);
				}
				for(int i=0;i<window-clear;i++){
						recbuf[i]=recbuf[i+clear];
				}
				for(int i=0;i<clear;i++){
						recbuf[window-clear+i]=-1;
				}
		}
		fprintf(rcor,"\n");
		lpr=acknum;
		fflush(rcor);
}
