# CBA - Chip8 Basic Assembler

A two-pass assembler for the Chip8 virtual console. Originally an extracurricular project relating to a Computer Hardware and Operating Systems module.

Built to explore "ROM" development for a limited/restricted system, the language can be used to develop any type of game so long as it satisfies the constraints of the Chip8 system.

The CBA language is based upon the Chip8 technical references found in the following sites:
* [Cowgod's Chip-8 Technical Reference v1.0](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM) 
* [Mastering CHIP-8 by Matthew Mikolay](http://mattmik.com/files/chip8/mastering/chip8.html)
* [CHIP-8 - Wikipedia](https://en.wikipedia.org/wiki/CHIP-8)

The CBA executable can be run as a command line tool which takes a single argument of the source file location (Of any extension).
If assembly is successful, CBA returns 0. If any errors were encountered, CBA returns 1.

All asm mnemonics can be found in LANGUAGE.txt

Last compiled: 3rd December 2017

Compiled using Visual Studio 2017 (version 15.4.0)
