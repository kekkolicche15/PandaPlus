#include <lib.h>

unsigned int createProcess (state_PTR, support_PTR, nsd_PTR), getTime (),
    getSupportPtr (), getProcessId (int), getChildren (int *, int);
void systemcallHandler (), passUpOrDieHandler (int), termProcess (pid_t),
    termProcessByPCB (pcb_PTR), doIO (int *, int *), clockWait ();

/**
 * @brief Carica il gestore relativo all'eccezione verificatasi.
 *
 */
void
exceptionHandler ()
{
  switch (CAUSE_GET_EXCCODE (getCAUSE ()))
    {
    case EXC_INT: // Interrupt
      interruptHandler ();
      break;
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS: // TLB exception
      passUpOrDieHandler (PGFAULTEXCEPT);
      break;
    case EXC_SYS: // Syscall
      systemcallHandler ();
      break;
    default:
      passUpOrDieHandler (GENERALEXCEPT);
      break;
    }
}

/**
 * @brief Esegue la syscall specificata in reg_a0.
 * @note I risultati, se presenti, vengono messi nel registro reg_v0.
 *
 */
void
systemcallHandler ()
{
  if ((getSTATUS () & USERPON) != 0)
    {
      setCAUSE ((getCAUSE () & ~CAUSE_EXCCODE_MASK)
                | (EXC_RI << CAUSE_EXCCODE_BIT)); // pone cause.exccode = 10
                                                  // (reserved instruction)
      passUpOrDieHandler (GENERALEXCEPT);
    }
  EXCEPTION_STATE->pc_epc += WORD_SIZE;
  switch (GPR (a0))
    {
    case CREATEPROCESS:
      GPR (v0) = createProcess ((state_PTR)GPR (a1), (support_PTR)GPR (a2),
                                (nsd_PTR)GPR (a3));
      break;
    case TERMPROCESS:
      termProcess ((pid_t)GPR (a1));
      break;
    case PASSEREN:
      passeren ((int *)GPR (a1));
      break;
    case VERHOGEN:
      verhogen ((int *)GPR (a1));
      break;
    case DOIO:
      doIO ((int *)GPR (a1), (int *)GPR (a2));
      break;
    case GETTIME:
      GPR (v0) = getTime ();
      break;
    case CLOCKWAIT:
      clockWait ();
      break;
    case GETSUPPORTPTR:
      GPR (v0) = getSupportPtr ();
      break;
    case GETPROCESSID:
      GPR (v0) = getProcessId ((int)GPR (a1));
      break;
    case GETCHILDREN:
      GPR (v0) = getChildren ((int *)GPR (a1), (int)GPR (a2));
      break;
    default:
      passUpOrDieHandler (GENERALEXCEPT);
    }
  LDST (EXCEPTION_STATE);
}

/**
 * @brief Gestisce l'evento di passaggio al livello di supporto o la
 * terminazione del processo chiamante.
 *
 * @param index Indice dell'evento da gestire. Necessario per garantire
 * l'integrità dei dati durante la gestione delle chiamate di sistema in un
 * contesto di parallelismo intra-processo.
 * @note È importante includere l'indice come parametro poiché, a causa delle
 * potenziali interruzioni e dei page fault durante la gestione delle chiamate
 * di sistema, è necessario preservare l'integrità dei dati della syscall in
 * corso di gestione.
 */
void
passUpOrDieHandler (int index)
{
  if (currentProcess->p_supportStruct == NULL)
    {
      termProcessByPCB (currentProcess);
      scheduler ();
    }
  currentProcess->p_supportStruct->sup_exceptState[index] = *EXCEPTION_STATE;
  context_t ctx = currentProcess->p_supportStruct->sup_exceptContext[index];
  LDCXT (ctx.stackPtr, ctx.status, ctx.pc);
}

/**
 * @brief Crea un nuovo processo
 *
 * @param statep Puntatore alla struttura di stato
 * @param supportp Puntatore alla struttura di supporto
 * @param ns Puntatore al namespace da associare al processo
 * @return Pid del nuovo processo, se e' stato possibile crearlo, -1 altrimenti
 */
unsigned int
createProcess (state_PTR statep, support_PTR supportp, nsd_PTR ns)
{
  pcb_PTR newProcess = allocPcb ();
  if (newProcess == NULL)
    return -1;
  processCount++;
  newProcess->p_s = *statep;
  newProcess->p_supportStruct = supportp;
  insertChild (currentProcess, newProcess);
  insertProcQ (&readyQueue, newProcess);
  if (ns)
    addNamespace (newProcess, ns);
  else
    addNamespace (newProcess, getNamespace (currentProcess, NS_PID));
  return newProcess->p_pid;
}

/**
 * @brief Termina un processo assieme alla sua progenie
 *
 * @param process Puntatore al PCB del processo da terminare
 */
void
termProcessByPCB (pcb_PTR process)
{
  outChild (process); // stacca process dal padre
  processCount--;
  if (process->p_semAdd != NULL)
    {
      if (process->p_semAdd == &itSemaphore
          || ((process->p_semAdd >= &deviceSemaphore[0])
              && (process->p_semAdd
                  <= &deviceSemaphore[EXTERNAL_DEVS_NO - 1])))
        softBlockedCount--;
      outBlocked (process);
    }
  outProcQ (&readyQueue, process);
  pcb_PTR child;
  while (((child = removeChild (process)) != NULL))
    termProcessByPCB (child);
  if ((process->p_parent == NULL)
      || (getNamespace (process, NS_PID)
          != getNamespace (process->p_parent, NS_PID)))
    {
      freeNamespace (getNamespace (process, NS_PID));
    }
  freePcb (process);
}

