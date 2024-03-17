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

#include "Metrics.h"
#include <string.h>

void Metrics::updateMetrics(const char *instance, double *metric) {
  const char *normInstance = getNormInstanceName(instance);
  for (auto &opMetricsIt: opMetrics) {
    if (!strcmp(opMetricsIt.first, normInstance)) {
      if (debug_verbose) {
        printf("We already have metric info for (%s, %s)\n",
               instance, normInstance);
      }
      double *oldMetric = opMetricsIt.second;
      if ((oldMetric[0] != metric[0])
          || (oldMetric[1] != metric[1])
          || (oldMetric[2] != metric[2])
          || (oldMetric[3] != metric[3])) {
        printf("We find different metric record for %s\n", normInstance);
        exit(-1);
      }
      return;
    }
  }
  opMetrics.insert(std::make_pair(normInstance, metric));
}

void Metrics::updateTemplMetrics(const char *instance, MetricGen *metric) {
  for (auto &opMetricsIt: templMetrics) {
    if (!strcmp(opMetricsIt.first, instance)) {
      if (debug_verbose) {
        printf("We already have metric info for %s\n", instance);
      }
      MetricGen *oldMetric = opMetricsIt.second;
      if (!(*oldMetric == *metric)) {
        printf("We find different metric record for %s\n", instance);
        exit(-1);
      }
      return;
    }
  }
  templMetrics[instance] = metric;
}


void Metrics::updateCachedMetrics(const char *instance, double *metric) {
  const char *normInstance = getNormInstanceName(instance);
  for (auto &cachedMetricsIt: cachedMetrics) {
    if (!strcmp(cachedMetricsIt.first, normInstance)) {
      if (debug_verbose) {
        printf("We already have metric info for (%s, %s) in the cache\n",
               instance, normInstance);
      }
      double *oldMetric = cachedMetricsIt.second;
      if ((oldMetric[0] != metric[0])
          || (oldMetric[1] != metric[1])
          || (oldMetric[2] != metric[2])
          || (oldMetric[3] != metric[3])) {
        printf("We find different metric record for %s in the cache\n",
               normInstance);
        exit(-1);
      }
      return;
    }
  }
  cachedMetrics.insert(std::make_pair(normInstance, metric));
}

double *Metrics::getOpMetric(const char *instance) {
  if (!_have_metrics) {
    return NULL;
  }
  if (instance == nullptr) {
    printf("Try to get metric for null instance!\n");
    exit(-1);
  }
  const char *normInstance = getNormInstanceName(instance);
  if (debug_verbose) {
    printf("get op metric for (%s, %s)\n", instance, normInstance);
  }
  for (auto &opMetricsIt: opMetrics) {
    if (!strcmp(opMetricsIt.first, normInstance)) {
      double *metric = opMetricsIt.second;
      return metric;
    }
  }
  double *met = calcMetric (instance, 1);
  if (met) {
    return met;
  }
  if (debug_verbose) {
    printf("We don't have metric info for (`%s`,`%s')\n",
           instance,
           normInstance);
  }
  return nullptr;
}

MetricGen *Metrics::getTemplMetric(const char *instance) {
  if (!_have_metrics) {
    return NULL;
  }
  if (instance == nullptr) {
    printf("Try to get metric for null instance!\n");
    exit(-1);
  }
  if (debug_verbose) {
    printf("get op metric for %s\n", instance);
  }
  for (auto &opMetricsIt: templMetrics) {
    if (!strcmp(opMetricsIt.first, instance)) {
      MetricGen *metric = opMetricsIt.second;
      return metric;
    }
  }
  if (debug_verbose) {
    printf("We don't have metric info for `%s`\n", instance);
  }
  return nullptr;
}

double *Metrics::calcMetric (const char *instance, int bitwidth)
{
  MetricGen *mg = getTemplMetric (instance);
  if (!mg) {
    return nullptr;
  }
  double *metric = new double[4];
  metric[METRIC_LEAK_POWER] = mg->getLeak (bitwidth);
  metric[METRIC_DYN_ENERGY] = mg->getEnergy (bitwidth);
  metric[METRIC_DELAY] =  mg->getDelay (bitwidth);
  metric[METRIC_AREA] = mg->getArea (bitwidth);
  return metric;
}

