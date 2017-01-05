/***********************  intel_mkl_feature_patch.c  **************************
* Author:           Agner Fog
* Date created:     2014-07-30
* Source URL:       http://www.agner.org/optimize/#asmlib
* Project:          asmlib.zip
* Language:         C or C++
*
* Description:
* Patch for Intel Math Kernel Library (MKL) version 14.0 and later, except
* the Vector Math Library (VML).
*
* Example of how to patch Intel's CPU feature dispatcher in order to improve 
* compatibility of Intel function libraries with non-Intel processors.
* In Windows: Use the static link libraries (*.lib), not the dynamic link
* librarise (*.DLL). 
* In Linux: use static linking (*.a) or dynamic linking (*.so).
*
* Include this code in your C or C++ program and call intel_mkl_patch();
* before any call to the MKL functions.
*
* See the manual in asmlib.zip for details.
* Copyright (c) 2014 GNU LGPL License v. 3.0 www.gnu.org/licenses/lgpl.html
******************************************************************************/


#ifdef __cplusplus  // use C-style linking
extern "C" {
#endif

// link to MKL libraries
extern long long __intel_mkl_feature_indicator;       // CPU feature bits
extern long long __intel_mkl_feature_indicator_x;     // CPU feature bits
void __intel_mkl_features_init();                     // checks CPU features for Intel CPU's only
void __intel_mkl_features_init_x();                   // checks CPU features without discriminating by CPU brand
// (Note: If you want to make your own version of __intel_mkl_features_init() you need assembly to save all registers)

#ifdef __cplusplus
}  // end of extern "C"
#endif

void intel_mkl_patch() {
    // force a re-evaluation of the CPU features without discriminating by CPU brand
    __intel_mkl_feature_indicator = 0;
    __intel_mkl_feature_indicator_x = 0;
    __intel_mkl_features_init_x();
    __intel_mkl_feature_indicator = __intel_mkl_feature_indicator_x;
}
