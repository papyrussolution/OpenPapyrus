# AVX Memmove
Highly optimized versions of memmove, memcpy, memset, and memcmp supporting SSE4.2, AVX, AVX2, and AVX512.  
  
This is an effort to make the fastest possible versions for AVX2+ supporting systems, so if you see a way to make any of them better (for any data size, not just big ones), please post in "Issues" or make a pull request. *NOTE: SSE4.2 support is mainly implemented as a fallback, as not all AVX functions used are available in AVX1 (meaning Intel Sandy Bridge and Ivy Bridge systems need the SSE4.2 stuff).*
  
At the moment, AVX_memmove currently beats GCC -O3 optimized (and vectorized) regular memmove at sizes >1024 bytes with aligned destinations. It also beats some similarly optimized memcpy implementations at 4096 and 8192 bytes with aligned destinations.
  
These are the same files used in https://github.com/KNNSpeed/Simple-Kernel, so they have been primarily designed to operate in a freestanding (no-OS) x86-64 environment. They also work in a regular OS environment just fine. :)
  
## Compilation Requirements 
 
**Windows:** MinGW-w64 based on GCC 8.1.0 or later  
**Mac:** Mac OS Sierra or later with the latest XCode Command Line Tools for the OS  
**Linux:** GCC 8.0.0 or later and Binutils 2.29.1 or later  

Compile and cleanup scripts are provided for Linux, Windows, and Mac, but you'll need to supply your own C file with main() and modify the compile script to add it to the linker line. You may also need to modify the GCC/clang folder location in the script for your system. 

To use them, just run the scripts from the same folder that the .h & .c files are in. That's it! You'll get an executable file called Output.(exe/elf/mach64) in the same folder.

See https://github.com/KNNSpeed/Simple-Kernel for how to make portable GCC 8+ installs on Windows and Linux. MacOS just uses the built-in clang. However, if you have a Linux version with GCC 8+ built-in already (run "gcc --version" in terminal to check), then you just need to modify Compiler.sh to point to it.
  
## License and Crediting  
  
Please see the LICENSE file for the complete terms and further information.
  
***TL;DR:***  

If you don't give credit to this project, per the license you aren't allowed to do anything with any of its source code that isn't already covered by an existing license (in other words, my license covers most of the code I wrote). That's pretty much it, and why it's "almost" PD, or "PD with Credit" if I have to give it a nickname, as there's no restriction on what it gets used for as long as the license is satisfied. If you have any issues, feature requests, etc. please post in "Issues" so it can be attended to/fixed.  

Note that many of these files already have appropriate crediting inside, so you could just leave what's already there to satisfy the terms for those files. You really should see the license file for complete information, though (it's short!!).  

## Acknowledgements  

- [Intel Corporation](https://www.intel.com/content/www/us/en/homepage.html) for the amazingly useful [Intel Intrinsics Guide](https://software.intel.com/sites/landingpage/IntrinsicsGuide/) and for publishing [software optimization manuals and architecture programming references](https://software.intel.com/en-us/articles/intel-sdm).
- [The Data Plane Development Kit project](https://www.dpdk.org/about/) for open-source examples of various really useful optimizations.
- [Agner Fog](https://www.agner.org/) for [amazing software optimization resources](https://www.agner.org/optimize/).
- Users [Peter Cordes](https://stackoverflow.com/users/224132/peter-cordes) and [BeeOnRope](https://stackoverflow.com/users/149138/beeonrope) on StackOverflow for incredibly detailed answers to questions about x86, which were very helpful to come across when researching x86 optimization. [(Like this one.)](https://stackoverflow.com/questions/4537753/when-should-i-use-mm-sfence-mm-lfence-and-mm-mfence/50780314#50780314)
