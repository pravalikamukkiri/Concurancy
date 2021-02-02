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

typedef struct Company Company;
typedef struct Vaccination_zone Vaccination_zone;
typedef struct Student Student;

struct Company{
	int indx;
	int batches;
	int capacity;
	float prob;
	pthread_t company_thread_id;
	pthread_mutex_t company_mutex;
	pthread_cond_t cv_vaccination_zone;

};

Company companies[100];
struct Vaccination_zone{
	int indx;
	int slots;
	int capacity;
	int occupancy;
	float batche_prob;
	pthread_t vaccination_zone_thread_id;
	pthread_mutex_t vaccination_zone_mutex;
	pthread_cond_t cv_student;

};
Vaccination_zone vaccination_zones[100];

struct Student{
	int indx;
	int time;
	pthread_t student_thread_id;

};
Student students[100];

int num_companies,num_vacc_zones,num_students;


void * company_thread(void * args);
void vaccine_ready(Company * cmpny);
void * vaccination_zone_thread(void * args);
void ready_to_vaccinate(Vaccination_zone *vczone);
void * student_thread(void * args);
void wait_for_slot(Student * student);
void student_in_slot(int i,float succ_prob,Student * student );
void test(Student * student);





int main()
{
	srand(time(0));
	
	printf("Enter no.of caompanies no.of vaccination zones no.of students\n");
	scanf("%d %d %d",&num_companies,&num_vacc_zones,&num_students);
	if(num_companies==0){
		printf("No companies\n");
		return 0;
	}
	if(num_vacc_zones==0){
		printf("No vacc zones\n");
	}
	if(num_students==0){
		printf("No students to vaccinate\n");
		return 0;
	}

	int i;
	for(i=1;i<=num_companies;i++){
		companies[i].indx=i;
		double x;
		scanf("%lf",&x);
		companies[i].prob=x;
	}

	for(i=1;i<=num_vacc_zones;i++)
		vaccination_zones[i].indx=i;

	for(i=1;i<=num_students;i++){
		students[i].indx=i;
		students[i].time=1;
	}

	printf("Beggining of simulation...\n\n");

	for(i=1;i<=num_companies;i++){
		pthread_create(&(companies[i].company_thread_id),NULL,company_thread,&companies[i]);
	}
	for(i=1;i<=num_vacc_zones;i++){
		pthread_create(&(vaccination_zones[i].vaccination_zone_thread_id),NULL,vaccination_zone_thread,&vaccination_zones[i]);
	}
	for(i=1;i<=num_students;i++)
		pthread_create(&(students[i].student_thread_id),NULL,student_thread,&(students[i]));

	for(int i=1; i <=num_students; i++){
		pthread_join(students[i].student_thread_id, 0);
	}


	printf("\nSimulation is over...\n\n");
	for(i=1;i<=num_companies;i++)
		pthread_mutex_destroy(&(companies[i].company_mutex));

	for(i=1;i<=num_vacc_zones;i++)
		pthread_mutex_destroy(&(vaccination_zones[i].vaccination_zone_mutex));

	return 0;
}



void * company_thread(void * args){
	int x;
	Company * company = (Company*) args;
	while(1){
		int x,y,z;
		z=rand()%80 + 20;
		float p=z/100.0;
		x=rand()%4 + 1;
		z=rand()%10 + 10;
		printf("\n\x1B[1;31mPharmaceutical Company %d is preparing %d batches of vaccines which have success probability %.2f. \e[0m\n",company->indx,x,company->prob);
		y=rand()%4+2;
		sleep(1);
		pthread_mutex_lock(&(company->company_mutex));
		company->batches =x;		
		company->capacity=z; 
		//company->prob=p;
		printf("\n\x1B[1;34mPharmaceutical Company %d has prepared %d batches of vaccines which have success probability %.2f.\nWaiting for all the vaccines to be used to resume production. \e[0m\n",company->indx,company->batches,company->prob);
		vaccine_ready(company);
	}
	return NULL;
}

void vaccine_ready(Company * cmpny){
	Company * company = (Company*) cmpny;
	while(1){
		if(company->batches > 0)
			pthread_cond_wait(&(company->cv_vaccination_zone), &(company->company_mutex));
		else
			break;
		
		
	}
	printf("\n\x1B[1;31mAll the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now. \e[0m\n",company->indx);
	pthread_mutex_unlock(&(company -> company_mutex));
}

