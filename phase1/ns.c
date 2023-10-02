#include <ns.h>

HIDDEN nsd_t type_nsd[MAXPROC];
HIDDEN struct list_head type_nsFree_h;
HIDDEN struct list_head type_nsList_h;

/**
 * @brief Alloca un nuovo namespace di tipo type
 *
 * @param type
 * @return nsd_PTR
 */
nsd_PTR
allocNamespace (int type)
{
  if (list_empty (&type_nsFree_h))
    return NULL;
  nsd_PTR ns = list_entry (type_nsFree_h.next, nsd_t, n_link);
  list_del (type_nsFree_h.next);
  list_add (&ns->n_link, &type_nsList_h);
  ns->n_type = type;
  return ns;
}

/**
 * @brief Aggiunge il namespace puntato da ns al PCB puntato da p e al
 * sottoalbero di PCB relativo a p
 *
 * @param p
 * @param ns
 * @return int
 */
int
addNamespace (pcb_PTR p, nsd_PTR ns)
{
  if (list_empty (&type_nsList_h))
    {
      nsd_PTR new = allocNamespace (ns->n_type);
      if (new == NULL)
        return FALSE;
      p->namespaces[ns->n_type] = new;
    }
  else
    p->namespaces[ns->n_type] = ns;
  pcb_PTR curr;
  list_for_each_entry (curr, &p->p_child, p_sib) addNamespace (curr, ns);
  return TRUE;
}

/**
 * @brief Restituisce il namespace di tipo type del PCB puntato da p
 *
 * @param p
 * @param type
 * @return nsd_PTR
 */
nsd_PTR
getNamespace (pcb_PTR p, int type)
{
  return p->namespaces[type];
}

/**
 * @brief Libera il namespace puntato dalla lista dei namespaces attivi
 * reinserendolo nella lista dei namespace liberi
 *
 * @param ns
 */
void
freeNamespace (nsd_PTR ns)
{
  list_del (&ns->n_link);
  list_add_tail (&ns->n_link, &type_nsFree_h);
}

/**
 * @brief Inizializza le liste di namespace
 *
 */
void
initNamespaces ()
{
  INIT_LIST_HEAD (&type_nsFree_h);
  INIT_LIST_HEAD (&type_nsList_h);
  for (int i = 0; i < MAXPROC; i++)
    {
      list_add_tail (&type_nsd[i].n_link, &type_nsFree_h);
    }
}
