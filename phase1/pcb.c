#include <pcb.h>

HIDDEN pcb_t pcbFree_table[MAXPROC];
HIDDEN struct list_head pcbFree_h;

/**
 * @brief Inizializza la lista dei PCB liberi.
 *
 */
void
initPcbs ()
{
  INIT_LIST_HEAD (&pcbFree_h);
  for (int i = MAXPROC - 1; i >= 0; i--)
    {
      list_add (&pcbFree_table[i].p_list, &pcbFree_h);
      pcbFree_table[i].p_pid = i + 1;
    }
}

/**
 * @brief Aggiunge un PCB alla lista dei PCB liberi.
 *
 * @param pcb Puntatore al PCB da aggiungere alla lista dei PCB liberi.
 */
void
freePcb (pcb_PTR pcb)
{
  list_add (&pcb->p_list, &pcbFree_h);
  pcb->p_alloctd = false;
}

/**
 * @brief Restituisce il puntatore al PCB associato al PID specificato.
 *
 * @param pid Il PID del processo per il quale si desidera ottenere il PCB.
 * @return Un puntatore al PCB del processo corrispondente al PID,
 * oppure NULL se il PID non è valido o non è associato a nessun processo.
 */
pcb_PTR
getProcessByPid (pid_t pid)
{
  if (pid < 1 || pid > MAXPROC || !pcbFree_table[pid - 1].p_alloctd)
    return NULL;
  return &pcbFree_table[pid - 1];
}

/**
 * @brief Alloca e restituisce il puntatore al primo PCB libero, rimuovendolo
 * dalla lista dei PCB liberi.
 *
 * @return Il puntatore al PCB allocato e inizializzato, oppure NULL se non è
 * disponibile alcun PCB libero.
 */
pcb_PTR
allocPcb ()
{
  if (list_empty (&pcbFree_h))
    return NULL;
  pcb_PTR pcb = container_of (pcbFree_h.next, pcb_t, p_list);
  pcb->p_parent = NULL;
  list_del (&pcb->p_list);
  mkEmptyProcQ (&pcb->p_list);
  pcb->p_parent = NULL;
  mkEmptyProcQ (&pcb->p_child);
  mkEmptyProcQ (&pcb->p_sib);
  pcb->p_time = 0;
  pcb->p_semAdd = NULL;
  pcb->p_supportStruct = NULL;
  for (int i = 0; i < NS_TYPE_MAX; i++)
    pcb->namespaces[i] = NULL;
  pcb->p_alloctd = true;
  return pcb;
}

/**
 * @brief Inizializza una lista vuota
 *
 * @param head Il puntatore alla testa della lista da inizializzare.
 */
void
mkEmptyProcQ (struct list_head *head)
{
  INIT_LIST_HEAD (head);
}

/**
 * @brief Verifica se una lista è vuota.
 *
 * @param head Il puntatore alla testa lista da verificare.
 * @return 1 se la lista è vuota, 0 altrimenti.
 */
int
emptyProcQ (struct list_head *head)
{
  return list_empty (head);
}

/**
 * @brief Inserisce un PCB in coda a una lista di processi.
 *
 * @param head Il puntatore alla testa della coda di processi in cui inserire
 * il PCB.
 * @param pcb Il puntatore al PCB da inserire nella coda.
 */
void
insertProcQ (struct list_head *head, pcb_PTR pcb)
{
  list_add_tail (&pcb->p_list, head);
}

/**
 * @brief Restituisce il puntatore al primo PCB in testa a una coda di
 * processi.
 *
 * @param head Il puntatore alla testa della coda di processi da cui ottenere
 * il primo PCB.
 * @return Il puntatore al primo PCB nella coda, oppure NULL se la coda è
 * vuota.
 */
pcb_PTR
headProcQ (struct list_head *head)
{
  return emptyProcQ (head) ? NULL : container_of (head->next, pcb_t, p_list);
}

/**
 * @brief Rimuove e restituisce il puntatore al primo PCB da una coda di
 * processi.
 *
 * @param head Il puntatore alla testa della coda di processi da cui rimuovere
 * il primo PCB.
 * @return Un puntatore al primo PCB rimosso dalla coda, oppure NULL se la coda
 * è vuota.
 */
pcb_PTR
removeProcQ (struct list_head *head)
{
  if (emptyProcQ (head))
    return NULL;
  pcb_PTR out = headProcQ (head);
  list_del (&out->p_list);
  return out;
}

/**
 * @brief Rimuove e restituisce il puntatore a un PCB specifico da una coda di
 * processi.
 *
 * @param head Il puntatore alla testa della coda di processi da cui rimuovere
 * il PCB.
 * @param pcb Il puntatore al PCB da rimuovere dalla coda.
 * @return Un puntatore al PCB rimosso dalla coda, oppure NULL se il PCB non è
 * presente nella coda.
 */
pcb_PTR
outProcQ (struct list_head *head, pcb_PTR pcb)
{
  pcb_PTR iter;
  list_for_each_entry (iter, head, p_list)
  {
    if (iter == pcb)
      {
        list_del (&pcb->p_list);
        return pcb;
      }
  }
  return NULL;
}

/**
 * @brief Verifica se un PCB non ha figli.
 *
 * @param pcb Il puntatore al PCB da verificare.
 * @return TRUE se il PCB non ha figli o è NULL, FALSE altrimenti.
 */
int
emptyChild (pcb_PTR pcb)
{
  return pcb == NULL || emptyProcQ (&pcb->p_child);
}

/**
 * @brief Inserisce un PCB in coda alla lista dei figli di un altro PCB.
 *
 * @param prnt Il puntatore al PCB genitore.
 * @param pcb Il puntatore al PCB figlio da inserire.
 */
void
insertChild (pcb_PTR prnt, pcb_PTR pcb)
{
  list_add_tail (&pcb->p_sib, &prnt->p_child);
  pcb->p_parent = prnt;
}

/**
 * @brief
 *
 * @param pcb
 * @return pcb_PTR
 */
pcb_PTR
removeChild (pcb_PTR pcb)
{
  // rimuove il primo figlio del PCB puntato da p, se esiste, e lo restituisce
  if (emptyProcQ (&pcb->p_child))
    return NULL;
  pcb_PTR child = container_of (pcb->p_child.next, pcb_t, p_sib);
  list_del (pcb->p_child.next);
  return child;
}

/**
 * @brief Rimuove e restituisce il puntatore al primo figlio del padre un PCB.
 *
 * @param pcb Il puntatore al PCB di cui padre rimuovere il primo figlio.
 * @return Un puntatore al primo figlio rimosso dal PCB, oppure NULL se il PCB
 * non ha padre o è NULL.
 */
pcb_PTR
outChild (pcb_PTR pcb)
{
  if (pcb->p_parent == NULL)
    return NULL;
  struct list_head *iter;
  list_for_each (iter, &pcb->p_parent->p_child)
  {
    pcb_PTR curr_pcb = container_of (iter, pcb_t, p_sib);
    if (curr_pcb == pcb)
      {
        list_del (iter);
        return pcb;
      }
  }
  return NULL;
}
