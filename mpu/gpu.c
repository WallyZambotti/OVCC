#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "mpu.h"
#include "gpu.h"
#include "gpuprimitives.h"
#include "linkedlists.h"

static pthread_t GPUthread;
static pthread_mutex_t GPUlock;
static pthread_cond_t GPUcond;
static pthread_mutex_t condLock;

struct _queueEntry
{
    unsigned int id;
    struct _queueEntry *nextEntry;
    unsigned char cmd;
    unsigned short p1, p2, p3, p4, p5;
};

typedef struct _queueEntry QueueRequest;

static LinkedList QueueList = { NULL, NULL, 0 };

static short int queueActive = 1;
static short int queueProcessing = 1;

#ifdef GPU_MODE_QUEUE

static void GPUsigHandler(int signo)
{
    // write(0, "!", 1);
}

void *ProcessGPUqueue(void *ptr)
{
    int sigdummy;
    sigset_t sigmask;
    QueueRequest gpuqueue;

    sigemptyset(&sigmask);               /* to zero out all bits */
    sigaddset(&sigmask, SIGUSR1);        /* to unblock SIGUSR1 */  
    pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);

    // fprintf(stderr, "GPU queue alive\n");

    while(queueActive)
    {
        while(QueueList.ListHead != NULL)
        {
            QueueRequest *request = (QueueRequest*)(QueueList.ListHead);
			if (request->cmd == 65) fprintf(stderr, "Found DestroyScreen\n");
            switch (request->cmd)
            {
                case CMD_NewScreen:
                    NewScreen(request->p1, request->p2, request->p3, request->p4, request->p5);
                break;

                case CMD_DestroyScreen:
			        //fprintf(stderr, "Calling DestroyScreen\n");
                    DestroyScreen(request->p1);
                break;

                case CMD_SetColor:
                    SetColor(request->p1, request->p2);
                break;

                case CMD_SetPixel:
                    SetPixel(request->p1, request->p2, request->p3);
                break;

                case CMD_DrawLine:
                    DrawLine(request->p1, request->p2, request->p3, request->p4, request->p5);
                break;

                default:
                    fprintf(stderr, "GPU : uknown command %d\n", request->cmd);
                break;
            }
            RemoveGPUrequest(request);
        }

        //write(0, ">", 1);
    	pthread_mutex_lock(&condLock);
	    pthread_cond_wait(&GPUcond, &condLock);
        pthread_mutex_unlock(&condLock);
        //write(0, "<", 1);
    }

    // write(2, "GPU stopped\n", 12);
}

void QueueGPUrequest(unsigned char cmd, unsigned short p1, unsigned short p2, unsigned short p3, unsigned short p4, unsigned short p5)
{
    QueueRequest *newGPUrequest = malloc(sizeof(QueueRequest));

    //if (cmd == 65) { fprintf(stderr, "Queue received a Destroy Screen\n");}

    newGPUrequest->cmd = cmd;
    newGPUrequest->p1 = p1;
    newGPUrequest->p2 = p2;
    newGPUrequest->p3 = p3;
    newGPUrequest->p4 = p4;
    newGPUrequest->p5 = p5;
    newGPUrequest->nextEntry = NULL;

    pthread_mutex_lock(&GPUlock);
    AppendListItem(&QueueList, (LinkedListItem*)newGPUrequest);
    pthread_mutex_unlock(&GPUlock);

    // Only wake the GPU thread if it is asleep
    pthread_cond_signal(&GPUcond);

    if (cmd == 65) 
    {
        fprintf(stderr, "Queued 65 %d\n", QueueList.itemCnt);
    }
}

void RemoveGPUrequest(QueueRequest *queueRequest)
{
    if (queueRequest == NULL) return;
    pthread_mutex_lock(&GPUlock);
    RemoveListHead(&QueueList);
    pthread_mutex_unlock(&GPUlock);
    free(queueRequest);
    //write(0, "-", 1);
}

void StartGPUQueue()
{
    struct sigaction action;

    /* set up signal handlers for SIGINT & SIGUSR1 */
    action.sa_flags = 0;
    action.sa_handler = GPUsigHandler;
    sigaction(SIGUSR1, &action, NULL);

    pthread_create(&GPUthread, NULL, ProcessGPUqueue, NULL);

    if (GPUthread == 0) 
    {
        fprintf(stderr, "Cannot start GPU thread\n");
    }
}

void StopGPUqueue()
{
    // write(2, "GPU queue stopping\n", 19);
    queueActive = 0;
    pthread_cond_signal(&GPUcond);
}
#endif
