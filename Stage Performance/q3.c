#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>
#include <semaphore.h>
#include<string.h>

typedef struct Stage Stage;
typedef struct Musician Musician;
typedef struct Coordinator Coordinator;

struct Stage{
	char performer_name[100];
	char stage_type;
	int indx;
	int type;
	int status;
	int performer_id;
	int joiner_id;
	pthread_t stage_id;
	pthread_mutex_t stage_mutex;

};
Stage stages[300];
struct Musician{
	char name[100];
	char instrument;
	int indx;
	int type;
	int arrival_time;
	int stage;
	int status;
	int performer_status;
	pthread_t musician_id;
	pthread_mutex_t musician_mutex;
	int performance_time;
	long double start_time;
	

};
Musician musicians[300];
struct Coordinator{
	int indx;
	int status;
	pthread_t coordinator_id;
	pthread_mutex_t coordinator_mutex;
};
Coordinator coordinators[300];

void * stage_thread(void * args);
void * musician_thread(void * args);
void * cordinator_thread(void * args);
void waiting_to_perform(Musician *musician);
void wait_for_tshirt(char name[]);

int k,a,e,c,t1,t2,t;
int main()
{
	srand(time(0));
	printf("Enter no.of performers,no.of acoustic stages, electric stages,performance time1,time2,wait time\n");
	scanf("%d%d%d%d%d%d%d",&k,&a,&e,&c,&t1,&t2,&t);
	if(a==0 || e==0){
		printf("No stages  :(\n");
		return 0;
	}
	if(k==0){
		printf("No singers  :(\n");
		return 0;
	}

	int i,j;
	for(i=1;i<=k;i++){
		char x;
		char name[100];
		scanf("%s %c %d",name,&x,&j);
		strcpy(musicians[i].name,name);
		musicians[i].indx=i;
		musicians[i].arrival_time=j;
		musicians[i].instrument=x;
		musicians[i].status=0;

		if(x=='v'){
			musicians[i].type=1;
		}
		else if(x=='b'){
			musicians[i].type=2;
		}
		else{
			musicians[i].type=3;
		}
	}
	for(i=1;i<=a;i++){
		stages[i].indx=i+1;
		stages[i].stage_type='a';
		stages[i].status=0;
	}
	for(i=a+1;i<=a+e;i++){
		stages[i].indx=i+1;
		stages[i].stage_type='e';
		stages[i].status=0;
	}
	for(i=1;i<=c;i++){
		coordinators[i].indx=i+1;
	}

	for(i=1;i<=a+e;i++){
		pthread_create(&(stages[i].stage_id),NULL,stage_thread,&stages[i]);
	}
	for(i=1;i<=k;i++){
		//printf("k\n");
		pthread_create(&(musicians[i].musician_id),NULL,musician_thread,&musicians[i]);
	}
	/*for(i=1;i<=c;i++)
		pthread_create(&(coordinators[i].coordinator_id),NULL,cordinator_thread,&(coordinators[i]));*/

	for(int i=1; i <=c; i++){
		pthread_join(musicians[i].musician_id, 0);
	}


	printf("\nFinished...\n\n");
	for(i=1;i<=a+e;i++)
		pthread_mutex_destroy(&(stages[i].stage_mutex));

	/*for(i=1;i<=k;i++)
		pthread_mutex_destroy(&(musicians[i].musician_mutex));*/

	return 0;


}

