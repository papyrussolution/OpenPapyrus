/*
   Fast Artificial Neural Network Library (fann)
   Copyright (C) 2003-2016 Steffen Nissen (steffen.fann@gmail.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* This file defines the user interface to the fann library.
   It is included from fixedfann.h, floatfann.h and doublefann.h and should
   NOT be included directly. If included directly it will react as if
   floatfann.h was included.
 */

/* Section: FANN Creation/Execution

   The FANN library is designed to be very easy to use.
   A feedforward ann can be created by a simple <fann_create_standard> function, while
   other ANNs can be created just as easily. The ANNs can be trained by <fann_train_on_file>
   and executed by <fann_run>.

   All of this can be done without much knowledge of the internals of ANNs, although the ANNs created will
   still be powerful and effective. If you have more knowledge about ANNs, and desire more control, almost
   every part of the ANNs can be parametrized to create specialized and highly optimal ANNs.
 */
/* Group: Creation, Destruction & Execution */

#define FANN_VERSION "2.2.0"
//
//typedef float fann_type__Removed;

#undef FLOATFANN
#define FLOATFANN
#define FANNPRINTF "%.20e"
#define FANNSCANF "%f"
//
//#ifndef FANN_INCLUDE
#if 0 
	// just to allow for inclusion of fann.h in normal stuations where only floats are needed 
	#ifdef FIXEDFANN
		#include "fixedfann.h"
	#else
		//#include "floatfann.h"
	#endif
#else
// COMPAT_TIME REPLACEMENT 
#ifndef _WIN32
	#include <sys/time.h>
#else
	#if !defined(_MSC_EXTENSIONS) && !defined(_INC_WINDOWS)
		extern ulong __stdcall GetTickCount(void);
	#else
		//#define WIN32_LEAN_AND_MEAN
		//#include <windows.h>
	#endif
#endif

#ifndef __fann_h__
#define __fann_h__

#ifdef __cplusplus
extern "C" {
#ifndef __cplusplus
} /* to fool automatic indention engines */
#endif
#endif  /* __cplusplus */
//#ifndef NULL
	//#define NULL 0
//#endif

/* ----- Macros used to define DLL external entrypoints ----- */
/*
   DLL Export, import and calling convention for Windows.
   Only defined for Microsoft VC++ FANN_EXTERNAL indicates
   that a function will be exported/imported from a dll
   FANN_API ensures that the DLL calling convention
   will be used for  a function regardless of the calling convention
   used when compiling.

   For a function to be exported from a DLL its prototype and
   declaration must be like this:
    FANN_EXTERNAL void FANN_API function(char *argument)

   The following ifdef block is a way of creating macros which
   make exporting from a DLL simple. All files within a DLL are
   compiled with the FANN_DLL_EXPORTS symbol defined on the
   command line. This symbol should not be defined on any project
   that uses this DLL. This way any other project whose source
   files include this file see FANN_EXTERNAL functions as being imported
   from a DLL, whereas a DLL sees symbols defined with this
   macro as being exported which makes calls more efficient.
   The __stdcall calling convention is used for functions in a
   windows DLL.

   The callback functions for fann_set_callback must be declared as FANN_API
   so the DLL and the application program both use the same
   calling convention.
 */
/*
   The following sets the default for MSVC++ 2003 or later to use
   the fann dll's. To use a lib or fixedfann.c, floatfann.c or doublefann.c
   with those compilers FANN_NO_DLL has to be defined before
   including the fann headers.
   The default for previous MSVC compilers such as VC++ 6 is not
   to use dll's. To use dll's FANN_USE_DLL has to be defined before
   including the fann headers.
 */
/* @sobolev
#if defined(_MSC_VER) && (_MSC_VER > 1300)
	#ifndef FANN_NO_DLL
		#define FANN_USE_DLL
	#endif
#endif
#if defined(_MSC_VER) && (defined(FANN_USE_DLL) || defined(FANN_DLL_EXPORTS))
	#ifdef FANN_DLL_EXPORTS
		#define FANN_EXTERNAL __declspec(dllexport)
	#else
		#define FANN_EXTERNAL __declspec(dllimport)
	#endif
	#define FANN_API __stdcall
#else
	#define FANN_EXTERNAL
	#define FANN_API
#endif
*/
#define FANN_EXTERNAL // @sobolev
#define FANN_API      // @sobolev
/* ----- End of macros used to define DLL external entrypoints ----- */

//#include "fann_error.h"
#define FANN_ERRSTR_MAX 128
struct fann_error;

/* Section: FANN Error Handling

   Errors from the fann library are usually reported on stderr.
   It is however possible to redirect these error messages to a file,
   or completely ignore them by the <fann_set_error_log> function.

   It is also possible to inspect the last error message by using the
   <fann_get_errno> and <fann_get_errstr> functions.
 */

/* Enum: fann_errno_enum
        Used to define error events on <Fann2> and <struct fann_train_data>.

        See also:
                <fann_get_errno>, <fann_reset_errno>, <fann_get_errstr>

        FANN_E_NO_ERROR - No error
        FANN_E_CANT_OPEN_CONFIG_R - Unable to open configuration file for reading
        FANN_E_CANT_OPEN_CONFIG_W - Unable to open configuration file for writing
        FANN_E_WRONG_CONFIG_VERSION - Wrong version of configuration file
        FANN_E_CANT_READ_CONFIG - Error reading info from configuration file
        FANN_E_CANT_READ_NEURON - Error reading neuron info from configuration file
        FANN_E_CANT_READ_CONNECTIONS - Error reading connections from configuration file
        FANN_E_WRONG_NUM_CONNECTIONS - Number of connections not equal to the number expected
        FANN_E_CANT_OPEN_TD_W - Unable to open train data file for writing
        FANN_E_CANT_OPEN_TD_R - Unable to open train data file for reading
        FANN_E_CANT_READ_TD - Error reading training data from file
        FANN_E_CANT_ALLOCATE_MEM - Unable to allocate memory
        FANN_E_CANT_TRAIN_ACTIVATION - Unable to train with the selected activation function
        FANN_E_CANT_USE_ACTIVATION - Unable to use the selected activation function
        FANN_E_TRAIN_DATA_MISMATCH - Irreconcilable differences between two <struct fann_train_data> structures
        FANN_E_CANT_USE_TRAIN_ALG - Unable to use the selected training algorithm
        FANN_E_TRAIN_DATA_SUBSET - Trying to take subset which is not within the training set
        FANN_E_INDEX_OUT_OF_BOUND - Index is out of bound
        FANN_E_SCALE_NOT_PRESENT - Scaling parameters not present
    FANN_E_INPUT_NO_MATCH - The number of input neurons in the ann and data don't match
    FANN_E_OUTPUT_NO_MATCH - The number of output neurons in the ann and data don't match
        FANN_E_WRONG_PARAMETERS_FOR_CREATE - The parameters for create_standard are wrong, either too few parameters
           provided or a negative/very high value provided
 */
enum fann_errno_enum {
	FANN_E_NO_ERROR = 0,
	FANN_E_CANT_OPEN_CONFIG_R,
	FANN_E_CANT_OPEN_CONFIG_W,
	FANN_E_WRONG_CONFIG_VERSION,
	FANN_E_CANT_READ_CONFIG,
	FANN_E_CANT_READ_NEURON,
	FANN_E_CANT_READ_CONNECTIONS,
	FANN_E_WRONG_NUM_CONNECTIONS,
	FANN_E_CANT_OPEN_TD_W,
	FANN_E_CANT_OPEN_TD_R,
	FANN_E_CANT_READ_TD,
	FANN_E_CANT_ALLOCATE_MEM,
	FANN_E_CANT_TRAIN_ACTIVATION,
	FANN_E_CANT_USE_ACTIVATION,
	FANN_E_TRAIN_DATA_MISMATCH,
	FANN_E_CANT_USE_TRAIN_ALG,
	FANN_E_TRAIN_DATA_SUBSET,
	FANN_E_INDEX_OUT_OF_BOUND,
	FANN_E_SCALE_NOT_PRESENT,
	FANN_E_INPUT_NO_MATCH,
	FANN_E_OUTPUT_NO_MATCH,
	FANN_E_WRONG_PARAMETERS_FOR_CREATE
};
// 
// Group: Error Handling 
// 
// Change where errors are logged to. Both <Fann2> and <struct fann_data> can be
// casted to <struct fann_error>, so this function can be used to set either of these.
// 
// If log_file is NULL, no errors will be printed.
// 
// If errdat is NULL, the default log will be set. The default log is the log used when creating
// <Fann2> and <struct fann_data>. This default log will also be the default for all new structs
// that are created.
// 
// The default behavior is to log them to stderr.
// 
// See also: <struct fann_error>
// 
// This function appears in FANN >= 1.1.0.
// 
FANN_EXTERNAL void FANN_API fann_set_error_log(struct fann_error * errdat, FILE * log_file);
// 
// Returns the last error number.
// See also: <fann_errno_enum>, <fann_reset_errno>
// This function appears in FANN >= 1.1.0.
// 
FANN_EXTERNAL enum fann_errno_enum FANN_API fann_get_errno(const struct fann_error * errdat);
// 
// Resets the last error number.
// This function appears in FANN >= 1.1.0.
// 
FANN_EXTERNAL void FANN_API fann_reset_errno(struct fann_error * errdat);
// 
// Resets the last error string.
// This function appears in FANN >= 1.1.0.
// 
FANN_EXTERNAL void FANN_API fann_reset_errstr(struct fann_error * errdat);
// 
// Returns the last errstr.
// This function calls <fann_reset_errno> and <fann_reset_errstr>
// This function appears in FANN >= 1.1.0.
// 
FANN_EXTERNAL char * FANN_API fann_get_errstr(struct fann_error * errdat);
// 
// Prints the last error to stderr.
// This function appears in FANN >= 1.1.0.
// 
FANN_EXTERNAL void FANN_API fann_print_error(struct fann_error * errdat);
FANN_EXTERNAL extern FILE * fann_default_error_log;
//
//#include "fann_activation.h"
/* internal include file, not to be included directly
*/

/* Implementation of the activation functions
*/

/* stepwise linear functions used for some of the activation functions */

/* defines used for the stepwise linear functions */

/* Uses the single precision version of math.h functions if
__doublefann_h__ is not defined
*/
#ifndef __doublefann_h__
	#define FANN_EXP(x) expf(x)
	#define FANN_SIN(x) sinf(x)
	#define FANN_COS(x) cosf(x)
#else
	#define FANN_EXP(x) exp(x)
	#define FANN_SIN(x) sin(x)
	#define FANN_COS(x) cos(x)
#endif

#define fann_linear_func(v1, r1, v2, r2, sum) (((((r2)-(r1)) * ((sum)-(v1)))/((v2)-(v1))) + (r1))
#define fann_stepwise(v1, v2, v3, v4, v5, v6, r1, r2, r3, r4, r5, r6, min, max, sum) (sum < v5 ? (sum < v3 ? (sum < v2 ? (sum < v1 ? min : fann_linear_func(v1, r1, v2, r2, sum)) : fann_linear_func(v2, r2, v3, r3, sum)) : (sum < v4 ? fann_linear_func(v3, r3, v4, r4, sum) : fann_linear_func(v4, r4, v5, r5, sum))) : (sum < v6 ? fann_linear_func(v5, r5, v6, r6, sum) : max))

/* FANN_LINEAR */
/* #define fann_linear(steepness, sum) fann_mult(steepness, sum) */
#define fann_linear_derive(steepness, value) (steepness)

/* FANN_SIGMOID */
/* #define fann_sigmoid(steepness, sum) (1.0f/(1.0f + exp(-2.0f * steepness * sum))) */
#define fann_sigmoid_real(sum) (1.0f/(1.0f + FANN_EXP(-2.0f * (sum))))
#define fann_sigmoid_derive(steepness, value) (2.0f * (steepness) * (value) * (1.0f - (value)))

/* FANN_SIGMOID_SYMMETRIC */
/* #define fann_sigmoid_symmetric(steepness, sum) (2.0f/(1.0f + exp(-2.0f * steepness * sum)) - 1.0f) */
#define fann_sigmoid_symmetric_real(sum) (2.0f/(1.0f + FANN_EXP(-2.0f * (sum))) - 1.0f)
#define fann_sigmoid_symmetric_derive(steepness, value) (steepness) * (1.0f - ((value)*(value)))

/* FANN_GAUSSIAN */
/* #define fann_gaussian(steepness, sum) (exp(-sum * steepness * sum * steepness)) */
#define fann_gaussian_real(sum) (FANN_EXP(-(sum) * (sum)))
#define fann_gaussian_derive(steepness, value, sum) (-2.0f * (sum) * (value) * (steepness) * (steepness))

/* FANN_GAUSSIAN_SYMMETRIC */
/* #define fann_gaussian_symmetric(steepness, sum) ((exp(-sum * steepness * sum * steepness)*2.0)-1.0) */
#define fann_gaussian_symmetric_real(sum) ((FANN_EXP(-(sum) * (sum))*2.0f)-1.0f)
#define fann_gaussian_symmetric_derive(steepness, value, sum) (-2.0f * (sum) * ((value)+1.0f) * (steepness) * (steepness))

/* FANN_ELLIOT */
/* #define fann_elliot(steepness, sum) (((sum * steepness) / 2.0f) / (1.0f + fabsf(sum * steepness)) + 0.5f) */
#define fann_elliot_real(sum) (((sum) / 2.0f) / (1.0f + fabsf(sum)) + 0.5f)
#define fann_elliot_derive(steepness, value, sum) ((steepness) * 1.0f / (2.0f * (1.0f + fabsf(sum)) * (1.0f + fabsf(sum))))

