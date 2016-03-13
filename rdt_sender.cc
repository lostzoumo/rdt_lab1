/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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
#include "rdt_sender.h"
#define window 10
int sheader_size=7;
unsigned int globalcnt=1;
int lar=-1;
int slpr=0;
int smaxpayload_size=RDT_PKTSIZE - sheader_size;
FILE *slog;
FILE *scor;
struct mypacket{
		packet *pkt;
		int globalcnt;
};
struct mypacket* sendbuf[window];
/* sender initialization, called once at the very beginning */
void Sender_Init()
{
		fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
		slog=fopen("slog.txt","w");
		scor=fopen("scor.txt","w");
		for(int i=0;i<window;i++){
				sendbuf[i]=(struct mypacket *)malloc(sizeof(struct mypacket));
				sendbuf[i]->globalcnt=0;
		}
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
		fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
		fclose(slog);
		fclose(scor);
}

char schecksum_odd(char *pkt,int num)
{
		int checksum=0;
		for(int i=1;i<num;i+=2){
				checksum+=pkt[i];
		}
		return checksum & 0xFF;
}

char schecksum_even(char *pkt,int num)
{
		int checksum=0;
		for(int i=0;i<num;i+=2){
				checksum+=pkt[i];
		}
		return checksum & 0xFF;
}


/* event handler, called when a message is passed from the upper layer at the 
   zo   sender */
void Sender_FromUpperLayer(struct message *msg)
{
		static int msgnum=0;
		int cursor = 0;
		int seqnum=1;
		int tot=(msg->size+smaxpayload_size-1)/smaxpayload_size;
		while (msg->size-cursor > smaxpayload_size) {
				packet *pkt=(packet *)malloc(sizeof(packet));
				pkt->data[0] = smaxpayload_size;
				pkt->data[1] = msgnum%256;
				pkt->data[2] = seqnum%256;
				pkt->data[6] = globalcnt%256;
				pkt->data[3] = tot;
				memcpy(pkt->data+sheader_size, msg->data+cursor, smaxpayload_size);
				pkt->data[4] = schecksum_odd(pkt->data+sheader_size,smaxpayload_size);
				pkt->data[5] = schecksum_even(pkt->data+sheader_size,smaxpayload_size);
				Sender_ToLowerLayer(pkt);
				cursor += smaxpayload_size;
				seqnum++;
				fprintf(slog,"check %x %x\n",pkt->data[4],pkt->data[5]);
				//just fill the send buffer, hope window size buffer would be enough
				//sendbuf[i]'s space is allocated in init() and we keep using them
/*				int bucketnum=globalcnt - lar-1;
				if(bucketnum<window){
				sendbuf[bucketnum]->globalcnt=globalcnt;
				sendbuf[bucketnum]->pkt=pkt;
				fprintf(scor,"sending glcnt %d\n",globalcnt%256);
				globalcnt++;
				}
				else	fprintf(scor,"sending allert!!\n");*/
		}
		if (msg->size > cursor) {
				packet *pkt=(packet *)malloc(sizeof(packet));
				pkt->data[0] = msg->size-cursor;
				pkt->data[1] = msgnum%256;
				pkt->data[2] = seqnum%256;
				pkt->data[6] =globalcnt%256;
				pkt->data[3] = tot;
				memcpy(pkt->data+sheader_size, msg->data+cursor, pkt->data[0]);
				pkt->data[4] = schecksum_odd(pkt->data+sheader_size,smaxpayload_size);
				pkt->data[5] = schecksum_even(pkt->data+sheader_size,smaxpayload_size);
				Sender_ToLowerLayer(pkt);
				fprintf(slog,"check %x %x\n",pkt->data[4],pkt->data[5]);
/*				int bucketnum=globalcnt - lar-1;
				if(bucketnum<window){
				sendbuf[bucketnum]->globalcnt=globalcnt;
				sendbuf[bucketnum]->pkt=pkt;
				fprintf(scor,"sending glcnt %d\n",globalcnt%256);
				globalcnt++;
				}
				else	fprintf(scor,"sending allert!!\n");*/

		}
		fprintf(slog,"msg%d sent in %d packet\n",msgnum,seqnum);
		for(int i=0;i<msg->size;i++){
				fprintf(slog,"%c",msg->data[i]);
		}
		fprintf(slog,"\n");
		fflush(slog);
		fflush(scor);
		msgnum++;
#if 0    
		static int msgnum=0;


		packet pkt;

		int cursor = 0;
		int seqnum=1;
		int tot=(msg->size+maxpayload_size-1)/maxpayload_size;
		while (msg->size-cursor > maxpayload_size) {
				pkt.data[0] = maxpayload_size;
				pkt.data[1] = msgnum%256;
				pkt.data[2] = seqnum;
				pkt.data[3] = tot;
				memcpy(pkt.data+header_size, msg->data+cursor, maxpayload_size);
				pkt.data[4] = checksum_odd(pkt.data+header_size,maxpayload_size);
				pkt.data[5] = checksum_even(pkt.data+header_size,maxpayload_size);
				Sender_ToLowerLayer(&pkt);

				cursor += maxpayload_size;
				seqnum++;
		}

		if (msg->size > cursor) {
				pkt.data[0] = msg->size-cursor;
				pkt.data[1] = msgnum%256;
				pkt.data[2] = seqnum;
				pkt.data[3] = tot;
				memcpy(pkt.data+header_size, msg->data+cursor, pkt.data[0]);
				pkt.data[4] = checksum_odd(pkt.data+header_size,maxpayload_size);
				pkt.data[5] = checksum_even(pkt.data+header_size,maxpayload_size);
				Sender_ToLowerLayer(&pkt);
		}
		fprintf(slog,"msg%d sent in %d packet\n",msgnum,seqnum);
		for(int i=0;i<msg->size;i++){
				fprintf(slog,"%c",msg->data[i]);
		}
		fprintf(slog,"\n");
		msgnum++;
#endif
}
bool schecksum(char odd,char even,char *pkt,int size)
{
		int checkodd=0,checkeven=0;
		for(int i=1;i<size;i+=2){
				checkodd+=pkt[i];
		}
		for(int i=0;i<size;i+=2){
				checkeven+=pkt[i];
		}
		return ((checkodd & 0xFF)==odd) && ((checkeven & 0xFF)==even);
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
		int acknum=pkt->data[0];
		bool check=schecksum(pkt->data[1],pkt->data[2],pkt->data+3,RDT_PKTSIZE-3);
		if(check == false ){
				fprintf(scor,"receiving corrupt\n");
				return;
		}
		fprintf(scor,"receiving ack%d lpr%d\n",acknum,slpr);
		if(slpr==acknum)	return;
		int clear=acknum-slpr;
		struct mypacket* tmp[window];
		for(int i=0;i<window-clear;i++){
				tmp[i]=sendbuf[i];
				sendbuf[i]=sendbuf[i+clear];
		}
		fprintf(scor,"	clear %dpacket",clear);
		for(int i=0;i<clear;i++){
				sendbuf[window-clear+i]=tmp[i];
				fprintf(scor," %d",tmp[i]->globalcnt);
				tmp[i]->globalcnt=-1;
				if(tmp[i]->pkt!=NULL)	free(tmp[i]->pkt);
		}
		slpr=acknum;
		fprintf(scor,"\n");
		fflush(scor);
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}
