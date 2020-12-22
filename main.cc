/*************************************************************************
 *
 *  This file is part of the ACT library
 *
 *  Copyright (c) 2020 Rajit Manohar
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *
 **************************************************************************
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <act/act.h>
#include <act/passes.h>
#include "config.h"


static void usage (char *name)
{
  fprintf (stderr, "Usage: %s <actfile> <process>\n", name);
  exit (1);
}


int main (int argc, char **argv)
{
  Act *a;
  char *proc;

  /* initialize ACT library */
  Act::Init (&argc, &argv);

  /* some usage check */
  if (argc != 3) {
    usage (argv[0]);
  }

  /* read in the ACT file */
  a = new Act (argv[1]);
  a->Expand ();

  Process *p = a->findProcess (argv[2]);
  if (!p) {
    fatal_error ("Could not find process `%s'", argv[2]);
  }

  if (!p->isExpanded()) {
    fatal_error ("Process `%s': is not expanded", argv[2]);
  }

  if (!p->getlang()->getdflow()) {
    fatal_error ("Process `%s': no dataflow body", argv[2]);
  }

  a->mangle (NULL);
  p->PrintHeader (stdout, "defproc");
  printf ("\n{\n");
  p->CurScope()->Print (stdout);

  int inst_id = 0;
  listitem_t *li;
  for (li = list_first (p->getlang()->getdflow()->dflow);
       li;
       li = list_next (li)) {
    act_dataflow_element *d = (act_dataflow_element *) list_value (li);
    switch (d->t) {
    case ACT_DFLOW_FUNC:
      /* -- what to do about expressions? -- */
      break;

      
    case ACT_DFLOW_SPLIT:
    case ACT_DFLOW_MERGE:
    case ACT_DFLOW_MIXER:
    case ACT_DFLOW_ARBITER:
      break;
      
    default:
      fatal_error ("Unknown dataflow type %d\n", d->t);
      break;
    }
  }
  printf ("\n}\n");
  return 0;
}
