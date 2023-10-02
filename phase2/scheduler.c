#include <lib.h>

/**
 * @brief Gestisce lo scheduling dei processi.
 *
 * Questa funzione è responsabile della gestione dello scheduling dei processi
 * all'interno del sistema scegliendo il processo da eseguire secondo una
 * politica round robin. Se non ci sono ne' processi pronti ne' processi
 * bloccati, si procede con l'arresto del sistema. Se non ci sono processi
 * pronti e sono tutti bloccati (DEADLOCK) il sistema va in PANIC; altrimenti
 * si aspetta che un device termini le proprie operazioni sbloccando uno dei
 * processi bloccati facendo riprendere l'esecuzione.
 */
void
scheduler ()
{
  currentProcess = removeProcQ (&readyQueue);
  if (currentProcess != NULL)
    {
      currentProcess->p_s.status = (currentProcess->p_s.status) | TEBITON;
      STCK (lastTOD);
      setTIMER (TIMESLICE * TIMESCALE);
      LDST (&currentProcess->p_s);
    }
  else if (processCount == 0)
    HALT ();
  else if (softBlockedCount == 0)
    PANIC ();
  else
    {
      setSTATUS ((getSTATUS () | IECON | IMON) & ~TEBITON);
      WAIT ();
    }
}
