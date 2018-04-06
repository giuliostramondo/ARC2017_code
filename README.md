PolyMem - A Polymorphic Register File (PRF) MaxJ implementation
=============================

This repository contains the MaxJ implementation of the PRF and a testbench that allow the user to check the correctness of the design. 
The testbench is integrated in the Maxeler project and can be found in in the CPUCode folder. 
The PRF maxj files are located in APP/EngineCode/src/exampleproject.

How the testbench works
-----------------------

PolyMem is - in this design - linked to the host through PCI-express. The testbench first fills all the capacity of PolyMem doing parallel writes with increasing numbers. After that PolyMem is filled with those numbers it starts to perform all the possible combination of reads, with all the shapes available - defined by the mapping scheme that was set in the PRF configuration file. The number read from the PRF are sent to the host which, simulating the PRF, checks their correctness. 

Usage
-----

* Configuration of the PRF
Our PRF implementation allow the user to customize the capacity, number of lanes, mapping scheme and frequency at wich the design is synthesized. All those paramethers can be set in the PRF configuration file PRFConfiguration.maxj located in APP/EngineCode/src/exampleproject

* Compilation
To compile the whole project - PRF bitstream and testbench - go in the CPUCode folder and run:

```Shell
make RUNRULE=DFE distclean
make RUNRULE=DFE build
```

This process could take more than one hour if the bitstream has not been synthesized yet.

* Running the testbench
Once the design is compiled the testbench binary will be located in /RunRule/DFE/binaries and called ExampleProject. The testbench needs to run with a set of parameters that describe how the PRF was configured. It is possible to do this when running the testbench setting the following parameters:
```
    -N <num>     Change the horizontal size of the input matrix (default 256)
    -M <num>     Change the vertical size of the input matrix (default 256)
    -p <num>     Change the vertical size of the PRF (default 2)
    -q <num>     Change the horizontal size of the PRF (default 4)
    -s <num>     Change the scheme used by the PRF (default 1 -> ReRo)
                  other schemes 0->ReO, 2->ReCo, 3->RoCo, 4->ReTr
```

You can test the default configuration of PolyMem ( the one preset in this repository ) running the testbench from the APP folder with no arguments (the default value match the default configuration of PolyMem):
```
./RunRules/DFE/binaries/ExampleProject
```


