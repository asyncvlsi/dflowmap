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

#include "config_pkg.h"

#define PIPELINE false
#ifdef FOUND_expropt
#define LOGIC_OPTIMIZER true
#else
#define LOGIC_OPTIMIZER false
#endif
#ifdef FOUND_exproptcommercial
#define COMMERCIAL_LOGIC_OPTIMIZER true
#else
#define COMMERCIAL_LOGIC_OPTIMIZER false
#endif
#ifdef FOUND_dflow_backend_netlist
#define GEN_NETLIST true
#else
#define GEN_NETLIST false
#endif

extern int quiet_mode;
extern int debug_verbose;