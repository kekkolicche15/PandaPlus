#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED

#include <pandos_const.h>
#include <pandos_types.h>
#include <types.h>

#define HIDDEN static

void initPcbs ();
void freePcb (pcb_PTR);
pcb_PTR allocPcb ();
void mkEmptyProcQ (struct list_head *);
int emptyProcQ (struct list_head *);
void insertProcQ (struct list_head *, pcb_PTR);
pcb_PTR headProcQ (struct list_head *);
pcb_PTR removeProcQ (struct list_head *);
pcb_PTR outProcQ (struct list_head *, pcb_PTR);

int emptyChild (pcb_PTR);
void insertChild (pcb_PTR, pcb_PTR);
pcb_PTR removeChild (pcb_PTR);
pcb_PTR outChild (pcb_PTR);

pcb_PTR getProcessByPid (pid_t pid);

#endif