/* FANN_ELLIOT_SYMMETRIC */
/* #define fann_elliot_symmetric(steepness, sum) ((sum * steepness) / (1.0f + fabsf(sum * steepness)))*/
#define fann_elliot_symmetric_real(sum) ((sum) / (1.0f + fabsf(sum)))
#define fann_elliot_symmetric_derive(steepness, value, sum) ((steepness) * 1.0f / ((1.0f + fabsf(sum)) * (1.0f + fabsf(sum))))
/* FANN_SIN_SYMMETRIC */
#define fann_sin_symmetric_real(sum) (FANN_SIN(sum))
#define fann_sin_symmetric_derive(steepness, sum) (steepness*cos(steepness*sum))
/* FANN_COS_SYMMETRIC */
#define fann_cos_symmetric_real(sum) (FANN_COS(sum))
#define fann_cos_symmetric_derive(steepness, sum) ((steepness)*-sin((steepness)*(sum)))
/* FANN_SIN */
#define fann_sin_real(sum) (FANN_SIN(sum)/2.0f+0.5f)
#define fann_sin_derive(steepness, sum) ((steepness)*cos((steepness)*(sum))/2.0f)
/* FANN_COS */
#define fann_cos_real(sum) (FANN_COS(sum)/2.0f+0.5f)
#define fann_cos_derive(steepness, sum) ((steepness)*-sin((steepness)*(sum))/2.0f)

#define fann_activation_switch(activation_function, value, result) \
switch(activation_function) { \
	case Fann2::FANN_LINEAR: result = static_cast<float>(value); break; \
	case Fann2::FANN_LINEAR_PIECE: result = static_cast<float>((value < 0) ? 0 : (value > 1) ? 1 : value); break; \
	case Fann2::FANN_LINEAR_PIECE_SYMMETRIC: result = static_cast<float>((value < -1) ? -1 : (value > 1) ? 1 : value); break; \
	case Fann2::FANN_SIGMOID: result = static_cast<float>(fann_sigmoid_real(value)); break; \
	case Fann2::FANN_SIGMOID_SYMMETRIC: result = static_cast<float>(fann_sigmoid_symmetric_real(value)); break; \
	case Fann2::FANN_SIGMOID_SYMMETRIC_STEPWISE: \
		result = (float)fann_stepwise(((float)-2.64665293693542480469e+00), ((float)-1.47221934795379638672e+00), ((float)-5.49306154251098632812e-01), ((float)5.49306154251098632812e-01), ((float)1.47221934795379638672e+00), ((float)2.64665293693542480469e+00), ((float)-9.90000009536743164062e-01), ((float)-8.99999976158142089844e-01), ((float)-5.00000000000000000000e-01), ((float)5.00000000000000000000e-01), ((float)8.99999976158142089844e-01), ((float)9.90000009536743164062e-01), -1, 1, value); \
        break; \
	case Fann2::FANN_SIGMOID_STEPWISE: \
		result = (float)fann_stepwise(((float)-2.64665246009826660156e+00), ((float)-1.47221946716308593750e+00), ((float)-5.49306154251098632812e-01), ((float)5.49306154251098632812e-01), ((float)1.47221934795379638672e+00), ((float)2.64665293693542480469e+00), ((float) 4.99999988824129104614e-03), ((float) 5.00000007450580596924e-02), ((float) 2.50000000000000000000e-01), ((float)7.50000000000000000000e-01), ((float)9.49999988079071044922e-01), ((float)9.95000004768371582031e-01), 0, 1, value); \
        break; \
	case Fann2::FANN_THRESHOLD: result = ((value < 0) ? 0.0f : 1.0f); break; \
	case Fann2::FANN_THRESHOLD_SYMMETRIC: result = ((value < 0) ? -1.0f : 1.0f); break; \
	case Fann2::FANN_GAUSSIAN: result = (float)fann_gaussian_real(value); break; \
	case Fann2::FANN_GAUSSIAN_SYMMETRIC: result = (float)fann_gaussian_symmetric_real(value); break; \
	case Fann2::FANN_ELLIOT: result = (float)fann_elliot_real(value); break; \
	case Fann2::FANN_ELLIOT_SYMMETRIC: result = (float)fann_elliot_symmetric_real(value); break; \
	case Fann2::FANN_SIN_SYMMETRIC: result = (float)fann_sin_symmetric_real(value); break; \
	case Fann2::FANN_COS_SYMMETRIC: result = (float)fann_cos_symmetric_real(value); break; \
	case Fann2::FANN_SIN: result = (float)fann_sin_real(value); break; \
	case Fann2::FANN_COS: result = (float)fann_cos_real(value); break; \
	case Fann2::FANN_GAUSSIAN_STEPWISE: result = 0; break; \
}
//
//#include "fann_data.h"
/* Section: FANN Datatypes

   The two main datatypes used in the fann library are <Fann2>,
   which represents an artificial neural network, and <struct fann_train_data>,
   which represents training data.
 */

/* Type: float
   float is the type used for the weights, inputs and outputs of the neural network.

        float is defined as a:
        float - if you include fann.h or floatfann.h
        double - if you include doublefann.h
        int - if you include fixedfann.h (please be aware that fixed point usage is
                        only to be used during execution, and not during training).
 */
// 
// Descr: The Training algorithms used when training on <struct fann_train_data> with functions like
// <fann_train_on_data> or <fann_train_on_file>. The incremental training alters the weights
// after each time it is presented an input pattern, while batch only alters the weights once after
// it has been presented to all the patterns.
// See also: <fann_set_training_algorithm>, <fann_get_training_algorithm>
// 
enum fann_train_enum {
	FANN_TRAIN_INCREMENTAL = 0, // Standard backpropagation algorithm, where the weights are
		// updated after each training pattern. This means that the weights are updated many
		// times during a single epoch. For this reason some problems will train very fast with
		// this algorithm, while other more advanced problems will not train very well.
	FANN_TRAIN_BATCH, // Standard backpropagation algorithm, where the weights are updated after
		// calculating the mean square error for the whole training set. This means that the weights
		// are only updated once during an epoch. For this reason some problems will train slower with
		// this algorithm. But since the mean square error is calculated more correctly than in
		// incremental training, some problems will reach better solutions with this algorithm.
	FANN_TRAIN_RPROP, // A more advanced batch training algorithm which achieves good results
		// for many problems. The RPROP training algorithm is adaptive, and does therefore not
		// use the learning_rate. Some other parameters can however be set to change the way the
		// RPROP algorithm works, but it is only recommended for users with insight in how the RPROP
		// training algorithm works. The RPROP training algorithm is described by
		// [Riedmiller and Braun, 1993], but the actual learning algorithm used here is the
		// iRPROP- training algorithm which is described by [Igel and Husken, 2000] which
		// is a variant of the standard RPROP training algorithm.
	FANN_TRAIN_QUICKPROP, // A more advanced batch training algorithm which achieves good results
		// for many problems. The quickprop training algorithm uses the learning_rate parameter
		// along with other more advanced parameters, but it is only recommended to change these
		// advanced parameters, for users with insight in how the quickprop training algorithm works.
		// The quickprop training algorithm is described by [Fahlman, 1988].
	FANN_TRAIN_SARPROP // THE SARPROP ALGORITHM: A SIMULATED ANNEALING ENHANCEMENT TO RESILIENT BACK PROPAGATION
		// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.47.8197&rep=rep1&type=pdf
};
//
// Constant array consisting of the names for the training algorithms, so that the name of an training function can be received by:
// (code)
// char *name = FANN_TRAIN_NAMES[train_function];
// (end)
// See Also: <fann_train_enum>
// 
static char const * const FANN_TRAIN_NAMES[] = { "FANN_TRAIN_INCREMENTAL", "FANN_TRAIN_BATCH", "FANN_TRAIN_RPROP", "FANN_TRAIN_QUICKPROP", "FANN_TRAIN_SARPROP" };

/* Constant: FANN_ACTIVATIONFUNC_NAMES

   Constant array consisting of the names for the activation function, so that the name of an
   activation function can be received by:
   (code)
   char *name = FANN_ACTIVATIONFUNC_NAMES[activation_function];
   (end)

   See Also: <fann_activationfunc_enum>
 */
static char const * const FANN_ACTIVATIONFUNC_NAMES[] = {
	"FANN_LINEAR",
	"FANN_THRESHOLD",
	"FANN_THRESHOLD_SYMMETRIC",
	"FANN_SIGMOID",
	"FANN_SIGMOID_STEPWISE",
	"FANN_SIGMOID_SYMMETRIC",
	"FANN_SIGMOID_SYMMETRIC_STEPWISE",
	"FANN_GAUSSIAN",
	"FANN_GAUSSIAN_SYMMETRIC",
	"FANN_GAUSSIAN_STEPWISE",
	"FANN_ELLIOT",
	"FANN_ELLIOT_SYMMETRIC",
	"FANN_LINEAR_PIECE",
	"FANN_LINEAR_PIECE_SYMMETRIC",
	"FANN_SIN_SYMMETRIC",
	"FANN_COS_SYMMETRIC",
	"FANN_SIN",
	"FANN_COS"
};
//
// Descr: Error function used during training.
// See also: <fann_set_train_error_function>, <fann_get_train_error_function>
//
enum fann_errorfunc_enum {
	FANN_ERRORFUNC_LINEAR = 0, // Standard linear error function.
	FANN_ERRORFUNC_TANH // Tanh error function, usually better but can require a lower learning rate. This error function aggressively targets outputs that
		// differ much from the desired, while not targeting outputs that only differ a little that much.
		// This activation function is not recommended for cascade training and incremental training.
};

/* Constant: FANN_ERRORFUNC_NAMES

   Constant array consisting of the names for the training error functions, so that the name of an
   error function can be received by:
   (code)
   char *name = FANN_ERRORFUNC_NAMES[error_function];
   (end)

   See Also: <fann_errorfunc_enum>
 */
static char const * const FANN_ERRORFUNC_NAMES[] = { "FANN_ERRORFUNC_LINEAR", "FANN_ERRORFUNC_TANH" };
// 
// Descr: Stop criteria used during training.
// 
enum fann_stopfunc_enum {
	FANN_STOPFUNC_MSE = 0, // Stop criterion is Mean Square Error (MSE) value.
	FANN_STOPFUNC_BIT      // Stop criterion is number of bits that fail. The number of bits; means the number of output neurons which differ more than the bit fail limit.
		// (see <fann_get_bit_fail_limit>, <fann_set_bit_fail_limit>).
		// The bits are counted in all of the training data, so this number can be higher than the number of training data.
};

/* Constant: FANN_STOPFUNC_NAMES

   Constant array consisting of the names for the training stop functions, so that the name of a
   stop function can be received by:
   (code)
   char *name = FANN_STOPFUNC_NAMES[stop_function];
   (end)

   See Also: <fann_stopfunc_enum>
 */
static char const * const FANN_STOPFUNC_NAMES[] = { "FANN_STOPFUNC_MSE", "FANN_STOPFUNC_BIT" };

/* Enum: fann_network_type_enum

    Definition of network types used by <fann_get_network_type>

    FANN_NETTYPE_LAYER - Each layer only has connections to the next layer
    FANN_NETTYPE_SHORTCUT - Each layer has connections to all following layers

   See Also: <fann_get_network_type>

   This enumeration appears in FANN >= 2.1.0
 */
enum fann_nettype_enum {
	FANN_NETTYPE_LAYER = 0, /* Each layer only has connections to the next layer */
	FANN_NETTYPE_SHORTCUT /* Each layer has connections to all following layers */
};
// 
// Constant: FANN_NETWORK_TYPE_NAMES
// 
// Constant array consisting of the names for the network types, so that the name of an
// network type can be received by:
// (code)
// char *network_type_name = FANN_NETWORK_TYPE_NAMES[fann_get_network_type(ann)];
// (end)
// 
// See Also: <fann_get_network_type>
// This constant appears in FANN >= 2.1.0
// 
static char const * const FANN_NETTYPE_NAMES[] = { "FANN_NETTYPE_LAYER", "FANN_NETTYPE_SHORTCUT" };

// forward declarations for use with the callback 
struct Fann2;
struct fann_train_data;
// 
// Type: fann_callback_type
// This callback function can be called during training when using <fann_train_on_data>, <fann_train_on_file> or <fann_cascadetrain_on_data>.
// 
//   >typedef int (FANN_API * fann_callback_type) (Fann2 *ann, struct fann_train_data *train, uint max_epochs, uint epochs_between_reports, float desired_error, uint epochs);
// 
// The callback can be set by using <fann_set_callback> and is very useful for doing custom
// things during training. It is recommended to use this function when implementing custom
// training procedures, or when visualizing the training in a GUI etc. The parameters which the
// callback function takes are the parameters given to <fann_train_on_data>, plus an epochs
// parameter which tells how many epochs the training has taken so far.
// 
// The callback function should return an integer, if the callback function returns -1, the training
// will terminate.
// 
// Example of a callback function:
//   >int FANN_API test_callback(Fann2 *ann, struct fann_train_data *train, uint max_epochs, uint epochs_between_reports, float desired_error, uint epochs)
//   >{
//   >	printf("Epochs     %8d. MSE: %.5f. Desired-MSE: %.5f\n", epochs, fann_get_MSE(ann), desired_error);
//   >	return 0;
//   >}
// 
// See also: <fann_set_callback>, <fann_train_on_data>
// 
FANN_EXTERNAL typedef int (FANN_API * fann_callback_type)(Fann2 * ann, struct fann_train_data * train, uint max_epochs, uint epochs_between_reports, float desired_error, uint epochs);
//
// Data structures
// No data within these structures should be altered directly by the user.
// 

