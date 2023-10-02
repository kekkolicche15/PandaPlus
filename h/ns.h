#ifndef NS_H_INCLUDED
#define NS_H_INCLUDED

#include <ash.h>

void initNamespaces();
nsd_PTR getNamespace(pcb_PTR, int);
int addNamespace(pcb_PTR, nsd_PTR);
nsd_PTR allocNamespace(int);
void freeNamespace(nsd_PTR);

#endif