double *Metrics::getCachedMetric(const char *instance) {
  if (!_have_metrics) {
    return NULL;
  }
  if (instance == nullptr) {
    printf("Try to get metric for null instance!\n");
    exit(-1);
  }
  const char *normInstance = getNormInstanceName(instance);
  if (debug_verbose) {
    printf("get op metric for (%s, %s) from cache\n", instance, normInstance);
  }
  for (auto &cachedMetricsIt: cachedMetrics) {
    if (!strcmp(cachedMetricsIt.first, normInstance)) {
      /* copy the netlist file from cache to the output directory */
      char *cached_netlist_file =
          new char[strlen(cache_dir) + strlen(normInstance) + 8];
      sprintf(cached_netlist_file, "%s/%s.act", cache_dir, normInstance);
      char *errMsg = new char[128];
      sprintf(errMsg,
              "Fail to copy the optimized netlist file from cache to the output directory!\n");
      copyFileToTargetDir(cached_netlist_file, custom_fu_dir, errMsg);
      /* update the local metric file */
      double *metric = cachedMetricsIt.second;
      writeLocalMetricFile(instance, metric);
      /* return the perf metric */
      return metric;
    }
  }
  if (debug_verbose) {
    printf("We don't have metric info for (`%s`,`%s') in the cache\n",
           instance,
           normInstance);
  }
  return nullptr;
}

double Metrics::getLP(double metric[4]) {
  if (!metric) {
    printf("Try to extract LP for null metric!\n");
    exit(-1);
  }
  return metric[METRIC_LEAK_POWER];
}

double Metrics::getEnergy(double metric[4]) {
  if (!metric) {
    printf("Try to extract energy for null metric!\n");
    exit(-1);
  }
  return metric[METRIC_DYN_ENERGY];
}

double Metrics::getDelay(double metric[4]) {
  if (!metric) {
    printf("Try to extract delay for null metric!\n");
    exit(-1);
  }
  return metric[METRIC_DELAY];
}

double Metrics::getArea(double metric[4]) {
  if (!metric) {
    printf("Try to extract area for null metric!\n");
    exit(-1);
  }
  return metric[METRIC_AREA];
}

void Metrics::printOpMetrics() {
  printf("Info in opMetrics:\n");
  for (auto &opMetricsIt: opMetrics) {
    printf("%s ", opMetricsIt.first);
  }
  printf("\n");
}

void Metrics::writeLocalMetricFile(const char *instance, double *metric) {
  const char *normInstance = getNormInstanceName(instance);
  std::ofstream metricFp;

  metricFp.open(custom_metrics, std::ios_base::app);
  metricFp << normInstance << "  " << metric[0] << "  " << metric[1] << "  "
           << metric[2] << "  " << metric[3] << "\n";
  metricFp.close();
}

void Metrics::writeCachedMetricFile(const char *instance, double *metric) {
  const char *normInstance = getNormInstanceName(instance);
  std::ofstream metricFp;
  metricFp.open(cached_metrics, std::ios_base::app);
  metricFp << normInstance << "  " << metric[0] << "  " << metric[1] << "  "
           << metric[2] << "  " << metric[3] << "\n";
  metricFp.close();
}

void Metrics::readMetricsFile(const char *metricsFP, bool forCache, bool stdfile) {
  if (debug_verbose) {
    printf("Read metric file: %s\n", metricsFP);
  }
  std::ifstream metricFp(metricsFP);
  if (metricFp.fail()) {
    if (debug_verbose) {
      printf ("--> failed to open file!\n");
    }
    _have_metrics = false;
    return;
  }
  std::string line;
  int lineno = 0;
  while (std::getline(metricFp, line)) {
    lineno++;

    if (lineno == 1 && !stdfile) {
      const char *tech = getenv ("ACT_TECH");
      assert (tech);
      if (strncmp (line.c_str(), tech, strlen (tech)) != 0) {
	fatal_error ("Using incompatible cached metrics file; invalidate with -i\n\t Requested: `%s'; Cache: `%s'", tech, line.c_str());
      }
      continue;
    }

    std::istringstream iss(line);
    char *instance = new char[MAX_INSTANCE_LEN];
    int metricCount = -1;
    auto metric = new double[4];
    MetricGen *mg = new MetricGen;
    bool emptyLine = true;
    do {
      std::string numStr;
      iss >> numStr;
      if (numStr.find("#", 0) == 0) break;
      if (!numStr.empty()) {
        emptyLine = false;
        if (metricCount >= 0) {
	  if (stdfile) {
	    if (!mg->readOneMetrics (metricCount, numStr.c_str())) {
	      warning ("%s: error reading metric %d (line %d)", instance, metricCount, lineno);
	    }
	  }
	  else {
	    metric[metricCount] = std::strtod(numStr.c_str(), nullptr);
	  }
        } else {
          sprintf(instance, "%s", numStr.c_str());
        }
        metricCount++;
      }
    } while (iss);
    if (!emptyLine && (metricCount != 4)) {
      printf("%s has %d metrics! (%d)\n", metricsFP, metricCount, lineno);
      exit(-1);
    }
    if (emptyLine) {
      delete mg;
      delete [] metric;
      continue;
    }
    if (forCache) {
      updateCachedMetrics(instance, metric);
      delete mg;
    } else {
      if (stdfile) {
	delete [] metric;
	updateTemplMetrics (instance, mg);
      }
      else {
	updateMetrics(instance, metric);
	delete mg;
      }
    }
  }
}