// 
// Descr: Structure used to store error-related information, both
//   <Fann2> and <struct fann_train_data> can be casted to this type.
//   See also: <fann_set_error_log>, <fann_get_errno>
// 
struct fann_error {
	enum fann_errno_enum errno_f;
	FILE * error_log;
	char * errstr;
};
// 
// Descr: The fast artificial neural network (fann) structure.
// 
// Data within this structure should never be accessed directly, but only by using the
//  * fann_get_...* and *fann_set_...* functions.
// 
// The fann structure is created using one of the *fann_create_...* functions and each of
// the functions which operates on the structure takes *Fann2 * ann* as the first parameter.
// 
// See also: <fann_create_standard>, <fann_destroy>
// 
struct Fann2 {
	// 
	// Descr: The activation functions used for the neurons during training. The activation functions
	//   can either be defined for a group of neurons by <fann_set_activation_function_hidden> and
	//   <fann_set_activation_function_output> or it can be defined for a single neuron by <fann_set_activation_function>.
	// 
	// The steepness of an activation function is defined in the same way by
	// <fann_set_activation_steepness_hidden>, <fann_set_activation_steepness_output> and <fann_set_activation_steepness>.
	// 
	// The functions are described with functions where:
	// * x is the input to the activation function,
	// * y is the output,
	// * s is the steepness and
	// * d is the derivation.
	// See also: <fann_set_activation_function_layer>, <fann_set_activation_function_hidden>,
	//   <fann_set_activation_function_output>, <fann_set_activation_steepness>, <fann_set_activation_function>
	// 
	enum ActivationFunc {
		FANN_LINEAR = 0, // Linear activation function. span: -inf < y < inf; y = x*s; d = 1*s; Can NOT be used in fixed point.
		FANN_THRESHOLD,  // Threshold activation function. x < 0 -> y = 0, x >= 0 -> y = 1; Can NOT be used during training.
		FANN_THRESHOLD_SYMMETRIC, // Threshold activation function. x < 0 -> y = -1, x >= 0 -> y = 1; Can NOT be used during training.
		FANN_SIGMOID, // Sigmoid activation function. One of the most used activation functions. span: 0 < y < 1; y = 1/(1 + exp(-2*s*x)); d = 2*s*y*(1 - y)
		FANN_SIGMOID_STEPWISE, // Stepwise linear approximation to sigmoid. Faster than sigmoid but a bit less precise.
		FANN_SIGMOID_SYMMETRIC, // Symmetric sigmoid activation function, aka. tanh. One of the most used activation functions.
			// * span: -1 < y < 1; y = tanh(s*x) = 2/(1 + exp(-2*s*x)) - 1; d = s*(1-(y*y))
		FANN_SIGMOID_SYMMETRIC_STEPWISE, // Stepwise linear approximation to symmetric sigmoid. Faster than symmetric sigmoid but a bit less precise.
		FANN_GAUSSIAN, // Gaussian activation function. 0 when x = -inf, 1 when x = 0 and 0 when x = inf. span: 0 < y < 1; y = exp(-x*s*x*s); d = -2*x*s*y*s
		FANN_GAUSSIAN_SYMMETRIC, // Symmetric gaussian activation function. -1 when x = -inf, 1 when x = 0 and 0 when x = inf. span: -1 < y < 1; y = exp(-x*s*x*s)*2-1; d = -2*x*s*(y+1)*s
		FANN_GAUSSIAN_STEPWISE, // Stepwise linear approximation to gaussian. Faster than gaussian but a bit less precise. NOT implemented yet.
		FANN_ELLIOT, // Fast (sigmoid like) activation function defined by David Elliott. span: 0 < y < 1; y = ((x*s) / 2) / (1 + |x*s|) + 0.5; d = s*1/(2*(1+|x*s|)*(1+|x*s|))
		FANN_ELLIOT_SYMMETRIC, // Fast (symmetric sigmoid like) activation function defined by David Elliott. span: -1 < y < 1; y = (x*s) / (1 + |x*s|); d = s*1/((1+|x*s|)*(1+|x*s|))
		FANN_LINEAR_PIECE,           // Bounded linear activation function. span: 0 <= y <= 1; y = x*s; d = 1*s
		FANN_LINEAR_PIECE_SYMMETRIC, // Bounded linear activation function. span: -1 <= y <= 1; y = x*s; d = 1*s
		FANN_SIN_SYMMETRIC, // Periodical sinus activation function. span: -1 <= y <= 1; y = sin(x*s); d = s*cos(x*s)
		FANN_COS_SYMMETRIC, // Periodical cosinus activation function. span: -1 <= y <= 1; y = cos(x*s); d = s*-sin(x*s)
		FANN_SIN,           // Periodical sinus activation function. span: 0 <= y <= 1; y = sin(x*s)/2+0.5; d = s*cos(x*s)/2
		FANN_COS            // Periodical cosinus activation function. span: 0 <= y <= 1; y = cos(x*s)/2+0.5; d = s*-sin(x*s)/2
	};
	struct Neuron {
		bool   FASTCALL IsEq(const Neuron & rS) const // @construction
		{
#define FLD(f) if(f != rS.f) return false;
			FLD(first_con);
			FLD(last_con);
			FLD(sum);
			FLD(value);
			FLD(activation_steepness);
			FLD(activation_function);
#undef FLD
			return true;
		}
		// Index to the first and last connection (actually the last is a past end index)
		uint   first_con;
		uint   last_con;
		float sum; // The sum of the inputs multiplied with the weights 
		float value; // The value of the activation function applied to the sum 
		float activation_steepness; // The steepness of the activation function 
		Fann2::ActivationFunc activation_function; // Used to choose which activation function to use 
	#ifdef __GNUC__
	} __attribute__ ((packed));
	#else
	};
	#endif
	// 
	// Descr: A single layer in the neural network.
	// 
	struct Layer {
		bool   FASTCALL IsEq(const Layer & rS) const // @construction
		{
			const uint nn = (uint)(last_neuron - first_neuron);
			const uint nn_s = (uint)(rS.last_neuron - rS.first_neuron);
			if(nn != nn_s)
				return false;
			else {
				for(uint i = 0; i < nn; i++) {
					if(!first_neuron[i].IsEq(rS.first_neuron[i]))
						return false;
				}
			}
			return true;
		}
		// A pointer to the first neuron in the layer
		// When allocated, all the neurons in all the layers are actually
		// in one long array, this is because we want to easily clear all the neurons at once.
		Neuron * first_neuron;
		Neuron * last_neuron; // A pointer to the neuron past the last neuron in the layer the number of neurons is last_neuron - first_neuron 
	};
	
	bool   FASTCALL IsEq(const Fann2 & rS) const; // @construction

	enum fann_errno_enum errno_f; // The type of error that last occured. 
	FILE * error_log;         // Where to log error messages. 
	char * errstr;            // A string representation of the last error. 
	float  learning_rate;     // the learning rate of the network 
	float  learning_momentum; // The learning momentum used for backpropagation algorithm. 
	float  connection_rate;   // the connection rate of the network between 0 and 1, 1 meaning fully connected
	// is 1 if shortcut connections are used in the ann otherwise 0
	// Shortcut connections are connections that skip layers.
	// A fully connected ann with shortcut connections are a ann where
	// neurons have connections to all neurons in all later layers.
	enum fann_nettype_enum network_type;
	Layer * first_layer; // pointer to the first layer (input layer) in an array af all the layers, including the input and outputlayers
	Layer * last_layer;  // pointer to the layer past the last layer in an array af all the layers, including the input and outputlayers
	uint   total_neurons; // Total number of neurons. very useful, because the actual neurons are allocated in one long array
	uint   num_input;     // Number of input neurons (not calculating bias) 
	uint   num_output;    // Number of output neurons (not calculating bias) 
	float * weights;  // The weight array 
	Neuron ** connections; // The connection array 
	// 
	// Used to contain the errors used during training
	// Is allocated during first training session,
	// which means that if we do not train, it is never allocated.
	// 
	float * train_errors;
	// Training algorithm used when calling fann_train_on_..
	enum fann_train_enum training_algorithm;
#ifdef FIXEDFANN
	uint decimal_point; // the decimal_point, used for shifting the fix point in fixed point integer operatons. 
	uint multiplier; // the multiplier, used for multiplying the fix point in fixed point integer operatons.
		// Only used in special cases, since the decimal_point is much faster.
	// 
	// When in choosen (or in fixed point), the sigmoid function is
	// calculated as a stepwise linear function. In the
	// activation_results array, the result is saved, and in the
	// two values arrays, the values that gives the results are saved.
	// 
	float sigmoid_results[6];
	float sigmoid_values[6];
	float sigmoid_symmetric_results[6];
	float sigmoid_symmetric_values[6];
#endif
	uint   total_connections; // Total number of connections. very useful, because the actual connections are allocated in one long array
	float * output; // used to store outputs in 
	uint   num_MSE; // the number of data used to calculate the mean square error.
	float  MSE_value; // the total error value. the real mean square error is MSE_value/num_MSE
	uint   num_bit_fail; // The number of outputs which would fail (only valid for classification problems)
	// The maximum difference between the actual output and the expected output
	// which is accepted when counting the bit fails.
	// This difference is multiplied by two when dealing with symmetric activation functions,
	// so that symmetric and not symmetric activation functions can use the same limit.
	float bit_fail_limit;
	enum fann_errorfunc_enum train_error_function; // The error function used during training. (default FANN_ERRORFUNC_TANH)
	enum fann_stopfunc_enum train_stop_function; // The stop function used during training. (default FANN_STOPFUNC_MSE)
	fann_callback_type callback; // The callback function used during training. (default NULL)
	void * user_data; // A pointer to user defined data. (default NULL)
	// 
	// Variables for use with Cascade Correlation 
	// 
	float  cascade_output_change_fraction;  // The error must change by at least this fraction of its old value to count as a significant change.
	uint   cascade_output_stagnation_epochs; // No change in this number of epochs will cause stagnation.
	float  cascade_candidate_change_fraction; // The error must change by at least this fraction of its old value to count as a significant change.
	uint   cascade_candidate_stagnation_epochs; // No change in this number of epochs will cause stagnation.
	uint   cascade_best_candidate; // The current best candidate, which will be installed.
	float cascade_candidate_limit; // The upper limit for a candidate score
	float cascade_weight_multiplier; // Scale of copied candidate output weights
	uint   cascade_max_out_epochs; // Maximum epochs to train the output neurons during cascade training
	uint   cascade_max_cand_epochs; // Maximum epochs to train the candidate neurons during cascade training
	uint   cascade_min_out_epochs; // Minimum epochs to train the output neurons during cascade training
	uint   cascade_min_cand_epochs; // Minimum epochs to train the candidate neurons during cascade training
	ActivationFunc * cascade_activation_functions; // An array consisting of the activation functions used when doing cascade training.
	uint   cascade_activation_functions_count; // The number of elements in the cascade_activation_functions array.
	float * cascade_activation_steepnesses; // An array consisting of the steepnesses used during cascade training.
	uint   cascade_activation_steepnesses_count; // The number of elements in the cascade_activation_steepnesses array.
	// The number of candidates of each type that will be present.
	// The actual number of candidates is then (cascade_activation_functions_count * cascade_activation_steepnesses_count * cascade_num_candidate_groups)
	uint   cascade_num_candidate_groups;
	float * cascade_candidate_scores; // An array consisting of the score of the individual candidates, which is used to decide which candidate is the best
	// 
	// The number of allocated neurons during cascade correlation algorithms.
	// This number might be higher than the actual number of neurons to avoid allocating new space too often.
	// 
	uint   total_neurons_allocated;
	// 
	// The number of allocated connections during cascade correlation algorithms.
	// This number might be higher than the actual number of neurons to avoid allocating new space too often.
	// 
	uint   total_connections_allocated;
	//
	// Variables for use with Quickprop training 
	//
	float  quickprop_decay; // Decay is used to make the weights not go so high 
	float  quickprop_mu;    // Mu is a factor used to increase and decrease the stepsize 
	//
	// Variables for use with with RPROP training 
	//
	float  rprop_increase_factor; // Tells how much the stepsize should increase during learning
	float  rprop_decrease_factor; // Tells how much the stepsize should decrease during learning
	float  rprop_delta_min;       // The minimum stepsize
	float  rprop_delta_max;       // The maximum stepsize
	float  rprop_delta_zero;      // The initial stepsize
	float  sarprop_weight_decay_shift; /* Defines how much the weights are constrained to smaller values at the beginning */
	float  sarprop_step_error_threshold_factor; /* Decides if the stepsize is too big with regard to the error */
	float  sarprop_step_error_shift; /* Defines how much the stepsize is influenced by the error */
	float  sarprop_temperature; /* Defines how much the epoch influences weight decay and noise */
	uint   sarprop_epoch; /* Current training epoch */
	// Used to contain the slope errors used during batch training
	// Is allocated during first training session,
	// which means that if we do not train, it is never allocated.
	float * train_slopes;
	float * prev_steps; // The previous step taken by the quickprop/rprop procedures. Not allocated if not used.
	float * prev_train_slopes; // The slope values used by the quickprop/rprop procedures. Not allocated if not used.
	// The last delta applied to a connection weight.
	// This is used for the momentum term in the backpropagation algorithm.
	// Not allocated if not used.
	float * prev_weights_deltas;
#ifndef FIXEDFANN
	float * scale_mean_in;       // Arithmetic mean used to remove steady component in input data.  
	float * scale_deviation_in;  // Standart deviation used to normalize input data (mostly to [-1;1]). 
	float * scale_new_min_in;    // User-defined new minimum for input data. Resulting data values may be less than user-defined minimum.
	float * scale_factor_in;     // Used to scale data to user-defined new maximum for input data. Resulting data values may be greater than user-defined maximum.
	float * scale_mean_out;      // Arithmetic mean used to remove steady component in output data.  
	float * scale_deviation_out; // Standart deviation used to normalize output data (mostly to [-1;1]). 
	float * scale_new_min_out;   // User-defined new minimum for output data. Resulting data values may be less than user-defined minimum.
	float * scale_factor_out;    // Used to scale data to user-defined new maximum for output data. Resulting data values may be greater than user-defined maximum.
#endif
};
// 
// Describes a connection between two neurons and its weight
// See Also: <fann_get_connection_array>, <fann_set_weight_array>
// This structure appears in FANN >= 2.1.0
// 
struct fann_connection {
	uint from_neuron; // Unique number used to identify source neuron 
	uint to_neuron; // Unique number used to identify destination neuron 
	float weight; // The numerical value of the weight 
};
//
#define FANN_FIX_VERSION "FANN_FIX_2.0"
#define FANN_FLO_VERSION "FANN_FLO_2.1"
#ifdef FIXEDFANN
	#define FANN_CONF_VERSION FANN_FIX_VERSION
#else
	#define FANN_CONF_VERSION FANN_FLO_VERSION
