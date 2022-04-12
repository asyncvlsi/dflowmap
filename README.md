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
* to compile the tool, run `sh build.sh`, and the binary is located at
  `build/src/dflowmap`

## Running instructions
Suppose we want to generate CHP description for the dfadd benchmark located at
`tests/dfadd.act`. Run `bin/dflowmap dfadd.act` in the home directory,
 and four output files will be generated in the same path with the input act file:
  * dfadd_lib.act: library of CHP processes used by dfadd
  * dfadd_chp.act: instances of CHP processes synthesized for dfadd. It invokes 
  `dfadd_lib.act` for the implementation of each CHP process
  * dfadd.conf: configuration file which specifues the performance (delay, area,
    leakage power, execution energy) of each CHP process
  * dfadd.stat: performance statistics of the generated CHP instances