void Metrics::readMetricsFile() {
  readMetricsFile(std_metrics, false, true);
  readMetricsFile(custom_metrics, false, false);
  readMetricsFile(cached_metrics, true, false);
}

Metrics::Metrics(const char *customFUMetricsFP,
                 const char *stdFUMetricsFP,
                 const char *statisticsFP) {
  _have_metrics = true;
  this->custom_metrics = customFUMetricsFP;
  this->std_metrics = stdFUMetricsFP;
  this->statisticsFilePath = statisticsFP;

  totalArea = 0;
  totalLeakPowewr = 0;
  mergeArea = 0;
  splitArea = 0;
  mergeLeakPower = 0;
  splitLeakPower = 0;
}

unsigned Metrics::getEquivalentBW(unsigned oriBW) {
  if (oriBW < 4) {
    return 1;
  } else if (oriBW < 12) {
    return 8;
  } else if (oriBW < 24) {
    return 16;
  } else if (oriBW < 48) {
    return 32;
  } else if (oriBW <= 64) {
    return 64;
  } else {
    printf("Invalid M/S bitwidth: %u!\n", oriBW);
    exit(-1);
  }
}

double *Metrics::getOrGenCopyMetric(unsigned bitwidth, unsigned numOut) {
  if (!_have_metrics) {
    return NULL;
  }

  if (numOut > 8) {
    numOut = 8;
  }

  updateCopyStatistics(bitwidth, numOut);
  char *instance = new char[1500];
  char *leafinst = new char[1500];
  sprintf(instance, "lib::copy<%u,%u>", bitwidth, numOut);
  sprintf(leafinst, "lib::copy_leaf<%u,%u>", bitwidth, numOut);
#if 0
  printf (" looking-for: %s\n", instance);
#endif  
  double *metric = getOpMetric(instance);
  if (!metric) {
    char *equivInstance = new char[1500];
    /*
      COPY implemented as a tree of 2-way copies
    */
    sprintf(equivInstance, "lib::copy<%u,%u>", bitwidth, numOut);
#if 0
    printf ("    > now-look-for: %s\n", equivInstance);
#endif    

    metric = getOpMetric (equivInstance);
    if (!metric) {
      char tmp[100];
      snprintf (tmp, 100, "lib::copy<1,%u>", numOut);
      metric = calcMetric (tmp, bitwidth);
      if (!metric) {
	printf("Missing metrics for copy %s\n", equivInstance);
	exit(-1);
      }
#if 0      
      printf ("    > used-interpolated copy<1 / %d,%d> [%g %g %g %g]\n",
	      bitwidth, numout, metric[0], metric[1], metric[2], metric[3]);
#endif      
    }
#if 0    
    metric[METRIC_LEAK_POWER] *= (numOut-1);
    metric[METRIC_DYN_ENERGY] *= (numOut-1);
    metric[METRIC_DELAY] *= ceil(log(numOut)/log(2));
    metric[METRIC_AREA] *= (numOut-1);
#endif    
#if 0
    printf ("    *> computed [%g %g %g %g]\n", metric[0], metric[1], metric[2],
	    metric[3]);
#endif    
  }
  else {
#if 0    
    printf ("  --> found [%g %g %g %g]\n", metric[0], metric[1], metric[2],
	    metric[3]);
#endif    
  }
  if (metric) {
    updateStatistics(instance, metric);
    updateStatistics(leafinst, metric);
  }
  return metric;
}

double *Metrics::getSinkMetric(int bw) {
  if (!_have_metrics) {
    return NULL;
  }
  char *instance = new char[100];
  sprintf (instance, "lib::sink<%d>", bw);
  double *metric = getOpMetric (instance);
  if (metric) {
    updateStatistics (instance, metric);
    return metric;
  }

  metric = calcMetric ("lib::sink<1>", bw);
  if (!metric) {
    printf("We fail to find metric for the stdlib process lib::sink!\n");
    exit(-1);
  }

  updateStatistics(instance, metric);
  return metric;
}

double *Metrics::getSourceMetric(int bw) {
  if (!_have_metrics) {
    return NULL;
  }
  char *instance = new char[64];
  sprintf (instance, "lib::source<%d>", bw);

  double *metric = getOpMetric (instance);
  if (!metric) {
    metric = calcMetric ("lib::source<1>", bw);
    if (!metric) {
      printf("We fail to find metric for the stdlib process lib::source!\n");
      exit(-1);
    }
  }

  updateStatistics(instance, metric);
  
  return metric;
}

double *Metrics::getOrGenInitMetric(unsigned int bitwidth) {
  if (!_have_metrics) {
    return NULL;
  }

  char *instance = new char[100];
  sprintf(instance, "lib::init<%u>", bitwidth);
  double *metric = getOpMetric(instance);
  if (!metric) {
    metric = calcMetric ("lib::init<1>", bitwidth);
    if (!metric) {
      fatal_error ("Failed to find metric for lib::init");
    }
  }
  updateStatistics(instance, metric);

  return metric;
}