#endif
#define FANN_GET(type, name) FANN_EXTERNAL type FANN_API fann_get_ ## name(const Fann2 *ann) { return ann->name; }
#define FANN_SET(type, name) FANN_EXTERNAL void FANN_API fann_set_ ## name(Fann2 *ann, type value) { ann->name = value; }
#define FANN_GET_SET(type, name) FANN_GET(type, name) FANN_SET(type, name)

//Fann2 *fann_allocate_structure(uint num_layers);
//void fann_allocate_neurons(Fann2 *ann);
//void fann_allocate_connections(Fann2 *ann);
//int fann_save_internal_fd(Fann2 *ann, FILE * conf, const char *configuration_file, uint save_as_fixed);
//int fann_save_train_internal(struct fann_train_data *data, const char *filename, uint save_as_fixed, uint decimal_point);
//int fann_save_train_internal_fd(struct fann_train_data *data, FILE * file, const char *filename, uint save_as_fixed, uint decimal_point);
void fann_update_stepwise(Fann2 *ann);
//void fann_seed_rand();
void fann_error_2(struct fann_error *errdat, const enum fann_errno_enum errno_f, ...);
void fann_init_error_data(struct fann_error *errdat);
Fann2 * fann_create_from_fd(FILE * conf, const char *configuration_file);
struct fann_train_data * fann_read_train_from_fd(FILE * file, const char *filename);
void fann_compute_MSE(Fann2 *ann, float * desired_output);
void fann_update_output_weights(Fann2 *ann);
void fann_backpropagate_MSE(Fann2 *ann);
//void fann_update_weights(Fann2 *ann);
void fann_update_slopes_batch(Fann2 *ann, Fann2::Layer *layer_begin, Fann2::Layer *layer_end);
void fann_update_weights_quickprop(Fann2 *ann, uint num_data, uint first_weight, uint past_end);
//void fann_update_weights_batch(Fann2 *ann, uint num_data, uint first_weight, uint past_end);
void fann_update_weights_irpropm(Fann2 *ann, uint first_weight, uint past_end);
void fann_update_weights_sarprop(Fann2 *ann, uint epoch, uint first_weight, uint past_end);
void fann_clear_train_arrays(Fann2 *ann);
float fann_activation(Fann2 * ann, uint activation_function, float steepness, float value);
//float fann_activation_derived(uint activation_function, float steepness, float value, float sum);
int fann_desired_error_reached(const Fann2 *ann, float desired_error);
//
// Some functions for cascade 
//
int fann_train_outputs(Fann2 *ann, struct fann_train_data *data, float desired_error);
float fann_train_outputs_epoch(Fann2 *ann, struct fann_train_data *data);
int fann_train_candidates(Fann2 *ann, struct fann_train_data *data);
float fann_train_candidates_epoch(Fann2 *ann, struct fann_train_data *data);
void fann_install_candidate(Fann2 *ann);
int fann_check_input_output_sizes(Fann2 *ann, const struct fann_train_data *data);
int fann_initialize_candidates(Fann2 *ann);
void fann_set_shortcut_connections(Fann2 *ann);
//int fann_allocate_scale(Fann2 *ann);

FANN_EXTERNAL void FANN_API fann_scale_data_to_range(float ** data, uint num_data, uint num_elem, float old_min, float old_max, float new_min, float new_max);

// called fann_max, in order to not interferre with predefined versions of max 
//#define fann_max(x, y) (((x) > (y)) ? (x) : (y))
//#define fann_min(x, y) (((x) < (y)) ? (x) : (y))
//#define fann_safe_free(x) {if(x) { SAlloc::F(x); x = NULL; }}
#define fann_clip(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))
#define fann_exp2(x) exp(0.69314718055994530942*(x))
/*#define fann_clip(x, lo, hi) (x)*/
#define fann_rand(min_value, max_value) (((float)(min_value))+(((float)(max_value)-((float)(min_value)))*rand()/(RAND_MAX+1.0f)))
//#define fann_abs__Removed(value) (((value) > 0) ? (value) : -(value))

#ifdef FIXEDFANN
	#define fann_mult(x,y) ((x*y) >> decimal_point)
	#define fann_div(x,y) (((x) << decimal_point)/(y))
	#define fann_random_weight() (float)(fann_rand(0,multiplier/10))
	#define fann_random_bias_weight() (float)(fann_rand((0-multiplier)/10,multiplier/10))
#else
	#define fann_mult(x,y) ((x)*(y))
	#define fann_div(x,y) ((x)/(y))
	#define fann_random_weight() (fann_rand(-0.1f,0.1f))
	#define fann_random_bias_weight() (fann_rand(-0.1f,0.1f))
#endif
//
//#include "fann_train.h"
/* Section: FANN Training

        There are many different ways of training neural networks and the FANN library supports
        a number of different approaches.

        Two fundamentally different approaches are the most commonly used:

                Fixed topology training - The size and topology of the ANN is determined in advance
                        and the training alters the weights in order to minimize the difference between
                        the desired output values and the actual output values. This kind of training is
                        supported by <fann_train_on_data>.

                Evolving topology training - The training start out with an empty ANN, only consisting
                        of input and output neurons. Hidden neurons and connections are added during training,
                        in order to reach the same goal as for fixed topology training. This kind of training
                        is supported by <FANN Cascade Training>.
 */

//
// Descr: Structure used to store data, for use with training.
//   The data inside this structure should never be manipulated directly, but should use some
//   of the supplied functions in <Training Data Manipulation>.
// 
// The training data structure is very usefull for storing data during training and testing of a neural network.
// See also: <fann_read_train_from_file>, <fann_train_on_data>, <fann_destroy_train>
// 
struct fann_train_data {
	enum fann_errno_enum errno_f;
	FILE * error_log;
	char * errstr;
	uint   num_data;
	uint   num_input;
	uint   num_output;
	float ** input;
	float ** output;
};

/* Section: FANN Training */

/* Group: Training */

#ifndef FIXEDFANN
// 
// Train one iteration with a set of inputs, and a set of desired outputs.
// This training is always incremental training (see <fann_train_enum>), since only one pattern is presented.
// 
// Parameters:
//   ann - The neural network structure
//   input - an array of inputs. This array must be exactly <fann_get_num_input> long.
//   desired_output - an array of desired outputs. This array must be exactly <fann_get_num_output> long.
// See also: <fann_train_on_data>, <fann_train_epoch>
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL void FANN_API fann_train(Fann2 * ann, float * input, float * desired_output, float * pRunOutput);

#endif  /* NOT FIXEDFANN */
// 
// Test with a set of inputs, and a set of desired outputs.
// This operation updates the mean square error, but does not change the network in any way.
// 
// See also: <fann_test_data>, <fann_train>
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL float * FANN_API fann_test(Fann2 * ann, float * input, float * desired_output);

/* Function: fann_get_MSE
   Reads the mean square error from the network.

   Reads the mean square error from the network. This value is calculated during
   training or testing, and can therefore sometimes be a bit off if the weights
   have been changed since the last calculation of the value.

   See also: <fann_test_data>

        This function appears in FANN >= 1.1.0.
 */
FANN_EXTERNAL float FANN_API fann_get_MSE(const Fann2 * ann);

/* Function: fann_get_bit_fail

        The number of fail bits; means the number of output neurons which differ more
        than the bit fail limit (see <fann_get_bit_fail_limit>, <fann_set_bit_fail_limit>).
        The bits are counted in all of the training data, so this number can be higher than
        the number of training data.

        This value is reset by <fann_reset_MSE> and updated by all the same functions which also
        update the MSE value (e.g. <fann_test_data>, <fann_train_epoch>)

        See also: <fann_stopfunc_enum>, <fann_get_MSE>

        This function appears in FANN >= 2.0.0
 */
FANN_EXTERNAL uint FANN_API fann_get_bit_fail(const Fann2 * ann);

/* Function: fann_reset_MSE
   Resets the mean square error from the network.

   This function also resets the number of bits that fail.

   See also: <fann_get_MSE>, <fann_get_bit_fail_limit>

    This function appears in FANN >= 1.1.0
 */
FANN_EXTERNAL void FANN_API fann_reset_MSE(Fann2 * ann);

/* Group: Training Data Training */

#ifndef FIXEDFANN

/* Function: fann_train_on_data

   Trains on an entire dataset, for a period of time.

   This training uses the training algorithm chosen by <fann_set_training_algorithm>,
   and the parameters set for these training algorithms.

   Parameters:
                ann - The neural network
                data - The data, which should be used during training
                max_epochs - The maximum number of epochs the training should continue
                epochs_between_reports - The number of epochs between printing a status report to stdout.
                        A value of zero means no reports should be printed.
                desired_error - The desired <fann_get_MSE> or <fann_get_bit_fail>, depending on which stop function
                        is chosen by <fann_set_train_stop_function>.

        Instead of printing out reports every epochs_between_reports, a callback function can be called
        (see <fann_set_callback>).

        See also: <fann_train_on_file>, <fann_train_epoch>, <Parameters>

        This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL void FANN_API fann_train_on_data(Fann2 * ann, struct fann_train_data * data, uint max_epochs, uint epochs_between_reports, float desired_error);
// 
// Does the same as <fann_train_on_data>, but reads the training data directly from a file.
// See also: <fann_train_on_data>
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL void FANN_API fann_train_on_file(Fann2 * ann, const char * filename, uint max_epochs, uint epochs_between_reports, float desired_error);
// 
// Train one epoch with a set of training data.
// Train one epoch with the training data stored in data. One epoch is where all of
// the training data is considered exactly once.
// 
// This function returns the MSE error as it is calculated either before or during
// the actual training. This is not the actual MSE after the training epoch, but since
// calculating this will require to go through the entire training set once more, it is
// more than adequate to use this value during training.
// 
// The training algorithm used by this function is chosen by the <fann_set_training_algorithm> function.
// See also: <fann_train_on_data>, <fann_test_data>
// This function appears in FANN >= 1.2.0.
// 
FANN_EXTERNAL float FANN_API fann_train_epoch(Fann2 * ann, struct fann_train_data * data);
#endif  /* NOT FIXEDFANN */
// 
// Test a set of training data and calculates the MSE for the training data.
// This function updates the MSE and the bit fail values.
// See also: <fann_test>, <fann_get_MSE>, <fann_get_bit_fail>
// This function appears in FANN >= 1.2.0.
// 
FANN_EXTERNAL float FANN_API fann_test_data(Fann2 * ann, struct fann_train_data * data);

/* Group: Training Data Manipulation */

/* Function: fann_read_train_from_file
   Reads a file that stores training data.

   The file must be formatted like:
   >num_train_data num_input num_output
   >inputdata separated by space
   >outputdata separated by space
   >
   >.
   >.
   >.
   >
   >inputdata separated by space
   >outputdata separated by space

   See also: <fann_train_on_data>, <fann_destroy_train>, <fann_save_train>

    This function appears in FANN >= 1.0.0
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_read_train_from_file(const char * filename);

/* Function: fann_create_train
   Creates an empty training data struct.

   See also: <fann_read_train_from_file>, <fann_train_on_data>, <fann_destroy_train>, <fann_save_train>, <fann_create_train_array>

    This function appears in FANN >= 2.2.0
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train(uint num_data, uint num_input, uint num_output);

/* Function: fann_create_train_pointer_array
   Creates an training data struct and fills it with data from provided arrays of pointer.

   A copy of the data is made so there are no restrictions on the
   allocation of the input/output data and the caller is responsible
   for the deallocation of the data pointed to by input and output.

   See also: <fann_read_train_from_file>, <fann_train_on_data>, <fann_destroy_train>, <fann_save_train>, <fann_create_train>, <fann_create_train_array>

    This function appears in FANN >= 2.3.0
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train_pointer_array(uint num_data, uint num_input, float ** input, uint num_output, float ** output);

/* Function: fann_create_train_array
   Creates an training data struct and fills it with data from provided arrays, where the arrays must have the
      dimensions:
   input[num_data*num_input]
   output[num_data*num_output]

   A copy of the data is made so there are no restrictions on the
   allocation of the input/output data and the caller is responsible
   for the deallocation of the data pointed to by input and output.

   See also: <fann_read_train_from_file>, <fann_train_on_data>, <fann_destroy_train>, <fann_save_train>, <fann_create_train>, <fann_create_train_pointer_array>

    This function appears in FANN >= 2.3.0
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train_array(uint num_data, uint num_input, float * input, uint num_output, float * output);

/* Function: fann_create_train_from_callback
   Creates the training data struct from a user supplied function.
   As the training data are numerable (data 1, data 2...), the user must write
   a function that receives the number of the training data set (input,output)
   and returns the set.  fann_create_train_from_callback will call the user
   supplied function 'num_data' times, one input-output pair each time. Each
   time the user supplied function is called, the time of the call will be passed
   as the 'num' parameter and the user supplied function must write the input
   and output to the corresponding parameters.


   Parameters:
     num_data      - The number of training data
     num_input     - The number of inputs per training data
     num_output    - The number of ouputs per training data
     user_function - The user supplied function

   Parameters for the user function:
     num        - The number of the training data set
     num_input  - The number of inputs per training data
     num_output - The number of ouputs per training data
     input      - The set of inputs
     output     - The set of desired outputs

   See also: <fann_read_train_from_file>, <fann_train_on_data>, <fann_destroy_train>, <fann_save_train>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train_from_callback(uint num_data,
    uint num_input, uint num_output, void(FANN_API * user_function)(uint, uint, uint, float *, float *));

/* Function: fann_destroy_train
   Destructs the training data and properly deallocates all of the associated data.
   Be sure to call this function when finished using the training data.

    This function appears in FANN >= 1.0.0
 */
FANN_EXTERNAL void FANN_API fann_destroy_train(struct fann_train_data * train_data);

/* Function: fann_get_train_input
   Gets the training input data at the given position

   See also: <fann_get_train_output>

   This function appears in FANN >= 2.3.0
 */
FANN_EXTERNAL float * FANN_API fann_get_train_input(struct fann_train_data * data, uint position);

/* Function: fann_get_train_output
   Gets the training output data at the given position

   See also: <fann_get_train_output>

   This function appears in FANN >= 2.3.0
 */
FANN_EXTERNAL float * FANN_API fann_get_train_output(struct fann_train_data * data, uint position);

