#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include "mpu.h"
#include "gpu.h"
#include "dma.h"
#include "gpuprimitives.h"
#include "gputextures.h"
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
    unsigned short i1, i2, i3, i4, i5, i6, i7;
    void *p1, *p2, *p3, *p4, *p5, *p6, *p7;
};

typedef struct _queueEntry QueueRequest;

static LinkedList QueueList = { NULL, NULL, 0 };

static short int queueActive = 1;
static short int queueProcessing = 1;

#ifdef GPU_MODE_QUEUE

void RemoveGPUrequest(QueueRequest *);

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
            switch (request->cmd)
            {
                case CMD_DestroyScreen:
                    DestroyScreen(request->i1);
                break;

                case CMD_SetColor:
                    SetColor(request->i1, request->i2);
                break;

                case CMD_SetPixel:
                    SetPixel(request->i1, request->i2, request->i3);
                break;

                case CMD_DrawLine:
                    DrawLine(request->i1, request->i2, request->i3, request->i4, request->i5);
                break;

                case CMD_DestroyTexture:
                    DestroyTexture(request->i1);
                break;

                case CMD_SetTextureTransparency:
                    SetTextureTransparency(request->i1, request->i2, request->i3);
                break;

                case CMD_LoadTexture:
                    LoadTexture(request->i1, request->i2, request->i3);
                break;

                case CMD_RenderTexture:
                    // fprintf(stderr, "ProcessGPUqueue : RenderTexture %d %d %d %d\n",
                    //     ((Rect*)(request->p3))->x, ((Rect*)(request->p3))->y, ((Rect*)(request->p3))->w, ((Rect*)(request->p3))->h);
                    QRenderTexture(request->p1, request->p2, request->i1, request->i2, request->p3);
                break;

                default:
                    fprintf(stderr, "GPU queue process : unhandled command %d\n", request->cmd);
                break;
            }
            RemoveGPUrequest(request);
        }

    	pthread_mutex_lock(&condLock);
	    pthread_cond_wait(&GPUcond, &condLock);
        pthread_mutex_unlock(&condLock);
    }
}

void QueueGPUrequest(unsigned char cmd, ...)
{
    va_list       ArgumentPointer;
    QueueRequest *newGPUrequest = malloc(sizeof(QueueRequest));

    newGPUrequest->cmd = cmd;

    switch (cmd)
    {
        case CMD_DestroyScreen: // 1 short int - Screen ID
            va_start(ArgumentPointer, 1);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
        break;
        
        case CMD_SetColor: // 2 short ints - Screen ID, Color
            va_start(ArgumentPointer, 2);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
            newGPUrequest->i2 = va_arg(ArgumentPointer, int);
        break;
        
        case CMD_SetPixel: // 3 short ints - Screen ID, x, y
            va_start(ArgumentPointer, 3);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
            newGPUrequest->i2 = va_arg(ArgumentPointer, int);
            newGPUrequest->i3 = va_arg(ArgumentPointer, int);
        break;
        
        case CMD_DrawLine: // 5 short ints - Screen ID, x1, y1, x2, y2
            va_start(ArgumentPointer, 5);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
            newGPUrequest->i2 = va_arg(ArgumentPointer, int);
            newGPUrequest->i3 = va_arg(ArgumentPointer, int);
            newGPUrequest->i4 = va_arg(ArgumentPointer, int);
            newGPUrequest->i5 = va_arg(ArgumentPointer, int);
        break;
        
        case CMD_DestroyTexture: // 1 short int1 - Texture ID
            va_start(ArgumentPointer, 1);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
        break;

        case CMD_SetTextureTransparency: // 3 short ints - Texture ID, onoff, color
            va_start(ArgumentPointer, 3);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
            newGPUrequest->i2 = va_arg(ArgumentPointer, int);
            newGPUrequest->i3 = va_arg(ArgumentPointer, int);
        break;
        
        case CMD_LoadTexture: // 3 short ints - Screen ID, Texture ID, memaddr
            va_start(ArgumentPointer, 3);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
            newGPUrequest->i2 = va_arg(ArgumentPointer, int);
            newGPUrequest->i3 = va_arg(ArgumentPointer, int);
        break;
        
        case CMD_RenderTexture: // 2 refs + 2 short ints + 1 Ref - Screen*, Texture*, screenx, screeny, Texture_Rectangle*
            va_start(ArgumentPointer, 5);
            newGPUrequest->p1 = va_arg(ArgumentPointer, void*);
            newGPUrequest->p2 = va_arg(ArgumentPointer, void*);
            newGPUrequest->i1 = va_arg(ArgumentPointer, int);
            newGPUrequest->i2 = va_arg(ArgumentPointer, int);
            newGPUrequest->p3 = va_arg(ArgumentPointer, void*);
            // fprintf(stderr, "QueueGPUrequest : RenderTexture %d %d %d %d\n",
            //     ((Rect*)(newGPUrequest->p3))->x, ((Rect*)(newGPUrequest->p3))->y, ((Rect*)(newGPUrequest->p3))->w, ((Rect*)(newGPUrequest->p3))->h);
        break;

        default:
            fprintf(stderr, "GPU queue add : unhandled cmd %d\n", cmd);
        break;
    }

    va_end(ArgumentPointer);
    newGPUrequest->nextEntry = NULL;

    pthread_mutex_lock(&GPUlock);
    AppendListItem(&QueueList, (LinkedListItem*)newGPUrequest);
    pthread_mutex_unlock(&GPUlock);

    // Only wake the GPU thread if it is asleep
    pthread_cond_signal(&GPUcond);
}

void RemoveGPUrequest(QueueRequest *queueRequest)
{
    if (queueRequest == NULL) return;
    pthread_mutex_lock(&GPUlock);
    QueueRequest *request = (QueueRequest*)RemoveListHead(&QueueList);
    pthread_mutex_unlock(&GPUlock);
    if (queueRequest != request) { fprintf(stderr, "Queue head not = request\n");}
    free(queueRequest);
}

void StartGPUQueue()
{
    struct sigaction action;

    /* set up signal handlers for SIGINT & SIGUSR1 */
    sigemptyset(&action.sa_mask);
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

void ReportQueue()
{
    fprintf(stderr, "GPU Queue depth %d\n", QueueList.itemCnt);
}

void GetQueueLen(ushort lenref)
{
    ushort len;

    len = QueueList.itemCnt;

    WriteCoCoInt(lenref, len);
}
