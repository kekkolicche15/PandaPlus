#include <ash.h>

HIDDEN semd_t semd_table[MAXPROC];
HIDDEN struct list_head semdFree_h;
HIDDEN DEFINE_HASHTABLE (semd_h, const_ilog2 (MAXPROC - 1) + 1);
/* semaphore descriptor (SEMD) data structure */

/*s_link e' la entry della tabella hash, s_key e' la chiave della tabella hash,
 * semd_h e' il nome della tabella hash*/

/**
 * @brief Ritorna un puntatore a un SEMD corrispondente alla chiave semAdd, se
 * esiste.
 *
 * @param semAdd Puntatore alla chiave del SEMD da cercare.
 * @return Puntatore al SEMD corrispondente alla chiave `semAdd`,
 * o NULL se non è presente alcun SEMD con la chiave specificata.
 */
HIDDEN semd_PTR
getSemd (int *semAdd)
{
  semd_PTR semd;
  hash_for_each_possible (semd_h, semd, s_link, (u32)semAdd)
  {
    if ((u32)semd->s_key == (u32)semAdd)
      {
        return semd;
      }
  }
  return NULL;
}

/**
 * @brief Inizializza la lista dei SEMD liberi.
 *
 */
void
initASH ()
{
  INIT_LIST_HEAD (&semdFree_h);
  for (int i = 0; i < MAXPROC; i++)
    list_add_tail (&semd_table[i].s_freelink, &semdFree_h);
}

/**
 * @brief Inserisce un processo nella coda dei processi bloccati di un SEMD.
 *
 * @param semAdd Puntatore alla chiave del SEMD in cui inserire il processo.
 * @param p Puntatore al PCB del processo da inserire nella coda bloccata.
 * @return TRUE se non è stato possibile inizializzare un SEMD, FALSE
 * altrimenti.
 */
int
insertBlocked (int *semAdd, pcb_PTR p)
{
  semd_PTR semd = getSemd (semAdd);
  if (semd == NULL)
    {
      if (list_empty (&semdFree_h))
        return TRUE;
      semd = container_of (semdFree_h.next, semd_t, s_freelink);
      list_del (semdFree_h.next);
      semd->s_key = semAdd;
      mkEmptyProcQ (&semd->s_procq);
      hash_add (semd_h, &semd->s_link, (u32)semAdd);
    }
  p->p_semAdd = semAdd;
  list_add_tail (&p->p_list, &semd->s_procq);
  return FALSE;
}

/**
 * @brief Rimuove e restituisce il primo processo dalla coda dei processi
 * bloccati di un SEMD.
 *
 * @param semAdd Puntatore alla chiave del SEMD da cui rimuovere il
 * processo.
 * @return Puntatore al PCB del processo rimosso, oppure NULL se il SEMD non
 * esiste o la coda dei processi bloccati del SEMD è vuota.
 */
pcb_PTR
removeBlocked (int *semAdd)
{
  semd_PTR semd = getSemd (semAdd);
  if (semd == NULL)
    return NULL;
  pcb_PTR p = container_of (semd->s_procq.next, pcb_t, p_list);
  p->p_semAdd = NULL;
  list_del (semd->s_procq.next);
  if (emptyProcQ (&semd->s_procq))
    {
      semd->s_key = NULL;
      hash_del (&semd->s_link);
      list_add_tail (&semd->s_freelink, &semdFree_h);
    }
  return p;
}

/**
 * @brief Verifica se il PCB puntato da p è presente in coda.
 *
 * @param p Puntatore al PCB da cercare e potenzialmente rimuovere dalla coda.
 * @return Puntatore al PCB `p` se è stato trovato e rimosso dalla coda, oppure
 * NULL se non è presente o manca l'associazione a un Semaphore Descriptor.
 * @note Se la coda dei processi bloccati associata al Semaphore Descriptor
 * diventa vuota dopo la rimozione del processo, il Semaphore Descriptor viene
 * rimosso dalla ASH e inserito nella lista dei semafori liberi.
 */
pcb_PTR
outBlocked (pcb_PTR p)
{
  if (p->p_semAdd == NULL)
    return NULL;
  semd_PTR semd = getSemd (p->p_semAdd);
  p->p_semAdd = NULL;
  list_del (&p->p_list);
  if (emptyProcQ (&semd->s_procq))
    {
      semd->s_key = NULL;
      hash_del (&semd->s_link);
      list_add_tail (&semd->s_freelink, &semdFree_h);
    }
  return p;
}

/**
 * @brief Verifica se un SEMD è presente nella ASH e ritorna il primo processo
 * bloccato.
 *
 * @param semAdd Puntatore alla chiave del SEMD da cercare nella ASH.
 * @return Puntatore al primo PCB in coda se il SEMD è presente e la coda non è
 * vuota, NULL altrimenti.
 */
pcb_PTR
headBlocked (int *semAdd)
{
  semd_PTR semd = getSemd (semAdd);
  if (semd == NULL)
    return NULL;
  pcb_PTR p = container_of (semd->s_procq.next, pcb_t, p_list);
  if (emptyProcQ (&semd->s_procq))
    return NULL;
  return p;
}
