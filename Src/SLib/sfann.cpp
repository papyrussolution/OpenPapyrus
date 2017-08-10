// SFANN.CPP
// Copyright (C) 2003-2016 Steffen Nissen (steffen.fann@gmail.com)
//
// #include "config.h"
// #undef PACKAGE  // Name of package
// #undef VERSION  // Version number of package
// #undef X86_64   // Define for the x86_64 CPU famyly
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include "fann.h"

#ifdef _MSC_VER
	#define vsnprintf _vsnprintf
	#define snprintf _snprintf
#endif
#if defined(_WIN32) && !defined(__MINGW32__)
	#define PATH_MAX _MAX_PATH
#endif
#ifndef PATH_MAX
	#ifdef _POSIX_PATH_MAX
		#define PATH_MAX _POSIX_PATH_MAX
	#else
		#define PATH_MAX 4096
	#endif
#endif

//FANN_EXTERNAL FILE * FANN_API fann_default_error_log = (FILE*)-1;

// #define FANN_NO_SEED
// #define DEBUGTRAIN

#define fann_scanf(type, name, val) { \
		if(fscanf(conf, name "="type "\n", val) != 1) { \
			fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, name, configuration_file); \
			fann_destroy(ann); \
			return NULL; \
		}}

#define fann_skip(name)	{ \
		if(fscanf(conf, name) != 0) { \
			fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, name, configuration_file); \
			fann_destroy(ann); \
			return NULL; \
		}}

//static
const char * Fann::GetAttrText(uint attr, uint value)
{
	switch(attr) {
		case attrNetType:
			switch(value) {
				case Fann::FANN_NETTYPE_LAYER: return "FANN_NETTYPE_LAYER";
				case Fann::FANN_NETTYPE_SHORTCUT: return "FANN_NETTYPE_SHORTCUT";
			}
			break;
		case attrTrainAlgorithm:
			switch(value) {
				case Fann::FANN_TRAIN_INCREMENTAL: return "FANN_TRAIN_INCREMENTAL";
				case Fann::FANN_TRAIN_BATCH: return "FANN_TRAIN_BATCH";
				case Fann::FANN_TRAIN_RPROP: return "FANN_TRAIN_RPROP";
				case Fann::FANN_TRAIN_QUICKPROP: return "FANN_TRAIN_QUICKPROP";
				case Fann::FANN_TRAIN_SARPROP: return "FANN_TRAIN_SARPROP";
			}
			break;
		case attrActivationFunc:
			switch(value) {
				case Fann::FANN_LINEAR: return "FANN_LINEAR";
				case Fann::FANN_THRESHOLD: return "FANN_THRESHOLD";
				case Fann::FANN_THRESHOLD_SYMMETRIC: return "FANN_THRESHOLD_SYMMETRIC";
				case Fann::FANN_SIGMOID: return "FANN_SIGMOID";
				case Fann::FANN_SIGMOID_STEPWISE: return "FANN_SIGMOID_STEPWISE";
				case Fann::FANN_SIGMOID_SYMMETRIC: return "FANN_SIGMOID_SYMMETRIC";
				case Fann::FANN_SIGMOID_SYMMETRIC_STEPWISE: return "FANN_SIGMOID_SYMMETRIC_STEPWISE";
				case Fann::FANN_GAUSSIAN: return "FANN_GAUSSIAN";
				case Fann::FANN_GAUSSIAN_SYMMETRIC: return "FANN_GAUSSIAN_SYMMETRIC";
				case Fann::FANN_GAUSSIAN_STEPWISE: return "FANN_GAUSSIAN_STEPWISE";
				case Fann::FANN_ELLIOT: return "FANN_ELLIOT";
				case Fann::FANN_ELLIOT_SYMMETRIC: return "FANN_ELLIOT_SYMMETRIC";
				case Fann::FANN_LINEAR_PIECE: return "FANN_LINEAR_PIECE";
				case Fann::FANN_LINEAR_PIECE_SYMMETRIC: return "FANN_LINEAR_PIECE_SYMMETRIC";
				case Fann::FANN_SIN_SYMMETRIC: return "FANN_SIN_SYMMETRIC";
				case Fann::FANN_COS_SYMMETRIC: return "FANN_COS_SYMMETRIC";
				case Fann::FANN_SIN: return "FANN_SIN";
				case Fann::FANN_COS: return "FANN_COS";
			}
			break;
		case attrErrorFunc:
			switch(value) {
				case Fann::FANN_ERRORFUNC_LINEAR: return "FANN_ERRORFUNC_LINEAR";
				case Fann::FANN_ERRORFUNC_TANH: return "FANN_ERRORFUNC_TANH";
			}
			break;
		case attrStopFunc:
			switch(value) {
				case Fann::FANN_STOPFUNC_MSE: return "FANN_STOPFUNC_MSE";
				case Fann::FANN_STOPFUNC_BIT: return "FANN_STOPFUNC_BIT";
			}
		default:
			return "unknown-attr";
	}
	return "unknown";
}

Fann::TrainData::TrainData()
{
	State = 0;
	Count = 0;
	NumInput = 0;
	NumOutput = 0;
}

int Fann::TrainData::Helper_Construct(uint numInput, uint numOutput, uint numSeries)
{
	int    ok = 1;
	State = 0;
	Count = numSeries;
	NumInput = numInput;
	NumOutput = numOutput;
	for(uint i = 0; i < Count; i++) {
		Fann::DataVector * p_new_inp = new Fann::DataVector(numInput);
		THROW_S(p_new_inp, SLERR_NOMEM);
		THROW(InpL.insert(p_new_inp));
		Fann::DataVector * p_new_out = new Fann::DataVector(numOutput);
		THROW_S(p_new_out, SLERR_NOMEM);
		THROW(OutL.insert(p_new_out));
	}
	CATCH
		Destroy();
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
}

Fann::TrainData::TrainData(const Fann & rAnn, uint numSeries)
{
	Helper_Construct(rAnn.NumInput, rAnn.NumOutput, numSeries);
}
Fann::TrainData::TrainData(uint numInput, uint numOutput, uint numSeries)
{
	Helper_Construct(numInput, numOutput, numSeries);
}

Fann::TrainData::~TrainData()
{
}

void Fann::TrainData::Destroy()
{
	State = 0;
	InpL.freeAll();
	OutL.freeAll();
	Count = 0;
	NumInput = 0;
	NumOutput = 0;
}

int Fann::TrainData::Read(const char * pFileName, long format)
{
	Destroy();

    int    ok = 1;
	long   actual_count = 0;
	long   _count = 0;
	long   _num_inp = 0;
	long   _num_out = 0;
    SString line_buf, temp_buf;
    SFile f_in(pFileName, SFile::mRead);
    THROW(f_in.IsValid());
	if(f_in.ReadLine(line_buf)) {
		StringSet ss(" ");
		ss.setBuf(line_buf.Chomp().Strip());
		uint   ssp = 0;
		int    input_output = 0; // Триггер, принимающий значение 0 при чтении входных данных и !0 - выходных
		RealArray value_list;
		THROW(ss.get(&ssp, temp_buf));
        _count = temp_buf.ToLong();
        THROW(ss.get(&ssp, temp_buf));
        _num_inp = temp_buf.ToLong();
        THROW(ss.get(&ssp, temp_buf));
        _num_out = temp_buf.ToLong();
        THROW(_count > 0 && _num_inp > 0 && _num_out > 0);
        while(actual_count < _count && f_in.ReadLine(line_buf)) {
            value_list.clear();
            ss.setBuf(line_buf.Chomp().Strip());
			for(ssp = 0; ss.get(&ssp, temp_buf);) {
				double value = temp_buf.ToReal();
                value_list.insert(&value);
			}
            if(input_output == 0) {
				THROW((long)value_list.getCount() == _num_inp);
				{
					Fann::DataVector * p_vect = new Fann::DataVector(value_list);
					THROW_S(p_vect, SLERR_NOMEM);
					THROW(p_vect->getCount() == value_list.getCount());
					THROW(InpL.insert(p_vect));
				}
				input_output = 1;
            }
            else {
				THROW((long)value_list.getCount() == _num_out);
				{
					Fann::DataVector * p_vect = new Fann::DataVector(value_list);
					THROW_S(p_vect, SLERR_NOMEM);
					THROW(p_vect->getCount() == value_list.getCount());
					THROW(OutL.insert(p_vect));
				}
				input_output = 0;
				actual_count++;
            }
        }
        THROW(InpL.getCount() == OutL.getCount());
		Count = actual_count;
		NumInput = (uint)_num_inp;
		NumOutput = (uint)_num_out;
	}
	CATCH
		Destroy();
		State |= stError;
		ok = 0;
	ENDCATCH
    return ok;
}

int Fann::TrainData::SetInputSeries(uint seriesN, const float * pData)
{
	int    ok = 1;
	if(pData && seriesN < InpL.getCount() && IsValid()) {
		Fann::DataVector * p_vect = InpL.at(seriesN);
		for(uint i = 0; i < p_vect->getCount(); i++)
			(*p_vect)[i] = pData[i];
	}
	else
		ok = 0;
	return ok;
}

int Fann::TrainData::SetOutputSeries(uint seriesN, const float * pData)
{
	int    ok = 1;
	if(pData && seriesN < OutL.getCount() && IsValid()) {
		Fann::DataVector * p_vect = OutL.at(seriesN);
		for(uint i = 0; i < p_vect->getCount(); i++)
			(*p_vect)[i] = pData[i];
	}
	else
		ok = 0;
	return ok;
}

int Fann::TrainData::IsValid() const
{
	int    ok = 1;
	THROW(!(State & stError));
	THROW(GetCount() == InpL.getCount());
	THROW(GetCount() == OutL.getCount());
	{
		for(uint i = 0; i < InpL.getCount(); i++) {
			THROW(InpL.at(i));
			THROW(InpL.at(i)->getCount() == GetInputCount());
		}
	}
	{
		for(uint i = 0; i < OutL.getCount(); i++) {
			THROW(OutL.at(i));
			THROW(OutL.at(i)->getCount() == GetOutputCount());
		}
	}
	CATCHZOK
	return ok;
}

uint Fann::TrainData::GetCount() const
{
	return Count;
}

uint Fann::TrainData::GetInputCount() const
{
	return NumInput;
}

uint Fann::TrainData::GetOutputCount() const
{
	return NumOutput;
}

FANN_EXTERNAL Fann * FANN_API fann_create_standard(uint numLayers, ...)
{
	Fann * ann = 0;
	LongArray layers;
	{
		va_list layer_sizes;
		va_start(layer_sizes, numLayers);
		for(uint i = 0; i < numLayers; i++) {
			const uint arg = va_arg(layer_sizes, uint);
			THROW_S(arg >= 0 && arg <= 1000000, SLERR_FANN_CANT_ALLOCATE_MEM);
			THROW(layers.add(arg));
		}
		va_end(layer_sizes);
	}
	THROW(ann = fann_create_standard_array(layers));
	CATCH
		ZDELETE(ann);
	ENDCATCH
	return ann;
}

FANN_EXTERNAL Fann * FANN_API fann_create_standard_array(/*uint numLayers, const uint * pLayers*/const LongArray & rLayers)
{
	return fann_create_sparse_array(1, rLayers/*numLayers, pLayers*/);
}

FANN_EXTERNAL Fann * FANN_API fann_create_sparse(float connection_rate, uint numLayers, ...)
{
	Fann * ann = 0;
	LongArray layers;
	{
		va_list layer_sizes;
		va_start(layer_sizes, numLayers);
		for(uint i = 0; i < numLayers; i++) {
			const uint arg = va_arg(layer_sizes, uint);
			THROW(arg >= 0 && arg <= 1000000);
			THROW(layers.add(arg));
		}
		va_end(layer_sizes);
	}
	THROW(ann = fann_create_sparse_array(connection_rate, layers));
	CATCH
		ZDELETE(ann);
	ENDCATCH
	return ann;
}

FANN_EXTERNAL Fann * FANN_API fann_create_sparse_array(float connectionRate, const LongArray & rLayers/*uint numLayers, const uint * pLayers*/)
{
	Fann * p_result = new Fann(Fann::FANN_NETTYPE_LAYER, connectionRate, rLayers);
	if(p_result && !p_result->IsValid())
		ZDELETE(p_result);
	return p_result;
}

FANN_EXTERNAL Fann * FANN_API fann_create_shortcut(uint num_layers, ...)
{
	Fann * ann = 0;
	LongArray layers;
	{
		va_list layer_sizes;
		va_start(layer_sizes, num_layers);
		//status = 1;
		for(uint i = 0; i < num_layers; i++) {
			uint arg = va_arg(layer_sizes, uint);
			THROW_S(arg >= 0 && arg <= 1000000, SLERR_FANN_CANT_ALLOCATE_MEM);
			//p_layers[i] = arg;
			THROW(layers.add(arg));
		}
		va_end(layer_sizes);
	}
	THROW(ann = fann_create_shortcut_array(layers/*num_layers, p_layers*/));
	CATCH
		ZDELETE(ann);
	ENDCATCH
	return ann;
}

FANN_EXTERNAL Fann * FANN_API fann_create_shortcut_array(const LongArray & rLayers)
{
	Fann * p_result = new Fann(Fann::FANN_NETTYPE_SHORTCUT, 1.0f, rLayers);
	if(p_result && !p_result->IsValid())
		ZDELETE(p_result);
	return p_result;
}

#define fann_activation_switch(activation_function, value, result) \
switch(activation_function) { \
	case FANN_LINEAR: result = (ANNTYP)value; break; \
	case FANN_LINEAR_PIECE: result = (ANNTYP)((value < 0) ? 0 : (value > 1) ? 1 : value); break; \
	case FANN_LINEAR_PIECE_SYMMETRIC: result = (ANNTYP)((value < -1) ? -1 : (value > 1) ? 1 : value); break; \
	case FANN_SIGMOID: result = (ANNTYP)fann_sigmoid_real(value); break; \
	case FANN_SIGMOID_SYMMETRIC: result = (ANNTYP)fann_sigmoid_symmetric_real(value); break; \
	case FANN_SIGMOID_SYMMETRIC_STEPWISE: \
		result = (ANNTYP)fann_stepwise(-2.64665293693542480469e+00, -1.47221934795379638672e+00, -5.49306154251098632812e-01, 5.49306154251098632812e-01, 1.47221934795379638672e+00, 2.64665293693542480469e+00, -9.90000009536743164062e-01, -8.99999976158142089844e-01, -5.00000000000000000000e-01, 5.00000000000000000000e-01, 8.99999976158142089844e-01, 9.90000009536743164062e-01, -1, 1, value); \
        break; \
	case FANN_SIGMOID_STEPWISE: \
		result = (ANNTYP)fann_stepwise(-2.64665246009826660156e+00, -1.47221946716308593750e+00, -5.49306154251098632812e-01, 5.49306154251098632812e-01, 1.47221934795379638672e+00, 2.64665293693542480469e+00, 4.99999988824129104614e-03, 5.00000007450580596924e-02, 2.50000000000000000000e-01, 7.50000000000000000000e-01, 9.49999988079071044922e-01, 9.95000004768371582031e-01, 0, 1, value); \
        break; \
	case FANN_THRESHOLD: result = (ANNTYP)((value < 0) ? 0 : 1); break; \
	case FANN_THRESHOLD_SYMMETRIC: result = (ANNTYP)((value < 0) ? -1 : 1); break; \
	case FANN_GAUSSIAN: result = (ANNTYP)fann_gaussian_real(value); break; \
	case FANN_GAUSSIAN_SYMMETRIC: result = (ANNTYP)fann_gaussian_symmetric_real(value); break; \
	case FANN_ELLIOT: result = (ANNTYP)fann_elliot_real(value); break; \
	case FANN_ELLIOT_SYMMETRIC: result = (ANNTYP)fann_elliot_symmetric_real(value); break; \
	case FANN_SIN_SYMMETRIC: result = (ANNTYP)fann_sin_symmetric_real(value); break; \
	case FANN_COS_SYMMETRIC: result = (ANNTYP)fann_cos_symmetric_real(value); break; \
	case FANN_SIN: result = (ANNTYP)fann_sin_real(value); break; \
	case FANN_COS: result = (ANNTYP)fann_cos_real(value); break; \
	case FANN_GAUSSIAN_STEPWISE: result = 0; break; \
}

ANNTYP FannActivationSwitch(int activationFunction, ANNTYP value)
{
	switch(activationFunction) {
		case Fann::FANN_LINEAR: return (ANNTYP)value;
		case Fann::FANN_LINEAR_PIECE: return (ANNTYP)((value < 0) ? 0 : (value > 1) ? 1 : value);
		case Fann::FANN_LINEAR_PIECE_SYMMETRIC: return (ANNTYP)((value < -1) ? -1 : (value > 1) ? 1 : value);
		case Fann::FANN_SIGMOID: return (ANNTYP)fann_sigmoid_real(value);
		case Fann::FANN_SIGMOID_SYMMETRIC: return (ANNTYP)fann_sigmoid_symmetric_real(value);
		case Fann::FANN_SIGMOID_SYMMETRIC_STEPWISE:
			return (ANNTYP)fann_stepwise(-2.64665293693542480469e+00, -1.47221934795379638672e+00, -5.49306154251098632812e-01, 5.49306154251098632812e-01, 1.47221934795379638672e+00, 2.64665293693542480469e+00, -9.90000009536743164062e-01, -8.99999976158142089844e-01, -5.00000000000000000000e-01, 5.00000000000000000000e-01, 8.99999976158142089844e-01, 9.90000009536743164062e-01, -1, 1, value);
		case Fann::FANN_SIGMOID_STEPWISE:
			return (ANNTYP)fann_stepwise(-2.64665246009826660156e+00, -1.47221946716308593750e+00, -5.49306154251098632812e-01, 5.49306154251098632812e-01, 1.47221934795379638672e+00, 2.64665293693542480469e+00, 4.99999988824129104614e-03, 5.00000007450580596924e-02, 2.50000000000000000000e-01, 7.50000000000000000000e-01, 9.49999988079071044922e-01, 9.95000004768371582031e-01, 0, 1, value);
		case Fann::FANN_THRESHOLD: return (ANNTYP)((value < 0) ? 0 : 1);
		case Fann::FANN_THRESHOLD_SYMMETRIC: return (ANNTYP)((value < 0) ? -1 : 1);
		case Fann::FANN_GAUSSIAN: return (ANNTYP)fann_gaussian_real(value);
		case Fann::FANN_GAUSSIAN_SYMMETRIC: return (ANNTYP)fann_gaussian_symmetric_real(value);
		case Fann::FANN_ELLIOT: return (ANNTYP)fann_elliot_real(value);
		case Fann::FANN_ELLIOT_SYMMETRIC: return (ANNTYP)fann_elliot_symmetric_real(value);
		case Fann::FANN_SIN_SYMMETRIC: return (ANNTYP)fann_sin_symmetric_real(value);
		case Fann::FANN_COS_SYMMETRIC: return (ANNTYP)fann_cos_symmetric_real(value);
		case Fann::FANN_SIN: return (ANNTYP)fann_sin_real(value);
		case Fann::FANN_COS: return (ANNTYP)fann_cos_real(value);
		case Fann::FANN_GAUSSIAN_STEPWISE: return 0;
	}
	return 0;
}

ANNTYP * Fann::Run(const ANNTYP * pInput)
{
	Fann::Neuron * p_neuron_it;
	Fann::Neuron * p_last_neuron;
	Fann::Neuron * p_neurons;
	Fann::Neuron ** pp_neuron_pointers;
	uint i;
	uint _num_connections;
	ANNTYP neuron_sum;
	ANNTYP * p_weights;
	Fann::Layer * p_layer_it;
	const Fann::Layer * p_last_layer;
	uint activation_function;
	ANNTYP steepness;
	// store some variabels local for fast access
	Fann::Neuron * p_first_neuron = P_FirstLayer->P_FirstNeuron;
#ifdef FIXEDFANN
	const int  multiplier = Multiplier;
	const uint decimal_point = DecimalPoint;
	// values used for the stepwise linear sigmoid function
	ANNTYP r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;
	ANNTYP v1 = 0, v2 = 0, v3 = 0, v4 = 0, v5 = 0, v6 = 0;
	ANNTYP last_steepness = 0;
	uint   last_activation_function = 0;
#else
	ANNTYP max_sum = 0;
#endif
	{
		//
		// first set the input
		//
		const uint ci = NumInput;
		for(i = 0; i != ci; i++) {
#ifdef FIXEDFANN
			if(fann_abs(pInput[i]) > multiplier) {
				printf("Warning input number %d is out of range -%d - %d with value %d, integer overflow may occur.\n", i, multiplier, multiplier, pInput[i]);
			}
#endif
			p_first_neuron[i].Value = pInput[i];
		}
	}
	//
	// Set the bias neuron in the input layer
	//
#ifdef FIXEDFANN
	(P_FirstLayer->P_LastNeuron-1)->Value = multiplier;
#else
	(P_FirstLayer->P_LastNeuron-1)->Value = 1;
#endif
	p_last_layer = P_LastLayer;
	for(p_layer_it = P_FirstLayer + 1; p_layer_it != p_last_layer; p_layer_it++) {
		p_last_neuron = p_layer_it->P_LastNeuron;
		for(p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
			if(p_neuron_it->FirstCon == p_neuron_it->LastCon) {
				// bias p_neurons
#ifdef FIXEDFANN
				p_neuron_it->Value = multiplier;
#else
				p_neuron_it->Value = 1;
#endif
				continue;
			}
			activation_function = p_neuron_it->ActivationFunction;
			steepness = p_neuron_it->ActivationSteepness;
			neuron_sum = 0;
			_num_connections = p_neuron_it->GetConCount();
			p_weights = P_Weights + p_neuron_it->FirstCon;
			if(ConnectionRate >= 1) {
				p_neurons = (NetworkType == FANN_NETTYPE_SHORTCUT) ? P_FirstLayer->P_FirstNeuron : (p_layer_it - 1)->P_FirstNeuron;
				{ // unrolled loop start
					i = _num_connections & 3; // same as modulo 4
					switch(i) {
						case 3: neuron_sum += fann_mult(p_weights[2], p_neurons[2].Value);
						case 2: neuron_sum += fann_mult(p_weights[1], p_neurons[1].Value);
						case 1: neuron_sum += fann_mult(p_weights[0], p_neurons[0].Value);
						case 0: break;
					}
					for(; i != _num_connections; i += 4) {
						neuron_sum += fann_mult(p_weights[i], p_neurons[i].Value) +
							fann_mult(p_weights[i+1], p_neurons[i+1].Value) +
							fann_mult(p_weights[i+2], p_neurons[i+2].Value) +
							fann_mult(p_weights[i+3], p_neurons[i+3].Value);
					}
				} // unrolled loop end
				/*for(i = 0;i != num_connections; i++){
					printf("%f += %f*%f, ", neuron_sum, weights[i], p_neurons[i].value);
					neuron_sum += fann_mult(weights[i], p_neurons[i].value);
				}*/
			}
			else {
				pp_neuron_pointers = PP_Connections + p_neuron_it->FirstCon;
				i = _num_connections & 3; // same as modulo 4
				switch(i) {
					case 3: neuron_sum += fann_mult(p_weights[2], pp_neuron_pointers[2]->Value);
					case 2: neuron_sum += fann_mult(p_weights[1], pp_neuron_pointers[1]->Value);
					case 1: neuron_sum += fann_mult(p_weights[0], pp_neuron_pointers[0]->Value);
					case 0: break;
				}
				for(; i != _num_connections; i += 4) {
					neuron_sum +=
					    fann_mult(p_weights[i], pp_neuron_pointers[i]->Value) +
					    fann_mult(p_weights[i + 1], pp_neuron_pointers[i+1]->Value) +
					    fann_mult(p_weights[i + 2], pp_neuron_pointers[i+2]->Value) +
					    fann_mult(p_weights[i + 3], pp_neuron_pointers[i+3]->Value);
				}
			}
#ifdef FIXEDFANN
			p_neuron_it->Sum = fann_mult(steepness, neuron_sum);
			if(activation_function != last_activation_function || steepness != last_steepness) {
				switch(activation_function) {
					case FANN_SIGMOID:
					case FANN_SIGMOID_STEPWISE:
					    r1 = SigmoidResults[0];
					    r2 = SigmoidResults[1];
					    r3 = SigmoidResults[2];
					    r4 = SigmoidResults[3];
					    r5 = SigmoidResults[4];
					    r6 = SigmoidResults[5];
					    v1 = SigmoidValues[0] / steepness;
					    v2 = SigmoidValues[1] / steepness;
					    v3 = SigmoidValues[2] / steepness;
					    v4 = SigmoidValues[3] / steepness;
					    v5 = SigmoidValues[4] / steepness;
					    v6 = SigmoidValues[5] / steepness;
					    break;
					case FANN_SIGMOID_SYMMETRIC:
					case FANN_SIGMOID_SYMMETRIC_STEPWISE:
					    r1 = SigmoidSymmetricResults[0];
					    r2 = SigmoidSymmetricResults[1];
					    r3 = SigmoidSymmetricResults[2];
					    r4 = SigmoidSymmetricResults[3];
					    r5 = SigmoidSymmetricResults[4];
					    r6 = SigmoidSymmetricResults[5];
					    v1 = SigmoidSymmetricValues[0] / steepness;
					    v2 = SigmoidSymmetricValues[1] / steepness;
					    v3 = SigmoidSymmetricValues[2] / steepness;
					    v4 = SigmoidSymmetricValues[3] / steepness;
					    v5 = SigmoidSymmetricValues[4] / steepness;
					    v6 = SigmoidSymmetricValues[5] / steepness;
					    break;
					case FANN_THRESHOLD:
					    break;
				}
			}
			switch(activation_function) {
				case FANN_SIGMOID:
				case FANN_SIGMOID_STEPWISE:
				    p_neuron_it->Value = (ANNTYP)fann_stepwise(v1, v2, v3, v4, v5, v6, r1, r2, r3, r4, r5, r6, 0, multiplier, neuron_sum);
				    break;
				case FANN_SIGMOID_SYMMETRIC:
				case FANN_SIGMOID_SYMMETRIC_STEPWISE:
				    p_neuron_it->Value = (ANNTYP)fann_stepwise(v1, v2, v3, v4, v5, v6, r1, r2, r3, r4, r5, r6, -multiplier, multiplier, neuron_sum);
				    break;
				case FANN_THRESHOLD:
				    p_neuron_it->Value = (ANNTYP)((neuron_sum < 0) ? 0 : multiplier);
				    break;
				case FANN_THRESHOLD_SYMMETRIC:
				    p_neuron_it->Value = (ANNTYP)((neuron_sum < 0) ? -multiplier : multiplier);
				    break;
				case FANN_LINEAR:
				    p_neuron_it->Value = neuron_sum;
				    break;
				case FANN_LINEAR_PIECE:
				    p_neuron_it->Value = (ANNTYP)((neuron_sum < 0) ? 0 : (neuron_sum > multiplier) ? multiplier : neuron_sum);
				    break;
				case FANN_LINEAR_PIECE_SYMMETRIC:
				    p_neuron_it->Value = (ANNTYP)((neuron_sum < -multiplier) ? -multiplier : (neuron_sum > multiplier) ? multiplier : neuron_sum);
				    break;
				case FANN_ELLIOT:
				case FANN_ELLIOT_SYMMETRIC:
				case FANN_GAUSSIAN:
				case FANN_GAUSSIAN_SYMMETRIC:
				case FANN_GAUSSIAN_STEPWISE:
				case FANN_SIN_SYMMETRIC:
				case FANN_COS_SYMMETRIC:
				    fann_error(&Err, SLERR_FANN_CANT_USE_ACTIVATION);
				    break;
			}
			last_steepness = steepness;
			last_activation_function = activation_function;
#else
			neuron_sum = fann_mult(steepness, neuron_sum);
			max_sum = 150/steepness;
			if(neuron_sum > max_sum)
				neuron_sum = max_sum;
			else if(neuron_sum < -max_sum)
				neuron_sum = -max_sum;
			p_neuron_it->Sum = neuron_sum;
			fann_activation_switch(activation_function, neuron_sum, p_neuron_it->Value);
#endif
		}
	}
	// set the output
	{
		ANNTYP * p_output = P_Output;
		p_neurons = (P_LastLayer-1)->P_FirstNeuron;
		const uint co = NumOutput;
		for(i = 0; i != co; i++) {
			p_output[i] = p_neurons[i].Value;
		}
	}
	return P_Output;
}

void Fann::RandomizeWeights(ANNTYP minWeight, ANNTYP maxWeight)
{
	const ANNTYP * p_last_weight = P_Weights + TotalConnections;
	for(ANNTYP * p_weights = P_Weights; p_weights != p_last_weight; p_weights++) {
		*p_weights = (ANNTYP)(fann_rand(minWeight, maxWeight));
	}
#ifndef FIXEDFANN
	if(P_PrevTrainSlopes)
		ClearTrainArrays();
#endif
}

int Fann::IsEqualScaleVect(uint count, const float * pVect, const float * pOtherVect) const
{
	int    yes = 1;
	THROW(BIN(pVect) == BIN(pOtherVect));
	if(pVect) {
		for(uint i = 0; i < count; i++) {
			THROW(pVect[i] == pOtherVect[i]);
		}
	}
	CATCH
		yes = 0;
	ENDCATCH
	return yes;
}

int Fann::IsEqual(const Fann & rS, long flags) const
{
	int    yes = 1;
#define CMPF(f) THROW(f == rS.f)
#define CMPA(a) { for(uint i = 0; i < SIZEOFARRAY(a); i++) { THROW(a[i] == rS.a[i]); } }
	THROW(Err.IsEqual(rS.Err));
	{
		LongArray layers1, layers2;
		GetLayerArray(layers1);
		rS.GetLayerArray(layers2);
		THROW(layers1 == layers2);
	}
	CMPF(Layers);
	CMPF(LearningRate);
	CMPF(LearningMomentum);
	CMPF(ConnectionRate);
	CMPF(NetworkType);
	CMPF(TotalNeurons);
	CMPF(NumInput);
	CMPF(NumOutput);
	CMPF(TrainingAlgorithm);
//#ifdef FIXEDFANN
	CMPF(DecimalPoint);
	CMPF(Multiplier);
	THROW(memcmp(SigmoidResults, rS.SigmoidResults, sizeof(SigmoidResults)) == 0);
	THROW(memcmp(SigmoidValues, rS.SigmoidValues, sizeof(SigmoidValues)) == 0);
	THROW(memcmp(SigmoidSymmetricResults, rS.SigmoidSymmetricResults, sizeof(SigmoidSymmetricResults)) == 0);
	THROW(memcmp(SigmoidSymmetricValues, rS.SigmoidSymmetricValues, sizeof(SigmoidSymmetricValues)) == 0);
//#else
	THROW(ScaleIn.IsEqual(NumInput, rS.ScaleIn));
	THROW(ScaleOut.IsEqual(NumOutput, rS.ScaleOut));
//#endif
	CMPF(TotalConnections);
	CMPF(num_MSE);
	CMPF(MSE_value);
	CMPF(NumBitFail);
	CMPF(BitFailLimit);
	CMPF(TrainErrorFunction);
	CMPF(TrainStopFunction);

	CMPF(CascadeOutputChangeFraction);
	CMPF(CascadeOutputStagnationEpochs);
	CMPF(CascadeCandidateChangeFraction);
	CMPF(CascadeCandidateStagnationEpochs);
	CMPF(CascadeBestCandidate);
	CMPF(CascadeCandidateLimit);
	CMPF(CascadeWeightMultiplier);
	CMPF(CascadeMaxOutEpochs);
	CMPF(CascadeMaxCandEpochs);
	CMPF(CascadeMinOutEpochs);
	CMPF(CascadeMinCandEpochs);

	CMPF(CascadeNumCandidateGroups);
	CMPF(TotalNeuronsAllocated);
	CMPF(TotalConnectionsAllocated);
	CMPF(QuickpropDecay);
	CMPF(QuickpropMu);
	CMPF(RpropIncreaseFactor);
	CMPF(RpropDecreaseFactor);
	CMPF(RpropDeltaMin);
	CMPF(RpropDeltaMax);
	CMPF(RpropDeltaZero);
	CMPF(SarpropWeightDecayShift);
	CMPF(SarpropStepErrorThresholdFactor);
	CMPF(SarpropStepErrorShift);
	CMPF(SarpropTemperature);
	CMPF(SarpropEpoch);
	CMPF(CascadeActivationFuncList);
	CMPF(CascadeActivationSteepnessesList);
	//
	//
	//
	{
		{
			// compare the neurons
			const Neuron * p_last_neuron = (P_LastLayer-1)->P_LastNeuron;
			const Neuron * p_neur = P_FirstLayer->P_FirstNeuron;
			const Neuron * p_s_neur = rS.P_FirstLayer->P_FirstNeuron;
			for(;p_neur != p_last_neuron; p_neur++, p_s_neur++) {
				THROW(p_neur->IsEqual(*p_s_neur));
			}
		}
		{
			// compare the connections
			const Neuron * p_first_neur = P_FirstLayer->P_FirstNeuron;
			const Neuron * p_s_first_neur = rS.P_FirstLayer->P_FirstNeuron;
			for(uint i = 0; i < TotalConnections; i++) {
				const uint _input_neuron = (uint)(PP_Connections[i] - p_first_neur);
				const uint _s_input_neuron = (uint)(rS.PP_Connections[i] - p_s_first_neur);
				THROW(_input_neuron == _s_input_neuron);
				THROW(P_Weights[i] == rS.P_Weights[i]);
			}
		}
		{
			THROW((P_TrainSlopes && rS.P_TrainSlopes) || (!P_TrainSlopes && !rS.P_TrainSlopes));
			THROW((P_PrevSteps && rS.P_PrevSteps) || (!P_PrevSteps && !rS.P_PrevSteps));
			THROW((P_PrevTrainSlopes && rS.P_PrevTrainSlopes) || (!P_PrevTrainSlopes && !rS.P_PrevTrainSlopes));
			THROW((P_PrevWeightsDeltas && rS.P_PrevWeightsDeltas) || (!P_PrevWeightsDeltas && !rS.P_PrevWeightsDeltas));
			for(uint i = 0; i < TotalConnectionsAllocated; i++) {
				THROW(!P_TrainSlopes       || P_TrainSlopes[i] == rS.P_TrainSlopes[i]);
				THROW(!P_PrevSteps         || P_PrevSteps[i] == rS.P_PrevSteps[i]);
				THROW(!P_PrevTrainSlopes   || P_PrevTrainSlopes[i] == rS.P_PrevTrainSlopes[i]);
				THROW(!P_PrevWeightsDeltas || P_PrevWeightsDeltas[i] == rS.P_PrevWeightsDeltas[i]);
			}
		}
	}
	CATCH
		yes = 0;
	ENDCATCH
#undef CMPF
	return yes;
}

int Fann::Copy(const Fann & rS)
{
#define COPYF(f) f = rS.f
	int    ok = 1;
	uint   i;
	uint   _input_neuron;
	Fann::Neuron * p_last_neuron = 0;
	Fann::Neuron * p_orig_neuron_it = 0;
	Fann::Neuron * p_copy_neuron_it = 0;
	Fann::Neuron * p_orig_first_neuron = 0;
	Fann::Neuron * p_copy_first_neuron = 0;
	Destroy();
	State = 0;
	COPYF(Err);
	COPYF(Layers);
	COPYF(NumInput);
	COPYF(NumOutput);
	COPYF(LearningRate);
	COPYF(LearningMomentum);
	COPYF(ConnectionRate);
	COPYF(NetworkType);
	COPYF(num_MSE);
	COPYF(MSE_value);
	COPYF(NumBitFail);
	COPYF(BitFailLimit);
	COPYF(TrainErrorFunction);
	COPYF(TrainStopFunction);
	COPYF(TrainingAlgorithm);
	COPYF(Callback);
	COPYF(P_UserData); // user_data is not deep copied.  user should use fann_copy_with_user_data() for that
//#ifdef FIXEDFANN
	COPYF(DecimalPoint);
	COPYF(Multiplier);
	memcpy(SigmoidResults,          rS.SigmoidResults, sizeof(SigmoidResults));
	memcpy(SigmoidValues,           rS.SigmoidValues,  sizeof(SigmoidValues));
	memcpy(SigmoidSymmetricResults, rS.SigmoidSymmetricResults, sizeof(SigmoidSymmetricResults));
	memcpy(SigmoidSymmetricValues,  rS.SigmoidSymmetricValues,  sizeof(SigmoidSymmetricValues));
//#else
	// copy scale parameters, when used
	ScaleIn.Copy(NumInput, rS.ScaleIn);
	ScaleOut.Copy(NumOutput, rS.ScaleOut);
	COPYF(CascadeOutputChangeFraction);
	COPYF(CascadeOutputStagnationEpochs);
	COPYF(CascadeCandidateChangeFraction);
	COPYF(CascadeCandidateStagnationEpochs);
	COPYF(CascadeBestCandidate);
	COPYF(CascadeCandidateLimit);
	COPYF(CascadeWeightMultiplier);
	COPYF(CascadeMaxOutEpochs);
	COPYF(CascadeMaxCandEpochs);
	COPYF(CascadeMinOutEpochs);
	COPYF(CascadeMinCandEpochs);
	COPYF(CascadeActivationFuncList);
	COPYF(CascadeActivationSteepnessesList);
	COPYF(CascadeNumCandidateGroups);
	// copy candidate scores, if used
	if(rS.P_CascadeCandidateScores == NULL) {
		P_CascadeCandidateScores = NULL;
	}
	else {
		P_CascadeCandidateScores = (ANNTYP*)SAlloc::M(GetCascadeNumCandidates() * sizeof(ANNTYP));
		THROW_S(P_CascadeCandidateScores, SLERR_FANN_CANT_ALLOCATE_MEM);
		memcpy(P_CascadeCandidateScores, rS.P_CascadeCandidateScores, GetCascadeNumCandidates() * sizeof(ANNTYP));
	}
//#endif // } FIXEDFANN
	COPYF(QuickpropDecay);
	COPYF(QuickpropMu);
	COPYF(RpropIncreaseFactor);
	COPYF(RpropDecreaseFactor);
	COPYF(RpropDeltaMin);
	COPYF(RpropDeltaMax);
	COPYF(RpropDeltaZero);
	COPYF(SarpropWeightDecayShift);
	COPYF(SarpropStepErrorThresholdFactor);
	COPYF(SarpropStepErrorShift);
	COPYF(SarpropTemperature);
	COPYF(SarpropEpoch);
	{
		Fann::Layer * p_orig_layer_it = 0;
		Fann::Layer * p_copy_layer_it = 0;
		//
		// copy layer sizes, prepare for fann_allocate_neurons
		//
		THROW(AllocateLayers());
		for(p_orig_layer_it = rS.P_FirstLayer, p_copy_layer_it = P_FirstLayer; p_orig_layer_it != rS.P_LastLayer; p_orig_layer_it++, p_copy_layer_it++) {
			const uint _layer_size = p_orig_layer_it->GetCount();
			p_copy_layer_it->P_LastNeuron = p_copy_layer_it->P_FirstNeuron + _layer_size;
			TotalNeurons += _layer_size;
		}
		assert(TotalNeurons == rS.TotalNeurons);
	}
	{
		//
		// copy the neurons
		//
		THROW(AllocateNeurons());
		const uint _layer_size = (rS.P_LastLayer-1)->GetCount();
		memcpy(P_Output, rS.P_Output, _layer_size * sizeof(ANNTYP));
		p_last_neuron = (rS.P_LastLayer-1)->P_LastNeuron;
		for(p_orig_neuron_it = rS.P_FirstLayer->P_FirstNeuron, p_copy_neuron_it = P_FirstLayer->P_FirstNeuron;
			p_orig_neuron_it != p_last_neuron; p_orig_neuron_it++, p_copy_neuron_it++) {
			memcpy(p_copy_neuron_it, p_orig_neuron_it, sizeof(Fann::Neuron));
		}
	}
	//
	// copy the connections
	//
	COPYF(TotalConnections);
	THROW(AllocateConnections());
	p_orig_first_neuron = rS.P_FirstLayer->P_FirstNeuron;
	p_copy_first_neuron = P_FirstLayer->P_FirstNeuron;
	for(i = 0; i < rS.TotalConnections; i++) {
		P_Weights[i] = rS.P_Weights[i];
		_input_neuron = (uint)(rS.PP_Connections[i] - p_orig_first_neuron);
		PP_Connections[i] = p_copy_first_neuron + _input_neuron;
	}
	if(rS.P_TrainSlopes) {
		P_TrainSlopes = (ANNTYP *)SAlloc::M(TotalConnectionsAllocated * sizeof(ANNTYP));
		THROW_S(P_TrainSlopes, SLERR_FANN_CANT_ALLOCATE_MEM);
		memcpy(P_TrainSlopes, rS.P_TrainSlopes, TotalConnectionsAllocated * sizeof(ANNTYP));
	}
	if(rS.P_PrevSteps) {
		P_PrevSteps = (ANNTYP*)SAlloc::M(TotalConnectionsAllocated * sizeof(ANNTYP));
		THROW_S(P_PrevSteps, SLERR_FANN_CANT_ALLOCATE_MEM);
		memcpy(P_PrevSteps, rS.P_PrevSteps, TotalConnectionsAllocated * sizeof(ANNTYP));
	}
	if(rS.P_PrevTrainSlopes) {
		P_PrevTrainSlopes = (ANNTYP *)SAlloc::M(TotalConnectionsAllocated * sizeof(ANNTYP));
		THROW_S(P_PrevTrainSlopes, SLERR_FANN_CANT_ALLOCATE_MEM);
		memcpy(P_PrevTrainSlopes, rS.P_PrevTrainSlopes, TotalConnectionsAllocated * sizeof(ANNTYP));
	}
	if(rS.P_PrevWeightsDeltas) {
		P_PrevWeightsDeltas = (ANNTYP *)SAlloc::M(TotalConnectionsAllocated * sizeof(ANNTYP));
		THROW_S(P_PrevWeightsDeltas, SLERR_FANN_CANT_ALLOCATE_MEM);
		memcpy(P_PrevWeightsDeltas, rS.P_PrevWeightsDeltas, TotalConnectionsAllocated * sizeof(ANNTYP));
	}
	CATCH
		Destroy();
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
#undef COPYF
}
//
// deep copy of the fann structure
//
FANN_EXTERNAL Fann * FANN_API fann_copy(const Fann * pOrig)
{
	Fann * p_copy = 0;
	if(pOrig) {
		p_copy = new Fann(*pOrig);
		if(!p_copy->IsValid()) {
			ZDELETE(p_copy);
		}
	}
	return p_copy;
}