/* Function: fann_shuffle_train_data

   Shuffles training data, randomizing the order.
   This is recommended for incremental training, while it has no influence during batch training.

   This function appears in FANN >= 1.1.0.
 */
FANN_EXTERNAL void FANN_API fann_shuffle_train_data(struct fann_train_data * train_data);

#ifndef FIXEDFANN

/* Function: fann_get_min_train_input

   Get the minimum value of all in the input data

   This function appears in FANN >= 2.3.0
 */
FANN_EXTERNAL float FANN_API fann_get_min_train_input(struct fann_train_data * train_data);
// 
// Get the maximum value of all in the input data
// This function appears in FANN >= 2.3.0
// 
FANN_EXTERNAL float FANN_API fann_get_max_train_input(struct fann_train_data * train_data);
// 
// Get the minimum value of all in the output data
// This function appears in FANN >= 2.3.0
// 
FANN_EXTERNAL float FANN_API fann_get_min_train_output(struct fann_train_data * train_data);
// 
// Get the maximum value of all in the output data
// This function appears in FANN >= 2.3.0
// 
FANN_EXTERNAL float FANN_API fann_get_max_train_output(struct fann_train_data * train_data);
// 
// Scale input and output data based on previously calculated parameters.
// Parameters:
//   ann      - ann for which trained parameters were calculated before
//   data     - training data that needs to be scaled
// See also: <fann_descale_train>, <fann_set_scaling_params>
// This function appears in FANN >= 2.1.0
// 
FANN_EXTERNAL void FANN_API fann_scale_train(Fann2 * ann, struct fann_train_data * data);

/* Function: fann_descale_train

   Descale input and output data based on previously calculated parameters.

   Parameters:
     ann      - ann for which trained parameters were calculated before
     data     - training data that needs to be descaled

   See also: <fann_scale_train>, <fann_set_scaling_params>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_descale_train(Fann2 * ann, struct fann_train_data * data);

/* Function: fann_set_input_scaling_params

   Calculate input scaling parameters for future use based on training data.

   Parameters:
         ann           - ann for which parameters need to be calculated
         data          - training data that will be used to calculate scaling parameters
         new_input_min - desired lower bound in input data after scaling (not strictly followed)
         new_input_max - desired upper bound in input data after scaling (not strictly followed)

   See also: <fann_set_output_scaling_params>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL int FANN_API fann_set_input_scaling_params(Fann2 * ann, const struct fann_train_data * data, float new_input_min, float new_input_max);

/* Function: fann_set_output_scaling_params

   Calculate output scaling parameters for future use based on training data.

   Parameters:
         ann            - ann for which parameters need to be calculated
         data           - training data that will be used to calculate scaling parameters
         new_output_min - desired lower bound in output data after scaling (not strictly followed)
         new_output_max - desired upper bound in output data after scaling (not strictly followed)

   See also: <fann_set_input_scaling_params>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL int FANN_API fann_set_output_scaling_params(Fann2 * ann, const struct fann_train_data * data, float new_output_min, float new_output_max);

/* Function: fann_set_scaling_params

   Calculate input and output scaling parameters for future use based on training data.

   Parameters:
         ann            - ann for which parameters need to be calculated
         data           - training data that will be used to calculate scaling parameters
         new_input_min  - desired lower bound in input data after scaling (not strictly followed)
         new_input_max  - desired upper bound in input data after scaling (not strictly followed)
         new_output_min - desired lower bound in output data after scaling (not strictly followed)
         new_output_max - desired upper bound in output data after scaling (not strictly followed)

   See also: <fann_set_input_scaling_params>, <fann_set_output_scaling_params>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL int FANN_API fann_set_scaling_params(Fann2 * ann, const struct fann_train_data * data, float new_input_min, float new_input_max, float new_output_min, float new_output_max);

/* Function: fann_clear_scaling_params

   Clears scaling parameters.

   Parameters:
     ann - ann for which to clear scaling parameters

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL int FANN_API fann_clear_scaling_params(Fann2 * ann);

/* Function: fann_scale_input

   Scale data in input vector before feeding it to ann based on previously calculated parameters.

   Parameters:
     ann          - for which scaling parameters were calculated
     input_vector - input vector that will be scaled

   See also: <fann_descale_input>, <fann_scale_output>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_scale_input(Fann2 * ann, float * input_vector);

/* Function: fann_scale_output

   Scale data in output vector before feeding it to ann based on previously calculated parameters.

   Parameters:
     ann           - for which scaling parameters were calculated
     output_vector - output vector that will be scaled

   See also: <fann_descale_output>, <fann_scale_input>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_scale_output(Fann2 * ann, float * output_vector);

/* Function: fann_descale_input

   Scale data in input vector after getting it from ann based on previously calculated parameters.

   Parameters:
     ann          - for which scaling parameters were calculated
     input_vector - input vector that will be descaled

   See also: <fann_scale_input>, <fann_descale_output>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_descale_input(Fann2 * ann, float * input_vector);

/* Function: fann_descale_output

   Scale data in output vector after getting it from ann based on previously calculated parameters.

   Parameters:
     ann           - for which scaling parameters were calculated
     output_vector - output vector that will be descaled

   See also: <fann_scale_output>, <fann_descale_input>

    This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_descale_output(Fann2 * ann, float * output_vector);

#endif

/* Function: fann_scale_input_train_data

   Scales the inputs in the training data to the specified range.

   A simplified scaling method, which is mostly useful in examples where it's known that all the
   data will be in one range and it should be transformed to another range.

   It is not recommended to use this on subsets of data as the complete input range might not be
   available in that subset.

   For more powerful scaling, please consider <fann_scale_train>

   See also: <fann_scale_output_train_data>, <fann_scale_train_data>, <fann_scala_input>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_scale_input_train_data(struct fann_train_data * train_data, float new_min, float new_max);

/* Function: fann_scale_output_train_data

   Scales the outputs in the training data to the specified range.

   A simplified scaling method, which is mostly useful in examples where it's known that all the
   data will be in one range and it should be transformed to another range.

   It is not recommended to use this on subsets of data as the complete input range might not be
   available in that subset.

   For more powerful scaling, please consider <fann_scale_train>

   See also: <fann_scale_input_train_data>, <fann_scale_train_data>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_scale_output_train_data(struct fann_train_data * train_data, float new_min, float new_max);

/* Function: fann_scale_train_data

   Scales the inputs and outputs in the training data to the specified range.

   A simplified scaling method, which is mostly useful in examples where it's known that all the
   data will be in one range and it should be transformed to another range.

   It is not recommended to use this on subsets of data as the complete input range might not be
   available in that subset.

   For more powerful scaling, please consider <fann_scale_train>

   See also: <fann_scale_output_train_data>, <fann_scale_input_train_data>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_scale_train_data(struct fann_train_data * train_data, float new_min, float new_max);

/* Function: fann_merge_train_data

   Merges the data from *data1* and *data2* into a new <struct fann_train_data>.

   This function appears in FANN >= 1.1.0.
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_merge_train_data(struct fann_train_data * data1, const struct fann_train_data * data2);

/* Function: fann_duplicate_train_data

   Returns an exact copy of a <struct fann_train_data>.

   This function appears in FANN >= 1.1.0.
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_duplicate_train_data(struct fann_train_data * data);

/* Function: fann_subset_train_data

   Returns an copy of a subset of the <struct fann_train_data>, starting at position *pos*
   and *length* elements forward.

   >fann_subset_train_data(train_data, 0, fann_length_train_data(train_data))

   Will do the same as <fann_duplicate_train_data>.

   See also: <fann_length_train_data>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL struct fann_train_data * FANN_API fann_subset_train_data(struct fann_train_data * data, uint pos, uint length);

/* Function: fann_length_train_data

   Returns the number of training patterns in the <struct fann_train_data>.

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_length_train_data(const struct fann_train_data * data);
// 
// Descr: Returns the number of inputs in each of the training patterns in the <struct fann_train_data>.
// See also: <fann_num_train_data>, <fann_num_output_train_data>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_num_input_train_data(const struct fann_train_data * data);
// 
// Descr: Returns the number of outputs in each of the training patterns in the <struct fann_train_data>.
// See also: <fann_num_train_data>, <fann_num_input_train_data>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_num_output_train_data(const struct fann_train_data * data);
// 
// Descr: Save the training structure to a file, with the format as specified in <fann_read_train_from_file>
// Return:
//   The function returns 0 on success and -1 on failure.
// See also: <fann_read_train_from_file>, <fann_save_train_to_fixed>
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL int FANN_API fann_save_train(struct fann_train_data * data, const char * filename);

/* Function: fann_save_train_to_fixed

   Saves the training structure to a fixed point data file.

   This function is very useful for testing the quality of a fixed point network.

   Return:
   The function returns 0 on success and -1 on failure.

   See also: <fann_save_train>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL int FANN_API fann_save_train_to_fixed(struct fann_train_data * data, const char * filename, uint decimal_point);

/* Group: Parameters */

