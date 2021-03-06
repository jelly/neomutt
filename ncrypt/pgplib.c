/**
 * @file
 * Misc PGP helper routines
 *
 * @authors
 * Copyright (C) 1997-2002 Thomas Roessler <roessler@does-not-exist.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page crypt_pgplib Misc PGP helper routines
 *
 * Misc PGP helper routines
 */

#include "config.h"
#include <stdbool.h>
#include "mutt/mutt.h"
#include "pgplib.h"

/**
 * pgp_pkalgbytype - Get the name of the algorithm from its ID
 * @param type Algorithm ID
 * @retval ptr Algorithm name
 */
const char *pgp_pkalgbytype(unsigned char type)
{
  switch (type)
  {
    case 1:
      return "RSA";
    case 2:
      return "RSA";
    case 3:
      return "RSA";
    case 16:
      return "ElG";
    case 17:
      return "DSA";
    case 20:
      return "ElG";
    default:
      return "unk";
  }
}

/**
 * pgp_canencrypt - Does this algorithm ID support encryption?
 * @param type Algorithm ID
 * @retval true If it does
 */
bool pgp_canencrypt(unsigned char type)
{
  switch (type)
  {
    case 1:
    case 2:
    case 16:
    case 20:
      return true;
    default:
      return false;
  }
}

/**
 * pgp_cansign - Does this algorithm ID support signing?
 * @param type Algorithm ID
 * @retval true If it does
 */
bool pgp_cansign(unsigned char type)
{
  switch (type)
  {
    case 1:
    case 3:
    case 17:
    case 20:
      return true;
    default:
      return false;
  }
}

/**
 * pgp_get_abilities - Get the capabilities of an algorithm
 * @param type Algorithm ID
 * @retval num Capabilities
 *
 * The abilities are OR'd together
 * - 1 If signing is possible
 * - 2 If encryption is possible
 */
short pgp_get_abilities(unsigned char type)
{
  return (pgp_canencrypt(type) << 1) | pgp_cansign(type);
}

/**
 * pgp_free_uid - Free a PGP UID
 * @param upp PGP UID to free
 */
static void pgp_free_uid(struct PgpUid **upp)
{
  struct PgpUid *up = NULL, *q = NULL;

  if (!upp || !*upp)
    return;
  for (up = *upp; up; up = q)
  {
    q = up->next;
    FREE(&up->addr);
    FREE(&up);
  }

  *upp = NULL;
}

/**
 * pgp_copy_uids - Copy a list of PGP UIDs
 * @param up     List of PGP UIDs
 * @param parent Parent PGP key
 * @retval ptr New list of PGP UIDs
 */
struct PgpUid *pgp_copy_uids(struct PgpUid *up, struct PgpKeyInfo *parent)
{
  struct PgpUid *l = NULL;
  struct PgpUid **lp = &l;

  for (; up; up = up->next)
  {
    *lp = mutt_mem_calloc(1, sizeof(struct PgpUid));
    (*lp)->trust = up->trust;
    (*lp)->flags = up->flags;
    (*lp)->addr = mutt_str_strdup(up->addr);
    (*lp)->parent = parent;
    lp = &(*lp)->next;
  }

  return l;
}

/**
 * free_key - Free a PGP Key info
 * @param kpp PGP Key info to free
 */
static void free_key(struct PgpKeyInfo **kpp)
{
  struct PgpKeyInfo *kp = NULL;

  if (!kpp || !*kpp)
    return;

  kp = *kpp;

  pgp_free_uid(&kp->address);
  FREE(&kp->keyid);
  FREE(&kp->fingerprint);
  FREE(kpp);
}

/**
 * pgp_remove_key - Remove a PGP key from a list
 * @param klist List of PGP Keys
 * @param key   Key to remove
 * @retval ptr Updated list of PGP Keys
 */
struct PgpKeyInfo *pgp_remove_key(struct PgpKeyInfo **klist, struct PgpKeyInfo *key)
{
  struct PgpKeyInfo **last = NULL;
  struct PgpKeyInfo *p = NULL, *q = NULL, *r = NULL;

  if (!klist || !*klist || !key)
    return NULL;

  if (key->parent && key->parent != key)
    key = key->parent;

  last = klist;
  for (p = *klist; p && p != key; p = p->next)
    last = &p->next;

  if (!p)
    return NULL;

  for (q = p->next, r = p; q && q->parent == p; q = q->next)
    r = q;

  if (r)
    r->next = NULL;

  *last = q;
  return q;
}

/**
 * pgp_free_key - Free a PGP key info
 * @param kpp PGP key info to free
 */
void pgp_free_key(struct PgpKeyInfo **kpp)
{
  struct PgpKeyInfo *p = NULL, *q = NULL, *r = NULL;

  if (!kpp || !*kpp)
    return;

  if ((*kpp)->parent && (*kpp)->parent != *kpp)
    *kpp = (*kpp)->parent;

  /* Order is important here:
   *
   * - First free all children.
   * - If we are an orphan (i.e., our parent was not in the key list),
   *   free our parent.
   * - free ourselves.
   */

  for (p = *kpp; p; p = q)
  {
    for (q = p->next; q && q->parent == p; q = r)
    {
      r = q->next;
      free_key(&q);
    }
    if (p->parent)
      free_key(&p->parent);

    free_key(&p);
  }

  *kpp = NULL;
}

/**
 * pgp_new_keyinfo - Create a new PgpKeyInfo
 * @retval ptr New PgpKeyInfo
 */
struct PgpKeyInfo *pgp_new_keyinfo(void)
{
  return mutt_mem_calloc(1, sizeof(struct PgpKeyInfo));
}
