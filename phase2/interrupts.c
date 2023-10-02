#include <lib.h>

void PLTHandler (), ITHandler (), NTHandler (int), startInterrupt (),
    endInterrupt (), deviceInterruptReturn (unsigned int *);
int getDeviceNumber (int);

/**
 * @brief Passa il controllo al corretto gestore degli interrupt
 */
void
interruptHandler ()
{
  startInterrupt ();
  if (CAUSE_GET_IP & LOCALTIMERINT)
    PLTHandler ();
  else if (CAUSE_GET_IP & TIMERINTERRUPT)
    ITHandler ();
  else
    NTHandler (CAUSE_GET_IP);
  endInterrupt ();
}

/**
 * @brief Aggiorna il tempo di esecuzione del processo corrente, cosi' da non
 * attribuirgli il tempo di gestione dell'interrupt
 */
void
startInterrupt ()
{
  if (currentProcess != NULL)
    currentProcess->p_time += getTimeElapsed ();
  setSTATUS (getSTATUS () & ~TEBITON);
}

/**
 *
 * @brief Ricarica il processo interrotto o, se questo non e' presente,
 * richiama lo scheduler
 *
 */
void
endInterrupt ()
{
  setSTATUS (getSTATUS () | TEBITON);
  STCK (lastTOD);
  if (currentProcess != NULL)
    LDST (EXCEPTION_STATE);
  else
    scheduler ();
}

/**
 * @brief Determina la linea dell'interrupt, individua il sub-device su cui e'
 * pendente l'interrupt, e chiama la procedura per mandare l'acknoledgement al
 * sub-device
 * @param ip Linee di interrupt pendenti
 */
void
NTHandler (int ip)
{
  int deviceLine, deviceNumber, mask;
  for (deviceLine = DEV_IL_START, mask = CAUSE_IP (deviceLine);
       deviceLine < N_IL; deviceLine++, mask <<= 1)
    if (ip & mask)
      {
        if ((deviceNumber = getDeviceNumber (deviceLine)) == -1)
          PANIC ();
        unsigned int *status
            = (unsigned int *)DEV_REG_ADDR (deviceLine, deviceNumber);
        status += HAS_TERM_TRANSM (deviceLine, status) ? TRANSM_OFFSET
                                                       : RECV_OFFSET;
        deviceInterruptReturn (status);
        break;
      }
}

/**
 * @brief Procedura che aggiorna il tempo di utilizzo della cpu del processo
 * corrente e mette il processo corrente nella readyQueue
 * @note Il PLT verra' ackato, se necessario, dallo scheduler.
 */
void
PLTHandler ()
{
  currentProcess->p_s = *EXCEPTION_STATE;
  currentProcess->p_time += getTimeElapsed ();
  if (currentProcess->p_alloctd)
    insertProcQ (&readyQueue, currentProcess);
  currentProcess = NULL;
}

/**
 * @brief Riattiva tutti i processi in attesa dell'interval timer e carica IT
 * a 100000 microsecondi
 */
void
ITHandler ()
{
  while (headBlocked (&itSemaphore) != NULL)
    {
      verhogen (&itSemaphore);
      softBlockedCount--;
    }
  LDIT (PSECOND);
}

/**
 * @brief Determina la posizione del sub-device sulla linea su cui e' pendente
 * l'interrupt
 * @param line Linea su cui e' pendente l'interrupt
 * @return La posizione del sub-device sulla linea
 */
int
getDeviceNumber (int line)
{
  devregarea_t *bus_reg_area = (devregarea_t *)BUS_REG_RAM_BASE;
  unsigned int bitmap = bus_reg_area->interrupt_dev[EXT_IL_INDEX (line)];
  for (int number = 0, mask = 1; number < N_DEV_PER_IL; number++, mask <<= 1)
    if (bitmap & mask)
      return number;
  return -1;
}

/**
 * @brief Procedura che manda l'acknoledgement al sub-device, effettua una V()
 * sul semaforo associato al sub-device e infine imposta il valore di ritorno
 * per la SYSCALL DO_IO
 * @param status Indirizzo del campo status del sub-device
 */
void
deviceInterruptReturn (unsigned int *status)
{
  unsigned int statusCode = *status;
  unsigned int *command = status + 1;
  *command = ACK;
  pcb_PTR readyProc = verhogen (getDeviceSemaphore ((unsigned int)command));
  softBlockedCount--;
  for (int i = 0; i < (((unsigned int)command >= TERM0ADDR) ? 2 : 4); i++)
    ((unsigned int *)(readyProc->p_s.reg_a2))[i] = statusCode;
  readyProc->p_s.reg_v0 = ((statusCode & TERMSTATMASK) == 5 ? 0 : -1);
}
