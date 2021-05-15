#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

/* linked list structure */
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head) {
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next) {
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry) {
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline void list_del_init(struct list_head *entry) {
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static inline void list_move(struct list_head *list, struct list_head *head) {
        __list_del(list->prev, list->next);
        list_add(list, head);
}

static inline void list_move_tail(struct list_head *list,
				  struct list_head *head) {
        __list_del(list->prev, list->next);
        list_add_tail(list, head);
}

static inline int list_empty(const struct list_head *head) {
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head);	\
       pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
		n = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#if 0    //DEBUG
#define debug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define debug(fmt, args...)
#endif

/* assignment */
LIST_HEAD(readyQueue);
LIST_HEAD(jobQueue);
LIST_HEAD(realtimeClassQueue);
LIST_HEAD(normalClassQueue);
LIST_HEAD(idleClassQueue);

int jobCount = 0;

typedef struct
{
    int pid;
    int arrival_time;
    int code_bytes;
}processInfo_t;

typedef struct
{
    int op;
    int len;
} operations_t;

typedef struct
{
	int pid;
    int arrival_time;
    int code_bytes;
	int op;
	int len;

	int isProcessArrival;
	int isRealtime;
	int programCounter;
	int progress;
	int timeSlice;

	int responseTime;
	int isResponse;
	int waitingTime;

	struct list_head ready;
	struct list_head job;
    struct list_head realtimeClass;
    struct list_head normalClass;
    struct list_head idleClass;
} process_t;

void addIdleProcess()
{		
	process_t *processPtr = (process_t *) malloc(sizeof(process_t));
	INIT_LIST_HEAD(&processPtr->idleClass);
	
	processPtr->pid = 100;
	processPtr->arrival_time = 0;
	processPtr->code_bytes = 2;
	processPtr->op = 0;
	processPtr->len = 0xFF;
	
	processPtr->isProcessArrival = 1;
	processPtr->isRealtime = 0;
	processPtr->programCounter = 0;
	processPtr->progress = 0;
	processPtr->timeSlice = 0;

	list_add(&processPtr->idleClass, &idleClassQueue);
}

void checkProcessArrival(int *cpuClock)
{
	process_t *cur, *next;

	/* add Process to each Queue */
	list_for_each_entry_safe(cur, next, &jobQueue, job)
	{
		if(*cpuClock >= cur->arrival_time && !cur->isProcessArrival)
		{
			printf("%04d CPU: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\tPC: %03d\n", *cpuClock, cur->pid, cur->arrival_time, cur->code_bytes, cur->programCounter);

			cur->isProcessArrival = 1;
			
			/* realtime Process : pid 80 ~ 99*/
			if(cur->pid >= 80 && cur->pid < 100)
				list_add_tail(&cur->realtimeClass, &realtimeClassQueue);

			/* normal Process */
			else
				list_add_tail(&cur->normalClass, &normalClassQueue);
		}
	}
}


void addtoreadyQueue(int *cpuClock)
{
	process_t *cur;

	/* realtime -> ready Queue */
	if(!list_empty(&realtimeClassQueue))
	{
		cur = list_first_entry(&realtimeClassQueue, process_t, realtimeClass);
		list_add_tail(&cur->ready, &readyQueue);
		list_del(&cur->realtimeClass);
	}

	/* normal -> ready Queue */
	else if (!list_empty(&normalClassQueue))
	{
		cur = list_first_entry(&normalClassQueue, process_t, normalClass);
		list_add_tail(&cur->ready, &readyQueue);
		list_del(&cur->normalClass);
	}

	/* idle -> ready Queue */
	else
	{
		cur = list_first_entry(&idleClassQueue, process_t, idleClass);
		list_add_tail(&cur->ready, &readyQueue);
		list_del(&cur->idleClass);
	}
}

void processSimulator()
{
	int cpuClock = 0;
	int idleClock = 0;
	int performedjobCount = 0;
	int totalResposeTime = 0;
	int totalWaitingTime = 0;

	process_t *cur, *next, *sw;

	while(1)
	{
		checkProcessArrival(&cpuClock);
		addtoreadyQueue(&cpuClock);
		
		/* add idle Process to idleClassQueue */
		if(cpuClock == 0)
		{
			addIdleProcess();
			printf("0000 CPU: Loaded PID: 100\tArrival: 000\tCodesize: 002\tPC: 000\n");
		}

		/* ready Queue */
		cur = list_first_entry(&readyQueue, process_t, ready);
		cur->isResponse = 1;

		/* idle Process */
		if(cur->pid == 100)
		{
			list_add(&cur->idleClass, &idleClassQueue);
			list_del(&cur->ready);

			checkProcessArrival(&cpuClock);
			addtoreadyQueue(&cpuClock);

			while(list_empty(&readyQueue))
			{
				cpuClock++;
				idleClock++;
				checkProcessArrival(&cpuClock);
				addtoreadyQueue(&cpuClock);
			}
			
			sw = list_first_entry(&readyQueue, process_t, ready);

			cpuClock += 5;
			idleClock += 5;

			printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", cpuClock, cur->pid, sw->pid);

			/* first load of process */
			if(!sw->isResponse)
			{
				sw->responseTime = cpuClock - sw->arrival_time;
				sw->isResponse = 1;
			}

			continue;
		}
		
		/* realtime Process */
		if(cur->isRealtime)
		{
			while(cur->progress < cur->len)
			{
				cpuClock++;
				cur->progress += 1;

				if(cur->progress == cur->len)
				{
					cur->programCounter += 1;
					performedjobCount++;
					printf("%04d CPU: Process is terminated\tPID:%03d\tPC:%03d\n", cpuClock, cur->pid, cur->programCounter);
					cur->waitingTime = cpuClock - cur->arrival_time - cur->len;
					list_del(&cur->ready);
				}

				checkProcessArrival(&cpuClock);
				addtoreadyQueue(&cpuClock);
			}

		}

		/* normal Process */
		if(!cur->isRealtime)
		{
			list_for_each_entry_safe(cur, next, &readyQueue, ready)
			{
				cpuClock++;
				cur->progress += 1;
				cur->timeSlice += 1;

				if(cur->progress == cur->len)
				{
					cur->programCounter += 1;
					performedjobCount++;
					printf("%04d CPU: Process is terminated\tPID:%03d\tPC:%03d\n", cpuClock, cur->pid, cur->programCounter);
					cur->waitingTime = cpuClock - cur->arrival_time - cur->len;
					list_del(&cur->ready);
				}

				checkProcessArrival(&cpuClock);
				addtoreadyQueue(&cpuClock);

				/* realtime process loaded*/
				if(!list_empty(&realtimeClassQueue))
				{
					sw = list_first_entry(&realtimeClassQueue, process_t, realtimeClass);

					list_add(&sw->ready, &readyQueue);
					list_del(&sw->realtimeClass);

					break;
				}

				/* context switching (round) */
				if(cur->timeSlice == 50)
				{
					/* next process in round */
					if(next != NULL)
					{
						cpuClock += 5;
						idleClock += 5;

						printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", cpuClock, cur->pid, next->pid);
						
						/* first load of process */
						if(!next->isResponse)
						{
							next->responseTime = cpuClock - next->arrival_time;
							next->isResponse = 1;
						}
					}

					/* round end */
					if(next == NULL)
					{
						printf("%04d CPU: ROUND ENDS. Recharge the Timeslices\n", cpuClock);

						sw = list_first_entry(&readyQueue, process_t, ready);

						/* process not change */
						if(cur == sw)
						{
							printf("%04d CPU: Not Switched\tPID: %03d\n", cpuClock, cur->pid);
						}

						else
						{
							cpuClock += 5;
							idleClock += 5;

							printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", cpuClock, cur->pid, sw->pid);
						}
					}
				}
			}
		}

		/* context switching */
		if(performedjobCount < jobCount)
		{
			checkProcessArrival(&cpuClock);
			addtoreadyQueue(&cpuClock);
			
			sw = list_first_entry(&readyQueue, process_t, ready);

			cpuClock += 5;
			idleClock += 5;

			printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", cpuClock, cur->pid, sw->pid);
			
			/* first load of process */
			if(!sw->isResponse)
			{
				sw->responseTime = cpuClock - sw->arrival_time;
				sw->isResponse = 1;
			}
		}

		if(performedjobCount == jobCount)
		{
			break;
		}
	}
	
	printf("PID: 100\tARRIVAL: 000\tCODESIZE: 002\tWAITING: 000\tRESPONSE: 000\n");

	/* calculate average waitingTime & responseTime */
	list_for_each_entry_safe_reverse(cur, next, &jobQueue, job)
	{
		printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\tWAITING: %03d\tRESPONSE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes, cur->waitingTime, cur->responseTime);
		totalWaitingTime += cur->waitingTime;
		totalResposeTime += cur->responseTime;
	}

	printf("*** TOTAL CLOCKS: %04d IDLE: %04d UTIL: %2.2f%% WAIT: %2.2f RESPONSE: %2.2f\n", \
	cpuClock, idleClock, (cpuClock-idleClock)*100/(float)cpuClock, (double)totalWaitingTime/jobCount, (double)totalResposeTime/jobCount);
}

int main(int argc, char* argv[])
{
    processInfo_t inputProcessInfo;
	process_t *cur, *next;

    while(fread(&inputProcessInfo, sizeof(processInfo_t), 1, stdin) == 1)
	{
        //printf("%d %d %d\n", inputProcessInfo.pid, inputProcessInfo.arrival_time, inputProcessInfo.code_bytes);

		process_t *processPtr = (process_t *) malloc(sizeof(process_t));

		INIT_LIST_HEAD(&processPtr->ready);
		
		/* inputProcessInfo ==> processPtr */
		processPtr->pid = inputProcessInfo.pid;
		processPtr->arrival_time = inputProcessInfo.arrival_time;
		processPtr->code_bytes = inputProcessInfo.code_bytes;
		processPtr->isProcessArrival = 0;
		processPtr->programCounter = 0;
		processPtr->progress = 0;
		processPtr->timeSlice = 0;
		processPtr->responseTime = 0;
		processPtr->isResponse = 0;
		processPtr->waitingTime = 0;

		if(processPtr->pid >= 80 && processPtr->pid < 100)
			processPtr->isRealtime = 1;

		/* fread code tuple */
		fread(&processPtr->op, sizeof(unsigned char), 1, stdin);
		fread(&processPtr->len, sizeof(unsigned char), 1, stdin);

		list_add_tail(&processPtr->job, &jobQueue);
		jobCount++;
	}

	processSimulator();

	/* free memory allocation */
	list_for_each_entry_safe(cur, next, &jobQueue, job)
	{
		list_del(&cur->job);
		free(cur);
	}

	return 0;
}