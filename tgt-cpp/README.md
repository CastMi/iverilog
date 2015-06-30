tgt-cpp
============

What is this?
--------------

I'm developing an Icarus Verilog target in order to print out C++ code.
The generated code will be C++11 compliant.

How can I simulate it?
--------------

The idea is to take advantage of the Warped2 simulation kernel:
https://github.com/wilseypa/warped2
Released under MIT License (MIT).

How can I compile it?
--------------

In general, the following commands should be sufficient:
			./configure
			make
Use the Icarus Verilog Readme in the root directory for any problem.

How can I use it?
--------------

Using, for instance, a file "circuit.v":
        iverilog -tcpp circuit.v
