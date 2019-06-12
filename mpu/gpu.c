#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "mpu.h"

static pthread_t GPUthread;
static pthread_mutex_t GPUlock;
static pthread_cond_t GPUcond;
static pthread_mutex_t condLock;

struct _queueEntry
{
    unsigned char cmd;
    unsigned short p1, p2, p3, p4;
    struct _queueEntry *nextEntry;
};

typedef struct _queueEntry QueueRequest;

static QueueRequest *GPUqueue = NULL;
static QueueRequest *GPUqueueEnd = NULL;

static short int queueActive = 1;
static int queueDepth = 0;
static short int queueProcessing = 1;

static unsigned short ScreenAddress;
static unsigned short ScreenWidth;
static unsigned short ScreenPitch;
static unsigned short ScreenHeight;
static unsigned short ScreenEnd;
static unsigned short BitsPerPixel;
static unsigned short PixelsPerByte;
static short PPBshift;
static unsigned short Color;
static unsigned char taskmmubank[8];

unsigned char pixelmasks[4][8] = 
{
    {
        0xff
    },
    {
        0xf0, 0x0f
    },
    {
        0xC0, 0x30, 0x0c, 0x03
    },
    {
        0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
    }
};

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
        while(GPUqueue != NULL)
        {
            switch (GPUqueue->cmd)
            {
                case CMD_SetScreen:
                    SetScreen(GPUqueue->p1, GPUqueue->p2, GPUqueue->p3, GPUqueue->p4);
                break;

                case CMD_SetColor:
                    SetColor(GPUqueue->p1);
                break;

                case CMD_SetPixel:
                    SetPixel(GPUqueue->p1, GPUqueue->p2);
                break;

                case CMD_DrawLine:
                    DrawLine(GPUqueue->p1, GPUqueue->p2, GPUqueue->p3, GPUqueue->p4);
                break;

                default:
                    fprintf(stderr, "GPU : uknown command %d\n", GPUqueue->cmd);
                break;
            }
            RemoveGPUrequest(GPUqueue);
        }

        //write(0, ">", 1);
    	pthread_mutex_lock(&condLock);
	    pthread_cond_wait(&GPUcond, &condLock);
        pthread_mutex_unlock(&condLock);
        //write(0, "<", 1);
    }

    // write(2, "GPU stopped\n", 12);
}

void QueueGPUrequest(unsigned char cmd, unsigned short p1, unsigned short p2, unsigned short p3, unsigned short p4)
{
    QueueRequest *newGPUrequest = malloc(sizeof(QueueRequest));

    newGPUrequest->cmd = cmd;
    newGPUrequest->p1 = p1;
    newGPUrequest->p2 = p2;
    newGPUrequest->p3 = p3;
    newGPUrequest->p4 = p4;
    newGPUrequest->nextEntry = NULL;

    pthread_mutex_lock(&GPUlock);

    if (GPUqueue == NULL)
    {
        GPUqueue = GPUqueueEnd = newGPUrequest;
    }
    else
    {
        GPUqueueEnd->nextEntry = newGPUrequest;
        GPUqueueEnd = newGPUrequest;
    }

    queueDepth++;
    pthread_mutex_unlock(&GPUlock);

    // Only wake the GPU thread if it is asleep
    pthread_cond_signal(&GPUcond);
    // write(0, "+", 1);
}