double *Metrics::getBuffMetric(unsigned nBuff, unsigned bw) {
  if (!_have_metrics) {
    return NULL;
  }
  char *instance = new char[100];
  double *metric = nullptr;

  sprintf (instance, "lib::onebuf<%d>", bw);
  metric = getOpMetric (instance);
  if (!metric) {
    metric = calcMetric ("lib::onebuf<1>", bw);
    if (!metric) {
      return nullptr;
    }
  }

#if 0
  /* this is wrong */
  double *actual = new double[4];
  for (int i=0; i < 4; i++) {
    actual[i] = nBuff * metric[i];
  }
#endif  
  
  updateStatistics (instance, metric);
  
  return metric;
}

void Metrics::callLogicOptimizer(
#if LOGIC_OPTIMIZER
    const char *instance,
    StringMap<unsigned> &inBW,
    StringMap<unsigned> &hiddenBW,
    Map<const char *, Expr *> &exprMap,
    Map<Expr *, Expr *> &hiddenExprs,
    Map<unsigned int, unsigned int> &outRecord,
    UIntVec &outBWList,
    double *&metric
#endif
) {
  if (!_have_metrics) {
#if LOGIC_OPTIMIZER    
    metric = new double[4];
    metric[0] = 0;
    metric[1] = 0;
    metric[2] = 0;
    metric[3] = 0;
#endif    
    return;
  }
    
#if LOGIC_OPTIMIZER
  if (debug_verbose) {
    printf("Will run logic optimizer for %s\n", instance);
  }
  /* Prepare in_expr_list */
  list_t *in_expr_list = list_new();
  iHashtable *in_expr_map = ihash_new(0);
  iHashtable *in_width_map = ihash_new(0);

  for (auto &inBWIt: inBW) {
    String inName = inBWIt.first;
    unsigned bw = inBWIt.second;
    char *inChar = new char[strlen(inName.c_str()) + 1];
    sprintf(inChar, "%s", inName.c_str());
    if (debug_verbose) {
      printf("inChar: %s\n", inChar);
    }
    Expr *inExpr = getExprFromName(inChar, exprMap, true, -1);
    list_append(in_expr_list, inExpr);
    ihash_bucket_t *b_expr, *b_width;
    b_expr = ihash_add(in_expr_map, (long) inExpr);
    b_expr->v = inChar;
    b_width = ihash_add(in_width_map, (long) inExpr);
    b_width->i = (int) bw;
  }
  /* Prepare hidden_expr_list */
  list_t *hidden_expr_list = list_new();
  list_t *hidden_expr_name_list = list_new();
  iHashtable *out_width_map = ihash_new(0);
  for (auto &hiddenBWIt: hiddenBW) {
    String hiddenName = hiddenBWIt.first;
    unsigned bw = hiddenBWIt.second;
    char *hiddenChar = new char[1 + strlen(hiddenName.c_str())];
    sprintf(hiddenChar, "%s", hiddenName.c_str());
    if (debug_verbose) {
      printf("hiddenChar: %s\n", hiddenChar);
    }
    Expr *hiddenRHS = getExprFromName(hiddenChar, exprMap, true, -1);
    ihash_bucket_t *b_expr2, *b_width2;

    if (debug_verbose) {
      printf ("hiddenRHS: ");
      print_expr(stdout, hiddenRHS);
      printf (" ; val = %p\n", hiddenRHS);
    }
    
    b_expr2 = ihash_add(in_expr_map, (long) hiddenRHS);
    b_expr2->v = hiddenChar;
    b_width2 = ihash_add(in_width_map, (long) hiddenRHS);
    b_width2->i = (int) bw;
    Expr *hiddenExpr = hiddenExprs.find(hiddenRHS)->second;

    if (debug_verbose) {
      printf ("hiddenExpr: ");
      print_expr (stdout, hiddenExpr);
      printf (" ; val = %p\n", hiddenExpr);
    }

    // hack
    if (hiddenExpr->type == E_BITFIELD) {
      char buf[10240];
      ((ActId *)hiddenExpr->u.e.l)->sPrint (buf, 10240);
      b_expr2 = ihash_add (in_expr_map, (long) hiddenExpr);
      b_expr2->v = Strdup (buf);
    }
    
    list_append(hidden_expr_list, hiddenExpr);
    list_append(hidden_expr_name_list, hiddenChar);
    ihash_bucket_t *b_width = ihash_lookup(out_width_map, (long) hiddenExpr);
    if (!b_width) {
      b_width = ihash_add(out_width_map, (long) hiddenExpr);
      b_width->i = (int) bw;
    }
  }
  /* Prepare out_expr_list */
  list_t *out_expr_list = list_new();
  list_t *out_expr_name_list = list_new();
  UIntVec processedResIDs;
  unsigned numOuts = outRecord.size();
  for (unsigned ii = 0; ii < numOuts; ii++) {
    unsigned resID = outRecord.find(ii)->second;
    char *resChar = new char[SHORT_STRING_LEN];
    sprintf(resChar, "res%u", resID);
    if (debug_verbose) {
      printf("resChar: %s\n", resChar);
    }
    Expr *resExpr = getExprFromName(resChar, exprMap, true, -1);
    list_append(out_expr_list, resExpr);
    char *outChar = new char[SHORT_STRING_LEN];
    sprintf(outChar, "out%d", ii);
    list_append(out_expr_name_list, outChar);
    if (std::find(processedResIDs.begin(), processedResIDs.end(), resID)
        != processedResIDs.end()) {
      continue;
    }
    ihash_bucket_t *b_width;
    b_width = ihash_add(out_width_map, (long) resExpr);
    unsigned bw = outBWList[ii];
    b_width->i = (int) bw;
    processedResIDs.push_back(resID);
  }
  /* Call the logic optimizer */
  if (debug_verbose) {
    listitem_t *li;
    printf("in_expr_bundle:\n");
    for (li = list_first (in_expr_list); li; li = list_next (li)) {
      long key = (long) list_value(li);
      char *val = (char *) ihash_lookup(in_expr_map, key)->v;
      int bw = ihash_lookup(in_width_map, key)->i;
      printf("key: %ld, val: %s, bw: %d\n", key, val, bw);
      Expr *e = (Expr *) list_value (li);
      print_expr(stdout, e);
      printf("\n");
    }
    printf("\nout_expr_bundle:\n");
    for (li = list_first (out_expr_list); li; li = list_next (li)) {
      long key = (long) list_value(li);
      int bw = ihash_lookup(out_width_map, key)->i;
      printf("key: %ld, bw: %d\n", key, bw);
      Expr *e = (Expr *) list_value (li);
      print_expr(stdout, e);
      printf("\n");
    }
    for (li = list_first (out_expr_name_list); li; li = list_next (li)) {
      char *outName = (char *) list_value(li);
      printf("outName: %s\n", outName);
    }
    printf("\nhidden expr:\n");
    for (li = list_first (hidden_expr_list); li; li = list_next (li)) {
      Expr *e = (Expr *) list_value (li);
      print_expr(stdout, e);
      printf("\n");
      long key = (long) list_value(li);
      int bw = ihash_lookup(out_width_map, key)->i;
      printf("key: %ld, bw: %d\n", key, bw);
    }
    printf("\n");
  }
  const char *normInstance = getNormInstanceName(instance);
  if (debug_verbose) {
    printf("Run logic optimizer for %s\n", normInstance);
  }
  char *rtlModuleName = new char[strlen(normInstance) + 7];
  sprintf(rtlModuleName, "%s_logic", normInstance);
  char *optimized_netlist_file =
      new char[strlen(rtlModuleName) + strlen(custom_fu_dir) + 16];
  sprintf(optimized_netlist_file, "%s/%s.act", custom_fu_dir, normInstance);
  const  char *software = "yosys";
  if (ExternalExprOpt::engineExists ("genus")) {
     software = "genus";
  }
  bool tie_cells = false;

  // This should be set in the expropt config file
  //char *act_home = getenv("ACT_HOME");
  //char* stdcell_path = new char[strlen(act_home) + 128];
  //sprintf(stdcell_path, "%s/act/std/cells.act", act_home);
  //config_set_string("expropt.act_cell_lib_bd", stdcell_path);
  //config_set_string("expropt.act_cell_lib_bd_namespace", "std::cells");
  auto optimizer =
      new ExternalExprOpt(software, bd, tie_cells, optimized_netlist_file);
  ExprBlockInfo *info = optimizer->run_external_opt(rtlModuleName,
                                                    in_expr_list,
                                                    in_expr_map,
                                                    in_width_map,
                                                    out_expr_list,
                                                    out_expr_name_list,
                                                    out_width_map,
                                                    hidden_expr_list,
                                                    hidden_expr_name_list);
  /* copy the optimized netlist file to the cache */
  char *errMsg = new char[128];
  sprintf(errMsg,
          "Fail to copy the optimized netlist file to the cache directory!\n");
  copyFileToTargetDir(optimized_netlist_file, cache_dir, errMsg);
  if (debug_verbose) {
    printf("Generated block %s: Area: %e m2, Dyn Power: %e W, "
           "Leak Power: %e W, delay: %e s\n",
           normInstance,
           info->getArea(),
           info->getDynamicPower().typ_val,
           info->getStaticPower().typ_val,
           info->getDelay().typ_val);
  }

  double leakpower, energy, delay, area;

  if (!info->exists()) {
    /* wires */
    leakpower = 0;
    energy = 0;
    delay = 0;
    area = 0;
  }
  else {
    leakpower = info->getStaticPower().typ_val * 1e9;  // Leakage power (nW)

    //energy = info->power_typ_dynamic * info->delay_typ * 1e15; (fJ)
    energy = info->getDynamicPower().typ_val *
      config_get_real ("expropt.dynamic_power_period") * 1e15; // fJ
    
    delay = info->getDelay().typ_val * 1e12; // Delay (ps)
    area = info->getArea() * 1e12;  // AREA (um^2)
  }

  
  /* XXX:

     check model here
  */
  int n_inputs = inBW.size();
  double avg_bw = 0;

  for (auto &inBWIt: inBW) {
    avg_bw += inBWIt.second;
  }
  avg_bw /= n_inputs;

  char *procname = new char[MAX_INSTANCE_LEN];
  snprintf (procname, MAX_INSTANCE_LEN, "lib::func<%d,1,%d>",
	    n_inputs, (int)avg_bw );

  double *ctrlmetric = getOpMetric (procname);

  if (!ctrlmetric) {
    snprintf (procname, MAX_INSTANCE_LEN, "lib::func<%d,1,1>", n_inputs);
    ctrlmetric = calcMetric (procname, (int)avg_bw);

    if (!ctrlmetric) {
      warning ("Update calibration to include %d-input functions", n_inputs);

      int num = n_inputs - 1;

      while (!ctrlmetric && num) {
	snprintf (procname, MAX_INSTANCE_LEN, "lib::func<%d,1,1>", num);
	ctrlmetric = calcMetric (procname, (int)avg_bw);
	num--;
      }
      if (!num) {
	fatal_error ("No function metrics?");
      }
      num++;
      warning ("Substituting %d-input function metrics instead of %d-input",
	       num, n_inputs);
    }
  }

  metric = new double[4];
  metric[0] = leakpower;
  metric[1] = energy;
  metric[2] = delay;
  metric[3] = area;

  /* cache has raw numbers, without the func template */
  writeLocalMetricFile(instance, metric);
  writeCachedMetricFile(instance, metric);
  updateMetrics(instance, metric);

  area = area + ctrlmetric[METRIC_AREA];
  leakpower = leakpower + ctrlmetric[METRIC_LEAK_POWER];
  energy = energy + ctrlmetric[METRIC_DYN_ENERGY];
  delay = delay + ctrlmetric[METRIC_DELAY];

  /* get the final metric */
  metric = new double[4];
  metric[0] = leakpower;
  metric[1] = energy;
  metric[2] = delay;
  metric[3] = area;
#endif
}

