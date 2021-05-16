#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef struct{
    unsigned char op;
    unsigned char len;
} code_tuple;

typedef struct{
    int pid;
    int arrival_time;
    int code_bytes;
} process;


int main(int argc,char* argv[]){
    process *cur;
    code_tuple *codes;
    int i,j, num_processes, prev_arrival=0;

    srand((unsigned int)time(NULL));

    num_processes = 2 + rand()%4;

    cur = (process*) malloc(sizeof(process));

    for(i=0; i<num_processes; i++) {        
        cur->pid = i;        
        cur->arrival_time = prev_arrival;
        prev_arrival += rand()%10;
        
        //cur->code_bytes = 2+rand()%5*2;
        cur->code_bytes = 2;

        codes = (code_tuple*) malloc(cur->code_bytes);

        codes->op = 0; 
        codes->len = 35+rand()%200;

        fwrite(cur, sizeof(process),1,stdout);
        fwrite(codes, cur->code_bytes,1,stdout);

        free(codes);
    }

    free(cur);

    return 0;
}