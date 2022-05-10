/*
 * This file is part of the ACT library
 *
 * Copyright (c) 2021 Rui Li
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "DflowMapPass.h"

DflowMapPass::DflowMapPass(Act *a,
                           const char *name,
                           Metrics *metrics,
                           ChpBackend *backend) : ActPass(a, name) {
  this->metrics = metrics;
  this->backend = backend;
  _count = 0;
}

void *DflowMapPass::local_op(Process *p, int mode) {
  if (!p) return nullptr;
  if (!p->isExpanded() || !p->isDefined()) return nullptr;
  auto proc_generator = new ProcGenerator(metrics, backend);
  proc_generator->run(p);
  _count++;
  return nullptr;
}