/**
 * @brief Termina un processo assieme alla sua progenie
 *
 * @param pid Pid del processo da terminare
 */
void
termProcess (pid_t pid)
{
  if (pid == 0)
    {
      termProcessByPCB (currentProcess);
      scheduler ();
    }
  pcb_PTR toKill = getProcessByPid (pid);
  if (toKill != NULL)
    termProcessByPCB (toKill);
  if (!currentProcess->p_alloctd)
    scheduler ();
}

/**
 * @brief Esegue la P() su un semaforo binario
 *
 * @param sem Identificatore del semaforo su cui eseguire la P()
 * @return Puntatore al PCB del processo sbloccato
 */
pcb_PTR
passeren (int *sem)
{
  pcb_PTR unblocked = NULL;
  if (*sem == 0)
    {
      if (insertBlocked (sem, currentProcess))
        PANIC ();
      currentProcess->p_s = *EXCEPTION_STATE;
      currentProcess->p_time += getTimeElapsed ();
      scheduler ();
    }
  else if (headBlocked (sem) != NULL)
    {
      unblocked = removeBlocked (sem);
      insertProcQ (&readyQueue, unblocked);
    }
  else
    *sem = 0;
  return unblocked;
}

/**
 * @brief Esegue la V() su un semaforo binario
 *
 * @param sem Identificatore del semaforo su cui eseguire la V()
 * @return Puntatore al PCB del processo sbloccato
 */
pcb_PTR
verhogen (int *sem)
{
  pcb_PTR unblocked = NULL;
  if (*sem == 1)
    {
      if (insertBlocked (sem, currentProcess))
        PANIC ();
      currentProcess->p_s = *EXCEPTION_STATE;
      currentProcess->p_time += getTimeElapsed ();
      scheduler ();
    }
  else if (headBlocked (sem) != NULL)
    {
      unblocked = removeBlocked (sem);
      insertProcQ (&readyQueue, unblocked);
    }
  else
    *sem = 1;
  return unblocked;
}

/**
 * @brief Effettua un'operazione di IO.
 *
 * @param commandAddr Indirizzo del (sotto-)dispositivo
 * @param commandVal Vettore contenente i dati per istruire il dispositivo
 * sull'operazione da eseguire
 */
void
doIO (int *commandAddr, int *commandVal)
{
  for (int i = (((unsigned int)commandAddr >= TERM0ADDR) ? 1 : 3); i >= 0; i--)
    commandAddr[i] = commandVal[i];
  softBlockedCount++;
  int *devSemaphore = getDeviceSemaphore ((unsigned int)commandAddr + 0x4);
  passeren (devSemaphore);
}

/**
 * @brief Restituisce il tempo di esecuzione del processo chiamante
 *
 * @return Numero di millisecondi per cui il processo e' stato in esecuzione
 */
unsigned int
getTime ()
{
  currentProcess->p_time += getTimeElapsed ();
  return currentProcess->p_time;
}

/**
 * @brief Blocca il processo chiamante fino al successivo tick dell'Interval
 * Timer
 *
 */
void
clockWait ()
{
  softBlockedCount++;
  passeren (&itSemaphore);
}

/**
 * @brief Restituisce il puntatore alla struttura di supporto del processo
 * corrente
 *
 * @return Puntatore alla struttura di supporto del processo
 * corrente
 */
unsigned int
getSupportPtr ()
{
  return (unsigned int)currentProcess->p_supportStruct;
}

/**
 * @brief Restituisce il pid del processo chiamante (se parent==0), quello del
 * padre altrimenti (parent==1)
 *
 * @param parent Se 0, restituisce il pid del processo chiamante; se 1,
 * restituisce il pid del processo padre.
 * @return Pid del processo richiesto
 */
unsigned int
getProcessId (int parent)
{
  if (parent == 0)
    return currentProcess->p_pid;
  if (currentProcess->p_parent == NULL
      || getNamespace (currentProcess, NS_PID)
             != getNamespace (currentProcess->p_parent, NS_PID))
    return 0;
  return currentProcess->p_parent->p_pid;
}

/**
 * @brief Carica il vettore children con i figli del processo chiamante
 *
 * @param children Vettore in cui vengono memorizzati i PID dei figli
 * @param size Dimensione del vettore children.
 * @return Numero di figli del processo chiamante
 * @note Se il numero di figli del processo chiamante è maggiore di size, solo
 * i primi size figli vengono caricati nel vettore. Gli identificatori dei
 * figli non caricati oltre la dimensione del vettore children non verranno
 * inclusi.
 */
unsigned int
getChildren (int *children, int size)
{
  struct list_head *curr;
  int count = 0;
  list_for_each (curr, &currentProcess->p_child)
  {
    pcb_PTR currPcb = container_of (curr, pcb_t, p_sib);
    if (getNamespace (currPcb, NS_PID)
        == getNamespace (currentProcess, NS_PID))
      {
        if (count < size)
          children[count] = currPcb->p_pid;
        count++;
      }
  }
  return count;
}
