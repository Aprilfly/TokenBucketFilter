#include<pthread.h>
#include<stdio.h>
#include <stdlib.h>
#include<string.h>
#include <signal.h>
#include<time.h>
#include<unistd.h>
#include<sys/time.h>
#include<math.h>
#include "packet_bucket_filter.h"


pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty=PTHREAD_COND_INITIALIZER;
sigset_t set;


long b=10,num=20,p=3,sigint_signal=0;
double lambda=0.5, mu=0.35, r=1.5;

long token_count=0,total_token_count=0;

long pt_died=0,tt_died=0;
long packet_counter=0,dropped_pack_count=0,dropped_tok_count=0;

struct timeval t1={0,0},t2={0,0},prevPacketTime={0,0};
int timestamp=1;

FILE *fp=NULL;

pthread_t serve_token,serve_packet,server,signal_thread;
My402List Q1, Q2;

double total_iat=0.0,total_st=0.0,totalTimeQ1=0.0,totalTimeQ2=0.0,totalTimeS=0.0,totalSystemTime=0.0,sqTotalST=0.0;

double timeDifference(struct timeval start_time, struct timeval end_time)
{

double timer_spent = (end_time.tv_sec - start_time.tv_sec) + ((end_time.tv_usec - start_time.tv_usec) / 1000000.0);
return (timer_spent*1000);
}

double timeAddition(struct timeval start_time, struct timeval end_time)
{
    double addition=end_time.tv_sec + start_time.tv_sec + (end_time.tv_usec + start_time.tv_usec) / 1000000.0;
    return (addition*1000);
}


void* servePacket(void* arg)
{
        char buff[300]="",local_buffer[100]="";
        int count2=1,i=0,c=0;
        long value=0;
        double temp1=0.0;
        Packet *pack=NULL;


        int temp_value=0,n1=0,n2=0;


        My402ListElem *elem1=NULL;
        struct timeval packet_time={0,0};
        double time_diff1=0.0, time_diff2=0.0;

        /*----------------Setting the total number of packets----------- */
        if(fp!=NULL)
        {
            if(fgets(buff,1024,fp)!=NULL);
                num=atol(buff);
        }
        while(packet_counter<num)
        {

            pack=(Packet*)malloc(sizeof(Packet));
            if(fp!=NULL)
            {
               if((fgets(buff,1024,fp)!=NULL))
               {
                   count2=1;c=0;
                   strcpy(local_buffer,"");
                   for(i=0;i<strlen(buff);i++)
                   {
                       if ((buff[i]!='\t')&&(buff[i]!=' ')&&(buff[i]!='\n'))
                       {
                           local_buffer[c]=buff[i];
                           c++;
                        }
                       else
                       {
                           local_buffer[c]='\0';



                                if(c>0)
                                {
                                    switch(count2)
                                    {
                                         case 1: temp1=strtod(local_buffer,NULL);
                                                 pack->iat=temp1;
                                                 break;

                                         case 2: value=atoi(local_buffer);
                                                 pack->no_of_tokens=value;
                                                 break;

                                         case 3: temp1=strtod(local_buffer,NULL);
                                                 pack->st=temp1;
                                                 break;
                                     }
                                     count2++;
                                }

                            strcpy(local_buffer,"");
                            temp1=0;
                            value=0;
                            c=0;
                       }

                   }
               }




        }

        else
        {
            pack->iat=(((double)1.0/lambda)*1000);
            pack->no_of_tokens=p;
            pack->st=(((double)1.0/mu)*1000);
        }


            pack->packet_ts=timestamp;
            timestamp++;

        if(pack->no_of_tokens>b)
        {

            dropped_pack_count++;
            packet_counter++;
            gettimeofday(&packet_time,NULL);
            time_diff1=timeDifference(t1,packet_time);
            printf("\n%012.3lfms: packet p%ld arrives, needs %ld tokens, dropped",time_diff1,pack->packet_ts,pack->no_of_tokens);
            free(pack);
            continue;
        }

        /*--------------Enqueuing the Packet in Q1---------------*/
        usleep(pack->iat*1000);

        pthread_mutex_lock(&m);




        gettimeofday(&packet_time,NULL);
        pack->arrival_time=packet_time;
        time_diff1=timeDifference(t1,pack->arrival_time);
        time_diff2=timeDifference(t1,prevPacketTime);
        if(pack->packet_ts==1)
            time_diff2=0.0;
        total_iat=total_iat+(time_diff1-time_diff2);
        printf("\n%012.3lfms: p%ld arrives, needs %ld tokens, inter-arrival time = %.3lfms",(time_diff1),pack->packet_ts,pack->no_of_tokens,(time_diff1-time_diff2));
        prevPacketTime=pack->arrival_time;
         if((n1=My402ListLength(&Q1))==0)
         {
            temp_value=My402ListAppend(&Q1,(void*)pack);
            if(temp_value!=1)
            {
                perror("\nAppend in Q1 Not Successful");
                exit(1);
            }

            gettimeofday(&packet_time,NULL);
            time_diff1=timeDifference(t1,packet_time);
            printf("\n%012.3lfms: p%ld enters Q1",time_diff1,pack->packet_ts);

            pack->enterQ1_time=packet_time;
            packet_counter++;

            if(token_count<pack->no_of_tokens)
                pthread_mutex_unlock(&m);

            else
            {

                   token_count= token_count-pack->no_of_tokens;

                    gettimeofday(&packet_time,NULL);
                    time_diff1=timeDifference(t1,packet_time);
                    pack->leaveQ1_time=packet_time;

                    elem1=My402ListFirst(&Q1);
                    My402ListUnlink(&Q1,elem1);
                    if(token_count>1)
                        printf("\n%012.3lfms: p%ld leaves Q1, time in Q1 = %.3lfms, token bucket now has %ld tokens",time_diff1,pack->packet_ts,time_diff2,token_count);
                    else
                        printf("\n%012.3lfms: p%ld leaves Q1, time in Q1 = %.3lfms, token bucket now has %ld token",time_diff1,pack->packet_ts,time_diff2,token_count);
                    temp_value=My402ListAppend(&Q2,(void *)pack);
                    gettimeofday(&packet_time,NULL);
                    pack->enterQ2_time=packet_time;
                    time_diff1=timeDifference(t1,packet_time);
                    printf("\n%012.3lfms: p%ld enters Q2",time_diff1,pack->packet_ts);
                    if(temp_value!=1)
                    {
                        perror("Append in Q2 Not Successful");
                        exit(1);
                    }
                if((n2=My402ListLength(&Q2))!=0)
                {
                    pthread_cond_broadcast(&queue_not_empty);
                }
                pthread_mutex_unlock(&m);
            }


        }
         else
         {

             temp_value=My402ListAppend(&Q1,(void*)pack);
             if(temp_value!=1)
             {
                 perror("\nAppend in Q1 Not Successful");
                 exit(1);
             }

             gettimeofday(&packet_time,NULL);
             time_diff1=timeDifference(t1,packet_time);

             printf("\n%012.3lfms: p%ld enters Q1",time_diff1,pack->packet_ts);

             pack->enterQ1_time=packet_time;
             packet_counter++;
             pthread_mutex_unlock(&m);

         }







        }

        pt_died=1;
	return NULL;


}