double *Metrics::getOrGenFUMetric(
#if LOGIC_OPTIMIZER
    StringMap<unsigned> &inBW,
    StringMap<unsigned> &hiddenBW,
    Map<const char *, Expr *> &exprMap,
    Map<Expr *, Expr *> &hiddenExprs,
    Map<unsigned int, unsigned int> &outRecord,
    UIntVec &outBWList,
#endif
    const char *instance) {
  if (!_have_metrics) {
    return NULL;
  }
  double *metric = getOpMetric(instance);
  if (!metric) {
    metric = getCachedMetric(instance);
  }
  if (metric) {
#if LOGIC_OPTIMIZER
    int n_inputs = inBW.size();
    double avg_bw = 0;

    for (auto &inBWIt: inBW) {
      avg_bw += inBWIt.second;
    }
    avg_bw /= n_inputs;

    char *procname = new char[MAX_INSTANCE_LEN];
    snprintf (procname, MAX_INSTANCE_LEN, "lib::func<%d,1,%d>",
	      n_inputs, (int)avg_bw );

    double *ctrlmetric = getOpMetric (procname);

    if (!ctrlmetric) {
      snprintf (procname, MAX_INSTANCE_LEN, "lib::func<%d,1,1>", n_inputs);
      ctrlmetric = calcMetric (procname, (int)avg_bw);

      if (!ctrlmetric) {
	warning ("Update calibration to include %d-input functions", n_inputs);

	int num = n_inputs - 1;

	while (!ctrlmetric && num) {
	  snprintf (procname, MAX_INSTANCE_LEN, "lib::func<%d,1,1>", num);
	  ctrlmetric = calcMetric (procname, (int)avg_bw);
	  num--;
	}
	if (!num) {
	  fatal_error ("No function metrics?");
	}
	num++;
	warning ("Substituting %d-input function metrics instead of %d-input",
		 num, n_inputs);
      }
    }
    double *tmp_met = metric;
    metric = new double [4];
    for (int i=0; i < 4; i++) {
      metric[i] = tmp_met[i] + ctrlmetric[i];
    }
#endif
  }

  if (!metric) {
#if LOGIC_OPTIMIZER
    callLogicOptimizer(instance,
                       inBW,
                       hiddenBW,
                       exprMap,
                       hiddenExprs,
                       outRecord,
                       outBWList,
                       metric);
#endif
  }
  if (metric) {
    updateStatistics(instance, metric);
  }
  return metric;
}

