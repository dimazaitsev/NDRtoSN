## `NDRtoSN: Convert from NDR to *SN`

# Convertor of Tina NDR Petri net file to Sleptsov Net file


How to use `NDRtoSN` as a part of experimental `SNC IDE&VM`:
--------------------------------------------------------

We list references to other components in "Compatibility" section.

1) Use `Tina` `nd` as graphical editor and its labels with special syntax (section "Transition substitution label") to specify transition substitution of `HSN`.

2) Use `NDRtoSN` to convert `NDR` file of `Tina` into `HSN` or `LSN`. 

3) Use `HSNtoLSN` to compile and link HSN file and mentioned in it `LSN` files into a single `LSN` file.

4) Run `LSN` file on `SN-VM` or `SN-VM-GPU`.


Compatibility: 
-------------- 

`Tina`, `nd`, and `NDR` file format according to https://projects.laas.fr/tina/index.php

`SN-VM` and `LSN` file format according to https://github.com/zhangq9919/Sleptsov-net-processor

`HSNtoLSN` and `HSN` file format according to https://github.com/HfZhao1998/Compiler-and-Linker-of-Sleptsov-net-Program

`SN-VM-GPU` and `MSN` file format according to https://github.com/tishtri/SN-VM-GPU


Command line format: 
-------------------- 

   >NDRtoSN NDR_file_name LSN_file_name 
   
   >NDRtoSN NDR_file_name HSN_file_name 
   
File type HSN/LSN is chosen based on the presence of transition substitution labels.
   
   
Examples of command lines: 
-------------------------- 

   >NDRtoSN fmul.ndr fmul.lsn
   
   >NDRtoSN add2.ndr add2.hsn
  
  
Transition substitution label:
------------------------------

*HSN(snname i hpname lpnum ... o hpname lpnum ... s pname lpnum f hpname lpnum)

"*HSN" - prefix of HSN label, should be at the beginning of the label

"(" - begin of specification

")" - end of specification

snname - name of subnet

i,o,s,f - labels of place type for place mapping: input, otput, start, finish, respectively

hpname - name of HSN place

lpnum - number of LSN place
   
   
References: 
----------- 
1. Zaitsev D.A. Sleptsov Nets Run Fast, IEEE Transactions on Systems, Man, and Cybernetics: Systems, 2016, Vol. 46, No. 5, 682 - 693. http://dx.doi.org/10.1109/TSMC.2015.2444414

2. Zaitsev D.A., Jürjens J. Programming in the Sleptsov net language for systems control, Advances in Mechanical Engineering, 2016, Vol. 8(4), 1-11. https://doi.org/10.1177%2F1687814016640159

3. Zaitsev D.A. Universal Sleptsov Net, International Journal of Computer Mathematics, 94(12) 2017, 2396-2408. http://dx.doi.org/10.1080/00207160.2017.1283410

4. Dmitry A. Zaitsev, Strong Sleptsov nets are Turing complete, Information Sciences, Volume 621, 2023, 172-182. https://doi.org/10.1016/j.ins.2022.11.098

5. Qing Zhang, Ding Liu, Yifan Hou, Sleptsov Net Processor, International Conference ”Problems of Infocommunications. Science and Technology” (PICST2022), 10-12 October, 2022, Kyiv, Ukraine.

6. Hongfei Zhao, Ding Liu, Yifan Hou, Compiler and Linker of Sleptsov Net Program,International Conference ”Problems of Infocommunications. Science and Technology” (PICST2022), 10-12 October, 2022, Kyiv, Ukraine.

7. Sleptsov Net Computing Resolves Modern Supercomputing Problems, The April 21, 2023, edition of ACM TechNews, https://technews.acm.org/archives.cfm?fo=2023-04-apr/apr-21-2023.html

----------------------------------------------------------------------- 
@ 2023 Dmitry Zaitsev: daze@acm.org 
