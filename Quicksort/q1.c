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

long int *array;
pthread_mutex_t mutex;


void merge(long int * arr,int low,int mid,int high){
	int x=high- low+1;
	long int sorted_array[x+5];
	int a,b,c;

	a=low;
	b=mid+1;
	c=0;
	while(a<=mid && b<=high){
		if(arr[a]<arr[b]){
			sorted_array[c]=arr[a];
			a++;
		}
		else{
			sorted_array[c]=arr[b];
			b++;
		}
		c++;
	}
	while(a<=mid){
		sorted_array[c]=arr[a];
		c++;
		a++;
	}
	while(b<=high){
		sorted_array[c]=arr[b];
		c++;
		b++;
	}
	int y=0;
	for(int i=low;i<=high;i++){
		arr[i]=sorted_array[y];
		y++;
	}

}

void selection_sort(long int *arr, int start,int end)
{
	int i,j,k;
	long int min;
	int n=end;
	for(i=start;i<n;i++){
		min=i;
		for(j=i+1;j<=n;j++){
			if(arr[j]<arr[min])
				min=j;
		}
		long int temp=arr[min];
		arr[min]=arr[i];
		arr[i]=temp;
	}

}

void concurrent_mergesort(long int * arr,int low,int high)
{
	int len,i,j;
	if(low>high){
		return ;
	}
	len=high - low +1;
	if(len<5){
		selection_sort(arr,low,high);
		return;
	}
	pid_t lpid,rpid;
	lpid=fork();
	if(lpid>0){
		rpid=fork();
		if(rpid==0){
			concurrent_mergesort(arr,low+len/2,high);
			exit(0);
		}
		else{
			int s;
			waitpid(lpid,&s,0);
			waitpid(rpid,&s,0);
		}

	}
	else if(lpid==0){
		concurrent_mergesort(arr,low,low+len/2-1);
		exit(0);
	}
	else{
		perror("Left child proc not created");
		exit(-1);
	}
	merge(arr,low,low+len/2-1,high);


}
struct arg
{
	//long int *arr;
	int low;
	int high;
	
};
void *thread_mergesort(void* a)
{

	struct arg *args = (struct arg*)a;

	int l,r;
	//long int *array;

	//array=args->arr;
	l=args->low;
	r=args->high;
	if(l>r){
		return NULL;
	}
	int len=r-l+1;
	if(len<5){
		selection_sort(array,l,r);
		return NULL;
	}

	int mid=l+ (r-l)/2;

	struct arg a1;
	//a1.arr=array;
	a1.low=0;
	a1.high=mid;

	pthread_t tidl;
	pthread_create(&tidl,NULL,thread_mergesort,&a1);
	struct arg a2;
	//a2.arr=array;
	a2.low=mid+1;
	a2.high=r;
	pthread_t tidr;
	pthread_create(&tidr,NULL,thread_mergesort,&a2);
	pthread_join(tidl,NULL);
	pthread_join(tidr,NULL);
	pthread_mutex_lock(&mutex);
	merge(array,l,mid,r);
	pthread_mutex_unlock(&mutex);

	pthread_exit(NULL);

}

void normal_mergesort(long int * arr,int low,int high){
	if(low<high){
		int mid=low + (high- low)/2;
		normal_mergesort(arr,low,mid);
		normal_mergesort(arr,mid+1,high);
		merge(arr,low,mid,high);
	}

}
int main()
{
	int n;
	scanf("%d",&n);
	key_t mem_key = IPC_PRIVATE;
	int shm_id = shmget(mem_key,n,IPC_CREAT | 0666);
	long int *arr;
	arr=shmat(shm_id,NULL,0);

	int i,j;
	for(i=0;i<n;i++)
		scanf("%ld",&arr[i]);
	
	long int *brr=malloc((n+5)*sizeof(long int));
	long int *crr=malloc((n+5)*sizeof(long int));
	for(i=0;i<n;i++){
		brr[i]=arr[i];
		crr[i]=arr[i];
	}
	array=arr;

	struct timespec ts;
	long double x,y,t1,t2,t3;

	//Normal merge sort
	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	x=ts.tv_nsec/(1e9)+ts.tv_sec;
	normal_mergesort(brr,0,n-1);
	printf("Normal mergesort\n");
	for(int i=0;i<n;i++)
		printf("%ld ",brr[i]);

	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	y=ts.tv_nsec/(1e9)+ts.tv_sec;

	t1=y-x;
	printf("\ntime %Lf\n",t1 );

	//Concurrent merge sort
	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	x=ts.tv_nsec/(1e9)+ts.tv_sec;

	concurrent_mergesort(crr,0,n-1);
	printf("concurrent mergesort\n");
	for(int i=0;i<n;i++)
		printf("%ld ",crr[i]);

	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	y=ts.tv_nsec/(1e9)+ts.tv_sec;

	t2=y-x;
	printf("\ntime %Lf\n",t2 );
	printf("normal mergesort runs %Lf faster than concurrent mergesort\n",t2/t1 );


	//multi thread merge sort
	pthread_t tid;
	struct arg a;
	//a.arr=arr;
	a.low=0;
	a.high=n-1;
	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	x=ts.tv_nsec/(1e9)+ts.tv_sec;

	pthread_create(&tid,NULL,thread_mergesort,&a);
	pthread_join(tid,NULL);
	printf("Multi thread mergesort\n");
	for(int i=0;i<n;i++)
		printf("%ld ",array[i]);
	

	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	y=ts.tv_nsec/(1e9)+ts.tv_sec;

	t3=y-x;
	printf("\ntime %Lf\n",t3 );
	printf("normal mergesort runs %Lf faster than concurrent mergesort\n",t3/t1 );

	return 0;
}