double *Metrics::getOrGenMergeMetric(unsigned guardBW,
                                     unsigned inBW,
                                     unsigned numIn) {
  if (!_have_metrics) {
    return NULL;
  }

  char *procName = new char[MAX_INSTANCE_LEN];
  
  const char *instance =
      NameGenerator::genMergeInstName(guardBW, inBW, numIn, procName);
  double *metric = getOpMetric(instance);

  if (!metric) {
    const char *equivInstance =
      NameGenerator::genMergeInstName(guardBW, inBW, (1 << guardBW), procName);
    metric = getOpMetric (equivInstance);

    if (!metric) {
      const char *basicInstance =
	NameGenerator::genMergeInstName(guardBW, 1, (1 << guardBW), procName);

      metric = calcMetric (basicInstance, inBW);

      if (!metric) {
	printf("No enough info to calculate metric for %s or %s!\n",
	       equivInstance, instance);
	exit(-1);
      }
    }
  }
  updateStatistics(instance, metric);
  updateMergeMetrics(metric);
  return metric;
}

double *Metrics::getOrGenSplitMetric(unsigned guardBW,
                                     unsigned inBW,
                                     unsigned numOut) {
  if (!_have_metrics) {
    return NULL;
  }
  
  char *procName = new char[MAX_INSTANCE_LEN];
  const char *instance =
      NameGenerator::genSplitInstName(guardBW, inBW, numOut, procName);
  double *metric = getOpMetric(instance);
  if (!metric) {
    const char *equivInstance =
      NameGenerator::genSplitInstName(guardBW, inBW, (1 << guardBW), procName);
    metric = getOpMetric(equivInstance);
    if (!metric) {
      const char *basicInstance =
	NameGenerator::genSplitInstName(guardBW, 1, (1 << guardBW), procName);

      metric = calcMetric (basicInstance, inBW);
      
      if (!metric) {
        printf("No enough info to calculate metric for %s or %s!\n",
               equivInstance, instance);
        exit(-1);
      }
    }
  }
  updateStatistics(instance, metric);
  updateSplitMetrics(metric);
  return metric;
}