void RemoveGPUrequest(QueueRequest *queueRequest)
{
    if (queueRequest == NULL)
    {
        write(2, "Remove null\n", 12);
        return;
    }
    if (queueDepth == 0)
    {
        write(2, "Remove empty\n", 12);
        return;
    }
    pthread_mutex_lock(&GPUlock);
    GPUqueue = queueRequest->nextEntry;
    queueDepth--;
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

void SetScreen(unsigned short address, unsigned short width, unsigned short height, unsigned short bitsperpixel)
{
    // fprintf(stderr, "SetScreen %x %d %d %d\n", address, width, height, bitsperpixel);

    if (bitsperpixel == 0) return ;

    ScreenAddress = address;
    ScreenWidth  = width;
    ScreenHeight = height;
    BitsPerPixel = bitsperpixel;
    PixelsPerByte = (8 / BitsPerPixel);
    ScreenPitch = width / PixelsPerByte;
    ScreenEnd = ScreenAddress + (ScreenPitch * ScreenHeight);

    PPBshift = -1;
    for(unsigned short int PPB = PixelsPerByte ; PPB ; PPB=PPB>>1) { PPBshift++; }

    // Interegate the taskmmubank and record the current process taskmmubank map

    unsigned char tr = (MemRead(0xFF91) & 0x01)<<3;
    unsigned short addr = 0xFFA0 + tr;

    for(short i = 0 ; i <  8 ; i++)
    {
        taskmmubank[i] = MemRead(addr++);
    }

    // fprintf(stderr, "SetScreen %d %d %d %d\n", PixelsPerByte, ScreenPitch, ScreenEnd, PPBshift);
}

void SetColor(unsigned short color)
{
    // fprintf(stderr, "SetColor %d\n", color);
    Color = color;
}

void SetPixel(unsigned short x, unsigned short y)
{
    // fprintf(stderr, "SetPixel %d %d\n", x, y);
    unsigned short pixaddr = ScreenAddress + (y * ScreenPitch) + (x>>PPBshift);
    if (pixaddr < ScreenAddress || pixaddr > ScreenEnd) 
    {
        // write(0, "?", 1);
        return;
    }
    unsigned short bankidx = pixaddr>>13;
    unsigned short xmodPPB = x%PixelsPerByte;
    unsigned char pixmask = pixelmasks[PPBshift][xmodPPB];
    // unsigned char pixelbyte = MemRead(pixaddr) & (pixmask^0xff);
    unsigned char pixelbyte = MmuRead(taskmmubank[bankidx], pixaddr) & (pixmask^0xff);
    pixelbyte |= Color<<(BitsPerPixel*(PixelsPerByte-xmodPPB-1));
    // MemWrite(pixelbyte, pixaddr);
    MmuWrite(pixelbyte, taskmmubank[bankidx], pixaddr);
}

void DrawLine(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
    // fprintf(stderr, "DrawLine %x %d %d %d\n", x1, y1, x2, y2);
	int dx, dy;
	int inc1, inc2;
	int x, y, d;
	int xEnd, yEnd;
	int xDir, yDir;
	
	dx = abs(x2 - x1);
	dy = abs(y2 - y1);

	if (dy <= dx) 
    {
		d = dy*2 - dx;
		inc1 = dy*2;
		inc2 = (dy-dx)*2;
		if (x1 > x2) { x = x2; y = y2; yDir = -1; xEnd = x1; } else { x = x1; y = y1; yDir = 1; xEnd = x2; }
		SetPixel(x, y);

		if (((y2-y1)*yDir) > 0) {
			while (x < xEnd) 
            {
				x++;
				if (d < 0) { d += inc1; } else { y++; d += inc2; }
				SetPixel(x, y);
			}
		} 
        else 
        {
			while (x < xEnd) 
            {
				x++;
				if (d < 0) { d += inc1; } else { y--; d += inc2; }
				SetPixel(x, y);
			}
		}		
	} 
    else 
    {
		d = dx*2 - dy;
		inc1 = dx*2;
		inc2 = (dx-dy)*2;
		if (y1 > y2) { y = y2; x = x2; yEnd = y1; xDir = -1; } 
        else { y = y1; x = x1; yEnd = y2; xDir = 1; }
		SetPixel(x, y);

		if (((x2-x1)*xDir) > 0) 
        {
			while (y < yEnd) 
            {
				y++;
				if (d < 0) { d += inc1; } else { x++; d += inc2; }
				SetPixel(x, y);
			}
		} 
        else 
        {
			while (y < yEnd) 
            {
				y++;
				if (d < 0) { d += inc1; } else { x--; d += inc2; }
				SetPixel(x, y);
			}
		}
	}
}