void* Server(void* arg)
{
    int length2=0, count=0,n2=0;
    My402ListElem *e1=NULL;
    Packet *pck=NULL;
    struct timeval server_time={0,0};
    double time_diff1=0.0,time_diff2=0.0,system_time=0.0;
    while(1)
    {
        pthread_mutex_lock(&m);
        while((length2=My402ListLength(&Q2))==0){
            pthread_cond_wait(&queue_not_empty,&m);
        if((length2=My402ListLength(&Q2))==0)
          {
            pthread_mutex_unlock(&m);
            pthread_cancel(signal_thread);
            return NULL;
          }
        }
        count++;
        e1=My402ListFirst(&Q2);
        pck=(Packet *)e1->obj;
        gettimeofday(&server_time,NULL);
        pck->leaveQ2_time=server_time;
        time_diff1=timeDifference(t1,server_time);
        time_diff2=timeDifference(pck->enterQ2_time,pck->leaveQ2_time);
        totalTimeQ2=totalTimeQ2+time_diff2;
        My402ListUnlink(&Q2,e1);

        printf("\n%012.3lfms: p%ld begin service at S, time in Q2 = %.3lfms",time_diff1,pck->packet_ts,time_diff2);
        gettimeofday(&server_time,NULL);
        pck->serviceStartTime=server_time;
        pthread_mutex_unlock(&m);
        usleep(pck->st*1000);
        gettimeofday(&server_time,NULL);
        pck->leave_system_time=server_time;
        time_diff1=timeDifference(t1,pck->leave_system_time);
        time_diff2=timeDifference(pck->serviceStartTime,pck->leave_system_time);
        total_st+=time_diff2;
        system_time=timeDifference(pck->arrival_time,pck->leave_system_time);
        totalSystemTime=totalSystemTime+system_time;
        sqTotalST+=(system_time*system_time);
        printf("\n%012.3lfms: p%ld departs from S, service time = %.3lfms, time in system = %.3lfms",time_diff1,pck->packet_ts,time_diff2,system_time);
        time_diff2=timeDifference(pck->enterQ1_time,pck->leaveQ1_time);
        totalTimeQ1=totalTimeQ1+time_diff2;
        if(sigint_signal==1)
        {
            pthread_mutex_unlock(&m);
            pthread_cancel(signal_thread);
            return NULL;
        }
        if((pt_died==1)&&(tt_died==1)&&((n2=My402ListLength(&Q2)==0)))
            break;

    }

    gettimeofday(&t2,NULL);
    pthread_cancel(signal_thread);

    return NULL;
}