double *Metrics::getArbiterMetric(unsigned numInputs,
                                  unsigned inBW,
                                  unsigned coutBW) {
  double *metric = getOrGenMergeMetric(coutBW, inBW, numInputs);
  return metric;
}

double *Metrics::getMixerMetric(unsigned numInputs,
                                unsigned inBW,
                                unsigned coutBW) {

  if (!_have_metrics) {
    return NULL;
  }

  char *procName = new char[MAX_INSTANCE_LEN];
  
  const char *instance =
    NameGenerator::genMixerInstName(coutBW, inBW, numInputs, procName);
  double *metric = getOpMetric(instance);

  if (!metric) {
    const char *basicInstance =
      NameGenerator::genMixerInstName(coutBW, 1, numInputs, procName);

    metric = calcMetric (basicInstance, inBW);

    if (!metric) {
      numInputs = 1 << (int) (ceil(log(numInputs)/log(2)));
      basicInstance = NameGenerator::genMixerInstName(coutBW, 1, numInputs, procName);
      metric = calcMetric (basicInstance, inBW);
      
      if (!metric) {
	printf("No enough info to calculate metric for %s or %s!\n",
	       basicInstance, instance);
	exit(-1);
      }
    }
  }
  updateStatistics(instance, metric);
  return metric;
}

void Metrics::updateCopyStatistics(unsigned bitwidth, unsigned numOutputs) {
  auto copyStatisticsIt = copyStatistics.find(bitwidth);
  if (copyStatisticsIt != copyStatistics.end()) {
    Map<unsigned, unsigned> &record = copyStatisticsIt->second;
    auto recordIt = record.find(numOutputs);
    if (recordIt != record.end()) {
      recordIt->second++;
    } else {
      record.insert({numOutputs, 1});
    }
  } else {
    Map<unsigned, unsigned> record = {{numOutputs, 1}};
    copyStatistics.insert({bitwidth, record});
  }
}

void Metrics::printStatistics() {
  if (debug_verbose) {
    printf("Print statistics to file: %s\n", statisticsFilePath);
  }
  FILE *statisticsFP = fopen(statisticsFilePath, "w");
  if (!statisticsFP) {
    printf("Could not create statistics file %s\n", statisticsFilePath);
    exit(-1);
  }
  fprintf(statisticsFP, "totalArea: %.2f, totalLeakPowewr: %.2f\n", totalArea,
          totalLeakPowewr);
  fprintf(statisticsFP, "Merge area: %.2f, ratio: %5.1f\n",
          mergeArea, ((double) 100 * mergeArea / totalArea));
  fprintf(statisticsFP, "Split area: %.2f, ratio: %5.1f\n",
          splitArea, ((double) 100 * splitArea / totalArea));
  fprintf(statisticsFP, "Merge LeakPower: %.2f, ratio: %5.1f\n",
          mergeLeakPower, ((double) 100 * mergeLeakPower / totalLeakPowewr));
  fprintf(statisticsFP, "Split LeakPower: %.2f, ratio: %5.1f\n",
          splitLeakPower, ((double) 100 * splitLeakPower / totalLeakPowewr));
  printAreaStatistics(statisticsFP);
  printLeakpowerStatistics(statisticsFP);
  fclose(statisticsFP);
}

