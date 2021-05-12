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
LIST_HEAD(job_queue);
LIST_HEAD(ready_queue);
LIST_HEAD(wait_queue);

int jobCount = 0;
int isIdleCheck = 0;

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
	int programCounter;

	operations_t *operations;

    struct list_head job;
    struct list_head ready;
    struct list_head wait; 
} process_t;

void addIdleProcess()
{		
	process_t *processPtr = (process_t *) malloc(sizeof(process_t));
	INIT_LIST_HEAD(&processPtr->job);
	
	processPtr->pid = 100;
	processPtr->arrival_time = 0;
	processPtr->code_bytes = 2;
	processPtr->programCounter = 0;

	processPtr->operations = (operations_t *) malloc(sizeof(operations_t) * ((processPtr->code_bytes)/2));
	processPtr->operations[0].op = 0xff;
	processPtr->operations[0].len = 0;

	list_add_tail(&processPtr->job, &job_queue);
}

void addIdleReadyQueue()
{
	process_t *cur, *next;
	
	list_for_each_entry_safe(cur, next, &job_queue, job)
	{
		if(cur->pid == 100)
		{
			list_add_tail(&cur->ready, &ready_queue);
			break;
		}
	}
}

void checkReadyQueue(int *cpuClock)
{
	process_t *cur, *next;

	list_for_each_entry_safe(cur, next, &job_queue, job)
	{
		/* add to ready queue */
		if(*cpuClock == cur->arrival_time)
		{
			if(cur->pid != 100)
			{
				printf("%04d CPU: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\tPC: %03d\n", *cpuClock, cur->pid, cur->arrival_time, cur->code_bytes, cur->programCounter);

				list_add_tail(&cur->ready, &ready_queue);
			}
		}
		
	}
}

void checkWaitQueue(int *cpuClock, int *performedjobCount)
{
	process_t *cur, *next;

	list_for_each_entry_safe(cur, next, &wait_queue, wait)
	{
		if(*cpuClock >= cur->endIOClock)
		{
			printf("%04d IO : COMPLETED! PID: %03d\tIOTIME: %03d\tPC: %03d\n", *cpuClock, cur->pid, cur->endIOClock, cur->programCounter);
			cur->programCounter += 1;

			if(cur->programCounter == (cur->code_bytes)/2)
			{
				*performedjobCount += 1;
			}

			list_add_tail(&cur->ready, &ready_queue);
			list_del(&cur->wait);
		}
	}
}

void processSimulator()
{
	int cpuClock = 0;
	int idleClock = 0;
	int performedjobCount = 0;

	process_t *cur, *next;
	process_t *cur1, *next1;

	while(1)
	{
		checkReadyQueue(&cpuClock);
		
		/* add Idle Process */
		if(cpuClock == 0 && isIdleCheck == 0)
		{
			addIdleProcess();
			printf("0000 CPU: Loaded PID: 100\tArrival: 000\tCodesize: 002\tPC: 000\n");
			isIdleCheck = 1;
		}

		list_for_each_entry_safe(cur, next, &ready_queue, ready)
		{
			while(cur->programCounter < (cur->code_bytes)/2)
			{
				/* CPU operation */
				if(cur->operations[cur->programCounter].op == 0)
				{
					//printf("%04d CPU: OP_CPU START len: %03d ends at: %04d\n", cpuClock, cur->operations[cur->programCounter].len, cpuClock+cur->operations[cur->programCounter].len);

					for(int i = 0; i < cur->operations[cur->programCounter].len; i++)
					{
						cpuClock++;
						checkReadyQueue(&cpuClock);
						checkWaitQueue(&cpuClock, &performedjobCount);
					}

					//printf("%04d Increase PC PID: %03d PC: %03d\n", cpuClock, cur->pid, cur->programCounter);
					cur->programCounter += 1;
					
					if(cur->programCounter == (cur->code_bytes)/2)
					{
						performedjobCount++;
					}
				}				

				/* IO operation */
				else if(cur->operations[cur->programCounter].op == 1)
				{
					checkReadyQueue(&cpuClock);
					checkWaitQueue(&cpuClock, &performedjobCount);

					//printf("%04d CPU: OP_IO START len: %03d ends at: %04d\n", cpuClock, cur->operations[cur->programCounter].len, cpuClock+cur->operations[cur->programCounter].len);
					
					cur->endIOClock = cpuClock + cur->operations[cur->programCounter].len;
					cpuClock++;

					cpuClock += 10;
					idleClock += 10;

					list_add_tail(&cur->wait, &wait_queue);
					list_del(&cur->ready);

					/* context switching */
					if(performedjobCount < jobCount && list_empty(&ready_queue) != 1)
					{
						list_for_each_entry_safe(cur1, next1, &ready_queue, ready)
						{	
							printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", cpuClock, cur->pid, cur1->pid);
							break;
						}
					}

					break;
				}
				
				/* IDLE operation */
				else if(cur->pid == 100)
				{
					list_del(&cur->ready);
					checkReadyQueue(&cpuClock);
					checkWaitQueue(&cpuClock, &performedjobCount);

					while(list_empty(&ready_queue) == 1)
					{
						cpuClock++;
						idleClock++;
						checkReadyQueue(&cpuClock);
						checkWaitQueue(&cpuClock, &performedjobCount);
					}
					
					cpuClock += 10;
					idleClock += 10;

					list_for_each_entry_safe(cur1, next1, &ready_queue, ready)
					{	
						cpuClock++;
						idleClock++;
						printf("%04d CPU: Switched\tfrom: 100\tto: %03d\n", cpuClock, cur1->pid);
						break;
					}

					break;
				}

			}

			/* context switching */
			if(performedjobCount < jobCount && cur->programCounter == (cur->code_bytes)/2)
			{
				cpuClock += 10;
				idleClock += 10;

				list_del(&cur->ready);

				/* list empty : 1, not empty : 0 */
				if(performedjobCount < jobCount && list_empty(&ready_queue) == 1 && list_empty(&wait_queue) != 1)
				{
					addIdleReadyQueue();

					printf("%04d CPU: Switched\tfrom: %03d\tto: 100\n", cpuClock, cur->pid);
				}

				else
				{
					list_for_each_entry_safe(cur1, next1, &ready_queue, ready)
					{	
						printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", cpuClock, cur->pid, cur1->pid);
						break;
					}
				}

			}

			/* list empty : 1 */
			if(performedjobCount < jobCount && list_empty(&ready_queue) == 1 && list_empty(&wait_queue) != 1)
			{
				addIdleReadyQueue();

				printf("%04d CPU: Switched\tfrom: %03d\tto: 100\n", cpuClock, cur->pid);
			}
		}

		if(performedjobCount == jobCount)
		{
			break;
		}
	}

	printf("*** TOTAL CLOCKS: %04d IDLE: %04d UTIL: %2.2f%%\n", cpuClock, idleClock, (cpuClock-idleClock)*100/(double)cpuClock);
}

