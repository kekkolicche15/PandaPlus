#ifndef ASH_H_INCLUDED
#define ASH_H_INCLUDED

#include <pcb.h>
#include <hash.h>
#include <hashtable.h>


int insertBlocked(int *, pcb_PTR);
pcb_PTR removeBlocked(int *);
pcb_PTR outBlocked(pcb_PTR);
pcb_PTR headBlocked(int *);
void initASH();

#endif