void Metrics::printCopyStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "%s\n", "COPY STATISTICS:");
  for (auto &copyStatisticsIt: copyStatistics) {
    fprintf(statisticsFP, "%d-bit COPY:\n", copyStatisticsIt.first);
    Map<unsigned, unsigned> &record = copyStatisticsIt.second;
    for (auto &recordIt: record) {
      fprintf(statisticsFP, "  %u outputs: %u\n",
              recordIt.first, recordIt.second);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::updateMergeMetrics(double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  mergeArea += area;
  mergeLeakPower += leakPower;
}

void Metrics::updateSplitMetrics(double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  splitArea += area;
  splitLeakPower += leakPower;
}

void Metrics::updateStatistics(const char *instName, double metric[4]) {
  double area = getArea(metric);
  double leakPower = getLP(metric);
  totalArea += area;
  totalLeakPowewr += leakPower;
  bool exist = false;
  for (auto &areaStatisticsIt: areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instName)) {
      areaStatisticsIt.second += area;
      exist = true;
    }
  }
  if (exist) {
    bool foundLP = false;
    for (auto &leakpowerStatisticsIt: leakpowerStatistics) {
      if (!strcmp(leakpowerStatisticsIt.first, instName)) {
        leakpowerStatisticsIt.second += leakPower;
        foundLP = true;
        break;
      }
    }
    if (!foundLP) {
      printf(
          "We could find %s in areaStatistics, but not in leakpowerStatistics!\n",
          instName);
      exit(-1);
    }
    for (auto &instanceCntIt: instanceCnt) {
      if (!strcmp(instanceCntIt.first, instName)) {
        instanceCntIt.second += 1;
        return;
      }
    }
    printf("We could find %s in areaStatistics, but not in instanceCnt!\n",
           instName);
    exit(-1);
  } else {
    areaStatistics.insert({instName, area});
    leakpowerStatistics.insert({instName, leakPower});
    instanceCnt.insert({instName, 1});
  }
}

int Metrics::getInstanceCnt(const char *instance) {
  for (auto &instanceCntIt: instanceCnt) {
    if (!strcmp(instanceCntIt.first, instance)) {
      return instanceCntIt.second;
    }
  }
  printf("We could not find %s in instanceCnt!\n", instance);
  exit(-1);
}

double Metrics::getInstanceArea(const char *instance) {
  for (auto &areaStatisticsIt: areaStatistics) {
    if (!strcmp(areaStatisticsIt.first, instance)) {
      return areaStatisticsIt.second;
    }
  }
  printf("We could not find %s in instanceCnt!\n", instance);
  exit(-1);
}

void Metrics::printLeakpowerStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "Leak Power Statistics:\n");
  fprintf(statisticsFP, "totalLeakPower: %.2f\n", totalLeakPowewr);
  if (!leakpowerStatistics.empty() && (totalLeakPowewr == 0)) {
    printf("leakpowerStatistics is not empty, but totalLeakPowewr is 0!\n");
    exit(-1);
  }
  std::multimap<double, const char *>
      sortedLeakpowers = flip_map(leakpowerStatistics);
  for (auto iter = sortedLeakpowers.rbegin();
       iter != sortedLeakpowers.rend(); iter++) {
    double leakPower = iter->first;
    double ratio = (double) leakPower / totalLeakPowewr * 100;
    const char *instance = iter->second;
    if (ratio > 0.1) {
      int cnt = getInstanceCnt(instance);
      fprintf(statisticsFP, "%80.50s %5.1f %5.1f %5d\n", instance, leakPower,
              ratio, cnt);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::printAreaStatistics(FILE *statisticsFP) {
  fprintf(statisticsFP, "Area Statistics:\n");
  fprintf(statisticsFP, "totalArea: %.2f\n", totalArea);
  if (!areaStatistics.empty() && (totalArea == 0)) {
    printf("areaStatistics is not empty, but totalArea is 0!\n");
    exit(-1);
  }
  std::multimap<double, const char *> sortedAreas = flip_map(areaStatistics);
  fprintf(statisticsFP,
          "instance name      area     percentage     # of instances\n");
  for (auto iter = sortedAreas.rbegin(); iter != sortedAreas.rend(); iter++) {
    double area = iter->first;
    double ratio = (double) area / totalArea * 100;
    const char *instance = iter->second;
    if (ratio > 0.1) {
      int cnt = getInstanceCnt(instance);
      fprintf(statisticsFP,
              "%80.80s %5.2f %5.1f %5d\n",
              instance,
              area,
              ratio,
              cnt);
    }
  }
  fprintf(statisticsFP, "\n");
}

void Metrics::dump() {
  printStatistics();
}