FANN_EXTERNAL void FANN_API fann_print_connections(Fann * ann)
{
	Fann::Layer * layer_it;
	Fann::Neuron * neuron_it;
	uint   i;
	int    value;
	uint   num_neurons = ann->GetTotalNeurons() - ann->GetNumOutput();
	char * neurons = (char*)SAlloc::M(num_neurons + 1);
	if(neurons == NULL) {
		fann_error(NULL, SLERR_FANN_CANT_ALLOCATE_MEM);
		return;
	}
	neurons[num_neurons] = 0;
	printf("Layer / Neuron ");
	for(i = 0; i < num_neurons; i++) {
		printf("%d", i % 10);
	}
	printf("\n");
	for(layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer; layer_it++) {
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
			memset(neurons, (int)'.', num_neurons);
			for(i = neuron_it->FirstCon; i < neuron_it->LastCon; i++) {
				if(ann->P_Weights[i] < 0) {
#ifdef FIXEDFANN
					value = (int)((ann->P_Weights[i] / (double)ann->Multiplier) - 0.5);
#else
					value = (int)((ann->P_Weights[i]) - 0.5);
#endif
					SETMAX(value, -25);
					neurons[ann->PP_Connections[i] - ann->P_FirstLayer->P_FirstNeuron] = (char)('a' - value);
				}
				else {
#ifdef FIXEDFANN
					value = (int)((ann->P_Weights[i] / (double)ann->Multiplier) + 0.5);
#else
					value = (int)((ann->P_Weights[i]) + 0.5);
#endif
					SETMIN(value, 25);
					neurons[ann->PP_Connections[i] - ann->P_FirstLayer->P_FirstNeuron] = (char)('A' + value);
				}
			}
			printf("L %3d / N %4d %s\n", (int)(layer_it - ann->P_FirstLayer), (int)(neuron_it - ann->P_FirstLayer->P_FirstNeuron), neurons);
		}
	}
	SAlloc::F(neurons);
}
//
// Initialize the weights using Widrow + Nguyen's algorithm.
//
//FANN_EXTERNAL void FANN_API fann_init_weights(Fann * ann, const Fann::TrainData * pData)
void Fann::InitWeights(const Fann::TrainData * pData)
{
#ifdef FIXEDFANN
	const uint multiplier = Multiplier;
#endif
	//ANNTYP _smallest_inp = pData->input[0][0];
	//ANNTYP _largest_inp  = pData->input[0][0];
	ANNTYP _smallest_inp;
	ANNTYP _largest_inp;
	Fann::DataVector::GetMinMax(pData->InpL, &_smallest_inp, &_largest_inp);
	const uint _num_hidden_neurons = (uint)(TotalNeurons - (NumInput + NumOutput + GetNumLayers()));
	const float _scale_factor = (float)(pow((double)(0.7f * (double)_num_hidden_neurons), (double)(1.0f / (double)NumInput)) / (double)(_largest_inp - _smallest_inp));
	const Fann::Neuron * p_bias_neuron = P_FirstLayer->P_LastNeuron-1;
	for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != P_LastLayer; p_layer_it++) {
		const Fann::Neuron * p_last_neuron = p_layer_it->P_LastNeuron;
		if(NetworkType == FANN_NETTYPE_LAYER) {
			p_bias_neuron = (p_layer_it - 1)->P_LastNeuron-1;
		}
		for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
			for(uint _num_connect = p_neuron_it->FirstCon; _num_connect < p_neuron_it->LastCon; _num_connect++) {
				if(p_bias_neuron == PP_Connections[_num_connect]) {
#ifdef FIXEDFANN
					P_Weights[_num_connect] = (ANNTYP)fann_rand(-_scale_factor, _scale_factor * multiplier);
#else
					P_Weights[_num_connect] = (ANNTYP)fann_rand(-_scale_factor, _scale_factor);
#endif
				}
				else {
#ifdef FIXEDFANN
					P_Weights[_num_connect] = (ANNTYP)fann_rand(0, _scale_factor * multiplier);
#else
					P_Weights[_num_connect] = (ANNTYP)fann_rand(0, _scale_factor);
#endif
				}
			}
		}
	}
#ifndef FIXEDFANN
	if(P_PrevTrainSlopes)
		ClearTrainArrays();
#endif
}

FANN_EXTERNAL void FANN_API fann_print_parameters(Fann * ann)
{
	Fann::Layer * layer_it;
#ifndef FIXEDFANN
	uint i;
#endif
	printf("Input layer                          :%4d neurons, 1 bias\n", ann->NumInput);
	for(layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer-1; layer_it++) {
		if(ann->NetworkType == Fann::FANN_NETTYPE_SHORTCUT) {
			printf("  Hidden layer                       :%4d neurons, 0 bias\n", (int)layer_it->GetCount());
		}
		else {
			printf("  Hidden layer                       :%4d neurons, 1 bias\n", (int)(layer_it->GetCount() - 1));
		}
	}
	printf("Output layer                         :%4d neurons\n", ann->NumOutput);
	printf("Total neurons and biases             :%4d\n", ann->GetTotalNeurons());
	printf("Total connections                    :%4d\n", ann->TotalConnections);
	printf("Connection rate                      :%8.3f\n", ann->ConnectionRate);
	printf("Network type                         :   %s\n", Fann::GetAttrText(Fann::attrNetType, ann->NetworkType));
#ifdef FIXEDFANN
	printf("Decimal point                        :%4d\n", ann->DecimalPoint);
	printf("Multiplier                           :%4d\n", ann->Multiplier);
#else
	printf("Training algorithm                   :   %s\n", Fann::GetAttrText(Fann::attrTrainAlgorithm, ann->TrainingAlgorithm));
	printf("Training error function              :   %s\n", Fann::GetAttrText(Fann::attrErrorFunc, ann->TrainErrorFunction));
	printf("Training stop function               :   %s\n", Fann::GetAttrText(Fann::attrStopFunc,  ann->TrainStopFunction));
#endif
#ifdef FIXEDFANN
	printf("Bit fail limit                       :%4d\n", ann->BitFailLimit);
#else
	printf("Bit fail limit                       :%8.3f\n", ann->BitFailLimit);
	printf("Learning rate                        :%8.3f\n", ann->LearningRate);
	printf("Learning momentum                    :%8.3f\n", ann->LearningMomentum);
	printf("Quickprop decay                      :%11.6f\n", ann->QuickpropDecay);
	printf("Quickprop mu                         :%8.3f\n", ann->QuickpropMu);
	printf("RPROP increase factor                :%8.3f\n", ann->RpropIncreaseFactor);
	printf("RPROP decrease factor                :%8.3f\n", ann->RpropDecreaseFactor);
	printf("RPROP delta min                      :%8.3f\n", ann->RpropDeltaMin);
	printf("RPROP delta max                      :%8.3f\n", ann->RpropDeltaMax);
	printf("Cascade output change fraction       :%11.6f\n", ann->CascadeOutputChangeFraction);
	printf("Cascade candidate change fraction    :%11.6f\n", ann->CascadeCandidateChangeFraction);
	printf("Cascade output stagnation epochs     :%4d\n", ann->CascadeOutputStagnationEpochs);
	printf("Cascade candidate stagnation epochs  :%4d\n", ann->CascadeCandidateStagnationEpochs);
	printf("Cascade max output epochs            :%4d\n", ann->CascadeMaxOutEpochs);
	printf("Cascade min output epochs            :%4d\n", ann->CascadeMinOutEpochs);
	printf("Cascade max candidate epochs         :%4d\n", ann->CascadeMaxCandEpochs);
	printf("Cascade min candidate epochs         :%4d\n", ann->CascadeMinCandEpochs);
	printf("Cascade weight multiplier            :%8.3f\n", ann->CascadeWeightMultiplier);
	printf("Cascade candidate limit              :%8.3f\n", ann->CascadeCandidateLimit);
	/*
	for(i = 0; i < ann->cascade_activation_functions_count; i++)
		printf("Cascade activation functions[%d]      :   %s\n", i, FANN_ACTIVATIONFUNC_NAMES[ann->P_CascadeActivationFunctions[i]]);
	*/
	for(i = 0; i < ann->CascadeActivationFuncList.getCount(); i++) {
		printf("Cascade activation functions[%d]      :   %s\n", i, Fann::GetAttrText(Fann::attrActivationFunc, ann->CascadeActivationFuncList.get(i)));
	}
	/*
	for(i = 0; i < ann->cascade_activation_steepnesses_count; i++)
		printf("Cascade activation steepnesses[%d]    :%8.3f\n", i, ann->CascadeActivationSteepnesses[i]);
	*/
	for(i = 0; i < ann->CascadeActivationSteepnessesList.getCount(); i++) {
		printf("Cascade activation steepnesses[%d]    :%8.3f\n", i, ann->CascadeActivationSteepnessesList[i]);
	}
	printf("Cascade candidate groups             :%4d\n", ann->CascadeNumCandidateGroups);
	printf("Cascade no. of candidates            :%4d\n", ann->GetCascadeNumCandidates());
	/* TODO: dump scale parameters */
#endif
}

FANN_EXTERNAL float FANN_API fann_get_connection_rate(Fann * ann)
{
	return ann->ConnectionRate;
}

void Fann::GetLayerArray(LongArray & rList) const
{
	rList.clear();
	for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
		uint count = p_layer_it->GetCount();
		// Remove the bias from the count of neurons
		switch(GetNetworkType()) {
			case FANN_NETTYPE_LAYER:
			    --count;
			    break;
			case FANN_NETTYPE_SHORTCUT:
			    // The bias in the first layer is reused for all layers
			    if(p_layer_it == P_FirstLayer)
				    --count;
			    break;
			default:
			    // Unknown network type, assume no bias present
			    break;
		}
		rList.add((long)count);
	}
	assert(rList == Layers);
}

//FANN_EXTERNAL void FANN_API fann_get_bias_array(Fann * ann, uint * pBias)
void Fann::GetBiasArray(LongArray & rList) const
{
	rList.clear();
	for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; ++p_layer_it) {
		long bias = 0; // For unknown network type assume no bias present
		switch(GetNetworkType()) {
			case FANN_NETTYPE_LAYER:
			    // Report one bias in each layer except the last
			    bias = (p_layer_it != (P_LastLayer-1)) ? 1 : 0;
			    break;
			case FANN_NETTYPE_SHORTCUT:
			    // The bias in the first layer is reused for all layers
			    bias = (p_layer_it == P_FirstLayer) ? 1 : 0;
			    break;
		}
		rList.add(bias);
	}
}

int Fann::GetConnectionArray(TSArray <FannConnection> & rList) const
{
	rList.clear();
	int    ok = 1;
	uint   _source_index = 0;
	uint   _destination_index = 0;
	const  Fann::Neuron * p_first_neuron = P_FirstLayer->P_FirstNeuron;
	// The following assumes that the last unused bias has no connections
	for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
		for(Fann::Neuron * neuron_it = p_layer_it->P_FirstNeuron; neuron_it != p_layer_it->P_LastNeuron; neuron_it++) {
			for(uint idx = neuron_it->FirstCon; idx < neuron_it->LastCon; idx++) {
				// Assign the source, destination and weight
				FannConnection c;
				c.FromNeuron = (uint)(PP_Connections[_source_index] - p_first_neuron);
				c.ToNeuron = _destination_index;
				c.Weight = P_Weights[_source_index];
				THROW(rList.insert(&c));
				_source_index++;
			}
			_destination_index++;
		}
	}
	CATCHZOK
	return ok;
}

//FANN_EXTERNAL void FANN_API fann_set_weight_array(Fann * ann, FannConnection * pConnections, uint num_connections)
void Fann::SetWeightArray(const FannConnection * pConnections, uint numConnections)
{
	for(uint idx = 0; idx < numConnections; idx++) {
		SetWeight(pConnections[idx].FromNeuron, pConnections[idx].ToNeuron, pConnections[idx].Weight);
	}
}

//FANN_EXTERNAL void FANN_API fann_set_weight(Fann * ann, uint fromNeuron, uint toNeuron, ANNTYP weight)
void Fann::SetWeight(uint fromNeuron, uint toNeuron, ANNTYP weight)
{
	uint   _source_index = 0;
	uint   _destination_index = 0;
	Fann::Neuron * p_first_neuron = P_FirstLayer->P_FirstNeuron;
	// Find the connection, simple brute force search through the network
	// for one or more connections that match to minimize datastructure dependencies.
	// Nothing is done if the connection does not already exist in the network.
	for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
		for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_layer_it->P_LastNeuron; p_neuron_it++) {
			for(uint idx = p_neuron_it->FirstCon; idx < p_neuron_it->LastCon; idx++) {
				// If the source and destination neurons match, assign the weight
				if(toNeuron == _destination_index && ((int)fromNeuron == (PP_Connections[_source_index] - p_first_neuron))) {
					P_Weights[_source_index] = weight;
				}
				_source_index++;
			}
			_destination_index++;
		}
	}
}

//FANN_EXTERNAL void FANN_API fann_get_weights(Fann * ann, ANNTYP * weights)
size_t Fann::GetWeights(ANNTYP * pWeights, size_t bufferSize) const
{
	size_t moved_size = 0;
	const size_t _size = sizeof(ANNTYP) * TotalConnections;
	if(pWeights == 0)
		moved_size = _size;
	else if(bufferSize >= _size) {
		memcpy(pWeights, P_Weights, _size);
		moved_size = _size;
	}
	return moved_size;
}

//FANN_EXTERNAL void FANN_API fann_set_weights(Fann * ann, ANNTYP * weights)
int Fann::SetWeights(const ANNTYP * pWeights)
{
	memcpy(P_Weights, pWeights, sizeof(ANNTYP) * TotalConnections);
	return 1;
}

//#ifdef FIXEDFANN

//FANN_GET(uint, decimal_point)
//FANN_GET(uint, multiplier)
//
// INTERNAL FUNCTION
// Adjust the steepwise functions (if used)
//
//void fann_update_stepwise(Fann * ann)
void Fann::UpdateStepwise()
{
	//
	// Calculate the parameters for the stepwise linear
	// sigmoid function fixed point.
	// Using a rewritten sigmoid function.
	// results 0.005, 0.05, 0.25, 0.75, 0.95, 0.995
	//
	SigmoidResults[0] = MAX((ANNTYP)(Multiplier / 200.0 + 0.5), 1);
	SigmoidResults[1] = MAX((ANNTYP)(Multiplier / 20.0 + 0.5), 1);
	SigmoidResults[2] = MAX((ANNTYP)(Multiplier / 4.0 + 0.5), 1);
	SigmoidResults[3] = MIN(Multiplier - (ANNTYP)(Multiplier / 4.0 + 0.5), Multiplier - 1);
	SigmoidResults[4] = MIN(Multiplier - (ANNTYP)(Multiplier / 20.0 + 0.5), Multiplier - 1);
	SigmoidResults[5] = MIN(Multiplier - (ANNTYP)(Multiplier / 200.0 + 0.5), Multiplier - 1);
	SigmoidSymmetricResults[0] = MAX((ANNTYP)((Multiplier / 100.0) - Multiplier - 0.5), (ANNTYP)(1 - (ANNTYP)Multiplier));
	SigmoidSymmetricResults[1] = MAX((ANNTYP)((Multiplier / 10.0) - Multiplier - 0.5), (ANNTYP)(1 - (ANNTYP)Multiplier));
	SigmoidSymmetricResults[2] = MAX((ANNTYP)((Multiplier / 2.0) - Multiplier - 0.5), (ANNTYP)(1 - (ANNTYP)Multiplier));
	SigmoidSymmetricResults[3] = MIN(Multiplier - (ANNTYP)(Multiplier / 2.0 + 0.5), Multiplier - 1);
	SigmoidSymmetricResults[4] = MIN(Multiplier - (ANNTYP)(Multiplier / 10.0 + 0.5), Multiplier - 1);
	SigmoidSymmetricResults[5] = MIN(Multiplier - (ANNTYP)(Multiplier / 100.0 + 1.0), Multiplier - 1);
	for(uint i = 0; i < 6; i++) {
		SigmoidValues[i] = (ANNTYP)(((log(Multiplier / (float)SigmoidResults[i] - 1) * (float)Multiplier) / -2.0) * (float)Multiplier);
		SigmoidSymmetricValues[i] = (ANNTYP)(((log((Multiplier - (float)SigmoidSymmetricResults[i]) / ((float)SigmoidSymmetricResults[i] + Multiplier)) * (float)Multiplier) / -2.0) * (float)Multiplier);
	}
}

//#endif

FannError::FannError()
{
	errno_f = SLERR_SUCCESS;
	//error_log = fann_default_error_log;
}

FannError & FannError::Copy(const FannError & rS)
{
	errno_f = rS.errno_f;
	//error_log = rS.error_log;
	Msg = rS.Msg;
	return *this;
}

IMPL_INVARIANT_C(Fann)
{
	S_INVARIANT_PROLOG(pInvP);
	const uint num_layers = Layers.getCount();
	S_ASSERT_P(num_layers > 1 && num_layers <= 1000, pInvP);
	{
		for(uint i = 0; i < num_layers; i++) {
			const long neurons_in_layer = Layers.get(i);
			S_ASSERT_P(neurons_in_layer >= 0 && neurons_in_layer <= 1000000, pInvP);
		}
	}
	S_ASSERT_P(Layers.at(0) == NumInput, pInvP);
	S_ASSERT_P(Layers.at(num_layers-1) == NumOutput, pInvP);
	{
		uint   ln = 0;
		uint   total_neurons = 0;
		for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
			S_ASSERT_P(Layers.at(ln) == p_layer_it->_Dim, pInvP);
			ln++;
			total_neurons += p_layer_it->GetCount();
		}
		S_ASSERT_P(ln == Layers.getCount(), pInvP);
		S_ASSERT_P(total_neurons == TotalNeurons, pInvP);
	}
	S_INVARIANT_EPILOG(pInvP);
}

int Fann::AllocateLayers()
{
	int    ok = 1;
	uint   i = 0;
	THROW_S(P_FirstLayer = new Layer[Layers.getCount()], SLERR_NOMEM);
	P_LastLayer = P_FirstLayer + Layers.getCount();
	for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
		p_layer_it->_Dim = Layers.get(i++);
		p_layer_it->P_FirstNeuron = NULL;
		p_layer_it->P_LastNeuron = NULL;
	}
	assert(i == Layers.getCount());
	CATCHZOK
	return ok;
}

int Fann::Helper_Construct(int type, float connectionRate, const LongArray & rLayers)
{
	int    ok = 1;
	State = 0;
	{
		const uint _lc = rLayers.getCount();
		assert(_lc > 1 && _lc <= 1000);
		THROW_S(_lc > 1 && _lc <= 1000, SLERR_FANN_INVLAYERCOUNT);
		for(uint i = 0; i < _lc; i++) {
			const long v = rLayers.get(i);
			assert(v >= 0 && v <= 1000000);
			THROW_S(v >= 0 && v <= 1000000, SLERR_FANN_INVLAYERSIZE);
		}
	}
	fann_seed_rand();
	NetworkType = type; // FANN_NETTYPE_LAYER;
	ConnectionRate = MINMAX(connectionRate, 0.00001f, 1.0f);
	Layers = rLayers;
	LearningRate = 0.7f;
	LearningMomentum = 0.0;
	TotalNeurons = 0;
	TotalConnections = 0;
	NumInput = 0;
	NumOutput = 0;
	P_TrainErrors = NULL;
	P_TrainSlopes = NULL;
	P_PrevSteps = NULL;
	P_PrevTrainSlopes = NULL;
	P_PrevWeightsDeltas = NULL;
	TrainingAlgorithm = FANN_TRAIN_RPROP;
	num_MSE = 0;
	MSE_value = 0;
	NumBitFail = 0;
	BitFailLimit = (ANNTYP)0.35;
	TrainErrorFunction = FANN_ERRORFUNC_TANH;
	TrainStopFunction = FANN_STOPFUNC_MSE;
	Callback = NULL;
	P_UserData = NULL; // User is responsible for deallocation
	P_Weights = NULL;
	PP_Connections = NULL;
	P_Output = NULL;
//#ifdef FIXEDFANN
	// these values are only boring defaults, and should really
	// never be used, since the real values are always loaded from a file.
	DecimalPoint = 8;
	Multiplier = 256;
//#else
	ScaleIn.Destroy();
	ScaleOut.Destroy();
//#endif
	// variables used for cascade correlation (reasonable defaults)
	CascadeOutputChangeFraction = 0.01f;
	CascadeCandidateChangeFraction = 0.01f;
	CascadeOutputStagnationEpochs = 12;
	CascadeCandidateStagnationEpochs = 12;
	CascadeNumCandidateGroups = 2;
	CascadeWeightMultiplier = (ANNTYP)0.4;
	CascadeCandidateLimit = (ANNTYP)1000.0;
	CascadeMaxOutEpochs = 150;
	CascadeMaxCandEpochs = 150;
	CascadeMinOutEpochs = 50;
	CascadeMinCandEpochs = 50;
	P_CascadeCandidateScores = NULL;
	{
		CascadeActivationFuncList.clear();
		CascadeActivationFuncList.addzlist(FANN_SIGMOID, FANN_SIGMOID_SYMMETRIC, FANN_GAUSSIAN, FANN_GAUSSIAN_SYMMETRIC,
			FANN_ELLIOT, FANN_ELLIOT_SYMMETRIC, FANN_SIN_SYMMETRIC, FANN_COS_SYMMETRIC, FANN_SIN, FANN_COS, 0);
		assert(CascadeActivationFuncList.getCount() == 10);
	}
	{
		CascadeActivationSteepnessesList.clear();
		CascadeActivationSteepnessesList.add(0.25f);
		CascadeActivationSteepnessesList.add(0.5f);
		CascadeActivationSteepnessesList.add(0.75f);
		CascadeActivationSteepnessesList.add(1.0f);
		assert(CascadeActivationSteepnessesList.getCount() == 4);
	}
	// Variables for use with with Quickprop training (reasonable defaults)
	QuickpropDecay = -0.0001f;
	QuickpropMu = 1.75;
	// Variables for use with with RPROP training (reasonable defaults)
	RpropIncreaseFactor = 1.2f;
	RpropDecreaseFactor = 0.5;
	RpropDeltaMin = 0.0;
	RpropDeltaMax = 50.0;
	RpropDeltaZero = 0.1f;
	// Variables for use with SARPROP training (reasonable defaults)
	SarpropWeightDecayShift = -6.644f;
	SarpropStepErrorThresholdFactor = 0.1f;
	SarpropStepErrorShift = 1.385f;
	SarpropTemperature = 0.015f;
	SarpropEpoch = 0;
	Err.Msg = 0;
	Err.errno_f = SLERR_SUCCESS;
	// allocate room for the layers
	//THROW_S(P_FirstLayer = (Fann::Layer *)SAlloc::C(rLayers.getCount(), sizeof(Fann::Layer)), SLERR_NOMEM);
	THROW(AllocateLayers());
	//
	const int multiplier = Multiplier;
	if(NetworkType == FANN_NETTYPE_LAYER) {
#ifdef FIXEDFANN
		UpdateStepwise();
#endif
		{
			// determine how many neurons there should be in each layer
			for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
				// we do not allocate room here, but we make sure that
				// last_neuron - first_neuron is the number of neurons
				p_layer_it->P_LastNeuron = p_layer_it->P_FirstNeuron + p_layer_it->_Dim + 1; // +1 for bias
				TotalNeurons += p_layer_it->GetCount();
			}
		}
		NumOutput = ((P_LastLayer-1)->GetCount() - 1);
		NumInput = (P_FirstLayer->GetCount() - 1);
		// allocate room for the actual neurons
		THROW(AllocateNeurons());
		{
			uint _num_neurons_in = NumInput;
			for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != P_LastLayer; p_layer_it++) {
				const uint _num_neurons_out = (p_layer_it->GetCount() - 1);
				// if all neurons in each layer should be connected to at least one neuron
				// in the previous layer, and one neuron in the next layer.
				// and the bias node should be connected to the all neurons in the next layer.
				// Then this is the minimum amount of neurons
				const uint _min_connections = MAX(_num_neurons_in, _num_neurons_out); // not calculating bias
				const uint _max_connections = _num_neurons_in * _num_neurons_out; // not calculating bias
				const uint _num_connections = MAX(_min_connections, (uint)(0.5 + (connectionRate * _max_connections))) + _num_neurons_out;
				const uint _connections_per_neuron = _num_connections / _num_neurons_out;
				uint _allocated_connections = 0;
				// Now split out the connections on the different neurons
				uint i = 0;
				for(i = 0; i != _num_neurons_out; i++) {
					p_layer_it->P_FirstNeuron[i].FirstCon = TotalConnections + _allocated_connections;
					_allocated_connections += _connections_per_neuron;
					p_layer_it->P_FirstNeuron[i].LastCon = TotalConnections + _allocated_connections;
					p_layer_it->P_FirstNeuron[i].ActivationFunction = Fann::FANN_SIGMOID_STEPWISE;
#ifdef FIXEDFANN
					p_layer_it->P_FirstNeuron[i].ActivationSteepness = Multiplier / 2;
#else
					p_layer_it->P_FirstNeuron[i].ActivationSteepness = 0.5;
#endif
					if(_allocated_connections < (_num_connections * (i + 1)) / _num_neurons_out) {
						p_layer_it->P_FirstNeuron[i].LastCon++;
						_allocated_connections++;
					}
				}
				// bias neuron also gets stuff
				p_layer_it->P_FirstNeuron[i].FirstCon = TotalConnections + _allocated_connections;
				p_layer_it->P_FirstNeuron[i].LastCon = TotalConnections + _allocated_connections;
				TotalConnections += _num_connections;
				// used in the next run of the loop
				_num_neurons_in = _num_neurons_out;
			}
		}
		THROW(AllocateConnections());
		if(connectionRate >= 1) {
			uint   prev_layer_size = NumInput + 1; // @debug
			const Fann::Layer * p_prev_layer = P_FirstLayer;
			const Fann::Layer * p_last_layer = P_LastLayer;
			for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != p_last_layer; p_layer_it++) {
				const Fann::Neuron * last_neuron = p_layer_it->P_LastNeuron - 1;
				for(Fann::Neuron * neuron_it = p_layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
					const uint _tmp_con = neuron_it->LastCon - 1;
					for(uint i = neuron_it->FirstCon; i != _tmp_con; i++) {
						P_Weights[i] = (ANNTYP)fann_random_weight();
						// these connections are still initialized for fully connected networks, to allow
						// operations to work, that are not optimized for fully connected networks.
						PP_Connections[i] = p_prev_layer->P_FirstNeuron + (i - neuron_it->FirstCon);
					}
					// bias weight
					P_Weights[_tmp_con] = (ANNTYP)fann_random_bias_weight();
					PP_Connections[_tmp_con] = p_prev_layer->P_FirstNeuron + (_tmp_con - neuron_it->FirstCon);
				}
				prev_layer_size = p_layer_it->GetCount(); // @debug
				p_prev_layer = p_layer_it;
			}
		}
		else {
			// make connections for a network, that are not fully connected
			//
			// generally, what we do is first to connect all the input
			// neurons to a output neuron, respecting the number of
			// available input neurons for each output neuron. Then
			// we go through all the output neurons, and connect the
			// rest of the connections to input neurons, that they are not allready connected to.
			//
			// All the connections are cleared by calloc, because we want to
			// be able to see which connections are allready connected */
			//
			for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != P_LastLayer; p_layer_it++) {
				const uint num_neurons_out = (p_layer_it->GetCount() - 1);
				const uint num_neurons_in = ((p_layer_it - 1)->GetCount() - 1);
				// first connect the bias neuron
				Fann::Neuron * p_bias_neuron = (p_layer_it - 1)->P_LastNeuron - 1;
				const Fann::Neuron * last_neuron = p_layer_it->P_LastNeuron - 1;
				{
					for(Fann::Neuron * neuron_it = p_layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
						PP_Connections[neuron_it->FirstCon] = p_bias_neuron;
						P_Weights[neuron_it->FirstCon] = (ANNTYP)fann_random_bias_weight();
					}
				}
				// then connect all neurons in the input layer
				last_neuron = (p_layer_it - 1)->P_LastNeuron - 1;
				{
					for(Fann::Neuron * neuron_it = (p_layer_it - 1)->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
						// random neuron in the output layer that has space for more connections
						Fann::Neuron * p_random_neuron;
						do {
							const uint _random_number = (int)(0.5 + fann_rand(0, num_neurons_out - 1));
							p_random_neuron = p_layer_it->P_FirstNeuron + _random_number;
							// checks the last space in the connections array for room
						} while(PP_Connections[p_random_neuron->LastCon-1]);
						// find an empty space in the connection array and connect
						for(uint i = p_random_neuron->FirstCon; i < p_random_neuron->LastCon; i++) {
							if(PP_Connections[i] == NULL) {
								PP_Connections[i] = neuron_it;
								P_Weights[i] = (ANNTYP)fann_random_weight();
								break;
							}
						}
					}
				}
				// then connect the rest of the unconnected neurons
				last_neuron = p_layer_it->P_LastNeuron - 1;
				{
					for(Fann::Neuron * neuron_it = p_layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
						// find empty space in the connection array and connect
						for(uint i = neuron_it->FirstCon; i < neuron_it->LastCon; i++) {
							if(PP_Connections[i] == NULL) { // continue if allready connected
								uint   _found_connection;
								Fann::Neuron * p_random_neuron = 0;
								do {
									_found_connection = 0;
									const uint _random_number = (int)(0.5 + fann_rand(0, num_neurons_in - 1));
									p_random_neuron = (p_layer_it - 1)->P_FirstNeuron + _random_number;
									// check to see if this connection is allready there
									for(uint j = neuron_it->FirstCon; j < i; j++) {
										if(p_random_neuron == PP_Connections[j]) {
											_found_connection = 1;
											break;
										}
									}
								} while(_found_connection);
								// we have found a neuron that is not allready connected to us, connect it
								PP_Connections[i] = p_random_neuron;
								P_Weights[i] = (ANNTYP)fann_random_weight();
							}
						}
					}
				}
			}
			//
			// TODO it would be nice to have the randomly created connections sorted for smoother memory access.
			//
		}
	}
	else if(NetworkType == FANN_NETTYPE_SHORTCUT) {
#ifdef FIXEDFANN
		UpdateStepwise();
#endif
		{
			// determine how many neurons there should be in each layer
			for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
				// we do not allocate room here, but we make sure that
				// last_neuron - P_FirstNeuron is the number of neurons
				p_layer_it->P_LastNeuron = p_layer_it->P_FirstNeuron + p_layer_it->_Dim/*pLayers[i++]*/;
				if(p_layer_it == P_FirstLayer)
					p_layer_it->P_LastNeuron++; // there is a bias neuron in the first layer
				TotalNeurons += p_layer_it->GetCount();
			}
		}
		NumOutput = (P_LastLayer-1)->GetCount();
		NumInput = (P_FirstLayer->GetCount() - 1);
		THROW(AllocateNeurons());
		{
			uint num_neurons_in = NumInput;
			for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != P_LastLayer; p_layer_it++) {
				const uint num_neurons_out = p_layer_it->GetCount();
				// Now split out the connections on the different neurons
				for(uint i = 0; i != num_neurons_out; i++) {
					p_layer_it->P_FirstNeuron[i].FirstCon = TotalConnections;
					TotalConnections += num_neurons_in + 1;
					p_layer_it->P_FirstNeuron[i].LastCon = TotalConnections;
					p_layer_it->P_FirstNeuron[i].ActivationFunction = Fann::FANN_SIGMOID_STEPWISE;
#ifdef FIXEDFANN
					p_layer_it->P_FirstNeuron[i].ActivationSteepness = Multiplier / 2;
#else
					p_layer_it->P_FirstNeuron[i].ActivationSteepness = 0.5;
#endif
				}
				num_neurons_in += num_neurons_out; // used in the next run of the loop
			}
		}
		THROW(AllocateConnections());
		{
			// Connections are created from all neurons to all neurons in later layers
			uint num_neurons_in = NumInput + 1;
			for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != P_LastLayer; p_layer_it++) {
				for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_layer_it->P_LastNeuron; p_neuron_it++) {
					uint i = p_neuron_it->FirstCon;
					for(Fann::Layer * p_layer_it2 = P_FirstLayer; p_layer_it2 != p_layer_it; p_layer_it2++) {
						for(Fann::Neuron * p_neuron_it2 = p_layer_it2->P_FirstNeuron; p_neuron_it2 != p_layer_it2->P_LastNeuron; p_neuron_it2++) {
							P_Weights[i] = (ANNTYP)fann_random_weight();
							PP_Connections[i] = p_neuron_it2;
							i++;
						}
					}
				}
				num_neurons_in += p_layer_it->GetCount();
			}
		}
	}
	{
		SInvariantParam ip;
		assert(InvariantC(&ip));
	}
	CATCH
		// ZFREE(P_CascadeActivationFunctions);
		// ZFREE(CascadeActivationSteepnesses);
		Destroy();
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
}

void Fann::Helper_Init()
{
	P_Weights = 0;
	PP_Connections = 0;
	P_FirstLayer = 0;
	P_LastLayer = 0;
	P_Output = 0;
	P_TrainErrors = 0;
	P_TrainSlopes = 0;
	P_PrevTrainSlopes = 0;
	P_PrevSteps = 0;
	P_PrevWeightsDeltas = 0;
	P_CascadeCandidateScores = 0;
	TotalNeurons = 0;
	TotalNeuronsAllocated = 0;
	TotalConnections = 0;
	TotalConnectionsAllocated = 0;
}

Fann::Fann(int type, float connectionRate, const LongArray & rLayers)
{
	Helper_Init();
	Helper_Construct(type, connectionRate, rLayers);
}

Fann::Fann(const Fann & rS)
{
	Helper_Init();
	Copy(rS);
}

Fann::Fann(SBuffer & rBuf, SSerializeContext * pSCtx)
{
	Helper_Init();
	if(!Serialize(-1, rBuf, pSCtx)) {
		Destroy();
		State |= stError;
	}
}

Fann::~Fann()
{
	Destroy();
}

void Fann::Destroy()
{
	State = 0;
	ZFREE(P_Weights);
	ZFREE(PP_Connections);
	if(P_FirstLayer) {
		ZFREE(P_FirstLayer->P_FirstNeuron);
		delete [] P_FirstLayer;
		P_FirstLayer = 0;
	}
	P_LastLayer = 0;
	ZFREE(P_Output);
	ZFREE(P_TrainErrors);
	ZFREE(P_TrainSlopes);
	ZFREE(P_PrevTrainSlopes);
	ZFREE(P_PrevSteps);
	ZFREE(P_PrevWeightsDeltas);
	ZFREE(P_CascadeCandidateScores);
	MEMSZERO(SigmoidResults);
	MEMSZERO(SigmoidValues);
	MEMSZERO(SigmoidSymmetricResults);
	MEMSZERO(SigmoidSymmetricValues);
	Layers.freeAll();
	ScaleIn.Destroy();
	ScaleOut.Destroy();
	TotalNeurons = 0;
	TotalConnectionsAllocated = 0;
	TotalConnections = 0;
	TotalConnectionsAllocated = 0;
	Callback = 0;
	P_UserData = 0;
	CascadeActivationFuncList.freeAll();
	CascadeActivationSteepnessesList.freeAll();
}

FANN_EXTERNAL void FANN_API fann_destroy(Fann * pAnn)
{
	ZDELETE(pAnn);
}
//
// INTERNAL FUNCTION
// Allocates the main structure and sets some default values.
//
/*Fann * fann_allocate_structure(uint num_layers)
{
	if(num_layers < 2) {
		return NULL;
	}
	else {
		Fann * p_ann = new Fann(num_layers);
		if(p_ann->IsError())
			ZDELETE(p_ann);
		return p_ann;
	}
}*/

