# dflowmap

This tool is used to
map [ACT dataflow](https://avlsi.csl.yale.edu/act/doku.php?id=language:langs:dflow)
program into circuit backend description.

## System requirements:

* The system must have [ACT](https://github.com/asyncvlsi/act) installed
* If you want to do logic optimization for ACT dataflow expressions, the system
  must have [expression optimizer](https://github.com/asyncvlsi/expropt)
  installed

## Build instructions:

* in `config.in`, specify which logic opimizer is available. If no optimizer is
  available, leave it blank. If an open-source optimizer is available, specify
  it as `expropt`. If a commercial optimizer is available, then add
  `exproptcommercial` in a new line after `expropt`
* to compile the tool, run `sh comile.sh`, and the binary is located at
  `build/dflowmap`
* to generate CHP description for the test file `tests/dfadd.act`, first copy
  the file to the home directory of dflowmap, and
  run `build/dflowmap tests/dfadd.act`, and there will be three output files:
  * lib_dfadd.act: CHP process library
  * conf_dfadd.act: specify the performance (delay, area,
    leakage power, execution energy) of each generated CHP process
  * result_dfadd.act: CHP instances
