#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define QUEUESIZE 10
#define LOOP 10
#define P 10


void *producer (void *args);
void *consumer (void *args);

struct workFunction {
  void * (*work)(void *);
  void * arg;
  struct timeval production_time;
};

typedef struct {
  struct workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

void* calculate_sin(void* arg){
   double *angles = (double *) arg;
   for(int i=0; i<10; i++){
      double x =  sin(angles[i]);
      //printf("Thread %ld: sin(%.3f) = %.4f\n", pthread_self(), angles[i], x);
   }
   free(angles);
   return NULL;
}

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, struct workFunction in);
void queueDel (queue *q, struct workFunction *out);

double total_delay;
int jobs_num;
pthread_mutex_t meas_mut = PTHREAD_MUTEX_INITIALIZER;



int run_experiment(int Q)
{

  queue *fifo;
  pthread_t pro[P], con[Q];
  
  
  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  for(int i=0; i<P; i++){
    pthread_create (&pro[i], NULL, producer, fifo);
  }
  
  for(int j=0; j<Q; j++){
    pthread_create (&con[j], NULL, consumer, fifo);
  }

   for(int i=0; i<P; i++){
    pthread_join (pro[i], NULL);
  }
	
  puts("Producers executed!");
  

  struct workFunction stop_job = {NULL, NULL};
  for (int i = 0; i < Q; i++) {
     pthread_mutex_lock(fifo->mut);
     while (fifo->full) { 
         pthread_cond_wait(fifo->notFull, fifo->mut);
     }
     queueAdd(fifo, stop_job);
     pthread_mutex_unlock(fifo->mut);
     pthread_cond_signal(fifo->notEmpty); 
  }



  for(int j = 0; j < Q; j++) {
      pthread_join(con[j], NULL);
  }


  pthread_mutex_lock(&meas_mut);
  if (jobs_num > 0) {
      printf("\n--- Στατιστικά Πειράματος ---\n");
      printf("Συνολικά Jobs: %d\n", jobs_num);
      printf("Μέσος Χρόνος Αναμονής (Latency): %.4f ms\n", total_delay / jobs_num);
      printf("-----------------------------\n");
  }
  pthread_mutex_unlock(&meas_mut);


  queueDelete (fifo);

  return 0;
}



int main(){
  int q_vals[] = {1, 3, 5, 8, 10, 12, 20};
  int num_tests = 7;

  printf("Threads\t | Avg Delay (ms)\n");
  printf("---------------------------\n");

  for (int i = 0; i < num_tests; i++) {
    int current_Q = q_vals[i];

      // Μηδενισμός των global μετρητών πριν από κάθε τεστ
      total_delay = 0;
      jobs_num = 0;

      // Εκτέλεση του πειράματος
      run_experiment(current_Q);

      // Εκτύπωση αποτελέσματος σε μορφή πίνακα
      printf("%d\t | %.4f\n", current_Q, total_delay / jobs_num);
    }

    return 0;
}

void *producer (void *q)
{
  queue *fifo;
	
  fifo = (queue *)q;

  for (int i = 0; i < LOOP; i++) {
    double *angles = malloc(10*sizeof(double));
    usleep(10000);
	for (int j = 0; j < LOOP; j++) {
		angles[j] =(double)rand()/ RAND_MAX;
	}

	struct workFunction job;
	job.work = calculate_sin;
	job.arg=angles;
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }

    gettimeofday(&job.production_time, NULL);
//    usleep(10000); /////////////////////////////////////////////////////////////////
    queueAdd (fifo, job);

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }
  
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  fifo = (queue *)q;
  struct workFunction job;
	
  while(1){

    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, &job);

    /* if (job.work == NULL) {
      pthread_cond_signal(fifo->notFull);
      pthread_mutex_unlock(fifo->mut);
      break; 
    }*/

    if (job.work == NULL) {
      pthread_cond_broadcast(fifo->notEmpty);
      pthread_cond_broadcast(fifo->notFull);
      pthread_mutex_unlock(fifo->mut);
      return NULL; 
    }

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);

    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    double delay = (end_time.tv_sec - job.production_time.tv_sec) * 1000.0;
    delay+= (end_time.tv_usec - job.production_time.tv_usec) / 1000.0;

    pthread_mutex_lock(&meas_mut);
    total_delay += delay;
    jobs_num++;
    pthread_mutex_unlock(&meas_mut);

    printf ("consumer: Received job!.\n");  
    job.work(job.arg);

  }
  return (NULL);
}

/*
  typedef struct {
  int buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, struct workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;

  if (q->tail == QUEUESIZE)
    q->tail = 0;

  if (q->tail == q->head)
    q->full = 1;

  q->empty = 0;

  return;
}

void queueDel (queue *q, struct workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;

  if (q->head == QUEUESIZE)
    q->head = 0;

  if (q->head == q->tail)
    q->empty = 1;

  q->full = 0;

  return;
}
