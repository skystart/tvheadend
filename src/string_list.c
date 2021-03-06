/*
 *  Sorted String List Functions
 *  Copyright (C) 2017 Tvheadend Foundation CIC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "string_list.h"

#include <string.h>
#include "htsmsg.h"

/// Sorted string list helper functions.
void
string_list_init(string_list_t *l)
{
  RB_INIT(l);
}

string_list_t *
string_list_create(void)
{
  string_list_t *ret = calloc(1, sizeof(string_list_t));
  string_list_init(ret);
  return ret;
}


void
string_list_destroy(string_list_t *l)
{
  if (!l) return;

  string_list_item_t *item;
  while ((item = RB_FIRST(l))) {
    RB_REMOVE(l, item, h_link);
    free(item->id);
    free(item);
  }
  free(l);
}

static inline int
string_list_item_cmp(const void *a, const void *b)
{
  return strcmp(((const string_list_item_t*)a)->id, ((const string_list_item_t*)b)->id);
}

void
string_list_insert(string_list_t *l, const char *id)
{
  if (!id) return;

  string_list_item_t *item = calloc(1, sizeof(string_list_item_t));
  item->id = strdup(id);
  if (RB_INSERT_SORTED(l, item, h_link, string_list_item_cmp)) {
    /* Duplicate, so not inserted. */
    free(item->id);
    free(item);
  }
}

htsmsg_t *
string_list_to_htsmsg(const string_list_t *l)
{
  htsmsg_t *ret = NULL;
  string_list_item_t *item;
  RB_FOREACH(item, l, h_link) {
    if (!ret) ret = htsmsg_create_list();
    const char *id = item->id;
    htsmsg_add_str(ret, NULL, id);
  }
  return ret;
}

string_list_t *
htsmsg_to_string_list(const htsmsg_t *m)
{
  string_list_t *ret = NULL;
  htsmsg_field_t *f;
  HTSMSG_FOREACH(f, m) {
    if (f->hmf_type == HMF_STR) {
      const char *str = f->hmf_str;
      if (str && *str) {
        if (!ret) ret = string_list_create();

        string_list_insert(ret, str);
      }
    }
  }
  return ret;
}

void
string_list_serialize(const string_list_t *l, htsmsg_t *m, const char *f)
{
    if (!l) return;

    htsmsg_t *msg = string_list_to_htsmsg(l);
    if (msg)
      htsmsg_add_msg(m, f, msg);
}

string_list_t *
string_list_deserialize(const htsmsg_t *m, const char *n)
{
  htsmsg_t *sub = htsmsg_get_list(m, n);
  if (!sub) return NULL;
  return htsmsg_to_string_list(sub);
}

char *
string_list_2_csv(const string_list_t *l, char delim, int human)
{
  if (!l) return NULL;

  htsmsg_t *msg = string_list_to_htsmsg(l);
  if (!msg) return NULL;

  char *ret = htsmsg_list_2_csv(msg, delim, human);
  htsmsg_destroy(msg);
  return ret;
}

int
string_list_cmp(const string_list_t *m1, const string_list_t *m2)
{
  /* Algorithm based on htsmsg_cmp */
  if (m1 == NULL && m2 == NULL)
    return 0;
  if (m1 == NULL || m2 == NULL)
    return 1;

  string_list_item_t *i1;
  string_list_item_t *i2 = RB_FIRST(m2);
  RB_FOREACH(i1, m1, h_link) {
    if (i2 == NULL)
      return 1;
    const int cmp = strcmp(i1->id, i2->id);
    /* Not equal? */
    if (cmp)
      return cmp;

    i2 = RB_NEXT(i2, h_link);
  }

  if (i2)
    return 1;

  return 0;
}

string_list_t *
string_list_copy(const string_list_t *src)
{
  if (!src) return NULL;
  string_list_t *ret = string_list_create();
  string_list_item_t *item;
  RB_FOREACH(item, src, h_link) {
    const char *id = item->id;
    string_list_insert(ret, id);
  }

  return ret;
}

int
string_list_contains_string(const string_list_t *src, const char *find)
{
  string_list_item_t skel;
  skel.id = (char*)find;

  string_list_item_t *item = RB_FIND(src, &skel, h_link, string_list_item_cmp);
  /* Can't just return item due to compiler settings preventing ptr to
   * int conversion
   */
  return item != NULL;
}