float * Fann::ScaleAllocate(uint c, float defValue)
{
	float * p_list = (float *)SAlloc::C(c, sizeof(float));
	if(p_list == NULL) {
		fann_error(&Err, SLERR_FANN_CANT_ALLOCATE_MEM);
		//fann_destroy(ann);
	}
	else {
		for(uint i = 0; i < c; i++)
			p_list[i] = defValue;
	}
	return p_list;
	/*
	ann->what ## _ ## where = (float*)SAlloc::C(ann->num_ ## where ## put, sizeof( float )); \
	if(ann->what ## _ ## where == NULL) { \
		fann_error(NULL, SLERR_FANN_CANT_ALLOCATE_MEM);				      \
		fann_destroy(ann);							      \
		return 1;														\
	}																	\
	for(i = 0; i < ann->num_ ## where ## put; i++)						  \
		ann->what ## _ ## where[ i ] = ( default_value );
	*/
}
//
// INTERNAL FUNCTION
// Allocates room for the scaling parameters.
//
//int fann_allocate_scale(Fann * ann)
int Fann::AllocateScale()
{
	// todo this should only be allocated when needed
#ifndef FIXEDFANN
	/*
	uint i = 0;
#define SCALE_ALLOCATE(what, where, default_value)				      \
	ann->what ## _ ## where = (float*)SAlloc::C(ann->num_ ## where ## put, sizeof( float )); \
	if(ann->what ## _ ## where == NULL) { \
		fann_error(NULL, SLERR_FANN_CANT_ALLOCATE_MEM);				      \
		fann_destroy(ann);							      \
		return 1;														\
	}																	\
	for(i = 0; i < ann->num_ ## where ## put; i++)						  \
		ann->what ## _ ## where[ i ] = ( default_value );
	SCALE_ALLOCATE(scale_mean,             in,      0.0)
	SCALE_ALLOCATE(scale_deviation,        in,      1.0)
	SCALE_ALLOCATE(scale_new_min,          in,     -1.0)
	SCALE_ALLOCATE(scale_factor,           in,      1.0)
	SCALE_ALLOCATE(scale_mean,             out,     0.0)
	SCALE_ALLOCATE(scale_deviation,        out,     1.0)
	SCALE_ALLOCATE(scale_new_min,          out,    -1.0)
	SCALE_ALLOCATE(scale_factor,           out,     1.0)
#undef SCALE_ALLOCATE
	*/
	THROW(ScaleIn.Allocate(NumInput));
	THROW(ScaleOut.Allocate(NumOutput));
	CATCH
		fann_destroy(this); // @bad
		return 0;
	ENDCATCH
#endif
	return 1;
}

int Fann::AllocateNeurons()
{
	int    ok = 1;
	EXCEPTVAR(*(int *)Err.errno_f);
	uint _num_neurons_so_far = 0;
	uint _num_neurons = 0;
	// all the neurons is allocated in one long array (calloc clears mem)
	Fann::Neuron * p_neurons = (Fann::Neuron *)SAlloc::C(TotalNeurons, sizeof(Fann::Neuron));
	TotalNeuronsAllocated = TotalNeurons;
	THROW_V(p_neurons, SLERR_FANN_CANT_ALLOCATE_MEM);
	for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
		_num_neurons = p_layer_it->GetCount();
		p_layer_it->P_FirstNeuron = p_neurons + _num_neurons_so_far;
		p_layer_it->P_LastNeuron = p_layer_it->P_FirstNeuron + _num_neurons;
		_num_neurons_so_far += _num_neurons;
	}
	THROW_V(P_Output = (ANNTYP *)SAlloc::C(_num_neurons, sizeof(ANNTYP)), SLERR_FANN_CANT_ALLOCATE_MEM);
	CATCH
		fann_error(&Err, Err.errno_f);
		ok = 0;
	ENDCATCH
	return ok;
}

int Fann::AllocateConnections()
{
	int    ok = 1;
	THROW(P_Weights = (ANNTYP *)SAlloc::C(TotalConnections, sizeof(ANNTYP)));
	TotalConnectionsAllocated = TotalConnections;
	// TODO make special cases for all places where the connections
	// is used, so that it is not needed for fully connected networks
	THROW(PP_Connections = (Fann::Neuron **)SAlloc::C(TotalConnectionsAllocated, sizeof(Fann::Neuron *)));
	CATCH
		ok = 0;
		fann_error(&Err, SLERR_FANN_CANT_ALLOCATE_MEM);
	ENDCATCH
	return ok;
}

#ifdef FANN_NO_SEED
	int FANN_SEED_RAND = 0;
#else
	int FANN_SEED_RAND = 1;
#endif

FANN_EXTERNAL void FANN_API fann_disable_seed_rand()
{
	FANN_SEED_RAND = 0;
}

FANN_EXTERNAL void FANN_API fann_enable_seed_rand()
{
	FANN_SEED_RAND = 1;
}
//
// INTERNAL FUNCTION
// Seed the random function.
//
void fann_seed_rand()
{
#ifndef _WIN32
	FILE * fp = fopen("/dev/urandom", "r");
	uint foo;
	struct timeval t;
	if(!fp) {
		gettimeofday(&t, 0);
		foo = t.tv_usec;
	}
	else {
		if(fread(&foo, sizeof(foo), 1, fp) != 1) {
			gettimeofday(&t, 0);
			foo = t.tv_usec;
		}
		fclose(fp);
	}
	if(FANN_SEED_RAND) {
		srand(foo);
	}
#else
	/* COMPAT_TIME REPLACEMENT */
	if(FANN_SEED_RAND) {
		srand(GetTickCount());
	}
#endif
}
//
// FANN_CASCADE
//
#ifndef FIXEDFANN

/* #define CASCADE_DEBUG */
/* #define CASCADE_DEBUG_FULL */

void fann_print_connections_raw(Fann * ann)
{
	for(uint i = 0; i < ann->TotalConnectionsAllocated; i++) {
		if(i == ann->TotalConnections)
			printf("* ");
		printf("%f ", ann->P_Weights[i]);
	}
	printf("\n\n");
}
//
// Cascade training directly on the training data.
// The connected_neurons pointers are not valid during training,
// but they will be again after training.
//
//FANN_EXTERNAL void FANN_API fann_cascadetrain_on_data(Fann * ann, Fann::TrainData * pData, uint maxNeurons, uint neuronsBetweenReports, float desiredError)
int Fann::CascadeTrainOnData(const Fann::TrainData * pData, uint maxNeurons, uint neuronsBetweenReports, float desiredError)
{
	int    ok = 1;
	float  _error;
	uint   _total_epochs = 0;
	int    is_desired_error_reached = 0;
	if(neuronsBetweenReports && !Callback) {
		printf("Max neurons %3d. Desired error: %.6f\n", maxNeurons, desiredError);
	}
	for(uint i = 1; i <= maxNeurons; i++) {
		// train output neurons
		_total_epochs += TrainOutputs(pData, desiredError);
		_error = GetMSE();
		is_desired_error_reached = DesiredErrorReached(desiredError);
		// print current error
		if(neuronsBetweenReports && (i % neuronsBetweenReports == 0 || i == maxNeurons || i == 1 || is_desired_error_reached)) {
			if(!Callback) {
				printf("Neurons     %3d. Current error: %.6f. Total error:%8.4f. Epochs %5d. Bit fail %3d", i-1, _error, MSE_value, _total_epochs, NumBitFail);
				if((P_LastLayer-2) != P_FirstLayer) {
					printf(". candidate steepness %.2f. function %s", (P_LastLayer-2)->P_FirstNeuron->ActivationSteepness,
						Fann::GetAttrText(Fann::attrActivationFunc, (P_LastLayer-2)->P_FirstNeuron->ActivationFunction));
				}
				printf("\n");
			}
			else if((*Callback)(this, pData, maxNeurons, neuronsBetweenReports, desiredError, _total_epochs) == -1) {
				break; // you can break the training by returning -1
			}
		}
		if(is_desired_error_reached)
			break;
		else {
			THROW(InitializeCandidates());
			// train new candidates
			_total_epochs += TrainCandidates(pData);
			//InstallCandidate(); // this installs the best candidate
			//void fann_install_candidate(Fann * ann)
			//void Fann::InstallCandidate()
			{
				Fann::Layer * p_new_layer = AddLayer(P_LastLayer-1);
				THROW(p_new_layer);
				AddCandidateNeuron(p_new_layer);
			}
		}
	}
	// Train outputs one last time but without any desired error
	_total_epochs += TrainOutputs(pData, 0.0);
	if(neuronsBetweenReports && !Callback) {
		printf("Train outputs    Current error: %.6f. Epochs %6d\n", GetMSE(), _total_epochs);
	}
	// Set pointers in connected_neurons
	// This is ONLY done in the end of cascade training,
	// since there is no need for them during training.
	SetShortcutConnections();
	CATCHZOK
	return ok;
}

/*FANN_EXTERNAL void FANN_API fann_cascadetrain_on_file(Fann * ann, const char * filename, uint max_neurons, uint neurons_between_reports, float desired_error)
{
	Fann::TrainData * p_data = fann_read_train_from_file(filename);
	if(p_data) {
		ann->CascadeTrainOnData(p_data, max_neurons, neurons_between_reports, desired_error);
		delete p_data;
	}
}*/

//int fann_train_outputs(Fann * ann, Fann::TrainData * pData, float desiredError)
int Fann::TrainOutputs(const Fann::TrainData * pData, float desiredError)
{
	float _target_improvement = 0.0;
	float _backslide_improvement = -1.0e20f;
	const uint _max_epochs = CascadeMaxOutEpochs;
	const uint _min_epochs = CascadeMinOutEpochs;
	uint  _stagnation = _max_epochs;
	// TODO should perhaps not clear all arrays
	ClearTrainArrays();
	// run an initial epoch to set the initital error
	const float initial_error = TrainOutputsEpoch(pData);
	if(DesiredErrorReached(desiredError))
		return 1;
	else {
		for(uint i = 1; i < _max_epochs; i++) {
			const float _error = TrainOutputsEpoch(pData);
			// printf("Epoch %6d. Current error: %.6f. Bit fail %d.\n", i, error, ann->num_bit_fail);
			if(DesiredErrorReached(desiredError)) {
				return (i + 1);
			}
			else {
				// Improvement since start of train
				const float error_improvement = initial_error - _error;
				// After any significant change, set a new goal and allow a new quota of epochs to reach it
				if((_target_improvement >= 0 && (error_improvement > _target_improvement || error_improvement < _backslide_improvement)) ||
					(_target_improvement < 0 && (error_improvement < _target_improvement || error_improvement > _backslide_improvement))) {
					/*printf("error_improvement=%f, target_improvement=%f, backslide_improvement=%f,
					  stagnation=%d\n", error_improvement, target_improvement, backslide_improvement, stagnation);
					  */
					_target_improvement = error_improvement * (1.0f + CascadeOutputChangeFraction);
					_backslide_improvement = error_improvement * (1.0f - CascadeOutputChangeFraction);
					_stagnation = i + CascadeOutputStagnationEpochs;
				}
				// No improvement in allotted period, so quit
				if(i >= _stagnation && i >= _min_epochs) {
					return (i + 1);
				}
			}
		}
		return _max_epochs;
	}
}

//float fann_train_outputs_epoch(Fann * ann, Fann::TrainData * data)
float Fann::TrainOutputsEpoch(const Fann::TrainData * pData)
{
	ResetMSE();
	for(uint i = 0; i < pData->GetCount(); i++) {
		Run((const float *)pData->InpL.at(i)->dataPtr());
		ComputeMSE((const float *)pData->OutL.at(i)->dataPtr());
		UpdateSlopesBatch(P_LastLayer-1, P_LastLayer-1);
	}
	switch(TrainingAlgorithm) {
		case FANN_TRAIN_RPROP:
		    UpdateWeightsIrpropm((P_LastLayer-1)->P_FirstNeuron->FirstCon, TotalConnections);
		    break;
		case FANN_TRAIN_SARPROP:
		    UpdateWeightsSarprop(SarpropEpoch, (P_LastLayer-1)->P_FirstNeuron->FirstCon, TotalConnections);
		    ++(SarpropEpoch);
		    break;
		case FANN_TRAIN_QUICKPROP:
		    UpdateWeightsQuickprop(pData->GetCount(), (P_LastLayer-1)->P_FirstNeuron->FirstCon, TotalConnections);
		    break;
		case FANN_TRAIN_BATCH:
		case FANN_TRAIN_INCREMENTAL:
		    fann_error(&Err, SLERR_FANN_CANT_USE_TRAIN_ALG);
	}
	return GetMSE();
}

//int fann_reallocate_connections(Fann * ann, uint totalConnections)
int Fann::ReallocateConnections(uint totalConnections)
{
	int   ok = 1;
	// The connections are allocated, but the pointers inside are
	// first moved in the end of the cascade training session.
	THROW(PP_Connections = (Fann::Neuron**)SAlloc::R(PP_Connections, totalConnections * sizeof(Fann::Neuron *)));
	THROW(P_Weights = (ANNTYP*)SAlloc::R(P_Weights, totalConnections * sizeof(ANNTYP)));
	THROW(P_TrainSlopes = (ANNTYP*)SAlloc::R(P_TrainSlopes, totalConnections * sizeof(ANNTYP)));
	THROW(P_PrevSteps = (ANNTYP*)SAlloc::R(P_PrevSteps, totalConnections * sizeof(ANNTYP)));
	THROW(P_PrevTrainSlopes = (ANNTYP*)SAlloc::R(P_PrevTrainSlopes, totalConnections * sizeof(ANNTYP)));
	TotalConnectionsAllocated = totalConnections;
	CATCHZOK
	return ok;
}

//int fann_reallocate_neurons(Fann * pAnn, uint totalNeurons)
int Fann::ReallocateNeurons(uint totalNeurons)
{
	int    ok = 1;
	Fann::Neuron * p_neurons = (Fann::Neuron*)SAlloc::R(P_FirstLayer->P_FirstNeuron, totalNeurons * sizeof(Fann::Neuron));
	TotalNeuronsAllocated = totalNeurons;
	THROW(p_neurons);
	// Also allocate room for more train_errors
	THROW(P_TrainErrors = (ANNTYP*)SAlloc::R(P_TrainErrors, totalNeurons * sizeof(ANNTYP)));
	if(p_neurons != P_FirstLayer->P_FirstNeuron) {
		// Then the memory has moved, also move the pointers
		// Move pointers from layers to neurons
		uint   _num_neurons_so_far = 0;
		for(Fann::Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
			uint _num_neurons = p_layer_it->GetCount();
			p_layer_it->P_FirstNeuron = p_neurons + _num_neurons_so_far;
			p_layer_it->P_LastNeuron = p_layer_it->P_FirstNeuron + _num_neurons;
			_num_neurons_so_far += _num_neurons;
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL Fann::DesiredErrorReached(float desired_error) const
{
	int   yes = 0;
	switch(TrainStopFunction) {
		case FANN_STOPFUNC_MSE:
			if(GetMSE() <= desired_error)
				yes = 1;
			break;
		case FANN_STOPFUNC_BIT:
			if(NumBitFail <= (uint)desired_error)
				yes = 1;
			break;
	}
	return yes;
}

//void initialize_candidate_weights(Fann * ann, uint first_con, uint last_con, float scale_factor)
void Fann::InitializeCandidateWeights(uint firstCon, uint lastCon, float scaleFactor)
{
	const uint _bias_weight = (firstCon + P_FirstLayer->GetCount() - 1);
	ANNTYP _prev_step = (TrainingAlgorithm == FANN_TRAIN_RPROP) ? RpropDeltaZero : 0;
	for(uint i = firstCon; i < lastCon; i++) {
		if(i == _bias_weight)
			P_Weights[i] = fann_rand(-scaleFactor, scaleFactor);
		else
			P_Weights[i] = fann_rand(0, scaleFactor);
		P_TrainSlopes[i] = 0;
		P_PrevSteps[i] = _prev_step;
		P_PrevTrainSlopes[i] = 0;
	}
}

//int fann_initialize_candidates(Fann * ann)
int Fann::InitializeCandidates()
{
	int    ok = 1;
	// The candidates are allocated after the normal neurons and connections,
	// but there is an empty place between the real neurons and the candidate neurons,
	// so that it will be possible to make room when the chosen candidate are copied in on the desired place.
	uint   _neurons_to_allocate;
	uint   _connections_to_allocate;
	const uint  _num_candidates = GetCascadeNumCandidates();
	const uint  _num_neurons = TotalNeurons + _num_candidates + 1;
	const uint  _num_hidden_neurons = TotalNeurons - NumInput - NumOutput;
	const uint  _candidate_connections_in = TotalNeurons - NumOutput;
	const uint  _candidate_connections_out = NumOutput;
	//
	// the number of connections going into a and out of a candidate is ann->TotalNeurons
	//
	const  uint _num_connections = TotalConnections + (TotalNeurons * (_num_candidates + 1));
	const  uint _first_candidate_connection = TotalConnections + TotalNeurons;
	const  uint _first_candidate_neuron = TotalNeurons + 1;
	// First make sure that there is enough room, and if not then allocate a
	// bit more so that we do not need to allocate more room each time.
	if(_num_neurons > TotalNeuronsAllocated) {
		// Then we need to allocate more neurons
		// Allocate half as many neurons as already exist (at least ten)
		_neurons_to_allocate = _num_neurons + _num_neurons / 2;
		SETMAX(_neurons_to_allocate, _num_neurons + 10);
		THROW(ReallocateNeurons(_neurons_to_allocate));
	}
	if(_num_connections > TotalConnectionsAllocated) {
		// Then we need to allocate more connections. Allocate half as many connections as already exist (at least enough for ten neurons)
		_connections_to_allocate = _num_connections + _num_connections / 2;
		if(_connections_to_allocate < (_num_connections + TotalNeurons * 10))
			_connections_to_allocate = _num_connections + TotalNeurons * 10;
		THROW(ReallocateConnections(_connections_to_allocate));
	}
	{
		// Some code to do semi Widrow + Nguyen initialization
		float  _scale_factor = (float)(2.0 * pow(0.7f * (float)_num_hidden_neurons, 1.0f / (float)NumInput));
		if(_scale_factor > 8)
			_scale_factor = 8;
		else if(_scale_factor < 0.5)
			_scale_factor = 0.5;
		{
			//
			// Set the neurons
			//
			uint connection_it = _first_candidate_connection;
			Fann::Neuron * p_neurons = P_FirstLayer->P_FirstNeuron;
			uint candidate_index = _first_candidate_neuron;
			for(uint i = 0; i < CascadeActivationFuncList.getCount(); i++) {
				for(uint j = 0; j < CascadeActivationSteepnessesList.getCount(); j++) {
					const ANNTYP steepness = CascadeActivationSteepnessesList[j];
					for(uint k = 0; k < CascadeNumCandidateGroups; k++) {
						// TODO candidates should actually be created both in
						// the last layer before the output layer, and in a new layer.
						p_neurons[candidate_index].Value = 0;
						p_neurons[candidate_index].Sum = 0;
						p_neurons[candidate_index].ActivationFunction = (ActivationFunc)CascadeActivationFuncList.get(i);
						p_neurons[candidate_index].ActivationSteepness = steepness; //CascadeActivationSteepnesses[j];
						p_neurons[candidate_index].FirstCon = connection_it;
						connection_it += _candidate_connections_in;
						p_neurons[candidate_index].LastCon = connection_it;
						// We have no specific pointers to the output weights, but they are available after last_con
						connection_it += _candidate_connections_out;
						P_TrainErrors[candidate_index] = 0;
						InitializeCandidateWeights(p_neurons[candidate_index].FirstCon, p_neurons[candidate_index].LastCon+_candidate_connections_out, _scale_factor);
						candidate_index++;
					}
				}
			}
		}
	}
	//
	// Now randomize the weights and zero out the arrays that needs zeroing out.
	//
	/*
	   #ifdef CASCADE_DEBUG_FULL
	   printf("random cand weight [%d ... %d]\n", first_candidate_connection, num_connections - 1);
	   #endif
	   for(i = first_candidate_connection; i < num_connections; i++) {
	       //P_Weights[i] = fann_random_weight();
	       P_Weights[i] = fann_rand(-2.0,2.0);
	       P_TrainSlopes[i] = 0;
	       P_PrevSteps[i] = 0;
	       P_PrevTrainSlopes[i] = initial_slope;
	   }
	 */
	CATCHZOK
	return ok;
}

//int fann_train_candidates(Fann * ann, Fann::TrainData * pData)
int Fann::TrainCandidates(const Fann::TrainData * pData)
{
	ANNTYP _best_cand_score = 0.0;
	ANNTYP _target_cand_score = 0.0;
	ANNTYP _backslide_cand_score = -1.0e20f;
	const uint _max_epochs = CascadeMaxCandEpochs;
	const uint _min_epochs = CascadeMinCandEpochs;
	uint _stagnation = _max_epochs;
	if(P_CascadeCandidateScores == NULL) {
		P_CascadeCandidateScores = (ANNTYP*)SAlloc::M(GetCascadeNumCandidates() * sizeof(ANNTYP));
		if(P_CascadeCandidateScores == NULL) {
			fann_error(&Err, SLERR_FANN_CANT_ALLOCATE_MEM);
			return 0;
		}
	}
	for(uint i = 0; i < _max_epochs; i++) {
		_best_cand_score = TrainCandidatesEpoch(pData);
		if((_best_cand_score / MSE_value) > CascadeCandidateLimit) {
			return i + 1;
		}
		else {
			if((_best_cand_score > _target_cand_score) || (_best_cand_score < _backslide_cand_score)) {
				_target_cand_score    = _best_cand_score * (1.0f + CascadeCandidateChangeFraction);
				_backslide_cand_score = _best_cand_score * (1.0f - CascadeCandidateChangeFraction);
				_stagnation = i + CascadeCandidateStagnationEpochs;
			}
			// No improvement in allotted period, so quit
			if(i >= _stagnation && i >= _min_epochs) {
				return i + 1;
			}
		}
	}
	return _max_epochs;
}

//void fann_update_candidate_slopes(Fann * ann)
void Fann::UpdateCandidateSlopes()
{
	Fann::Neuron * p_neurons = P_FirstLayer->P_FirstNeuron;
	Fann::Neuron * p_first_cand = p_neurons + TotalNeurons + 1;
	Fann::Neuron * p_last_cand = p_first_cand + GetCascadeNumCandidates();
	const uint _num_output = NumOutput;
	const ANNTYP * p_output_train_errors = P_TrainErrors + (TotalNeurons - NumOutput);
	for(Fann::Neuron * p_cand_it = p_first_cand; p_cand_it < p_last_cand; p_cand_it++) {
		ANNTYP cand_score = P_CascadeCandidateScores[p_cand_it - p_first_cand];
		// code more or less stolen from fann_run to fast forward pass
		ANNTYP cand_sum = 0.0;
		const uint num_connections = p_cand_it->GetConCount();
		const ANNTYP * p_weights = P_Weights + p_cand_it->FirstCon;
		{
			// unrolled loop start
			uint i = num_connections & 3; // same as modulo 4
			switch(i) {
				case 3: cand_sum += p_weights[2] * p_neurons[2].Value;
				case 2: cand_sum += p_weights[1] * p_neurons[1].Value;
				case 1: cand_sum += p_weights[0] * p_neurons[0].Value;
				case 0: break;
			}
			for(; i != num_connections; i += 4) {
				cand_sum +=
					p_weights[i]   * p_neurons[i].Value +
					p_weights[i+1] * p_neurons[i+1].Value +
					p_weights[i+2] * p_neurons[i+2].Value +
					p_weights[i+3] * p_neurons[i+3].Value;
			}
			/*for(i = 0; i < num_connections; i++) {
				cand_sum += p_weights[i] * p_neurons[i].value;
			}*/
			// unrolled loop end
		}
		{
			const ANNTYP _max_sum = 150.0f/p_cand_it->ActivationSteepness;
			if(cand_sum > _max_sum)
				cand_sum = _max_sum;
			else if(cand_sum < -_max_sum)
				cand_sum = -_max_sum;
		}
		{
			ANNTYP _error_value = 0.0;
			const ANNTYP activation = Activation(p_cand_it->ActivationFunction, p_cand_it->ActivationSteepness, cand_sum);
			// printf("%f = sigmoid(%f);\n", activation, cand_sum);
			p_cand_it->Sum = cand_sum;
			p_cand_it->Value = activation;
			const ANNTYP derived = p_cand_it->ActivationDerived(activation, cand_sum);
			// The output weights is located right after the input weights in the weight array.
			const ANNTYP * p_cand_out_weights = p_weights + num_connections;
			ANNTYP * p_cand_out_slopes = P_TrainSlopes + p_cand_it->FirstCon + num_connections;
			for(uint j = 0; j < _num_output; j++) {
				const ANNTYP diff = (activation * p_cand_out_weights[j]) - p_output_train_errors[j];
				p_cand_out_slopes[j] -= 2.0f * diff * activation;
				_error_value += diff * p_cand_out_weights[j];
				cand_score -= (diff * diff);
			}
			P_CascadeCandidateScores[p_cand_it-p_first_cand] = cand_score;
			_error_value *= derived;
			{
				ANNTYP * p_cand_slopes = P_TrainSlopes + p_cand_it->FirstCon;
				for(uint cidx = 0; cidx < num_connections; cidx++)
					p_cand_slopes[cidx] -= _error_value * p_neurons[cidx].Value;
			}
		}
	}
}

//void fann_update_candidate_weights(Fann * ann, uint numData)
void Fann::UpdateCandidateWeights(uint numData)
{
	const Fann::Neuron * p_first_cand = (P_LastLayer-1)->P_LastNeuron + 1; // there is an empty neuron between the actual neurons and the candidate neuron
	const Fann::Neuron * p_last_cand = p_first_cand + GetCascadeNumCandidates() - 1;
	switch(TrainingAlgorithm) {
		case FANN_TRAIN_RPROP:
		    UpdateWeightsIrpropm(p_first_cand->FirstCon, p_last_cand->LastCon + NumOutput);
		    break;
		case FANN_TRAIN_SARPROP:
		    // TODO: increase epoch?
		    UpdateWeightsSarprop(SarpropEpoch, p_first_cand->FirstCon, p_last_cand->LastCon + NumOutput);
		    break;
		case FANN_TRAIN_QUICKPROP:
		    UpdateWeightsQuickprop(numData, p_first_cand->FirstCon, p_last_cand->LastCon + NumOutput);
		    break;
		case FANN_TRAIN_BATCH:
		case FANN_TRAIN_INCREMENTAL:
		    fann_error(&Err, SLERR_FANN_CANT_USE_TRAIN_ALG);
		    break;
	}
}

//ANNTYP fann_train_candidates_epoch(Fann * ann, Fann::TrainData * pData)
ANNTYP Fann::TrainCandidatesEpoch(const Fann::TrainData * pData)
{
	ANNTYP _best_score = 0;
	uint   i;
	const  uint _num_cand = GetCascadeNumCandidates();
	ANNTYP * p_output_train_errors = P_TrainErrors + (TotalNeurons - NumOutput);
	Fann::Neuron * p_output_neurons = (P_LastLayer-1)->P_FirstNeuron;
	for(i = 0; i < _num_cand; i++) {
		// The ann->MSE_value is actually the sum squared error
		P_CascadeCandidateScores[i] = MSE_value;
	}
	for(i = 0; i < pData->GetCount(); i++) {
		const DataVector * p_inp_vect = pData->InpL.at(i);
		const DataVector * p_out_vect = pData->OutL.at(i);
		Run((const float *)p_inp_vect->dataPtr());
		for(uint j = 0; j < NumOutput; j++) {
			/* TODO only debug, but the error is in opposite direction, this might be usefull info */
			/*          if(output_train_errors[j] != (ann->output[j] - data->output[i][j])){
			 * printf("difference in calculated error at %f != %f; %f = %f - %f;\n", output_train_errors[j],
			 *(ann->output[j] - data->output[i][j]), output_train_errors[j], ann->output[j],
			 *data->output[i][j]);
			 * } */

			/*
			 * output_train_errors[j] = (data->output[i][j] - ann->output[j])/2;
			 * output_train_errors[j] = ann->output[j] - data->output[i][j];
			 */
			p_output_train_errors[j] = (p_out_vect->at(j) - P_Output[j]);
			switch(p_output_neurons[j].ActivationFunction) {
				case FANN_LINEAR_PIECE_SYMMETRIC:
				case FANN_SIGMOID_SYMMETRIC:
				case FANN_SIGMOID_SYMMETRIC_STEPWISE:
				case FANN_THRESHOLD_SYMMETRIC:
				case FANN_ELLIOT_SYMMETRIC:
				case FANN_GAUSSIAN_SYMMETRIC:
				case FANN_SIN_SYMMETRIC:
				case FANN_COS_SYMMETRIC:
				    p_output_train_errors[j] /= 2.0;
				    break;
				case FANN_LINEAR:
				case FANN_THRESHOLD:
				case FANN_SIGMOID:
				case FANN_SIGMOID_STEPWISE:
				case FANN_GAUSSIAN:
				case FANN_GAUSSIAN_STEPWISE:
				case FANN_ELLIOT:
				case FANN_LINEAR_PIECE:
				case FANN_SIN:
				case FANN_COS:
				    break;
			}
		}
		UpdateCandidateSlopes();
	}
	UpdateCandidateWeights(pData->GetCount());
	{
		// find the best candidate score
		uint   _best_candidate = 0;
		_best_score = P_CascadeCandidateScores[_best_candidate];
		for(i = 1; i < _num_cand; i++) {
			// Fann::Neuron *cand = ann->P_FirstLayer->P_FirstNeuron + ann->TotalNeurons + 1 + i;
			// printf("candidate[%d] = activation: %s, steepness: %f, score: %f\n",
			// i, FANN_ACTIVATIONFUNC_NAMES[cand->activation_function],
			// cand->ActivationSteepness, ann->P_CascadeCandidateScores[i]);
			if(P_CascadeCandidateScores[i] > _best_score) {
				_best_candidate = i;
				_best_score = P_CascadeCandidateScores[_best_candidate];
			}
		}
		CascadeBestCandidate = TotalNeurons + _best_candidate + 1;
	}
	return _best_score;
}
//
// add a layer at the position pointed to by *layer
//
//Fann::Layer * fann_add_layer(Fann * ann, Fann::Layer * pLayer)
Fann::Layer * Fann::AddLayer(Fann::Layer * pLayer)
{
	Fann::Layer * p_result = 0;
	const int _layer_pos = (int)(pLayer - P_FirstLayer);
	const int _num_layers = (int)(GetNumLayers() + 1);
	// allocate the layer
	Fann::Layer * p_layers = (Fann::Layer*)SAlloc::R(P_FirstLayer, _num_layers * sizeof(Fann::Layer));
	THROW(p_layers);
	// copy layers so that the free space is at the right location
	for(int i = _num_layers - 1; i >= _layer_pos; i--) {
		p_layers[i] = p_layers[i-1];
	}
	// the newly allocated layer is empty
	p_layers[_layer_pos].P_FirstNeuron = p_layers[_layer_pos+1].P_FirstNeuron;
	p_layers[_layer_pos].P_LastNeuron = p_layers[_layer_pos+1].P_FirstNeuron;
	// Set the ann pointers correctly
	P_FirstLayer = p_layers;
	P_LastLayer = p_layers + _num_layers;
	p_result = (p_layers + _layer_pos);
	CATCH
		p_result = 0;
	ENDCATCH
	return p_result;
}

//void fann_set_shortcut_connections(Fann * ann)
void Fann::SetShortcutConnections()
{
	uint   _num_connections = 0;
	Fann::Neuron ** pp_neuron_pointers = PP_Connections;
	Fann::Neuron * p_neurons = P_FirstLayer->P_FirstNeuron;
	for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != P_LastLayer; p_layer_it++) {
		for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_layer_it->P_LastNeuron; p_neuron_it++) {
			pp_neuron_pointers += _num_connections;
			_num_connections = p_neuron_it->GetConCount();
			for(uint i = 0; i != _num_connections; i++) {
				pp_neuron_pointers[i] = p_neurons + i;
			}
		}
	}
}

//void fann_add_candidate_neuron(Fann * ann, Fann::Layer * pLayer)
void Fann::AddCandidateNeuron(Fann::Layer * pLayer)
{
	const uint num_connections_in = (uint)(pLayer->P_FirstNeuron - P_FirstLayer->P_FirstNeuron);
	const uint num_connections_out = (uint)((P_LastLayer-1)->P_LastNeuron - (pLayer+1)->P_FirstNeuron);
	uint   num_connections_move = num_connections_out + num_connections_in;
	uint   candidate_con;
	uint   candidate_output_weight;
	Fann::Neuron * p_neuron_place;
	Fann::Neuron * p_candidate;
	// We know that there is enough room for the new neuron
	// (the candidates are in the same arrays), so move
	// the last neurons to make room for this neuron.
	//
	// first move the pointers to neurons in the layer structs
	{
		for(Fann::Layer * p_layer_it = P_LastLayer-1; p_layer_it != pLayer; p_layer_it--) {
			p_layer_it->P_FirstNeuron++;
			p_layer_it->P_LastNeuron++;
		}
	}
	// also move the last neuron in the layer that needs the neuron added
	pLayer->P_LastNeuron++;
	// this is the place that should hold the new neuron
	p_neuron_place = pLayer->P_LastNeuron - 1;
	p_candidate = P_FirstLayer->P_FirstNeuron + CascadeBestCandidate;
	// the output weights for the candidates are located after the input weights
	candidate_output_weight = p_candidate->LastCon;
	{
		// move the actual output neurons and the indexes to the connection arrays
		for(Fann::Neuron * p_neuron_it = (P_LastLayer-1)->P_LastNeuron - 1; p_neuron_it != p_neuron_place; p_neuron_it--) {
			*p_neuron_it = *(p_neuron_it-1);
			// move the weights
			for(int i = p_neuron_it->LastCon-1; i >= (int)p_neuron_it->FirstCon; i--) {
				P_Weights[i+num_connections_move-1] = P_Weights[i];
			}
			// move the indexes to weights
			p_neuron_it->LastCon += num_connections_move;
			num_connections_move--;
			p_neuron_it->FirstCon += num_connections_move;
			// set the new weight to the newly allocated neuron
			P_Weights[p_neuron_it->LastCon-1] = (P_Weights[candidate_output_weight]) * CascadeWeightMultiplier;
			candidate_output_weight++;
		}
	}
	// Now inititalize the actual neuron
	p_neuron_place->Value = 0;
	p_neuron_place->Sum = 0;
	p_neuron_place->ActivationFunction = p_candidate->ActivationFunction;
	p_neuron_place->ActivationSteepness = p_candidate->ActivationSteepness;
	p_neuron_place->LastCon = (p_neuron_place + 1)->FirstCon;
	p_neuron_place->FirstCon = p_neuron_place->LastCon - num_connections_in;
	candidate_con = p_candidate->FirstCon;
	// initialize the input weights at random
	{
		for(uint i = 0; i < num_connections_in; i++) {
			P_Weights[i+p_neuron_place->FirstCon] = P_Weights[i+candidate_con];
		}
	}
	// Change some of main variables
	TotalNeurons++;
	TotalConnections += num_connections_in + num_connections_out;
}

#endif /* FIXEDFANN */

/*FANN_EXTERNAL uint FANN_API fann_get_cascade_num_candidates(const Fann * pAnn)
{
	return pAnn->GetCascadeNumCandidates();
}*/

FANN_EXTERNAL void FANN_API fann_set_cascade_activation_functions(Fann * ann, const Fann::ActivationFunc * cascade_activation_functions, uint cascade_activation_functions_count)
{
	/*
	if(ann->cascade_activation_functions_count != cascade_activation_functions_count) {
		ann->cascade_activation_functions_count = cascade_activation_functions_count;
		// reallocate mem
		ann->P_CascadeActivationFunctions = (fann_activationfunc_enum*)SAlloc::R(ann->P_CascadeActivationFunctions,
			ann->cascade_activation_functions_count * sizeof(fann_activationfunc_enum));
		if(ann->P_CascadeActivationFunctions == NULL) {
			fann_error(&ann->Err, SLERR_FANN_CANT_ALLOCATE_MEM);
			return;
		}
	}
	memmove(ann->P_CascadeActivationFunctions, cascade_activation_functions, ann->cascade_activation_functions_count * sizeof(fann_activationfunc_enum));
	*/
	ann->SetCascadeActivationFunctions(cascade_activation_functions, cascade_activation_functions_count);
}

void Fann::SetCascadeActivationFunctions(const Fann::ActivationFunc * pCascadeActivationFunctions, uint cascadeActivationFunctionsCount)
{
	/*
	if(ann->cascade_activation_functions_count != cascade_activation_functions_count) {
		ann->cascade_activation_functions_count = cascade_activation_functions_count;
		// reallocate mem
		ann->P_CascadeActivationFunctions = (fann_activationfunc_enum*)SAlloc::R(ann->P_CascadeActivationFunctions,
			ann->cascade_activation_functions_count * sizeof(fann_activationfunc_enum));
		if(ann->P_CascadeActivationFunctions == NULL) {
			fann_error(&ann->Err, SLERR_FANN_CANT_ALLOCATE_MEM);
			return;
		}
	}
	memmove(ann->P_CascadeActivationFunctions, cascade_activation_functions, ann->cascade_activation_functions_count * sizeof(fann_activationfunc_enum));
	*/
	CascadeActivationFuncList.clear();
	for(uint i = 0; i < cascadeActivationFunctionsCount; i++) {
		CascadeActivationFuncList.add(pCascadeActivationFunctions[i]);
	}
}

//FANN_GET(uint, cascade_activation_steepnesses_count)
//FANN_GET(ANNTYP *, cascade_activation_steepnesses)

/*FANN_EXTERNAL void FANN_API fann_set_cascade_activation_steepnesses(Fann * ann, ANNTYP * cascade_activation_steepnesses, uint cascade_activation_steepnesses_count)
{
	if(ann->cascade_activation_steepnesses_count != cascade_activation_steepnesses_count) {
		ann->cascade_activation_steepnesses_count = cascade_activation_steepnesses_count;
		// reallocate mem
		ann->CascadeActivationSteepnesses = (ANNTYP*)SAlloc::R(ann->CascadeActivationSteepnesses, ann->cascade_activation_steepnesses_count * sizeof(ANNTYP));
		if(ann->CascadeActivationSteepnesses == NULL) {
			fann_error(&ann->Err, SLERR_FANN_CANT_ALLOCATE_MEM);
			return;
		}
	}
	memmove(ann->CascadeActivationSteepnesses, cascade_activation_steepnesses, ann->cascade_activation_steepnesses_count * sizeof(ANNTYP));
}*/

void Fann::SetCascadeActivationSteepnesses(const FloatArray & rList)
{
	CascadeActivationSteepnessesList = rList;
}
//
// FANN_ERROR
//
//
// resets the last error number
//
FANN_EXTERNAL void FANN_API fann_reset_errno(FannError * errdat)
{
	errdat->errno_f = SLERR_SUCCESS;
}
//
// resets the last errstr
//
FANN_EXTERNAL void FANN_API fann_reset_errstr(FannError * pErr)
{
	if(pErr)
		pErr->Msg = 0;
}
//
// returns the last error number
 //
FANN_EXTERNAL int FANN_API fann_get_errno(const FannError * pErr)
{
	return pErr->errno_f;
}
//
// returns the last errstr
//
FANN_EXTERNAL const char * FANN_API fann_get_errstr(const FannError * pErr)
{
	return pErr ? pErr->Msg.cptr() : 0;
}
//
// change where errors are logged to
//
/*FANN_EXTERNAL void FANN_API fann_set_error_log(FannError * errdat, FILE * log_file)
{
	if(errdat == NULL)
		fann_default_error_log = log_file;
	else
		errdat->error_log = log_file;
}*/

/* prints the last error to stderr
 */
FANN_EXTERNAL void FANN_API fann_print_error(FannError * errdat)
{
	if(errdat->errno_f && errdat->Msg.NotEmpty()) {
		fprintf(stderr, "FANN Error %d: %s", errdat->errno_f, errdat->Msg.cptr());
	}
}
//
// INTERNAL FUNCTION
// Populate the error information
//
void fann_error(FannError * errdat, int errno_f, ...)
{
	va_list ap;
	size_t errstr_max = FANN_ERRSTR_MAX + PATH_MAX - 1;
	char   errstr[FANN_ERRSTR_MAX + PATH_MAX];
	FILE * error_log = 0; // fann_default_error_log;
	if(errdat)
		errdat->errno_f = errno_f;
	va_start(ap, errno_f);
	switch(errno_f) {
		case SLERR_SUCCESS:
		    return;
		case SLERR_FANN_CANT_OPEN_CONFIG_R:
		    vsnprintf(errstr, errstr_max, "Unable to open configuration file \"%s\" for reading.\n", ap);
		    break;
		case SLERR_FANN_CANT_OPEN_CONFIG_W:
		    vsnprintf(errstr, errstr_max, "Unable to open configuration file \"%s\" for writing.\n", ap);
		    break;
		case SLERR_FANN_WRONG_CONFIG_VERSION:
		    vsnprintf(errstr, errstr_max, "Wrong version of configuration file, aborting read of configuration file \"%s\".\n", ap);
		    break;
		case SLERR_FANN_CANT_READ_CONFIG:
		    vsnprintf(errstr, errstr_max, "Error reading \"%s\" from configuration file \"%s\".\n", ap);
		    break;
		case SLERR_FANN_CANT_READ_NEURON:
		    vsnprintf(errstr, errstr_max, "Error reading neuron info from configuration file \"%s\".\n", ap);
		    break;
		case SLERR_FANN_CANT_READ_CONNECTIONS:
		    vsnprintf(errstr, errstr_max, "Error reading connections from configuration file \"%s\".\n", ap);
		    break;
		case SLERR_FANN_WRONG_NUM_CONNECTIONS:
		    vsnprintf(errstr, errstr_max, "ERROR connections_so_far=%d, total_connections=%d\n", ap);
		    break;
		case SLERR_FANN_CANT_OPEN_TD_W:
		    vsnprintf(errstr, errstr_max, "Unable to open train data file \"%s\" for writing.\n", ap);
		    break;
		case SLERR_FANN_CANT_OPEN_TD_R:
		    vsnprintf(errstr, errstr_max, "Unable to open train data file \"%s\" for writing.\n", ap);
		    break;
		case SLERR_FANN_CANT_READ_TD:
		    vsnprintf(errstr, errstr_max, "Error reading info from train data file \"%s\", line: %d.\n", ap);
		    break;
		case SLERR_NOMEM:
		case SLERR_FANN_CANT_ALLOCATE_MEM:
		    STRNSCPY(errstr, "Unable to allocate memory.\n");
		    break;
		case SLERR_FANN_CANT_TRAIN_ACTIVATION:
		    STRNSCPY(errstr, "Unable to train with the selected activation function.\n");
		    break;
		case SLERR_FANN_CANT_USE_ACTIVATION:
		    STRNSCPY(errstr, "Unable to use the selected activation function.\n");
		    break;
		case SLERR_FANN_TRAIN_DATA_MISMATCH:
		    STRNSCPY(errstr, "Training data must be of equivalent structure.\n");
		    break;
		case SLERR_FANN_CANT_USE_TRAIN_ALG:
		    STRNSCPY(errstr, "Unable to use the selected training algorithm.\n");
		    break;
		case SLERR_FANN_TRAIN_DATA_SUBSET:
		    vsnprintf(errstr, errstr_max, "Subset from %d of length %d not valid in training set of length %d.\n", ap);
		    break;
		case SLERR_FANN_INDEX_OUT_OF_BOUND:
		    vsnprintf(errstr, errstr_max, "Index %d is out of bound.\n", ap);
		    break;
		case SLERR_FANN_SCALE_NOT_PRESENT:
		    strcpy(errstr, "Scaling parameters not present.\n");
		    break;
		case SLERR_FANN_INPUT_NO_MATCH:
		    vsnprintf(errstr, errstr_max, "The number of input neurons in the ann (%d) and data (%d) don't match\n", ap);
		    break;
		case SLERR_FANN_OUTPUT_NO_MATCH:
		    vsnprintf(errstr, errstr_max, "The number of output neurons in the ann (%d) and data (%d) don't match\n", ap);
		    break;
		case SLERR_FANN_WRONG_PARAMETERS_FOR_CREATE:
		    strcpy(errstr, "The parameters for create_standard are wrong, either too few parameters provided or a negative/very high value provided.\n");
		    break;
	}
	va_end(ap);
	if(errdat) {
		errdat->Msg = errstr;
		//error_log = errdat->error_log;
	}
	/*if(error_log == (FILE*)-1) { // This is the default behavior and will give stderr
		fprintf(stderr, "FANN Error %d: %s", errno_f, errstr);
	}
	else*/
	if(error_log && ((int)error_log) != -1)
		fprintf(error_log, "FANN Error %d: %s", errno_f, errstr);
}
//
// INTERNAL FUNCTION
// Initialize an error data strcuture
//
/*void fann_init_error_data(FannError * errdat)
{
	errdat->Msg = 0;
	errdat->errno_f = SLERR_SUCCESS;
	//errdat->error_log = fann_default_error_log;
}*/
//
// FANN_IO
//
//
// Create a network from a configuration file.
//
/*FANN_EXTERNAL Fann * FANN_API fann_create_from_file(const char * configuration_file)
{
	Fann * ann = 0;
	FILE * conf = fopen(configuration_file, "r");
	if(!conf) {
		fann_error(NULL, SLERR_FANN_CANT_OPEN_CONFIG_R, configuration_file);
	}
	else {
		ann = fann_create_from_fd(conf, configuration_file);
		fclose(conf);
	}
	return ann;
}*/
//
// Save the network.
//
FANN_EXTERNAL int FANN_API fann_save(Fann * ann, const char * configuration_file)
{
	return fann_save_internal(ann, configuration_file, 0);
}
//
// Save the network as fixed point data.
//
FANN_EXTERNAL int FANN_API fann_save_to_fixed(Fann * ann, const char * configuration_file)
{
	return fann_save_internal(ann, configuration_file, 1);
}
//
// INTERNAL FUNCTION
// Used to save the network to a file.
//
int fann_save_internal(Fann * ann, const char * configuration_file, uint save_as_fixed)
{
	FILE * conf = fopen(configuration_file, "w+");
	if(!conf) {
		fann_error(&ann->Err, SLERR_FANN_CANT_OPEN_CONFIG_W, configuration_file);
		return -1;
	}
	else {
		int retval = fann_save_internal_fd(ann, conf, configuration_file, save_as_fixed);
		fclose(conf);
		return retval;
	}
}

void Fann::ScaleSave(FILE * pF, uint c, const float * pList, const char * pField) const
{
	/*
	#define SCALE_SAVE(what, where) fprintf(conf, # what "_" # where "="); \
		for(i = 0; i < ann->num_ ## where ## put; i++)						  \
			fprintf(conf, "%f ", ann->what ## _ ## where[ i ]);				  \
		fprintf(conf, "\n");
	*/
	fprintf(pF, "%s=", pField);
	for(uint i = 0; i < c; i++)
		fprintf(pF, "%f ", pList[i]);
	fprintf(pF, "\n");
}

int Fann::ScaleLoad(FILE * pF, uint c, float * pList, const char * pField)
{
	/*
#define SCALE_LOAD(what, where)											      \
	fann_skip(# what "_" # where "=");									\
	for(i = 0; i < ann->num_ ## where ## put; i++) { \
		if(fscanf(conf, "%f ", (float*)&ann->what ## _ ## where[ i ]) != 1) { \
			fann_error(&ann->Err, SLERR_FANN_CANT_READ_CONFIG, # what "_" # where, configuration_file); \
			fann_destroy(ann);												\
			return NULL;													\
		}																	\
	}
#define fann_skip(name)	{ \
		if(fscanf(conf, name) != 0) { \
			fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, name, configuration_file); \
			fann_destroy(ann); \
			return NULL; \
		}}
	*/
	int    ok = 1;
	SString temp_buf;
	char   _buf[256];
	(temp_buf = pField).CatChar('=');
	STRNSCPY(_buf, temp_buf);
	THROW(fscanf(pF, _buf) == 0);
	for(uint i = 0; i < c; i++) {
		THROW(fscanf(pF, "%f ", &pList[i]) == 1);
	}
	CATCH
		fann_error(&Err, SLERR_FANN_CANT_READ_CONFIG, pField, ""/*configuration_file*/);
		ok = 0;
	ENDCATCH
	return ok;
}

Fann::StorageHeader::StorageHeader()
{
	THISZERO();
	Sign[0] = 'S';
	Sign[1] = 'F';
	Sign[2] = 'N';
	Sign[3] = 'N';
	Ver = 1;
#ifdef FIXEDFANN
	Type = 3;
#else
	if(sizeof(ANNTYP) == 4)
		Type = 1;
	else if(sizeof(ANNTYP) == 8)
		Type = 2;
	else
		Type = 0; // error
#endif
}

int Fann::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	/*struct StorageHeader {
		StorageHeader();

		uint8  Sign[4]; //
		uint32 Crc32;
		uint32 Ver;  //
		uint32 Type; // 1 - float, 2 - double, 3 - fixed (int32)
		uint8  Reserve[48];
	};*/
	StorageHeader pattern;
	StorageHeader hdr;
	if(dir < 0) {
		THROW(pSCtx->SerializeBlock(dir, sizeof(StorageHeader), &hdr, rBuf, 0));
		THROW(memcmp(pattern.Sign, hdr.Sign, sizeof(pattern.Sign)) == 0);
		THROW(hdr.Ver >= 1);
		THROW(hdr.Type == pattern.Type);
	}
	else if(dir > 0) {
		THROW(pSCtx->SerializeBlock(dir, sizeof(StorageHeader), &hdr, rBuf, 0));
	}
	THROW(pSCtx->Serialize(dir, NetworkType, rBuf));
	THROW(pSCtx->Serialize(dir, &Layers, rBuf));
	THROW(pSCtx->Serialize(dir, NumInput, rBuf));
	THROW(pSCtx->Serialize(dir, NumOutput, rBuf));
    THROW(pSCtx->Serialize(dir, LearningRate, rBuf));
	THROW(pSCtx->Serialize(dir, LearningMomentum, rBuf));
	THROW(pSCtx->Serialize(dir, ConnectionRate, rBuf));
	THROW(pSCtx->Serialize(dir, TotalNeurons, rBuf));
	THROW(pSCtx->Serialize(dir, TrainingAlgorithm, rBuf));

	THROW(pSCtx->Serialize(dir, DecimalPoint, rBuf));
	THROW(pSCtx->Serialize(dir, Multiplier, rBuf));
	THROW(pSCtx->SerializeBlock(dir, sizeof(SigmoidResults), SigmoidResults, rBuf, 0));
	THROW(pSCtx->SerializeBlock(dir, sizeof(SigmoidValues), SigmoidValues, rBuf, 0));
	THROW(pSCtx->SerializeBlock(dir, sizeof(SigmoidSymmetricResults), SigmoidSymmetricResults, rBuf, 0));
	THROW(pSCtx->SerializeBlock(dir, sizeof(SigmoidSymmetricValues), SigmoidSymmetricValues, rBuf, 0));

	THROW(pSCtx->Serialize(dir, TotalConnections, rBuf));
	THROW(pSCtx->Serialize(dir, num_MSE, rBuf));
	THROW(pSCtx->Serialize(dir, MSE_value, rBuf));
	THROW(pSCtx->Serialize(dir, NumBitFail, rBuf));
	THROW(pSCtx->Serialize(dir, BitFailLimit, rBuf));
	THROW(pSCtx->Serialize(dir, TrainErrorFunction, rBuf));
	THROW(pSCtx->Serialize(dir, TrainStopFunction, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeOutputChangeFraction, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeOutputStagnationEpochs, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeCandidateChangeFraction, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeCandidateStagnationEpochs, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeBestCandidate, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeCandidateLimit, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeWeightMultiplier, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeMaxOutEpochs, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeMaxCandEpochs, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeMinOutEpochs, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeMinCandEpochs, rBuf));
	THROW(pSCtx->Serialize(dir, &CascadeActivationFuncList, rBuf));
	THROW(pSCtx->Serialize(dir, &CascadeActivationSteepnessesList, rBuf));
	THROW(pSCtx->Serialize(dir, CascadeNumCandidateGroups, rBuf));
	THROW(pSCtx->Serialize(dir, TotalNeuronsAllocated, rBuf));
	THROW(pSCtx->Serialize(dir, TotalConnectionsAllocated, rBuf));
	THROW(pSCtx->Serialize(dir, QuickpropDecay, rBuf));
	THROW(pSCtx->Serialize(dir, QuickpropMu, rBuf));
	THROW(pSCtx->Serialize(dir, RpropIncreaseFactor, rBuf));
	THROW(pSCtx->Serialize(dir, RpropDecreaseFactor, rBuf));
	THROW(pSCtx->Serialize(dir, RpropDeltaMin, rBuf));
	THROW(pSCtx->Serialize(dir, RpropDeltaMax, rBuf));
	THROW(pSCtx->Serialize(dir, RpropDeltaZero, rBuf));
	THROW(pSCtx->Serialize(dir, SarpropWeightDecayShift, rBuf));
	THROW(pSCtx->Serialize(dir, SarpropStepErrorThresholdFactor, rBuf));
	THROW(pSCtx->Serialize(dir, SarpropStepErrorShift, rBuf));
	THROW(pSCtx->Serialize(dir, SarpropTemperature, rBuf));
	THROW(pSCtx->Serialize(dir, SarpropEpoch, rBuf));
	{
		LongArray layer_sizes;
		if(dir > 0) {
			for(Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
				// the number of neurons in the layers (in the last layer, there is always one too many neurons, because of an unused bias)
				THROW(layer_sizes.add((long)p_layer_it->GetCount()));
			}
			THROW(pSCtx->Serialize(dir, &layer_sizes, rBuf));
		}
		else if(dir < 0) {
			THROW(pSCtx->Serialize(dir, &layer_sizes, rBuf));
			THROW(layer_sizes.getCount() == Layers.getCount());
			THROW(AllocateLayers());
			{
				uint   layer_no = 0;
				uint   _local_total_neurons = 0;
				for(Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
					const long layer_size = layer_sizes.get(layer_no);
					p_layer_it->_Dim = Layers.get(layer_no);
					p_layer_it->P_FirstNeuron = NULL;
					p_layer_it->P_LastNeuron = p_layer_it->P_FirstNeuron + layer_size;
					_local_total_neurons += layer_size;
					layer_no++;
				}
				THROW(TotalNeurons == _local_total_neurons);
			}
		}
	}
	THROW(ScaleIn.Serialize(dir, NumInput, rBuf, pSCtx));
	THROW(ScaleOut.Serialize(dir, NumOutput, rBuf, pSCtx));
	{
		if(dir > 0) {
			for(Layer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
				for(Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_layer_it->P_LastNeuron; p_neuron_it++) {
					uint   _con_count = p_neuron_it->GetConCount();
					THROW(pSCtx->Serialize(dir, _con_count, rBuf));
					THROW(pSCtx->Serialize(dir, p_neuron_it->Sum, rBuf));
					THROW(pSCtx->Serialize(dir, p_neuron_it->Value, rBuf));
					THROW(pSCtx->Serialize(dir, p_neuron_it->ActivationFunction, rBuf));
					THROW(pSCtx->Serialize(dir, p_neuron_it->ActivationSteepness, rBuf));
				}
			}
			//
			for(uint i = 0; i < TotalConnections; i++) {
				// save the connection "(source weight) "
				uint32 nc = (uint32)(PP_Connections[i] - P_FirstLayer->P_FirstNeuron);
				THROW(pSCtx->Serialize(dir, nc, rBuf));
				THROW(pSCtx->Serialize(dir, P_Weights[i], rBuf));
			}
		}
		else if(dir < 0) {
			uint   _local_total_conn = 0;
			THROW(AllocateNeurons());
			for(Neuron * p_neuron_it = P_FirstLayer->P_FirstNeuron; p_neuron_it != (P_LastLayer-1)->P_LastNeuron; p_neuron_it++) {
				uint   _con_count = 0;
				THROW(pSCtx->Serialize(dir, _con_count, rBuf));
				THROW(pSCtx->Serialize(dir, p_neuron_it->Sum, rBuf));
				THROW(pSCtx->Serialize(dir, p_neuron_it->Value, rBuf));
				THROW(pSCtx->Serialize(dir, p_neuron_it->ActivationFunction, rBuf));
				THROW(pSCtx->Serialize(dir, p_neuron_it->ActivationSteepness, rBuf));
				p_neuron_it->FirstCon = _local_total_conn;
				_local_total_conn += _con_count;
				p_neuron_it->LastCon = _local_total_conn;
			}
			THROW(TotalConnections == _local_total_conn);
			//
			THROW(AllocateConnections());
			for(uint i = 0; i < TotalConnections; i++) {
				uint32 nc = 0;
				THROW(pSCtx->Serialize(dir, nc, rBuf));
				THROW(pSCtx->Serialize(dir, P_Weights[i], rBuf));
				PP_Connections[i] = P_FirstLayer->P_FirstNeuron + nc;
			}
		}
	}
	{
		/*
			THROW((P_TrainSlopes && rS.P_TrainSlopes) || (!P_TrainSlopes && !rS.P_TrainSlopes));
			THROW((P_PrevSteps && rS.P_PrevSteps) || (!P_PrevSteps && !rS.P_PrevSteps));
			THROW((P_PrevTrainSlopes && rS.P_PrevTrainSlopes) || (!P_PrevTrainSlopes && !rS.P_PrevTrainSlopes));
			THROW((P_PrevWeightsDeltas && rS.P_PrevWeightsDeltas) || (!P_PrevWeightsDeltas && !rS.P_PrevWeightsDeltas));
			for(uint i = 0; i < TotalConnectionsAllocated; i++) {
				THROW(!P_TrainSlopes       || P_TrainSlopes[i] == rS.P_TrainSlopes[i]);
				THROW(!P_PrevSteps         || P_PrevSteps[i] == rS.P_PrevSteps[i]);
				THROW(!P_PrevTrainSlopes   || P_PrevTrainSlopes[i] == rS.P_PrevTrainSlopes[i]);
				THROW(!P_PrevWeightsDeltas || P_PrevWeightsDeltas[i] == rS.P_PrevWeightsDeltas[i]);
			}
		*/
		uint32 _pc = 0;
		if(dir > 0) {
			_pc = P_TrainSlopes ? TotalConnectionsAllocated : 0;
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_TrainSlopes[0]), P_TrainSlopes, rBuf, 0));
			}
			//
			_pc = P_PrevSteps ? TotalConnectionsAllocated : 0;
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_PrevSteps[0]), P_PrevSteps, rBuf, 0));
			}
			//
			_pc = P_PrevTrainSlopes ? TotalConnectionsAllocated : 0;
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_PrevTrainSlopes[0]), P_PrevTrainSlopes, rBuf, 0));
			}
			//
			_pc = P_PrevWeightsDeltas ? TotalConnectionsAllocated : 0;
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_PrevWeightsDeltas[0]), P_PrevWeightsDeltas, rBuf, 0));
			}
		}
		else if(dir < 0) {
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				ZDELETE(P_TrainSlopes);
				THROW(P_TrainSlopes = (ANNTYP *)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_TrainSlopes[0]), P_TrainSlopes, rBuf, 0));
			}
			//
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				ZDELETE(P_PrevSteps);
				THROW(P_PrevSteps = (ANNTYP *)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_PrevSteps[0]), P_PrevSteps, rBuf, 0));
			}
			//
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				ZDELETE(P_PrevTrainSlopes);
				THROW(P_PrevTrainSlopes = (ANNTYP *)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_PrevTrainSlopes[0]), P_PrevTrainSlopes, rBuf, 0));
			}
			//
			THROW(pSCtx->Serialize(dir, _pc, rBuf));
			if(_pc) {
				ZDELETE(P_PrevWeightsDeltas);
				THROW(P_PrevWeightsDeltas = (ANNTYP *)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
				THROW(pSCtx->SerializeBlock(dir, _pc * sizeof(P_PrevWeightsDeltas[0]), P_PrevWeightsDeltas, rBuf, 0));
			}
		}
	}
	CATCHZOK
	return ok;
}

