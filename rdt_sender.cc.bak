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
#define window 1000 
#define timerwindow 1000
#define timeout 0.3
int sheader_size=10;
unsigned int globalcnt=1;
int lar=-1;
int slpr=0;
int smaxpayload_size=RDT_PKTSIZE - sheader_size;
FILE *slog;
FILE *scor;
FILE *timelog;
struct mypacket{
		packet *pkt;
		int globalcnt;
};
struct mypacket* sendbuf[window];
struct timer{
		int glcnt;
		int pktnum;
		double time;
};
struct timer* timerlist[timerwindow+1];
int timersize=0;
/* sender initialization, called once at the very beginning */
void Sender_Init()
{
		fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
		slog=fopen("slog.txt","w");
		scor=fopen("scor.txt","w");
		timelog=fopen("timelog.txt","w");
		for(int i=0;i<window;i++){
				sendbuf[i]=(struct mypacket *)malloc(sizeof(struct mypacket));
				sendbuf[i]->globalcnt=-1;
		}
		for(int i=0;i<timerwindow+1;i++){
				timerlist[i]=(struct timer*)malloc(sizeof(struct timer));
				timerlist[i]->glcnt=0;
				timerlist[i]->pktnum=0;
				timerlist[i]->time=0;
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
		fclose(timelog);
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
				pkt->data[1] = msgnum & 0xFF;
				pkt->data[2] = seqnum & 0xFF;
				pkt->data[6] = globalcnt&0xFF;
				pkt->data[7] = (globalcnt>>8)&0xFF;
				pkt->data[8] = (globalcnt>>16)&0xFF;
				pkt->data[9] = (globalcnt>>24)&0xFF;
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
				int bucketnum=globalcnt - slpr-1;
				if(bucketnum<window){
				sendbuf[bucketnum]->globalcnt=globalcnt;
				sendbuf[bucketnum]->pkt=pkt;
				fprintf(scor,"sending glcnt %d\n",globalcnt);
				globalcnt++;
				}
				else	fprintf(scor,"sending allert!!\n");
		}
		if (msg->size > cursor) {
				packet *pkt=(packet *)malloc(sizeof(packet));
				pkt->data[0] = msg->size-cursor;
				pkt->data[1] = msgnum & 0xFF;
				pkt->data[2] = seqnum & 0xFF;
				pkt->data[6] = globalcnt&0xFF;
				pkt->data[7] = (globalcnt>>8)&0xFF;
				pkt->data[8] = (globalcnt>>16)&0xFF;
				pkt->data[9] = (globalcnt>>24)&0xFF;
				pkt->data[3] = tot;
				memcpy(pkt->data+sheader_size, msg->data+cursor, pkt->data[0]);
				pkt->data[4] = schecksum_odd(pkt->data+sheader_size,smaxpayload_size);
				pkt->data[5] = schecksum_even(pkt->data+sheader_size,smaxpayload_size);
				Sender_ToLowerLayer(pkt);
				fprintf(slog,"check %x %x\n",pkt->data[4],pkt->data[5]);
				int bucketnum=globalcnt - slpr-1;
				if(bucketnum<window){
				sendbuf[bucketnum]->globalcnt=globalcnt;
				sendbuf[bucketnum]->pkt=pkt;
				fprintf(scor,"sending glcnt %d\n",globalcnt);
				globalcnt++;
				}
				else	fprintf(scor,"sending allert!!\n");

		}
		fprintf(slog,"msg%d sent in %d packet\n",msgnum,seqnum);
		for(int i=0;i<msg->size;i++){
				fprintf(slog,"%c",msg->data[i]);
		}
		fprintf(slog,"\n");
		fflush(slog);
		fflush(scor);
		msgnum++;
#if 1
//timer
		fprintf(timelog,"sending glcnt%d pktnum%d curtime%f cursize%d\n",globalcnt-tot,tot,GetSimulationTime(),timersize);
		if(timersize>=timerwindow)	fprintf(timelog,"sending timersize too big\n");
		timerlist[timersize]->glcnt=globalcnt-tot;
		timerlist[timersize]->pktnum=tot;
		timerlist[timersize]->time=GetSimulationTime()+timeout;
		if(!Sender_isTimerSet())	Sender_StartTimer(timeout);
		timersize++;
		fflush(timelog);
#endif
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
		return ((checkodd & 0xFF)==(odd & 0xFF)) && ((checkeven & 0xFF)==(even & 0xFF));
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
//ack packet format :4bytes ack,2byte checksum
		int acknum=(pkt->data[0]&0xFF)+
					((pkt->data[1]&0xFF)<<8)+
					((pkt->data[2]&0xFF)<<16)+
					((pkt->data[3]&0xFF)<<24);
		bool check=schecksum(pkt->data[4],pkt->data[5],pkt->data+6,RDT_PKTSIZE-6);
		if(check == false ){
				fprintf(scor,"receiving corrupt\n");
				return;
		}
		fprintf(scor,"receiving ack%d lpr%d\n",acknum,slpr);
		if(slpr>=acknum)	return;
		int clear=acknum-slpr;
		struct mypacket* tmp[window];
		for(int i=0;i<clear;i++){
				tmp[i]=sendbuf[i];
		}
		for(int i=0;i<window-clear;i++){
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
		int start = timerlist[0]->glcnt;
		int end = start+timerlist[0]->pktnum-1;
		double now=GetSimulationTime();
		fprintf(timelog,"timeout begin slpr%d glcnt%d pktnum%d curtime%f cursize%d\n",slpr,start,end-start+1,now,timersize);
		fflush(timelog);
		for(int i=start;i<=end;i++){
				if(i>slpr){	
						fprintf(timelog,"bug start\n");
						Sender_ToLowerLayer(sendbuf[i-slpr-1]->pkt);
						fprintf(timelog,"%s\n",sendbuf[i-slpr-1]->pkt==NULL?"is null":"not null");
						fflush(timelog);
						timerlist[timersize]->glcnt=i;
						timerlist[timersize]->pktnum=1;
						timerlist[timersize]->time=now+timeout;
						timersize++;
						fprintf(timelog,"timeout new glcnt%d cursize%d\n",i,timersize);
						fflush(timelog);
				}
		}
		//handled
		struct timer * tmp= timerlist[0];
		for(int i=0;i<timersize-1;i++)	timerlist[i]=timerlist[i+1];
		timerlist[timersize-1]=tmp;
		tmp->glcnt=0;
		tmp->pktnum=0;
		tmp->time=0;
		timersize--;
		fprintf(timelog,"timeout after cursize%d next%f\n",timersize,timerlist[0]->time);
		fflush(timelog);
		if(timersize>0) Sender_StartTimer(timerlist[0]->time-now);
}
