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

#ifndef DFLOWMAP_METRICS_H
#define DFLOWMAP_METRICS_H

#include <cstdio>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <act/act.h>
#include "src/common/common.h"
#include "src/common/Helper.h"
#include "src/common/config.h"
#include "src/core/NameGenerator.h"
#if LOGIC_OPTIMIZER
#include <act/expropt.h>
#endif


#define METRIC_LEAK_POWER 0
#define METRIC_DYN_ENERGY 1
#define METRIC_DELAY      2
#define METRIC_AREA       3


/*
 * Model for metrics
 */
class MetricGen {

private:
  struct m_eqn {
    double t_log, t_lin;
  } m[4];
  double t_const[4];

  // reeturn metric given bit-width
  double getEqn (int id, int bw) {
    assert (0 <= id && id < 4);
    struct m_eqn *x = &m[id];
    double x_const = t_const[id];
    int lg;

    if (bw > 0) {
      lg = ceil(log(bw)/log(2));
      if (lg < 1) {
	lg = 1;
      }
    }
    else {
      lg = 0;
    }
    
    return x_const + x->t_log*lg + x->t_lin*bw;
  }

public:

  MetricGen() {
    for (int i=0; i < 4; i++) {
      m[i].t_log = 0;
      m[i].t_lin = 0;
      t_const[i] = 0;
    }
  }

  MetricGen (double *d) {
    for (int i=0; i < 4; i++) {
      t_const[i] = d[i];
      m[i].t_log = 0;
      m[i].t_lin = 0;
    }
  }

  void populateDouble (double *d) {
    for (int i=0; i < 4; i++) {
      d[i] = t_const[i];
    }
  }

  double *getConst () {
    return t_const;
  }
     
  bool operator==(MetricGen &x) {
    for (int i=0; i < 4; i++) {
      if (x.t_const[i] != t_const[i]) return false;
      if (x.m[i].t_log != m[i].t_log) return false;
      if (x.m[i].t_lin != m[i].t_lin) return false;
    }
    return true;
  }
	
  bool readMetrics (FILE *fp) {
    for (int i=0; i < 4; i++) {
      if (fscanf (fp, "c=%lg,lg=%lg,li=%lg",
		  &t_const[i], &m[i].t_log, &m[i].t_lin) != 3) {
	if (fscanf (fp, "%lg", &t_const[i]) != 1) {
	  return false;
	}
	else {
	  m[i].t_log = 0;
	  m[i].t_lin = 0;
	}
      }
    }
    return true;
  }

  bool readOneMetrics (int i, const char *s) {
    assert (0 <= i && i < 4);
    if (sscanf (s, "c=%lg,lg=%lg,li=%lg",
		&t_const[i], &m[i].t_log, &m[i].t_lin) != 3) {
      if (sscanf (s, "%lg", &t_const[i]) != 1) {
	return false;
      }
      else {
	m[i].t_log = 0;
	m[i].t_lin = 0;
      }
    }
    return true;
  }

  void writeMetrics (FILE *fp) {
    for (int i=0; i < 4; i++) {
      fprintf (fp, "c=%lg,lg=%lg,li=%lg",
	       t_const[i], m[i].t_log, m[i].t_lin);
      if (i != 3) {
	fprintf (fp, " ");
      }
    }
  }

  char *metricToString (int i) {
    char buf[1024];
    assert (0 <= i && i < 4);
    snprintf (buf, 1024, "c=%g,lg=%g,li=%g",
	      t_const[i], m[i].t_log, m[i].t_lin);
    return Strdup (buf);
  }

  double getEnergy (int bw) { return getEqn (METRIC_DYN_ENERGY, bw); }
  double getLeak (int bw) { return getEqn (METRIC_LEAK_POWER, bw); }
  double getArea (int bw) { return getEqn (METRIC_AREA, bw); }
  double getDelay (int bw) { return getEqn (METRIC_DELAY, bw); }
};
      
