#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED

#include <ns.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>

#define EXCEPTION_STATE ((state_t *)BIOSDATAPAGE)
#define CAUSE_GET_IP  (getCAUSE() & CAUSE_IP_MASK)
#define GPR(field) (((state_t *)BIOSDATAPAGE)->reg_##field)


extern pcb_PTR currentProcess;
extern struct list_head readyQueue;
extern unsigned int processCount, softBlockedCount;
extern int itSemaphore, deviceSemaphore[EXTERNAL_DEVS_NO];
extern cpu_t lastTOD;
cpu_t getTimeElapsed ();

void exceptionHandler (), interruptHandler (), scheduler (void), memcpy (void *, const void *, unsigned int);
int *getDeviceSemaphore (unsigned int address);
pcb_PTR verhogen (int *), passeren (int *);

#endif