//
// INTERNAL FUNCTION
// Used to save the network to a file descriptor.
//
int fann_save_internal_fd(Fann * ann, FILE * conf, const char * configuration_file, uint save_as_fixed)
{
	Fann::Layer * layer_it;
	int calculated_decimal_point = 0;
	Fann::Neuron * neuron_it, * first_neuron;
	ANNTYP * weights;
	Fann::Neuron ** connected_neurons;
	uint i = 0;
#ifndef FIXEDFANN
	// variabels for use when saving floats as fixed point variabels
	uint   decimal_point = 0;
	uint   fixed_multiplier = 0;
	ANNTYP max_possible_value = 0;
	uint   bits_used_for_max = 0;
	ANNTYP current_max_value = 0;
#endif
	// save the version information
#ifndef FIXEDFANN
	if(save_as_fixed)
		fprintf(conf, FANN_FIX_VERSION "\n");
	else
		fprintf(conf, FANN_FLO_VERSION "\n");
#else
	fprintf(conf, FANN_FIX_VERSION "\n");
#endif
#ifndef FIXEDFANN
	if(save_as_fixed) {
		// calculate the maximal possible shift value
		for(layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer; layer_it++) {
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
				// look at all connections to each neurons, and see how high a value we can get
				current_max_value = 0;
				for(i = neuron_it->FirstCon; i != neuron_it->LastCon; i++) {
					current_max_value += fann_abs(ann->P_Weights[i]);
				}
				if(current_max_value > max_possible_value) {
					max_possible_value = current_max_value;
				}
			}
		}
		for(bits_used_for_max = 0; max_possible_value >= 1; bits_used_for_max++) {
			max_possible_value /= 2.0;
		}
		//
		// The maximum number of bits we shift the fix point, is the number
		// of bits in a integer, minus one for the sign, one for the minus
		// in stepwise, and minus the bits used for the maximum.
		// This is devided by two, to allow multiplication of two fixed point numbers.
		//
		calculated_decimal_point = (sizeof(int) * 8 - 2 - bits_used_for_max) / 2;
		decimal_point = (calculated_decimal_point < 0) ? 0 : calculated_decimal_point;
		fixed_multiplier = 1 << decimal_point;
		fprintf(conf, "decimal_point=%u\n", decimal_point); // save the decimal_point on a seperate line
	}
#else
	// save the decimal_point on a seperate line
	fprintf(conf, "decimal_point=%u\n", ann->DecimalPoint);
#endif
	// Save network parameters
	fprintf(conf, "num_layers=%d\n", (int)ann->GetNumLayers());
	fprintf(conf, "learning_rate=%f\n", ann->LearningRate);
	fprintf(conf, "connection_rate=%f\n", ann->ConnectionRate);
	fprintf(conf, "network_type=%u\n", ann->NetworkType);
	fprintf(conf, "learning_momentum=%f\n", ann->LearningMomentum);
	fprintf(conf, "training_algorithm=%u\n", ann->TrainingAlgorithm);
	fprintf(conf, "train_error_function=%u\n", ann->TrainErrorFunction);
	fprintf(conf, "train_stop_function=%u\n", ann->TrainStopFunction);
	fprintf(conf, "cascade_output_change_fraction=%f\n", ann->CascadeOutputChangeFraction);
	fprintf(conf, "quickprop_decay=%f\n", ann->QuickpropDecay);
	fprintf(conf, "quickprop_mu=%f\n", ann->QuickpropMu);
	fprintf(conf, "rprop_increase_factor=%f\n", ann->RpropIncreaseFactor);
	fprintf(conf, "rprop_decrease_factor=%f\n", ann->RpropDecreaseFactor);
	fprintf(conf, "rprop_delta_min=%f\n", ann->RpropDeltaMin);
	fprintf(conf, "rprop_delta_max=%f\n", ann->RpropDeltaMax);
	fprintf(conf, "rprop_delta_zero=%f\n", ann->RpropDeltaZero);
	fprintf(conf, "cascade_output_stagnation_epochs=%u\n", ann->CascadeOutputStagnationEpochs);
	fprintf(conf, "cascade_candidate_change_fraction=%f\n", ann->CascadeCandidateChangeFraction);
	fprintf(conf, "cascade_candidate_stagnation_epochs=%u\n", ann->CascadeCandidateStagnationEpochs);
	fprintf(conf, "cascade_max_out_epochs=%u\n", ann->CascadeMaxOutEpochs);
	fprintf(conf, "cascade_min_out_epochs=%u\n", ann->CascadeMinOutEpochs);
	fprintf(conf, "cascade_max_cand_epochs=%u\n", ann->CascadeMaxCandEpochs);
	fprintf(conf, "cascade_min_cand_epochs=%u\n", ann->CascadeMinCandEpochs);
	fprintf(conf, "cascade_num_candidate_groups=%u\n", ann->CascadeNumCandidateGroups);
#ifndef FIXEDFANN
	if(save_as_fixed) {
		fprintf(conf, "bit_fail_limit=%u\n", (int)floor((ann->BitFailLimit * fixed_multiplier) + 0.5));
		fprintf(conf, "cascade_candidate_limit=%u\n", (int)floor((ann->CascadeCandidateLimit * fixed_multiplier) + 0.5));
		fprintf(conf, "cascade_weight_multiplier=%u\n", (int)floor((ann->CascadeWeightMultiplier * fixed_multiplier) + 0.5));
	}
	else
#endif
	{
		fprintf(conf, "bit_fail_limit=\"FANNPRINTF \"\n", ann->BitFailLimit);
		fprintf(conf, "cascade_candidate_limit=\"FANNPRINTF \"\n", ann->CascadeCandidateLimit);
		fprintf(conf, "cascade_weight_multiplier=\"FANNPRINTF \"\n", ann->CascadeWeightMultiplier);
	}
	fprintf(conf, "cascade_activation_functions_count=%u\n", /*ann->cascade_activation_functions_count*/ann->CascadeActivationFuncList.getCount());
	fprintf(conf, "cascade_activation_functions=");
	for(i = 0; i < ann->CascadeActivationFuncList.getCount(); i++)
		fprintf(conf, "%u ", /*ann->P_CascadeActivationFunctions[i]*/ann->CascadeActivationFuncList.get(i));
	fprintf(conf, "\n");
	fprintf(conf, "cascade_activation_steepnesses_count=%u\n", /*ann->cascade_activation_steepnesses_count*/ann->CascadeActivationSteepnessesList.getCount());
	fprintf(conf, "cascade_activation_steepnesses=");
	for(i = 0; i < ann->CascadeActivationSteepnessesList.getCount(); i++) {
/*
#ifndef FIXEDFANN
		if(save_as_fixed)
			fprintf(conf, "%u ", (int)floor((ann->CascadeActivationSteepnesses[i] * fixed_multiplier) + 0.5));
		else
#endif
*/
		fprintf(conf, FANNPRINTF " ", ann->CascadeActivationSteepnessesList[i]);
	}
	fprintf(conf, "\n");
	fprintf(conf, "layer_sizes=");
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		// the number of neurons in the layers (in the last layer, there is always one too many neurons, because of an unused bias)
		fprintf(conf, "%d ", (int)layer_it->GetCount());
	}
	fprintf(conf, "\n");
#ifndef FIXEDFANN
	/* 2.1 */
	/*
	#define SCALE_SAVE(what, where) fprintf(conf, # what "_" # where "="); \
		for(i = 0; i < ann->num_ ## where ## put; i++)						  \
			fprintf(conf, "%f ", ann->what ## _ ## where[ i ]);				  \
		fprintf(conf, "\n");
	*/
	if(!save_as_fixed) {
		if(ann->ScaleIn.IsPresent()) {
			fprintf(conf, "scale_included=1\n");
			ann->ScaleIn.Save(conf, ann->NumInput, "in");
			ann->ScaleOut.Save(conf, ann->NumOutput, "out");
			/*SCALE_SAVE(scale_mean,         in)
			SCALE_SAVE(scale_deviation,    in)
			SCALE_SAVE(scale_new_min,      in)
			SCALE_SAVE(scale_factor,       in)
			SCALE_SAVE(scale_mean,         out)
			SCALE_SAVE(scale_deviation,    out)
			SCALE_SAVE(scale_new_min,      out)
			SCALE_SAVE(scale_factor,       out)*/
		}
		else
			fprintf(conf, "scale_included=0\n");
	}
//#undef SCALE_SAVE
#endif
	/* 2.0 */
	fprintf(conf, "neurons (num_inputs, activation_function, activation_steepness)=");
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		/* the neurons */
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
#ifndef FIXEDFANN
			if(save_as_fixed) {
				fprintf(conf, "(%u, %u, %u) ", neuron_it->GetConCount(), neuron_it->ActivationFunction, (int)floor((neuron_it->ActivationSteepness * fixed_multiplier) + 0.5));
			}
			else {
				fprintf(conf, "(%u, %u, " FANNPRINTF ") ", neuron_it->GetConCount(), neuron_it->ActivationFunction, neuron_it->ActivationSteepness);
			}
#else
			fprintf(conf, "(%u, %u, " FANNPRINTF ") ", neuron_it->GetConCount(), neuron_it->ActivationFunction, neuron_it->ActivationSteepness);
#endif
		}
	}
	fprintf(conf, "\n");
	connected_neurons = ann->PP_Connections;
	weights = ann->P_Weights;
	first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	//
	// Now save all the connections.
	// We only need to save the source and the weight, since the destination is given by the order.
	//
	// The weight is not saved binary due to differences in binary definition of floating point numbers.
	// Especially an iPAQ does not use the same binary representation as an i386 machine.
	//
	fprintf(conf, "connections (connected_to_neuron, weight)=");
	for(i = 0; i < ann->TotalConnections; i++) {
		// save the connection "(source weight) "
#ifndef FIXEDFANN
		if(save_as_fixed) {
			fprintf(conf, "(%d, %d) ", (int)(connected_neurons[i] - first_neuron), (int)floor((weights[i] * fixed_multiplier) + 0.5));
		}
		else {
			fprintf(conf, "(%d, " FANNPRINTF ") ", (int)(connected_neurons[i] - first_neuron), weights[i]);
		}
#else
		fprintf(conf, "(%d, " FANNPRINTF ") ", (int)(connected_neurons[i] - first_neuron), weights[i]);
#endif
	}
	fprintf(conf, "\n");
	return calculated_decimal_point;
}
//
// INTERNAL FUNCTION
// Create a network from a configuration file descriptor.
//
#if 0 // {
Fann * fann_create_from_fd(FILE * conf, const char * configuration_file)
{
	uint num_layers, layer_size, input_neuron, i, num_connections;
	uint tmpVal;
#ifdef FIXEDFANN
	uint decimal_point, multiplier;
#else
	uint scale_included;
#endif
	Fann::Neuron * p_first_neuron;
	Fann::Neuron * neuron_it;
	Fann::Neuron * last_neuron;
	Fann::Neuron ** connected_neurons;
	ANNTYP * weights;
	Fann::Layer * layer_it;
	Fann * ann = NULL;
	char * read_version = (char *)SAlloc::C(strlen(FANN_CONF_VERSION "\n"), 1);
	if(read_version == NULL) {
		fann_error(NULL, SLERR_FANN_CANT_ALLOCATE_MEM);
		return NULL;
	}
	if(fread(read_version, 1, strlen(FANN_CONF_VERSION "\n"), conf) == 1) {
		fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, "FANN_VERSION", configuration_file);
		return NULL;
	}
	// compares the version information
	if(strncmp(read_version, FANN_CONF_VERSION "\n", strlen(FANN_CONF_VERSION "\n")) != 0) {
#ifdef FIXEDFANN
		if(strncmp(read_version, "FANN_FIX_1.1\n", strlen("FANN_FIX_1.1\n")) == 0) {
#else
		if(strncmp(read_version, "FANN_FLO_1.1\n", strlen("FANN_FLO_1.1\n")) == 0) {
#endif
			SAlloc::F(read_version);
			return 0/*fann_create_from_fd_1_1(conf, configuration_file)*/;
		}

#ifndef FIXEDFANN
		// Maintain compatibility with 2.0 version that doesnt have scale parameters
		if(strncmp(read_version, "FANN_FLO_2.0\n", strlen("FANN_FLO_2.0\n")) != 0 &&
		    strncmp(read_version, "FANN_FLO_2.1\n", strlen("FANN_FLO_2.1\n")) != 0)
#else
		if(strncmp(read_version, "FANN_FIX_2.0\n", strlen("FANN_FIX_2.0\n")) != 0 &&
		    strncmp(read_version, "FANN_FIX_2.1\n", strlen("FANN_FIX_2.1\n")) != 0)
#endif
		{
			SAlloc::F(read_version);
			fann_error(NULL, SLERR_FANN_WRONG_CONFIG_VERSION, configuration_file);
			return NULL;
		}
	}
	SAlloc::F(read_version);
#ifdef FIXEDFANN
	fann_scanf("%u", "decimal_point", &decimal_point);
	multiplier = 1 << decimal_point;
#endif
	fann_scanf("%u", "num_layers", &num_layers);
	THROW(ann = fann_allocate_structure(num_layers));
	fann_scanf("%f", "learning_rate", &ann->LearningRate);
	fann_scanf("%f", "connection_rate", &ann->ConnectionRate);
	fann_scanf("%u", "network_type", &tmpVal);
	ann->NetworkType = (Fann::NetType)tmpVal;
	fann_scanf("%f", "learning_momentum", &ann->LearningMomentum);
	fann_scanf("%u", "training_algorithm", &tmpVal);
	ann->TrainingAlgorithm = (Fann::TrainAlg)tmpVal;
	fann_scanf("%u", "train_error_function", &tmpVal);
	ann->TrainErrorFunction = (Fann::ErrorFunc)tmpVal;
	fann_scanf("%u", "train_stop_function", &tmpVal);
	ann->TrainStopFunction = (Fann::StopFunc)tmpVal;
	fann_scanf("%f", "cascade_output_change_fraction", &ann->CascadeOutputChangeFraction);
	fann_scanf("%f", "quickprop_decay", &ann->QuickpropDecay);
	fann_scanf("%f", "quickprop_mu", &ann->QuickpropMu);
	fann_scanf("%f", "rprop_increase_factor", &ann->RpropIncreaseFactor);
	fann_scanf("%f", "rprop_decrease_factor", &ann->RpropDecreaseFactor);
	fann_scanf("%f", "rprop_delta_min", &ann->RpropDeltaMin);
	fann_scanf("%f", "rprop_delta_max", &ann->RpropDeltaMax);
	fann_scanf("%f", "rprop_delta_zero", &ann->RpropDeltaZero);
	fann_scanf("%u", "cascade_output_stagnation_epochs", &ann->CascadeOutputStagnationEpochs);
	fann_scanf("%f", "cascade_candidate_change_fraction", &ann->CascadeCandidateChangeFraction);
	fann_scanf("%u", "cascade_candidate_stagnation_epochs", &ann->CascadeCandidateStagnationEpochs);
	fann_scanf("%u", "cascade_max_out_epochs", &ann->CascadeMaxOutEpochs);
	fann_scanf("%u", "cascade_min_out_epochs", &ann->CascadeMinOutEpochs);
	fann_scanf("%u", "cascade_max_cand_epochs", &ann->CascadeMaxCandEpochs);
	fann_scanf("%u", "cascade_min_cand_epochs", &ann->CascadeMinCandEpochs);
	fann_scanf("%u", "cascade_num_candidate_groups", &ann->CascadeNumCandidateGroups);
	fann_scanf(FANNSCANF, "bit_fail_limit", &ann->BitFailLimit);
	fann_scanf(FANNSCANF, "cascade_candidate_limit", &ann->CascadeCandidateLimit);
	fann_scanf(FANNSCANF, "cascade_weight_multiplier", &ann->CascadeWeightMultiplier);
	{
		ann->CascadeActivationFuncList.clear();
		uint   cafc = 0;
		fann_scanf("%u", "cascade_activation_functions_count", &cafc);
		fann_skip("cascade_activation_functions=");
		for(i = 0; i < cafc; i++) {
			if(fscanf(conf, "%u ", &tmpVal) != 1) {
				fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, "cascade_activation_functions", configuration_file);
				fann_destroy(ann);
				return NULL;
			}
			//ann->P_CascadeActivationFunctions[i] = (fann_activationfunc_enum)tmpVal;
			ann->CascadeActivationFuncList.add(tmpVal);
		}
		/*
		fann_scanf("%u", "cascade_activation_functions_count", &ann->cascade_activation_functions_count);
		// reallocate mem
		ann->P_CascadeActivationFunctions = (fann_activationfunc_enum*)SAlloc::R(ann->P_CascadeActivationFunctions,
			ann->cascade_activation_functions_count * sizeof(fann_activationfunc_enum));
		if(ann->P_CascadeActivationFunctions == NULL) {
			fann_error(&ann->Err, SLERR_FANN_CANT_ALLOCATE_MEM);
			fann_destroy(ann);
			return NULL;
		}
		fann_skip("cascade_activation_functions=");
		for(i = 0; i < ann->cascade_activation_functions_count; i++) {
			if(fscanf(conf, "%u ", &tmpVal) != 1) {
				fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, "cascade_activation_functions", configuration_file);
				fann_destroy(ann);
				return NULL;
			}
			ann->P_CascadeActivationFunctions[i] = (fann_activationfunc_enum)tmpVal;
		}
		*/
	}
	{
		ann->CascadeActivationSteepnessesList.clear();
		uint   casc = 0;
		fann_scanf("%u", "cascade_activation_steepnesses_count", &casc);
		fann_skip("cascade_activation_steepnesses=");
		for(i = 0; i < casc; i++) {
			float fval;
			if(fscanf(conf, "%f ", &fval) != 1) {
				fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, "cascade_activation_steepnesses", configuration_file);
				fann_destroy(ann);
				return NULL;
			}
			else {
				ann->CascadeActivationSteepnessesList.add(fval);
			}
		}
		/*
		fann_scanf("%u", "cascade_activation_steepnesses_count", &ann->cascade_activation_steepnesses_count);
		// reallocate mem
		ann->CascadeActivationSteepnesses = (ANNTYP*)SAlloc::R(ann->CascadeActivationSteepnesses, ann->cascade_activation_steepnesses_count * sizeof(ANNTYP));
		if(ann->CascadeActivationSteepnesses == NULL) {
			fann_error(&ann->Err, SLERR_FANN_CANT_ALLOCATE_MEM);
			fann_destroy(ann);
			return NULL;
		}
		fann_skip("cascade_activation_steepnesses=");
		for(i = 0; i < ann->cascade_activation_steepnesses_count; i++) {
			if(fscanf(conf, FANNSCANF " ", &ann->CascadeActivationSteepnesses[i]) != 1) {
				fann_error(NULL, SLERR_FANN_CANT_READ_CONFIG, "cascade_activation_steepnesses", configuration_file);
				fann_destroy(ann);
				return NULL;
			}
		}
		*/
	}
