/*
 * parallel_FANN.h
 *
 *     Author: Alessandro Pietro Bardelli
 */
#ifndef DISABLE_PARALLEL_FANN
#ifndef PARALLEL_FANN_H_
#define PARALLEL_FANN_H_

#include "fann.h"

#ifdef __cplusplus
extern "C"
{
	
#ifndef __cplusplus
} /* to fool automatic indention engines */ 
#endif
#endif	/* __cplusplus */

#ifndef FIXEDFANN
FANN_EXTERNAL float FANN_API fann_train_epoch_batch_parallel(Fann *ann, FannTrainData *data, const uint threadnumb);
FANN_EXTERNAL float FANN_API fann_train_epoch_irpropm_parallel(Fann *ann, FannTrainData *data, const uint threadnumb);
FANN_EXTERNAL float FANN_API fann_train_epoch_quickprop_parallel(Fann *ann, FannTrainData *data, const uint threadnumb);
FANN_EXTERNAL float FANN_API fann_train_epoch_sarprop_parallel(Fann *ann, FannTrainData *data, const uint threadnumb);
FANN_EXTERNAL float FANN_API fann_train_epoch_incremental_mod(Fann *ann, FannTrainData *data);
FANN_EXTERNAL float FANN_API fann_test_data_parallel(Fann *ann, FannTrainData *data, const uint threadnumb);
#endif /* FIXEDFANN */

#ifdef __cplusplus
#ifndef __cplusplus
/* to fool automatic indention engines */ 
{
	
#endif
} 
#endif	/* __cplusplus */

#endif /* PARALLEL_FANN_H_ */
#endif /* DISABLE_PARALLEL_FANN */