/* Function: fann_get_training_algorithm

   Return the training algorithm as described by <fann_train_enum>. This training algorithm
   is used by <fann_train_on_data> and associated functions.

   Note that this algorithm is also used during <fann_cascadetrain_on_data>, although only
   FANN_TRAIN_RPROP and FANN_TRAIN_QUICKPROP is allowed during cascade training.

   The default training algorithm is FANN_TRAIN_RPROP.

   See also: <fann_set_training_algorithm>, <fann_train_enum>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL enum fann_train_enum FANN_API fann_get_training_algorithm(Fann2 * ann);

/* Function: fann_set_training_algorithm

   Set the training algorithm.

   More info available in <fann_get_training_algorithm>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_training_algorithm(Fann2 * ann, enum fann_train_enum training_algorithm);

/* Function: fann_get_learning_rate

   Return the learning rate.

   The learning rate is used to determine how aggressive training should be for some of the
   training algorithms (FANN_TRAIN_INCREMENTAL, FANN_TRAIN_BATCH, FANN_TRAIN_QUICKPROP).
   Do however note that it is not used in FANN_TRAIN_RPROP.

   The default learning rate is 0.7.

   See also: <fann_set_learning_rate>, <fann_set_training_algorithm>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL float FANN_API fann_get_learning_rate(Fann2 * ann);

/* Function: fann_set_learning_rate

   Set the learning rate.

   More info available in <fann_get_learning_rate>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_learning_rate(Fann2 * ann, float learning_rate);

/* Function: fann_get_learning_momentum

   Get the learning momentum.

   The learning momentum can be used to speed up FANN_TRAIN_INCREMENTAL training.
   A too high momentum will however not benefit training. Setting momentum to 0 will
   be the same as not using the momentum parameter. The recommended value of this parameter
   is between 0.0 and 1.0.

   The default momentum is 0.

   See also: <fann_set_learning_momentum>, <fann_set_training_algorithm>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL float FANN_API fann_get_learning_momentum(Fann2 * ann);

/* Function: fann_set_learning_momentum

   Set the learning momentum.

   More info available in <fann_get_learning_momentum>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_learning_momentum(Fann2 * ann, float learning_momentum);

/* Function: fann_get_activation_function

   Get the activation function for neuron number *neuron* in layer number *layer*,
   counting the input layer as layer 0.

   It is not possible to get activation functions for the neurons in the input layer.

   Information about the individual activation functions is available at <fann_activationfunc_enum>.

   Returns:
    The activation function for the neuron or -1 if the neuron is not defined in the neural network.

   See also: <fann_set_activation_function_layer>, <fann_set_activation_function_hidden>,
        <fann_set_activation_function_output>, <fann_set_activation_steepness>, <fann_set_activation_function>

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL Fann2::ActivationFunc FANN_API fann_get_activation_function(Fann2 * ann, int layer, int neuron);

/* Function: fann_set_activation_function

   Set the activation function for neuron number *neuron* in layer number *layer*,
   counting the input layer as layer 0.

   It is not possible to set activation functions for the neurons in the input layer.

   When choosing an activation function it is important to note that the activation
   functions have different range. FANN_SIGMOID is e.g. in the 0 - 1 range while
   FANN_SIGMOID_SYMMETRIC is in the -1 - 1 range and FANN_LINEAR is unbounded.

   Information about the individual activation functions is available at <fann_activationfunc_enum>.

   The default activation function is FANN_SIGMOID_STEPWISE.

   See also: <fann_set_activation_function_layer>, <fann_set_activation_function_hidden>,
        <fann_set_activation_function_output>, <fann_set_activation_steepness>, <fann_get_activation_function>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_function(Fann2 * ann, Fann2::ActivationFunc activation_function, int layer, int neuron);

/* Function: fann_set_activation_function_layer

   Set the activation function for all the neurons in the layer number *layer*,
   counting the input layer as layer 0.

   It is not possible to set activation functions for the neurons in the input layer.

   See also: <fann_set_activation_function>, <fann_set_activation_function_hidden>,
        <fann_set_activation_function_output>, <fann_set_activation_steepness_layer>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_function_layer(Fann2 * ann, Fann2::ActivationFunc activation_function, int layer);

/* Function: fann_set_activation_function_hidden

   Set the activation function for all of the hidden layers.

   See also: <fann_set_activation_function>, <fann_set_activation_function_layer>,
        <fann_set_activation_function_output>, <fann_set_activation_steepness_hidden>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_function_hidden(Fann2 * ann, Fann2::ActivationFunc activation_function);

/* Function: fann_set_activation_function_output

   Set the activation function for the output layer.

   See also: <fann_set_activation_function>, <fann_set_activation_function_layer>,
        <fann_set_activation_function_hidden>, <fann_set_activation_steepness_output>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_function_output(Fann2 * ann, Fann2::ActivationFunc activation_function);

/* Function: fann_get_activation_steepness

   Get the activation steepness for neuron number *neuron* in layer number *layer*,
   counting the input layer as layer 0.

   It is not possible to get activation steepness for the neurons in the input layer.

   The steepness of an activation function says something about how fast the activation function
   goes from the minimum to the maximum. A high value for the activation function will also
   give a more aggressive training.

   When training neural networks where the output values should be at the extremes (usually 0 and 1,
   depending on the activation function), a steep activation function can be used (e.g. 1.0).

   The default activation steepness is 0.5.

   Returns:
    The activation steepness for the neuron or -1 if the neuron is not defined in the neural network.

   See also: <fann_set_activation_steepness_layer>, <fann_set_activation_steepness_hidden>,
        <fann_set_activation_steepness_output>, <fann_set_activation_function>, <fann_set_activation_steepness>

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL float FANN_API fann_get_activation_steepness(Fann2 * ann, int layer, int neuron);

/* Function: fann_set_activation_steepness

   Set the activation steepness for neuron number *neuron* in layer number *layer*,
   counting the input layer as layer 0.

   It is not possible to set activation steepness for the neurons in the input layer.

   The steepness of an activation function says something about how fast the activation function
   goes from the minimum to the maximum. A high value for the activation function will also
   give a more aggressive training.

   When training neural networks where the output values should be at the extremes (usually 0 and 1,
   depending on the activation function), a steep activation function can be used (e.g. 1.0).

   The default activation steepness is 0.5.

   See also: <fann_set_activation_steepness_layer>, <fann_set_activation_steepness_hidden>,
        <fann_set_activation_steepness_output>, <fann_set_activation_function>, <fann_get_activation_steepness>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_steepness(Fann2 * ann, float steepness, int layer, int neuron);

/* Function: fann_set_activation_steepness_layer

   Set the activation steepness for all of the neurons in layer number *layer*,
   counting the input layer as layer 0.

   It is not possible to set activation steepness for the neurons in the input layer.

   See also: <fann_set_activation_steepness>, <fann_set_activation_steepness_hidden>,
        <fann_set_activation_steepness_output>, <fann_set_activation_function_layer>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_steepness_layer(Fann2 * ann, float steepness, int layer);

/* Function: fann_set_activation_steepness_hidden

   Set the steepness of the activation steepness in all of the hidden layers.

   See also: <fann_set_activation_steepness>, <fann_set_activation_steepness_layer>,
        <fann_set_activation_steepness_output>, <fann_set_activation_function_hidden>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_steepness_hidden(Fann2 * ann, float steepness);

/* Function: fann_set_activation_steepness_output

   Set the steepness of the activation steepness in the output layer.

   See also: <fann_set_activation_steepness>, <fann_set_activation_steepness_layer>,
        <fann_set_activation_steepness_hidden>, <fann_set_activation_function_output>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_activation_steepness_output(Fann2 * ann, float steepness);

/* Function: fann_get_train_error_function

   Returns the error function used during training.

   The error functions are described further in <fann_errorfunc_enum>

   The default error function is FANN_ERRORFUNC_TANH

   See also: <fann_set_train_error_function>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL enum fann_errorfunc_enum FANN_API fann_get_train_error_function(Fann2 * ann);

/* Function: fann_set_train_error_function

   Set the error function used during training.

   The error functions are described further in <fann_errorfunc_enum>

   See also: <fann_get_train_error_function>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_train_error_function(Fann2 * ann, enum fann_errorfunc_enum train_error_function);

/* Function: fann_get_train_stop_function

   Returns the the stop function used during training.

   The stop function is described further in <fann_stopfunc_enum>

   The default stop function is FANN_STOPFUNC_MSE

   See also: <fann_get_train_stop_function>, <fann_get_bit_fail_limit>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL enum fann_stopfunc_enum FANN_API fann_get_train_stop_function(Fann2 * ann);

/* Function: fann_set_train_stop_function

   Set the stop function used during training.

   Returns the the stop function used during training.

   The stop function is described further in <fann_stopfunc_enum>

   See also: <fann_get_train_stop_function>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_train_stop_function(Fann2 * ann, enum fann_stopfunc_enum train_stop_function);

/* Function: fann_get_bit_fail_limit

   Returns the bit fail limit used during training.

   The bit fail limit is used during training where the <fann_stopfunc_enum> is set to FANN_STOPFUNC_BIT.

   The limit is the maximum accepted difference between the desired output and the actual output during
   training. Each output that diverges more than this limit is counted as an error bit.
   This difference is divided by two when dealing with symmetric activation functions,
   so that symmetric and not symmetric activation functions can use the same limit.

   The default bit fail limit is 0.35.

   See also: <fann_set_bit_fail_limit>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL float FANN_API fann_get_bit_fail_limit(Fann2 * ann);

/* Function: fann_set_bit_fail_limit

   Set the bit fail limit used during training.

   See also: <fann_get_bit_fail_limit>

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_bit_fail_limit(Fann2 * ann, float bit_fail_limit);

/* Function: fann_set_callback

   Sets the callback function for use during training.

   See <fann_callback_type> for more information about the callback function.

   The default callback function simply prints out some status information.

   This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_callback(Fann2 * ann, fann_callback_type callback);

/* Function: fann_get_quickprop_decay

   The decay is a small negative valued number which is the factor that the weights
   should become smaller in each iteration during quickprop training. This is used
   to make sure that the weights do not become too high during training.

   The default decay is -0.0001.

   See also: <fann_set_quickprop_decay>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL float FANN_API fann_get_quickprop_decay(Fann2 * ann);

/* Function: fann_set_quickprop_decay

   Sets the quickprop decay factor.

   See also: <fann_get_quickprop_decay>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_quickprop_decay(Fann2 * ann, float quickprop_decay);

/* Function: fann_get_quickprop_mu

   The mu factor is used to increase and decrease the step-size during quickprop training.
   The mu factor should always be above 1, since it would otherwise decrease the step-size
   when it was supposed to increase it.

   The default mu factor is 1.75.

   See also: <fann_set_quickprop_mu>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL float FANN_API fann_get_quickprop_mu(Fann2 * ann);

/* Function: fann_set_quickprop_mu

    Sets the quickprop mu factor.

   See also: <fann_get_quickprop_mu>

   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_quickprop_mu(Fann2 * ann, float quickprop_mu);

/* Function: fann_get_rprop_increase_factor
   The increase factor is a value larger than 1, which is used to
   increase the step-size during RPROP training.
   The default increase factor is 1.2.
   See also: <fann_set_rprop_increase_factor>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL float FANN_API fann_get_rprop_increase_factor(Fann2 * ann);

/* Function: fann_set_rprop_increase_factor
   The increase factor used during RPROP training.
   See also: <fann_get_rprop_increase_factor>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_rprop_increase_factor(Fann2 * ann, float rprop_increase_factor);

/* Function: fann_get_rprop_decrease_factor
   The decrease factor is a value smaller than 1, which is used to decrease the step-size during RPROP training.
   The default decrease factor is 0.5.
   See also: <fann_set_rprop_decrease_factor>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL float FANN_API fann_get_rprop_decrease_factor(Fann2 * ann);

/* Function: fann_set_rprop_decrease_factor
   The decrease factor is a value smaller than 1, which is used to decrease the step-size during RPROP training.
   See also: <fann_get_rprop_decrease_factor>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_rprop_decrease_factor(Fann2 * ann, float rprop_decrease_factor);

/* Function: fann_get_rprop_delta_min
   The minimum step-size is a small positive number determining how small the minimum step-size may be.
   The default value delta min is 0.0.
   See also: <fann_set_rprop_delta_min>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL float FANN_API fann_get_rprop_delta_min(Fann2 * ann);

/* Function: fann_set_rprop_delta_min
   The minimum step-size is a small positive number determining how small the minimum step-size may be.
   See also: <fann_get_rprop_delta_min>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_rprop_delta_min(Fann2 * ann, float rprop_delta_min);

/* Function: fann_get_rprop_delta_max
   The maximum step-size is a positive number determining how large the maximum step-size may be.
   The default delta max is 50.0.
   See also: <fann_set_rprop_delta_max>, <fann_get_rprop_delta_min>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL float FANN_API fann_get_rprop_delta_max(Fann2 * ann);

/* Function: fann_set_rprop_delta_max
   The maximum step-size is a positive number determining how large the maximum step-size may be.
   See also: <fann_get_rprop_delta_max>, <fann_get_rprop_delta_min>
   This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_rprop_delta_max(Fann2 * ann, float rprop_delta_max);

/* Function: fann_get_rprop_delta_zero
   The initial step-size is a positive number determining the initial step size.
   The default delta zero is 0.1.
   See also: <fann_set_rprop_delta_zero>, <fann_get_rprop_delta_min>, <fann_get_rprop_delta_max>
   This function appears in FANN >= 2.1.0.
 */
FANN_EXTERNAL float FANN_API fann_get_rprop_delta_zero(Fann2 * ann);

/* Function: fann_set_rprop_delta_zero
   The initial step-size is a positive number determining the initial step size.
   See also: <fann_get_rprop_delta_zero>, <fann_get_rprop_delta_zero>
   This function appears in FANN >= 2.1.0.
 */
FANN_EXTERNAL void FANN_API fann_set_rprop_delta_zero(Fann2 * ann, float rprop_delta_max);

/* Method: fann_get_sarprop_weight_decay_shift
   The sarprop weight decay shift.
   The default delta max is -6.644.
   See also: <fann fann_set_sarprop_weight_decay_shift>
   This function appears in FANN >= 2.1.0.
 */
FANN_EXTERNAL float FANN_API fann_get_sarprop_weight_decay_shift(Fann2 * ann);

/* Method: fann_set_sarprop_weight_decay_shift
   Set the sarprop weight decay shift.
   This function appears in FANN >= 2.1.0.
   See also: <fann_set_sarprop_weight_decay_shift>
 */
FANN_EXTERNAL void FANN_API fann_set_sarprop_weight_decay_shift(Fann2 * ann, float sarprop_weight_decay_shift);

/* Method: fann_get_sarprop_step_error_threshold_factor
   The sarprop step error threshold factor.
   The default delta max is 0.1.
   See also: <fann fann_get_sarprop_step_error_threshold_factor>
   This function appears in FANN >= 2.1.0.
 */
FANN_EXTERNAL float FANN_API fann_get_sarprop_step_error_threshold_factor(Fann2 * ann);

/* Method: fann_set_sarprop_step_error_threshold_factor
   Set the sarprop step error threshold factor.
   This function appears in FANN >= 2.1.0.
   See also: <fann_get_sarprop_step_error_threshold_factor>
 */
FANN_EXTERNAL void FANN_API fann_set_sarprop_step_error_threshold_factor(Fann2 * ann, float sarprop_step_error_threshold_factor);

/* Method: fann_get_sarprop_step_error_shift
   The get sarprop step error shift.
   The default delta max is 1.385.
   See also: <fann_set_sarprop_step_error_shift>
   This function appears in FANN >= 2.1.0.
 */
FANN_EXTERNAL float FANN_API fann_get_sarprop_step_error_shift(Fann2 * ann);

/* Method: fann_set_sarprop_step_error_shift
   Set the sarprop step error shift.
   This function appears in FANN >= 2.1.0.
   See also: <fann_get_sarprop_step_error_shift>
 */
FANN_EXTERNAL void FANN_API fann_set_sarprop_step_error_shift(Fann2 * ann, float sarprop_step_error_shift);

/* Method: fann_get_sarprop_temperature
   The sarprop weight decay shift.
   The default delta max is 0.015.
   See also: <fann_set_sarprop_temperature>
   This function appears in FANN >= 2.1.0.
 */
FANN_EXTERNAL float FANN_API fann_get_sarprop_temperature(Fann2 * ann);

/* Method: fann_set_sarprop_temperature
   Set the sarprop_temperature.
   This function appears in FANN >= 2.1.0.
   See also: <fann_get_sarprop_temperature>
 */
FANN_EXTERNAL void FANN_API fann_set_sarprop_temperature(Fann2 * ann, float sarprop_temperature);
//
//#include "fann_cascade.h"
/* Section: FANN Cascade Training
   Cascade training differs from ordinary training in the sense that it starts with an empty neural network
   and then adds neurons one by one, while it trains the neural network. The main benefit of this approach
   is that you do not have to guess the number of hidden layers and neurons prior to training, but cascade
   training has also proved better at solving some problems.

   The basic idea of cascade training is that a number of candidate neurons are trained separate from the
   real network, then the most promising of these candidate neurons is inserted into the neural network.
   Then the output connections are trained and new candidate neurons are prepared. The candidate neurons are
   created as shortcut connected neurons in a new hidden layer, which means that the final neural network
   will consist of a number of hidden layers with one shortcut connected neuron in each.
 */

/* Group: Cascade Training */

#ifndef FIXEDFANN // {
/* Function: fann_cascadetrain_on_data

   Trains on an entire dataset, for a period of time using the Cascade2 training algorithm.
   This algorithm adds neurons to the neural network while training, which means that it
   needs to start with an ANN without any hidden layers. The neural network should also use
   shortcut connections, so <fann_create_shortcut> should be used to create the ANN like this:
   >Fann2 *ann = fann_create_shortcut(2, fann_num_input_train_data(train_data),
      fann_num_output_train_data(train_data));

   This training uses the parameters set using the fann_set_cascade_..., but it also uses another
   training algorithm as it's internal training algorithm. This algorithm can be set to either
   FANN_TRAIN_RPROP or FANN_TRAIN_QUICKPROP by <fann_set_training_algorithm>, and the parameters
   set for these training algorithms will also affect the cascade training.

   Parameters:
                ann - The neural network
                data - The data, which should be used during training
                max_neuron - The maximum number of neurons to be added to neural network
                neurons_between_reports - The number of neurons between printing a status report to stdout.
                        A value of zero means no reports should be printed.
                desired_error - The desired <fann_get_MSE> or <fann_get_bit_fail>, depending on which stop function
                        is chosen by <fann_set_train_stop_function>.

        Instead of printing out reports every neurons_between_reports, a callback function can be called
        (see <fann_set_callback>).

        See also: <fann_train_on_data>, <fann_cascadetrain_on_file>, <Parameters>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_cascadetrain_on_data(Fann2 * ann, struct fann_train_data * data, uint max_neurons, uint neurons_between_reports, float desired_error);
// 
// Does the same as <fann_cascadetrain_on_data>, but reads the training data directly from a file.
// See also: <fann_cascadetrain_on_data>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL void FANN_API fann_cascadetrain_on_file(Fann2 * ann, const char * filename, uint max_neurons, uint neurons_between_reports, float desired_error);

/* Group: Parameters */

