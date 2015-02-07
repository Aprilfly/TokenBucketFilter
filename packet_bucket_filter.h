#include<pthread.h>

#include "my402list.h"



extern long b,num,p;
extern FILE *fp;
extern double lambda, mu, r;
extern My402List Q1,Q2;

extern struct timeval t1,prevPacketTime,t2;

extern long token_count,pt_died,tt_died,total_token_count,packet_counter,dropped_pack_count,dropped_tok_count;
extern int timestamp,sigin_signal;

extern double total_iat,total_st,totalTimeQ1,totalTimeQ2,totalTimeS,totalSystemTime,sqTotalST;

typedef struct token
{

int ID;
int token_ts;
}Token;

typedef struct packet
{
long packet_ts;
long no_of_tokens;
double iat;
double st;
struct timeval arrival_time , enterQ1_time , leaveQ1_time , enterQ2_time , leaveQ2_time , serviceStartTime,leave_system_time;
}Packet;

extern void* Server(void* arg);
extern void* serveToken(void* arg);
extern void* servePacket(void* arg);
extern double timeDifference(struct timeval start_time, struct timeval end_time);
extern double timeAddition(struct timeval start_time, struct timeval end_time);