void * stage_thread(void * args){
	Stage * stage=(Stage *)args;
	//printf("ok\n");
}
void * musician_thread(void * args){
	//printf("ok2\n");
	Musician * musician = (Musician *) args;
	sleep(musician->arrival_time);
	printf("\n\x1B[1;32m%s %c arrived \n",musician->name,musician->instrument);
	int got_stage=0;
	int time_exceeded=0;
	clock_t ta = clock();

	while(got_stage==0){
		clock_t time_taken = clock() - ta;
		double times = ((double)time_taken)/CLOCKS_PER_SEC;

		if(times> (double)t){
			//printf("\n%s %c left because of impatience\n",musician->name,musician->instrument );
			time_exceeded=1;
			break;
		}
		int f=0;
		for(int i=1;i<=a+e;i++){
			pthread_mutex_lock(&(stages[i].stage_mutex));
			if(stages[i].status==0 && i<=a && stages[i].stage_type=='a' && musician->instrument=='v'){
				stages[i].status=1;
				musician->stage=i;
				musician->status=1;
				strcpy(stages[i].performer_name,musician->name);
				f=1;
				pthread_mutex_unlock(&(stages[i].stage_mutex));
				break;

			}
			else if(stages[i].status==0 && i<=a+e && stages[i].stage_type=='e' && musician->instrument=='b'){
				stages[i].status=1;
				musician->stage=i;
				musician->status=1;
				strcpy(stages[i].performer_name,musician->name);
				f=1;
				pthread_mutex_unlock(&(stages[i].stage_mutex));
				break;
				
			}
			else if(stages[i].status==0 && musician->type==3){
				stages[i].status=1;
				musician->stage=i;
				musician->status=1;
				strcpy(stages[i].performer_name,musician->name);
				f=1;
				pthread_mutex_unlock(&(stages[i].stage_mutex));
				break;

			}
			else if(stages[i].status==1 && musician->type==3 && musician->instrument=='s'){
				stages[i].status=2;
				musician->stage=i;
				musician->status=2;
				strcpy(stages[i].performer_name,musician->name);
				f=1;
				pthread_mutex_unlock(&(stages[i].stage_mutex));
				break;

			}
			pthread_mutex_unlock(&(stages[i].stage_mutex));
		}
		if(f==1){
			got_stage=1;
			break;
		}
	}
	if(time_exceeded==1){
		printf("\x1B[1;31m%s left because of impatience\n",musician->name );
		return NULL;
	}
	else{
		//printf("kkk %s %d %d %d\n",musician->name,musician->status,musician->stage,got_stage );
	}
	if(got_stage==1 ){
		if(musician->status==2){
			printf("\x1B[1;33m%s joined %s's performance, performance extended by 2 secs\n",musician->name,stages[musician->stage].performer_name );
			//strcpy(stages[musician->stage].joiner,musician->name);
			stages[musician->stage].joiner_id=musician->indx;
		}
		else{
			waiting_to_perform(musician);
		}

	}

}
void * cordinator_thread(void * args){
	//printf("ok3\n");
}

void waiting_to_perform(Musician *musician){
	musician->performance_time=rand()%(t2-t1) +t1;
	char tp[15];
	int i=musician->stage;
	

	if(musician->stage <=a){
		strcpy(tp,"acoustic");
	}
	else{
		strcpy(tp,"electric");
	}
	printf("\x1B[1;35m%s performing %c  at %s stage for %d sec \n",musician->name,musician->instrument,tp,musician->performance_time);
	sleep(musician->performance_time);
	if(stages[musician->stage].status==2){
		sleep(2);
		printf("\x1B[1;36m%s performance at %s stage ended\n",musician->name,tp );
		int x=stages[musician->stage].joiner_id;
		wait_for_tshirt(musicians[x].name);
	}
	else{
		printf("\x1B[1;36m%s performance at %s stage ended\n",musician->name,tp );
	}
	wait_for_tshirt(musician->name);
	pthread_mutex_lock(&(stages[i].stage_mutex));
	stages[musician->stage].status=0;
	pthread_mutex_unlock(&(stages[i].stage_mutex));
	return;
}

void wait_for_tshirt(char name[]){
	int got_tshirt=0;
	while(got_tshirt==0){
		for(int i=1;i<=c;i++){
			if(coordinators[i].status==0){
				pthread_mutex_lock(&(coordinators[i].coordinator_mutex));
				coordinators[i].status=1;			
				sleep(rand()%3+1);
				printf("\x1B[1;34m%s collecting tshirt\n",name );
				coordinators[i].status=0;
				got_tshirt=1;
				pthread_mutex_unlock(&(coordinators[i].coordinator_mutex));
			}
		}
	}
	return;

}