int main(int argc, char* argv[])
{
    processInfo_t inputProcessInfo;
	process_t *cur, *next;

    while(fread(&inputProcessInfo, sizeof(processInfo_t), 1, stdin) == 1)
	{
        //fprintf(stdout, "%d %d %d\n", inputProcessInfo.pid, inputProcessInfo.arrival_time, inputProcessInfo.code_bytes);

		process_t *processPtr = (process_t *) malloc(sizeof(process_t));

		INIT_LIST_HEAD(&processPtr->job);
		INIT_LIST_HEAD(&processPtr->ready);
		INIT_LIST_HEAD(&processPtr->wait);
		
		/* inputProcessInfo ==> processPtr */
		processPtr->pid = inputProcessInfo.pid;
		processPtr->arrival_time = inputProcessInfo.arrival_time;
		processPtr->code_bytes = inputProcessInfo.code_bytes;
		processPtr->endIOClock = 0;
		processPtr->programCounter = 0;

		/* dynamic allocation codeInfo size * code_bytes/2 */
		processPtr->operations = (operations_t *) malloc(sizeof(operations_t) * ((processPtr->code_bytes)/2));

		/* fread codeInfo 1 byte for each element */
		for(int i = 0; i < ((processPtr->code_bytes)/2); i++)
		{
			if(fread(&processPtr->operations[i].op, sizeof(unsigned char), 1, stdin) != 1)
                fprintf(stdout, "operations error1\n");    
            
            if(fread(&processPtr->operations[i].len, sizeof(unsigned char), 1, stdin) != 1)
                fprintf(stdout, "operations error2\n");   

            //fprintf(stdout, "%d %d\n", processPtr->operations[i].op, processPtr->operations[i].len);
		}

		list_add_tail(&processPtr->job, &job_queue);

		jobCount++;
	}

	/* assignment 1-1 job traverse reverse */
	/*
	list_for_each_entry_safe_reverse(cur, next, &job_queue, job)
	{
		printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes);

		for(int i = 0; i < ((cur->code_bytes)/2); i++)
		{
			printf("%d %d\n", cur->operations[i].op, cur->operations[i].len);
		}

		list_del(&cur->job);
		free(cur->operations);
		free(cur);
	}
	*/

	/* assignment 1-3 */
	processSimulator();

	/* free memory allocation */
	list_for_each_entry_safe_reverse(cur, next, &job_queue, job)
	{
		list_del(&cur->job);
		free(cur->operations);
		free(cur);
	}

	return 0;
}