#ifdef FIXEDFANN
	ann->DecimalPoint = decimal_point;
	ann->Multiplier = multiplier;
#endif
#ifdef FIXEDFANN
	ann->UpdateStepwise();
#endif
	fann_skip("layer_sizes=");
	// determine how many neurons there should be in each layer
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		if(fscanf(conf, "%u ", &layer_size) != 1) {
			fann_error(&ann->Err, SLERR_FANN_CANT_READ_CONFIG, "layer_sizes", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		// we do not allocate room here, but we make sure that last_neuron - first_neuron is the number of neurons
		layer_it->P_FirstNeuron = NULL;
		layer_it->P_LastNeuron = layer_it->P_FirstNeuron + layer_size;
		ann->TotalNeurons += layer_size;
	}
	ann->NumInput = (uint)(ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron - 1);
	ann->NumOutput = (uint)((ann->P_LastLayer-1)->P_LastNeuron - (ann->P_LastLayer-1)->P_FirstNeuron);
	if(ann->NetworkType == Fann::FANN_NETTYPE_LAYER) {
		ann->NumOutput--; // one too many (bias) in the output layer
	}
#ifndef FIXEDFANN
	if(fscanf(conf, "scale_included=%u\n", &scale_included) == 1 && scale_included == 1) {
		ann->AllocateScale();
		THROW(ann->ScaleIn.Load(conf, ann->NumInput, "in"));
		THROW(ann->ScaleOut.Load(conf, ann->NumOutput, "out"));
	}
#undef SCALE_LOAD
#endif
	// allocate room for the actual neurons
	THROW(ann->AllocateNeurons());
	last_neuron = (ann->P_LastLayer-1)->P_LastNeuron;
	fann_skip("neurons (num_inputs, activation_function, activation_steepness)=");
	for(neuron_it = ann->P_FirstLayer->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
		THROW_S_S(fscanf(conf, "(%u, %u, " FANNSCANF ") ", &num_connections, &tmpVal, &neuron_it->ActivationSteepness) == 3,
			SLERR_FANN_CANT_READ_NEURON, configuration_file);
		neuron_it->ActivationFunction = (Fann::ActivationFunc)tmpVal;
		neuron_it->FirstCon = ann->TotalConnections;
		ann->TotalConnections += num_connections;
		neuron_it->LastCon = ann->TotalConnections;
	}
	THROW(ann->AllocateConnections());
	connected_neurons = ann->PP_Connections;
	weights = ann->P_Weights;
	p_first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	fann_skip("connections (connected_to_neuron, weight)=");
	for(i = 0; i < ann->TotalConnections; i++) {
		THROW_S_S(fscanf(conf, "(%u, " FANNSCANF ") ", &input_neuron, &weights[i]) == 2, SLERR_FANN_CANT_READ_CONNECTIONS, configuration_file);
		connected_neurons[i] = p_first_neuron + input_neuron;
	}
	CATCH
		ZDELETE(ann);
	ENDCATCH
	return ann;
}
#endif // } 0
//
// FANN_TRAIN
//
int FASTCALL Fann::Neuron::IsEqual(const Fann::Neuron & rS) const
{
	int    yes = 1;
#define CMPF(f) THROW(f == rS.f)
	CMPF(FirstCon);
	CMPF(LastCon);
	CMPF(Sum);
	CMPF(Value);
	CMPF(ActivationSteepness);
	CMPF(ActivationFunction);
#undef CMPF
	CATCH
		yes = 0;
	ENDCATCH
	return yes;
}

#ifndef FIXEDFANN
//
// Calculates the derived of a value, given an activation function and a steepness
//
//ANNTYP fann_activation_derived(const Fann::Neuron * pN, ANNTYP value, ANNTYP sum)
ANNTYP Fann::Neuron::ActivationDerived(ANNTYP value, ANNTYP sum) const
{
	switch(ActivationFunction) {
		case Fann::FANN_LINEAR:
		case Fann::FANN_LINEAR_PIECE:
		case Fann::FANN_LINEAR_PIECE_SYMMETRIC:
			return (ANNTYP)fann_linear_derive(ActivationSteepness, value);
		case Fann::FANN_SIGMOID:
		case Fann::FANN_SIGMOID_STEPWISE:
		    value = MINMAX(value, 0.01f, 0.99f);
		    return (ANNTYP)fann_sigmoid_derive(ActivationSteepness, value);
		case Fann::FANN_SIGMOID_SYMMETRIC:
		case Fann::FANN_SIGMOID_SYMMETRIC_STEPWISE:
		    value = MINMAX(value, -0.98f, 0.98f);
		    return (ANNTYP)fann_sigmoid_symmetric_derive(ActivationSteepness, value);
		case Fann::FANN_GAUSSIAN:
		    // value = MINMAX(value, 0.01f, 0.99f);
		    return (ANNTYP)fann_gaussian_derive(ActivationSteepness, value, sum);
		case Fann::FANN_GAUSSIAN_SYMMETRIC:
		    // value = MINMAX(value, -0.98f, 0.98f);
		    return (ANNTYP)fann_gaussian_symmetric_derive(ActivationSteepness, value, sum);
		case Fann::FANN_ELLIOT:
		    value = MINMAX(value, 0.01f, 0.99f);
		    return (ANNTYP)fann_elliot_derive(ActivationSteepness, value, sum);
		case Fann::FANN_ELLIOT_SYMMETRIC:
		    value = MINMAX(value, -0.98f, 0.98f);
		    return (ANNTYP)fann_elliot_symmetric_derive(ActivationSteepness, value, sum);
		case Fann::FANN_SIN_SYMMETRIC:
		    return (ANNTYP)fann_sin_symmetric_derive(ActivationSteepness, sum);
		case Fann::FANN_COS_SYMMETRIC:
		    return (ANNTYP)fann_cos_symmetric_derive(ActivationSteepness, sum);
		case Fann::FANN_SIN:
		    return (ANNTYP)fann_sin_derive(ActivationSteepness, sum);
		case Fann::FANN_COS:
		    return (ANNTYP)fann_cos_derive(ActivationSteepness, sum);
		case Fann::FANN_THRESHOLD:
		    fann_error(NULL, SLERR_FANN_CANT_TRAIN_ACTIVATION);
	}
	return (ANNTYP)0;
}
//
// INTERNAL FUNCTION
// Calculates the activation of a value, given an activation function and a steepness
//
/*ANNTYP fann_activation(Fann * ann, uint activation_function, ANNTYP steepness, ANNTYP value)
{
	value = fann_mult(steepness, value);
	fann_activation_switch(activation_function, value, value);
	return value;
}*/

ANNTYP Fann::Activation(uint activationFunction, ANNTYP steepness, ANNTYP value) const
{
	value = fann_mult(steepness, value);
	fann_activation_switch(activationFunction, value, value);
	return value;
}
//
// Trains the network with the backpropagation algorithm.
//
/*FANN_EXTERNAL void FANN_API fann_train(Fann * pAnn, ANNTYP * pInput, ANNTYP * pDesiredOutput)
{
	pAnn->Train(pInput, pDesiredOutput);
}*/

int Fann::Train(const ANNTYP * pInput, const ANNTYP * pDesiredOutput)
{
	return TrainWithOutput(pInput, pDesiredOutput, 0);
}

int Fann::TrainWithOutput(const ANNTYP * pInput, const ANNTYP * pDesiredOutput, ANNTYP * pResult)
{
	int    ok = 1;
	const ANNTYP * p_result = Run(pInput);
	if(pResult) {
		memcpy(pResult, p_result, NumOutput * sizeof(ANNTYP));
	}
	THROW(ComputeMSE(pDesiredOutput));
	BackpropagateMSE();
	THROW(UpdateWeights());
	CATCHZOK
	return ok;
}

#endif
//
// INTERNAL FUNCTION
// Helper function to update the MSE value and return a diff which takes symmetric functions into account
//
//ANNTYP fann_update_MSE(Fann * ann, Fann::Neuron * pNeuron, ANNTYP neuronDiff)
ANNTYP Fann::UpdateMSE(Fann::Neuron * pNeuron, ANNTYP neuronDiff)
{
	float _neuron_diff2;
	switch(pNeuron->ActivationFunction) {
		case FANN_LINEAR_PIECE_SYMMETRIC:
		case FANN_THRESHOLD_SYMMETRIC:
		case FANN_SIGMOID_SYMMETRIC:
		case FANN_SIGMOID_SYMMETRIC_STEPWISE:
		case FANN_ELLIOT_SYMMETRIC:
		case FANN_GAUSSIAN_SYMMETRIC:
		case FANN_SIN_SYMMETRIC:
		case FANN_COS_SYMMETRIC:
		    neuronDiff /= (ANNTYP)2.0;
		    break;
		case FANN_THRESHOLD:
		case FANN_LINEAR:
		case FANN_SIGMOID:
		case FANN_SIGMOID_STEPWISE:
		case FANN_GAUSSIAN:
		case FANN_GAUSSIAN_STEPWISE:
		case FANN_ELLIOT:
		case FANN_LINEAR_PIECE:
		case FANN_SIN:
		case FANN_COS:
		    break;
	}
#ifdef FIXEDFANN
	_neuron_diff2 = (neuronDiff / (float)Multiplier) * (neuronDiff / (float)Multiplier);
#else
	_neuron_diff2 = (float)(neuronDiff * neuronDiff);
#endif
	MSE_value += _neuron_diff2;
	// printf("neuron_diff %f = (%f - %f)[/2], neuron_diff2=%f, sum=%f, MSE_value=%f, num_MSE=%d\n", neuron_diff, *desired_output, neuron_value, neuron_diff2, last_layer_begin->sum, ann->MSE_value, ann->num_MSE);
	if(fann_abs(neuronDiff) >= BitFailLimit) {
		NumBitFail++;
	}
	return neuronDiff;
}
//
// Tests the network.
//
//FANN_EXTERNAL ANNTYP * FANN_API fann_test(Fann * ann, ANNTYP * pInput, ANNTYP * pDesiredOutput)
/*ANNTYP * Fann::Test_(const ANNTYP * pInput, const ANNTYP * pDesiredOutput)
{
	ANNTYP * p_output_begin = Run(pInput);
	const ANNTYP * p_output_end = p_output_begin + NumOutput;
	Fann::Neuron * p_output_neuron = (P_LastLayer-1)->P_FirstNeuron;
	// calculate the error
	for(ANNTYP * p_output_it = p_output_begin; p_output_it != p_output_end; p_output_it++) {
		const ANNTYP neuron_value = *p_output_it;
		ANNTYP neuron_diff = (*pDesiredOutput - neuron_value);
		neuron_diff = UpdateMSE(p_output_neuron, neuron_diff);
		pDesiredOutput++;
		p_output_neuron++;
		num_MSE++;
	}
	return p_output_begin;
}*/

#ifndef FIXEDFANN

int Fann::ComputeMSE(const ANNTYP * pDesiredOutput)
{
	int    ok = 1;
	ANNTYP * p_error_it = 0;
	ANNTYP * p_error_begin = 0;
	Fann::Neuron * p_last_layer_begin = (P_LastLayer-1)->P_FirstNeuron;
	const Fann::Neuron * p_last_layer_end = p_last_layer_begin + NumOutput;
	const Fann::Neuron * p_first_neuron = P_FirstLayer->P_FirstNeuron;
	// if no room allocated for the error variabels, allocate it now
	if(P_TrainErrors == NULL) {
		THROW(P_TrainErrors = (ANNTYP*)SAlloc::C(TotalNeurons, sizeof(ANNTYP)));
	}
	else {
		// clear the error variabels
		memzero(P_TrainErrors, TotalNeurons * sizeof(ANNTYP));
	}
	p_error_begin = P_TrainErrors;
	// calculate the error and place it in the output layer
	p_error_it = p_error_begin + (p_last_layer_begin - p_first_neuron);
	for(; p_last_layer_begin != p_last_layer_end; p_last_layer_begin++) {
		const ANNTYP neuron_value = p_last_layer_begin->Value;
		ANNTYP neuron_diff = *pDesiredOutput - neuron_value;
		// @todo Если neuron_diff == 0, то нет смысла делать следующие действия, достаточно присвоить *p_error_it = 0
		neuron_diff = UpdateMSE(p_last_layer_begin, neuron_diff);
		if(TrainErrorFunction) { // TODO make switch when more functions
			if(neuron_diff < -0.9999999)
				neuron_diff = -17.0;
			else if(neuron_diff > 0.9999999)
				neuron_diff = 17.0;
			else
				neuron_diff = (ANNTYP)log((1.0 + neuron_diff) / (1.0 - neuron_diff));
		}
		*p_error_it = p_last_layer_begin->ActivationDerived(neuron_value, p_last_layer_begin->Sum) * neuron_diff;
		pDesiredOutput++;
		p_error_it++;
		num_MSE++;
	}
	CATCHZOK
	return ok;
}

void Fann::BackpropagateMSE()
{
	ANNTYP * p_error_begin = P_TrainErrors;
	ANNTYP * p_error_prev_layer;
	const Fann::Neuron * p_first_neuron = P_FirstLayer->P_FirstNeuron;
	const Fann::Layer  * p_second_layer = P_FirstLayer + 1;
	// go through all the layers, from last to first.
	// And propagate the error backwards
	for(Fann::Layer * p_layer_it = P_LastLayer-1; p_layer_it > p_second_layer; --p_layer_it) {
		const Fann::Neuron * p_last_neuron = p_layer_it->P_LastNeuron;
		// for each connection in this layer, propagate the error backwards
		if(ConnectionRate >= 1.0f) {
			if(NetworkType == FANN_NETTYPE_LAYER) {
				p_error_prev_layer = p_error_begin + ((p_layer_it-1)->P_FirstNeuron - p_first_neuron);
			}
			else {
				p_error_prev_layer = p_error_begin;
			}
			for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
				const ANNTYP tmp_error = p_error_begin[p_neuron_it - p_first_neuron];
				const ANNTYP * p_weights = P_Weights + p_neuron_it->FirstCon;
				for(uint i = p_neuron_it->GetConCount(); i--; ) {
					p_error_prev_layer[i] += tmp_error * p_weights[i];
				}
			}
		}
		else {
			for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
				const ANNTYP tmp_error = p_error_begin[p_neuron_it - p_first_neuron];
				const ANNTYP * p_weights = P_Weights + p_neuron_it->FirstCon;
				Fann::Neuron ** pp_connections = PP_Connections + p_neuron_it->FirstCon;
				for(uint i = p_neuron_it->GetConCount(); i--; ) {
					p_error_begin[pp_connections[i] - p_first_neuron] += tmp_error * p_weights[i];
				}
			}
		}
		{
			// then calculate the actual errors in the previous layer
			p_error_prev_layer = p_error_begin + ((p_layer_it-1)->P_FirstNeuron - p_first_neuron);
			p_last_neuron = (p_layer_it - 1)->P_LastNeuron;
			for(Fann::Neuron * p_neuron_it = (p_layer_it-1)->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
				*p_error_prev_layer *= p_neuron_it->ActivationDerived(p_neuron_it->Value, p_neuron_it->Sum);
				p_error_prev_layer++;
			}
		}
	}
}

