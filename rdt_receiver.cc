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

#include "rdt_struct.h"
#include "rdt_receiver.h"

FILE *rlog;
FILE *rstruc;
int header_size=6;
int maxpayload_size=RDT_PKTSIZE-header_size;
/* receiver initialization, called once at the very beginning */
struct mypacket{
		int payload_size;
		unsigned char msgnum;
		int seqnum;
		int tot;
		char odd;
		char even;
		char data[RDT_PKTSIZE-6];
};
#define window 10
struct message * bucket[window];
int mark[window][20];
int lmr=-1;//last message received
int lam=lmr+window;//last acceptable message
void Receiver_Init()
{
		fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
		rlog=fopen("rlog.txt","w");
		rstruc=fopen("rstruc.txt","w");
		for(int i=0;i<window;i++){
				bucket[i]=(struct message *)malloc(sizeof(struct message));
				bucket[i]->size=0;
		}
		for(int i=0;i<window;i++){
				for(int j=0;j<20;j++){
						mark[i][j]=0;
				}
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
}
bool checksum(char odd,char even,char *pkt,int size)
{
		int checkodd=0,checkeven=0;
		for(int i=1;i<size;i+=2){
				checkodd+=pkt[i];
		}
		for(int i=0;i<size;i+=2){
				checkeven+=pkt[i];
		}
		return ((checkodd && 0xFF)==odd) && ((checkeven && 0xFF)==even);
}

void Receiver_MergePkt(struct mypacket *mpkt)
{
		int bucketnum=(mpkt->msgnum-lmr-1+256)%256;
		int seqnum=mpkt->seqnum;
		fprintf(rstruc,"lmr%d bucketnum%d seqnum%d msgnum%d max%d tot%d \n",lmr,bucketnum,seqnum,mpkt->msgnum,mark[bucketnum][0],mpkt->tot);
		fflush(rstruc);
		if(bucketnum>=window){
				return;
		}
		if(mark[bucketnum][seqnum]){
				return;
		}
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
		mpkt->msgnum=pkt->data[1];
		mpkt->seqnum=pkt->data[2];
		mpkt->tot=pkt->data[3];
		mpkt->odd=pkt->data[4];
		mpkt->even=pkt->data[5];
		memcpy(mpkt->data,pkt->data+header_size,maxpayload_size);
}
/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
		struct mypacket *mpkt=(struct mypacket*)malloc(sizeof(struct mypacket));
		pktFormat(pkt,mpkt);

		bool check=checksum(mpkt->odd,mpkt->even,mpkt->data,maxpayload_size);
		if(check==false){
				fprintf(rlog,"corrupt\n");
				return;
		}
		fprintf(rlog,"msgnum%d seqnum%d\n",mpkt->msgnum,mpkt->seqnum);
		for(int i=0;i<mpkt->payload_size;i++){
			fprintf(rlog,"%c",mpkt->data[i]);
		}
		fprintf(rlog,"\n");
		Receiver_MergePkt(mpkt);
		if(mpkt!=NULL) free(mpkt);
		/* int header_size = 6;

		   struct message *msg = (struct message*) malloc(sizeof(struct message));
		   ASSERT(msg!=NULL);

		   msg->size = pkt->data[0];

		   if (msg->size<0) msg->size=0;
		   if (msg->size>RDT_PKTSIZE-header_size) msg->size=RDT_PKTSIZE-header_size;
		   char odd = pkt->data[4];
		   char even = pkt->data[5];
		   char msgnum = pkt->data[1];
		   char seqnum = pkt->data[2];
		   char tot = pkt->data[3];
		   bool check=checksum(odd,even,pkt->data+header_size,RDT_PKTSIZE-header_size);
		   fprintf(rlog,"msg%d packet%d received %s tot%d\n",msgnum,seqnum,check==true?"good":"corrupt",tot);
		   if(check==false){
		   return;
		   }

		   msg->data = (char*) malloc(msg->size);
		   ASSERT(msg->data!=NULL);
		   memcpy(msg->data, pkt->data+header_size, msg->size);
		   Receiver_ToUpperLayer(msg);

		   if (msg->data!=NULL) free(msg->data);
		   if (msg!=NULL) free(msg);*/
}