void * vaccination_zone_thread(void * args){
	Vaccination_zone * vaccination_zone= (Vaccination_zone *) args;
	int f=0;
	while(1){
		int i;
		f=0;
		for(i=1;i<=num_vacc_zones;i++){
			pthread_mutex_lock(&(companies[i].company_mutex));
			if(companies[i].batches > 0){
				vaccination_zone->batche_prob=companies[i].prob;
				vaccination_zone->capacity = companies[i].capacity;
				companies[i].batches--;
				f=1;
				printf("\n\x1B[1;36mPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %.2f. \e[0m\n",i,vaccination_zone->indx,companies[i].prob);
				pthread_cond_signal(&(companies[i].cv_vaccination_zone));
				pthread_mutex_unlock(&(companies[i].company_mutex));
                break;
			}
			else{
				pthread_cond_signal(&(companies[i].cv_vaccination_zone));
				pthread_mutex_unlock(&(companies[i].company_mutex));
			}
		}
		while(f){
			int s=8;
			pthread_mutex_lock(&(vaccination_zone->vaccination_zone_mutex));
			if(vaccination_zone->capacity == 0){
				printf("\n\n\x1B[1;31mVaccination Zone %d has run out of vaccines \e[0m\n",vaccination_zone->indx );
				pthread_mutex_unlock(&(vaccination_zone->vaccination_zone_mutex));
				break;
			}

			vaccination_zone->occupancy=0;
			if(vaccination_zone->capacity <=8 && vaccination_zone->capacity>=1)
				s=vaccination_zone->capacity;
			vaccination_zone->slots=s;
			vaccination_zone->capacity -= vaccination_zone->slots;
			printf("\n\x1B[1;34mVaccination Zone %d is ready to vaccinate with %d slots \e[0m\n",vaccination_zone->indx,vaccination_zone->slots );
			ready_to_vaccinate(vaccination_zone);
		}
	}

	return NULL;
}

void ready_to_vaccinate(Vaccination_zone *vczone){
	Vaccination_zone * vaccination_zone = (Vaccination_zone *) vczone;
	sleep(rand()%2+1);
	printf("\n\x1B[1;35mVaccination Zone %d entering Vaccination Phase \e[0m\n",vaccination_zone->indx );
	while(1){
		if(vaccination_zone->occupancy < vaccination_zone->slots){
			pthread_cond_wait(&(vaccination_zone-> cv_student), &(vaccination_zone-> vaccination_zone_mutex));
		}
		else{
			printf("\n\x1B[1;31mVaccination Zone %d has run out of vaccines \e[0m\n",vaccination_zone->indx);
            break;
		}

	}
	pthread_mutex_unlock(&(vaccination_zone->vaccination_zone_mutex));
}

void * student_thread(void * args){
	Student * student = (Student*)args;

	int std_time = student->time;
	char arr[3];
	if(std_time==1){
		arr[0]='s';
		arr[0]='t';
	}
	else{
		arr[0]='r';
		arr[0]='d';
	}
	printf("\n\x1B[1;33mStudent %d has arrived for his %d%s round of Vaccination \e[0m\n",student->indx,std_time,arr);

    int arrival_time = rand()%2 +2;
    sleep(arrival_time);
    printf("\n \x1B[1;36mStudent %d is waiting to be allocated a slot on a Vaccination Zone \e[0m\n",student->indx);
    wait_for_slot(student);
	return NULL;
}

void wait_for_slot(Student * student)
{
	int idx=student->indx;
    int ff =0;
    while(ff==0)
    {
        for(int i=1; i <= num_vacc_zones; i++)
        {
            pthread_mutex_lock(&(vaccination_zones[i].vaccination_zone_mutex));
            if(vaccination_zones[i].slots > vaccination_zones[i].occupancy )
            {
            	ff = 1;
                vaccination_zones[i].occupancy++;
                student_in_slot(i,vaccination_zones[i].batche_prob, student);
                break;
            }
            else{
           		pthread_cond_signal(&(vaccination_zones[i].cv_student));
            	pthread_mutex_unlock(&(vaccination_zones[i].vaccination_zone_mutex));
            }
        }
    }
    return;
}
void student_in_slot(int i,float succ_prob,Student * student )
{
	int idx=student->indx;
	int x=students[i].time;
    printf("\n\x1B[1;33mStudent %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated \e[0m\n",idx, i);
    sleep(rand()%2+1);
    printf("\n\x1B[1;32mStudent %d on Vaccination Zone %d has been vaccinated which has success probability %.2f \e[0m\n",idx, i,succ_prob);
    pthread_cond_signal(&(vaccination_zones[i].cv_student));
    pthread_mutex_unlock(&(vaccination_zones[i].vaccination_zone_mutex));
    test(student);
    return;
}

void test(Student * student){
	int x=rand()%2;
	if(x==0){
		printf("\x1B[1;31mStudent %d has tested negative for antibodies \e[0m\n",student->indx);
		student->time++;
		if(student->time > 3){
			printf("\x1B[1;31mYou cant enter college \e[0m\n");
		}
		else
			student_thread(student);

	}
	else{
		printf("\n\x1B[1;32mStudent %d has tested positive for antibodies \e[0m\n",student->indx);
	}
}