int Fann::UpdateWeights()
{
	int    ok = 1;
	Fann::Neuron * p_neuron_it;
	Fann::Neuron * p_last_neuron;
	Fann::Neuron * p_prev_neurons;
	Fann::Layer * p_layer_it;
	// store some variabels local for fast access
	const float learning_rate = LearningRate;
	const float learning_momentum = LearningMomentum;
	Fann::Neuron * p_first_neuron = P_FirstLayer->P_FirstNeuron;
	Fann::Layer * p_first_layer = P_FirstLayer;
	const Fann::Layer * p_last_layer = P_LastLayer;
	ANNTYP * p_error_begin = P_TrainErrors;
	ANNTYP * p_deltas_begin;
	// if no room allocated for the deltas, allocate it now
	if(P_PrevWeightsDeltas == NULL) {
		THROW(P_PrevWeightsDeltas = (ANNTYP*)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
	}
	p_deltas_begin = P_PrevWeightsDeltas;
	p_prev_neurons = p_first_neuron;
	for(p_layer_it = (p_first_layer + 1); p_layer_it != p_last_layer; p_layer_it++) {
		p_last_neuron = p_layer_it->P_LastNeuron;
		if(ConnectionRate >= 1) {
			if(NetworkType == FANN_NETTYPE_LAYER) {
				p_prev_neurons = (p_layer_it - 1)->P_FirstNeuron;
			}
			for(p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
				const ANNTYP tmp_error = p_error_begin[p_neuron_it - p_first_neuron] * learning_rate;
				const uint num_connections = p_neuron_it->GetConCount();
				ANNTYP * p_weights = P_Weights + p_neuron_it->FirstCon;
				ANNTYP * p_weights_deltas = p_deltas_begin + p_neuron_it->FirstCon;
				for(uint i = 0; i != num_connections; i++) {
					const ANNTYP delta_w = tmp_error * p_prev_neurons[i].Value + learning_momentum * p_weights_deltas[i];
					p_weights[i] += delta_w;
					p_weights_deltas[i] = delta_w;
				}
			}
		}
		else {
			for(p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
				const ANNTYP tmp_error = p_error_begin[p_neuron_it - p_first_neuron] * learning_rate;
				const uint num_connections = p_neuron_it->GetConCount();
				ANNTYP * p_weights = P_Weights + p_neuron_it->FirstCon;
				ANNTYP * p_weights_deltas = p_deltas_begin + p_neuron_it->FirstCon;
				for(uint i = 0; i != num_connections; i++) {
					const ANNTYP delta_w = tmp_error * p_prev_neurons[i].Value + learning_momentum * p_weights_deltas[i];
					p_weights[i] += delta_w;
					p_weights_deltas[i] = delta_w;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// INTERNAL FUNCTION
// Update slopes for batch training
// layer_begin = ann->P_FirstLayer+1 and layer_end = ann->last_layer-1 will update all slopes.
//
int Fann::UpdateSlopesBatch(const Fann::Layer * pLayerBegin, const Fann::Layer * pLayerEnd)
{
	int    ok = 1;
	// store some variabels local for fast access
	Fann::Neuron * p_first_neuron = P_FirstLayer->P_FirstNeuron;
	const ANNTYP * p_error_begin = P_TrainErrors;
	// if no room allocated for the slope variabels, allocate it now
	if(P_TrainSlopes == NULL) {
		THROW(P_TrainSlopes = (ANNTYP *)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
	}
	SETIFZ(pLayerBegin, (P_FirstLayer + 1));
	SETIFZ(pLayerEnd, (P_LastLayer - 1));
	ANNTYP * p_slope_begin = P_TrainSlopes;
	Fann::Neuron * p_prev_neurons = p_first_neuron;
	for(; pLayerBegin <= pLayerEnd; pLayerBegin++) {
		const Fann::Neuron * p_last_neuron = pLayerBegin->P_LastNeuron;
		if(ConnectionRate >= 1) {
			if(NetworkType == FANN_NETTYPE_LAYER) {
				p_prev_neurons = (pLayerBegin - 1)->P_FirstNeuron;
			}
			for(Fann::Neuron * p_neuron_it = pLayerBegin->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
				const ANNTYP tmp_error = p_error_begin[p_neuron_it - p_first_neuron];
				ANNTYP * p_neuron_slope = p_slope_begin + p_neuron_it->FirstCon;
				const uint cc = p_neuron_it->GetConCount(); // num_connections
				for(uint i = 0; i != cc; i++) {
					p_neuron_slope[i] += tmp_error * p_prev_neurons[i].Value;
				}
			}
		}
		else {
			for(Fann::Neuron * p_neuron_it = pLayerBegin->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
				const ANNTYP tmp_error = p_error_begin[p_neuron_it - p_first_neuron];
				ANNTYP * p_neuron_slope = p_slope_begin + p_neuron_it->FirstCon;
				const uint cc = p_neuron_it->GetConCount(); // num_connections
				Fann::Neuron ** pp_connections = PP_Connections + p_neuron_it->FirstCon;
				for(uint i = 0; i != cc; i++) {
					p_neuron_slope[i] += tmp_error * pp_connections[i]->Value;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// INTERNAL FUNCTION
// Clears arrays used for training before a new training session.
// Also creates the arrays that do not exist yet.
//
//void fann_clear_train_arrays(Fann * ann)
int Fann::ClearTrainArrays()
{
	int    ok = 1;
	// if no room allocated for the slope variabels, allocate it now (calloc clears mem)
	if(P_TrainSlopes == NULL) {
		THROW(P_TrainSlopes = (ANNTYP*)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
	}
	else
		memzero(P_TrainSlopes, TotalConnectionsAllocated * sizeof(ANNTYP));
	// if no room allocated for the variabels, allocate it now
	if(P_PrevSteps == NULL) {
		THROW(P_PrevSteps = (ANNTYP *)SAlloc::M(TotalConnectionsAllocated * sizeof(ANNTYP)));
	}
	if(TrainingAlgorithm == FANN_TRAIN_RPROP) {
		const ANNTYP _delta_zero = RpropDeltaZero;
		for(uint i = 0; i < TotalConnectionsAllocated; i++)
			P_PrevSteps[i] = _delta_zero;
	}
	else
		memzero(P_PrevSteps, TotalConnectionsAllocated * sizeof(ANNTYP));
	// if no room allocated for the variabels, allocate it now
	if(P_PrevTrainSlopes == NULL) {
		THROW(P_PrevTrainSlopes = (ANNTYP *)SAlloc::C(TotalConnectionsAllocated, sizeof(ANNTYP)));
	}
	else
		memzero(P_PrevTrainSlopes, TotalConnectionsAllocated * sizeof(ANNTYP));
	CATCHZOK
	return ok;
}
//
// INTERNAL FUNCTION
// Update weights for batch training
//
//void fann_update_weights_batch(Fann * ann, uint numData, uint firstWeight, uint pastEnd)
void Fann::UpdateWeightsBatch(uint numData, uint firstWeight, uint pastEnd)
{
	ANNTYP * p_train_slopes = P_TrainSlopes;
	ANNTYP * p_weights = P_Weights;
	const float _epsilon = LearningRate / numData;
	for(uint i = firstWeight; i != pastEnd; i++) {
		p_weights[i] += p_train_slopes[i] * _epsilon;
		p_train_slopes[i] = 0.0;
	}
}
//
// INTERNAL FUNCTION
// The quickprop training algorithm
//
//void fann_update_weights_quickprop(Fann * ann, uint numData, uint firstWeight, uint pastEnd)
void Fann::UpdateWeightsQuickprop(uint numData, uint firstWeight, uint pastEnd)
{
	ANNTYP * p_train_slopes = P_TrainSlopes;
	ANNTYP * p_weights = P_Weights;
	ANNTYP * p_prev_steps = P_PrevSteps;
	ANNTYP * p_prev_train_slopes = P_PrevTrainSlopes;
	const float _epsilon = LearningRate / numData;
	const float _decay = QuickpropDecay; // -0.0001
	const float _mu = QuickpropMu; // 1.75
	const float _shrink_factor = (float)(_mu / (1.0 + _mu));
	for(uint i = firstWeight; i != pastEnd; i++) {
		ANNTYP w = p_weights[i];
		const ANNTYP _prev_step = p_prev_steps[i];
		const ANNTYP _slope = p_train_slopes[i] + _decay * w;
		const ANNTYP _prev_slope = p_prev_train_slopes[i];
		ANNTYP _next_step = 0.0;
		// The step must always be in direction opposite to the slope
		if(_prev_step > 0.001) {
			// If last step was positive...
			if(_slope > 0.0) // Add in linear term if current slope is still positive
				_next_step += _epsilon * _slope;
			// If current slope is close to or larger than prev slope...
			if(_slope > (_shrink_factor * _prev_slope))
				_next_step += _mu * _prev_step; // Take maximum size negative step
			else
				_next_step += _prev_step * _slope / (_prev_slope - _slope); // Else, use quadratic estimate
		}
		else if(_prev_step < -0.001) {
			// If last step was negative...
			if(_slope < 0.0) // Add in linear term if current slope is still negative
				_next_step += _epsilon * _slope;
			// If current slope is close to or more neg than prev slope...
			if(_slope < (_shrink_factor * _prev_slope))
				_next_step += _mu * _prev_step; // Take maximum size negative step
			else
				_next_step += _prev_step * _slope / (_prev_slope - _slope); // Else, use quadratic estimate
		}
		else // Last step was zero, so use only linear term
			_next_step += _epsilon * _slope;
		/*
		   if(next_step > 1000 || next_step < -1000)
		   {
		        printf("quickprop[%d] weight=%f, slope=%f, prev_slope=%f, next_step=%f, prev_step=%f\n",
		                   i, weights[i], slope, prev_slope, next_step, prev_step);

		           if(next_step > 1000)
		           next_step = 1000;
		           else
		           next_step = -1000;
		   }
		 */
		//
		// update global data arrays
		//
		p_prev_steps[i] = _next_step;
		w += _next_step;
		if(w > 1500)
			p_weights[i] = 1500;
		else if(w < -1500)
			p_weights[i] = -1500;
		else
			p_weights[i] = w;
		// weights[i] = w;
		p_prev_train_slopes[i] = _slope;
		p_train_slopes[i] = 0.0;
	}
}
//
// INTERNAL FUNCTION
// The iRprop- algorithm
//
//void fann_update_weights_irpropm(Fann * ann, uint firstWeight, uint pastEnd)
void Fann::UpdateWeightsIrpropm(uint firstWeight, uint pastEnd)
{
	ANNTYP * p_train_slopes = P_TrainSlopes;
	ANNTYP * p_weights = P_Weights;
	ANNTYP * p_prev_steps = P_PrevSteps;
	ANNTYP * p_prev_train_slopes = P_PrevTrainSlopes;
	const float _increase_factor = RpropIncreaseFactor;     /*1.2; */
	const float _decrease_factor = RpropDecreaseFactor;     /*0.5; */
	const float _delta_min = RpropDeltaMin; /*0.0; */
	const float _delta_max = RpropDeltaMax; /*50.0; */
	for(uint i = firstWeight; i != pastEnd; i++) {
		const ANNTYP _prev_step = MAX(p_prev_steps[i], (ANNTYP)0.0001); // prev_step may not be zero because then the training will stop
		const ANNTYP _prev_slope = p_prev_train_slopes[i];
		ANNTYP _slope = p_train_slopes[i];
		const ANNTYP _same_sign = _prev_slope * _slope;
		ANNTYP   _next_step;
		if(_same_sign >= 0.0)
			_next_step = MIN(_prev_step * _increase_factor, _delta_max);
		else {
			_next_step = MAX(_prev_step * _decrease_factor, _delta_min);
			_slope = 0;
		}
		if(_slope < 0) {
			p_weights[i] -= _next_step;
			SETMAX(p_weights[i], -1500);
		}
		else {
			p_weights[i] += _next_step;
			SETMIN(p_weights[i], 1500);
		}
		/*if(i == 2) {
			printf("weight=%f, slope=%f, next_step=%f, prev_step=%f\n", weights[i], slope, next_step, prev_step);
		}*/
		// update global data arrays
		p_prev_steps[i] = _next_step;
		p_prev_train_slopes[i] = _slope;
		p_train_slopes[i] = 0.0;
	}
}
//
// INTERNAL FUNCTION
// The SARprop- algorithm
//
//void fann_update_weights_sarprop(Fann * ann, uint epoch, uint firstWeight, uint pastEnd)
void Fann::UpdateWeightsSarprop(uint epoch, uint firstWeight, uint pastEnd)
{
	assert(firstWeight <= pastEnd);
	ANNTYP * p_train_slopes = P_TrainSlopes;
	ANNTYP * p_weights = P_Weights;
	ANNTYP * p_prev_steps = P_PrevSteps;
	ANNTYP * p_prev_train_slopes = P_PrevTrainSlopes;

	// These should be set from variables
	const float _increase_factor = RpropIncreaseFactor;     /*1.2; */
	const float _decrease_factor = RpropDecreaseFactor;     /*0.5; */
	// TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993)
	const float _delta_min = 0.000001f;
	const float _delta_max = RpropDeltaMax; /*50.0; */
	const float _weight_decay_shift = SarpropWeightDecayShift; /* ld 0.01 = -6.644 */
	const float _step_error_threshold_factor = SarpropStepErrorThresholdFactor; /* 0.1 */
	const float _step_error_shift = SarpropStepErrorShift; /* ld 3 = 1.585 */
	const float _T = SarpropTemperature;
	const float _MSE = GetMSE();
	const float _RMSE = sqrtf(_MSE);
	// for all weights; TODO: are biases included?
	for(uint i = firstWeight; i != pastEnd; i++) {
		// TODO: confirm whether 1x10^-6 == delta_min is really better
		const ANNTYP _prev_step = MAX(p_prev_steps[i], (ANNTYP)0.000001); // prev_step may not be zero because then the training will stop
		// calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)
		ANNTYP _slope = -p_train_slopes[i] - p_weights[i] * (ANNTYP)fann_exp2(-_T * epoch + _weight_decay_shift);
		// TODO: is prev_train_slopes[i] 0.0 in the beginning?
		const ANNTYP _prev_slope = p_prev_train_slopes[i];
		const ANNTYP _same_sign = _prev_slope * _slope;
		ANNTYP _next_step = 0;
		if(_same_sign > 0.0) {
			_next_step = MIN(_prev_step * _increase_factor, _delta_max);
			// TODO: are the signs inverted? see differences between SARPROP paper and iRprop
			if(_slope < 0.0)
				p_weights[i] += _next_step;
			else
				p_weights[i] -= _next_step;
		}
		else if(_same_sign < 0.0) {
			if(_prev_step < _step_error_threshold_factor * _MSE)
				_next_step = _prev_step * _decrease_factor + (float)rand() / RAND_MAX * _RMSE * (ANNTYP)fann_exp2(-_T * epoch + _step_error_shift);
			else
				_next_step = MAX(_prev_step * _decrease_factor, _delta_min);
			_slope = 0.0;
		}
		else {
			if(_slope < 0.0)
				p_weights[i] += _prev_step;
			else
				p_weights[i] -= _prev_step;
		}
		// if(i == 2) {
		//  printf("weight=%f, slope=%f, next_step=%f, prev_step=%f\n", weights[i], slope, next_step, prev_step);
		// }
		// update global data arrays
		p_prev_steps[i] = _next_step;
		p_prev_train_slopes[i] = _slope;
		p_train_slopes[i] = 0.0;
	}
}

#endif

//FANN_EXTERNAL void FANN_API fann_set_activation_function_hidden(Fann * ann, fann_activationfunc_enum activation_function)
void Fann::SetActivationFunctionHidden(Fann::ActivationFunc activationFunction)
{
	const Fann::Layer * p_last_layer = P_LastLayer - 1;    /* -1 to not update the output layer */
	for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != p_last_layer; p_layer_it++) {
		const Fann::Neuron * p_last_neuron = p_layer_it->P_LastNeuron;
		for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++)
			p_neuron_it->ActivationFunction = activationFunction;
	}
}

/*FANN_EXTERNAL Fann::Layer * FANN_API fann_get_layer(Fann * ann, int layer)
{
	return ann->GetLayer(layer);
}*/
/*FANN_EXTERNAL Fann::Neuron * FANN_API fann_get_neuron_layer(Fann * ann, Fann::Layer * layer, int neuron)
{
	return ann->GetNeuronLayer(layer, neuron);
}*/
/*FANN_EXTERNAL Fann::Neuron * FANN_API fann_get_neuron(Fann * ann, uint layer, int neuron)
{
	return ann->GetNeuron(layer, neuron);
}*/

//FANN_EXTERNAL Fann::ActivationFunc FANN_API fann_get_activation_function(Fann * ann, int layer, int neuron)
Fann::ActivationFunc Fann::GetActivationFunction(uint layer, int neuron) const
{
	Fann::Neuron * p_neuron_it = GetNeuron(layer, neuron);
	return p_neuron_it ? (Fann::ActivationFunc)p_neuron_it->ActivationFunction : (Fann::ActivationFunc)-1;
}

//FANN_EXTERNAL void FANN_API fann_set_activation_function(Fann * ann, Fann::ActivationFunc activation_function, int layer, int neuron)
int Fann::SetActivationFunction(Fann::ActivationFunc activationFunction, uint layer, int neuron)
{
	int    ok = 1;
	Fann::Neuron * p_neuron_it = GetNeuron(layer, neuron);
	if(p_neuron_it)
		p_neuron_it->ActivationFunction = activationFunction;
	else
		ok = 0;
	return ok;
}

//FANN_EXTERNAL void FANN_API fann_set_activation_function_layer(Fann * ann, Fann::ActivationFunc activation_function, int layer)
void Fann::SetActivationFunctionLayer(Fann::ActivationFunc activationFunction, uint layer)
{
	Fann::Layer * p_layer_it = GetLayer(layer);
	if(p_layer_it) {
		Fann::Neuron * p_last_neuron = p_layer_it->P_LastNeuron;
		for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++)
			p_neuron_it->ActivationFunction = activationFunction;
	}
}

//FANN_EXTERNAL void FANN_API fann_set_activation_function_output(Fann * ann, Fann::ActivationFunc activation_function)
void Fann::SetActivationFunctionOutput(Fann::ActivationFunc activationFunction)
{
	const Fann::Layer  * p_last_layer = P_LastLayer - 1;
	Fann::Neuron * p_last_neuron = p_last_layer->P_LastNeuron;
	for(Fann::Neuron * p_neuron_it = p_last_layer->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++) {
		p_neuron_it->ActivationFunction = activationFunction;
	}
}

//FANN_EXTERNAL void FANN_API fann_set_activation_steepness_hidden(Fann * ann, ANNTYP steepness)
void Fann::SetActivationSteepnessHidden(ANNTYP steepness)
{
	const Fann::Layer * p_last_layer = P_LastLayer - 1; // -1 to not update the output layer
	for(Fann::Layer * p_layer_it = P_FirstLayer + 1; p_layer_it != p_last_layer; p_layer_it++) {
		const Fann::Neuron * p_last_neuron = p_layer_it->P_LastNeuron;
		for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++)
			p_neuron_it->ActivationSteepness = steepness;
	}
}

//FANN_EXTERNAL ANNTYP FANN_API fann_get_activation_steepness(Fann * ann, int layer, int neuron)
ANNTYP Fann::GetActivationSteepness(uint layer, int neuron) const
{
	const Fann::Neuron * p_neuron_it = GetNeuron(layer, neuron);
	return p_neuron_it ? p_neuron_it->ActivationSteepness : -1 /* layer or neuron out of bounds */;
}

//FANN_EXTERNAL void FANN_API fann_set_activation_steepness(Fann * ann, ANNTYP steepness, int layer, int neuron)
void Fann::SetActivationSteepness(ANNTYP steepness, uint layer, int neuron)
{
	Fann::Neuron * p_neur = GetNeuron(layer, neuron);
	if(p_neur)
		p_neur->ActivationSteepness = steepness;
}

//FANN_EXTERNAL void FANN_API fann_set_activation_steepness_layer(Fann * ann, ANNTYP steepness, int layer)
void Fann::SetActivationSteepnessLayer(ANNTYP steepness, uint layer)
{
	Fann::Layer * p_layer_it = GetLayer(layer);
	if(p_layer_it) {
		Fann::Neuron * p_last_neuron = p_layer_it->P_LastNeuron;
		for(Fann::Neuron * p_neuron_it = p_layer_it->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++)
			p_neuron_it->ActivationSteepness = steepness;
	}
}

//FANN_EXTERNAL void FANN_API fann_set_activation_steepness_output(Fann * ann, ANNTYP steepness)
void Fann::SetActivationSteepnessOutput(ANNTYP steepness)
{
	const Fann::Layer * p_last_layer = P_LastLayer - 1;
	const Fann::Neuron * p_last_neuron = p_last_layer->P_LastNeuron;
	for(Fann::Neuron * p_neuron_it = p_last_layer->P_FirstNeuron; p_neuron_it != p_last_neuron; p_neuron_it++)
		p_neuron_it->ActivationSteepness = steepness;
}

/*
FANN_GET_SET(void *, user_data)
FANN_GET_SET(float, cascade_output_change_fraction)
FANN_GET_SET(uint, cascade_output_stagnation_epochs)
FANN_GET_SET(float, cascade_candidate_change_fraction)
FANN_GET_SET(uint, cascade_candidate_stagnation_epochs)
FANN_GET_SET(uint, cascade_num_candidate_groups)
FANN_GET_SET(ANNTYP, cascade_weight_multiplier)
FANN_GET_SET(ANNTYP, cascade_candidate_limit)
FANN_GET_SET(uint, cascade_max_out_epochs)
FANN_GET_SET(uint, cascade_max_cand_epochs)
FANN_GET_SET(uint, cascade_min_out_epochs)
FANN_GET_SET(uint, cascade_min_cand_epochs)
FANN_GET_SET(fann_train_enum, training_algorithm)
FANN_GET_SET(fann_errorfunc_enum, train_error_function)
FANN_GET_SET(fann_callback_type, callback)
FANN_GET_SET(float, quickprop_decay)
FANN_GET_SET(float, quickprop_mu)
FANN_GET_SET(float, rprop_increase_factor)
FANN_GET_SET(float, rprop_decrease_factor)
FANN_GET_SET(float, rprop_delta_min)
FANN_GET_SET(float, rprop_delta_max)
FANN_GET_SET(float, rprop_delta_zero)
FANN_GET_SET(float, sarprop_weight_decay_shift)
FANN_GET_SET(float, sarprop_step_error_threshold_factor)
FANN_GET_SET(float, sarprop_step_error_shift)
FANN_GET_SET(float, sarprop_temperature)
FANN_GET_SET(fann_stopfunc_enum, train_stop_function)
FANN_GET_SET(ANNTYP, bit_fail_limit)
FANN_GET_SET(float, LearningRate)
FANN_GET_SET(float, LearningMomentum)

FANN_GET(uint, cascade_activation_functions_count)
FANN_GET(fann_activationfunc_enum *, cascade_activation_functions)
FANN_GET(uint, num_input)
FANN_GET(uint, num_output)
FANN_GET(uint, total_connections)
*/
//
// FANN_TRAIN_DATA
//
//
// Reads training data from a file.
//
/*FANN_EXTERNAL Fann::TrainData * FANN_API fann_read_train_from_file(const char * configuration_file)
{
	Fann::TrainData * data = 0;
	FILE * file = fopen(configuration_file, "r");
	if(!file) {
		fann_error(NULL, SLERR_FANN_CANT_OPEN_CONFIG_R, configuration_file);
	}
	else {
		data = fann_read_train_from_fd(file, configuration_file);
		fclose(file);
	}
	return data;
}*/
//
// Save training data to a file
//
FANN_EXTERNAL int FANN_API fann_save_train(Fann::TrainData * data, const char * filename)
{
	return fann_save_train_internal(data, filename, 0, 0);
}
//
// Save training data to a file in fixed point algebra. (Good for testing a network in fixed point)
//
FANN_EXTERNAL int FANN_API fann_save_train_to_fixed(Fann::TrainData * data, const char * filename, uint decimal_point)
{
	return fann_save_train_internal(data, filename, 1, decimal_point);
}
//
// deallocate the train data structure.
//
FANN_EXTERNAL void FANN_API fann_destroy_train(Fann::TrainData * pData)
{
	delete pData;
}
//
// Test a set of training data and calculate the MSE
//
//FANN_EXTERNAL float FANN_API fann_test_data(Fann * ann, Fann::TrainData * data)
float Fann::TestData(const Fann::TrainData * pData)
{
	if(!CheckInputOutputSizes(pData))
		return 0;
	else {
		ResetMSE();
		for(uint i = 0; i != pData->GetCount(); i++) {
			const float * p_input = (const float *)pData->InpL.at(i)->dataPtr();
			const float * p_output = (const float *)pData->OutL.at(i)->dataPtr();
			//Test(p_input, p_output);
			//ANNTYP * Fann::Test_(const ANNTYP * pInput, const ANNTYP * pDesiredOutput)
			{
				const ANNTYP * p_output_begin = Run(p_input);
				const ANNTYP * p_output_end = p_output_begin + NumOutput;
				Fann::Neuron * p_output_neuron = (P_LastLayer-1)->P_FirstNeuron;
				// calculate the error
				for(const ANNTYP * p_output_it = p_output_begin; p_output_it != p_output_end; p_output_it++) {
					const ANNTYP neuron_value = *p_output_it;
					ANNTYP neuron_diff = (*p_output - neuron_value);
					neuron_diff = UpdateMSE(p_output_neuron, neuron_diff);
					p_output++;
					p_output_neuron++;
					num_MSE++;
				}
				//return p_output_begin;
			}
		}
		return GetMSE();
	}
}

#ifndef FIXEDFANN

int Fann::Helper_TrainData(const Fann::TrainData * pData)
{
	int    ok = 1;
	ResetMSE();
	for(uint i = 0; i < pData->GetCount(); i++) {
		Run((const float *)pData->InpL.at(i)->dataPtr());
		THROW(ComputeMSE((const float *)pData->OutL.at(i)->dataPtr()));
		BackpropagateMSE();
		THROW(UpdateSlopesBatch(P_FirstLayer+1, P_LastLayer-1));
	}
	CATCHZOK
	return ok;
}

//FANN_EXTERNAL void FANN_API fann_train_on_data(Fann * ann, Fann::TrainData * pData, uint maxEpochs, uint epochsBetweenReports, float desiredError)
int Fann::TrainOnData(const Fann::TrainData * pData, uint maxEpochs, uint epochsBetweenReports, float desiredError)
{
	int    ok = 1;
	THROW(CheckInputOutputSizes(pData));
	if(epochsBetweenReports && !Callback)
		printf("Max epochs %8d. Desired error: %.10f.\n", maxEpochs, desiredError);
	int    is_desired_error_reached = 0;
	for(uint i = 1; !is_desired_error_reached && i <= maxEpochs; i++) {
		//
		// Train for one epoch with the selected training algorithm
		//
		switch(TrainingAlgorithm) {
			case FANN_TRAIN_QUICKPROP:
				if(P_PrevTrainSlopes == NULL)
					THROW(ClearTrainArrays());
				THROW(Helper_TrainData(pData));
				UpdateWeightsQuickprop(pData->GetCount(), 0, TotalConnections);
				break;
			case FANN_TRAIN_RPROP:
				if(P_PrevTrainSlopes == NULL)
					THROW(ClearTrainArrays());
				THROW(Helper_TrainData(pData));
				UpdateWeightsIrpropm(0, TotalConnections);
				break;
			case FANN_TRAIN_SARPROP:
				if(P_PrevTrainSlopes == NULL)
					THROW(ClearTrainArrays());
				THROW(Helper_TrainData(pData));
				UpdateWeightsSarprop(SarpropEpoch, 0, TotalConnections);
				++(SarpropEpoch);
				break;
			case FANN_TRAIN_BATCH:
				THROW(Helper_TrainData(pData));
				UpdateWeightsBatch(pData->GetCount(), 0, TotalConnections);
				break;
			case FANN_TRAIN_INCREMENTAL:
				{
					ResetMSE();
					for(uint didx = 0; didx != pData->GetCount(); didx++) {
						Train((const float *)pData->InpL.at(didx)->dataPtr(), (const float *)pData->OutL.at(didx)->dataPtr());
					}
				}
				break;
			default:
                CALLEXCEPT_S(SLERR_FANN_INVTRAINALG);
                break;
		}
		const float _error = GetMSE();
		//
		is_desired_error_reached = DesiredErrorReached(desiredError);
		//
		// print current output
		//
		if(epochsBetweenReports && (i % epochsBetweenReports == 0 || i == maxEpochs || i == 1 || is_desired_error_reached)) {
			if(!Callback) {
				printf("Epochs     %8d. Current error: %.10f. Bit fail %d.\n", i, _error, NumBitFail);
			}
			else if(((*Callback)(this, pData, maxEpochs, epochsBetweenReports, desiredError, i)) == -1)
				break; // you can break the training by returning -1
		}
	}
	CATCHZOK
	return ok;
}

/*FANN_EXTERNAL void FANN_API fann_train_on_file(Fann * ann, const char * filename, uint max_epochs, uint epochs_between_reports, float desired_error)
{
	Fann::TrainData * p_data = fann_read_train_from_file(filename);
	if(p_data) {
		ann->TrainOnData(p_data, max_epochs, epochs_between_reports, desired_error);
		delete p_data;
	}
}*/

#endif
//
// shuffles training data, randomizing the order
//
//FANN_EXTERNAL void FANN_API fann_shuffle_train_data(Fann::TrainData * train_data)
void Fann::TrainData::Shuffle()
{
	for(uint dat = 0; dat < GetCount(); dat++) {
		uint   swap = (uint)(rand() % GetCount());
		if(swap != dat) {
			uint   elem;
			Fann::DataVector * p_inp_vect_swap = InpL.at(swap);
			Fann::DataVector * p_out_vect_swap = OutL.at(swap);
			Fann::DataVector * p_inp_vect = InpL.at(dat);
			Fann::DataVector * p_out_vect = OutL.at(dat);
			for(elem = 0; elem < p_inp_vect->getCount(); elem++) {
				Exchange(&p_inp_vect->at(elem), &p_inp_vect_swap->at(elem));
			}
			for(elem = 0; elem < p_out_vect->getCount(); elem++) {
				Exchange(&p_out_vect->at(elem), &p_out_vect_swap->at(elem));
			}
		}
	}
}
//
// INTERNAL FUNCTION calculates min and max of train data
//
/*void fann_get_min_max_data(ANNTYP ** data, uint num_data, uint num_elem, ANNTYP * min, ANNTYP * max)
{
	*min = *max = data[0][0];
	for(uint dat = 0; dat < num_data; dat++) {
		for(uint elem = 0; elem < num_elem; elem++) {
			const ANNTYP temp = data[dat][elem];
			if(temp < *min)
				*min = temp;
			else if(temp > *max)
				*max = temp;
		}
	}
}*/

//static
void Fann::DataVector::GetMinMax(const TSCollection <DataVector> & rData, ANNTYP * pMin, ANNTYP * pMax)
{
	ANNTYP _min = rData.at(0)->at(0);
	ANNTYP _max = _min;
	for(uint dat = 0; dat < rData.getCount(); dat++) {
		const DataVector * p_vect = rData.at(dat);
		for(uint elem = 0; elem < p_vect->getCount(); elem++) {
			const ANNTYP v = p_vect->at(elem);
			SETMIN(_min, v);
			SETMAX(_max, v);
		}
	}
	ASSIGN_PTR(pMin, _min);
	ASSIGN_PTR(pMax, _max);
}

//FANN_EXTERNAL ANNTYP FANN_API fann_get_min_train_input(Fann::TrainData * train_data)
ANNTYP Fann::TrainData::GetMinInput() const
{
	ANNTYP min, max;
	//fann_get_min_max_data(train_data->input, train_data->num_data, train_data->num_input, &min, &max);
	Fann::DataVector::GetMinMax(InpL, &min, &max);
	return min;
}

//FANN_EXTERNAL ANNTYP FANN_API fann_get_max_train_input(Fann::TrainData * train_data)
ANNTYP Fann::TrainData::GetMaxInput() const
{
	ANNTYP min, max;
	//fann_get_min_max_data(train_data->input, train_data->num_data, train_data->num_input, &min, &max);
	Fann::DataVector::GetMinMax(InpL, &min, &max);
	return max;
}

//FANN_EXTERNAL ANNTYP FANN_API fann_get_min_train_output(Fann::TrainData * train_data)
ANNTYP Fann::TrainData::GetMinOutput() const
{
	ANNTYP min, max;
	//fann_get_min_max_data(train_data->output, train_data->num_data, train_data->num_output, &min, &max);
	Fann::DataVector::GetMinMax(OutL, &min, &max);
	return min;
}

//FANN_EXTERNAL ANNTYP FANN_API fann_get_max_train_output(Fann::TrainData * train_data)
ANNTYP Fann::TrainData::GetMaxOutput() const
{
	ANNTYP min, max;
	//fann_get_min_max_data(train_data->output, train_data->num_data, train_data->num_output, &min, &max);
	Fann::DataVector::GetMinMax(OutL, &min, &max);
	return max;
}
//
// INTERNAL FUNCTION Scales data to a specific range
//
/*void fann_scale_data(ANNTYP ** data, uint num_data, uint num_elem, ANNTYP new_min, ANNTYP new_max)
{
	ANNTYP old_min, old_max;
	fann_get_min_max_data(data, num_data, num_elem, &old_min, &old_max);
	fann_scale_data_to_range(data, num_data, num_elem, old_min, old_max, new_min, new_max);
}*/

//static
//void Fann::ScaleData(TSCollection <DataVector> & rData, ANNTYP newMin, ANNTYP newMax)
//
// INTERNAL FUNCTION Scales data to a specific range
//
/*FANN_EXTERNAL void FANN_API fann_scale_data_to_range(ANNTYP ** data, uint num_data, uint num_elem,
    ANNTYP old_min, ANNTYP old_max, ANNTYP new_min, ANNTYP new_max)
{
	const ANNTYP old_span = old_max - old_min;
	const ANNTYP new_span = new_max - new_min;
	const ANNTYP factor = new_span / old_span;
	// printf("max %f, min %f, factor %f\n", old_max, old_min, factor);
	for(uint dat = 0; dat < num_data; dat++) {
		for(uint elem = 0; elem < num_elem; elem++) {
			const ANNTYP temp = (data[dat][elem] - old_min) * factor + new_min;
			if(temp < new_min) {
				data[dat][elem] = new_min;
				// printf("error %f < %f\n", temp, new_min);
			}
			else if(temp > new_max) {
				data[dat][elem] = new_max;
				// printf("error %f > %f\n", temp, new_max);
			}
			else {
				data[dat][elem] = temp;
			}
		}
	}
}*/

//void Fann::ScaleDataToRange(ANNTYP ** data, uint num_data, uint num_elem, ANNTYP old_min, ANNTYP old_max, ANNTYP new_min, ANNTYP new_max)
//static
//void Fann::ScaleDataToRange(TSCollection <DataVector> & rData, ANNTYP oldMin, ANNTYP oldMax, ANNTYP newMin, ANNTYP newMax)
//static
void Fann::DataVector::ScaleToRange(TSCollection <DataVector> & rData, ANNTYP oldMin, ANNTYP oldMax, ANNTYP newMin, ANNTYP newMax)
{
	const ANNTYP old_span = oldMax - oldMin;
	const ANNTYP new_span = newMax - newMin;
	const ANNTYP factor = new_span / old_span;
	// printf("max %f, min %f, factor %f\n", old_max, old_min, factor);
	for(uint dat = 0; dat < rData.getCount(); dat++) {
		const DataVector * p_vect = rData.at(dat);
		for(uint elem = 0; elem < p_vect->getCount(); elem++) {
			const ANNTYP temp = (p_vect->at(elem) - oldMin) * factor + newMin;
			if(temp < newMin) {
				p_vect->at(elem) = newMin;
				// printf("error %f < %f\n", temp, new_min);
			}
			else if(temp > newMax) {
				p_vect->at(elem) = newMax;
				// printf("error %f > %f\n", temp, new_max);
			}
			else {
				p_vect->at(elem) = temp;
			}
		}
	}
}
//
// Scales the inputs in the training data to the specified range
//
//FANN_EXTERNAL void FANN_API fann_scale_input_train_data(Fann::TrainData * train_data, ANNTYP new_min, ANNTYP new_max)
void Fann::TrainData::ScaleInput(ANNTYP newMin, ANNTYP newMax)
{
	//fann_scale_data(train_data->input, train_data->num_data, train_data->num_input, new_min, new_max);
	Fann::DataVector::Scale(InpL, newMin, newMax);
}
//
// Scales the inputs in the training data to the specified range
//
//FANN_EXTERNAL void FANN_API fann_scale_output_train_data(Fann::TrainData * train_data, ANNTYP new_min, ANNTYP new_max)
void Fann::TrainData::ScaleOutput(ANNTYP newMin, ANNTYP newMax)
{
	//fann_scale_data(train_data->output, train_data->num_data, train_data->num_output, new_min, new_max);
	Fann::DataVector::Scale(OutL, newMin, newMax);
}
//
// Scales the inputs in the training data to the specified range
//
//FANN_EXTERNAL void FANN_API fann_scale_train_data(Fann::TrainData * pData, ANNTYP newMin, ANNTYP newMax)
void Fann::TrainData::Scale(ANNTYP newMin, ANNTYP newMax)
{
	//fann_scale_data(train_data->input, train_data->num_data, train_data->num_input, new_min, new_max);
	//fann_scale_data(train_data->output, train_data->num_data, train_data->num_output, new_min, new_max);
	ScaleInput(newMin, newMax);
	ScaleOutput(newMin, newMax);
}
//
// INTERNAL FUNCTION
// Save the train data structure.
//
int fann_save_train_internal(Fann::TrainData * data, const char * filename, uint save_as_fixed, uint decimal_point)
{
	int retval = 0;
	FILE * file = fopen(filename, "w");
	if(!file) {
		fann_error((FannError*)data, SLERR_FANN_CANT_OPEN_TD_W, filename);
		return -1;
	}
	retval = fann_save_train_internal_fd(data, file, filename, save_as_fixed, decimal_point);
	fclose(file);
	return retval;
}
//
// INTERNAL FUNCTION
// Save the train data structure.
//
int fann_save_train_internal_fd(Fann::TrainData * data, FILE * file, const char * filename, uint save_as_fixed, uint decimal_point)
{
	uint num_data = data->GetCount();
	uint num_input = data->GetInputCount();
	uint num_output = data->GetOutputCount();
	uint i, j;
	int retval = 0;
#ifndef FIXEDFANN
	uint multiplier = 1 << decimal_point;
#endif
	fprintf(file, "%u %u %u\n", data->GetCount(), data->GetInputCount(), data->GetOutputCount());
	for(i = 0; i < num_data; i++) {
		const Fann::DataVector * p_inp_vect = data->InpL.at(i);
		const Fann::DataVector * p_out_vect = data->OutL.at(i);
		for(j = 0; j < num_input; j++) {
			const float iv = p_inp_vect->at(j);
#ifndef FIXEDFANN
			if(save_as_fixed) {
				fprintf(file, "%d ", (int)(iv * multiplier));
			}
			else {
				if(((int)floor(iv + 0.5) * 1000000) == ((int)floor(iv * 1000000.0 + 0.5))) {
					fprintf(file, "%d ", (int)iv);
				}
				else {
					fprintf(file, "%f ", iv);
				}
			}
#else
			fprintf(file, FANNPRINTF " ", iv);
#endif
		}
		fprintf(file, "\n");

		for(j = 0; j < num_output; j++) {
			const float ov = p_out_vect->at(j);
#ifndef FIXEDFANN
			if(save_as_fixed) {
				fprintf(file, "%d ", (int)(ov * multiplier));
			}
			else {
				if(((int)floor(ov + 0.5) * 1000000) == ((int)floor(ov * 1000000.0 + 0.5))) {
					fprintf(file, "%d ", (int)ov);
				}
				else {
					fprintf(file, "%f ", ov);
				}
			}
#else
			fprintf(file, FANNPRINTF " ", ov);
#endif
		}
		fprintf(file, "\n");
	}
	return retval;
}
//
//
//
Fann::ScaleParam::ScaleParam()
{
	P_Mean = 0;
	P_Deviation = 0;
	P_NewMin = 0;
	P_Factor = 0;
}

Fann::ScaleParam::~ScaleParam()
{
	Destroy();
}

int Fann::ScaleParam::Copy(uint count, const ScaleParam & rS)
{
	int    ok = 1;
	Destroy();
	if(rS.P_Mean) {
		THROW(P_Mean = Helper_Allocate(count, 0.0));
		memcpy(P_Mean, rS.P_Mean, count * sizeof(float));
	}
	if(rS.P_Deviation) {
		THROW(P_Deviation = Helper_Allocate(count, 0.0));
		memcpy(P_Deviation, rS.P_Deviation, count * sizeof(float));
	}
	if(rS.P_NewMin) {
		THROW(P_NewMin = Helper_Allocate(count, 0.0));
		memcpy(P_NewMin, rS.P_NewMin, count * sizeof(float));
	}
	if(rS.P_Factor) {
		THROW(P_Factor = Helper_Allocate(count, 0.0));
		memcpy(P_Factor, rS.P_Factor, count * sizeof(float));
	}
	CATCHZOK
	return ok;
}

void Fann::ScaleParam::Destroy()
{
	ZFREE(P_Mean);
	ZFREE(P_Deviation);
	ZFREE(P_NewMin);
	ZFREE(P_Factor);
}

int Fann::ScaleParam::Allocate(uint count)
{
	int    ok = 1;
	Destroy();
	THROW(P_Mean = Helper_Allocate(count,      0.0));
	THROW(P_Deviation = Helper_Allocate(count, 1.0));
	THROW(P_NewMin = Helper_Allocate(count,   -1.0));
	THROW(P_Factor = Helper_Allocate(count,    1.0));
	CATCH
		Destroy();
		ok = 0;
	ENDCATCH
	return ok;
}

int Fann::ScaleParam::ScaleVector(Fann::DataVector * pV) const
{
	int    ok = 1;
	THROW_S(P_Mean && P_Deviation && P_Factor && P_NewMin, SLERR_FANN_SCALE_NOT_PRESENT);
	for(uint i = 0; i < pV->getCount(); i++) {
		pV->at(i) = (ANNTYP)(((pV->at(i) - P_Mean[i]) / P_Deviation[i] - (-1.0f) /* This is old_min */) * P_Factor[i] + P_NewMin[i]);
	}
	CATCHZOK
	return ok;
}

int Fann::ScaleParam::DescaleVector(Fann::DataVector * pV) const
{
	int    ok = 1;
	THROW_S(P_Mean && P_Deviation && P_Factor && P_NewMin, SLERR_FANN_SCALE_NOT_PRESENT);
	for(uint i = 0; i < pV->getCount(); i++) {
		pV->at(i) = (ANNTYP)((((float)pV->at(i) - P_NewMin[i]) / P_Factor[i] + (-1.0f) /* This is old_min */) * P_Deviation[i] + P_Mean[i]);
	}
	CATCHZOK
	return ok;
}

int Fann::ScaleParam::IsEqualVect(uint count, const float * pVect, const float * pOtherVect) const
{
	int    ok = 1;
	THROW(BIN(pVect) == BIN(pOtherVect));
	if(pVect) {
		for(uint i = 0; i < count; i++) {
			THROW(pVect[i] == pOtherVect[i]);
		}
	}
	CATCHZOK
	return ok;
}

int Fann::ScaleParam::IsEqual(uint c, const ScaleParam & rS) const
{
	int    ok = 1;
	THROW(IsEqualVect(c,  P_Mean,       rS.P_Mean));
	THROW(IsEqualVect(c,  P_Deviation,  rS.P_Deviation));
	THROW(IsEqualVect(c,  P_NewMin,     rS.P_NewMin));
	THROW(IsEqualVect(c,  P_Factor,     rS.P_Factor));
	CATCHZOK
	return ok;
}

void Fann::ScaleParam::Set(uint c, const TSCollection <Fann::DataVector> & rData, float newMin, float newMax)
{
	uint cur_neuron, cur_sample;
	// Calculate mean: sum(x)/length
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		P_Mean[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < rData.getCount(); cur_sample++) {
			const Fann::DataVector * p_vect = rData.at(cur_sample);
			P_Mean[cur_neuron] += p_vect->at(cur_neuron);
		}
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		P_Mean[cur_neuron] /= (float)rData.getCount();
	// Calculate deviation: sqrt(sum((x-mean)^2)/length)
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		P_Deviation[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < rData.getCount(); cur_sample++) {
			const Fann::DataVector * p_vect = rData.at(cur_sample);
			P_Deviation[cur_neuron] += (p_vect->at(cur_neuron) - P_Mean[cur_neuron]) * (p_vect->at(cur_neuron) - P_Mean[cur_neuron]);
		}
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		P_Deviation[cur_neuron] = sqrtf(P_Deviation[cur_neuron] / (float)rData.getCount());
		// Calculate factor: (new_max-new_min)/(old_max(1)-old_min(-1))
		// Looks like we dont need whole array of factors?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		P_Factor[cur_neuron] = (newMax - newMin) / (1.0f - (-1.0f));
	// Copy new minimum.
	// Looks like we dont need whole array of new minimums?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		P_NewMin[cur_neuron] = newMin;
}

int Fann::ScaleParam::Serialize(int dir, uint c, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint   _count = (dir > 0) ? (P_Mean ? c : 0) : 0;
	THROW(pSCtx->Serialize(dir, _count, rBuf));
	if(dir > 0) {
		if(_count) {
			assert(P_Mean && P_Deviation && P_NewMin && P_Factor);
			for(uint i = 0; i < _count; i++) {
				THROW(pSCtx->Serialize(dir, P_Mean[i], rBuf));
				THROW(pSCtx->Serialize(dir, P_Deviation[i], rBuf));
				THROW(pSCtx->Serialize(dir, P_NewMin[i], rBuf));
				THROW(pSCtx->Serialize(dir, P_Factor[i], rBuf));
			}
		}
	}
	else if(dir < 0) {
		THROW(_count == 0 || _count == c);
		if(_count) {
			THROW(Allocate(_count));
			for(uint i = 0; i < _count; i++) {
				THROW(pSCtx->Serialize(dir, P_Mean[i], rBuf));
				THROW(pSCtx->Serialize(dir, P_Deviation[i], rBuf));
				THROW(pSCtx->Serialize(dir, P_NewMin[i], rBuf));
				THROW(pSCtx->Serialize(dir, P_Factor[i], rBuf));
			}
		}
		else
			Destroy();
	}
	CATCHZOK
	return ok;
}

int Fann::ScaleParam::Save(FILE * pF, uint c, const char * pSuffix)
{
	int    ok = 1;
	SString temp_buf;
	(temp_buf = "scale_mean").CatChar('_').Cat(pSuffix);
	SaveVector(pF, c, P_Mean, temp_buf);
	(temp_buf = "scale_deviation").CatChar('_').Cat(pSuffix);
	SaveVector(pF, c, P_Deviation, temp_buf);
	(temp_buf = "scale_new_min").CatChar('_').Cat(pSuffix);
	SaveVector(pF, c, P_NewMin, temp_buf);
	(temp_buf = "scale_factor").CatChar('_').Cat(pSuffix);
	SaveVector(pF, c, P_Factor, temp_buf);
	return ok;
}

int Fann::ScaleParam::Load(FILE * pF, uint c, const char * pSuffix)
{
	int    ok = 1;
	SString temp_buf;
	(temp_buf = "scale_mean").CatChar('_').Cat(pSuffix);
	THROW(LoadVector(pF, c, P_Mean, temp_buf));
	(temp_buf = "scale_deviation").CatChar('_').Cat(pSuffix);
	THROW(LoadVector(pF, c, P_Deviation, temp_buf));
	(temp_buf = "scale_new_min").CatChar('_').Cat(pSuffix);
	THROW(LoadVector(pF, c, P_NewMin, temp_buf));
	(temp_buf = "scale_factor").CatChar('_').Cat(pSuffix);
	THROW(LoadVector(pF, c, P_Factor, temp_buf));
	CATCHZOK
	return ok;
}

float * Fann::ScaleParam::Helper_Allocate(uint c, float defValue)
{
	float * p_list = (float *)SAlloc::M(c * sizeof(float));
	if(p_list) {
		for(uint i = 0; i < c; i++)
			p_list[i] = defValue;
	}
	return p_list;
}

void Fann::ScaleParam::SaveVector(FILE * pF, uint c, const float * pList, const char * pField) const
{
	fprintf(pF, "%s=", pField);
	for(uint i = 0; i < c; i++)
		fprintf(pF, "%f ", pList[i]);
	fprintf(pF, "\n");
}

int Fann::ScaleParam::LoadVector(FILE * pF, uint c, float * pList, const char * pField)
{
	int    ok = 1;
	SString temp_buf;
	char   _buf[256];
	(temp_buf = pField).CatChar('=');
	STRNSCPY(_buf, temp_buf);
	THROW_S(fscanf(pF, _buf) == 0, SLERR_FANN_CANT_READ_CONFIG);
	for(uint i = 0; i < c; i++) {
		THROW_S(fscanf(pF, "%f ", &pList[i]) == 1, SLERR_FANN_CANT_READ_CONFIG);
	}
	CATCHZOK
	return ok;
}
//
// Creates an empty set of training data
//
FANN_EXTERNAL Fann::TrainData * FANN_API fann_create_train(uint numData, uint numInput, uint numOutput)
{
	Fann::TrainData * p_data = new Fann::TrainData(numInput, numOutput, numData);
	if(p_data && !p_data->IsValid()) {
		ZDELETE(p_data);
	}
	return p_data;
}
//
// INTERNAL FUNCTION Reads training data from a file descriptor.
//
/*Fann::TrainData * fann_read_train_from_fd(FILE * file, const char * filename)
{
	Fann::TrainData * data = 0;
	uint num_input, num_output, num_data, i, j;
	uint line = 1;
	if(fscanf(file, "%u %u %u\n", &num_data, &num_input, &num_output) != 3) {
		fann_error(NULL, SLERR_FANN_CANT_READ_TD, filename, line);
	}
	else {
		line++;
		data = fann_create_train(num_data, num_input, num_output);
		if(data) {
			for(i = 0; i != num_data; i++) {
				Fann::DataVector * p_inp_vect = data->InpL.at(i);
				Fann::DataVector * p_out_vect = data->OutL.at(i);
				for(j = 0; j != num_input; j++) {
					THROW_S(fscanf(file, FANNSCANF " ", &p_inp_vect->at(j)) == 1, SLERR_FANN_CANT_READ_TD);
				}
				line++;
				for(j = 0; j != num_output; j++) {
					THROW_S(fscanf(file, FANNSCANF " ", &p_out_vect->at(j)) == 1, SLERR_FANN_CANT_READ_TD);
				}
				line++;
			}
		}
	}
	CATCH
		ZDELETE(data);
	ENDCATCH
	return data;
}*/
//
// INTERNAL FUNCTION returns 1 if the desired error is reached and 0 if it is not reached
//
/*int fann_desired_error_reached(const Fann * ann, float desired_error)
	{ return ann->DesiredErrorReached(desired_error); }*/

#ifndef FIXEDFANN
/*
 * Scale input and output data based on previously calculated parameters.
 */
//FANN_EXTERNAL void FANN_API fann_scale_train(Fann * ann, Fann::TrainData * data)
int Fann::ScaleTrain(Fann::TrainData * pData)
{
	int    ok = 1;
	THROW_S(ScaleIn.IsPresent(), SLERR_FANN_SCALE_NOT_PRESENT);
	// Check that we have good training data.
	THROW(CheckInputOutputSizes(pData));
	{
		for(uint i = 0; i < pData->InpL.getCount(); i++) {
			THROW(ScaleIn.ScaleVector(pData->InpL.at(i)));
		}
	}
	{
		for(uint i = 0; i < pData->OutL.getCount(); i++) {
			THROW(ScaleOut.ScaleVector(pData->OutL.at(i)));
		}
	}
	CATCHZOK
	return ok;
}
//
// Scale input and output data based on previously calculated parameters.
//
//FANN_EXTERNAL void FANN_API fann_descale_train(Fann * ann, Fann::TrainData * data)
int Fann::DescaleTrain(Fann::TrainData * pData)
{
	int    ok = 1;
	THROW_S(ScaleIn.IsPresent(), SLERR_FANN_SCALE_NOT_PRESENT);
	// Check that we have good training data
	THROW(CheckInputOutputSizes(pData));
	{
		for(uint i = 0; i < pData->InpL.getCount(); i++) {
			THROW(ScaleIn.DescaleVector(pData->InpL.at(i)));
		}
	}
	{
		for(uint i = 0; i < pData->OutL.getCount(); i++) {
			THROW(ScaleOut.DescaleVector(pData->OutL.at(i)));
		}
	}
	CATCHZOK
	return ok;
}

void Fann::ScaleReset(uint c, float * pArray, float value)
{
	for(uint i = 0; i < c; i++)
		pArray[i] = value;
}

#if 0 // {
#define SCALE_RESET(what, where, default_value)							      \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++) \
		ann->what ## _ ## where[cur_neuron] = ( default_value );

#define SCALE_SET_PARAM(where)																		      \
        /* Calculate mean: sum(x)/length */																	\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_mean_ ## where[cur_neuron] = 0.0f;													  \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		for(cur_sample = 0; cur_sample < data->num_data; cur_sample++)								      \
			ann->scale_mean_ ## where[cur_neuron] += (float)data->where ## put[ cur_sample ][cur_neuron]; \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_mean_ ## where[cur_neuron] /= (float)data->num_data;								  \
        /* Calculate deviation: sqrt(sum((x-mean)^2)/length) */												\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_deviation_ ## where[cur_neuron] = 0.0f;												  \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		for(cur_sample = 0; cur_sample < data->num_data; cur_sample++)								      \
			ann->scale_deviation_ ## where[cur_neuron] +=												  \
			    /* Another local variable in macro? Oh no! */										    \
			    ((float)data->where ## put[ cur_sample ][cur_neuron] - ann->scale_mean_ ## where[cur_neuron] \
			    )																						    \
			    *																						    \
			    ((float)data->where ## put[ cur_sample ][cur_neuron] - ann->scale_mean_ ## where[cur_neuron] \
			    );																						    \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_deviation_ ## where[cur_neuron] = sqrtf(ann->scale_deviation_ ## where[cur_neuron] / (float)data->num_data); \
        /* Calculate factor: (new_max-new_min)/(old_max(1)-old_min(-1)) */									\
        /* Looks like we dont need whole array of factors? */												\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_factor_ ## where[cur_neuron] =														  \
		    ( new_ ## where ## put_max - new_ ## where ## put_min )											    \
		    /																							    \
		    ( 1.0f - ( -1.0f ) );																	    \
        /* Copy new minimum. */																				\
        /* Looks like we dont need whole array of new minimums? */											\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_new_min_ ## where[cur_neuron] = new_ ## where ## put_min;
#endif // } 0

void Fann::ScaleSetParam(uint c, uint numData, ANNTYP ** const ppData, float newMin, float newMax,
	float * pScaleMean, float * pScaleDeviation, float * pScaleNewMin, float * pScaleFactor)
{
	uint cur_neuron, cur_sample;
	// Calculate mean: sum(x)/length
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		pScaleMean[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < numData; cur_sample++)
			pScaleMean[cur_neuron] += (float)ppData[cur_sample][cur_neuron];
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		pScaleMean[cur_neuron] /= (float)numData;
	// Calculate deviation: sqrt(sum((x-mean)^2)/length)
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		pScaleDeviation[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < numData; cur_sample++) {
			pScaleDeviation[cur_neuron] += ((float)ppData[cur_sample][cur_neuron] - pScaleMean[cur_neuron]) * ((float)ppData[cur_sample][cur_neuron] - pScaleMean[cur_neuron]);
		}
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		pScaleDeviation[cur_neuron] = sqrtf(pScaleDeviation[cur_neuron] / (float)numData);
        // Calculate factor: (new_max-new_min)/(old_max(1)-old_min(-1))
        // Looks like we dont need whole array of factors?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		pScaleFactor[cur_neuron] = (newMax - newMin) / (1.0f - (-1.0f));
	// Copy new minimum.
	// Looks like we dont need whole array of new minimums?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		pScaleNewMin[cur_neuron] = newMin;
}

/*void Fann::ScaleSetParam2(uint c, uint numData, ANNTYP ** const ppData, float newMin, float newMax, Fann::ScaleParam & rParam)
{
	uint cur_neuron, cur_sample;
	// Calculate mean: sum(x)/length
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Mean[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < numData; cur_sample++)
			rParam.P_Mean[cur_neuron] += (float)ppData[cur_sample][cur_neuron];
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Mean[cur_neuron] /= (float)numData;
	// Calculate deviation: sqrt(sum((x-mean)^2)/length)
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Deviation[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < numData; cur_sample++) {
			rParam.P_Deviation[cur_neuron] += ((float)ppData[cur_sample][cur_neuron] - rParam.P_Mean[cur_neuron]) * ((float)ppData[cur_sample][cur_neuron] - rParam.P_Mean[cur_neuron]);
		}
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Deviation[cur_neuron] = sqrtf(rParam.P_Deviation[cur_neuron] / (float)numData);
        // Calculate factor: (new_max-new_min)/(old_max(1)-old_min(-1))
        // Looks like we dont need whole array of factors?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Factor[cur_neuron] = (newMax - newMin) / (1.0f - (-1.0f));
	// Copy new minimum.
	// Looks like we dont need whole array of new minimums?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_NewMin[cur_neuron] = newMin;
}*/

/*void Fann::ScaleSetParam3(uint c, const TSCollection <Fann::DataVector> & rData, float newMin, float newMax, Fann::ScaleParam & rParam)
{
	uint cur_neuron, cur_sample;
	// Calculate mean: sum(x)/length
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Mean[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < rData.getCount(); cur_sample++) {
			const Fann::DataVector * p_vect = rData.at(cur_sample);
			rParam.P_Mean[cur_neuron] += p_vect->at(cur_neuron);
		}
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Mean[cur_neuron] /= (float)rData.getCount();
	// Calculate deviation: sqrt(sum((x-mean)^2)/length)
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Deviation[cur_neuron] = 0.0f;
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		for(cur_sample = 0; cur_sample < rData.getCount(); cur_sample++) {
			const Fann::DataVector * p_vect = rData.at(cur_sample);
			rParam.P_Deviation[cur_neuron] += (p_vect->at(cur_neuron) - rParam.P_Mean[cur_neuron]) * (p_vect->at(cur_neuron) - rParam.P_Mean[cur_neuron]);
		}
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Deviation[cur_neuron] = sqrtf(rParam.P_Deviation[cur_neuron] / (float)rData.getCount());
        // Calculate factor: (new_max-new_min)/(old_max(1)-old_min(-1))
        // Looks like we dont need whole array of factors?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_Factor[cur_neuron] = (newMax - newMin) / (1.0f - (-1.0f));
	// Copy new minimum.
	// Looks like we dont need whole array of new minimums?
	for(cur_neuron = 0; cur_neuron < c; cur_neuron++)
		rParam.P_NewMin[cur_neuron] = newMin;
}*/

/* FANN_EXTERNAL int FANN_API fann_set_input_scaling_params(Fann * ann, const Fann::TrainData * pData, float new_input_min, float new_input_max)
{
	//uint cur_neuron, cur_sample;
	// Check that we have good training data.
	// No need for if( !params || !ann )
	if(pData->num_input != ann->NumInput || pData->num_output != ann->NumOutput) {
		fann_error(&ann->Err, SLERR_FANN_TRAIN_DATA_MISMATCH);
		return -1;
	}
	if(ann->scale_mean_in == NULL)
		ann->AllocateScale();
	if(ann->scale_mean_in == NULL)
		return -1;
	const uint c = ann->NumInput;
	if(!pData->num_data) {
		//SCALE_RESET(scale_mean,        in,  0.0)
		//SCALE_RESET(scale_deviation,   in,  1.0)
		//SCALE_RESET(scale_new_min,     in, -1.0)
		//SCALE_RESET(scale_factor,      in,  1.0)
		ann->ScaleReset(c, ann->scale_mean_in,      0.0f);
		ann->ScaleReset(c, ann->scale_deviation_in, 1.0f);
		ann->ScaleReset(c, ann->scale_new_min_in,  -1.0f);
		ann->ScaleReset(c, ann->scale_factor_in,    1.0f);
	}
	else {
		//SCALE_SET_PARAM(in);
		ann->ScaleSetParam(c, pData->num_input, pData->input, new_input_min, new_input_max,
			ann->scale_mean_in, ann->scale_deviation_in, ann->scale_new_min_in, ann->scale_factor_in);
	}
	return 0;
}*/

int Fann::SetInputScalingParams(const Fann::TrainData * pData, float newInputMin, float newInputMax)
{
	int    ok = 1;
	//uint cur_neuron, cur_sample;
	// Check that we have good training data.
	// No need for if( !params || !ann )
	THROW_S(pData->GetInputCount() == NumInput && pData->GetOutputCount() == NumOutput, SLERR_FANN_TRAIN_DATA_MISMATCH);
	if(!pData->GetCount()) {
		THROW(ScaleIn.Allocate(NumInput));
	}
	else {
		if(!ScaleIn.IsPresent())
			THROW(ScaleIn.Allocate(NumInput));
		//SCALE_SET_PARAM(in);
		//ScaleSetParam(c, pData->num_input, pData->input, newInputMin, newInputMax, scale_mean_in, scale_deviation_in, scale_new_min_in, scale_factor_in);
		//ScaleSetParam2(NumInput, pData->GetInputCount(), pData->input, newInputMin, newInputMax, ScaleIn);
		//ScaleSetParam3(NumInput, pData->InpL, newInputMin, newInputMax, ScaleIn);
		ScaleIn.Set(NumInput, pData->InpL, newInputMin, newInputMax);
	}
	CATCHZOK
	return ok;
}


/*FANN_EXTERNAL int FANN_API fann_set_output_scaling_params(Fann * ann, const Fann::TrainData * data, float new_output_min, float new_output_max)
{
	//uint cur_neuron, cur_sample;
	// Check that we have good training data.
	// No need for if( !params || !ann )
	if(data->num_input != ann->NumInput || data->num_output != ann->NumOutput) {
		fann_error(&ann->Err, SLERR_FANN_TRAIN_DATA_MISMATCH);
		return -1;
	}
	if(ann->scale_mean_out == NULL)
		ann->AllocateScale();
	if(ann->scale_mean_out == NULL)
		return -1;
	const uint c = ann->NumOutput;
	if(!data->num_data) {
		//SCALE_RESET(scale_mean,      out,  0.0)
		//SCALE_RESET(scale_deviation, out,  1.0)
		//SCALE_RESET(scale_new_min,   out, -1.0)
		//SCALE_RESET(scale_factor,    out,  1.0)
		ann->ScaleReset(c, ann->scale_mean_out,      0.0f);
		ann->ScaleReset(c, ann->scale_deviation_out, 1.0f);
		ann->ScaleReset(c, ann->scale_new_min_out,  -1.0f);
		ann->ScaleReset(c, ann->scale_factor_out,    1.0f);
	}
	else {
		//SCALE_SET_PARAM(out);
		ann->ScaleSetParam(c, data->num_output, data->output, new_output_min, new_output_max,
			ann->scale_mean_out, ann->scale_deviation_out, ann->scale_new_min_out, ann->scale_factor_out);
	}
	return 0;
}*/

int Fann::SetOutputScalingParams(const Fann::TrainData * pData, float newOutputMin, float newOutputMax)
{
	int    ok = 1;
	//uint cur_neuron, cur_sample;
	// Check that we have good training data.
	// No need for if( !params || !ann )
	THROW_S(pData->GetInputCount() == NumInput && pData->GetOutputCount() == NumOutput, SLERR_FANN_TRAIN_DATA_MISMATCH);
	if(!pData->GetCount()) {
		THROW(ScaleOut.Allocate(NumOutput));
	}
	else {
		if(!ScaleOut.IsPresent())
			THROW(ScaleOut.Allocate(NumOutput));
		//SCALE_SET_PARAM(out);
		//ScaleSetParam(c, pData->num_output, pData->output, newOutputMin, newOutputMax, scale_mean_out, scale_deviation_out, scale_new_min_out, scale_factor_out);
		//ScaleSetParam2(NumOutput, pData->GetOutputCount(), pData->output, newOutputMin, newOutputMax, ScaleOut);
		//ScaleSetParam3(NumOutput, pData->OutL, newOutputMin, newOutputMax, ScaleOut);
		ScaleOut.Set(NumOutput, pData->OutL, newOutputMin, newOutputMax);
	}
	CATCHZOK
	return ok;
}
//
// Calculate scaling parameters for future use based on training data.
//
/*FANN_EXTERNAL int FANN_API fann_set_scaling_params(Fann * ann,
    const Fann::TrainData * data, float new_input_min, float new_input_max, float new_output_min, float new_output_max)
{
	return (ann->SetInputScalingParams(data, new_input_min, new_input_max) == 0) ? ann->SetOutputScalingParams(data, new_output_min, new_output_max) : -1;
}*/

int Fann::SetScalingParams(const Fann::TrainData * pData, float newInputMin, float newInputMax, float newOutputMin, float newOutputMax)
{
	return (SetInputScalingParams(pData, newInputMin, newInputMax) == 0) ? SetOutputScalingParams(pData, newOutputMin, newOutputMax) : -1;
}
//
// Clears scaling parameters.
//
/*FANN_EXTERNAL int FANN_API fann_clear_scaling_params(Fann * ann)
{
	//uint cur_neuron;
	if(ann->scale_mean_out == NULL)
		ann->AllocateScale();
	if(ann->scale_mean_out == NULL)
		return -1;
	//SCALE_RESET(scale_mean,      in,   0.0)
	//SCALE_RESET(scale_deviation, in,   1.0)
	//SCALE_RESET(scale_new_min,   in,  -1.0)
	//SCALE_RESET(scale_factor,    in,   1.0)
	ann->ScaleReset(ann->NumInput, ann->scale_mean_in,      0.0f);
	ann->ScaleReset(ann->NumInput, ann->scale_deviation_in, 1.0f);
	ann->ScaleReset(ann->NumInput, ann->scale_new_min_in,  -1.0f);
	ann->ScaleReset(ann->NumInput, ann->scale_factor_in,    1.0f);
	//SCALE_RESET(scale_mean,      out,  0.0)
	//SCALE_RESET(scale_deviation, out,  1.0)
	//SCALE_RESET(scale_new_min,   out, -1.0)
	//SCALE_RESET(scale_factor,    out,  1.0)
	ann->ScaleReset(ann->NumOutput, ann->scale_mean_out,      0.0f);
	ann->ScaleReset(ann->NumOutput, ann->scale_deviation_out, 1.0f);
	ann->ScaleReset(ann->NumOutput, ann->scale_new_min_out,  -1.0f);
	ann->ScaleReset(ann->NumOutput, ann->scale_factor_out,    1.0f);
	return 0;
}*/

int Fann::ClearScalingParams()
{
	//uint cur_neuron;
	int    ok = 1;
	THROW(ScaleIn.Allocate(NumInput));
	THROW(ScaleOut.Allocate(NumOutput));
	/*
	if(scale_mean_out == NULL)
		AllocateScale();
	if(scale_mean_out == NULL)
		return -1;
	//SCALE_RESET(scale_mean,      in,   0.0)
	//SCALE_RESET(scale_deviation, in,   1.0)
	//SCALE_RESET(scale_new_min,   in,  -1.0)
	//SCALE_RESET(scale_factor,    in,   1.0)
	ScaleReset(NumInput, scale_mean_in,      0.0f);
	ScaleReset(NumInput, scale_deviation_in, 1.0f);
	ScaleReset(NumInput, scale_new_min_in,  -1.0f);
	ScaleReset(NumInput, scale_factor_in,    1.0f);
	//SCALE_RESET(scale_mean,      out,  0.0)
	//SCALE_RESET(scale_deviation, out,  1.0)
	//SCALE_RESET(scale_new_min,   out, -1.0)
	//SCALE_RESET(scale_factor,    out,  1.0)
	ScaleReset(NumOutput, scale_mean_out,      0.0f);
	ScaleReset(NumOutput, scale_deviation_out, 1.0f);
	ScaleReset(NumOutput, scale_new_min_out,  -1.0f);
	ScaleReset(NumOutput, scale_factor_out,    1.0f);
	*/
	CATCHZOK
	return ok;
}

#endif // !FIXEDFANN

//int fann_check_input_output_sizes(Fann * ann, Fann::TrainData * pData)
int Fann::CheckInputOutputSizes(const Fann::TrainData * pData)
{
	int   ok = 1;
	THROW_S(NumInput == pData->GetInputCount(), SLERR_FANN_INPUT_NO_MATCH); //fann_error(&Err, SLERR_FANN_INPUT_NO_MATCH, NumInput, pData->num_input);
	THROW_S(NumOutput == pData->GetOutputCount(), SLERR_FANN_OUTPUT_NO_MATCH); // fann_error(&Err, SLERR_FANN_OUTPUT_NO_MATCH, NumOutput, pData->num_output);
	CATCHZOK
	return ok;
}

float Fann::ExamineTrain(/*Fann::TrainAlg tal, Fann::ActivationFunc hact, Fann::ActivationFunc oact*/
	const ExamineTrainParam & rParam, const Fann::TrainData * pTrainData, const Fann::TrainData * pTestData)
{
	SetTrainingAlgorithm(rParam.TrAlg);
	SetActivationFunctionHidden(rParam.HiddActF);
	SetActivationFunctionOutput(rParam.OutpActF);
	//fann_set_activation_function_output(ann, oact);
	//fann_set_callback(ann, LogOut );
	TrainOnData(pTrainData, NZOR(rParam.MaxEpoch, 2000), 0, 0.0f);
	//fann_train_on_data(ann, TrainData, 2000, 250, 0.0);
	float  train_mse = GetMSE(); //fann_get_MSE(ann);
	float  test_mse = -1.0f;
	if(pTestData /*&& ft.overtraining*/) {
		//fann_reset_MSE(ann);
		ResetMSE();
		//fann_test_data(ann,ft.TestData);
		TestData(pTestData);
		test_mse = GetMSE(); // fann_get_MSE(ann);
		return (train_mse + test_mse) / 2.0f;
	}
	else
		return train_mse;
}

//static
int Fann::DetectOptimal(Fann::DetectOptimalParam & rParam)
{
	int    ok = 1;
	ANNTYP * p_preserve_weights = 0;
	THROW(rParam.P_TrainData);
	{
		Fann   test_ann(rParam.NetworkType, rParam.ConnectionRate, rParam.Layers);
		THROW(test_ann.IsValid());
		const size_t weights_buffer_size = test_ann.GetWeights(0, 0);
		THROW(p_preserve_weights = (ANNTYP *)SAlloc::M(weights_buffer_size));
		THROW(test_ann.GetWeights(p_preserve_weights, weights_buffer_size));
		if(rParam.Flags & rParam.fDetectActivationFunc) {
			uint   best_hi = 0;
			uint   best_oi = 0;
			float  best_mse = MAXFLOAT;

			LongArray activation_func_list;
			activation_func_list.add(Fann::FANN_LINEAR);
			// (Can NOT be used during training) activation_func_list.add(Fann::FANN_THRESHOLD);
			// (Can NOT be used during training) activation_func_list.add(Fann::FANN_THRESHOLD_SYMMETRIC);
			activation_func_list.add(Fann::FANN_SIGMOID);
			activation_func_list.add(Fann::FANN_SIGMOID_STEPWISE);
			activation_func_list.add(Fann::FANN_SIGMOID_SYMMETRIC);
			activation_func_list.add(Fann::FANN_SIGMOID_SYMMETRIC_STEPWISE);
			activation_func_list.add(Fann::FANN_GAUSSIAN);
			activation_func_list.add(Fann::FANN_GAUSSIAN_SYMMETRIC);
			activation_func_list.add(Fann::FANN_GAUSSIAN_STEPWISE);
			activation_func_list.add(Fann::FANN_ELLIOT);
			activation_func_list.add(Fann::FANN_ELLIOT_SYMMETRIC);
			activation_func_list.add(Fann::FANN_LINEAR_PIECE);
			activation_func_list.add(Fann::FANN_LINEAR_PIECE_SYMMETRIC);
			activation_func_list.add(Fann::FANN_SIN_SYMMETRIC);
			activation_func_list.add(Fann::FANN_COS_SYMMETRIC);
			activation_func_list.add(Fann::FANN_SIN);
			activation_func_list.add(Fann::FANN_COS);

			for(uint hi = 0; hi < activation_func_list.getCount(); hi++) {
				for(uint oi = 0; oi < activation_func_list.getCount(); oi++) {
					test_ann.SetWeights(p_preserve_weights);
					ExamineTrainParam etp;
					etp.HiddActF = (Fann::ActivationFunc)activation_func_list.at(hi);
					etp.OutpActF = (Fann::ActivationFunc)activation_func_list.at(oi);
					const float mse = test_ann.ExamineTrain(etp, rParam.P_TrainData, rParam.P_TestData);
						//ysa, (fann_train_enum)Method->value(),(fann_activationfunc_enum) Act[i],(fann_activationfunc_enum) Act[j],TrainData);
					if(mse < best_mse) {
						best_mse = mse;
						best_hi = hi;
						best_oi = oi;
					}
				}
			}
			rParam.BestHiddActF = activation_func_list.get(best_hi);
			rParam.BestOutpActF = activation_func_list.get(best_oi);
			rParam.ResultFlags |= rParam.rfHiddActFuncDetected;
			rParam.ResultFlags |= rParam.rfOutpActFuncDetected;
		}
		if(rParam.Flags & rParam.fDetectTrainAlg) {
			uint   best_ai = 0;
			float  best_mse = MAXFLOAT;

			LongArray alg_list;
			alg_list.add(FANN_TRAIN_INCREMENTAL);
			alg_list.add(FANN_TRAIN_BATCH);
			alg_list.add(FANN_TRAIN_RPROP);
			alg_list.add(FANN_TRAIN_QUICKPROP);
			alg_list.add(FANN_TRAIN_SARPROP);
			for(uint ai = 0; ai < alg_list.getCount(); ai++) {
				test_ann.SetWeights(p_preserve_weights);
				ExamineTrainParam etp;
				etp.HiddActF = (Fann::ActivationFunc)((rParam.BestHiddActF >= 0) ? rParam.BestHiddActF : rParam.HiddActF);
				etp.OutpActF = (Fann::ActivationFunc)((rParam.BestOutpActF >= 0) ? rParam.BestOutpActF : rParam.OutpActF);
				etp.TrAlg = (Fann::TrainAlg)alg_list.get(ai);
				const float mse = test_ann.ExamineTrain(etp, rParam.P_TrainData, rParam.P_TestData);
					//ysa, (fann_train_enum)Method->value(),(fann_activationfunc_enum) Act[i],(fann_activationfunc_enum) Act[j],TrainData);
				if(mse < best_mse) {
					best_mse = mse;
					best_ai = ai;
				}
			}
			rParam.BestTrainAlg = alg_list.get(best_ai);
			rParam.ResultFlags |= rParam.rfTrainAlgDetected;
		}
	}
	CATCHZOK
	ZFREE(p_preserve_weights);
	return ok;

/*
  if(TrainData==NULL){
    fl_alert("Firstly Load Train Data !");
    return;
  }
  if(working) return;
  int best_ha,best_oa;
  Out->clear();
  ActivateStop();
  if(Layer->value()==5)
    ysa = fann_create_sparse(ConnectionRate->value(),5,(int)Input->value(),(int)Hid1->value(),(int)Hid2->value(),(int)Hid3->value(),(int)Output->value());
  if(Layer->value()==4)
    ysa = fann_create_sparse(ConnectionRate->value(),4,(int)Input->value(),(int)Hid1->value(),(int)Hid2->value(),(int)Output->value());
  else
    ysa = fann_create_sparse(ConnectionRate->value(),3,(int)Input->value(),(int)Hid1->value(),(int)Output->value());
  fann_type *w=GetWeigths(ysa);

  int best_ta;
  fann_type min=1,mse;
  char Buf[512];
  for(int  i=0; i<13 && !stop;i++) {
    for(int  j=0; j<13&& !stop;j++) {
    SetWeights(ysa,w);
      sprintf(Buf,"@C4Hid Activation Func. : %s --- Out Activation Func. : %s ",FANN_ACTIVATIONFUNC_NAMES[Act[i]],FANN_ACTIVATIONFUNC_NAMES[Act[j]]);
      Out->add(Buf);
      mse=ExamineTrain(ysa,(fann_train_enum)Method->value(),(fann_activationfunc_enum) Act[i],(fann_activationfunc_enum) Act[j],TrainData);
      if(mse<min){
         min=mse;
         best_ha=i;
         best_oa=j;

      }
    }
  }
  */
}

//
// parallel_FANN.c
//    Author: Alessandro Pietro Bardelli
//
#ifndef DISABLE_PARALLEL_FANN
#include <omp.h>
//#include "parallel_fann.h"
//#include "config.h"
//#include "fann.h"

FANN_EXTERNAL float FANN_API fann_train_epoch_batch_parallel(Fann * ann, Fann::TrainData * data, const uint threadnumb)
{
	/*vector<Fann *> ann_vect(threadnumb);*/
	Fann** ann_vect = (Fann**)SAlloc::M(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	ann->ResetMSE();
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i = 0; i<(int)threadnumb; i++) {
			ann_vect[i] = fann_copy(ann);
		}
		//parallel computing of the updates
	#pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j = omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	//parallel update of the weights
	{
		const uint num_data = data->num_data;
		const uint first_weight = 0;
		const uint past_end = ann->TotalConnections;
		ANNTYP * weights = ann->P_Weights;
		const ANNTYP epsilon = ann->LearningRate / num_data;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				ANNTYP temp_slopes = 0.0;
				uint k;
				ANNTYP * train_slopes;
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->P_TrainSlopes;
					temp_slopes += train_slopes[i];
					train_slopes[i] = 0.0;
				}
				weights[i] += temp_slopes*epsilon;
			}
		}
	}
	//merge of MSEs
	for(i = 0; i<(int)threadnumb; ++i) {
		ann->MSE_value += ann_vect[i]->MSE_value;
		ann->num_MSE += ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	SAlloc::F(ann_vect);
	return ann->GetMSE();
}

FANN_EXTERNAL float FANN_API fann_train_epoch_irpropm_parallel(Fann * ann, Fann::TrainData * data, const uint threadnumb)
{
	Fann** ann_vect = (Fann**)SAlloc::M(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
	//#define THREADNUM 1
	ann->ResetMSE();
	/*vector<Fann *> ann_vect(threadnumb);*/
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i = 0; i<(int)threadnumb; i++) {
			ann_vect[i] = fann_copy(ann);
		}
		//parallel computing of the updates
	#pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j = omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	{
		ANNTYP * weights = ann->P_Weights;
		ANNTYP * prev_steps = ann->P_PrevSteps;
		ANNTYP * prev_train_slopes = ann->P_PrevTrainSlopes;
		ANNTYP next_step;
		const float increase_factor = ann->RpropIncreaseFactor; //1.2;
		const float decrease_factor = ann->RpropDecreaseFactor; //0.5;
		const float delta_min = ann->RpropDeltaMin; //0.0;
		const float delta_max = ann->RpropDeltaMax; //50.0;
		const uint first_weight = 0;
		const uint past_end = ann->TotalConnections;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				ANNTYP prev_slope, same_sign;
				const ANNTYP prev_step = MAX(prev_steps[i], (ANNTYP)0.0001); // prev_step may not be zero because
					// then the training will stop
				ANNTYP temp_slopes = 0.0;
				uint k;
				ANNTYP * train_slopes;
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->P_TrainSlopes;
					temp_slopes += train_slopes[i];
					train_slopes[i] = 0.0;
				}
				prev_slope = prev_train_slopes[i];
				same_sign = prev_slope * temp_slopes;
				if(same_sign >= 0.0)
					next_step = MIN(prev_step * increase_factor, delta_max);
				else {
					next_step = MAX(prev_step * decrease_factor, delta_min);
					temp_slopes = 0;
				}
				if(temp_slopes < 0) {
					weights[i] -= next_step;
					SETMAX(weights[i], -1500);
				}
				else {
					weights[i] += next_step;
					SETMIN(weights[i], 1500);
				}
				// update global data arrays
				prev_steps[i] = next_step;
				prev_train_slopes[i] = temp_slopes;
			}
		}
	}
	//merge of MSEs
	for(i = 0; i<(int)threadnumb; ++i) {
		ann->MSE_value += ann_vect[i]->MSE_value;
		ann->num_MSE += ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	SAlloc::F(ann_vect);
	return ann->GetMSE();
}

FANN_EXTERNAL float FANN_API fann_train_epoch_quickprop_parallel(Fann * ann, Fann::TrainData * data, const uint threadnumb)
{
	Fann** ann_vect = (Fann**)SAlloc::M(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
	//#define THREADNUM 1
	ann->ResetMSE();
	/*vector<Fann *> ann_vect(threadnumb);*/
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i = 0; i<(int)threadnumb; i++) {
			ann_vect[i] = fann_copy(ann);
		}
		//parallel computing of the updates
	#pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j = omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	{
		ANNTYP * weights = ann->P_Weights;
		ANNTYP * prev_steps = ann->P_PrevSteps;
		ANNTYP * prev_train_slopes = ann->P_PrevTrainSlopes;
		const uint first_weight = 0;
		const uint past_end = ann->TotalConnections;

		ANNTYP w = 0.0, next_step;

		const float epsilon = ann->LearningRate / data->num_data;
		const float decay = ann->QuickpropDecay; // -0.0001
		const float mu = ann->QuickpropMu; // 1.75
		const float shrink_factor = (float)(mu / (1.0 + mu));

		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(w, next_step)
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				ANNTYP temp_slopes = 0.0;
				uint k;
				ANNTYP * train_slopes;
				ANNTYP prev_step, prev_slope;
				w = weights[i];
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->P_TrainSlopes;
					temp_slopes += train_slopes[i];
					train_slopes[i] = 0.0;
				}
				temp_slopes += decay * w;
				prev_step = prev_steps[i];
				prev_slope = prev_train_slopes[i];
				next_step = 0.0;
				/* The step must always be in direction opposite to the slope. */
				if(prev_step > 0.001) {
					// If last step was positive...
					if(temp_slopes > 0.0) // Add in linear term if current slope is still positive
						next_step += epsilon * temp_slopes;
					// If current slope is close to or larger than prev slope...
					if(temp_slopes > (shrink_factor * prev_slope))
						next_step += mu * prev_step; /* Take maximum size negative step. */
					else
						next_step += prev_step * temp_slopes / (prev_slope - temp_slopes); // Else, use quadratic estimate
				}
				else if(prev_step < -0.001) {
					/* If last step was negative...  */
					if(temp_slopes < 0.0) // Add in linear term if current slope is still negative.
						next_step += epsilon * temp_slopes;
					/* If current slope is close to or more neg than prev slope... */
					if(temp_slopes < (shrink_factor * prev_slope))
						next_step += mu * prev_step; // Take maximum size negative step
					else
						next_step += prev_step * temp_slopes / (prev_slope - temp_slopes); // Else, use quadratic estimate
				}
				else // Last step was zero, so use only linear term
					next_step += epsilon * temp_slopes;
				/* update global data arrays */
				prev_steps[i] = next_step;
				prev_train_slopes[i] = temp_slopes;
				w += next_step;
				if(w > 1500)
					weights[i] = 1500;
				else if(w < -1500)
					weights[i] = -1500;
				else
					weights[i] = w;
			}
		}
	}
	//merge of MSEs
	for(i = 0; i<(int)threadnumb; ++i) {
		ann->MSE_value += ann_vect[i]->MSE_value;
		ann->num_MSE += ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	SAlloc::F(ann_vect);
	return ann->GetMSE();
}

FANN_EXTERNAL float FANN_API fann_train_epoch_sarprop_parallel(Fann * ann, Fann::TrainData * data, const uint threadnumb)
{
	Fann** ann_vect = (Fann**)SAlloc::M(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
	//#define THREADNUM 1
	ann->ResetMSE();
	/*vector<Fann *> ann_vect(threadnumb);*/

	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i = 0; i<(int)threadnumb; i++) {
			ann_vect[i] = fann_copy(ann);
		}
		//parallel computing of the updates
#pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j = omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	{
		ANNTYP * weights = ann->P_Weights;
		ANNTYP * prev_steps = ann->P_PrevSteps;
		ANNTYP * prev_train_slopes = ann->P_PrevTrainSlopes;
		const uint first_weight = 0;
		const uint past_end = ann->TotalConnections;
		const uint epoch = ann->SarpropEpoch;

		ANNTYP next_step;

		/* These should be set from variables */
		const float increase_factor = ann->RpropIncreaseFactor; /*1.2; */
		const float decrease_factor = ann->RpropDecreaseFactor; /*0.5; */
		/* TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993) */
		const float delta_min = 0.000001f;
		const float delta_max = ann->RpropDeltaMax; /*50.0; */
		const float weight_decay_shift = ann->SarpropWeightDecayShift; /* ld 0.01 = -6.644 */
		const float step_error_threshold_factor = ann->SarpropStepErrorThresholdFactor; /* 0.1 */
		const float step_error_shift = ann->SarpropStepErrorShift; /* ld 3 = 1.585 */
		const float T = ann->SarpropTemperature;
		float MSE, RMSE;
		//merge of MSEs
		for(i = 0; i<(int)threadnumb; ++i) {
			ann->MSE_value += ann_vect[i]->MSE_value;
			ann->num_MSE += ann_vect[i]->num_MSE;
		}
		MSE = ann->GetMSE();
		RMSE = sqrtf(MSE);
		// for all weights; TODO: are biases included?
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				/* TODO: confirm whether 1x10^-6 == delta_min is really better */
				const ANNTYP prev_step  = MAX(prev_steps[i], (ANNTYP)0.000001); // prev_step may not be zero because then the training will stop
				// calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)
				ANNTYP prev_slope, same_sign;
				ANNTYP temp_slopes = 0.0;
				uint k;
				ANNTYP * train_slopes;
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->P_TrainSlopes;
					temp_slopes += train_slopes[i];
					train_slopes[i] = 0.0;
				}
				temp_slopes = -temp_slopes - weights[i] * (ANNTYP)fann_exp2(-T * epoch + weight_decay_shift);
				next_step = 0.0;
				// TODO: is prev_train_slopes[i] 0.0 in the beginning?
				prev_slope = prev_train_slopes[i];
				same_sign = prev_slope * temp_slopes;
				if(same_sign > 0.0) {
					next_step = MIN(prev_step * increase_factor, delta_max);
					// TODO: are the signs inverted? see differences between SARPROP paper and iRprop
					if(temp_slopes < 0.0)
						weights[i] += next_step;
					else
						weights[i] -= next_step;
				}
				else if(same_sign < 0.0) {
#ifndef RAND_MAX
	#define RAND_MAX        0x7fffffff
#endif
					if(prev_step < step_error_threshold_factor * MSE)
						next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (ANNTYP)fann_exp2(-T * epoch + step_error_shift);
					else
						next_step = MAX(prev_step * decrease_factor, delta_min);
					temp_slopes = 0.0;
				}
				else {
					if(temp_slopes < 0.0)
						weights[i] += prev_step;
					else
						weights[i] -= prev_step;
				}
				/* update global data arrays */
				prev_steps[i] = next_step;
				prev_train_slopes[i] = temp_slopes;
			}
		}
	}
	++(ann->SarpropEpoch);
	//already computed before
	/*/ /merge of MSEs
	for(i = 0; i<threadnumb; ++i) {
		ann->MSE_value += ann_vect[i]->MSE_value;
		ann->num_MSE += ann_vect[i]->num_MSE;
	}
	*/
	//destroy the copies of the ann
	for(i = 0; i<(int)threadnumb; i++) {
		fann_destroy(ann_vect[i]);
	}
	SAlloc::F(ann_vect);
	return ann->GetMSE();
}