void* serveToken( void* arg)
{
    My402ListElem *elem1=NULL;
    Packet *pack=NULL;
    int temp_value=0;
    int n1=0,n2=0;
    n1=My402ListLength(&Q1);

    struct timeval token_time={0,0};
    double time_diff1=0.0,time_diff2=0.0;
    while(1)
    {

        usleep((1/r)*1000000);


        pthread_mutex_lock(&m);

        /*Increasing Token Count*/

        if(token_count==b)
        {
            total_token_count++;
            dropped_tok_count++;
            gettimeofday(&token_time,NULL);
            time_diff1=timeDifference(t1,token_time);
            printf("\n%012.3lfms: token t%ld arrives, dropped",time_diff1,total_token_count);
            pthread_mutex_unlock(&m);
            continue;
        }


        if(token_count<b)
        {
             token_count++;
             total_token_count++;
             gettimeofday(&token_time,NULL);
             time_diff1=timeDifference(t1,token_time);
             if(token_count>1)
                printf("\n%012.3lfms: token t%ld arrives, token bucket now has %ld tokens",time_diff1,total_token_count,token_count);
             else
                printf("\n%012.3lfms: token t%ld arrives, token bucket now has %ld token",time_diff1,total_token_count,token_count);
        }

        n1=My402ListLength(&Q1);
        if(n1==0)
        {
            pthread_mutex_unlock(&m);
        }

        else
        {

            elem1=My402ListFirst(&Q1);
            pack=(Packet *)elem1->obj;


            if(token_count<(pack->no_of_tokens))
                pthread_mutex_unlock(&m);

            else{


                    token_count=token_count-pack->no_of_tokens;
                    gettimeofday(&token_time,NULL);
                    time_diff1=timeDifference(t1,token_time);
                    pack->leaveQ1_time=token_time;

                    My402ListUnlink(&Q1,elem1);
                    if(token_count>1)
                        printf("\n%012.3lfms: p%ld leaves Q1, time in Q1 = %.3lfms, token bucket now has %ld tokens",time_diff1,pack->packet_ts,time_diff2,token_count);
                    else
                        printf("\n%012.3lfms: p%ld leaves Q1, time in Q1 = %.3lfms, token bucket now has %ld token",time_diff1,pack->packet_ts,time_diff2,token_count);
                    temp_value=My402ListAppend(&Q2,(void *)pack);
                    gettimeofday(&token_time,NULL);
                    pack->enterQ2_time=token_time;
                    time_diff1=timeDifference(t1,token_time);
                    printf("\n%012.3lfms: p%ld enters Q2",time_diff1,pack->packet_ts);
                    if(temp_value!=1)
                    {
                        perror("Append in Q2 Not Successful");
                        exit(1);
                    }
                    n2=My402ListLength(&Q2);
                    if(n2!=0)
                    {
                        pthread_cond_signal(&queue_not_empty);
                        pthread_mutex_unlock(&m);
                    }
                pthread_mutex_unlock(&m);
        }
        }

        if(((n1=My402ListLength(&Q1))==0)&&(packet_counter==num))
        {
            pthread_mutex_unlock(&m);
            pthread_cond_signal(&queue_not_empty);
            break;
        }

    }
    pthread_mutex_unlock(&m);
    tt_died=1;
    return NULL;
}

void* monitor()
{
    
    while(1)
    {
        sigwait(&set);
        gettimeofday(&t2,NULL);
        sigint_signal=1;
        pthread_cancel(serve_packet);
        pthread_cancel(serve_token);
        pthread_cond_signal(&queue_not_empty);



    }

}