class Metrics {
 public:
  Metrics(const char *customFUMetricsFP,
          const char *stdFUMetricsFP,
          const char *statisticsFP);

  void readMetricsFile();

private:
  void updateMetrics(const char *instance, double *metric);
  void updateTemplMetrics(const char *instance, MetricGen *metric);
  double *calcMetric (const char *templ, int bitwidth);

  void updateCachedMetrics(const char *instance,  double *metric);

  void updateCopyStatistics(unsigned bitwidth, unsigned numOutputs);

  void updateStatistics(const char *instName, double *metric);

  void printOpMetrics();

  int getInstanceCnt(const char *instance);

  double getInstanceArea(const char *instance);

  void writeLocalMetricFile(const char *instance, double *metric);

  void writeCachedMetricFile(const char *instance, double *metric);

  void updateMergeMetrics(double *metric);

  void updateSplitMetrics(double *metric);

  static unsigned getEquivalentBW(unsigned oriBW);

  double *getOpMetric(const char *instance);

  // to be fixed!
  double *getxOpMetric(const char *instance);

  MetricGen *getTemplMetric (const char *instance);

  double *getCachedMetric(const char *instance);

  void callLogicOptimizer(
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
  );

public:
  
  void dump();

  double *getOrGenFUMetric(
#if LOGIC_OPTIMIZER
      StringMap<unsigned> &inBW,
      StringMap<unsigned> &hiddenBW,
      Map<const char *, Expr *> &exprMap,
      Map<Expr *, Expr *> &hiddenExprs,
      Map<unsigned int, unsigned int> &outRecord,
      UIntVec &outBWList,
#endif
      const char *instance);

  double *getSourceMetric(int bw);

  double *getOrGenMergeMetric(unsigned guardBW, unsigned inBW, unsigned numIn);

  double *getOrGenSplitMetric(unsigned guardBW, unsigned inBW, unsigned numOut);

  double *getArbiterMetric(unsigned numInputs, unsigned inBW, unsigned coutBW);

  double *getMixerMetric(unsigned numInputs,
			    unsigned inBW,
			    unsigned coutBW);

  double *getOrGenCopyMetric(unsigned bitwidth, unsigned numOut);

  double *getSinkMetric(int bw);

  double *getOrGenInitMetric(unsigned bitwidth);

  double *getBuffMetric(unsigned nBuff, unsigned bw);

  
  bool validMetrics() { return _have_metrics; }


 private:

  bool _have_metrics;
  
  /* operator, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2)) */
  Map<const char *, double *> opMetrics;
  Map<const char *, double *> cachedMetrics;
  Map<const char *, MetricGen *> templMetrics;

  /* copy bitwidth,< # of output, # of instances of this COPY> */
  Map<unsigned, Map<unsigned, unsigned >> copyStatistics;

  double totalArea;

  /* instanceName, area (um^2) of all of the instances of the process */
  Map<const char *, double> areaStatistics;

  double totalLeakPowewr;

  /* instanceName, LeakPower (nW) of all of the instances of the process */
  Map<const char *, double> leakpowerStatistics;

  double mergeArea;

  double splitArea;

  double mergeLeakPower;

  double splitLeakPower;

  /* instanceName, # of instances */
  Map<const char *, int> instanceCnt;

  const char *custom_metrics;

  const char *std_metrics;

  const char *statisticsFilePath;

  void printLeakpowerStatistics(FILE *statisticsFP);

  void printAreaStatistics(FILE *statisticsFP);

  void printCopyStatistics(FILE *statisticsFP);

  void printStatistics();

  static double getArea(double metric[4]);

  static double getLP(double metric[4]);

  static double getEnergy(double metric[4]);

  static double getDelay(double metric[4]);

  void readMetricsFile(const char *metricsFP, bool forCache, bool stdfile);
};

#endif //DFLOWMAP_METRICS_H