/* Function: fann_get_cascade_output_change_fraction

   The cascade output change fraction is a number between 0 and 1 determining how large a fraction
   the <fann_get_MSE> value should change within <fann_get_cascade_output_stagnation_epochs> during
   training of the output connections, in order for the training not to stagnate. If the training
   stagnates, the training of the output connections will be ended and new candidates will be prepared.

   This means:
   If the MSE does not change by a fraction of <fann_get_cascade_output_change_fraction> during a
   period of <fann_get_cascade_output_stagnation_epochs>, the training of the output connections
   is stopped because the training has stagnated.

   If the cascade output change fraction is low, the output connections will be trained more and if the
   fraction is high they will be trained less.

   The default cascade output change fraction is 0.01, which is equivalent to a 1% change in MSE.

   See also: <fann_set_cascade_output_change_fraction>, <fann_get_MSE>, <fann_get_cascade_output_stagnation_epochs>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL float FANN_API fann_get_cascade_output_change_fraction(Fann2 * ann);
// 
// Descr: Sets the cascade output change fraction.
// See also: <fann_get_cascade_output_change_fraction>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL void FANN_API fann_set_cascade_output_change_fraction(Fann2 * ann, float cascade_output_change_fraction);
// 
// Descr: The number of cascade output stagnation epochs determines the number of epochs training is allowed to
// continue without changing the MSE by a fraction of <fann_get_cascade_output_change_fraction>.
// 
// See more info about this parameter in <fann_get_cascade_output_change_fraction>.
// 
// The default number of cascade output stagnation epochs is 12.
// See also: <fann_set_cascade_output_stagnation_epochs>, <fann_get_cascade_output_change_fraction>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_get_cascade_output_stagnation_epochs(Fann2 * ann);
// 
// Descr: Sets the number of cascade output stagnation epochs.
// See also: <fann_get_cascade_output_stagnation_epochs>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL void FANN_API fann_set_cascade_output_stagnation_epochs(Fann2 * ann, uint cascade_output_stagnation_epochs);
// 
// Descr: The cascade candidate change fraction is a number between 0 and 1 determining how large a fraction
// the <fann_get_MSE> value should change within <fann_get_cascade_candidate_stagnation_epochs> during
// training of the candidate neurons, in order for the training not to stagnate. If the training
// stagnates, the training of the candidate neurons will be ended and the best candidate will be selected.
// This means:
// If the MSE does not change by a fraction of <fann_get_cascade_candidate_change_fraction> during a
// period of <fann_get_cascade_candidate_stagnation_epochs>, the training of the candidate neurons
// is stopped because the training has stagnated.
// 
// If the cascade candidate change fraction is low, the candidate neurons will be trained more and if the
// fraction is high they will be trained less.
// The default cascade candidate change fraction is 0.01, which is equivalent to a 1% change in MSE.
// 
// See also: <fann_set_cascade_candidate_change_fraction>, <fann_get_MSE>, <fann_get_cascade_candidate_stagnation_epochs>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL float FANN_API fann_get_cascade_candidate_change_fraction(Fann2 * ann);
// 
// Sets the cascade candidate change fraction.
// See also: <fann_get_cascade_candidate_change_fraction>
// This function appears in FANN >= 2.0.0.
//
FANN_EXTERNAL void FANN_API fann_set_cascade_candidate_change_fraction(Fann2 * ann, float cascade_candidate_change_fraction);

/* Function: fann_get_cascade_candidate_stagnation_epochs

   The number of cascade candidate stagnation epochs determines the number of epochs training is allowed to
   continue without changing the MSE by a fraction of <fann_get_cascade_candidate_change_fraction>.

   See more info about this parameter in <fann_get_cascade_candidate_change_fraction>.

   The default number of cascade candidate stagnation epochs is 12.

   See also: <fann_set_cascade_candidate_stagnation_epochs>, <fann_get_cascade_candidate_change_fraction>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_candidate_stagnation_epochs(Fann2 * ann);

/* Function: fann_set_cascade_candidate_stagnation_epochs

   Sets the number of cascade candidate stagnation epochs.

   See also: <fann_get_cascade_candidate_stagnation_epochs>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_candidate_stagnation_epochs(Fann2 * ann, uint cascade_candidate_stagnation_epochs);

/* Function: fann_get_cascade_weight_multiplier

   The weight multiplier is a parameter which is used to multiply the weights from the candidate neuron
   before adding the neuron to the neural network. This parameter is usually between 0 and 1, and is used
   to make the training a bit less aggressive.

   The default weight multiplier is 0.4

   See also: <fann_set_cascade_weight_multiplier>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL float FANN_API fann_get_cascade_weight_multiplier(Fann2 * ann);

/* Function: fann_set_cascade_weight_multiplier
   Sets the weight multiplier.
   See also: <fann_get_cascade_weight_multiplier>
        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_weight_multiplier(Fann2 * ann, float cascade_weight_multiplier);

/* Function: fann_get_cascade_candidate_limit
   The candidate limit is a limit for how much the candidate neuron may be trained.
   The limit is a limit on the proportion between the MSE and candidate score.
   Set this to a lower value to avoid overfitting and to a higher if overfitting is
   not a problem.
   The default candidate limit is 1000.0
   See also: <fann_set_cascade_candidate_limit>
	This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL float FANN_API fann_get_cascade_candidate_limit(Fann2 * ann);

/* Function: fann_set_cascade_candidate_limit

   Sets the candidate limit.

   See also: <fann_get_cascade_candidate_limit>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_candidate_limit(Fann2 * ann, float cascade_candidate_limit);

/* Function: fann_get_cascade_max_out_epochs

   The maximum out epochs determines the maximum number of epochs the output connections
   may be trained after adding a new candidate neuron.

   The default max out epochs is 150

   See also: <fann_set_cascade_max_out_epochs>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_max_out_epochs(Fann2 * ann);

/* Function: fann_set_cascade_max_out_epochs

   Sets the maximum out epochs.

   See also: <fann_get_cascade_max_out_epochs>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_max_out_epochs(Fann2 * ann, uint cascade_max_out_epochs);

/* Function: fann_get_cascade_min_out_epochs

   The minimum out epochs determines the minimum number of epochs the output connections
   must be trained after adding a new candidate neuron.

   The default min out epochs is 50

   See also: <fann_set_cascade_min_out_epochs>

        This function appears in FANN >= 2.2.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_min_out_epochs(Fann2 * ann);

/* Function: fann_set_cascade_min_out_epochs

   Sets the minimum out epochs.

   See also: <fann_get_cascade_min_out_epochs>

        This function appears in FANN >= 2.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_min_out_epochs(Fann2 * ann, uint cascade_min_out_epochs);

/* Function: fann_get_cascade_max_cand_epochs

   The maximum candidate epochs determines the maximum number of epochs the input
   connections to the candidates may be trained before adding a new candidate neuron.

   The default max candidate epochs is 150

   See also: <fann_set_cascade_max_cand_epochs>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_max_cand_epochs(Fann2 * ann);

/* Function: fann_set_cascade_max_cand_epochs

   Sets the max candidate epochs.

   See also: <fann_get_cascade_max_cand_epochs>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_max_cand_epochs(Fann2 * ann, uint cascade_max_cand_epochs);

/* Function: fann_get_cascade_min_cand_epochs

   The minimum candidate epochs determines the minimum number of epochs the input
   connections to the candidates may be trained before adding a new candidate neuron.

   The default min candidate epochs is 50

   See also: <fann_set_cascade_min_cand_epochs>

        This function appears in FANN >= 2.2.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_min_cand_epochs(Fann2 * ann);

/* Function: fann_set_cascade_min_cand_epochs

   Sets the min candidate epochs.

   See also: <fann_get_cascade_min_cand_epochs>

        This function appears in FANN >= 2.2.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_min_cand_epochs(Fann2 * ann, uint cascade_min_cand_epochs);

/* Function: fann_get_cascade_num_candidates

   The number of candidates used during training (calculated by multiplying
      <fann_get_cascade_activation_functions_count>,
   <fann_get_cascade_activation_steepnesses_count> and <fann_get_cascade_num_candidate_groups>).

   The actual candidates is defined by the <fann_get_cascade_activation_functions> and
   <fann_get_cascade_activation_steepnesses> arrays. These arrays define the activation functions
   and activation steepnesses used for the candidate neurons. If there are 2 activation functions
   in the activation function array and 3 steepnesses in the steepness array, then there will be
   2x3=6 different candidates which will be trained. These 6 different candidates can be copied into
   several candidate groups, where the only difference between these groups is the initial weights.
   If the number of groups is set to 2, then the number of candidate neurons will be 2x3x2=12. The
   number of candidate groups is defined by <fann_set_cascade_num_candidate_groups>.

   The default number of candidates is 6x4x2 = 48

   See also: <fann_get_cascade_activation_functions>, <fann_get_cascade_activation_functions_count>,
	<fann_get_cascade_activation_steepnesses>, <fann_get_cascade_activation_steepnesses_count>, <fann_get_cascade_num_candidate_groups>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_num_candidates(const Fann2 * ann);

/* Function: fann_get_cascade_activation_functions_count

   The number of activation functions in the <fann_get_cascade_activation_functions> array.

   The default number of activation functions is 10.

   See also: <fann_get_cascade_activation_functions>, <fann_set_cascade_activation_functions>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_activation_functions_count(Fann2 * ann);

/* Function: fann_get_cascade_activation_functions

   The cascade activation functions array is an array of the different activation functions used by
   the candidates.

   See <fann_get_cascade_num_candidates> for a description of which candidate neurons will be
   generated by this array.

   The default activation functions are {FANN_SIGMOID, FANN_SIGMOID_SYMMETRIC, FANN_GAUSSIAN,
   FANN_GAUSSIAN_SYMMETRIC, FANN_ELLIOT, FANN_ELLIOT_SYMMETRIC, FANN_SIN_SYMMETRIC,
   FANN_COS_SYMMETRIC, FANN_SIN, FANN_COS}

   See also: <fann_get_cascade_activation_functions_count>, <fann_set_cascade_activation_functions>, <fann_activationfunc_enum>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL Fann2::ActivationFunc * FANN_API fann_get_cascade_activation_functions(Fann2 * ann);

/* Function: fann_set_cascade_activation_functions

   Sets the array of cascade candidate activation functions. The array must be just as long
   as defined by the count.

   See <fann_get_cascade_num_candidates> for a description of which candidate neurons will be
   generated by this array.

   See also: <fann_get_cascade_activation_steepnesses_count>, <fann_get_cascade_activation_steepnesses>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_activation_functions(Fann2 * ann, const Fann2::ActivationFunc * cascade_activation_functions, uint cascade_activation_functions_count);
// 
// The number of activation steepnesses in the <fann_get_cascade_activation_functions> array.
// The default number of activation steepnesses is 4.
// See also: <fann_get_cascade_activation_steepnesses>, <fann_set_cascade_activation_functions>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_get_cascade_activation_steepnesses_count(Fann2 * ann);
// 
// The cascade activation steepnesses array is an array of the different activation functions used by the candidates.
// 
// See <fann_get_cascade_num_candidates> for a description of which candidate neurons will be
// generated by this array.
// The default activation steepnesses is {0.25, 0.50, 0.75, 1.00}
// See also: <fann_set_cascade_activation_steepnesses>, <fann_get_cascade_activation_steepnesses_count>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL float * FANN_API fann_get_cascade_activation_steepnesses(Fann2 * ann);

/* Function: fann_set_cascade_activation_steepnesses

   Sets the array of cascade candidate activation steepnesses. The array must be just as long
   as defined by the count.

   See <fann_get_cascade_num_candidates> for a description of which candidate neurons will be
   generated by this array.

   See also: <fann_get_cascade_activation_steepnesses>, <fann_get_cascade_activation_steepnesses_count>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_activation_steepnesses(Fann2 * ann, const float * cascade_activation_steepnesses, uint cascade_activation_steepnesses_count);

/* Function: fann_get_cascade_num_candidate_groups
   The number of candidate groups is the number of groups of identical candidates which will be used
   during training.
   This number can be used to have more candidates without having to define new parameters for the candidates.
   See <fann_get_cascade_num_candidates> for a description of which candidate neurons will be
   generated by this parameter.
   The default number of candidate groups is 2
   See also: <fann_set_cascade_num_candidate_groups>
        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_cascade_num_candidate_groups(Fann2 * ann);

/* Function: fann_set_cascade_num_candidate_groups
   Sets the number of candidate groups.
   See also: <fann_get_cascade_num_candidate_groups>
        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL void FANN_API fann_set_cascade_num_candidate_groups(Fann2 * ann, uint cascade_num_candidate_groups);
#endif // } FIXEDFANN 
//
//#include "fann_io.h"
/* Section: FANN File Input/Output

   It is possible to save an entire ann to a file with <fann_save> for future loading with <fann_create_from_file>.
 */

/* Group: File Input and Output */