int main(int argc, char* argv[])
{

  int rc=0;
  double diff=0.0,total_emulation_time=0.0,sd=0.0;

  int i=0;
  char fields[6][7];
  long values[4]={0,0,0,0};
  double value[3]={0,0,0};
  char filename[1024]="\0";


  sigemptyset(&set);
  sigaddset(&set,SIGINT);
  pthread_sigmask(SIG_BLOCK,&set,0);
  pthread_create(& signal_thread,0,monitor,NULL);
  rc=My402ListInit(&Q1);
  if(rc==0)
  {
    perror("\nInitializing Q1 failed");
  }
  rc=My402ListInit(&Q2);
  if(rc==0)
  {
    perror("\nInitializing Q2 failed");
  }

  for(i=0;i<6;i++)
  {
      *(fields[i])='\0';
  }

  printf("\nEmulation Parameters:");

  for(i=0;i<(argc-1);i++)
      {
          if(strcmp(argv[i],"-n")==0)
          {
              values[0]=atol(argv[i+1]);
              if(values[0]>2147483647)
              {
                  fprintf(stderr,"\nValue out of n bounds");
                  exit(1);
              }
              num=values[0];
          }

          if(strcmp(argv[i],"-r")==0)
          {
              value[0]=strtod(argv[i+1],NULL);
              r=value[0];
          }

          if(strcmp(argv[i],"-B")==0)
          {
              values[1]=atol(argv[i+1]);
              if(values[1]>2147483647)
              {
                  fprintf(stderr,"\nValue out of B bounds");
                  exit(1);
              }
              b=values[1];
          }

          if(strcmp(argv[i],"-P")==0)
          {
              values[2]=atol(argv[i+1]);
              if(values[2]>2147483647)
              {
                  fprintf(stderr,"\nValue out of P bounds");
                  exit(1);
              }
              p=values[2];
          }

          if(strcmp(argv[i],"-lambda")==0)
          {
              value[1]=strtod(argv[i+1],NULL);
              lambda=value[1];
          }

          if(strcmp(argv[i],"-mu")==0)
          {
              value[2]=strtod(argv[i+1],NULL);
              mu=value[2];
          }

          if(strcmp(argv[i],"-t")==0)
          {
              values[0]=20;
              num=values[0];
              values[2]=3;
              p=values[2];
              values[3]=1;
              value[1]=0.5;
              lambda=value[1];
              value[2]=0.35;
               mu=value[2];
              strcpy(filename,argv[i+1]);
          }


      }

  if(r<0.1)
    r=0.1;

  if(lambda<0.1)
      lambda=0.1;

  if(mu<0.1)
      mu=0.1;

  if(values[3]==1)
  {
      fp=fopen(filename,"r");
      printf("\n\tr=%lf",r);
      printf("\n\tB=%ld",b);
      printf("\n\ttsfile=%s\n",filename);
  }

  else
  {
      printf("\n\tlambda=%lf",lambda);
      printf("\n\tmu=%lf",mu);
      printf("\n\tr=%lf",r);
      printf("\n\tB=%ld",b);
      printf("\n\tP=%ld",p);
      printf("\n\tnumber to arrrive=%ld\n",num);

  }




  gettimeofday(&t1,NULL);
  diff=timeDifference(t1,t1);
  printf("\n\n%012.3lfms: emulation begins",diff);


  pthread_create(& serve_packet,0,servePacket,NULL);
  pthread_create(& serve_token,0, serveToken,NULL);
  pthread_create(& server,0,Server,NULL);


  pthread_join(serve_packet,NULL);
  pthread_join(serve_token,NULL);
  pthread_cond_signal(&queue_not_empty);
  pthread_join(server,NULL);

  total_emulation_time=timeDifference(t1,t2);
  printf("\n\n\nStatistics:\n");
  printf("\n\taverage packet inter-arrival time = %.6gsec",fabs(total_iat/(num*1000)));
  printf("\n\taverage packet service time = %.6gsec",fabs(total_st/(num*1000)));
  printf("\n\n\taverage number of packets in Q1 = %.6g",fabs(totalTimeQ1/total_emulation_time));
  printf("\n\taverage number of packets in Q2 = %.6g",fabs(totalTimeQ2/total_emulation_time));
  printf("\n\taverage number of packets at S = %.6g",fabs(total_st/total_emulation_time));
  printf("\n\n\taverage time a packet spent in system = %.6gsec",fabs(totalSystemTime/((packet_counter-dropped_pack_count)*1000)));
  sd=sqrt(fabs((sqTotalST/(packet_counter*1000000))-((totalSystemTime/(packet_counter*1000))*(totalSystemTime/(packet_counter*1000)))));
  printf("\n\tstandard deviation for time spent in system = %0.6g",fabs(sd));
  printf("\n\n\ttoken drop probability = %.6g",fabs((double)dropped_tok_count/total_token_count));
  printf("\n\tpacket drop probability = %.6g\n",fabs((double)dropped_pack_count/packet_counter));
  return(0);
}