FANN_EXTERNAL float FANN_API fann_train_epoch_incremental_mod(Fann * ann, Fann::TrainData * data)
{
	ann->ResetMSE();
	for(uint i = 0; i != data->num_data; i++)
		ann->Train(data->input[i], data->output[i]);
	return ann->GetMSE();
}

#endif // DISABLE_PARALLEL_FANN
//
// parallel_FANN.cpp
//    Author: Alessandro Pietro Bardelli
//
#ifndef DISABLE_PARALLEL_FANN
//#include "parallel_fann.hpp"
#include <omp.h>
using namespace std;
namespace parallel_fann {
// TODO rewrite all these functions in c++ using fann_cpp interface

float train_epoch_batch_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb)
{
	ann->ResetMSE();
	vector <Fann *> ann_vect(threadnumb);
	int i=0,j=0;
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++) {
			ann_vect[i]=fann_copy(ann);
		}
    //parallel computing of the updates
        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j=omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    //parallel update of the weights
	{
		const uint num_data=data->num_data;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;
		ANNTYP *weights = ann->P_Weights;
		const ANNTYP epsilon = ann->LearningRate / num_data;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					ANNTYP temp_slopes=0.0;
					uint k;
					ANNTYP *train_slopes;
					for(k=0; k < threadnumb; ++k) {
						train_slopes=ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					weights[i] += temp_slopes*epsilon;
				}
			}
	}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_irpropm_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb)
{
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
	//#define THREADNUM 1
	ann->ResetMSE();
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++) {
			ann_vect[i]=fann_copy(ann);
		}
    //parallel computing of the updates
        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j=omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	{
    	ANNTYP *weights = ann->P_Weights;
    	ANNTYP *prev_steps = ann->P_PrevSteps;
    	ANNTYP *prev_train_slopes = ann->P_PrevTrainSlopes;
    	ANNTYP next_step;
    	const float increase_factor = ann->RpropIncreaseFactor;	//1.2;
    	const float decrease_factor = ann->RpropDecreaseFactor;	//0.5;
    	const float delta_min = ann->RpropDeltaMin;	//0.0;
    	const float delta_max = ann->RpropDeltaMax;	//50.0;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;

		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
		    		const ANNTYP prev_step = MAX(prev_steps[i], (ANNTYP) 0.0001);	// prev_step may not be zero because then the training will stop
		    		ANNTYP temp_slopes=0.0;
					uint k;
					ANNTYP *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
		    		const ANNTYP prev_slope = prev_train_slopes[i];
		    		const ANNTYP same_sign = prev_slope * temp_slopes;
		    		if(same_sign >= 0.0)
		    			next_step = MIN(prev_step * increase_factor, delta_max);
		    		else {
		    			next_step = MAX(prev_step * decrease_factor, delta_min);
		    			temp_slopes = 0;
		    		}
		    		if(temp_slopes < 0) {
		    			weights[i] -= next_step;
		    			SETMAX(weights[i], -1500);
		    		}
		    		else {
		    			weights[i] += next_step;
		    			SETMIN(weights[i], 1500);
		    		}
		    		// update global data arrays
		    		prev_steps[i] = next_step;
		    		prev_train_slopes[i] = temp_slopes;
				}
			}
	}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_quickprop_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb)
{
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
	//#define THREADNUM 1
	ann->ResetMSE();
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++) {
			ann_vect[i]=fann_copy(ann);
		}
    //parallel computing of the updates
        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j=omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    {
    	ANNTYP *weights = ann->P_Weights;
    	ANNTYP *prev_steps = ann->P_PrevSteps;
    	ANNTYP *prev_train_slopes = ann->P_PrevTrainSlopes;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;
    	ANNTYP w=0.0, next_step;
    	const float epsilon = ann->LearningRate / data->num_data;
    	const float decay = ann->QuickpropDecay; // -0.0001
    	const float mu = ann->QuickpropMu; // 1.75
    	const float shrink_factor = (float) (mu / (1.0 + mu));
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(w, next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					w = weights[i];
					ANNTYP temp_slopes=0.0;
					uint k;
					ANNTYP *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes+= decay * w;
					const ANNTYP prev_step = prev_steps[i];
					const ANNTYP prev_slope = prev_train_slopes[i];
					next_step = 0.0;
					/* The step must always be in direction opposite to the slope. */
					if(prev_step > 0.001) {
						/* If last step was positive...  */
						if(temp_slopes > 0.0) /*  Add in linear term if current slope is still positive. */
							next_step += epsilon * temp_slopes;
						/*If current slope is close to or larger than prev slope...  */
						if(temp_slopes > (shrink_factor * prev_slope))
							next_step += mu * prev_step;	/* Take maximum size negative step. */
						else
							next_step += prev_step * temp_slopes / (prev_slope - temp_slopes);	/* Else, use quadratic estimate. */
					}
					else if(prev_step < -0.001) {
						/* If last step was negative...  */
						if(temp_slopes < 0.0) /*  Add in linear term if current slope is still negative. */
							next_step += epsilon * temp_slopes;
						/* If current slope is close to or more neg than prev slope... */
						if(temp_slopes < (shrink_factor * prev_slope))
							next_step += mu * prev_step;	/* Take maximum size negative step. */
						else
							next_step += prev_step * temp_slopes / (prev_slope - temp_slopes);	/* Else, use quadratic estimate. */
					}
					else /* Last step was zero, so use only linear term. */
						next_step += epsilon * temp_slopes;
					/* update global data arrays */
					prev_steps[i] = next_step;
					prev_train_slopes[i] = temp_slopes;
					w += next_step;
					if(w > 1500)
						weights[i] = 1500;
					else if(w < -1500)
						weights[i] = -1500;
					else
						weights[i] = w;
				}
		}
	}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_sarprop_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb)
{
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
	//#define THREADNUM 1
	ann->ResetMSE();
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{
		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++) {
			ann_vect[i]=fann_copy(ann);
		}
    //parallel computing of the updates
        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j=omp_get_thread_num();
			ann_vect[j]->Run(data->input[i]);
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    {
    	ANNTYP *weights = ann->P_Weights;
    	ANNTYP *prev_steps = ann->P_PrevSteps;
    	ANNTYP *prev_train_slopes = ann->P_PrevTrainSlopes;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;
		const uint epoch=ann->SarpropEpoch;
    	ANNTYP next_step;
    	/* These should be set from variables */
    	const float increase_factor = ann->RpropIncreaseFactor;	/*1.2; */
    	const float decrease_factor = ann->RpropDecreaseFactor;	/*0.5; */
    	/* TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993) */
    	const float delta_min = 0.000001f;
    	const float delta_max = ann->RpropDeltaMax;	/*50.0; */
    	const float weight_decay_shift = ann->SarpropWeightDecayShift; /* ld 0.01 = -6.644 */
    	const float step_error_threshold_factor = ann->SarpropStepErrorThresholdFactor; /* 0.1 */
    	const float step_error_shift = ann->SarpropStepErrorShift; /* ld 3 = 1.585 */
    	const float T = ann->SarpropTemperature;
    	//merge of MSEs
    	for(i=0;i<(int)threadnumb;++i) {
    		ann->MSE_value+= ann_vect[i]->MSE_value;
    		ann->num_MSE+=ann_vect[i]->num_MSE;
    	}
    	const float MSE = ann->GetMSE();
    	const float RMSE = sqrtf(MSE);
    	/* for all weights; TODO: are biases included? */
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					/* TODO: confirm whether 1x10^-6 == delta_min is really better */
					const ANNTYP prev_step  = MAX(prev_steps[i], (ANNTYP) 0.000001);	/* prev_step may not be zero because then the training will stop */
					/* calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)*/
					ANNTYP temp_slopes=0.0;
					uint k;
					ANNTYP *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes= -temp_slopes - weights[i] * (ANNTYP)fann_exp2(-T * epoch + weight_decay_shift);
					next_step=0.0;
					/* TODO: is prev_train_slopes[i] 0.0 in the beginning? */
					const ANNTYP prev_slope = prev_train_slopes[i];
					const ANNTYP same_sign = prev_slope * temp_slopes;
					if(same_sign > 0.0) {
						next_step = MIN(prev_step * increase_factor, delta_max);
						/* TODO: are the signs inverted? see differences between SARPROP paper and iRprop */
						if(temp_slopes < 0.0)
							weights[i] += next_step;
						else
							weights[i] -= next_step;
					}
					else if(same_sign < 0.0) {
#ifndef RAND_MAX
	#define	RAND_MAX	0x7fffffff
#endif
						if(prev_step < step_error_threshold_factor * MSE)
							next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (ANNTYP)fann_exp2(-T * epoch + step_error_shift);
						else
							next_step = MAX(prev_step * decrease_factor, delta_min);
						temp_slopes = 0.0;
					}
					else {
						if(temp_slopes < 0.0)
							weights[i] += prev_step;
						else
							weights[i] -= prev_step;
					}
					/* update global data arrays */
					prev_steps[i] = next_step;
					prev_train_slopes[i] = temp_slopes;
				}
		}
    }
	++(ann->SarpropEpoch);
	//already computed before
	/*//merge of MSEs
	for(i=0;i<threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
	}*/
	//destroy the copies of the ann
	for(i=0; i<(int)threadnumb; i++) {
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_incremental_mod(Fann *ann, Fann::TrainData *data)
{
	ann->ResetMSE();
	for(uint i = 0; i != data->num_data; i++) {
		ann->Train(data->input[i], data->output[i]);
	}
	return ann->GetMSE();
}

//the following versions returns also the outputs via the predicted_outputs parameter

float train_epoch_batch_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb,vector< vector<ANNTYP> >& predicted_outputs)
{
	ann->ResetMSE();
	predicted_outputs.resize(data->num_data,vector<ANNTYP> (data->num_output));
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{

		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++) {
			ann_vect[i]=fann_copy(ann);
		}
    //parallel computing of the updates
        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j=omp_get_thread_num();
			ANNTYP* temp_predicted_output=ann_vect[j]->Run(data->input[i]);
			for(uint k=0;k<data->num_output;++k) {
				predicted_outputs[i][k]=temp_predicted_output[k];
			}
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    //parallel update of the weights
	{
		const uint num_data=data->num_data;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;
		ANNTYP *weights = ann->P_Weights;
		const ANNTYP epsilon = ann->LearningRate / num_data;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					ANNTYP temp_slopes=0.0;
					ANNTYP * train_slopes;
					for(uint k = 0; k < threadnumb; ++k) {
						train_slopes = ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					weights[i] += temp_slopes*epsilon;
				}
			}
	}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_irpropm_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb, vector< vector<ANNTYP> >& predicted_outputs)
{
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
		ann->ResetMSE();
		predicted_outputs.resize(data->num_data,vector<ANNTYP> (data->num_output));
		vector<Fann *> ann_vect(threadnumb);
		int i=0,j=0;
		//generate copies of the ann
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(j)
		{

			#pragma omp for schedule(static)
			for(i=0; i<(int)threadnumb; i++) {
				ann_vect[i]=fann_copy(ann);
			}
	    //parallel computing of the updates
	        #pragma omp for schedule(static)
			for(i = 0; i < (int)data->num_data; i++) {
				j=omp_get_thread_num();
				ANNTYP * temp_predicted_output=ann_vect[j]->Run(data->input[i]);
				for(uint k=0;k<data->num_output;++k) {
					predicted_outputs[i][k]=temp_predicted_output[k];
				}
				ann_vect[j]->ComputeMSE(data->output[i]);
				ann_vect[j]->BackpropagateMSE();
				ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
			}
	}
	{
    	ANNTYP *weights = ann->P_Weights;
    	ANNTYP *prev_steps = ann->P_PrevSteps;
    	ANNTYP *prev_train_slopes = ann->P_PrevTrainSlopes;
    	ANNTYP next_step;
    	const float increase_factor = ann->RpropIncreaseFactor;	//1.2;
    	const float decrease_factor = ann->RpropDecreaseFactor;	//0.5;
    	const float delta_min = ann->RpropDeltaMin;	//0.0;
    	const float delta_max = ann->RpropDeltaMax;	//50.0;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
		    		const ANNTYP prev_step = MAX(prev_steps[i], (ANNTYP) 0.0001);	// prev_step may not be zero because then the training will stop
		    		ANNTYP temp_slopes=0.0;
					uint k;
					ANNTYP *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
		    		const ANNTYP prev_slope = prev_train_slopes[i];
		    		const ANNTYP same_sign = prev_slope * temp_slopes;
		    		if(same_sign >= 0.0)
		    			next_step = MIN(prev_step * increase_factor, delta_max);
		    		else {
		    			next_step = MAX(prev_step * decrease_factor, delta_min);
		    			temp_slopes = 0;
		    		}
		    		if(temp_slopes < 0) {
		    			weights[i] -= next_step;
		    			SETMAX(weights[i], -1500);
		    		}
		    		else {
		    			weights[i] += next_step;
		    			SETMIN(weights[i], 1500);
		    		}
		    		// update global data arrays
		    		prev_steps[i] = next_step;
		    		prev_train_slopes[i] = temp_slopes;

				}
			}
	}
	//merge of MSEs
	for(i = 0; i < (int)threadnumb; ++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_quickprop_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb, vector< vector<ANNTYP> >& predicted_outputs)
{
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
		ann->ResetMSE();
		predicted_outputs.resize(data->num_data,vector<ANNTYP> (data->num_output));
		vector<Fann *> ann_vect(threadnumb);
		int i=0,j=0;
		//generate copies of the ann
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(j)
		{
			#pragma omp for schedule(static)
			for(i=0; i<(int)threadnumb; i++) {
				ann_vect[i]=fann_copy(ann);
			}
	    //parallel computing of the updates
	        #pragma omp for schedule(static)
			for(i = 0; i < (int)data->num_data; i++) {
				j=omp_get_thread_num();
				ANNTYP* temp_predicted_output=ann_vect[j]->Run(data->input[i]);
				for(uint k=0;k<data->num_output;++k) {
					predicted_outputs[i][k]=temp_predicted_output[k];
				}
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    {
    	ANNTYP *weights = ann->P_Weights;
    	ANNTYP *prev_steps = ann->P_PrevSteps;
    	ANNTYP *prev_train_slopes = ann->P_PrevTrainSlopes;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;
    	ANNTYP w=0.0, next_step;
    	const float epsilon = ann->LearningRate / data->num_data;
    	const float decay = ann->QuickpropDecay; // -0.0001
    	const float mu = ann->QuickpropMu; // 1.75
    	const float shrink_factor = (float) (mu / (1.0 + mu));
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(w, next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					w = weights[i];
					ANNTYP temp_slopes=0.0;
					uint k;
					ANNTYP *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes+= decay * w;

					const ANNTYP prev_step = prev_steps[i];
					const ANNTYP prev_slope = prev_train_slopes[i];
					next_step = 0.0;
					/* The step must always be in direction opposite to the slope. */
					if(prev_step > 0.001) {
						/* If last step was positive...  */
						if(temp_slopes > 0.0) /*  Add in linear term if current slope is still positive. */
							next_step += epsilon * temp_slopes;
						/*If current slope is close to or larger than prev slope...  */
						if(temp_slopes > (shrink_factor * prev_slope))
							next_step += mu * prev_step;	/* Take maximum size negative step. */
						else
							next_step += prev_step * temp_slopes / (prev_slope - temp_slopes);	/* Else, use quadratic estimate. */
					}
					else if(prev_step < -0.001) {
						/* If last step was negative...  */
						if(temp_slopes < 0.0) /*  Add in linear term if current slope is still negative. */
							next_step += epsilon * temp_slopes;

						/* If current slope is close to or more neg than prev slope... */
						if(temp_slopes < (shrink_factor * prev_slope))
							next_step += mu * prev_step;	/* Take maximum size negative step. */
						else
							next_step += prev_step * temp_slopes / (prev_slope - temp_slopes);	/* Else, use quadratic estimate. */
					}
					else /* Last step was zero, so use only linear term. */
						next_step += epsilon * temp_slopes;

					/* update global data arrays */
					prev_steps[i] = next_step;
					prev_train_slopes[i] = temp_slopes;

					w += next_step;

					if(w > 1500)
						weights[i] = 1500;
					else if(w < -1500)
						weights[i] = -1500;
					else
						weights[i] = w;
				}
		}
	}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_sarprop_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb, vector< vector<ANNTYP> >& predicted_outputs)
{
	if(ann->P_PrevTrainSlopes == NULL) {
		ann->ClearTrainArrays();
	}
		ann->ResetMSE();
		predicted_outputs.resize(data->num_data,vector<ANNTYP> (data->num_output));
		vector<Fann *> ann_vect(threadnumb);
		int i=0,j=0;

		//generate copies of the ann
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(j)
		{
			#pragma omp for schedule(static)
			for(i=0; i<(int)threadnumb; i++) {
				ann_vect[i]=fann_copy(ann);
			}

	    //parallel computing of the updates
	        #pragma omp for schedule(static)
			for(i = 0; i < (int)data->num_data; i++) {
				j=omp_get_thread_num();
				ANNTYP* temp_predicted_output=ann_vect[j]->Run(data->input[i]);
				for(uint k=0;k<data->num_output;++k) {
					predicted_outputs[i][k]=temp_predicted_output[k];
				}
			ann_vect[j]->ComputeMSE(data->output[i]);
			ann_vect[j]->BackpropagateMSE();
			ann_vect[j]->UpdateSlopesBatch(ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    {
    	ANNTYP *weights = ann->P_Weights;
    	ANNTYP *prev_steps = ann->P_PrevSteps;
    	ANNTYP *prev_train_slopes = ann->P_PrevTrainSlopes;
		const uint first_weight=0;
		const uint past_end=ann->TotalConnections;
		const uint epoch=ann->SarpropEpoch;
    	ANNTYP next_step;
    	/* These should be set from variables */
    	const float increase_factor = ann->RpropIncreaseFactor;	/*1.2; */
    	const float decrease_factor = ann->RpropDecreaseFactor;	/*0.5; */
    	/* TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993) */
    	const float delta_min = 0.000001f;
    	const float delta_max = ann->RpropDeltaMax;	/*50.0; */
    	const float weight_decay_shift = ann->SarpropWeightDecayShift; /* ld 0.01 = -6.644 */
    	const float step_error_threshold_factor = ann->SarpropStepErrorThresholdFactor; /* 0.1 */
    	const float step_error_shift = ann->SarpropStepErrorShift; /* ld 3 = 1.585 */
    	const float T = ann->SarpropTemperature;
    	//merge of MSEs
    	for(i=0;i<(int)threadnumb;++i) {
    		ann->MSE_value+= ann_vect[i]->MSE_value;
    		ann->num_MSE+=ann_vect[i]->num_MSE;
    	}
    	const float MSE = ann->GetMSE();
    	const float RMSE = (float)sqrt(MSE);
    	/* for all weights; TODO: are biases included? */
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					/* TODO: confirm whether 1x10^-6 == delta_min is really better */
					const ANNTYP prev_step  = MAX(prev_steps[i], (ANNTYP) 0.000001);	/* prev_step may not be zero because then the training will stop */
					/* calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)*/
					ANNTYP temp_slopes=0.0;
					uint k;
					ANNTYP *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->P_TrainSlopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes= -temp_slopes - weights[i] * (ANNTYP)fann_exp2(-T * epoch + weight_decay_shift);
					next_step=0.0;
					/* TODO: is prev_train_slopes[i] 0.0 in the beginning? */
					const ANNTYP prev_slope = prev_train_slopes[i];
					const ANNTYP same_sign = prev_slope * temp_slopes;
					if(same_sign > 0.0) {
						next_step = MIN(prev_step * increase_factor, delta_max);
						/* TODO: are the signs inverted? see differences between SARPROP paper and iRprop */
						if (temp_slopes < 0.0)
							weights[i] += next_step;
						else
							weights[i] -= next_step;
					}
					else if(same_sign < 0.0) {
						#ifndef RAND_MAX
						#define	RAND_MAX	0x7fffffff
						#endif
						if(prev_step < step_error_threshold_factor * MSE)
							next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (ANNTYP)fann_exp2(-T * epoch + step_error_shift);
						else
							next_step = MAX(prev_step * decrease_factor, delta_min);

						temp_slopes = 0.0;
					}
					else {
						if(temp_slopes < 0.0)
							weights[i] += prev_step;
						else
							weights[i] -= prev_step;
					}

					/* update global data arrays */
					prev_steps[i] = next_step;
					prev_train_slopes[i] = temp_slopes;

				}
		}
    }
	++(ann->SarpropEpoch);
	//already computed before
	/*//merge of MSEs
	for(i=0;i<threadnumb;++i)
	{
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
	}*/
	//destroy the copies of the ann
	for(i=0; i<(int)threadnumb; i++) {
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float train_epoch_incremental_mod(Fann *ann, Fann::TrainData *data, vector< vector<ANNTYP> >& predicted_outputs)
{
	predicted_outputs.resize(data->num_data,vector<ANNTYP> (data->num_output));
	ann->ResetMSE();
	for(uint i = 0; i < data->num_data; ++i) {
		ANNTYP * temp_predicted_output=ann->Run(data->input[i]);
		for(uint k=0;k<data->num_output;++k) {
			predicted_outputs[i][k]=temp_predicted_output[k];
		}
		ann->ComputeMSE(data->output[i]);
		ann->BackpropagateMSE();
		ann->UpdateWeights();
	}
	return ann->GetMSE();
}

float test_data_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb)
{
	if(!ann->CheckInputOutputSizes(data))
		return 0;
	ann->ResetMSE();
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;

		//generate copies of the ann
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(j)
		{
			#pragma omp for schedule(static)
			for(i=0; i<(int)threadnumb; i++) {
				ann_vect[i]=fann_copy(ann);
			}
			//parallel computing of the updates
	        #pragma omp for schedule(static)
			for(i = 0; i < (int)data->num_data; ++i) {
				j=omp_get_thread_num();
				ann_vect[j]->Test(data->input[i],data->output[i]);
			}
		}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}

float test_data_parallel(Fann *ann, Fann::TrainData *data, const uint threadnumb, vector< vector<ANNTYP> >& predicted_outputs)
{
	if(!ann->CheckInputOutputSizes(data))
		return 0;
	predicted_outputs.resize(data->num_data,vector<ANNTYP> (data->num_output));
	ann->ResetMSE();
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;
		//generate copies of the ann
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(j)
		{
			#pragma omp for schedule(static)
			for(i=0; i<(int)threadnumb; i++) {
				ann_vect[i]=fann_copy(ann);
			}
			//parallel computing of the updates
	        #pragma omp for schedule(static)
			for(i = 0; i < (int)data->num_data; ++i) {
				j=omp_get_thread_num();
				ANNTYP* temp_predicted_output = ann_vect[j]->Test(data->input[i],data->output[i]);
				for(uint k=0;k<data->num_output;++k) {
					predicted_outputs[i][k]=temp_predicted_output[k];
				}

			}
		}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return ann->GetMSE();
}
} // namespace parallel_fann

#endif // DISABLE_PARALLEL_FANN

#if 0 // @construction {
//
// FANN_TEST.CPP
//
#include <vector>
//#include "fann_test.h"
#include "gtest/gtest.h"
#include "doublefann.h"
#include "fann_cpp.h"
//#include "fann_test_data.h"
#include "gtest/gtest.h"
#include "doublefann.h"
#include "fann_cpp.h"
//#include "fann_test_train.h"
//#include "fann_test.h"

using namespace FANN;

class FannTest : public testing::Test {
protected:
    neural_net net;
    training_data data;
    void AssertCreateAndCopy(neural_net &net, uint numLayers, const uint *layers, uint neurons,
		uint connections);
    void AssertCreate(neural_net &net, uint numLayers, const uint *layers,
		uint neurons, uint connections);
    void AssertWeights(neural_net &net, ANNTYP min, ANNTYP max, ANNTYP avg);
    virtual void SetUp();
    virtual void TearDown();
};

class FannTestData : public FannTest {
protected:
    uint numData;
    uint numInput;
    uint numOutput;
    ANNTYP inputValue;
    ANNTYP outputValue;
    ANNTYP **inputData;
    ANNTYP **outputData;
    virtual void SetUp();
    virtual void TearDown();
    void AssertTrainData(FANN::training_data &trainingData, uint numData, uint numInput,
		uint numOutput, ANNTYP inputValue, ANNTYP outputValue);
    void InitializeTrainDataStructure(uint numData, uint numInput, uint numOutput,
		ANNTYP inputValue, ANNTYP outputValue, ANNTYP **inputData, ANNTYP **outputData);
};

using namespace std;

void FannTest::SetUp()
{
    //ensure random generator is seeded at a known value to ensure reproducible results
    srand(0);
    fann_disable_seed_rand();
}

void FannTest::TearDown()
{
    net.destroy();
    data.destroy_train();
}

void FannTest::AssertCreate(neural_net &net, uint numLayers, const uint *layers,
	uint neurons, uint connections)
{
    EXPECT_EQ(numLayers, net.get_num_layers());
    EXPECT_EQ(layers[0], net.get_num_input());
    EXPECT_EQ(layers[numLayers - 1], net.get_num_output());
    uint *layers_res = new uint[numLayers];
    net.get_layer_array(layers_res);
    for(uint i = 0; i < numLayers; i++) {
        EXPECT_EQ(layers[i], layers_res[i]);
    }
    delete layers_res;
    EXPECT_EQ(neurons, net.get_total_neurons());
    EXPECT_EQ(connections, net.get_total_connections());
    AssertWeights(net, -0.09, 0.09, 0.0);
}

void FannTest::AssertCreateAndCopy(neural_net &net, uint numLayers, const uint *layers, uint neurons,
	uint connections)
{
    AssertCreate(net, numLayers, layers, neurons, connections);
    neural_net net_copy(net);
    AssertCreate(net_copy, numLayers, layers, neurons, connections);
}

void FannTest::AssertWeights(neural_net &net, ANNTYP min, ANNTYP max, ANNTYP avg)
{
    connection *connections = new connection[net.get_total_connections()];
    net.get_connection_array(connections);
    ANNTYP minWeight = connections[0].weight;
    ANNTYP maxWeight = connections[0].weight;
    ANNTYP totalWeight = 0.0;
    for(int i = 1; i < net.get_total_connections(); ++i) {
        if(connections[i].weight < minWeight)
            minWeight = connections[i].weight;
        if(connections[i].weight > maxWeight)
            maxWeight = connections[i].weight;
        totalWeight += connections[i].weight;
    }
    EXPECT_NEAR(min, minWeight, 0.05);
    EXPECT_NEAR(max, maxWeight, 0.05);
    EXPECT_NEAR(avg, totalWeight / (ANNTYP) net.get_total_connections(), 0.5);
}

TEST_F(FannTest, CreateStandardThreeLayers)
{
    neural_net net(LAYER, 3, 2, 3, 4);
    AssertCreateAndCopy(net, 3, (const uint[]) {2, 3, 4}, 11, 25);
}

TEST_F(FannTest, CreateStandardThreeLayersUsingCreateMethod)
{
    ASSERT_TRUE(net.create_standard(3, 2, 3, 4));
    uint layers[] = {2, 3, 4};
    AssertCreateAndCopy(net, 3, layers, 11, 25);
}

TEST_F(FannTest, CreateStandardFourLayersArray)
{
    uint layers[] = {2, 3, 4, 5};
    neural_net net(LAYER, 4, layers);
    AssertCreateAndCopy(net, 4, layers, 17, 50);
}

TEST_F(FannTest, CreateStandardFourLayersArrayUsingCreateMethod)
{
    uint layers[] = {2, 3, 4, 5};
    ASSERT_TRUE(net.create_standard_array(4, layers));
    AssertCreateAndCopy(net, 4, layers, 17, 50);
}

TEST_F(FannTest, CreateStandardFourLayersVector)
{
    vector<uint> layers{2, 3, 4, 5};
    neural_net net(LAYER, layers.begin(), layers.end());
    AssertCreateAndCopy(net, 4, layers.data(), 17, 50);
}

TEST_F(FannTest, CreateSparseFourLayers)
{
    neural_net net(0.5, 4, 2, 3, 4, 5);
    AssertCreateAndCopy(net, 4, (const uint[]){2, 3, 4, 5}, 17, 31);
}

TEST_F(FannTest, CreateSparseFourLayersUsingCreateMethod)
{
    ASSERT_TRUE(net.create_sparse(0.5f, 4, 2, 3, 4, 5));
    AssertCreateAndCopy(net, 4, (const uint[]){2, 3, 4, 5}, 17, 31);
}

TEST_F(FannTest, CreateSparseArrayFourLayers)
{
    uint layers[] = {2, 3, 4, 5};
    neural_net net(0.5f, 4, layers);
    AssertCreateAndCopy(net, 4, layers, 17, 31);
}

TEST_F(FannTest, CreateSparseArrayFourLayersUsingCreateMethod)
{
    uint layers[] = {2, 3, 4, 5};
    ASSERT_TRUE(net.create_sparse_array(0.5f, 4, layers));
    AssertCreateAndCopy(net, 4, layers, 17, 31);
}

TEST_F(FannTest, CreateSparseArrayWithMinimalConnectivity)
{
    uint layers[] = {2, 2, 2};
    neural_net net(0.01f, 3, layers);
    AssertCreateAndCopy(net, 3, layers, 8, 8);
}

TEST_F(FannTest, CreateShortcutFourLayers)
{
    neural_net net(SHORTCUT, 4, 2, 3, 4, 5);
    AssertCreateAndCopy(net, 4, (const uint[]){2, 3, 4, 5}, 15, 83);
    EXPECT_EQ(SHORTCUT, net.get_network_type());
}

TEST_F(FannTest, CreateShortcutFourLayersUsingCreateMethod)
{
    ASSERT_TRUE(net.create_shortcut(4, 2, 3, 4, 5));
    AssertCreateAndCopy(net, 4, (const uint[]){2, 3, 4, 5}, 15, 83);
    EXPECT_EQ(SHORTCUT, net.get_network_type());
}

TEST_F(FannTest, CreateShortcutArrayFourLayers)
{
    uint layers[] = {2, 3, 4, 5};
    neural_net net(SHORTCUT, 4, layers);
    AssertCreateAndCopy(net, 4, layers, 15, 83);
    EXPECT_EQ(SHORTCUT, net.get_network_type());
}

TEST_F(FannTest, CreateShortcutArrayFourLayersUsingCreateMethod)
{
    uint layers[] = {2, 3, 4, 5};
    ASSERT_TRUE(net.create_shortcut_array(4, layers));
    AssertCreateAndCopy(net, 4, layers, 15, 83);
    EXPECT_EQ(SHORTCUT, net.get_network_type());
}

TEST_F(FannTest, CreateFromFile)
{
    ASSERT_TRUE(net.create_standard(3, 2, 3, 4));
    neural_net netToBeSaved(LAYER, 3, 2, 3, 4);
    ASSERT_TRUE(netToBeSaved.save("tmpfile"));
    neural_net netToBeLoaded("tmpfile");
    AssertCreateAndCopy(netToBeLoaded, 3, (const uint[]){2, 3, 4}, 11, 25);
}

TEST_F(FannTest, CreateFromFileUsingCreateMethod)
{
    ASSERT_TRUE(net.create_standard(3, 2, 3, 4));
    neural_net inputNet(LAYER, 3, 2, 3, 4);
    ASSERT_TRUE(inputNet.save("tmpfile"));
    ASSERT_TRUE(net.create_from_file("tmpfile"));
    AssertCreateAndCopy(net, 3, (const uint[]){2, 3, 4}, 11, 25);
}

TEST_F(FannTest, RandomizeWeights) {
    neural_net net(LAYER, 2, 20, 10);
    net.randomize_weights(-1.0, 1.0);
    AssertWeights(net, -1.0, 1.0, 0);
}
//
// FANN_TEST_DATA.CPP
//
void FannTestData::SetUp()
{
    FannTest::SetUp();
    numData = 2;
    numInput = 3;
    numOutput = 1;
    inputValue = 1.1;
    outputValue = 2.2;
    inputData = new ANNTYP *[numData];
    outputData = new ANNTYP *[numData];
    InitializeTrainDataStructure(numData, numInput, numOutput, inputValue, outputValue, inputData, outputData);
}

void FannTestData::TearDown()
{
    FannTest::TearDown();
    delete(inputData);
    delete(outputData);
}

void FannTestData::InitializeTrainDataStructure(uint numData,
	uint numInput, uint numOutput, ANNTYP inputValue, ANNTYP outputValue,
	ANNTYP **inputData, ANNTYP **outputData)
{
    for (uint i = 0; i < numData; i++) {
        inputData[i] = new ANNTYP[numInput];
        outputData[i] = new ANNTYP[numOutput];
        for (uint j = 0; j < numInput; j++)
            inputData[i][j] = inputValue;
        for (uint j = 0; j < numOutput; j++)
            outputData[i][j] = outputValue;
    }
}

void FannTestData::AssertTrainData(training_data &trainingData, uint numData, uint numInput,
	uint numOutput, ANNTYP inputValue, ANNTYP outputValue)
{
    EXPECT_EQ(numData, trainingData.length_train_data());
    EXPECT_EQ(numInput, trainingData.num_input_train_data());
    EXPECT_EQ(numOutput, trainingData.num_output_train_data());
    for (int i = 0; i < numData; i++) {
        for (int j = 0; j < numInput; j++)
                EXPECT_DOUBLE_EQ(inputValue, trainingData.get_input()[i][j]);
        for (int j = 0; j < numOutput; j++)
                EXPECT_DOUBLE_EQ(outputValue, trainingData.get_output()[i][j]);
    }
}

TEST_F(FannTestData, CreateTrainDataFromPointerArrays)
{
    data.set_train_data(numData, numInput, inputData, numOutput, outputData);
    AssertTrainData(data, numData, numInput, numOutput, inputValue, outputValue);
}

TEST_F(FannTestData, CreateTrainDataFromArrays)
{
    ANNTYP input[] = {inputValue, inputValue, inputValue, inputValue, inputValue, inputValue};
    ANNTYP output[] = {outputValue, outputValue};
    data.set_train_data(numData, numInput, input, numOutput, output);
    AssertTrainData(data, numData, numInput, numOutput, inputValue, outputValue);
}

TEST_F(FannTestData, CreateTrainDataFromCopy)
{
    data.set_train_data(numData, numInput, inputData, numOutput, outputData);
    training_data dataCopy(data);
    AssertTrainData(dataCopy, numData, numInput, numOutput, inputValue, outputValue);
}

TEST_F(FannTestData, CreateTrainDataFromFile)
{
    data.set_train_data(numData, numInput, inputData, numOutput, outputData);
    data.save_train("tmpFile");
    training_data dataCopy;
    dataCopy.read_train_from_file("tmpFile");
    AssertTrainData(dataCopy, numData, numInput, numOutput, inputValue, outputValue);
}

void callBack(uint pos, uint numInput, uint numOutput, ANNTYP *input, ANNTYP *output)
{
    for(uint i = 0; i < numInput; i++)
        input[i] = (ANNTYP) 1.2;
    for(uint i = 0; i < numOutput; i++)
        output[i] = (ANNTYP) 2.3;
}

TEST_F(FannTestData, CreateTrainDataFromCallback)
{
    data.create_train_from_callback(numData, numInput, numOutput, callBack);
    AssertTrainData(data, numData, numInput, numOutput, 1.2, 2.3);
}

TEST_F(FannTestData, ShuffleTrainData)
{
    //only really ensures that the data doesn't get corrupted, a more complete test would need to check
    //that this was indeed a permutation of the original data
    data.set_train_data(numData, numInput, inputData, numOutput, outputData);
    data.shuffle_train_data();
    AssertTrainData(data, numData, numInput, numOutput, inputValue, outputValue);
}

TEST_F(FannTestData, MergeTrainData)
{
    data.set_train_data(numData, numInput, inputData, numOutput, outputData);
    training_data dataCopy(data);
    data.merge_train_data(dataCopy);
    AssertTrainData(data, numData*2, numInput, numOutput, inputValue, outputValue);
}

TEST_F(FannTestData, SubsetTrainData)
{
    data.set_train_data(numData, numInput, inputData, numOutput, outputData);
    //call merge 2 times to get 8 data samples
    data.merge_train_data(data);
    data.merge_train_data(data);
    data.subset_train_data(2, 5);
    AssertTrainData(data, 5, numInput, numOutput, inputValue, outputValue);
}

TEST_F(FannTestData, ScaleOutputData)
{
    ANNTYP input[] = {0.0, 1.0, 0.5, 0.0, 1.0, 0.5};
    ANNTYP output[] = {0.0, 1.0};
    data.set_train_data(2, 3, input, 1, output);
    data.scale_output_train_data(-1.0, 2.0);
    EXPECT_DOUBLE_EQ(0.0, data.get_min_input());
    EXPECT_DOUBLE_EQ(1.0, data.get_max_input());
    EXPECT_DOUBLE_EQ(-1.0, data.get_min_output());
    EXPECT_DOUBLE_EQ(2.0, data.get_max_output());
}

TEST_F(FannTestData, ScaleInputData)
{
    ANNTYP input[] = {0.0, 1.0, 0.5, 0.0, 1.0, 0.5};
    ANNTYP output[] = {0.0, 1.0};
    data.set_train_data(2, 3, input, 1, output);
    data.scale_input_train_data(-1.0, 2.0);
    EXPECT_DOUBLE_EQ(-1.0, data.get_min_input());
    EXPECT_DOUBLE_EQ(2.0, data.get_max_input());
    EXPECT_DOUBLE_EQ(0.0, data.get_min_output());
    EXPECT_DOUBLE_EQ(1.0, data.get_max_output());
}

TEST_F(FannTestData, ScaleData)
{
    ANNTYP input[] = {0.0, 1.0, 0.5, 0.0, 1.0, 0.5};
    ANNTYP output[] = {0.0, 1.0};
    data.set_train_data(2, 3, input, 1, output);
    data.scale_train_data(-1.0, 2.0);
    for(uint i = 0; i < 2; i++) {
        ANNTYP *train_input = data.get_train_input(i);
        EXPECT_DOUBLE_EQ(-1.0, train_input[0]);
        EXPECT_DOUBLE_EQ(2.0, train_input[1]);
        EXPECT_DOUBLE_EQ(0.5, train_input[2]);
    }
    EXPECT_DOUBLE_EQ(-1.0, data.get_train_output(0)[0]);
    EXPECT_DOUBLE_EQ(2.0, data.get_train_output(0)[1]);
}
//
// FANN_TEST_TRAIN.CPP
//
class FannTestTrain : public FannTest {
protected:
    ANNTYP xorInput[8] = {
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0};
    ANNTYP xorOutput[4] = {
		0.0,
		1.0,
		1.0,
		0.0};
    virtual void SetUp();
    virtual void TearDown();
};

void FannTestTrain::SetUp()
{
    FannTest::SetUp();
}

void FannTestTrain::TearDown()
{
    FannTest::TearDown();
}

TEST_F(FannTestTrain, TrainOnDateSimpleXor)
{
    neural_net net(LAYER, 3, 2, 3, 1);
    data.set_train_data(4, 2, xorInput, 1, xorOutput);
    net.train_on_data(data, 100, 100, 0.001);
    EXPECT_LT(net.get_MSE(), 0.001);
    EXPECT_LT(net.test_data(data), 0.001);
}

TEST_F(FannTestTrain, TrainSimpleIncrementalXor)
{
    neural_net net(LAYER, 3, 2, 3, 1);
    for(int i = 0; i < 100000; i++) {
        net.train((ANNTYP*) (const ANNTYP[]) {0.0, 0.0}, (ANNTYP*) (const ANNTYP[]) {0.0});
        net.train((ANNTYP*) (const ANNTYP[]) {1.0, 0.0}, (ANNTYP*) (const ANNTYP[]) {1.0});
        net.train((ANNTYP*) (const ANNTYP[]) {0.0, 1.0}, (ANNTYP*) (const ANNTYP[]) {1.0});
        net.train((ANNTYP*) (const ANNTYP[]) {1.0, 1.0}, (ANNTYP*) (const ANNTYP[]) {0.0});
    }
    EXPECT_LT(net.get_MSE(), 0.01);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#endif // } 0 @construction

#if SLTEST_RUNNING // {

static int AssertWeights(STestCase * pTc, const Fann * pNet, ANNTYP min, ANNTYP max, ANNTYP avg)
{
	TSArray <FannConnection> connections;
	pNet->GetConnectionArray(connections);
    ANNTYP min_weight = connections.at(0).Weight;
    ANNTYP max_weight = connections.at(0).Weight;
    ANNTYP total_weight = 0.0;
	const  uint conn_count = pNet->GetTotalConnections();
    for(uint i = 1; i < conn_count; ++i) {
        if(connections.at(i).Weight < min_weight)
            min_weight = connections.at(i).Weight;
        if(connections.at(i).Weight > max_weight)
            max_weight = connections.at(i).Weight;
        total_weight += connections.at(i).Weight;
    }
    pTc->SLTEST_CHECK_EQ_TOL(min, min_weight, 0.05f);
    pTc->SLTEST_CHECK_EQ_TOL(max, max_weight, 0.05f);
    pTc->SLTEST_CHECK_EQ_TOL(avg, total_weight / (ANNTYP)conn_count, 0.5f);
	return pTc->GetCurrentStatus();
}

static int AssertCreate(STestCase * pTc, Fann * pNet, uint numLayers, const uint * pLayers, uint neurons, uint connections)
{
	LongArray layers;
    pTc->SLTEST_CHECK_EQ(numLayers, pNet->GetNumLayers());
    pNet->GetLayerArray(layers);
    pTc->SLTEST_CHECK_EQ(layers.get(0), (long)pNet->GetNumInput());
    pTc->SLTEST_CHECK_EQ(layers.get(numLayers-1), (long)pNet->GetNumOutput());
    for(uint i = 0; i < numLayers; i++) {
        pTc->SLTEST_CHECK_EQ(pLayers[i], (uint)layers.get(i));
    }
    pTc->SLTEST_CHECK_EQ(neurons, pNet->GetTotalNeurons());
    pTc->SLTEST_CHECK_EQ(connections, pNet->GetTotalConnections());
    AssertWeights(pTc, pNet, -0.09f, 0.09f, 0.0f);
	return pTc->GetCurrentStatus();
}

static int AssertCreateAndCopy(STestCase * pTc, Fann * pNet, uint numLayers, const uint * pLayers, uint neurons, uint connections)
{
    AssertCreate(pTc, pNet, numLayers, pLayers, neurons, connections);
    Fann * p_copy = fann_copy(pNet);
    AssertCreate(pTc, p_copy, numLayers, pLayers, neurons, connections);
	pTc->SLTEST_CHECK_NZ(p_copy->IsEqual(*pNet, 0));
    fann_destroy(p_copy);
	return pTc->GetCurrentStatus();
}

SLTEST_R(FANN)
{
	LongArray layers;
	// CreateStandardThreeLayers
	{
		const uint p_layer_dim[] = { 2, 3, 4 };
		Fann * p_ann = fann_create_standard(3, 2, 3, 4);
		AssertCreateAndCopy(this, p_ann, 3, p_layer_dim, 11, 25);
		fann_destroy(p_ann);
	}
	// CreateStandardFourLayersArray
	{
		const uint p_layer_dim[] = { 2, 3, 4, 5 };
		Fann * p_ann = fann_create_standard(4, 2, 3, 4, 5);
		AssertCreateAndCopy(this, p_ann, 4, p_layer_dim, 17, 50);
		fann_destroy(p_ann);
	}
	// CreateSparseFourLayers
	{
		const uint p_layer_dim[] = { 2, 3, 4, 5 };
		Fann * p_ann = fann_create_sparse(0.5f, 4, 2, 3, 4, 5);
		AssertCreateAndCopy(this, p_ann, 4, p_layer_dim, 17, 31);
		fann_destroy(p_ann);
	}
	// CreateSparseArrayWithMinimalConnectivity
	{
		const uint p_layer_dim[] = { 2, 2, 2 };
		Fann * p_ann = fann_create_sparse(0.01f, 3, 2, 2, 2);
		AssertCreateAndCopy(this, p_ann, 3, p_layer_dim, 8, 8);
		fann_destroy(p_ann);
	}
	// CreateShortcutFourLayers
	{
		const uint p_layer_dim[] = { 2, 3, 4, 5 };
		Fann * p_ann = fann_create_shortcut(4, 2, 3, 4, 5);
		AssertCreateAndCopy(this, p_ann, 4, p_layer_dim, 15, 83);
		SLTEST_CHECK_EQ((long)p_ann->GetNetworkType(), (long)Fann::FANN_NETTYPE_SHORTCUT);
		fann_destroy(p_ann);
	}
	/*
	// CreateFromFile
	{
		ASSERT_TRUE(net.create_standard(3, 2, 3, 4));
		neural_net netToBeSaved(LAYER, 3, 2, 3, 4);
		ASSERT_TRUE(netToBeSaved.save("tmpfile"));
		neural_net netToBeLoaded("tmpfile");
		AssertCreateAndCopy(netToBeLoaded, 3, (const uint[]){2, 3, 4}, 11, 25);
	}
	// CreateFromFileUsingCreateMethod
	{
		ASSERT_TRUE(net.create_standard(3, 2, 3, 4));
		neural_net inputNet(LAYER, 3, 2, 3, 4);
		ASSERT_TRUE(inputNet.save("tmpfile"));
		ASSERT_TRUE(net.create_from_file("tmpfile"));
		AssertCreateAndCopy(net, 3, (const uint[]){2, 3, 4}, 11, 25);
	}
	// RandomizeWeights
	{
		neural_net net(LAYER, 2, 20, 10);
		net.randomize_weights(-1.0, 1.0);
		AssertWeights(net, -1.0, 1.0, 0);
	}
	*/
	{
		ANNTYP XorInput[] = {
			0.0, 0.0,
			0.0, 1.0,
			1.0, 0.0,
			1.0, 1.0
		};
		ANNTYP XorOutput[] = {
			0.0,
			1.0,
			1.0,
			0.0
		};
		// TrainOnDateSimpleXor
		{
			const uint c_in = 2;
			const uint c_out = 1;
			const uint c_d = 4;
			Fann * p_ann = fann_create_standard(3, c_in, 3, c_out);
			Fann::TrainData train_data(*p_ann, c_d);
			for(uint i = 0; i < c_d; i++) {
				SLTEST_CHECK_NZ(train_data.SetInputSeries(i, XorInput + i * c_in));
				SLTEST_CHECK_NZ(train_data.SetOutputSeries(i, XorOutput + i * c_out));
			}
			p_ann->TrainOnData(&train_data, 100, 100, 0.001f);
			SLTEST_CHECK_LT(p_ann->GetMSE(), 0.001f);
			SLTEST_CHECK_LT(p_ann->TestData(&train_data), 0.001f);
			{
				SBuffer sbuf;
				int    r = 0;
				{
					SSerializeContext sctx;
					SLTEST_CHECK_NZ(r = p_ann->Serialize(+1, sbuf, &sctx));
				}
				if(r) {
					SSerializeContext sctx;
					Fann new_ann(sbuf, &sctx);
					SLTEST_CHECK_NZ(new_ann.IsValid());
					SLTEST_CHECK_NZ(new_ann.IsEqual(*p_ann, 0));
				}
			}
			fann_destroy(p_ann);
		}
		// TrainSimpleIncrementalXor
		{
			const uint c_in = 2;
			const uint c_out = 1;
			layers.clear();
			layers.addzlist(2, 3, 1, 0);
			Fann ann(Fann::FANN_NETTYPE_LAYER, 1.0f, layers);
			SLTEST_CHECK_NZ(ann.IsValid());
			for(int i = 0; i < 100000; i++) {
				ann.Train(XorInput + 0 * c_in, XorOutput + 0 * c_out);
				ann.Train(XorInput + 1 * c_in, XorOutput + 1 * c_out);
				ann.Train(XorInput + 2 * c_in, XorOutput + 2 * c_out);
				ann.Train(XorInput + 3 * c_in, XorOutput + 3 * c_out);
			}
			SLTEST_CHECK_LT(ann.GetMSE(), 0.01f);
			SLTEST_CHECK_EQ(ann.Run(XorInput + 0 * c_in)[0], XorOutput[0 * c_out]);
			SLTEST_CHECK_EQ(ann.Run(XorInput + 1 * c_in)[0], XorOutput[1 * c_out]);
			SLTEST_CHECK_EQ(ann.Run(XorInput + 2 * c_in)[0], XorOutput[2 * c_out]);
			SLTEST_CHECK_EQ(ann.Run(XorInput + 3 * c_in)[0], XorOutput[3 * c_out]);
			{
				SBuffer sbuf;
				int    r = 0;
				{
					SSerializeContext sctx;
					SLTEST_CHECK_NZ(r = ann.Serialize(+1, sbuf, &sctx));
				}
				if(r) {
					SSerializeContext sctx;
					Fann new_ann(sbuf, &sctx);
					SLTEST_CHECK_NZ(new_ann.IsValid());
					SLTEST_CHECK_NZ(new_ann.IsEqual(ann, 0));
				}
			}
			/*
			neural_net net(LAYER, 3, 2, 3, 1);
			for(int i = 0; i < 100000; i++) {
				net.train((ANNTYP*) (const ANNTYP[]) {0.0, 0.0}, (ANNTYP*) (const ANNTYP[]) {0.0});
				net.train((ANNTYP*) (const ANNTYP[]) {1.0, 0.0}, (ANNTYP*) (const ANNTYP[]) {1.0});
				net.train((ANNTYP*) (const ANNTYP[]) {0.0, 1.0}, (ANNTYP*) (const ANNTYP[]) {1.0});
				net.train((ANNTYP*) (const ANNTYP[]) {1.0, 1.0}, (ANNTYP*) (const ANNTYP[]) {0.0});
			}
			EXPECT_LT(net.get_MSE(), 0.01);
			*/
		}
		if(0) {
			SString temp_buf;
			SString test_item_name;
			SString input_file_name;
			SString test_input_file_name;
			for(uint ap = 0; EnumArg(&ap, test_item_name);) {
				float local_err = 0.0f;
				input_file_name = MakeInputFilePath((temp_buf = test_item_name).Dot().Cat("train"));
				test_input_file_name = MakeInputFilePath((temp_buf = test_item_name).Dot().Cat("test"));
				if(fileExists(input_file_name) && fileExists(test_input_file_name)) {
					Fann::TrainData train_data;
					Fann::TrainData test_data;
					int    trr_ = 0;
					int    tsr_ = 0;
					int    dor_ = 0;
					SLTEST_CHECK_NZ(trr_ = train_data.Read(input_file_name, 0));
					SLTEST_CHECK_NZ(tsr_ = test_data.Read(test_input_file_name, 0));
					if(trr_ && tsr_) {
						layers.clear();
						layers.addzlist(train_data.GetInputCount(), /*train_data.GetInputCount() * 10*/30, train_data.GetOutputCount(), 0);
						Fann::DetectOptimalParam dop;
						dop.Layers = layers;
						dop.Flags |= (dop.fDetectActivationFunc|dop.fDetectTrainAlg);
						dop.P_TrainData = &train_data;
						SLTEST_CHECK_NZ(dor_ = Fann::DetectOptimal(dop));
						{
							Fann ann(Fann::FANN_NETTYPE_LAYER, 1.0f, layers);
							SLTEST_CHECK_NZ(ann.IsValid());
							if(dop.ResultFlags & dop.rfTrainAlgDetected && dop.BestTrainAlg >= 0)
								ann.SetTrainingAlgorithm((Fann::TrainAlg)dop.BestTrainAlg);
							if(dop.ResultFlags & dop.rfHiddActFuncDetected && dop.BestHiddActF >= 0)
								ann.SetActivationFunctionHidden((Fann::ActivationFunc)dop.BestHiddActF);
							if(dop.ResultFlags & dop.rfOutpActFuncDetected && dop.BestOutpActF >= 0)
								ann.SetActivationFunctionOutput((Fann::ActivationFunc)dop.BestOutpActF);
							ann.TrainOnData(&train_data, 1000, 0, 0.000001f);
							{
								local_err = ann.TestData(&test_data);
							}
						}
					}
				}
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
