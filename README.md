# DRAM-Controller

The DRAM Controller was a project I created for my graduate level advanced microprocessor class.
The project simulates a DRAM controller for a 4GB DDR4 DRAM. The project went through rigorous 
testing at the hands of my professor, Mark Faust, who fed the controller text files with a simulation
or DRAM requests from the cache controller.

Prior to demoing the project, some members of my time created test files that simulated DRAM requests coming from
the cache controller. They submitted bugs and I fixed the controller code. When we were finished, the controller
correctly implemented an open page policy but did not include refresh.

I also created an excel spreadsheet that would encode/decode any 32-bit address into the DRAM specific row/column/bank
information in an effort to verify correct address encoding/decoding.
