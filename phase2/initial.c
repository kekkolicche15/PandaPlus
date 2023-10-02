#include <lib.h>

pcb_PTR currentProcess;
struct list_head readyQueue;
unsigned int processCount, softBlockedCount;
int itSemaphore, deviceSemaphore[EXTERNAL_DEVS_NO];
cpu_t lastTOD;

extern void test (), uTLB_RefillHandler ();

/**
 * @brief Copia una sequenza di dati dalla sorgente alla destinazione
 *
 * @param dest Puntatore alla destinazione
 * @param src Puntatore alla sorgente
 * @param n Numero di byte da copiare
 */
void
memcpy (void *dest, const void *src, unsigned int n)
{
  for (unsigned int i = 0; i < n; i++)
    ((char *)dest)[i] = ((char *)src)[i];
}

/**
 * @brief Restituisce il puntatore al semaforo del (sotto-)dispositivo indicato
 * da commandAddr
 *
 * @param commandAddr Indirizzo del registro del dispositivo.
 * @return Puntatore al semaforo del dispositivo indicato da commandAddr.
 */
int *
getDeviceSemaphore (unsigned int commandAddr)
{
  unsigned int offset = (commandAddr & 0x8) ? 0 : N_DEV_PER_IL;
  unsigned int semIndex
      = ((commandAddr - DEV_REG_START) / DEV_REG_SIZE) + offset;
  return &deviceSemaphore[semIndex];
}

/**
 * @brief Restituisce il tempo trascorso dalla precedente chiamata a questa
 * funzione.
 *
 * @return Il tempo trascorso dalla precedente chiamata in unità di tempo della
 * CPU.
 */
cpu_t
getTimeElapsed ()
{
  cpu_t currTOD;
  STCK (currTOD);
  cpu_t getTimeElapsed = currTOD - lastTOD;
  STCK (lastTOD);
  return getTimeElapsed;
}

int
main ()
{
  passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
  passupvector->tlb_refill_handler = (unsigned int)uTLB_RefillHandler;
  passupvector->exception_stackPtr = (unsigned int)KERNELSTACK;
  passupvector->tlb_refill_stackPtr = (unsigned int)KERNELSTACK;
  passupvector->exception_handler = (unsigned int)exceptionHandler;
  initPcbs ();
  initASH ();
  initNamespaces ();
  processCount = 0;
  softBlockedCount = 0;
  mkEmptyProcQ (&readyQueue);
  currentProcess = NULL;
  itSemaphore = 0;
  for (int i = 0; i < EXTERNAL_DEVS_NO; i++)
    deviceSemaphore[i] = 0;
  LDIT (PSECOND); // impostare l'interval timer a 100000 microsecond
  pcb_PTR init = allocPcb ();
  addNamespace (init, allocNamespace (NS_PID));
  RAMTOP (init->p_s.reg_sp);
  init->p_s.status
      = ALLOFF | IMON | IEPON
        | TEBITON; // Interrupt abilitati + PLT abilitato + Kernel mode
  init->p_s.reg_t9 = init->p_s.pc_epc = (unsigned int)test;
  insertProcQ (&readyQueue, init);
  processCount++;
  scheduler ();
  return 0;
}