/* Function: fann_create_from_file

   Constructs a backpropagation neural network from a configuration file, which has been saved by <fann_save>.

   See also: <fann_save>, <fann_save_to_fixed>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL Fann2 * FANN_API fann_create_from_file(const char * configuration_file);

/* Function: fann_save

   Save the entire network to a configuration file.

   The configuration file contains all information about the neural network and enables
   <fann_create_from_file> to create an exact copy of the neural network and all of the
   parameters associated with the neural network.

   These three parameters (<fann_set_callback>, <fann_set_error_log>,
   <fann_set_user_data>) are *NOT* saved to the file because they cannot safely be
   ported to a different location. Also temporary parameters generated during training
   like <fann_get_MSE> are not saved.

   Return:
   The function returns 0 on success and -1 on failure.

   See also: <fann_create_from_file>, <fann_save_to_fixed>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL int FANN_API fann_save(Fann2 * ann, const char * configuration_file);

/* Function: fann_save_to_fixed

   Saves the entire network to a configuration file.
   But it is saved in fixed point format no matter which
   format it is currently in.

   This is useful for training a network in floating points,
   and then later executing it in fixed point.

   The function returns the bit position of the fix point, which
   can be used to find out how accurate the fixed point network will be.
   A high value indicates high precision, and a low value indicates low
   precision.

   A negative value indicates very low precision, and a very
   strong possibility for overflow.
   (the actual fix point will be set to 0, since a negative
   fix point does not make sense).

   Generally, a fix point lower than 6 is bad, and should be avoided.
   The best way to avoid this, is to have less connections to each neuron,
   or just less neurons in each layer.

   The fixed point use of this network is only intended for use on machines that
   have no floating point processor, like an iPAQ. On normal computers the floating
   point version is actually faster.

   See also: <fann_create_from_file>, <fann_save>

   This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL int FANN_API fann_save_to_fixed(Fann2 * ann, const char * configuration_file);
//
/* Function: fann_create_standard

        Creates a standard fully connected backpropagation neural network.

        There will be a bias neuron in each layer (except the output layer),
        and this bias neuron will be connected to all neurons in the next layer.
        When running the network, the bias nodes always emits 1.

        To destroy a <Fann2> use the <fann_destroy> function.

        Parameters:
                num_layers - The total number of layers including the input and the output layer.
                ... - Integer values determining the number of neurons in each layer starting with the
                        input layer and ending with the output layer.

        Returns:
                A pointer to the newly created <Fann2>.

        Example:
                > // Creating an ANN with 2 input neurons, 1 output neuron,
                > // and two hidden layers with 8 and 9 neurons
                > Fann2 *ann = fann_create_standard(4, 2, 8, 9, 1);

        See also:
                <fann_create_standard_array>, <fann_create_sparse>, <fann_create_shortcut>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL Fann2 * FANN_API fann_create_standard_2(uint num_layers, ...);

/* Function: fann_create_standard_array
   Just like <fann_create_standard>, but with an array of layer sizes
   instead of individual parameters.

        Example:
                > // Creating an ANN with 2 input neurons, 1 output neuron,
                > // and two hidden layers with 8 and 9 neurons
                > uint layers[4] = {2, 8, 9, 1};
                > Fann2 *ann = fann_create_standard_array(4, layers);

        See also:
                <fann_create_standard>, <fann_create_sparse>, <fann_create_shortcut>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL Fann2 * FANN_API fann_create_standard_array(uint num_layers, const uint * layers);
// 
// Creates a standard backpropagation neural network, which is not fully connected.
// Parameters:
//   connection_rate - The connection rate controls how many connections there will be in the
//    network. If the connection rate is set to 1, the network will be fully
//    connected, but if it is set to 0.5 only half of the connections will be set.
//    A connection rate of 1 will yield the same result as <fann_create_standard>
//   num_layers - The total number of layers including the input and the output layer.
//    ... - Integer values determining the number of neurons in each layer starting with the
//    input layer and ending with the output layer.
// Returns:
//    A pointer to the newly created <Fann2>.
// See also: <fann_create_sparse_array>, <fann_create_standard>, <fann_create_shortcut>
// This function appears in FANN >= 2.0.0.
// 
FANN_EXTERNAL Fann2 * FANN_API fann_create_sparse_2(float connection_rate, uint num_layers, ...);

/* Function: fann_create_sparse_array
   Just like <fann_create_sparse>, but with an array of layer sizes
   instead of individual parameters.

        See <fann_create_standard_array> for a description of the parameters.

        See also:
                <fann_create_sparse>, <fann_create_standard>, <fann_create_shortcut>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL Fann2 * FANN_API fann_create_sparse_array(float connection_rate, uint num_layers, const uint * layers);

/* Function: fann_create_shortcut

        Creates a standard backpropagation neural network, which is fully connected and which
        also has shortcut connections.

        Shortcut connections are connections that skip layers. A fully connected network with shortcut
        connections is a network where all neurons are connected to all neurons in later layers.
        Including direct connections from the input layer to the output layer.

        See <fann_create_standard> for a description of the parameters.

        See also:
                <fann_create_shortcut_array>, <fann_create_standard>, <fann_create_sparse>,

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL Fann2 * FANN_API fann_create_shortcut_2(uint num_layers, ...);

/* Function: fann_create_shortcut_array
   Just like <fann_create_shortcut>, but with an array of layer sizes
   instead of individual parameters.

        See <fann_create_standard_array> for a description of the parameters.

        See also:
                <fann_create_shortcut>, <fann_create_standard>, <fann_create_sparse>

        This function appears in FANN >= 2.0.0.
 */
FANN_EXTERNAL Fann2 * FANN_API fann_create_shortcut_array(uint num_layers, const uint * layers);
/* Function: fann_destroy
   Destroys the entire network, properly freeing all the associated memory.

        This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL void FANN_API fann_destroy(Fann2 * ann);

/* Function: fann_copy
   Creates a copy of a fann structure.

   Data in the user data <fann_set_user_data> is not copied, but the user data pointer is copied.

        This function appears in FANN >= 2.2.0.
 */
FANN_EXTERNAL Fann2 * FANN_API fann_copy(Fann2 * ann);

/* Function: fann_run
        Will run input through the neural network, returning an array of outputs, the number of which being
        equal to the number of neurons in the output layer.

        See also:
                <fann_test>

        This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL float * FANN_API fann_run(Fann2 * ann, float * input);

/* Function: fann_randomize_weights
        Give each connection a random weight between *min_weight* and *max_weight*

        From the beginning the weights are random between -0.1 and 0.1.

        See also:
                <fann_init_weights>

        This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL void FANN_API fann_randomize_weights(Fann2 * ann, float min_weight, float max_weight);

/* Function: fann_init_weights
        Initialize the weights using Widrow + Nguyen's algorithm.

        This function behaves similarly to fann_randomize_weights. It will use the algorithm developed
        by Derrick Nguyen and Bernard Widrow to set the weights in such a way
        as to speed up training. This technique is not always successful, and in some cases can be less
        efficient than a purely random initialization.

        The algorithm requires access to the range of the input data (ie, largest and smallest input),
        and therefore accepts a second argument, data, which is the training data that will be used to
        train the network.

        See also:
                <fann_randomize_weights>, <fann_read_train_from_file>

        This function appears in FANN >= 1.1.0.
 */
FANN_EXTERNAL void FANN_API fann_init_weights(Fann2 * ann, struct fann_train_data * train_data);

/* Function: fann_print_connections
        Will print the connections of the ann in a compact matrix, for easy viewing of the internals
        of the ann.

        The output from fann_print_connections on a small (2 2 1) network trained on the xor problem
        >Layer / Neuron 012345
        >L   1 / N    3 BBa...
        >L   1 / N    4 BBA...
        >L   1 / N    5 ......
        >L   2 / N    6 ...BBA
        >L   2 / N    7 ......

        This network has five real neurons and two bias neurons. This gives a total of seven neurons
        named from 0 to 6. The connections between these neurons can be seen in the matrix. "." is a
        place where there is no connection, while a character tells how strong the connection is on a
        scale from a-z. The two real neurons in the hidden layer (neuron 3 and 4 in layer 1) have
        connections from the three neurons in the previous layer as is visible in the first two lines.
        The output neuron (6) has connections from the three neurons in the hidden layer 3 - 5 as is
        visible in the fourth line.

        To simplify the matrix output neurons are not visible as neurons that connections can come from,
        and input and bias neurons are not visible as neurons that connections can go to.

        This function appears in FANN >= 1.2.0.
 */
FANN_EXTERNAL void FANN_API fann_print_connections(Fann2 * ann);
// 
// Group: Parameters 
//

// 
// Descr: Prints all of the parameters and options of the ANN
// This function appears in FANN >= 1.2.0.
// 
FANN_EXTERNAL void FANN_API fann_print_parameters(Fann2 * ann);
// 
// Get the number of input neurons.
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_get_num_input(Fann2 * ann);
// 
// Get the number of output neurons.
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_get_num_output(Fann2 * ann);
// 
// Get the total number of neurons in the entire network. This number does also include the
//   bias neurons, so a 2-4-2 network has 2+4+2 +2(bias) = 10 neurons.
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_get_total_neurons(const Fann2 * ann);
// 
// Get the total number of connections in the entire network.
// This function appears in FANN >= 1.0.0.
// 
FANN_EXTERNAL uint FANN_API fann_get_total_connections(Fann2 * ann);
// 
// Get the type of neural network it was created as.
// Parameters:
//   ann - A previously created neural network structure of type <Fann2> pointer.
// Returns:
//   The neural network type from enum <fann_network_type_enum>
// See Also: <fann_network_type_enum>
// This function appears in FANN >= 2.1.0
// 
FANN_EXTERNAL enum fann_nettype_enum FANN_API fann_get_network_type(const Fann2 * ann);

/* Function: fann_get_connection_rate

    Get the connection rate used when the network was created

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

        Returns:
        The connection rate

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL float FANN_API fann_get_connection_rate(const Fann2 * ann);

/* Function: fann_get_num_layers

    Get the number of layers in the network

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

        Returns:
                The number of layers in the neural network

        Example:
                > // Obtain the number of layers in a neural network
                > Fann2 *ann = fann_create_standard(4, 2, 8, 9, 1);
        > uint num_layers = fann_get_num_layers(ann);

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL uint FANN_API fann_get_num_layers(const Fann2 * ann);

/*Function: fann_get_layer_array

    Get the number of neurons in each layer in the network.

    Bias is not included so the layers match the fann_create functions.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

    The layers array must be preallocated to at least
    sizeof(uint) * fann_num_layers() long.

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_get_layer_array(Fann2 * ann, uint * layers);

/* Function: fann_get_bias_array

    Get the number of bias in each layer in the network.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

    The bias array must be preallocated to at least
    sizeof(uint) * fann_num_layers() long.

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_get_bias_array(Fann2 * ann, uint * bias);

/* Function: fann_get_connection_array

    Get the connections in the network.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

    The connections array must be preallocated to at least
    sizeof(struct fann_connection) * fann_get_total_connections() long.

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_get_connection_array(Fann2 * ann, struct fann_connection * connections);

/* Function: fann_set_weight_array

    Set connections in the network.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

    Only the weights can be changed, connections and weights are ignored
    if they do not already exist in the network.

    The array must have sizeof(struct fann_connection) * num_connections size.

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_set_weight_array(Fann2 * ann, const struct fann_connection * connections, uint num_connections);

/* Function: fann_set_weight

    Set a connection in the network.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

    Only the weights can be changed. The connection/weight is
    ignored if it does not already exist in the network.

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_set_weight(Fann2 * ann,
    uint from_neuron, uint to_neuron, float weight);

/* Function: fann_get_weights

    Get all the network weights.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.
                weights - A float pointer to user data. It is the responsibility
                        of the user to allocate sufficient space to store all the weights.

   This function appears in FANN >= x.y.z
 */
FANN_EXTERNAL void FANN_API fann_get_weights(const Fann2 * ann, float * weights);

/* Function: fann_set_weights

    Set network weights.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.
                weights - A float pointer to user data. It is the responsibility
                        of the user to make the weights array sufficient long
                        to store all the weights.

   This function appears in FANN >= x.y.z
 */
FANN_EXTERNAL void FANN_API fann_set_weights(Fann2 * ann, const float * weights);

/* Function: fann_set_user_data

    Store a pointer to user defined data. The pointer can be
    retrieved with <fann_get_user_data> for example in a
    callback. It is the user's responsibility to allocate and
    deallocate any data that the pointer might point to.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.
                user_data - A void pointer to user defined data.

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void FANN_API fann_set_user_data(Fann2 * ann, void * user_data);

/* Function: fann_get_user_data

    Get a pointer to user defined data that was previously set
    with <fann_set_user_data>. It is the user's responsibility to
    allocate and deallocate any data that the pointer might point to.

    Parameters:
                ann - A previously created neural network structure of
            type <Fann2> pointer.

    Returns:
        A void pointer to user defined data.

   This function appears in FANN >= 2.1.0
 */
FANN_EXTERNAL void * FANN_API fann_get_user_data(Fann2 * ann);

/* Function: fann_disable_seed_rand

   Disables the automatic random generator seeding that happens in FANN.

   Per default FANN will always seed the random generator when creating a new network,
   unless FANN_NO_SEED is defined during compilation of the library. This method can
   disable this at runtime.

   This function appears in FANN >= 2.3.0
 */
FANN_EXTERNAL void FANN_API fann_disable_seed_rand();

/* Function: fann_enable_seed_rand

   Enables the automatic random generator seeding that happens in FANN.

   Per default FANN will always seed the random generator when creating a new network,
   unless FANN_NO_SEED is defined during compilation of the library. This method can
   disable this at runtime.

   This function appears in FANN >= 2.3.0
 */
FANN_EXTERNAL void FANN_API fann_enable_seed_rand();

#ifdef FIXEDFANN

/* Function: fann_get_decimal_point

        Returns the position of the decimal point in the ann.

        This function is only available when the ANN is in fixed point mode.

        The decimal point is described in greater detail in the tutorial <Fixed Point Usage>.

        See also:
                <Fixed Point Usage>, <fann_get_multiplier>, <fann_save_to_fixed>, <fann_save_train_to_fixed>

        This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_decimal_point(Fann2 * ann);

/* Function: fann_get_multiplier

    returns the multiplier that fix point data is multiplied with.

        This function is only available when the ANN is in fixed point mode.

        The multiplier is the used to convert between floating point and fixed point notation.
        A floating point number is multiplied with the multiplier in order to get the fixed point
        number and visa versa.

        The multiplier is described in greater detail in the tutorial <Fixed Point Usage>.

        See also:
                <Fixed Point Usage>, <fann_get_decimal_point>, <fann_save_to_fixed>, <fann_save_train_to_fixed>

        This function appears in FANN >= 1.0.0.
 */
FANN_EXTERNAL uint FANN_API fann_get_multiplier(Fann2 * ann);

#endif  /* FIXEDFANN */

#ifdef __cplusplus
#ifndef __cplusplus
/* to fool automatic indention engines */
{
#endif
}
#endif  /* __cplusplus */
#endif  /* __fann_h__ */
#endif /* NOT FANN_INCLUDE */
