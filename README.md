# dflowmap
This tool is used to map [ACT dataflow](https://avlsi.csl.yale.edu/act/doku.php?id=language:langs:dflow) program into circuit backend 
description.
## System requirements:
* The system must have [ACT](https://github.com/asyncvlsi/act) installed
* If you want to do logic optimization for ACT dataflow expressions, the 
  system must have [expression optimizer](https://github.com/asyncvlsi/expropt) installed

## Build instructions:
* in `config.in`, specify which logic opimizer is available. If no optimizer 
  is available, leave it blank. If an open-source optimizer is available, 
  specify it as `expropt`. If a commercial optimizer is available, then add 
  `exproptcommercial` in a new line after `expropt`
* to compile the tool, run `sh comile.sh`, and the binary is located at 
  `build/dflowmap`
* to run the tool, run `build/dflowmap tests/dfadd.act`, and the tool will 
  generate CHP description for `tests/dfadd.act`. The CHP instances can be 
  found in `result_dfadd.act`, and the CHP libraries can be found in 
  `lib_dfadd.act`. The tool also generates a configuration file `conf_dfadd.
  act`, which is used to specify the performance (delay, area, leakage power,
  execution energy) of each generated CHP process.
