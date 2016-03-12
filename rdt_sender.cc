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

FILE *slog;
/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    slog=fopen("slog.txt","w");
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
    fclose(slog);
}

char checksum_odd(char *pkt,int num)
{
	int checksum=0;
	for(int i=1;i<num;i+=2){
		checksum+=pkt[i];
	}
	return checksum && 0xFF;
}

char checksum_even(char *pkt,int num)
{
	int checksum=0;
	for(int i=0;i<num;i+=2){
		checksum+=pkt[i];
	}
	return checksum && 0xFF;
}
/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    
    int header_size = 6;
    static int msgnum=0;
    int maxpayload_size = RDT_PKTSIZE - header_size;


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
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}
