// Fast Artificial Neural Network Library (fann)
// Copyright (C) 2003-2016 Steffen Nissen (steffen.fann@gmail.com)
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
#include <slib-internal.h>
#pragma hdrstop
#include "fann2.h"

int FASTCALL Fann2::IsEqual(const Fann2 & rS) const // @construction
{
	/*
		num_layers=5
		learning_rate=0.700000
		connection_rate=1.000000
		network_type=0
		learning_momentum=0.000000
		training_algorithm=2
		train_error_function=1
		train_stop_function=0
		cascade_output_change_fraction=0.010000
		quickprop_decay=-0.000100
		quickprop_mu=1.750000
		rprop_increase_factor=1.200000
		rprop_decrease_factor=0.500000
		rprop_delta_min=0.000000
		rprop_delta_max=50.000000
		rprop_delta_zero=0.100000
		cascade_output_stagnation_epochs=12
		cascade_candidate_change_fraction=0.010000
		cascade_candidate_stagnation_epochs=12
		cascade_max_out_epochs=150
		cascade_min_out_epochs=50
		cascade_max_cand_epochs=150
		cascade_min_cand_epochs=50
		cascade_num_candidate_groups=2
		bit_fail_limit=3.49999994039535522461e-01
		cascade_candidate_limit=1.00000000000000000000e+03
		cascade_weight_multiplier=4.00000005960464477539e-01
		cascade_activation_functions_count=10
		cascade_activation_functions=3 5 7 8 10 11 14 15 16 17 
		cascade_activation_steepnesses_count=4
		cascade_activation_steepnesses=2.50000000000000000000e-01 5.00000000000000000000e-01 7.50000000000000000000e-01 1.00000000000000000000e+00 
		layer_sizes=10 513 257 65 2 
		scale_included=0
	*/
#define FLD(f) if(f != rS.f) return 0;
	FLD(learning_rate);
	FLD(learning_momentum);
	FLD(connection_rate);
	FLD(network_type);
	FLD(total_neurons);
	FLD(num_input);
	FLD(num_output);
	FLD(training_algorithm);
	FLD(total_connections);
	FLD(num_MSE);
	FLD(MSE_value);
	FLD(num_bit_fail);
	FLD(bit_fail_limit);
	FLD(train_error_function);
	FLD(train_stop_function);
	FLD(cascade_output_change_fraction);
	FLD(cascade_output_stagnation_epochs);
	FLD(cascade_candidate_change_fraction);
	FLD(cascade_candidate_stagnation_epochs);
	FLD(cascade_best_candidate);
	FLD(cascade_candidate_limit);
	FLD(cascade_weight_multiplier);
	FLD(cascade_max_out_epochs);
	FLD(cascade_max_cand_epochs);
	FLD(cascade_min_out_epochs);
	FLD(cascade_min_cand_epochs);
	//cascade_activation_functions;
	FLD(cascade_activation_functions_count);
	//cascade_activation_steepnesses;
	FLD(cascade_activation_steepnesses_count);
	FLD(cascade_num_candidate_groups);
	//cascade_candidate_scores;
	FLD(total_neurons_allocated);
	FLD(total_connections_allocated);
	FLD(quickprop_decay);
	FLD(quickprop_mu);
	FLD(rprop_increase_factor);
	FLD(rprop_decrease_factor);
	FLD(rprop_delta_min);
	FLD(rprop_delta_max);
	FLD(rprop_delta_zero);
	FLD(sarprop_weight_decay_shift);
	FLD(sarprop_step_error_threshold_factor);
	FLD(sarprop_step_error_shift);
	FLD(sarprop_temperature);
	FLD(sarprop_epoch);
	//train_slopes;
	//prev_steps;
	//prev_train_slopes;
	//prev_weights_deltas;
	//float * scale_mean_in;
	//float * scale_deviation_in;
	//float * scale_new_min_in;
	//float * scale_factor_in;
	//float * scale_mean_out;
	//float * scale_deviation_out;
	//float * scale_new_min_out;
	//float * scale_factor_out;
#undef FLD
	uint nl = (last_layer - first_layer);
	uint nl_s = (rS.last_layer - rS.first_layer);
	if(nl != nl_s)
		return 0;
	else {
		// first_layer..last_layer
		// weights
		// connections
		// train_errors
		// output
	}
	return 1;
}

static void fann_seed_rand_2();

/* #define FANN_NO_SEED */

FANN_EXTERNAL Fann2 * FANN_API fann_create_standard_2(uint num_layers, ...)
{
	Fann2 * ann = 0;
	uint * layers = static_cast<uint *>(SAlloc::C(num_layers, sizeof(uint)));
	if(layers == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
	}
	else {
		va_list layer_sizes;
		int status;
		int arg;
		va_start(layer_sizes, num_layers);
		status = 1;
		for(uint i = 0; i < (int)num_layers; i++) {
			arg = va_arg(layer_sizes, uint);
			if(arg < 0 || arg > 1000000)
				status = 0;
			layers[i] = arg;
		}
		va_end(layer_sizes);
		if(!status) {
			fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
			SAlloc::F(layers);
			return NULL;
		}
		ann = fann_create_standard_array(num_layers, layers);
		SAlloc::F(layers);
	}
	return ann;
}

FANN_EXTERNAL Fann2 * FANN_API fann_create_standard_array(uint num_layers, const uint * layers)
{
	return fann_create_sparse_array(1, num_layers, layers);
}

FANN_EXTERNAL Fann2 * FANN_API fann_create_sparse_2(float connection_rate, uint num_layers, ...)
{
	Fann2 * ann;
	va_list layer_sizes;
	int i;
	int status;
	int arg;
	uint * layers = static_cast<uint *>(SAlloc::C(num_layers, sizeof(uint)));
	if(layers == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	va_start(layer_sizes, num_layers);
	status = 1;
	for(i = 0; i < (int)num_layers; i++) {
		arg = va_arg(layer_sizes, uint);
		if(arg < 0 || arg > 1000000)
			status = 0;
		layers[i] = arg;
	}
	va_end(layer_sizes);
	if(!status) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		SAlloc::F(layers);
		return NULL;
	}
	ann = fann_create_sparse_array(connection_rate, num_layers, layers);
	SAlloc::F(layers);
	return ann;
}
// 
// INTERNAL FUNCTION
// Allocates the main structure and sets some default values.
// 
static Fann2 * fann_allocate_structure(uint num_layers)
{
	Fann2 * ann;
	if(num_layers < 2) {
#ifdef DEBUG
		printf("less than 2 layers - ABORTING.\n");
#endif
		return NULL;
	}
	/* allocate and initialize the main network structure */
	ann = static_cast<Fann2 *>(SAlloc::M(sizeof(Fann2)));
	if(ann == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	ann->errno_f = FANN_E_NO_ERROR;
	ann->error_log = fann_default_error_log;
	ann->errstr = NULL;
	ann->learning_rate = 0.7f;
	ann->learning_momentum = 0.0;
	ann->total_neurons = 0;
	ann->total_connections = 0;
	ann->num_input = 0;
	ann->num_output = 0;
	ann->train_errors = NULL;
	ann->train_slopes = NULL;
	ann->prev_steps = NULL;
	ann->prev_train_slopes = NULL;
	ann->prev_weights_deltas = NULL;
	ann->training_algorithm = FANN_TRAIN_RPROP;
	ann->num_MSE = 0;
	ann->MSE_value = 0;
	ann->num_bit_fail = 0;
	ann->bit_fail_limit = 0.35f;
	ann->network_type = FANN_NETTYPE_LAYER;
	ann->train_error_function = FANN_ERRORFUNC_TANH;
	ann->train_stop_function = FANN_STOPFUNC_MSE;
	ann->callback = NULL;
	ann->user_data = NULL; /* User is responsible for deallocation */
	ann->weights = NULL;
	ann->connections = NULL;
	ann->output = NULL;
#ifndef FIXEDFANN
	ann->scale_mean_in = NULL;
	ann->scale_deviation_in = NULL;
	ann->scale_new_min_in = NULL;
	ann->scale_factor_in = NULL;
	ann->scale_mean_out = NULL;
	ann->scale_deviation_out = NULL;
	ann->scale_new_min_out = NULL;
	ann->scale_factor_out = NULL;
#endif
	/* variables used for cascade correlation (reasonable defaults) */
	ann->cascade_output_change_fraction = 0.01f;
	ann->cascade_candidate_change_fraction = 0.01f;
	ann->cascade_output_stagnation_epochs = 12;
	ann->cascade_candidate_stagnation_epochs = 12;
	ann->cascade_num_candidate_groups = 2;
	ann->cascade_weight_multiplier = 0.4f;
	ann->cascade_candidate_limit = 1000.0f;
	ann->cascade_max_out_epochs = 150;
	ann->cascade_max_cand_epochs = 150;
	ann->cascade_min_out_epochs = 50;
	ann->cascade_min_cand_epochs = 50;
	ann->cascade_candidate_scores = NULL;
	ann->cascade_activation_functions_count = 10;
	ann->cascade_activation_functions = static_cast<Fann2::ActivationFunc *>(SAlloc::C(ann->cascade_activation_functions_count, sizeof(Fann2::ActivationFunc)));
	if(ann->cascade_activation_functions == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		SAlloc::F(ann);
		return NULL;
	}
	ann->cascade_activation_functions[0] = Fann2::FANN_SIGMOID;
	ann->cascade_activation_functions[1] = Fann2::FANN_SIGMOID_SYMMETRIC;
	ann->cascade_activation_functions[2] = Fann2::FANN_GAUSSIAN;
	ann->cascade_activation_functions[3] = Fann2::FANN_GAUSSIAN_SYMMETRIC;
	ann->cascade_activation_functions[4] = Fann2::FANN_ELLIOT;
	ann->cascade_activation_functions[5] = Fann2::FANN_ELLIOT_SYMMETRIC;
	ann->cascade_activation_functions[6] = Fann2::FANN_SIN_SYMMETRIC;
	ann->cascade_activation_functions[7] = Fann2::FANN_COS_SYMMETRIC;
	ann->cascade_activation_functions[8] = Fann2::FANN_SIN;
	ann->cascade_activation_functions[9] = Fann2::FANN_COS;

	ann->cascade_activation_steepnesses_count = 4;
	ann->cascade_activation_steepnesses = static_cast<float *>(SAlloc::C(ann->cascade_activation_steepnesses_count, sizeof(float)));
	if(ann->cascade_activation_steepnesses == NULL) {
		ZFREE(ann->cascade_activation_functions);
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		SAlloc::F(ann);
		return NULL;
	}
	ann->cascade_activation_steepnesses[0] = 0.25f;
	ann->cascade_activation_steepnesses[1] = 0.5f;
	ann->cascade_activation_steepnesses[2] = 0.75f;
	ann->cascade_activation_steepnesses[3] = 1.0f;
	/* Variables for use with with Quickprop training (reasonable defaults) */
	ann->quickprop_decay = -0.0001f;
	ann->quickprop_mu = 1.75f;
	/* Variables for use with with RPROP training (reasonable defaults) */
	ann->rprop_increase_factor = 1.2f;
	ann->rprop_decrease_factor = 0.5f;
	ann->rprop_delta_min = 0.0f;
	ann->rprop_delta_max = 50.0f;
	ann->rprop_delta_zero = 0.1f;

	/* Variables for use with SARPROP training (reasonable defaults) */
	ann->sarprop_weight_decay_shift = -6.644f;
	ann->sarprop_step_error_threshold_factor = 0.1f;
	ann->sarprop_step_error_shift = 1.385f;
	ann->sarprop_temperature = 0.015f;
	ann->sarprop_epoch = 0;

	fann_init_error_data(reinterpret_cast<struct fann_error *>(ann));

#ifdef FIXEDFANN
	// these values are only boring defaults, and should really never be used, since the real values are always loaded from a file.
	ann->decimal_point = 8;
	ann->multiplier = 256;
#endif
	// allocate room for the layers 
	ann->first_layer = static_cast<Fann2::Layer *>(SAlloc::C(num_layers, sizeof(Fann2::Layer)));
	if(ann->first_layer == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		SAlloc::F(ann);
		return NULL;
	}
	ann->last_layer = ann->first_layer + num_layers;
	return ann;
}
// 
// INTERNAL FUNCTION
// Allocate room for the connections.
// 
static void fann_allocate_connections(Fann2 * ann)
{
	ann->weights = static_cast<float *>(SAlloc::C(ann->total_connections, sizeof(float)));
	if(ann->weights == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
	}
	else {
		ann->total_connections_allocated = ann->total_connections;
		// TODO make special cases for all places where the connections
		// is used, so that it is not needed for fully connected networks.
		ann->connections = static_cast<Fann2::Neuron **>(SAlloc::C(ann->total_connections_allocated, sizeof(Fann2::Neuron *)));
		if(ann->connections == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		}
	}
}
// 
// INTERNAL FUNCTION
// Allocates room for the neurons.
// 
static void fann_allocate_neurons(Fann2 * ann)
{
	Fann2::Layer * layer_it;
	uint num_neurons_so_far = 0;
	uint num_neurons = 0;
	// all the neurons is allocated in one long array (calloc clears mem) 
	Fann2::Neuron * neurons = static_cast<Fann2::Neuron *>(SAlloc::C(ann->total_neurons, sizeof(Fann2::Neuron)));
	ann->total_neurons_allocated = ann->total_neurons;
	if(neurons == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return;
	}
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		num_neurons = (uint)(layer_it->last_neuron - layer_it->first_neuron);
		layer_it->first_neuron = neurons + num_neurons_so_far;
		layer_it->last_neuron = layer_it->first_neuron + num_neurons;
		num_neurons_so_far += num_neurons;
	}
	ann->output = static_cast<float *>(SAlloc::C(num_neurons, sizeof(float)));
	if(ann->output == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return;
	}
}

FANN_EXTERNAL Fann2 * FANN_API fann_create_sparse_array(float connection_rate, uint num_layers, const uint * layers)
{
	Fann2::Layer * layer_it, * last_layer, * prev_layer;
	Fann2 * ann;
	Fann2::Neuron * neuron_it, * last_neuron, * random_neuron, * bias_neuron;
#ifdef DEBUG
	uint prev_layer_size;
#endif
	uint num_neurons_in, num_neurons_out, i, j;
	uint min_connections, max_connections, num_connections;
	uint connections_per_neuron, allocated_connections;
	uint random_number, found_connection, tmp_con;
#ifdef FIXEDFANN
	uint multiplier;
#endif
	if(connection_rate > 1) {
		connection_rate = 1;
	}
	fann_seed_rand_2();
	/* allocate the general structure */
	ann = fann_allocate_structure(num_layers);
	if(ann == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	ann->connection_rate = connection_rate;
#ifdef FIXEDFANN
	multiplier = ann->multiplier;
	fann_update_stepwise(ann);
#endif
	/* determine how many neurons there should be in each layer */
	i = 0;
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		/* we do not allocate room here, but we make sure that
		 * last_neuron - first_neuron is the number of neurons */
		layer_it->first_neuron = NULL;
		layer_it->last_neuron = layer_it->first_neuron + layers[i++] + 1;       /* +1 for bias */
		ann->total_neurons += (uint)(layer_it->last_neuron - layer_it->first_neuron);
	}
	ann->num_output = (uint)((ann->last_layer - 1)->last_neuron - (ann->last_layer - 1)->first_neuron - 1);
	ann->num_input = (uint)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);
	/* allocate room for the actual neurons */
	fann_allocate_neurons(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
#ifdef DEBUG
	printf("creating network with connection rate %f\n", connection_rate);
	printf("input\n");
	printf("  layer       : %d neurons, 1 bias\n", (int)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1));
#endif
	num_neurons_in = ann->num_input;
	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++) {
		num_neurons_out = (uint)(layer_it->last_neuron - layer_it->first_neuron - 1);
		// if all neurons in each layer should be connected to at least one neuron
		// in the previous layer, and one neuron in the next layer.
		// and the bias node should be connected to the all neurons in the next layer.
		// Then this is the minimum amount of neurons 
		min_connections = MAX(num_neurons_in, num_neurons_out); // not calculating bias 
		max_connections = num_neurons_in * num_neurons_out; // not calculating bias 
		num_connections = MAX(min_connections, (uint)(0.5f + (connection_rate * max_connections))) + num_neurons_out;
		connections_per_neuron = num_connections / num_neurons_out;
		allocated_connections = 0;
		// Now split out the connections on the different neurons 
		for(i = 0; i != num_neurons_out; i++) {
			layer_it->first_neuron[i].first_con = ann->total_connections + allocated_connections;
			allocated_connections += connections_per_neuron;
			layer_it->first_neuron[i].last_con = ann->total_connections + allocated_connections;
			layer_it->first_neuron[i].activation_function = Fann2::FANN_SIGMOID_STEPWISE;
#ifdef FIXEDFANN
			layer_it->first_neuron[i].activation_steepness = ann->multiplier / 2;
#else
			layer_it->first_neuron[i].activation_steepness = 0.5;
#endif
			if(allocated_connections < (num_connections * (i + 1)) / num_neurons_out) {
				layer_it->first_neuron[i].last_con++;
				allocated_connections++;
			}
		}
		/* bias neuron also gets stuff */
		layer_it->first_neuron[i].first_con = ann->total_connections + allocated_connections;
		layer_it->first_neuron[i].last_con = ann->total_connections + allocated_connections;
		ann->total_connections += num_connections;
		/* used in the next run of the loop */
		num_neurons_in = num_neurons_out;
	}
	fann_allocate_connections(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
	if(connection_rate >= 1) {
#ifdef DEBUG
		prev_layer_size = ann->num_input + 1;
#endif
		prev_layer = ann->first_layer;
		last_layer = ann->last_layer;
		for(layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++) {
			last_neuron = layer_it->last_neuron - 1;
			for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
				tmp_con = neuron_it->last_con - 1;
				for(i = neuron_it->first_con; i != tmp_con; i++) {
					ann->weights[i] = (float)fann_random_weight();
					// these connections are still initialized for fully connected networks, to allow
					// operations to work, that are not optimized for fully connected networks.
					ann->connections[i] = prev_layer->first_neuron + (i - neuron_it->first_con);
				}
				/* bias weight */
				ann->weights[tmp_con] = (float)fann_random_bias_weight();
				ann->connections[tmp_con] = prev_layer->first_neuron + (tmp_con - neuron_it->first_con);
			}
#ifdef DEBUG
			prev_layer_size = layer_it->last_neuron - layer_it->first_neuron;
#endif
			prev_layer = layer_it;
#ifdef DEBUG
			printf("  layer       : %d neurons, 1 bias\n", prev_layer_size - 1);
#endif
		}
	}
	else {
		/* make connections for a network, that are not fully connected */

		/* generally, what we do is first to connect all the input
		 * neurons to a output neuron, respecting the number of
		 * available input neurons for each output neuron. Then
		 * we go through all the output neurons, and connect the
		 * rest of the connections to input neurons, that they are
		 * not allready connected to.
		 */
		// All the connections are cleared by calloc, because we want to
		// be able to see which connections are allready connected 
		for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++) {
			num_neurons_out = (uint)(layer_it->last_neuron - layer_it->first_neuron - 1);
			num_neurons_in = (uint)((layer_it - 1)->last_neuron - (layer_it - 1)->first_neuron - 1);
			// first connect the bias neuron 
			bias_neuron = (layer_it - 1)->last_neuron - 1;
			last_neuron = layer_it->last_neuron - 1;
			for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
				ann->connections[neuron_it->first_con] = bias_neuron;
				ann->weights[neuron_it->first_con] = (float)fann_random_bias_weight();
			}
			// then connect all neurons in the input layer 
			last_neuron = (layer_it - 1)->last_neuron - 1;
			for(neuron_it = (layer_it - 1)->first_neuron; neuron_it != last_neuron; neuron_it++) {
				// random neuron in the output layer that has space for more connections 
				do {
					random_number = static_cast<int>(0.5f + fann_rand(0, num_neurons_out - 1));
					random_neuron = layer_it->first_neuron + random_number;
					/* checks the last space in the connections array for room */
				} while(ann->connections[random_neuron->last_con - 1]);
				// find an empty space in the connection array and connect 
				for(i = random_neuron->first_con; i < random_neuron->last_con; i++) {
					if(ann->connections[i] == NULL) {
						ann->connections[i] = neuron_it;
						ann->weights[i] = (float)fann_random_weight();
						break;
					}
				}
			}
			// then connect the rest of the unconnected neurons 
			last_neuron = layer_it->last_neuron - 1;
			for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
				// find empty space in the connection array and connect 
				for(i = neuron_it->first_con; i < neuron_it->last_con; i++) {
					// continue if allready connected 
					if(ann->connections[i] != NULL)
						continue;
					do {
						found_connection = 0;
						random_number = static_cast<int>(0.5f + fann_rand(0, num_neurons_in - 1));
						random_neuron = (layer_it - 1)->first_neuron + random_number;
						// check to see if this connection is allready there 
						for(j = neuron_it->first_con; j < i; j++) {
							if(random_neuron == ann->connections[j]) {
								found_connection = 1;
								break;
							}
						}
					}
					while(found_connection);
					// we have found a neuron that is not allready connected to us, connect it 
					ann->connections[i] = random_neuron;
					ann->weights[i] = (float)fann_random_weight();
				}
			}

#ifdef DEBUG
			printf("  layer       : %d neurons, 1 bias\n", num_neurons_out);
#endif
		}
		// TODO it would be nice to have the randomly created connections sorted for smoother memory access.
	}
#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}

FANN_EXTERNAL Fann2 * FANN_API fann_create_shortcut_2(uint num_layers, ...)
{
	Fann2 * ann = 0;
	int status;
	int arg;
	va_list layer_sizes;
	uint * layers = static_cast<uint *>(SAlloc::C(num_layers, sizeof(uint)));
	if(layers == NULL)
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
	else {
		va_start(layer_sizes, num_layers);
		status = 1;
		for(uint i = 0; i < num_layers; i++) {
			arg = va_arg(layer_sizes, uint);
			if(arg < 0 || arg > 1000000)
				status = 0;
			layers[i] = arg;
		}
		va_end(layer_sizes);
		if(!status) {
			fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
			SAlloc::F(layers);
		}
		else {
			ann = fann_create_shortcut_array(num_layers, layers);
			SAlloc::F(layers);
		}
	}
	return ann;
}

FANN_EXTERNAL Fann2 * FANN_API fann_create_shortcut_array(uint num_layers, const uint * layers)
{
	Fann2::Layer * layer_it, * layer_it2, * last_layer;
	Fann2 * ann;
	Fann2::Neuron * neuron_it, * neuron_it2 = 0;
	uint i;
	uint num_neurons_in, num_neurons_out;
#ifdef FIXEDFANN
	uint multiplier;
#endif
	fann_seed_rand_2();
	/* allocate the general structure */
	ann = fann_allocate_structure(num_layers);
	if(ann == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	ann->connection_rate = 1;
	ann->network_type = FANN_NETTYPE_SHORTCUT;
#ifdef FIXEDFANN
	multiplier = ann->multiplier;
	fann_update_stepwise(ann);
#endif
	// determine how many neurons there should be in each layer 
	i = 0;
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		// we do not allocate room here, but we make sure that
		// last_neuron - first_neuron is the number of neurons 
		layer_it->first_neuron = NULL;
		layer_it->last_neuron = layer_it->first_neuron + layers[i++];
		if(layer_it == ann->first_layer) {
			layer_it->last_neuron++; // there is a bias neuron in the first layer 
		}
		ann->total_neurons += (uint)(layer_it->last_neuron - layer_it->first_neuron);
	}
	ann->num_output = (uint)((ann->last_layer - 1)->last_neuron - (ann->last_layer - 1)->first_neuron);
	ann->num_input = (uint)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);
	/* allocate room for the actual neurons */
	fann_allocate_neurons(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
#ifdef DEBUG
	printf("creating fully shortcut connected network.\n");
	printf("input\n");
	printf("  layer       : %d neurons, 1 bias\n", (int)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1));
#endif
	num_neurons_in = ann->num_input;
	last_layer = ann->last_layer;
	for(layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++) {
		num_neurons_out = (uint)(layer_it->last_neuron - layer_it->first_neuron);
		// Now split out the connections on the different neurons 
		for(i = 0; i != num_neurons_out; i++) {
			layer_it->first_neuron[i].first_con = ann->total_connections;
			ann->total_connections += num_neurons_in + 1;
			layer_it->first_neuron[i].last_con = ann->total_connections;
			layer_it->first_neuron[i].activation_function = Fann2::FANN_SIGMOID_STEPWISE;
#ifdef FIXEDFANN
			layer_it->first_neuron[i].activation_steepness = ann->multiplier / 2;
#else
			layer_it->first_neuron[i].activation_steepness = 0.5;
#endif
		}

#ifdef DEBUG
		printf("  layer       : %d neurons, 0 bias\n", num_neurons_out);
#endif
		num_neurons_in += num_neurons_out; // used in the next run of the loop 
	}
	fann_allocate_connections(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
	// Connections are created from all neurons to all neurons in later layers
	num_neurons_in = ann->num_input + 1;
	for(layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++) {
		for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++) {
			i = neuron_it->first_con;
			for(layer_it2 = ann->first_layer; layer_it2 != layer_it; layer_it2++) {
				for(neuron_it2 = layer_it2->first_neuron; neuron_it2 != layer_it2->last_neuron;
				    neuron_it2++) {
					ann->weights[i] = (float)fann_random_weight();
					ann->connections[i] = neuron_it2;
					i++;
				}
			}
		}
		num_neurons_in += (uint)(layer_it->last_neuron - layer_it->first_neuron);
	}
#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}

FANN_EXTERNAL float * FANN_API fann_run(Fann2 * ann, float * input)
{
	Fann2::Neuron * neuron_it, * last_neuron, * neurons/*, ** neuron_pointers*/;
	uint i, /*num_connections,*/ num_input, num_output;
	float /*neuron_sum,*/ * output;
	//float * weights;
	Fann2::Layer * layer_it, * last_layer;
	//uint activation_function;
	//float steepness;
	/* store some variabels local for fast access */
	Fann2::Neuron * first_neuron = ann->first_layer->first_neuron;
#ifdef FIXEDFANN
	int multiplier = ann->multiplier;
	uint decimal_point = ann->decimal_point;
	/* values used for the stepwise linear sigmoid function */
	float r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;
	float v1 = 0, v2 = 0, v3 = 0, v4 = 0, v5 = 0, v6 = 0;
	float last_steepness = 0;
	uint last_activation_function = 0;
#else
	float max_sum = 0;
#endif
	// first set the input 
	num_input = ann->num_input;
	for(i = 0; i != num_input; i++) {
#ifdef FIXEDFANN
		if(fabsf(input[i]) > multiplier) {
			printf("Warning input number %d is out of range -%d - %d with value %d, integer overflow may occur.\n", i, multiplier, multiplier, input[i]);
		}
#endif
		first_neuron[i].value = input[i];
	}
	// Set the bias neuron in the input layer 
#ifdef FIXEDFANN
	(ann->first_layer->last_neuron - 1)->value = multiplier;
#else
	(ann->first_layer->last_neuron - 1)->value = 1;
#endif
	last_layer = ann->last_layer;
	for(layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++) {
		last_neuron = layer_it->last_neuron;
		for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
			if(neuron_it->first_con == neuron_it->last_con) {
				/* bias neurons */
#ifdef FIXEDFANN
				neuron_it->value = multiplier;
#else
				neuron_it->value = 1;
#endif
				continue;
			}
			const uint activation_function = neuron_it->activation_function;
			const float steepness = neuron_it->activation_steepness;
			const uint num_connections = neuron_it->last_con - neuron_it->first_con;
			const float * weights = ann->weights + neuron_it->first_con;
			float neuron_sum = 0;
			if(ann->connection_rate >= 1) {
				if(ann->network_type == FANN_NETTYPE_SHORTCUT) {
					neurons = ann->first_layer->first_neuron;
				}
				else {
					neurons = (layer_it - 1)->first_neuron;
				}
				/* unrolled loop start */
				i = num_connections & 3;        /* same as modulo 4 */
				switch(i) {
					case 3: neuron_sum += fann_mult(weights[2], neurons[2].value);
					case 2: neuron_sum += fann_mult(weights[1], neurons[1].value);
					case 1: neuron_sum += fann_mult(weights[0], neurons[0].value);
					case 0: break;
				}
				for(; i != num_connections; i += 4) {
					neuron_sum += fann_mult(weights[i], neurons[i].value) + fann_mult(weights[i+1], neurons[i+1].value) +
					    fann_mult(weights[i+2], neurons[i+2].value) + fann_mult(weights[i+3], neurons[i+3].value);
				}
				/* unrolled loop end */

				/*
				 * for(i = 0;i != num_connections; i++){
				 * printf("%f += %f*%f, ", neuron_sum, weights[i], neurons[i].value);
				 * neuron_sum += fann_mult(weights[i], neurons[i].value);
				 * }
				 */
			}
			else {
				Fann2::Neuron ** neuron_pointers = ann->connections + neuron_it->first_con;
				i = num_connections & 3; // same as modulo 4 
				switch(i) {
					case 3: neuron_sum += fann_mult(weights[2], neuron_pointers[2]->value);
					case 2: neuron_sum += fann_mult(weights[1], neuron_pointers[1]->value);
					case 1: neuron_sum += fann_mult(weights[0], neuron_pointers[0]->value);
					case 0: break;
				}
				for(; i != num_connections; i += 4) {
					neuron_sum += fann_mult(weights[i], neuron_pointers[i]->value) + fann_mult(weights[i+1], neuron_pointers[i+1]->value) +
					    fann_mult(weights[i+2], neuron_pointers[i+2]->value) + fann_mult(weights[i + 3], neuron_pointers[i + 3]->value);
				}
			}
#ifdef FIXEDFANN
			neuron_it->sum = fann_mult(steepness, neuron_sum);
			if(activation_function != last_activation_function || steepness != last_steepness) {
				switch(activation_function) {
					case FANN_SIGMOID:
					case FANN_SIGMOID_STEPWISE:
					    r1 = ann->sigmoid_results[0];
					    r2 = ann->sigmoid_results[1];
					    r3 = ann->sigmoid_results[2];
					    r4 = ann->sigmoid_results[3];
					    r5 = ann->sigmoid_results[4];
					    r6 = ann->sigmoid_results[5];
					    v1 = ann->sigmoid_values[0] / steepness;
					    v2 = ann->sigmoid_values[1] / steepness;
					    v3 = ann->sigmoid_values[2] / steepness;
					    v4 = ann->sigmoid_values[3] / steepness;
					    v5 = ann->sigmoid_values[4] / steepness;
					    v6 = ann->sigmoid_values[5] / steepness;
					    break;
					case FANN_SIGMOID_SYMMETRIC:
					case FANN_SIGMOID_SYMMETRIC_STEPWISE:
					    r1 = ann->sigmoid_symmetric_results[0];
					    r2 = ann->sigmoid_symmetric_results[1];
					    r3 = ann->sigmoid_symmetric_results[2];
					    r4 = ann->sigmoid_symmetric_results[3];
					    r5 = ann->sigmoid_symmetric_results[4];
					    r6 = ann->sigmoid_symmetric_results[5];
					    v1 = ann->sigmoid_symmetric_values[0] / steepness;
					    v2 = ann->sigmoid_symmetric_values[1] / steepness;
					    v3 = ann->sigmoid_symmetric_values[2] / steepness;
					    v4 = ann->sigmoid_symmetric_values[3] / steepness;
					    v5 = ann->sigmoid_symmetric_values[4] / steepness;
					    v6 = ann->sigmoid_symmetric_values[5] / steepness;
					    break;
					case FANN_THRESHOLD:
					    break;
				}
			}
			switch(activation_function) {
				case FANN_SIGMOID:
				case FANN_SIGMOID_STEPWISE:
				    neuron_it->value = (float)fann_stepwise(v1, v2, v3, v4, v5, v6, r1, r2, r3, r4, r5, r6, 0, multiplier, neuron_sum);
				    break;
				case FANN_SIGMOID_SYMMETRIC:
				case FANN_SIGMOID_SYMMETRIC_STEPWISE:
				    neuron_it->value = (float)fann_stepwise(v1, v2, v3, v4, v5, v6, r1, r2, r3, r4, r5, r6, -multiplier, multiplier, neuron_sum);
				    break;
				case FANN_THRESHOLD:
				    neuron_it->value = (float)((neuron_sum < 0) ? 0 : multiplier);
				    break;
				case FANN_THRESHOLD_SYMMETRIC:
				    neuron_it->value = (float)((neuron_sum < 0) ? -multiplier : multiplier);
				    break;
				case FANN_LINEAR:
				    neuron_it->value = neuron_sum;
				    break;
				case FANN_LINEAR_PIECE:
				    neuron_it->value = (float)((neuron_sum < 0) ? 0 : (neuron_sum > multiplier) ? multiplier : neuron_sum);
				    break;
				case FANN_LINEAR_PIECE_SYMMETRIC:
				    neuron_it->value = (float)((neuron_sum < -multiplier) ? -multiplier : (neuron_sum > multiplier) ? multiplier : neuron_sum);
				    break;
				case FANN_ELLIOT:
				case FANN_ELLIOT_SYMMETRIC:
				case FANN_GAUSSIAN:
				case FANN_GAUSSIAN_SYMMETRIC:
				case FANN_GAUSSIAN_STEPWISE:
				case FANN_SIN_SYMMETRIC:
				case FANN_COS_SYMMETRIC:
				    fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_USE_ACTIVATION);
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
			neuron_it->sum = neuron_sum;
			fann_activation_switch(activation_function, neuron_sum, neuron_it->value);
#endif
		}
	}
	/* set the output */
	output = ann->output;
	num_output = ann->num_output;
	neurons = (ann->last_layer - 1)->first_neuron;
	for(i = 0; i != num_output; i++) {
		output[i] = neurons[i].value;
	}
	return ann->output;
}

FANN_EXTERNAL void FANN_API fann_destroy(Fann2 * ann)
{
	if(ann) {
		ZFREE(ann->weights);
		ZFREE(ann->connections);
		ZFREE(ann->first_layer->first_neuron);
		ZFREE(ann->first_layer);
		ZFREE(ann->output);
		ZFREE(ann->train_errors);
		ZFREE(ann->train_slopes);
		ZFREE(ann->prev_train_slopes);
		ZFREE(ann->prev_steps);
		ZFREE(ann->prev_weights_deltas);
		ZFREE(ann->errstr);
		ZFREE(ann->cascade_activation_functions);
		ZFREE(ann->cascade_activation_steepnesses);
		ZFREE(ann->cascade_candidate_scores);
#ifndef FIXEDFANN
		ZFREE(ann->scale_mean_in);
		ZFREE(ann->scale_deviation_in);
		ZFREE(ann->scale_new_min_in);
		ZFREE(ann->scale_factor_in);
		ZFREE(ann->scale_mean_out);
		ZFREE(ann->scale_deviation_out);
		ZFREE(ann->scale_new_min_out);
		ZFREE(ann->scale_factor_out);
#endif
		ZFREE(ann);
	}
}

FANN_EXTERNAL void FANN_API fann_randomize_weights(Fann2 * ann, float min_weight, float max_weight)
{
	float * weights = ann->weights;
	float * last_weight = weights + ann->total_connections;
	for(; weights != last_weight; weights++) {
		*weights = (float)(fann_rand(min_weight, max_weight));
	}
#ifndef FIXEDFANN
	if(ann->prev_train_slopes != NULL) {
		fann_clear_train_arrays(ann);
	}
#endif
}
// 
// INTERNAL FUNCTION
// Allocates room for the scaling parameters.
// 
static int fann_allocate_scale(Fann2 * ann)
{
	// todo this should only be allocated when needed 
#ifndef FIXEDFANN
	uint i = 0;
#define SCALE_ALLOCATE(what, where, default_value)                                    \
	ann->what ## _ ## where = static_cast<float *>(SAlloc::C(ann->num_ ## where ## put, sizeof(float))); \
	if(ann->what ## _ ## where == NULL) { \
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);                                   \
		fann_destroy(ann);                                                            \
		return 1;                                                                                                               \
	}                                                                                                                                       \
	for(i = 0; i < ann->num_ ## where ## put; i++)                                            \
		ann->what ## _ ## where[ i ] = ( default_value );

	SCALE_ALLOCATE(scale_mean,             in,             0.0)
	SCALE_ALLOCATE(scale_deviation,        in,             1.0)
	SCALE_ALLOCATE(scale_new_min,  in,             -1.0)
	SCALE_ALLOCATE(scale_factor,           in,             1.0)

	SCALE_ALLOCATE(scale_mean,             out,    0.0)
	SCALE_ALLOCATE(scale_deviation,        out,    1.0)
	SCALE_ALLOCATE(scale_new_min,  out,    -1.0)
	SCALE_ALLOCATE(scale_factor,           out,    1.0)
#undef SCALE_ALLOCATE
#endif
	return 0;
}
//
// deep copy of the fann structure 
//
FANN_EXTERNAL Fann2 * FANN_API fann_copy(Fann2* orig)
{
	uint num_layers = (uint)(orig->last_layer - orig->first_layer);
	Fann2::Layer * orig_layer_it, * copy_layer_it;
	uint layer_size;
	Fann2::Neuron * last_neuron, * orig_neuron_it, * copy_neuron_it;
	uint i;
	Fann2::Neuron * orig_first_neuron, * copy_first_neuron;
	uint input_neuron;
	Fann2 * copy = fann_allocate_structure(num_layers);
	if(copy==NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	copy->errno_f = orig->errno_f;
	if(orig->errstr) {
		copy->errstr = static_cast<char *>(SAlloc::M(FANN_ERRSTR_MAX));
		if(copy->errstr == NULL) {
			fann_destroy(copy);
			return NULL;
		}
		strcpy(copy->errstr, orig->errstr);
	}
	copy->error_log = orig->error_log;

	copy->learning_rate = orig->learning_rate;
	copy->learning_momentum = orig->learning_momentum;
	copy->connection_rate = orig->connection_rate;
	copy->network_type = orig->network_type;
	copy->num_MSE = orig->num_MSE;
	copy->MSE_value = orig->MSE_value;
	copy->num_bit_fail = orig->num_bit_fail;
	copy->bit_fail_limit = orig->bit_fail_limit;
	copy->train_error_function = orig->train_error_function;
	copy->train_stop_function = orig->train_stop_function;
	copy->training_algorithm = orig->training_algorithm;
	copy->callback = orig->callback;
	copy->user_data = orig->user_data;
#ifndef FIXEDFANN
	copy->cascade_output_change_fraction = orig->cascade_output_change_fraction;
	copy->cascade_output_stagnation_epochs = orig->cascade_output_stagnation_epochs;
	copy->cascade_candidate_change_fraction = orig->cascade_candidate_change_fraction;
	copy->cascade_candidate_stagnation_epochs = orig->cascade_candidate_stagnation_epochs;
	copy->cascade_best_candidate = orig->cascade_best_candidate;
	copy->cascade_candidate_limit = orig->cascade_candidate_limit;
	copy->cascade_weight_multiplier = orig->cascade_weight_multiplier;
	copy->cascade_max_out_epochs = orig->cascade_max_out_epochs;
	copy->cascade_max_cand_epochs = orig->cascade_max_cand_epochs;

	/* copy cascade activation functions */
	copy->cascade_activation_functions_count = orig->cascade_activation_functions_count;
	copy->cascade_activation_functions = static_cast<Fann2::ActivationFunc *>(SAlloc::R(copy->cascade_activation_functions, copy->cascade_activation_functions_count * sizeof(Fann2::ActivationFunc)));
	if(copy->cascade_activation_functions == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(copy);
		return NULL;
	}
	memcpy(copy->cascade_activation_functions, orig->cascade_activation_functions, copy->cascade_activation_functions_count * sizeof(Fann2::ActivationFunc));
	/* copy cascade activation steepnesses */
	copy->cascade_activation_steepnesses_count = orig->cascade_activation_steepnesses_count;
	copy->cascade_activation_steepnesses = static_cast<float *>(SAlloc::R(copy->cascade_activation_steepnesses, copy->cascade_activation_steepnesses_count * sizeof(float)));
	if(copy->cascade_activation_steepnesses == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(copy);
		return NULL;
	}
	memcpy(copy->cascade_activation_steepnesses, orig->cascade_activation_steepnesses, copy->cascade_activation_steepnesses_count * sizeof(float));
	copy->cascade_num_candidate_groups = orig->cascade_num_candidate_groups;
	/* copy candidate scores, if used */
	if(orig->cascade_candidate_scores == NULL) {
		copy->cascade_candidate_scores = NULL;
	}
	else {
		copy->cascade_candidate_scores = static_cast<float *>(SAlloc::M(fann_get_cascade_num_candidates(copy) * sizeof(float)));
		if(copy->cascade_candidate_scores == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->cascade_candidate_scores, orig->cascade_candidate_scores, fann_get_cascade_num_candidates(copy) * sizeof(float));
	}
#endif /* FIXEDFANN */
	copy->quickprop_decay = orig->quickprop_decay;
	copy->quickprop_mu = orig->quickprop_mu;
	copy->rprop_increase_factor = orig->rprop_increase_factor;
	copy->rprop_decrease_factor = orig->rprop_decrease_factor;
	copy->rprop_delta_min = orig->rprop_delta_min;
	copy->rprop_delta_max = orig->rprop_delta_max;
	copy->rprop_delta_zero = orig->rprop_delta_zero;
	copy->user_data = orig->user_data; // user_data is not deep copied.  user should use fann_copy_with_user_data() for that 
#ifdef FIXEDFANN
	copy->decimal_point = orig->decimal_point;
	copy->multiplier = orig->multiplier;
	memcpy(copy->sigmoid_results, orig->sigmoid_results, 6*sizeof(float));
	memcpy(copy->sigmoid_values, orig->sigmoid_values, 6*sizeof(float));
	memcpy(copy->sigmoid_symmetric_results, orig->sigmoid_symmetric_results, 6*sizeof(float));
	memcpy(copy->sigmoid_symmetric_values, orig->sigmoid_symmetric_values, 6*sizeof(float));
#endif
	/* copy layer sizes, prepare for fann_allocate_neurons */
	for(orig_layer_it = orig->first_layer, copy_layer_it = copy->first_layer;
	    orig_layer_it != orig->last_layer; orig_layer_it++, copy_layer_it++) {
		layer_size = (uint)(orig_layer_it->last_neuron - orig_layer_it->first_neuron);
		copy_layer_it->first_neuron = NULL;
		copy_layer_it->last_neuron = copy_layer_it->first_neuron + layer_size;
		copy->total_neurons += layer_size;
	}
	copy->num_input = orig->num_input;
	copy->num_output = orig->num_output;
	/* copy scale parameters, when used */
#ifndef FIXEDFANN
	if(orig->scale_mean_in != NULL) {
		fann_allocate_scale(copy);
		for(i = 0; i < orig->num_input; i++) {
			copy->scale_mean_in[i] = orig->scale_mean_in[i];
			copy->scale_deviation_in[i] = orig->scale_deviation_in[i];
			copy->scale_new_min_in[i] = orig->scale_new_min_in[i];
			copy->scale_factor_in[i] = orig->scale_factor_in[i];
		}
		for(i = 0; i < orig->num_output; i++) {
			copy->scale_mean_out[i] = orig->scale_mean_out[i];
			copy->scale_deviation_out[i] = orig->scale_deviation_out[i];
			copy->scale_new_min_out[i] = orig->scale_new_min_out[i];
			copy->scale_factor_out[i] = orig->scale_factor_out[i];
		}
	}
#endif
	// copy the neurons 
	fann_allocate_neurons(copy);
	if(copy->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(copy);
		return NULL;
	}
	layer_size = (uint)((orig->last_layer-1)->last_neuron - (orig->last_layer-1)->first_neuron);
	memcpy(copy->output, orig->output, layer_size * sizeof(float));

	last_neuron = (orig->last_layer - 1)->last_neuron;
	for(orig_neuron_it = orig->first_layer->first_neuron, copy_neuron_it = copy->first_layer->first_neuron;
	    orig_neuron_it != last_neuron; orig_neuron_it++, copy_neuron_it++) {
		memcpy(copy_neuron_it, orig_neuron_it, sizeof(Fann2::Neuron));
	}
	/* copy the connections */
	copy->total_connections = orig->total_connections;
	fann_allocate_connections(copy);
	if(copy->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(copy);
		return NULL;
	}
	orig_first_neuron = orig->first_layer->first_neuron;
	copy_first_neuron = copy->first_layer->first_neuron;
	for(i = 0; i < orig->total_connections; i++) {
		copy->weights[i] = orig->weights[i];
		input_neuron = (uint)(orig->connections[i] - orig_first_neuron);
		copy->connections[i] = copy_first_neuron + input_neuron;
	}
	if(orig->train_slopes) {
		copy->train_slopes = static_cast<float *>(SAlloc::M(copy->total_connections_allocated * sizeof(float)));
		if(copy->train_slopes == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->train_slopes, orig->train_slopes, copy->total_connections_allocated * sizeof(float));
	}
	if(orig->prev_steps) {
		copy->prev_steps = static_cast<float *>(SAlloc::M(copy->total_connections_allocated * sizeof(float)));
		if(copy->prev_steps == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->prev_steps, orig->prev_steps, copy->total_connections_allocated * sizeof(float));
	}
	if(orig->prev_train_slopes) {
		copy->prev_train_slopes = static_cast<float *>(SAlloc::M(copy->total_connections_allocated * sizeof(float)));
		if(copy->prev_train_slopes == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->prev_train_slopes, orig->prev_train_slopes, copy->total_connections_allocated * sizeof(float));
	}
	if(orig->prev_weights_deltas) {
		copy->prev_weights_deltas = static_cast<float *>(SAlloc::M(copy->total_connections_allocated * sizeof(float)));
		if(copy->prev_weights_deltas == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(orig), FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->prev_weights_deltas, orig->prev_weights_deltas, copy->total_connections_allocated * sizeof(float));
	}
	return copy;
}

FANN_EXTERNAL void FANN_API fann_print_connections(Fann2 * ann)
{
	Fann2::Layer * layer_it;
	Fann2::Neuron * neuron_it;
	uint i;
	int value;
	uint num_neurons = fann_get_total_neurons(ann) - fann_get_num_output(ann);
	char * neurons = static_cast<char *>(SAlloc::M(num_neurons + 1));
	if(neurons == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}
	neurons[num_neurons] = 0;
	printf("Layer / Neuron ");
	for(i = 0; i < num_neurons; i++) {
		printf("%d", i % 10);
	}
	printf("\n");
	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++) {
		for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++) {
			memset(neurons, (int)'.', num_neurons);
			for(i = neuron_it->first_con; i < neuron_it->last_con; i++) {
				if(ann->weights[i] < 0) {
#ifdef FIXEDFANN
					value = (int)((ann->weights[i] / (double)ann->multiplier) - 0.5);
#else
					value = (int)((ann->weights[i]) - 0.5);
#endif
					if(value < -25)
						value = -25;
					neurons[ann->connections[i] - ann->first_layer->first_neuron] = (char)('a' - value);
				}
				else {
#ifdef FIXEDFANN
					value = (int)((ann->weights[i] / (double)ann->multiplier) + 0.5);
#else
					value = (int)((ann->weights[i]) + 0.5);
#endif
					if(value > 25)
						value = 25;
					neurons[ann->connections[i] - ann->first_layer->first_neuron] = (char)('A' + value);
				}
			}
			printf("L %3d / N %4d %s\n", (int)(layer_it - ann->first_layer), (int)(neuron_it - ann->first_layer->first_neuron), neurons);
		}
	}
	SAlloc::F(neurons);
}

/* Initialize the weights using Widrow + Nguyen's algorithm.
 */
FANN_EXTERNAL void FANN_API fann_init_weights(Fann2 * ann, struct fann_train_data * train_data)
{
	float smallest_inp, largest_inp;
	uint dat = 0, elem, num_connect, num_hidden_neurons;
	Fann2::Layer * layer_it;
	Fann2::Neuron * neuron_it, * last_neuron, * bias_neuron;

#ifdef FIXEDFANN
	uint multiplier = ann->multiplier;
#endif
	float scale_factor;
	for(smallest_inp = largest_inp = train_data->input[0][0]; dat < train_data->num_data; dat++) {
		for(elem = 0; elem < train_data->num_input; elem++) {
			if(train_data->input[dat][elem] < smallest_inp)
				smallest_inp = train_data->input[dat][elem];
			if(train_data->input[dat][elem] > largest_inp)
				largest_inp = train_data->input[dat][elem];
		}
	}
	num_hidden_neurons = (uint)(ann->total_neurons - (ann->num_input + ann->num_output + (ann->last_layer - ann->first_layer)));
	scale_factor = (float)(pow((double)(0.7f * (double)num_hidden_neurons), (double)(1.0f / (double)ann->num_input)) / (double)(largest_inp - smallest_inp));
#ifdef DEBUG
	printf("Initializing weights with scale factor %f\n", scale_factor);
#endif
	bias_neuron = ann->first_layer->last_neuron - 1;
	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++) {
		last_neuron = layer_it->last_neuron;
		if(ann->network_type == FANN_NETTYPE_LAYER) {
			bias_neuron = (layer_it - 1)->last_neuron - 1;
		}

		for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
			for(num_connect = neuron_it->first_con; num_connect < neuron_it->last_con;
			    num_connect++) {
				if(bias_neuron == ann->connections[num_connect]) {
#ifdef FIXEDFANN
					ann->weights[num_connect] = (float)fann_rand(-scale_factor, scale_factor * multiplier);
#else
					ann->weights[num_connect] = (float)fann_rand(-scale_factor, scale_factor);
#endif
				}
				else {
#ifdef FIXEDFANN
					ann->weights[num_connect] = (float)fann_rand(0, scale_factor * multiplier);
#else
					ann->weights[num_connect] = (float)fann_rand(0, scale_factor);
#endif
				}
			}
		}
	}

#ifndef FIXEDFANN
	if(ann->prev_train_slopes != NULL) {
		fann_clear_train_arrays(ann);
	}
#endif
}

FANN_EXTERNAL void FANN_API fann_print_parameters(Fann2 * ann)
{
	Fann2::Layer * layer_it;
#ifndef FIXEDFANN
	uint i;
#endif
	printf("Input layer                          :%4d neurons, 1 bias\n", ann->num_input);
	for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer - 1; layer_it++) {
		if(ann->network_type == FANN_NETTYPE_SHORTCUT) {
			printf("  Hidden layer                       :%4d neurons, 0 bias\n", (int)(layer_it->last_neuron - layer_it->first_neuron));
		}
		else {
			printf("  Hidden layer                       :%4d neurons, 1 bias\n", (int)(layer_it->last_neuron - layer_it->first_neuron - 1));
		}
	}
	printf("Output layer                         :%4d neurons\n", ann->num_output);
	printf("Total neurons and biases             :%4d\n", fann_get_total_neurons(ann));
	printf("Total connections                    :%4d\n", ann->total_connections);
	printf("Connection rate                      :%8.3f\n", ann->connection_rate);
	printf("Network type                         :   %s\n", FANN_NETTYPE_NAMES[ann->network_type]);
#ifdef FIXEDFANN
	printf("Decimal point                        :%4d\n", ann->decimal_point);
	printf("Multiplier                           :%4d\n", ann->multiplier);
#else
	printf("Training algorithm                   :   %s\n", FANN_TRAIN_NAMES[ann->training_algorithm]);
	printf("Training error function              :   %s\n", FANN_ERRORFUNC_NAMES[ann->train_error_function]);
	printf("Training stop function               :   %s\n", FANN_STOPFUNC_NAMES[ann->train_stop_function]);
#endif
#ifdef FIXEDFANN
	printf("Bit fail limit                       :%4d\n", ann->bit_fail_limit);
#else
	printf("Bit fail limit                       :%8.3f\n", ann->bit_fail_limit);
	printf("Learning rate                        :%8.3f\n", ann->learning_rate);
	printf("Learning momentum                    :%8.3f\n", ann->learning_momentum);
	printf("Quickprop decay                      :%11.6f\n", ann->quickprop_decay);
	printf("Quickprop mu                         :%8.3f\n", ann->quickprop_mu);
	printf("RPROP increase factor                :%8.3f\n", ann->rprop_increase_factor);
	printf("RPROP decrease factor                :%8.3f\n", ann->rprop_decrease_factor);
	printf("RPROP delta min                      :%8.3f\n", ann->rprop_delta_min);
	printf("RPROP delta max                      :%8.3f\n", ann->rprop_delta_max);
	printf("Cascade output change fraction       :%11.6f\n", ann->cascade_output_change_fraction);
	printf("Cascade candidate change fraction    :%11.6f\n", ann->cascade_candidate_change_fraction);
	printf("Cascade output stagnation epochs     :%4d\n", ann->cascade_output_stagnation_epochs);
	printf("Cascade candidate stagnation epochs  :%4d\n", ann->cascade_candidate_stagnation_epochs);
	printf("Cascade max output epochs            :%4d\n", ann->cascade_max_out_epochs);
	printf("Cascade min output epochs            :%4d\n", ann->cascade_min_out_epochs);
	printf("Cascade max candidate epochs         :%4d\n", ann->cascade_max_cand_epochs);
	printf("Cascade min candidate epochs         :%4d\n", ann->cascade_min_cand_epochs);
	printf("Cascade weight multiplier            :%8.3f\n", ann->cascade_weight_multiplier);
	printf("Cascade candidate limit              :%8.3f\n", ann->cascade_candidate_limit);
	for(i = 0; i < ann->cascade_activation_functions_count; i++)
		printf("Cascade activation functions[%d]      :   %s\n", i, FANN_ACTIVATIONFUNC_NAMES[ann->cascade_activation_functions[i]]);
	for(i = 0; i < ann->cascade_activation_steepnesses_count; i++)
		printf("Cascade activation steepnesses[%d]    :%8.3f\n", i, ann->cascade_activation_steepnesses[i]);
	printf("Cascade candidate groups             :%4d\n", ann->cascade_num_candidate_groups);
	printf("Cascade no. of candidates            :%4d\n", fann_get_cascade_num_candidates(ann));
	/* TODO: dump scale parameters */
#endif
}

FANN_GET(uint, num_input)
FANN_GET(uint, num_output)

FANN_EXTERNAL uint FANN_API fann_get_total_neurons(const Fann2 * ann)
{
	return ann->network_type ? ann->total_neurons : (ann->total_neurons-1)/* -1, because there is always an unused bias neuron in the last layer */;
}

FANN_GET(uint, total_connections)

FANN_EXTERNAL enum fann_nettype_enum FANN_API fann_get_network_type(const Fann2 * ann)
{
	/* Currently two types: LAYER = 0, SHORTCUT = 1 */
	/* Enum network_types must be set to match the return values  */
	return ann->network_type;
}

FANN_EXTERNAL float FANN_API fann_get_connection_rate(const Fann2 * ann) { return ann->connection_rate; }
FANN_EXTERNAL uint FANN_API fann_get_num_layers(const Fann2 * ann) { return (uint)(ann->last_layer - ann->first_layer); }

FANN_EXTERNAL void FANN_API fann_get_layer_array(Fann2 * ann, uint * layers)
{
	Fann2::Layer * layer_it;

	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		uint count = (uint)(layer_it->last_neuron - layer_it->first_neuron);
		/* Remove the bias from the count of neurons. */
		switch(fann_get_network_type(ann)) {
			case FANN_NETTYPE_LAYER: {
			    --count;
			    break;
		    }
			case FANN_NETTYPE_SHORTCUT: {
			    /* The bias in the first layer is reused for all layers */
			    if(layer_it == ann->first_layer)
				    --count;
			    break;
		    }
			default: {
			    /* Unknown network type, assume no bias present  */
			    break;
		    }
		}
		*layers++ = count;
	}
}

FANN_EXTERNAL void FANN_API fann_get_bias_array(Fann2 * ann, uint * bias)
{
	Fann2::Layer * layer_it;
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; ++layer_it, ++bias) {
		switch(fann_get_network_type(ann)) {
			case FANN_NETTYPE_LAYER: {
			    /* Report one bias in each layer except the last */
			    if(layer_it != ann->last_layer-1)
				    *bias = 1;
			    else
				    *bias = 0;
			    break;
		    }
			case FANN_NETTYPE_SHORTCUT: {
			    /* The bias in the first layer is reused for all layers */
			    if(layer_it == ann->first_layer)
				    *bias = 1;
			    else
				    *bias = 0;
			    break;
		    }
			default: {
			    /* Unknown network type, assume no bias present  */
			    *bias = 0;
			    break;
		    }
		}
	}
}

FANN_EXTERNAL void FANN_API fann_get_connection_array(Fann2 * ann, struct fann_connection * connections)
{
	Fann2::Layer * layer_it;
	Fann2::Neuron * neuron_it;
	uint idx;
	uint source_index;
	uint destination_index;
	Fann2::Neuron * first_neuron = ann->first_layer->first_neuron;
	source_index = 0;
	destination_index = 0;
	/* The following assumes that the last unused bias has no connections */
	/* for each layer */
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		/* for each neuron */
		for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++) {
			/* for each connection */
			for(idx = neuron_it->first_con; idx < neuron_it->last_con; idx++) {
				/* Assign the source, destination and weight */
				connections->from_neuron = (uint)(ann->connections[source_index] - first_neuron);
				connections->to_neuron = destination_index;
				connections->weight = ann->weights[source_index];

				connections++;
				source_index++;
			}
			destination_index++;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_set_weight_array(Fann2 * ann, const struct fann_connection * connections, uint num_connections)
{
	for(uint idx = 0; idx < num_connections; idx++) {
		fann_set_weight(ann, connections[idx].from_neuron, connections[idx].to_neuron, connections[idx].weight);
	}
}

FANN_EXTERNAL void FANN_API fann_set_weight(Fann2 * ann, uint from_neuron, uint to_neuron, float weight)
{
	Fann2::Neuron * first_neuron;
	Fann2::Layer * layer_it;
	Fann2::Neuron * neuron_it;
	uint idx;
	uint source_index;
	uint destination_index;
	first_neuron = ann->first_layer->first_neuron;
	source_index = 0;
	destination_index = 0;
	/* Find the connection, simple brute force search through the network
	   for one or more connections that match to minimize datastructure dependencies.
	   Nothing is done if the connection does not already exist in the network. */

	/* for each layer */
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		/* for each neuron */
		for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++) {
			/* for each connection */
			for(idx = neuron_it->first_con; idx < neuron_it->last_con; idx++) {
				/* If the source and destination neurons match, assign the weight */
				if(((int)from_neuron == ann->connections[source_index] - first_neuron) &&
				    (to_neuron == destination_index)) {
					ann->weights[source_index] = weight;
				}
				source_index++;
			}
			destination_index++;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_get_weights(const Fann2 * ann, float * weights)
	{ memcpy(weights, ann->weights, sizeof(float)*ann->total_connections); }
FANN_EXTERNAL void FANN_API fann_set_weights(Fann2 * ann, const float * weights)
	{ memcpy(ann->weights, weights, sizeof(float)*ann->total_connections); }

FANN_GET_SET(void *, user_data)

#ifdef FIXEDFANN

FANN_GET(uint, decimal_point)
FANN_GET(uint, multiplier)
// 
// INTERNAL FUNCTION
// Adjust the steepwise functions (if used)
// 
void fann_update_stepwise(Fann2 * ann)
{
	uint i = 0;

	/* Calculate the parameters for the stepwise linear
	 * sigmoid function fixed point.
	 * Using a rewritten sigmoid function.
	 * results 0.005, 0.05, 0.25, 0.75, 0.95, 0.995
	 */
	ann->sigmoid_results[0] = MAX((float)(ann->multiplier / 200.0 + 0.5), 1);
	ann->sigmoid_results[1] = MAX((float)(ann->multiplier / 20.0 + 0.5), 1);
	ann->sigmoid_results[2] = MAX((float)(ann->multiplier / 4.0 + 0.5), 1);
	ann->sigmoid_results[3] = MIN(ann->multiplier - (float)(ann->multiplier / 4.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_results[4] = MIN(ann->multiplier - (float)(ann->multiplier / 20.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_results[5] = MIN(ann->multiplier - (float)(ann->multiplier / 200.0 + 0.5), ann->multiplier - 1);

	ann->sigmoid_symmetric_results[0] = MAX((float)((ann->multiplier / 100.0) - ann->multiplier - 0.5), (float)(1 - (float)ann->multiplier));
	ann->sigmoid_symmetric_results[1] =     MAX((float)((ann->multiplier / 10.0) - ann->multiplier - 0.5), (float)(1 - (float)ann->multiplier));
	ann->sigmoid_symmetric_results[2] =     MAX((float)((ann->multiplier / 2.0) - ann->multiplier - 0.5), (float)(1 - (float)ann->multiplier));
	ann->sigmoid_symmetric_results[3] = MIN(ann->multiplier - (float)(ann->multiplier / 2.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_symmetric_results[4] = MIN(ann->multiplier - (float)(ann->multiplier / 10.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_symmetric_results[5] = MIN(ann->multiplier - (float)(ann->multiplier / 100.0 + 1.0), ann->multiplier - 1);

	for(i = 0; i < 6; i++) {
		ann->sigmoid_values[i] = (float)(((log(ann->multiplier / (float)ann->sigmoid_results[i] - 1) * (float)ann->multiplier) / -2.0) * (float)ann->multiplier);
		ann->sigmoid_symmetric_values[i] = (float)(((log((ann->multiplier - (float)ann->sigmoid_symmetric_results[i]) /
		    ((float)ann->sigmoid_symmetric_results[i] + ann->multiplier)) * (float)ann->multiplier) / -2.0) * (float)ann->multiplier);
	}
}

#endif
#ifdef FANN_NO_SEED
	static int FANN_SEED_RAND_2 = 0;
#else
	static int FANN_SEED_RAND_2 = 1;
#endif

FANN_EXTERNAL void FANN_API fann_disable_seed_rand() { FANN_SEED_RAND_2 = 0; }
FANN_EXTERNAL void FANN_API fann_enable_seed_rand() { FANN_SEED_RAND_2 = 1; }
// 
// INTERNAL FUNCTION
// Seed the random function.
// 
static void fann_seed_rand_2()
{
#ifndef _WIN32
	FILE * fp = fopen("/dev/urandom", "r");
	uint foo;
	struct timeval t;
	if(!fp) {
		gettimeofday(&t, NULL);
		foo = t.tv_usec;
#ifdef DEBUG
		printf("unable to open /dev/urandom\n");
#endif
	}
	else {
		if(fread(&foo, sizeof(foo), 1, fp) != 1) {
			gettimeofday(&t, NULL);
			foo = t.tv_usec;
#ifdef DEBUG
			printf("unable to read from /dev/urandom\n");
#endif
		}
		fclose(fp);
	}
	if(FANN_SEED_RAND_2) {
		srand(foo);
	}
#else
	// COMPAT_TIME REPLACEMENT 
	if(FANN_SEED_RAND_2) {
		srand(GetTickCount());
	}
#endif
}
//
// FANN_ERROR
//
#ifdef _MSC_VER
	#define vsnprintf _vsnprintf
	#define snprintf _snprintf
#endif

/* define path max if not defined */
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

FANN_EXTERNAL FILE * FANN_API fann_default_error_log = (FILE *)-1;

/* resets the last error number
 */
FANN_EXTERNAL void FANN_API fann_reset_errno(struct fann_error *errdat)
{
	errdat->errno_f = FANN_E_NO_ERROR;
}
// 
// resets the last errstr
// 
FANN_EXTERNAL void FANN_API fann_reset_errstr(struct fann_error *errdat)
{
	ZFREE(errdat->errstr);
}
// 
// returns the last error number
// 
FANN_EXTERNAL enum fann_errno_enum FANN_API fann_get_errno(const struct fann_error *errdat)
{
	return errdat->errno_f;
}
// 
// returns the last errstr
// 
FANN_EXTERNAL char *FANN_API fann_get_errstr(struct fann_error *errdat)
{
	char *errstr = errdat->errstr;
	fann_reset_errno(errdat);
	fann_reset_errstr(errdat);
	return errstr;
}
// 
// change where errors are logged to
// 
FANN_EXTERNAL void FANN_API fann_set_error_log(struct fann_error *errdat, FILE * log_file)
{
	if(errdat == NULL)
		fann_default_error_log = log_file;
	else
		errdat->error_log = log_file;
}
// 
// prints the last error to stderr
// 
FANN_EXTERNAL void FANN_API fann_print_error(struct fann_error *errdat)
{
	if(errdat->errno_f != FANN_E_NO_ERROR && errdat->errstr != NULL) {
		slfprintf_stderr("FANN Error %d: %s", errdat->errno_f, errdat->errstr);
	}
}
// 
// INTERNAL FUNCTION
// Populate the error information
//
void fann_error_2(struct fann_error *errdat, const enum fann_errno_enum errno_f, ...)
{
	va_list ap;
	size_t errstr_max = FANN_ERRSTR_MAX + PATH_MAX - 1;
	char errstr[FANN_ERRSTR_MAX + PATH_MAX];
	FILE * error_log = fann_default_error_log;
	if(errdat != NULL)
		errdat->errno_f = errno_f;
	va_start(ap, errno_f);
	switch(errno_f) {
		case FANN_E_NO_ERROR:
			return;
		case FANN_E_CANT_OPEN_CONFIG_R: vsnprintf(errstr, errstr_max, "Unable to open configuration file \"%s\" for reading.\n", ap); break;
		case FANN_E_CANT_OPEN_CONFIG_W: vsnprintf(errstr, errstr_max, "Unable to open configuration file \"%s\" for writing.\n", ap); break;
		case FANN_E_WRONG_CONFIG_VERSION: vsnprintf(errstr, errstr_max, "Wrong version of configuration file, aborting read of configuration file \"%s\".\n", ap); break;
		case FANN_E_CANT_READ_CONFIG: vsnprintf(errstr, errstr_max, "Error reading \"%s\" from configuration file \"%s\".\n", ap); break;
		case FANN_E_CANT_READ_NEURON: vsnprintf(errstr, errstr_max, "Error reading neuron info from configuration file \"%s\".\n", ap); break;
		case FANN_E_CANT_READ_CONNECTIONS: vsnprintf(errstr, errstr_max, "Error reading connections from configuration file \"%s\".\n", ap); break;
		case FANN_E_WRONG_NUM_CONNECTIONS: vsnprintf(errstr, errstr_max, "ERROR connections_so_far=%d, total_connections=%d\n", ap); break;
		case FANN_E_CANT_OPEN_TD_W: vsnprintf(errstr, errstr_max, "Unable to open train data file \"%s\" for writing.\n", ap); break;
		case FANN_E_CANT_OPEN_TD_R: vsnprintf(errstr, errstr_max, "Unable to open train data file \"%s\" for writing.\n", ap); break;
		case FANN_E_CANT_READ_TD: vsnprintf(errstr, errstr_max, "Error reading info from train data file \"%s\", line: %d.\n", ap); break;
		case FANN_E_CANT_ALLOCATE_MEM: strcpy(errstr, "Unable to allocate memory.\n"); break;
		case FANN_E_CANT_TRAIN_ACTIVATION: strcpy(errstr, "Unable to train with the selected activation function.\n"); break;
		case FANN_E_CANT_USE_ACTIVATION: strcpy(errstr, "Unable to use the selected activation function.\n"); break;
		case FANN_E_TRAIN_DATA_MISMATCH: strcpy(errstr, "Training data must be of equivalent structure.\n"); break;
		case FANN_E_CANT_USE_TRAIN_ALG: strcpy(errstr, "Unable to use the selected training algorithm.\n"); break;
		case FANN_E_TRAIN_DATA_SUBSET: vsnprintf(errstr, errstr_max, "Subset from %d of length %d not valid in training set of length %d.\n", ap); break;
		case FANN_E_INDEX_OUT_OF_BOUND: vsnprintf(errstr, errstr_max, "Index %d is out of bound.\n", ap); break;
		case FANN_E_SCALE_NOT_PRESENT: strcpy(errstr, "Scaling parameters not present.\n"); break;
		case FANN_E_INPUT_NO_MATCH: vsnprintf(errstr, errstr_max, "The number of input neurons in the ann (%d) and data (%d) don't match\n", ap); break;
		case FANN_E_OUTPUT_NO_MATCH: vsnprintf(errstr, errstr_max, "The number of output neurons in the ann (%d) and data (%d) don't match\n", ap); break; 
		case FANN_E_WRONG_PARAMETERS_FOR_CREATE: strcpy(errstr, "The parameters for create_standard are wrong, either too few parameters provided or a negative/very high value provided.\n"); break;
	}
	va_end(ap);
	if(errdat) {
		if(errdat->errstr == NULL) {
			errdat->errstr = static_cast<char *>(SAlloc::M(strlen(errstr) + 1));
		}
		else if(strlen(errdat->errstr) < strlen(errstr)) {
			errdat->errstr = static_cast<char *>(SAlloc::R(errdat->errstr, strlen(errstr) + 1));
		}
		/* allocation failed */
		if(errdat->errstr == NULL) {
			slfprintf_stderr("Unable to allocate memory.\n");
			return;
		}
		strcpy(errdat->errstr, errstr);
		error_log = errdat->error_log;
	}
	if(error_log == (FILE *)-1) { /* This is the default behavior and will give stderr */
		slfprintf_stderr("FANN Error %d: %s", errno_f, errstr);
	}
	else if(error_log != NULL) {
		fprintf(error_log, "FANN Error %d: %s", errno_f, errstr);
	}
}
// 
// INTERNAL FUNCTION
// Initialize an error data strcuture
// 
void fann_init_error_data(struct fann_error *errdat)
{
	errdat->errstr = NULL;
	errdat->errno_f = FANN_E_NO_ERROR;
	errdat->error_log = fann_default_error_log;
}

#ifndef FIXEDFANN // {
// 
// INTERNAL FUNCTION
// Calculates the derived of a value, given an activation function and a steepness
// 
static FORCEINLINE float fann_activation_derived(uint activation_function, float steepness, float value, float sum)
{
	switch(activation_function) {
		case Fann2::FANN_LINEAR:
		case Fann2::FANN_LINEAR_PIECE:
		case Fann2::FANN_LINEAR_PIECE_SYMMETRIC:
		    return (float)fann_linear_derive(steepness, value);
		case Fann2::FANN_SIGMOID:
		case Fann2::FANN_SIGMOID_STEPWISE:
		    value = fann_clip(value, 0.01f, 0.99f);
		    return (float)fann_sigmoid_derive(steepness, value);
		case Fann2::FANN_SIGMOID_SYMMETRIC:
		case Fann2::FANN_SIGMOID_SYMMETRIC_STEPWISE:
		    value = fann_clip(value, -0.98f, 0.98f);
		    return (float)fann_sigmoid_symmetric_derive(steepness, value);
		case Fann2::FANN_GAUSSIAN:
		    // value = fann_clip(value, 0.01f, 0.99f); 
		    return (float)fann_gaussian_derive(steepness, value, sum);
		case Fann2::FANN_GAUSSIAN_SYMMETRIC:
		    // value = fann_clip(value, -0.98f, 0.98f); 
		    return (float)fann_gaussian_symmetric_derive(steepness, value, sum);
		case Fann2::FANN_ELLIOT:
		    value = fann_clip(value, 0.01f, 0.99f);
		    return (float)fann_elliot_derive(steepness, value, sum);
		case Fann2::FANN_ELLIOT_SYMMETRIC: 
			value = fann_clip(value, -0.98f, 0.98f);
		    return (float)fann_elliot_symmetric_derive(steepness, value, sum);
		case Fann2::FANN_SIN_SYMMETRIC: return (float)fann_sin_symmetric_derive(steepness, sum);
		case Fann2::FANN_COS_SYMMETRIC: return (float)fann_cos_symmetric_derive(steepness, sum);
		case Fann2::FANN_SIN: return (float)fann_sin_derive(steepness, sum);
		case Fann2::FANN_COS: return (float)fann_cos_derive(steepness, sum);
		case Fann2::FANN_THRESHOLD:
		    fann_error_2(NULL, FANN_E_CANT_TRAIN_ACTIVATION);
	}
	return 0;
}
#endif
// 
// FANN_CASCADE
//
#ifndef FIXEDFANN // {

/* #define CASCADE_DEBUG */
/* #define CASCADE_DEBUG_FULL */

void fann_print_connections_raw(const Fann2 * ann)
{
	for(uint i = 0; i < ann->total_connections_allocated; i++) {
		if(i == ann->total_connections) {
			printf("* ");
		}
		printf("%f ", ann->weights[i]);
	}
	printf("\n\n");
}
// 
// Cascade training directly on the training data.
// The connected_neurons pointers are not valid during training,
// but they will be again after training.
// 
FANN_EXTERNAL void FANN_API fann_cascadetrain_on_data(Fann2 * ann, struct fann_train_data * data, uint max_neurons, uint neurons_between_reports, float desired_error)
{
	float error;
	uint i;
	uint total_epochs = 0;
	int desired_error_reached;
	if(neurons_between_reports && ann->callback == NULL) {
		printf("Max neurons %3d. Desired error: %.6f\n", max_neurons, desired_error);
	}
	for(i = 1; i <= max_neurons; i++) {
		/* train output neurons */
		total_epochs += fann_train_outputs(ann, data, desired_error);
		error = fann_get_MSE(ann);
		desired_error_reached = fann_desired_error_reached(ann, desired_error);
		/* print current error */
		if(neurons_between_reports && (i % neurons_between_reports == 0 || i == max_neurons || i == 1 || desired_error_reached == 0)) {
			if(ann->callback == NULL) {
				printf("Neurons     %3d. Current error: %.6f. Total error:%8.4f. Epochs %5d. Bit fail %3d",
				    i-1, error, ann->MSE_value, total_epochs, ann->num_bit_fail);
				if((ann->last_layer-2) != ann->first_layer) {
					printf(". candidate steepness %.2f. function %s",
					    (ann->last_layer-2)->first_neuron->activation_steepness,
					    FANN_ACTIVATIONFUNC_NAMES[(ann->last_layer-2)->first_neuron->activation_function]);
				}
				printf("\n");
			}
			else if((*ann->callback)(ann, data, max_neurons, neurons_between_reports, desired_error, total_epochs) == -1) {
				break; // you can break the training by returning -1 
			}
		}
		if(desired_error_reached == 0)
			break;
		if(fann_initialize_candidates(ann) == -1) {
			break; // Unable to initialize room for candidates 
		}
		total_epochs += fann_train_candidates(ann, data); // train new candidates 
		fann_install_candidate(ann); // this installs the best candidate 
	}
	/* Train outputs one last time but without any desired error */
	total_epochs += fann_train_outputs(ann, data, 0.0);
	if(neurons_between_reports && ann->callback == NULL) {
		printf("Train outputs    Current error: %.6f. Epochs %6d\n", fann_get_MSE(ann), total_epochs);
	}
	/* Set pointers in connected_neurons
	 * This is ONLY done in the end of cascade training,
	 * since there is no need for them during training.
	 */
	fann_set_shortcut_connections(ann);
}

FANN_EXTERNAL void FANN_API fann_cascadetrain_on_file(Fann2 * ann, const char * filename, uint max_neurons, uint neurons_between_reports, float desired_error)
{
	struct fann_train_data * data = fann_read_train_from_file(filename);
	if(data) {
		fann_cascadetrain_on_data(ann, data, max_neurons, neurons_between_reports, desired_error);
		fann_destroy_train(data);
	}
}

int fann_train_outputs(Fann2 * ann, struct fann_train_data * data, float desired_error)
{
	float error, initial_error, error_improvement;
	float target_improvement = 0.0;
	float backslide_improvement = -1.0e20f;
	uint i;
	uint max_epochs = ann->cascade_max_out_epochs;
	uint min_epochs = ann->cascade_min_out_epochs;
	uint stagnation = max_epochs;
	/* TODO should perhaps not clear all arrays */
	fann_clear_train_arrays(ann);
	/* run an initial epoch to set the initital error */
	initial_error = fann_train_outputs_epoch(ann, data);
	if(fann_desired_error_reached(ann, desired_error) == 0)
		return 1;
	for(i = 1; i < max_epochs; i++) {
		error = fann_train_outputs_epoch(ann, data);
		/*printf("Epoch %6d. Current error: %.6f. Bit fail %d.\n", i, error, ann->num_bit_fail); */
		if(fann_desired_error_reached(ann, desired_error) == 0) {
#ifdef CASCADE_DEBUG
			printf("Error %f < %f\n", error, desired_error);
#endif
			return i + 1;
		}
		// Improvement since start of train 
		error_improvement = initial_error - error;
		// After any significant change, set a new goal and allow a new quota of epochs to reach it 
		if((target_improvement >= 0 && (error_improvement > target_improvement || error_improvement < backslide_improvement)) ||
		    (target_improvement < 0 && (error_improvement < target_improvement || error_improvement > backslide_improvement))) {
			/*printf("error_improvement=%f, target_improvement=%f, backslide_improvement=%f,
			   stagnation=%d\n", error_improvement, target_improvement, backslide_improvement, stagnation);
			   */
			target_improvement = error_improvement * (1.0f + ann->cascade_output_change_fraction);
			backslide_improvement = error_improvement * (1.0f - ann->cascade_output_change_fraction);
			stagnation = i + ann->cascade_output_stagnation_epochs;
		}
		/* No improvement in allotted period, so quit */
		if(i >= stagnation && i >= min_epochs) {
			return i + 1;
		}
	}
	return max_epochs;
}

float fann_train_outputs_epoch(Fann2 * ann, struct fann_train_data * data)
{
	fann_reset_MSE(ann);
	for(uint i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_update_slopes_batch(ann, ann->last_layer - 1, ann->last_layer - 1);
	}
	switch(ann->training_algorithm) {
		case FANN_TRAIN_RPROP:
		    fann_update_weights_irpropm(ann, (ann->last_layer - 1)->first_neuron->first_con, ann->total_connections);
		    break;
		case FANN_TRAIN_SARPROP:
		    fann_update_weights_sarprop(ann, ann->sarprop_epoch, (ann->last_layer - 1)->first_neuron->first_con, ann->total_connections);
		    ++(ann->sarprop_epoch);
		    break;
		case FANN_TRAIN_QUICKPROP:
		    fann_update_weights_quickprop(ann, data->num_data, (ann->last_layer - 1)->first_neuron->first_con, ann->total_connections);
		    break;
		case FANN_TRAIN_BATCH:
		case FANN_TRAIN_INCREMENTAL:
		    fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_USE_TRAIN_ALG);
	}
	return fann_get_MSE(ann);
}

int fann_reallocate_connections(Fann2 * ann, uint total_connections)
{
	/* The connections are allocated, but the pointers inside are
	 * first moved in the end of the cascade training session.
	 */
#ifdef CASCADE_DEBUG
	printf("realloc from %d to %d\n", ann->total_connections_allocated, total_connections);
#endif
	ann->connections = static_cast<Fann2::Neuron **>(SAlloc::R(ann->connections, total_connections * sizeof(Fann2::Neuron *)));
	if(ann->connections == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->weights = static_cast<float *>(SAlloc::R(ann->weights, total_connections * sizeof(float)));
	if(ann->weights == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->train_slopes = static_cast<float *>(SAlloc::R(ann->train_slopes, total_connections * sizeof(float)));
	if(ann->train_slopes == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->prev_steps = static_cast<float *>(SAlloc::R(ann->prev_steps, total_connections * sizeof(float)));
	if(ann->prev_steps == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->prev_train_slopes = static_cast<float *>(SAlloc::R(ann->prev_train_slopes, total_connections * sizeof(float)));
	if(ann->prev_train_slopes == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->total_connections_allocated = total_connections;
	return 0;
}

int fann_reallocate_neurons(Fann2 * ann, uint total_neurons)
{
	Fann2::Layer * layer_it;
	Fann2::Neuron * neurons;
	uint num_neurons = 0;
	uint num_neurons_so_far = 0;
	neurons = static_cast<Fann2::Neuron *>(SAlloc::R(ann->first_layer->first_neuron, total_neurons * sizeof(Fann2::Neuron)));
	ann->total_neurons_allocated = total_neurons;
	if(neurons == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	// Also allocate room for more train_errors 
	ann->train_errors = static_cast<float *>(SAlloc::R(ann->train_errors, total_neurons * sizeof(float)));
	if(ann->train_errors == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	if(neurons != ann->first_layer->first_neuron) {
		/* Then the memory has moved, also move the pointers */
#ifdef CASCADE_DEBUG_FULL
		printf("Moving neuron pointers\n");
#endif
		/* Move pointers from layers to neurons */
		for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
			num_neurons = (uint)(layer_it->last_neuron - layer_it->first_neuron);
			layer_it->first_neuron = neurons + num_neurons_so_far;
			layer_it->last_neuron = layer_it->first_neuron + num_neurons;
			num_neurons_so_far += num_neurons;
		}
	}
	return 0;
}

void initialize_candidate_weights(Fann2 * ann, uint first_con, uint last_con, float scale_factor)
{
	float prev_step;
	const uint bias_weight = (uint)(first_con + (ann->first_layer->last_neuron - ann->first_layer->first_neuron) - 1);
	if(ann->training_algorithm == FANN_TRAIN_RPROP)
		prev_step = ann->rprop_delta_zero;
	else
		prev_step = 0;
	for(uint i = first_con; i < last_con; i++) {
		if(i == bias_weight)
			ann->weights[i] = fann_rand(-scale_factor, scale_factor);
		else
			ann->weights[i] = fann_rand(0, scale_factor);
		ann->train_slopes[i] = 0;
		ann->prev_steps[i] = prev_step;
		ann->prev_train_slopes[i] = 0;
	}
}

int fann_initialize_candidates(Fann2 * ann)
{
	// 
	// The candidates are allocated after the normal neurons and connections,
	// but there is an empty place between the real neurons and the candidate neurons,
	// so that it will be possible to make room when the chosen candidate are copied in
	// on the desired place.
	// 
	uint neurons_to_allocate, connections_to_allocate;
	uint num_candidates = fann_get_cascade_num_candidates(ann);
	uint num_neurons = ann->total_neurons + num_candidates + 1;
	uint num_hidden_neurons = ann->total_neurons - ann->num_input - ann->num_output;
	uint candidate_connections_in = ann->total_neurons - ann->num_output;
	uint candidate_connections_out = ann->num_output;
	// the number of connections going into a and out of a candidate is ann->total_neurons 
	uint num_connections = ann->total_connections + (ann->total_neurons * (num_candidates + 1));
	uint first_candidate_connection = ann->total_connections + ann->total_neurons;
	uint first_candidate_neuron = ann->total_neurons + 1;
	uint connection_it, i, j, k, candidate_index;
	Fann2::Neuron * neurons;
	float scale_factor;
	// 
	// First make sure that there is enough room, and if not then allocate a
	// bit more so that we do not need to allocate more room each time.
	// 
	if(num_neurons > ann->total_neurons_allocated) {
		// Then we need to allocate more neurons
		// Allocate half as many neurons as already exist (at least ten)
		neurons_to_allocate = num_neurons + num_neurons / 2;
		if(neurons_to_allocate < num_neurons + 10) {
			neurons_to_allocate = num_neurons + 10;
		}
		if(fann_reallocate_neurons(ann, neurons_to_allocate) == -1) {
			return -1;
		}
	}
	if(num_connections > ann->total_connections_allocated) {
		// Then we need to allocate more connections
		// Allocate half as many connections as already exist (at least enough for ten neurons)
		connections_to_allocate = num_connections + num_connections / 2;
		if(connections_to_allocate < num_connections + ann->total_neurons * 10) {
			connections_to_allocate = num_connections + ann->total_neurons * 10;
		}
		if(fann_reallocate_connections(ann, connections_to_allocate) == -1) {
			return -1;
		}
	}
	// Some code to do semi Widrow + Nguyen initialization 
	scale_factor = (float)(2.0 * pow(0.7f * (float)num_hidden_neurons, 1.0f / (float)ann->num_input));
	if(scale_factor > 8)
		scale_factor = 8;
	else if(scale_factor < 0.5)
		scale_factor = 0.5;
	//
	// Set the neurons.
	//
	connection_it = first_candidate_connection;
	neurons = ann->first_layer->first_neuron;
	candidate_index = first_candidate_neuron;
	for(i = 0; i < ann->cascade_activation_functions_count; i++) {
		for(j = 0; j < ann->cascade_activation_steepnesses_count; j++) {
			for(k = 0; k < ann->cascade_num_candidate_groups; k++) {
				// TODO candidates should actually be created both in the last layer before the output layer, and in a new layer.
				neurons[candidate_index].value = 0;
				neurons[candidate_index].sum = 0;
				neurons[candidate_index].activation_function = ann->cascade_activation_functions[i];
				neurons[candidate_index].activation_steepness = ann->cascade_activation_steepnesses[j];
				neurons[candidate_index].first_con = connection_it;
				connection_it += candidate_connections_in;
				neurons[candidate_index].last_con = connection_it;
				// We have no specific pointers to the output weights, but they are available after last_con 
				connection_it += candidate_connections_out;
				ann->train_errors[candidate_index] = 0;
				initialize_candidate_weights(ann, neurons[candidate_index].first_con, neurons[candidate_index].last_con+candidate_connections_out, scale_factor);
				candidate_index++;
			}
		}
	}
	// Now randomize the weights and zero out the arrays that needs zeroing out.
	/*
	 #ifdef CASCADE_DEBUG_FULL
	   printf("random cand weight [%d ... %d]\n", first_candidate_connection, num_connections - 1);
	 #endif
	   for(i = first_candidate_connection; i < num_connections; i++) {
	       //ann->weights[i] = fann_random_weight();
	       ann->weights[i] = fann_rand(-2.0,2.0);
	       ann->train_slopes[i] = 0;
	       ann->prev_steps[i] = 0;
	       ann->prev_train_slopes[i] = initial_slope;
	   }
	 */
	return 0;
}

int fann_train_candidates(Fann2 * ann, struct fann_train_data * data)
{
	float best_cand_score = 0.0;
	float target_cand_score = 0.0;
	float backslide_cand_score = -1.0e20f;
	uint i;
	uint max_epochs = ann->cascade_max_cand_epochs;
	uint min_epochs = ann->cascade_min_cand_epochs;
	uint stagnation = max_epochs;
	if(ann->cascade_candidate_scores == NULL) {
		ann->cascade_candidate_scores = static_cast<float *>(SAlloc::M(fann_get_cascade_num_candidates(ann) * sizeof(float)));
		if(ann->cascade_candidate_scores == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
			return 0;
		}
	}
	for(i = 0; i < max_epochs; i++) {
		best_cand_score = fann_train_candidates_epoch(ann, data);
		if(best_cand_score / ann->MSE_value > ann->cascade_candidate_limit) {
#ifdef CASCADE_DEBUG
			printf("above candidate limit %f/%f > %f", best_cand_score, ann->MSE_value, ann->cascade_candidate_limit);
#endif
			return i + 1;
		}
		if((best_cand_score > target_cand_score) || (best_cand_score < backslide_cand_score)) {
#ifdef CASCADE_DEBUG_FULL
			printf("Best candidate score %f, real score: %f\n", ann->MSE_value - best_cand_score, best_cand_score);
			/* printf("best_cand_score=%f, target_cand_score=%f, backslide_cand_score=%f, stagnation=%d\n",
			   best_cand_score, target_cand_score, backslide_cand_score, stagnation); */
#endif
			target_cand_score = best_cand_score * (1.0f + ann->cascade_candidate_change_fraction);
			backslide_cand_score = best_cand_score * (1.0f - ann->cascade_candidate_change_fraction);
			stagnation = i + ann->cascade_candidate_stagnation_epochs;
		}
		/* No improvement in allotted period, so quit */
		if(i >= stagnation && i >= min_epochs) {
#ifdef CASCADE_DEBUG
			printf("Stagnation with %d epochs, best candidate score %f, real score: %f\n", i + 1, ann->MSE_value - best_cand_score, best_cand_score);
#endif
			return i + 1;
		}
	}
#ifdef CASCADE_DEBUG
	printf("Max epochs %d reached, best candidate score %f, real score: %f\n", max_epochs, ann->MSE_value - best_cand_score, best_cand_score);
#endif
	return max_epochs;
}

void fann_update_candidate_slopes(Fann2 * ann)
{
	Fann2::Neuron * neurons = ann->first_layer->first_neuron;
	Fann2::Neuron * first_cand = neurons + ann->total_neurons + 1;
	Fann2::Neuron * last_cand = first_cand + fann_get_cascade_num_candidates(ann);
	Fann2::Neuron * cand_it;
	uint i, j, num_connections;
	uint num_output = ann->num_output;
	float max_sum, cand_sum, activation, derived, error_value, diff, cand_score;
	float * weights, * cand_out_weights, * cand_slopes, * cand_out_slopes;
	float * output_train_errors = ann->train_errors + (ann->total_neurons - ann->num_output);
	for(cand_it = first_cand; cand_it < last_cand; cand_it++) {
		cand_score = ann->cascade_candidate_scores[cand_it - first_cand];
		error_value = 0.0;
		// code more or less stolen from fann_run to fast forward pass
		cand_sum = 0.0;
		num_connections = cand_it->last_con - cand_it->first_con;
		weights = ann->weights + cand_it->first_con;
		/* unrolled loop start */
		i = num_connections & 3;        /* same as modulo 4 */
		switch(i) {
			case 3: cand_sum += weights[2] * neurons[2].value;
			case 2: cand_sum += weights[1] * neurons[1].value;
			case 1: cand_sum += weights[0] * neurons[0].value;
			case 0: break;
		}
		for(; i != num_connections; i += 4) {
			cand_sum += weights[i] * neurons[i].value + weights[i+1] * neurons[i+1].value + weights[i+2] * neurons[i+2].value + weights[i + 3] * neurons[i + 3].value;
		}
		/*
		 * for(i = 0; i < num_connections; i++){
		 * cand_sum += weights[i] * neurons[i].value;
		 * }
		 */
		/* unrolled loop end */

		max_sum = 150/cand_it->activation_steepness;
		if(cand_sum > max_sum)
			cand_sum = max_sum;
		else if(cand_sum < -max_sum)
			cand_sum = -max_sum;
		activation = fann_activation(ann, cand_it->activation_function, cand_it->activation_steepness, cand_sum);
		/* printf("%f = sigmoid(%f);\n", activation, cand_sum); */
		cand_it->sum = cand_sum;
		cand_it->value = activation;
		derived = fann_activation_derived(cand_it->activation_function, cand_it->activation_steepness, activation, cand_sum);
		// The output weights is located right after the input weights in the weight array.
		cand_out_weights = weights + num_connections;
		cand_out_slopes = ann->train_slopes + cand_it->first_con + num_connections;
		for(j = 0; j < num_output; j++) {
			diff = (activation * cand_out_weights[j]) - output_train_errors[j];
#ifdef CASCADE_DEBUG_FULL
			// printf("diff = %f = (%f * %f) - %f;\n", diff, activation, cand_out_weights[j], output_train_errors[j]);
#endif
			cand_out_slopes[j] -= 2.0f * diff * activation;
#ifdef CASCADE_DEBUG_FULL
			// printf("cand_out_slopes[%d] <= %f += %f * %f;\n", j, cand_out_slopes[j], diff, activation);
#endif
			error_value += diff * cand_out_weights[j];
			cand_score -= (diff * diff);
#ifdef CASCADE_DEBUG_FULL
			// printf("cand_score[%d][%d] = %f -= (%f * %f)\n", cand_it - first_cand, j, cand_score, diff, diff);
			printf("cand[%d]: error=%f, activation=%f, diff=%f, slope=%f\n", cand_it - first_cand,
			    output_train_errors[j], (activation * cand_out_weights[j]), diff, -2.0 * diff * activation);
#endif
		}
		ann->cascade_candidate_scores[cand_it - first_cand] = cand_score;
		error_value *= derived;
		cand_slopes = ann->train_slopes + cand_it->first_con;
		for(i = 0; i < num_connections; i++) {
			cand_slopes[i] -= error_value * neurons[i].value;
		}
	}
}

void fann_update_candidate_weights(Fann2 * ann, uint num_data)
{
	Fann2::Neuron * first_cand = (ann->last_layer - 1)->last_neuron + 1; // there is an empty neuron between the actual neurons and the candidate neuron 
	Fann2::Neuron * last_cand = first_cand + fann_get_cascade_num_candidates(ann) - 1;
	switch(ann->training_algorithm) {
		case FANN_TRAIN_RPROP:
		    fann_update_weights_irpropm(ann, first_cand->first_con,
			last_cand->last_con + ann->num_output);
		    break;
		case FANN_TRAIN_SARPROP:
		    /* TODO: increase epoch? */
		    fann_update_weights_sarprop(ann, ann->sarprop_epoch, first_cand->first_con,
			last_cand->last_con + ann->num_output);
		    break;
		case FANN_TRAIN_QUICKPROP:
		    fann_update_weights_quickprop(ann, num_data, first_cand->first_con,
			last_cand->last_con + ann->num_output);
		    break;
		case FANN_TRAIN_BATCH:
		case FANN_TRAIN_INCREMENTAL:
		    fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_USE_TRAIN_ALG);
		    break;
	}
}

float fann_train_candidates_epoch(Fann2 * ann, struct fann_train_data * data)
{
	uint i, j;
	uint best_candidate;
	float best_score;
	uint num_cand = fann_get_cascade_num_candidates(ann);
	float * output_train_errors = ann->train_errors + (ann->total_neurons - ann->num_output);
	Fann2::Neuron * output_neurons = (ann->last_layer - 1)->first_neuron;

	for(i = 0; i < num_cand; i++) {
		/* The ann->MSE_value is actually the sum squared error */
		ann->cascade_candidate_scores[i] = ann->MSE_value;
	}
	/*printf("start score: %f\n", ann->MSE_value); */

	for(i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		for(j = 0; j < ann->num_output; j++) {
			/* TODO only debug, but the error is in opposite direction, this might be usefull info */
			/*          if(output_train_errors[j] != (ann->output[j] - data->output[i][j])){
			 * printf("difference in calculated error at %f != %f; %f = %f - %f;\n", output_train_errors[j],
			 *(ann->output[j] - data->output[i][j]), output_train_errors[j], ann->output[j],
			 *data->output[i][j]);
			 * } */
			// 
			// output_train_errors[j] = (data->output[i][j] - ann->output[j])/2;
			// output_train_errors[j] = ann->output[j] - data->output[i][j];
			// 
			output_train_errors[j] = (data->output[i][j] - ann->output[j]);
			switch(output_neurons[j].activation_function) {
				case Fann2::FANN_LINEAR_PIECE_SYMMETRIC:
				case Fann2::FANN_SIGMOID_SYMMETRIC:
				case Fann2::FANN_SIGMOID_SYMMETRIC_STEPWISE:
				case Fann2::FANN_THRESHOLD_SYMMETRIC:
				case Fann2::FANN_ELLIOT_SYMMETRIC:
				case Fann2::FANN_GAUSSIAN_SYMMETRIC:
				case Fann2::FANN_SIN_SYMMETRIC:
				case Fann2::FANN_COS_SYMMETRIC:
				    output_train_errors[j] /= 2.0;
				    break;
				case Fann2::FANN_LINEAR:
				case Fann2::FANN_THRESHOLD:
				case Fann2::FANN_SIGMOID:
				case Fann2::FANN_SIGMOID_STEPWISE:
				case Fann2::FANN_GAUSSIAN:
				case Fann2::FANN_GAUSSIAN_STEPWISE:
				case Fann2::FANN_ELLIOT:
				case Fann2::FANN_LINEAR_PIECE:
				case Fann2::FANN_SIN:
				case Fann2::FANN_COS:
				    break;
			}
		}
		fann_update_candidate_slopes(ann);
	}
	fann_update_candidate_weights(ann, data->num_data);
	/* find the best candidate score */
	best_candidate = 0;
	best_score = ann->cascade_candidate_scores[best_candidate];
	for(i = 1; i < num_cand; i++) {
		/*Fann2::Neuron *cand = ann->first_layer->first_neuron + ann->total_neurons + 1 + i;
		 * printf("candidate[%d] = activation: %s, steepness: %f, score: %f\n",
		 * i, FANN_ACTIVATIONFUNC_NAMES[cand->activation_function],
		 * cand->activation_steepness, ann->cascade_candidate_scores[i]); */
		if(ann->cascade_candidate_scores[i] > best_score) {
			best_candidate = i;
			best_score = ann->cascade_candidate_scores[best_candidate];
		}
	}
	ann->cascade_best_candidate = ann->total_neurons + best_candidate + 1;
#ifdef CASCADE_DEBUG
	printf("Best candidate[%d]: with score %f, real score: %f\n", best_candidate, ann->MSE_value - best_score, best_score);
#endif
	return best_score;
}
//
// add a layer at the position pointed to by *layer 
//
Fann2::Layer * fann_add_layer(Fann2 * ann, Fann2::Layer * layer)
{
	int layer_pos = (int)(layer - ann->first_layer);
	int num_layers = (int)(ann->last_layer - ann->first_layer + 1);
	int i;
	// allocate the layer 
	Fann2::Layer * layers = static_cast<Fann2::Layer *>(SAlloc::R(ann->first_layer, num_layers * sizeof(Fann2::Layer)));
	if(layers == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	// copy layers so that the free space is at the right location 
	for(i = num_layers - 1; i >= layer_pos; i--) {
		layers[i] = layers[i - 1];
	}
	/* the newly allocated layer is empty */
	layers[layer_pos].first_neuron = layers[layer_pos + 1].first_neuron;
	layers[layer_pos].last_neuron = layers[layer_pos + 1].first_neuron;
	/* Set the ann pointers correctly */
	ann->first_layer = layers;
	ann->last_layer = layers + num_layers;
#ifdef CASCADE_DEBUG_FULL
	printf("add layer at pos %d\n", layer_pos);
#endif
	return layers + layer_pos;
}

void fann_set_shortcut_connections(Fann2 * ann)
{
	uint num_connections = 0;
	Fann2::Neuron ** neuron_pointers = ann->connections;
	Fann2::Neuron * neurons = ann->first_layer->first_neuron;
	for(Fann2::Layer * layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++) {
		for(Fann2::Neuron * neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++) {
			neuron_pointers += num_connections;
			num_connections = neuron_it->last_con - neuron_it->first_con;
			for(uint i = 0; i != num_connections; i++) {
				neuron_pointers[i] = neurons + i;
			}
		}
	}
}

void fann_add_candidate_neuron(Fann2 * ann, Fann2::Layer * layer)
{
	uint num_connections_in = (uint)(layer->first_neuron - ann->first_layer->first_neuron);
	uint num_connections_out = (uint)((ann->last_layer - 1)->last_neuron - (layer + 1)->first_neuron);
	uint num_connections_move = num_connections_out + num_connections_in;
	uint candidate_con, candidate_output_weight;
	int i;
	Fann2::Layer * layer_it;
	Fann2::Neuron * neuron_it, * neuron_place, * candidate;
	// 
	// We know that there is enough room for the new neuron
	// (the candidates are in the same arrays), so move
	// the last neurons to make room for this neuron.
	// 
	// first move the pointers to neurons in the layer structs 
	for(layer_it = ann->last_layer - 1; layer_it != layer; layer_it--) {
#ifdef CASCADE_DEBUG_FULL
		printf("move neuron pointers in layer %d, first(%d -> %d), last(%d -> %d)\n",
		    layer_it - ann->first_layer,
		    layer_it->first_neuron - ann->first_layer->first_neuron,
		    layer_it->first_neuron - ann->first_layer->first_neuron + 1,
		    layer_it->last_neuron - ann->first_layer->first_neuron,
		    layer_it->last_neuron - ann->first_layer->first_neuron + 1);
#endif
		layer_it->first_neuron++;
		layer_it->last_neuron++;
	}
	// also move the last neuron in the layer that needs the neuron added 
	layer->last_neuron++;
	// this is the place that should hold the new neuron 
	neuron_place = layer->last_neuron - 1;
#ifdef CASCADE_DEBUG_FULL
	printf("num_connections_in=%d, num_connections_out=%d\n", num_connections_in, num_connections_out);
#endif
	candidate = ann->first_layer->first_neuron + ann->cascade_best_candidate;
	// the output weights for the candidates are located after the input weights 
	candidate_output_weight = candidate->last_con;
	// move the actual output neurons and the indexes to the connection arrays 
	for(neuron_it = (ann->last_layer - 1)->last_neuron - 1; neuron_it != neuron_place; neuron_it--) {
#ifdef CASCADE_DEBUG_FULL
		printf("move neuron %d -> %d\n", neuron_it - ann->first_layer->first_neuron - 1, neuron_it - ann->first_layer->first_neuron);
#endif
		*neuron_it = *(neuron_it - 1);
		// move the weights 
#ifdef CASCADE_DEBUG_FULL
		printf("move weight[%d ... %d] -> weight[%d ... %d]\n", neuron_it->first_con,
		    neuron_it->last_con - 1, neuron_it->first_con + num_connections_move - 1,
		    neuron_it->last_con + num_connections_move - 2);
#endif
		for(i = neuron_it->last_con - 1; i >= (int)neuron_it->first_con; i--) {
#ifdef CASCADE_DEBUG_FULL
			printf("move weight[%d] = weight[%d]\n", i + num_connections_move - 1, i);
#endif
			ann->weights[i + num_connections_move - 1] = ann->weights[i];
		}
		/* move the indexes to weights */
		neuron_it->last_con += num_connections_move;
		num_connections_move--;
		neuron_it->first_con += num_connections_move;
		/* set the new weight to the newly allocated neuron */
		ann->weights[neuron_it->last_con - 1] = (ann->weights[candidate_output_weight]) * ann->cascade_weight_multiplier;
		candidate_output_weight++;
	}
	/* Now inititalize the actual neuron */
	neuron_place->value = 0;
	neuron_place->sum = 0;
	neuron_place->activation_function = candidate->activation_function;
	neuron_place->activation_steepness = candidate->activation_steepness;
	neuron_place->last_con = (neuron_place + 1)->first_con;
	neuron_place->first_con = neuron_place->last_con - num_connections_in;
#ifdef CASCADE_DEBUG_FULL
	printf("neuron[%d] = weights[%d ... %d] activation: %s, steepness: %f\n",
	    neuron_place - ann->first_layer->first_neuron, neuron_place->first_con,
	    neuron_place->last_con - 1, FANN_ACTIVATIONFUNC_NAMES[neuron_place->activation_function],
	    neuron_place->activation_steepness);       /* TODO remove */
#endif

	candidate_con = candidate->first_con;
	/* initialize the input weights at random */
#ifdef CASCADE_DEBUG_FULL
	printf("move cand weights[%d ... %d] -> [%d ... %d]\n", candidate_con,
	    candidate_con + num_connections_in - 1, neuron_place->first_con,
	    neuron_place->last_con - 1);
#endif

	for(i = 0; i < (int)num_connections_in; i++) {
		ann->weights[i + neuron_place->first_con] = ann->weights[i + candidate_con];
#ifdef CASCADE_DEBUG_FULL
		printf("move weights[%d] -> weights[%d] (%f)\n", i + candidate_con,
		    i + neuron_place->first_con, ann->weights[i + neuron_place->first_con]);
#endif
	}
	/* Change some of main variables */
	ann->total_neurons++;
	ann->total_connections += num_connections_in + num_connections_out;
	return;
}

void fann_install_candidate(Fann2 * ann)
{
	Fann2::Layer * layer = fann_add_layer(ann, ann->last_layer - 1);
	fann_add_candidate_neuron(ann, layer);
	return;
}

#endif // } FIXEDFANN 

FANN_EXTERNAL uint FANN_API fann_get_cascade_num_candidates(const Fann2 * ann)
{
	return ann->cascade_activation_functions_count * ann->cascade_activation_steepnesses_count * ann->cascade_num_candidate_groups;
}

FANN_GET_SET(float, cascade_output_change_fraction)
FANN_GET_SET(uint, cascade_output_stagnation_epochs)
FANN_GET_SET(float, cascade_candidate_change_fraction)
FANN_GET_SET(uint, cascade_candidate_stagnation_epochs)
FANN_GET_SET(uint, cascade_num_candidate_groups)
FANN_GET_SET(float, cascade_weight_multiplier)
FANN_GET_SET(float, cascade_candidate_limit)
FANN_GET_SET(uint, cascade_max_out_epochs)
FANN_GET_SET(uint, cascade_max_cand_epochs)
FANN_GET_SET(uint, cascade_min_out_epochs)
FANN_GET_SET(uint, cascade_min_cand_epochs)

FANN_GET(uint, cascade_activation_functions_count)
FANN_GET(Fann2::ActivationFunc *, cascade_activation_functions)

FANN_EXTERNAL void FANN_API fann_set_cascade_activation_functions(Fann2 * ann, const Fann2::ActivationFunc * cascade_activation_functions, uint cascade_activation_functions_count)
{
	if(ann->cascade_activation_functions_count != cascade_activation_functions_count) {
		ann->cascade_activation_functions_count = cascade_activation_functions_count;
		// reallocate mem 
		ann->cascade_activation_functions = static_cast<Fann2::ActivationFunc *>(SAlloc::R(ann->cascade_activation_functions,
			ann->cascade_activation_functions_count * sizeof(Fann2::ActivationFunc)));
		if(ann->cascade_activation_functions == NULL) {
			fann_error_2((struct fann_error *)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	memmove(ann->cascade_activation_functions, cascade_activation_functions, ann->cascade_activation_functions_count * sizeof(Fann2::ActivationFunc));
}

FANN_GET(uint, cascade_activation_steepnesses_count)
FANN_GET(float *, cascade_activation_steepnesses)

FANN_EXTERNAL void FANN_API fann_set_cascade_activation_steepnesses(Fann2 * ann, const float * cascade_activation_steepnesses, uint cascade_activation_steepnesses_count)
{
	if(ann->cascade_activation_steepnesses_count != cascade_activation_steepnesses_count) {
		ann->cascade_activation_steepnesses_count = cascade_activation_steepnesses_count;
		/* reallocate mem */
		ann->cascade_activation_steepnesses = static_cast<float *>(SAlloc::R(ann->cascade_activation_steepnesses, ann->cascade_activation_steepnesses_count * sizeof(float)));
		if(ann->cascade_activation_steepnesses == NULL) {
			fann_error_2((struct fann_error *)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	memmove(ann->cascade_activation_steepnesses, cascade_activation_steepnesses, ann->cascade_activation_steepnesses_count * sizeof(float));
}
//
// FANN_IO
//
//
// Create a network from a configuration file.
//
FANN_EXTERNAL Fann2 * FANN_API fann_create_from_file(const char * configuration_file)
{
	Fann2 * ann = 0;
	FILE * conf = fopen(configuration_file, "r");
	if(!conf)
		fann_error_2(NULL, FANN_E_CANT_OPEN_CONFIG_R, configuration_file);
	else {
		ann = fann_create_from_fd(conf, configuration_file);
		fclose(conf);
	}
	return ann;
}

int fann_is_equal(const Fann2 * p1, const Fann2 * p2)
{
	return 0;
}
// 
// INTERNAL FUNCTION
// Used to save the network to a file descriptor.
// 
static int fann_save_internal_fd_2(Fann2 * ann, FILE * conf, const char * configuration_file, uint save_as_fixed)
{
	Fann2::Layer * layer_it;
	int calculated_decimal_point = 0;
	Fann2::Neuron * neuron_it, * first_neuron;
	float * weights;
	Fann2::Neuron ** connected_neurons;
	uint i = 0;
#ifndef FIXEDFANN
	//
	// variabels for use when saving floats as fixed point variabels 
	//
	uint decimal_point = 0;
	uint fixed_multiplier = 0;
	float max_possible_value = 0;
	uint bits_used_for_max = 0;
	float current_max_value = 0;
#endif
#ifndef FIXEDFANN
	if(save_as_fixed) {
		fprintf(conf, FANN_FIX_VERSION "\n"); // save the version information 
	}
	else {
		fprintf(conf, FANN_FLO_VERSION "\n"); // save the version information 
	}
#else
	fprintf(conf, FANN_FIX_VERSION "\n"); // save the version information 
#endif
#ifndef FIXEDFANN
	if(save_as_fixed) {
		// calculate the maximal possible shift value 
		for(layer_it = ann->first_layer + 1; layer_it != ann->last_layer; layer_it++) {
			for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++) {
				// look at all connections to each neurons, and see how high a value we can get 
				current_max_value = 0;
				for(i = neuron_it->first_con; i != neuron_it->last_con; i++) {
					current_max_value += fabsf(ann->weights[i]);
				}
				if(current_max_value > max_possible_value) {
					max_possible_value = current_max_value;
				}
			}
		}
		for(bits_used_for_max = 0; max_possible_value >= 1.0f; bits_used_for_max++) {
			max_possible_value /= 2.0f;
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
#ifdef DEBUG
		printf("calculated_decimal_point=%d, decimal_point=%u, bits_used_for_max=%u\n", calculated_decimal_point, decimal_point, bits_used_for_max);
#endif
		fprintf(conf, "decimal_point=%u\n", decimal_point); // save the decimal_point on a seperate line 
	}
#else
	fprintf(conf, "decimal_point=%u\n", ann->decimal_point); // save the decimal_point on a seperate line 
#endif
	/* Save network parameters */
	fprintf(conf, "num_layers=%d\n", (int)(ann->last_layer - ann->first_layer));
	fprintf(conf, "learning_rate=%f\n", ann->learning_rate);
	fprintf(conf, "connection_rate=%f\n", ann->connection_rate);
	fprintf(conf, "network_type=%u\n", ann->network_type);
	fprintf(conf, "learning_momentum=%f\n", ann->learning_momentum);
	fprintf(conf, "training_algorithm=%u\n", ann->training_algorithm);
	fprintf(conf, "train_error_function=%u\n", ann->train_error_function);
	fprintf(conf, "train_stop_function=%u\n", ann->train_stop_function);
	fprintf(conf, "cascade_output_change_fraction=%f\n", ann->cascade_output_change_fraction);
	fprintf(conf, "quickprop_decay=%f\n", ann->quickprop_decay);
	fprintf(conf, "quickprop_mu=%f\n", ann->quickprop_mu);
	fprintf(conf, "rprop_increase_factor=%f\n", ann->rprop_increase_factor);
	fprintf(conf, "rprop_decrease_factor=%f\n", ann->rprop_decrease_factor);
	fprintf(conf, "rprop_delta_min=%f\n", ann->rprop_delta_min);
	fprintf(conf, "rprop_delta_max=%f\n", ann->rprop_delta_max);
	fprintf(conf, "rprop_delta_zero=%f\n", ann->rprop_delta_zero);
	fprintf(conf, "cascade_output_stagnation_epochs=%u\n", ann->cascade_output_stagnation_epochs);
	fprintf(conf, "cascade_candidate_change_fraction=%f\n", ann->cascade_candidate_change_fraction);
	fprintf(conf, "cascade_candidate_stagnation_epochs=%u\n", ann->cascade_candidate_stagnation_epochs);
	fprintf(conf, "cascade_max_out_epochs=%u\n", ann->cascade_max_out_epochs);
	fprintf(conf, "cascade_min_out_epochs=%u\n", ann->cascade_min_out_epochs);
	fprintf(conf, "cascade_max_cand_epochs=%u\n", ann->cascade_max_cand_epochs);
	fprintf(conf, "cascade_min_cand_epochs=%u\n", ann->cascade_min_cand_epochs);
	fprintf(conf, "cascade_num_candidate_groups=%u\n", ann->cascade_num_candidate_groups);
#ifndef FIXEDFANN
	if(save_as_fixed) {
		fprintf(conf, "bit_fail_limit=%u\n", ffloori((ann->bit_fail_limit * fixed_multiplier) + 0.5));
		fprintf(conf, "cascade_candidate_limit=%u\n", ffloori((ann->cascade_candidate_limit * fixed_multiplier) + 0.5));
		fprintf(conf, "cascade_weight_multiplier=%u\n", ffloori((ann->cascade_weight_multiplier * fixed_multiplier) + 0.5));
	}
	else
#endif
	{
		fprintf(conf, "bit_fail_limit=" FANNPRINTF "\n", ann->bit_fail_limit);
		fprintf(conf, "cascade_candidate_limit=" FANNPRINTF "\n", ann->cascade_candidate_limit);
		fprintf(conf, "cascade_weight_multiplier=" FANNPRINTF "\n", ann->cascade_weight_multiplier);
	}
	fprintf(conf, "cascade_activation_functions_count=%u\n", ann->cascade_activation_functions_count);
	fprintf(conf, "cascade_activation_functions=");
	for(i = 0; i < ann->cascade_activation_functions_count; i++)
		fprintf(conf, "%u ", ann->cascade_activation_functions[i]);
	fprintf(conf, "\n");
	fprintf(conf, "cascade_activation_steepnesses_count=%u\n", ann->cascade_activation_steepnesses_count);
	fprintf(conf, "cascade_activation_steepnesses=");
	for(i = 0; i < ann->cascade_activation_steepnesses_count; i++) {
#ifndef FIXEDFANN
		if(save_as_fixed)
			fprintf(conf, "%u ", ffloori((ann->cascade_activation_steepnesses[i] * fixed_multiplier) + 0.5));
		else
#endif
		fprintf(conf, FANNPRINTF " ", ann->cascade_activation_steepnesses[i]);
	}
	fprintf(conf, "\n");
	fprintf(conf, "layer_sizes=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		// the number of neurons in the layers (in the last layer, there is always one too many neurons, because of an unused bias) 
		fprintf(conf, "%d ", (int)(layer_it->last_neuron - layer_it->first_neuron));
	}
	fprintf(conf, "\n");

#ifndef FIXEDFANN
	/* 2.1 */
	#define SCALE_SAVE(what, where) \
	fprintf(conf, #what "_" #where "="); \
	for(i = 0; i < ann->num_ ## where ## put; i++) \
		fprintf(conf, "%f ", ann->what ## _ ## where[ i ]); \
	fprintf(conf, "\n");
	if(!save_as_fixed) {
		if(ann->scale_mean_in != NULL) {
			fprintf(conf, "scale_included=1\n");
			SCALE_SAVE(scale_mean,                 in)
			SCALE_SAVE(scale_deviation,    in)
			SCALE_SAVE(scale_new_min,              in)
			SCALE_SAVE(scale_factor,               in)

			SCALE_SAVE(scale_mean,                 out)
			SCALE_SAVE(scale_deviation,    out)
			SCALE_SAVE(scale_new_min,              out)
			SCALE_SAVE(scale_factor,               out)
		}
		else
			fprintf(conf, "scale_included=0\n");
	}
#undef SCALE_SAVE
#endif
	/* 2.0 */
	fprintf(conf, "neurons (num_inputs, activation_function, activation_steepness)=");
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		/* the neurons */
		for(neuron_it = layer_it->first_neuron; neuron_it != layer_it->last_neuron; neuron_it++) {
#ifndef FIXEDFANN
			if(save_as_fixed) {
				fprintf(conf, "(%u, %u, %u) ", neuron_it->last_con - neuron_it->first_con,
				    neuron_it->activation_function, ffloori((neuron_it->activation_steepness * fixed_multiplier) + 0.5));
			}
			else {
				fprintf(conf, "(%u, %u, " FANNPRINTF ") ", neuron_it->last_con - neuron_it->first_con,
				    neuron_it->activation_function, neuron_it->activation_steepness);
			}
#else
			fprintf(conf, "(%u, %u, " FANNPRINTF ") ", neuron_it->last_con - neuron_it->first_con,
			    neuron_it->activation_function, neuron_it->activation_steepness);
#endif
		}
	}
	fprintf(conf, "\n");

	connected_neurons = ann->connections;
	weights = ann->weights;
	first_neuron = ann->first_layer->first_neuron;

	/* Now save all the connections.
	 * We only need to save the source and the weight,
	 * since the destination is given by the order.
	 *
	 * The weight is not saved binary due to differences
	 * in binary definition of floating point numbers.
	 * Especially an iPAQ does not use the same binary
	 * representation as an i386 machine.
	 */
	fprintf(conf, "connections (connected_to_neuron, weight)=");
	for(i = 0; i < ann->total_connections; i++) {
#ifndef FIXEDFANN
		if(save_as_fixed) {
			// save the connection "(source weight) " 
			fprintf(conf, "(%d, %d) ", (int)(connected_neurons[i] - first_neuron), ffloori((weights[i] * fixed_multiplier) + 0.5));
		}
		else {
			// save the connection "(source weight) " 
			fprintf(conf, "(%d, " FANNPRINTF ") ", (int)(connected_neurons[i] - first_neuron), weights[i]);
		}
#else
		// save the connection "(source weight) " 
		fprintf(conf, "(%d, " FANNPRINTF ") ", (int)(connected_neurons[i] - first_neuron), weights[i]);
#endif
	}
	fprintf(conf, "\n");
	return calculated_decimal_point;
}
// 
// INTERNAL FUNCTION
// Used to save the network to a file.
// 
static int fann_save_internal_2(Fann2 * ann, const char * configuration_file, uint save_as_fixed)
{
	int    retval = -1;
	FILE * conf = fopen(configuration_file, "w+");
	if(!conf)
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_OPEN_CONFIG_W, configuration_file);
	else {
		retval = fann_save_internal_fd_2(ann, conf, configuration_file, save_as_fixed);
		fclose(conf);
	}
	return retval;
}
// 
// Save the network.
// 
FANN_EXTERNAL int FANN_API fann_save(Fann2 * ann, const char * configuration_file)
{
	return fann_save_internal_2(ann, configuration_file, 0);
}
// 
// Save the network as fixed point data.
// 
FANN_EXTERNAL int FANN_API fann_save_to_fixed(Fann2 * ann, const char * configuration_file)
{
	return fann_save_internal_2(ann, configuration_file, 1);
}

Fann2 * fann_create_from_fd_1_1(FILE * conf, const char * configuration_file);

#define fann_scanf(type, name, val) \
	{ \
		if(fscanf(conf, name "=" type "\n", val) != 1) { \
			fann_error_2(NULL, FANN_E_CANT_READ_CONFIG, name, configuration_file); \
			fann_destroy(ann); \
			return NULL; \
		} \
	}

#define fann_skip(name) \
	{ \
		if(fscanf(conf, name) != 0) { \
			fann_error_2(NULL, FANN_E_CANT_READ_CONFIG, name, configuration_file); \
			fann_destroy(ann); \
			return NULL; \
		} \
	}

/* INTERNAL FUNCTION
   Create a network from a configuration file descriptor.
 */
Fann2 * fann_create_from_fd(FILE * conf, const char * configuration_file)
{
	uint num_layers, layer_size, input_neuron, i, num_connections;
	uint tmpVal;
#ifdef FIXEDFANN
	uint decimal_point, multiplier;
#else
	uint scale_included;
#endif
	Fann2::Neuron * first_neuron, * neuron_it, * last_neuron, ** connected_neurons;
	float * weights;
	Fann2::Layer * layer_it;
	Fann2 * ann = NULL;
	char * read_version;
	read_version = static_cast<char *>(SAlloc::C(strlen(FANN_CONF_VERSION "\n"), 1));
	if(read_version == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	if(fread(read_version, 1, strlen(FANN_CONF_VERSION "\n"), conf) == 1) {
		fann_error_2(NULL, FANN_E_CANT_READ_CONFIG, "FANN_VERSION", configuration_file);
		return NULL;
	}
	/* compares the version information */
	if(strncmp(read_version, FANN_CONF_VERSION "\n", strlen(FANN_CONF_VERSION "\n")) != 0) {
#ifdef FIXEDFANN
		if(strncmp(read_version, "FANN_FIX_1.1\n", strlen("FANN_FIX_1.1\n")) == 0) {
#else
		if(strncmp(read_version, "FANN_FLO_1.1\n", strlen("FANN_FLO_1.1\n")) == 0) {
#endif
			SAlloc::F(read_version);
			return fann_create_from_fd_1_1(conf, configuration_file);
		}

#ifndef FIXEDFANN
		// Maintain compatibility with 2.0 version that doesnt have scale parameters. 
		if(strncmp(read_version, "FANN_FLO_2.0\n", strlen("FANN_FLO_2.0\n")) != 0 &&
		    strncmp(read_version, "FANN_FLO_2.1\n", strlen("FANN_FLO_2.1\n")) != 0)
#else
		if(strncmp(read_version, "FANN_FIX_2.0\n", strlen("FANN_FIX_2.0\n")) != 0 &&
		    strncmp(read_version, "FANN_FIX_2.1\n", strlen("FANN_FIX_2.1\n")) != 0)
#endif
		{
			SAlloc::F(read_version);
			fann_error_2(NULL, FANN_E_WRONG_CONFIG_VERSION, configuration_file);
			return NULL;
		}
	}
	SAlloc::F(read_version);
#ifdef FIXEDFANN
	fann_scanf("%u", "decimal_point", &decimal_point);
	multiplier = 1 << decimal_point;
#endif
	fann_scanf("%u", "num_layers", &num_layers);
	ann = fann_allocate_structure(num_layers);
	if(ann == NULL) {
		return NULL;
	}
	fann_scanf("%f", "learning_rate", &ann->learning_rate);
	fann_scanf("%f", "connection_rate", &ann->connection_rate);
	fann_scanf("%u", "network_type", &tmpVal);
	ann->network_type = (enum fann_nettype_enum)tmpVal;
	fann_scanf("%f", "learning_momentum", &ann->learning_momentum);
	fann_scanf("%u", "training_algorithm", &tmpVal);
	ann->training_algorithm = (enum fann_train_enum)tmpVal;
	fann_scanf("%u", "train_error_function", &tmpVal);
	ann->train_error_function = (enum fann_errorfunc_enum)tmpVal;
	fann_scanf("%u", "train_stop_function", &tmpVal);
	ann->train_stop_function = (enum fann_stopfunc_enum)tmpVal;
	fann_scanf("%f", "cascade_output_change_fraction", &ann->cascade_output_change_fraction);
	fann_scanf("%f", "quickprop_decay", &ann->quickprop_decay);
	fann_scanf("%f", "quickprop_mu", &ann->quickprop_mu);
	fann_scanf("%f", "rprop_increase_factor", &ann->rprop_increase_factor);
	fann_scanf("%f", "rprop_decrease_factor", &ann->rprop_decrease_factor);
	fann_scanf("%f", "rprop_delta_min", &ann->rprop_delta_min);
	fann_scanf("%f", "rprop_delta_max", &ann->rprop_delta_max);
	fann_scanf("%f", "rprop_delta_zero", &ann->rprop_delta_zero);
	fann_scanf("%u", "cascade_output_stagnation_epochs", &ann->cascade_output_stagnation_epochs);
	fann_scanf("%f", "cascade_candidate_change_fraction", &ann->cascade_candidate_change_fraction);
	fann_scanf("%u", "cascade_candidate_stagnation_epochs", &ann->cascade_candidate_stagnation_epochs);
	fann_scanf("%u", "cascade_max_out_epochs", &ann->cascade_max_out_epochs);
	fann_scanf("%u", "cascade_min_out_epochs", &ann->cascade_min_out_epochs);
	fann_scanf("%u", "cascade_max_cand_epochs", &ann->cascade_max_cand_epochs);
	fann_scanf("%u", "cascade_min_cand_epochs", &ann->cascade_min_cand_epochs);
	fann_scanf("%u", "cascade_num_candidate_groups", &ann->cascade_num_candidate_groups);

	fann_scanf(FANNSCANF, "bit_fail_limit", &ann->bit_fail_limit);
	fann_scanf(FANNSCANF, "cascade_candidate_limit", &ann->cascade_candidate_limit);
	fann_scanf(FANNSCANF, "cascade_weight_multiplier", &ann->cascade_weight_multiplier);

	fann_scanf("%u", "cascade_activation_functions_count", &ann->cascade_activation_functions_count);

	/* reallocate mem */
	ann->cascade_activation_functions = static_cast<Fann2::ActivationFunc *>(SAlloc::R(ann->cascade_activation_functions,
		ann->cascade_activation_functions_count * sizeof(Fann2::ActivationFunc)));
	if(ann->cascade_activation_functions == NULL) {
		fann_error_2((struct fann_error *)ann, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(ann);
		return NULL;
	}
	fann_skip("cascade_activation_functions=");
	for(i = 0; i < ann->cascade_activation_functions_count; i++) {
		if(fscanf(conf, "%u ", &tmpVal) != 1) {
			fann_error_2(NULL, FANN_E_CANT_READ_CONFIG, "cascade_activation_functions", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		ann->cascade_activation_functions[i] = (Fann2::ActivationFunc)tmpVal;
	}
	fann_scanf("%u", "cascade_activation_steepnesses_count", &ann->cascade_activation_steepnesses_count);
	/* reallocate mem */
	ann->cascade_activation_steepnesses = static_cast<float *>(SAlloc::R(ann->cascade_activation_steepnesses, ann->cascade_activation_steepnesses_count * sizeof(float)));
	if(ann->cascade_activation_steepnesses == NULL) {
		fann_error_2((struct fann_error *)ann, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(ann);
		return NULL;
	}
	fann_skip("cascade_activation_steepnesses=");
	for(i = 0; i < ann->cascade_activation_steepnesses_count; i++) {
		if(fscanf(conf, FANNSCANF " ", &ann->cascade_activation_steepnesses[i]) != 1) {
			fann_error_2(NULL, FANN_E_CANT_READ_CONFIG, "cascade_activation_steepnesses", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
	}
#ifdef FIXEDFANN
	ann->decimal_point = decimal_point;
	ann->multiplier = multiplier;
#endif
#ifdef FIXEDFANN
	fann_update_stepwise(ann);
#endif
#ifdef DEBUG
	printf("creating network with %d layers\n", num_layers);
	printf("input\n");
#endif
	fann_skip("layer_sizes=");
	/* determine how many neurons there should be in each layer */
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		if(fscanf(conf, "%u ", &layer_size) != 1 || layer_size == 0 || layer_size > INT_MAX - ann->total_neurons) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_READ_CONFIG, "layer_sizes", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		// we do not allocate room here, but we make sure that last_neuron - first_neuron is the number of neurons 
		layer_it->first_neuron = NULL;
		layer_it->last_neuron = layer_it->first_neuron + layer_size;
		ann->total_neurons += layer_size;
#ifdef DEBUG
		if(ann->network_type == FANN_NETTYPE_SHORTCUT && layer_it != ann->first_layer) {
			printf("  layer       : %d neurons, 0 bias\n", layer_size);
		}
		else {
			printf("  layer       : %d neurons, 1 bias\n", layer_size - 1);
		}
#endif
	}
	ann->num_input = (uint)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);
	ann->num_output = (uint)((ann->last_layer - 1)->last_neuron - (ann->last_layer - 1)->first_neuron);
	if(ann->network_type == FANN_NETTYPE_LAYER) {
		ann->num_output--; // one too many (bias) in the output layer 
	}
#ifndef FIXEDFANN
#define SCALE_LOAD(what, where)                                                                                       \
	fann_skip( #what "_" #where "=");                                                                      \
	for(i = 0; i < ann->num_ ## where ## put; i++) { \
		if(fscanf(conf, "%f ", (float *)&ann->what ## _ ## where[ i ]) != 1) { \
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_READ_CONFIG, #what "_" #where, configuration_file); \
			fann_destroy(ann);                                                                                              \
			return NULL;                                                                                                    \
		}                                                                                                                                       \
	}
	//
	if(fscanf(conf, "scale_included=%u\n", &scale_included) == 1 && scale_included == 1) {
		fann_allocate_scale(ann);
		SCALE_LOAD(scale_mean,                 in)
		SCALE_LOAD(scale_deviation,    in)
		SCALE_LOAD(scale_new_min,              in)
		SCALE_LOAD(scale_factor,               in)

		SCALE_LOAD(scale_mean,                 out)
		SCALE_LOAD(scale_deviation,    out)
		SCALE_LOAD(scale_new_min,              out)
		SCALE_LOAD(scale_factor,               out)
	}
#undef SCALE_LOAD
#endif
	/* allocate room for the actual neurons */
	fann_allocate_neurons(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
	last_neuron = (ann->last_layer - 1)->last_neuron;
	fann_skip("neurons (num_inputs, activation_function, activation_steepness)=");
	for(neuron_it = ann->first_layer->first_neuron; neuron_it != last_neuron; neuron_it++) {
		if(fscanf(conf, "(%u, %u, " FANNSCANF ") ", &num_connections, &tmpVal, &neuron_it->activation_steepness) != 3) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		neuron_it->activation_function = (Fann2::ActivationFunc)tmpVal;
		neuron_it->first_con = ann->total_connections;
		ann->total_connections += num_connections;
		neuron_it->last_con = ann->total_connections;
	}
	fann_allocate_connections(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
	connected_neurons = ann->connections;
	weights = ann->weights;
	first_neuron = ann->first_layer->first_neuron;
	fann_skip("connections (connected_to_neuron, weight)=");
	for(i = 0; i < ann->total_connections; i++) {
		if(fscanf(conf, "(%u, " FANNSCANF ") ", &input_neuron, &weights[i]) != 2) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_READ_CONNECTIONS, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		connected_neurons[i] = first_neuron + input_neuron;
	}
#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}
// 
// INTERNAL FUNCTION
// Create a network from a configuration file descriptor. (backward compatible read of version 1.1 files)
// 
Fann2 * fann_create_from_fd_1_1(FILE * conf, const char * configuration_file)
{
	uint num_layers, layer_size, input_neuron, i, network_type, num_connections;
	uint activation_function_hidden, activation_function_output;
#ifdef FIXEDFANN
	uint decimal_point, multiplier;
#endif
	float activation_steepness_hidden, activation_steepness_output;
	float learning_rate, connection_rate;
	Fann2::Neuron * first_neuron, * neuron_it, * last_neuron, ** connected_neurons;
	float * weights;
	Fann2::Layer * layer_it;
	Fann2 * ann;
#ifdef FIXEDFANN
	if(fscanf(conf, "%u\n", &decimal_point) != 1) {
		fann_error_2(NULL, FANN_E_CANT_READ_CONFIG, "decimal_point", configuration_file);
		return NULL;
	}
	multiplier = 1 << decimal_point;
#endif
	if(fscanf(conf, "%u %f %f %u %u %u " FANNSCANF " " FANNSCANF "\n", &num_layers, &learning_rate,
	    &connection_rate, &network_type, &activation_function_hidden,
	    &activation_function_output, &activation_steepness_hidden,
	    &activation_steepness_output) != 8) {
		fann_error_2(NULL, FANN_E_CANT_READ_CONFIG, "parameters", configuration_file);
		return NULL;
	}
	ann = fann_allocate_structure(num_layers);
	if(ann == NULL) {
		return NULL;
	}
	ann->connection_rate = connection_rate;
	ann->network_type = (enum fann_nettype_enum)network_type;
	ann->learning_rate = learning_rate;

#ifdef FIXEDFANN
	ann->decimal_point = decimal_point;
	ann->multiplier = multiplier;
#endif
#ifdef FIXEDFANN
	fann_update_stepwise(ann);
#endif
#ifdef DEBUG
	printf("creating network with learning rate %f\n", learning_rate);
	printf("input\n");
#endif
	// determine how many neurons there should be in each layer 
	for(layer_it = ann->first_layer; layer_it != ann->last_layer; layer_it++) {
		if(fscanf(conf, "%u ", &layer_size) != 1) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		// we do not allocate room here, but we make sure that last_neuron - first_neuron is the number of neurons 
		layer_it->first_neuron = NULL;
		layer_it->last_neuron = layer_it->first_neuron + layer_size;
		ann->total_neurons += layer_size;
#ifdef DEBUG
		if(ann->network_type == FANN_NETTYPE_SHORTCUT && layer_it != ann->first_layer) {
			printf("  layer       : %d neurons, 0 bias\n", layer_size);
		}
		else {
			printf("  layer       : %d neurons, 1 bias\n", layer_size - 1);
		}
#endif
	}
	ann->num_input = (uint)(ann->first_layer->last_neuron - ann->first_layer->first_neuron - 1);
	ann->num_output = (uint)((ann->last_layer - 1)->last_neuron - (ann->last_layer - 1)->first_neuron);
	if(ann->network_type == FANN_NETTYPE_LAYER) {
		// one too many (bias) in the output layer 
		ann->num_output--;
	}
	/* allocate room for the actual neurons */
	fann_allocate_neurons(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
	last_neuron = (ann->last_layer - 1)->last_neuron;
	for(neuron_it = ann->first_layer->first_neuron; neuron_it != last_neuron; neuron_it++) {
		if(fscanf(conf, "%u ", &num_connections) != 1) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		neuron_it->first_con = ann->total_connections;
		ann->total_connections += num_connections;
		neuron_it->last_con = ann->total_connections;
	}
	fann_allocate_connections(ann);
	if(ann->errno_f == FANN_E_CANT_ALLOCATE_MEM) {
		fann_destroy(ann);
		return NULL;
	}
	connected_neurons = ann->connections;
	weights = ann->weights;
	first_neuron = ann->first_layer->first_neuron;
	for(i = 0; i < ann->total_connections; i++) {
		if(fscanf(conf, "(%u " FANNSCANF ") ", &input_neuron, &weights[i]) != 2) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_READ_CONNECTIONS, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		connected_neurons[i] = first_neuron + input_neuron;
	}
	fann_set_activation_steepness_hidden(ann, activation_steepness_hidden);
	fann_set_activation_steepness_output(ann, activation_steepness_output);
	fann_set_activation_function_hidden(ann, (Fann2::ActivationFunc)activation_function_hidden);
	fann_set_activation_function_output(ann, (Fann2::ActivationFunc)activation_function_output);
#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}
//
// FANN_TRAIN
//
#ifndef FIXEDFANN // {
// 
// INTERNAL FUNCTION
// Calculates the activation of a value, given an activation function and a steepness
// 
float fann_activation(Fann2 * ann, uint activation_function, float steepness, float value)
{
	value = fann_mult(steepness, value);
	fann_activation_switch(activation_function, value, value);
	return value;
}
// 
// INTERNAL FUNCTION
// Update weights for incremental training
// 
void fann_update_weights(Fann2 * ann)
{
	Fann2::Neuron * neuron_it, * last_neuron, * prev_neurons;
	float tmp_error, delta_w, * weights;
	Fann2::Layer * layer_it;
	uint i;
	uint num_connections;
	// store some variabels local for fast access 
	const float learning_rate = ann->learning_rate;
	const float learning_momentum = ann->learning_momentum;
	Fann2::Neuron * first_neuron = ann->first_layer->first_neuron;
	Fann2::Layer * first_layer = ann->first_layer;
	const Fann2::Layer * last_layer = ann->last_layer;
	float * error_begin = ann->train_errors;
	float * deltas_begin, * weights_deltas;
	// if no room allocated for the deltas, allocate it now 
	if(ann->prev_weights_deltas == NULL) {
		ann->prev_weights_deltas = static_cast<float *>(SAlloc::C(ann->total_connections_allocated, sizeof(float)));
		if(ann->prev_weights_deltas == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
#ifdef DEBUGTRAIN
	printf("\nupdate weights\n");
#endif
	deltas_begin = ann->prev_weights_deltas;
	prev_neurons = first_neuron;
	for(layer_it = (first_layer + 1); layer_it != last_layer; layer_it++) {
#ifdef DEBUGTRAIN
		printf("layer[%d]\n", layer_it - first_layer);
#endif
		last_neuron = layer_it->last_neuron;
		if(ann->connection_rate >= 1) {
			if(ann->network_type == FANN_NETTYPE_LAYER) {
				prev_neurons = (layer_it - 1)->first_neuron;
			}
			for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - first_neuron] * learning_rate;
				num_connections = neuron_it->last_con - neuron_it->first_con;
				weights = ann->weights + neuron_it->first_con;
				weights_deltas = deltas_begin + neuron_it->first_con;
				for(i = 0; i != num_connections; i++) {
					delta_w = tmp_error * prev_neurons[i].value + learning_momentum * weights_deltas[i];
					weights[i] += delta_w;
					weights_deltas[i] = delta_w;
				}
			}
		}
		else {
			for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - first_neuron] * learning_rate;
				num_connections = neuron_it->last_con - neuron_it->first_con;
				weights = ann->weights + neuron_it->first_con;
				weights_deltas = deltas_begin + neuron_it->first_con;
				for(i = 0; i != num_connections; i++) {
					delta_w = tmp_error * prev_neurons[i].value + learning_momentum * weights_deltas[i];
					weights[i] += delta_w;
					weights_deltas[i] = delta_w;
				}
			}
		}
	}
}
// 
// Trains the network with the backpropagation algorithm.
// 
FANN_EXTERNAL void FANN_API fann_train(Fann2 * ann, float * input, float * desired_output, float * pRunOutput)
{
	const float * p_out = fann_run(ann, input);
	// @v10.2.7 {
	assert(p_out);
	if(pRunOutput) {
		memcpy(pRunOutput, p_out, sizeof(float) * ann->num_output);
	}
	// } @v10.2.7 
	fann_compute_MSE(ann, desired_output);
	fann_backpropagate_MSE(ann);
	fann_update_weights(ann);
}

#endif
// 
// INTERNAL FUNCTION
// Helper function to update the MSE value and return a diff which takes symmetric functions into account
// 
static FORCEINLINE float fann_update_MSE(Fann2 * ann, Fann2::Neuron * neuron, float neuron_diff)
{
	switch(neuron->activation_function) {
		case Fann2::FANN_LINEAR_PIECE_SYMMETRIC:
		case Fann2::FANN_THRESHOLD_SYMMETRIC:
		case Fann2::FANN_SIGMOID_SYMMETRIC:
		case Fann2::FANN_SIGMOID_SYMMETRIC_STEPWISE:
		case Fann2::FANN_ELLIOT_SYMMETRIC:
		case Fann2::FANN_GAUSSIAN_SYMMETRIC:
		case Fann2::FANN_SIN_SYMMETRIC:
		case Fann2::FANN_COS_SYMMETRIC:
		    neuron_diff /= (float)2.0;
		    break;
		case Fann2::FANN_THRESHOLD:
		case Fann2::FANN_LINEAR:
		case Fann2::FANN_SIGMOID:
		case Fann2::FANN_SIGMOID_STEPWISE:
		case Fann2::FANN_GAUSSIAN:
		case Fann2::FANN_GAUSSIAN_STEPWISE:
		case Fann2::FANN_ELLIOT:
		case Fann2::FANN_LINEAR_PIECE:
		case Fann2::FANN_SIN:
		case Fann2::FANN_COS:
		    break;
	}
#ifdef FIXEDFANN
	float neuron_diff2 = (neuron_diff / (float)ann->multiplier) * (neuron_diff / (float)ann->multiplier);
#else
	float neuron_diff2 = (float)(neuron_diff * neuron_diff);
#endif
	ann->MSE_value += neuron_diff2;
	//printf("neuron_diff %f = (%f - %f)[/2], neuron_diff2=%f, sum=%f, MSE_value=%f, num_MSE=%d\n", neuron_diff, *desired_output, neuron_value, neuron_diff2, last_layer_begin->sum, ann->MSE_value, ann->num_MSE);
	if(fabsf(neuron_diff) >= ann->bit_fail_limit) {
		ann->num_bit_fail++;
	}
	return neuron_diff;
}
// 
// Tests the network.
// 
FANN_EXTERNAL float * FANN_API fann_test(Fann2 * ann, float * input, float * desired_output)
{
	float neuron_value;
	float * output_begin = fann_run(ann, input);
	const float * output_end = output_begin + ann->num_output;
	float neuron_diff;
	Fann2::Neuron * output_neuron = (ann->last_layer - 1)->first_neuron;
	// calculate the error 
	for(float * output_it = output_begin; output_it != output_end; output_it++) {
		neuron_value = *output_it;
		neuron_diff = (*desired_output - neuron_value);
		neuron_diff = fann_update_MSE(ann, output_neuron, neuron_diff);
		desired_output++;
		output_neuron++;
		ann->num_MSE++;
	}
	return output_begin;
}
// 
// get the mean square error.
// 
FANN_EXTERNAL float FANN_API fann_get_MSE(const Fann2 * ann)
{
	return ann->num_MSE ? (ann->MSE_value / (float)ann->num_MSE) : 0.0f;
}

FANN_EXTERNAL uint FANN_API fann_get_bit_fail(const Fann2 * ann)
{
	return ann->num_bit_fail;
}
// 
// reset the mean square error.
// 
FANN_EXTERNAL void FANN_API fann_reset_MSE(Fann2 * ann)
{
	// printf("resetMSE %d %f\n", ann->num_MSE, ann->MSE_value);
	ann->num_MSE = 0;
	ann->MSE_value = 0;
	ann->num_bit_fail = 0;
}

#ifndef FIXEDFANN
// 
// INTERNAL FUNCTION
// compute the error at the network output
//   (usually, after forward propagation of a certain input vector, fann_run)
//   the error is a sum of squares for all the output units
//   also increments a counter because MSE is an average of such errors
// 
// After this train_errors in the output layer will be set to: neuron_value_derived * (desired_output - neuron_value)
// 
void fann_compute_MSE(Fann2 * ann, float * desired_output)
{
	float neuron_value, neuron_diff, * error_it = 0, * error_begin = 0;
	Fann2::Neuron * last_layer_begin = (ann->last_layer - 1)->first_neuron;
	const Fann2::Neuron * last_layer_end = last_layer_begin + ann->num_output;
	const Fann2::Neuron * first_neuron = ann->first_layer->first_neuron;
	// if no room allocated for the error variabels, allocate it now 
	if(ann->train_errors == NULL) {
		ann->train_errors = static_cast<float *>(SAlloc::C(ann->total_neurons, sizeof(float)));
		if(ann->train_errors == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	else
		memzero(ann->train_errors, (ann->total_neurons) * sizeof(float)); // clear the error variabels 
	error_begin = ann->train_errors;
#ifdef DEBUGTRAIN
	printf("\ncalculate errors\n");
#endif
	// calculate the error and place it in the output layer 
	error_it = error_begin + (last_layer_begin - first_neuron);
	for(; last_layer_begin != last_layer_end; last_layer_begin++) {
		neuron_value = last_layer_begin->value;
		neuron_diff = *desired_output - neuron_value;
		neuron_diff = fann_update_MSE(ann, last_layer_begin, neuron_diff);
#ifdef FLOATFANN
		if(ann->train_error_function) { // TODO make switch when more functions 
			if(neuron_diff < -0.9999999f)
				neuron_diff = -17.0f;
			else if(neuron_diff > 0.9999999f)
				neuron_diff = 17.0f;
			else
				neuron_diff = logf((1.0f + neuron_diff) / (1.0f - neuron_diff));
		}
#else
		if(ann->train_error_function) { // TODO make switch when more functions 
			if(neuron_diff < -0.9999999)
				neuron_diff = -17.0;
			else if(neuron_diff > 0.9999999)
				neuron_diff = 17.0;
			else
				neuron_diff = (float)log((1.0 + neuron_diff) / (1.0 - neuron_diff));
		}
#endif
		*error_it = fann_activation_derived(last_layer_begin->activation_function, last_layer_begin->activation_steepness, neuron_value, last_layer_begin->sum) * neuron_diff;
		desired_output++;
		error_it++;
		ann->num_MSE++;
	}
}
// 
// INTERNAL FUNCTION
// Propagate the error backwards from the output layer.
// After this the train_errors in the hidden layers will be:
// neuron_value_derived * sum(outgoing_weights * connected_neuron)
// 
void fann_backpropagate_MSE(Fann2 * ann)
{
	float * error_begin = ann->train_errors;
	float * error_prev_layer;
	const Fann2::Neuron * first_neuron = ann->first_layer->first_neuron;
	const Fann2::Layer * second_layer = ann->first_layer + 1;
	Fann2::Layer * last_layer = ann->last_layer;
	// go through all the layers, from last to first. And propagate the error backwards 
	for(Fann2::Layer * layer_it = last_layer - 1; layer_it > second_layer; --layer_it) {
		Fann2::Neuron * last_neuron = layer_it->last_neuron;
		// for each connection in this layer, propagate the error backwards 
		if(ann->connection_rate >= 1.0f) {
			error_prev_layer = (ann->network_type == FANN_NETTYPE_LAYER) ? (error_begin + ((layer_it-1)->first_neuron - first_neuron)) :  error_begin;
			for(const Fann2::Neuron * neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
				const float tmp_error = error_begin[neuron_it - first_neuron];
				float * weights = ann->weights + neuron_it->first_con;
				for(uint i = neuron_it->last_con - neuron_it->first_con; i--;) {
					//printf("i = %d\n", i);
					//printf("error_prev_layer[%d] = %f\n", i, error_prev_layer[i]);
					//printf("weights[%d] = %f\n", i, weights[i]);
					error_prev_layer[i] += tmp_error * weights[i];
				}
			}
		}
		else {
			for(const Fann2::Neuron * neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
				const float tmp_error = error_begin[neuron_it - first_neuron];
				float * weights = ann->weights + neuron_it->first_con;
				Fann2::Neuron ** connections = ann->connections + neuron_it->first_con;
				for(uint i = neuron_it->last_con - neuron_it->first_con; i--;) {
					error_begin[connections[i] - first_neuron] += tmp_error * weights[i];
				}
			}
		}
		{
			// then calculate the actual errors in the previous layer 
			error_prev_layer = error_begin + ((layer_it - 1)->first_neuron - first_neuron);
			last_neuron = (layer_it - 1)->last_neuron;
			for(const Fann2::Neuron * neuron_it = (layer_it - 1)->first_neuron; neuron_it != last_neuron; neuron_it++) {
				*error_prev_layer *= fann_activation_derived(neuron_it->activation_function, neuron_it->activation_steepness, neuron_it->value, neuron_it->sum);
				error_prev_layer++;
			}
		}
	}
}
// 
// INTERNAL FUNCTION
// Update slopes for batch training
// layer_begin = ann->first_layer+1 and layer_end = ann->last_layer-1 will update all slopes.
// 
void fann_update_slopes_batch(Fann2 * ann, Fann2::Layer * layer_begin, Fann2::Layer * layer_end)
{
	Fann2::Neuron * neuron_it, * last_neuron, * prev_neurons, ** connections;
	float tmp_error;
	uint i, num_connections;
	/* store some variabels local for fast access */
	Fann2::Neuron * first_neuron = ann->first_layer->first_neuron;
	float * error_begin = ann->train_errors;
	float * slope_begin, * neuron_slope;
	/* if no room allocated for the slope variabels, allocate it now */
	if(ann->train_slopes == NULL) {
		ann->train_slopes = static_cast<float *>(SAlloc::C(ann->total_connections_allocated, sizeof(float)));
		if(ann->train_slopes == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	if(layer_begin == NULL) {
		layer_begin = ann->first_layer + 1;
	}
	if(layer_end == NULL) {
		layer_end = ann->last_layer - 1;
	}
	slope_begin = ann->train_slopes;
#ifdef DEBUGTRAIN
	printf("\nupdate slopes\n");
#endif
	prev_neurons = first_neuron;
	for(; layer_begin <= layer_end; layer_begin++) {
#ifdef DEBUGTRAIN
		printf("layer[%d]\n", layer_begin - ann->first_layer);
#endif
		last_neuron = layer_begin->last_neuron;
		if(ann->connection_rate >= 1) {
			if(ann->network_type == FANN_NETTYPE_LAYER) {
				prev_neurons = (layer_begin - 1)->first_neuron;
			}
			for(neuron_it = layer_begin->first_neuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - first_neuron];
				neuron_slope = slope_begin + neuron_it->first_con;
				num_connections = neuron_it->last_con - neuron_it->first_con;
				for(i = 0; i != num_connections; i++) {
					neuron_slope[i] += tmp_error * prev_neurons[i].value;
				}
			}
		}
		else {
			for(neuron_it = layer_begin->first_neuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - first_neuron];
				neuron_slope = slope_begin + neuron_it->first_con;
				num_connections = neuron_it->last_con - neuron_it->first_con;
				connections = ann->connections + neuron_it->first_con;
				for(i = 0; i != num_connections; i++) {
					neuron_slope[i] += tmp_error * connections[i]->value;
				}
			}
		}
	}
}
// 
// INTERNAL FUNCTION
// Clears arrays used for training before a new training session.
// Also creates the arrays that do not exist yet.
// 
void fann_clear_train_arrays(Fann2 * ann)
{
	// if no room allocated for the slope variabels, allocate it now (calloc clears mem) 
	if(ann->train_slopes == NULL) {
		ann->train_slopes = static_cast<float *>(SAlloc::C(ann->total_connections_allocated, sizeof(float)));
		if(ann->train_slopes == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	else {
		memzero(ann->train_slopes, (ann->total_connections_allocated) * sizeof(float));
	}
	// if no room allocated for the variabels, allocate it now 
	if(ann->prev_steps == NULL) {
		ann->prev_steps = static_cast<float *>(SAlloc::M(ann->total_connections_allocated * sizeof(float)));
		if(ann->prev_steps == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	if(ann->training_algorithm == FANN_TRAIN_RPROP) {
		const float delta_zero = ann->rprop_delta_zero;
		for(uint i = 0; i < ann->total_connections_allocated; i++)
			ann->prev_steps[i] = delta_zero;
	}
	else
		memzero(ann->prev_steps, (ann->total_connections_allocated) * sizeof(float));
	// if no room allocated for the variabels, allocate it now 
	if(ann->prev_train_slopes == NULL) {
		ann->prev_train_slopes = static_cast<float *>(SAlloc::C(ann->total_connections_allocated, sizeof(float)));
		if(ann->prev_train_slopes == NULL) {
			fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	else
		memzero(ann->prev_train_slopes, (ann->total_connections_allocated) * sizeof(float));
}
// 
// INTERNAL FUNCTION
// Update weights for batch training
// 
/* (inlined) static void fann_update_weights_batch(Fann2 * ann, uint num_data, uint first_weight, uint past_end)
{
	float * train_slopes = ann->train_slopes;
	float * weights = ann->weights;
	const float epsilon = ann->learning_rate / num_data;
	for(uint i = first_weight; i != past_end; i++) {
		weights[i] += train_slopes[i] * epsilon;
		train_slopes[i] = 0.0;
	}
}*/
// 
// INTERNAL FUNCTION
// The quickprop training algorithm
// 
void fann_update_weights_quickprop(Fann2 * ann, uint num_data, uint first_weight, uint past_end)
{
	float * train_slopes = ann->train_slopes;
	float * weights = ann->weights;
	float * prev_steps = ann->prev_steps;
	float * prev_train_slopes = ann->prev_train_slopes;
	float w, prev_step, slope, prev_slope, next_step;
	float epsilon = ann->learning_rate / num_data;
	float decay = ann->quickprop_decay;     /*-0.0001;*/
	float mu = ann->quickprop_mu;   /*1.75; */
	float shrink_factor = (mu / (1.0f + mu));
	uint i = first_weight;
	for(; i != past_end; i++) {
		w = weights[i];
		prev_step = prev_steps[i];
		slope = train_slopes[i] + decay * w;
		prev_slope = prev_train_slopes[i];
		next_step = 0.0;
		/* The step must always be in direction opposite to the slope. */
		if(prev_step > 0.001) {
			/* If last step was positive...  */
			if(slope > 0.0) /*  Add in linear term if current slope is still positive. */
				next_step += epsilon * slope;
			/*If current slope is close to or larger than prev slope...  */
			if(slope > (shrink_factor * prev_slope))
				next_step += mu * prev_step;    /* Take maximum size negative step. */
			else
				next_step += prev_step * slope / (prev_slope - slope); // Else, use quadratic estimate.
		}
		else if(prev_step < -0.001) {
			/* If last step was negative...  */
			if(slope < 0.0) /*  Add in linear term if current slope is still negative. */
				next_step += epsilon * slope;
			/* If current slope is close to or more neg than prev slope... */
			if(slope < (shrink_factor * prev_slope))
				next_step += mu * prev_step;    /* Take maximum size negative step. */
			else
				next_step += prev_step * slope / (prev_slope - slope); // Else, use quadratic estimate.
		}
		else /* Last step was zero, so use only linear term. */
			next_step += epsilon * slope;

		/*
		   if(next_step > 1000 || next_step < -1000) {
		        printf("quickprop[%d] weight=%f, slope=%f, prev_slope=%f, next_step=%f, prev_step=%f\n", i, weights[i], slope, prev_slope, next_step, prev_step);
		           if(next_step > 1000)
		           next_step = 1000;
		           else
		           next_step = -1000;
		   }
		 */
		/* update global data arrays */
		prev_steps[i] = next_step;
		w += next_step;
		if(w > 1500)
			weights[i] = 1500;
		else if(w < -1500)
			weights[i] = -1500;
		else
			weights[i] = w;
		/*weights[i] = w;*/
		prev_train_slopes[i] = slope;
		train_slopes[i] = 0.0;
	}
}
// 
// INTERNAL FUNCTION
// The iRprop- algorithm
// 
void fann_update_weights_irpropm(Fann2 * ann, uint first_weight, uint past_end)
{
	float * train_slopes = ann->train_slopes;
	float * weights = ann->weights;
	float * prev_steps = ann->prev_steps;
	float * prev_train_slopes = ann->prev_train_slopes;
	float next_step;
	float increase_factor = ann->rprop_increase_factor;     /*1.2; */
	float decrease_factor = ann->rprop_decrease_factor;     /*0.5; */
	float delta_min = ann->rprop_delta_min; /*0.0; */
	float delta_max = ann->rprop_delta_max; /*50.0; */
	for(uint i = first_weight; i != past_end; i++) {
		const float prev_step = MAX(prev_steps[i], (float)0.0001); // prev_step may not be zero because then the training will stop 
		float slope = train_slopes[i];
		const float prev_slope = prev_train_slopes[i];
		const float same_sign = prev_slope * slope;
		if(same_sign >= 0.0)
			next_step = MIN(prev_step * increase_factor, delta_max);
		else {
			next_step = MAX(prev_step * decrease_factor, delta_min);
			slope = 0;
		}
		if(slope < 0) {
			weights[i] -= next_step;
			if(weights[i] < -1500)
				weights[i] = -1500;
		}
		else {
			weights[i] += next_step;
			if(weights[i] > 1500)
				weights[i] = 1500;
		}
		// if(i == 2) { printf("weight=%f, slope=%f, next_step=%f, prev_step=%f\n", weights[i], slope, next_step, prev_step); }
		//
		// update global data arrays 
		//
		prev_steps[i] = next_step;
		prev_train_slopes[i] = slope;
		train_slopes[i] = 0.0;
	}
}
// 
// INTERNAL FUNCTION
// The SARprop- algorithm
// 
void fann_update_weights_sarprop(Fann2 * ann, uint epoch, uint first_weight, uint past_end)
{
	float * train_slopes = ann->train_slopes;
	float * weights = ann->weights;
	float * prev_steps = ann->prev_steps;
	float * prev_train_slopes = ann->prev_train_slopes;
	float prev_step, slope, prev_slope, next_step = 0, same_sign;
	/* These should be set from variables */
	float increase_factor = ann->rprop_increase_factor;     /*1.2; */
	float decrease_factor = ann->rprop_decrease_factor;     /*0.5; */
	/* TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993) */
	float delta_min = 0.000001f;
	float delta_max = ann->rprop_delta_max; /*50.0; */
	float weight_decay_shift = ann->sarprop_weight_decay_shift; /* ld 0.01 = -6.644 */
	float step_error_threshold_factor = ann->sarprop_step_error_threshold_factor; /* 0.1 */
	float step_error_shift = ann->sarprop_step_error_shift; /* ld 3 = 1.585 */
	float T = ann->sarprop_temperature;
	float MSE = fann_get_MSE(ann);
	float RMSE = sqrtf(MSE);
	uint i = first_weight;
	/* for all weights; TODO: are biases included? */
	for(; i != past_end; i++) {
		/* TODO: confirm whether 1x10^-6 == delta_min is really better */
		prev_step = MAX(prev_steps[i], (float)0.000001); // prev_step may not be zero because then the training will stop 
		// calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)
		slope = -train_slopes[i] - weights[i] * (float)fann_exp2(-T * epoch + weight_decay_shift);
		/* TODO: is prev_train_slopes[i] 0.0 in the beginning? */
		prev_slope = prev_train_slopes[i];
		same_sign = prev_slope * slope;
		if(same_sign > 0.0) {
			next_step = MIN(prev_step * increase_factor, delta_max);
			// TODO: are the signs inverted? see differences between SARPROP paper and iRprop 
			if(slope < 0.0)
				weights[i] += next_step;
			else
				weights[i] -= next_step;
		}
		else if(same_sign < 0.0) {
			if(prev_step < step_error_threshold_factor * MSE)
				next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (float)fann_exp2(-T * epoch + step_error_shift);
			else
				next_step = MAX(prev_step * decrease_factor, delta_min);
			slope = 0.0;
		}
		else {
			if(slope < 0.0)
				weights[i] += prev_step;
			else
				weights[i] -= prev_step;
		}
		// if(i == 2) { printf("weight=%f, slope=%f, next_step=%f, prev_step=%f\n", weights[i], slope, next_step, prev_step); } 
		//
		// update global data arrays 
		//
		prev_steps[i] = next_step;
		prev_train_slopes[i] = slope;
		train_slopes[i] = 0.0;
	}
}

#endif

FANN_GET_SET(enum fann_train_enum, training_algorithm)
FANN_GET_SET(float, learning_rate)

FANN_EXTERNAL void FANN_API fann_set_activation_function_hidden(Fann2 * ann, Fann2::ActivationFunc activation_function)
{
	Fann2::Layer * last_layer = ann->last_layer - 1;    /* -1 to not update the output layer */
	for(Fann2::Layer * layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++) {
		Fann2::Neuron * last_neuron = layer_it->last_neuron;
		for(Fann2::Neuron * neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_function = activation_function;
		}
	}
}

FANN_EXTERNAL Fann2::Layer* FANN_API fann_get_layer(Fann2 * ann, int layer)
{
	if(layer <= 0 || layer >= (ann->last_layer - ann->first_layer)) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_INDEX_OUT_OF_BOUND, layer);
		return NULL;
	}
	else
		return (ann->first_layer + layer);
}

FANN_EXTERNAL Fann2::Neuron* FANN_API fann_get_neuron_layer(Fann2 * ann, Fann2::Layer* layer, int neuron)
{
	if(neuron >= (layer->last_neuron - layer->first_neuron)) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_INDEX_OUT_OF_BOUND, neuron);
		return NULL;
	}
	else
		return (layer->first_neuron + neuron);
}

FANN_EXTERNAL Fann2::Neuron * FANN_API fann_get_neuron(Fann2 * ann, uint layer, int neuron)
{
	Fann2::Layer * layer_it = fann_get_layer(ann, layer);
	return layer_it ? fann_get_neuron_layer(ann, layer_it, neuron) : 0;
}

FANN_EXTERNAL Fann2::ActivationFunc FANN_API fann_get_activation_function(Fann2 * ann, int layer, int neuron) 
{
	Fann2::Neuron * neuron_it = fann_get_neuron(ann, layer, neuron);
	return neuron_it ? neuron_it->activation_function : (Fann2::ActivationFunc)-1 /*layer or neuron out of bounds*/;
}

FANN_EXTERNAL void FANN_API fann_set_activation_function(Fann2 * ann, Fann2::ActivationFunc activation_function, int layer, int neuron)
{
	Fann2::Neuron * neuron_it = fann_get_neuron(ann, layer, neuron);
	if(neuron_it)
		neuron_it->activation_function = activation_function;
}

FANN_EXTERNAL void FANN_API fann_set_activation_function_layer(Fann2 * ann, Fann2::ActivationFunc activation_function, int layer)
{
	Fann2::Neuron * last_neuron, * neuron_it;
	Fann2::Layer * layer_it = fann_get_layer(ann, layer);
	if(layer_it) {
		last_neuron = layer_it->last_neuron;
		for(neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_function = activation_function;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_function_output(Fann2 * ann, Fann2::ActivationFunc activation_function)
{
	Fann2::Layer * last_layer = ann->last_layer - 1;
	Fann2::Neuron * last_neuron = last_layer->last_neuron;
	for(Fann2::Neuron * neuron_it = last_layer->first_neuron; neuron_it != last_neuron; neuron_it++) {
		neuron_it->activation_function = activation_function;
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness_hidden(Fann2 * ann, float steepness)
{
	Fann2::Layer * last_layer = ann->last_layer - 1; // -1 to not update the output layer 
	for(Fann2::Layer * layer_it = ann->first_layer + 1; layer_it != last_layer; layer_it++) {
		Fann2::Neuron * last_neuron = layer_it->last_neuron;
		for(Fann2::Neuron * neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_steepness = steepness;
		}
	}
}

FANN_EXTERNAL float FANN_API fann_get_activation_steepness(Fann2 * ann, int layer, int neuron)
{
	Fann2::Neuron * neuron_it = fann_get_neuron(ann, layer, neuron);
	return neuron_it ? neuron_it->activation_steepness : -1/* layer or neuron out of bounds */;
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness(Fann2 * ann, float steepness, int layer, int neuron)
{
	Fann2::Neuron * neuron_it = fann_get_neuron(ann, layer, neuron);
	if(neuron_it)
		neuron_it->activation_steepness = steepness;
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness_layer(Fann2 * ann, float steepness, int layer)
{
	Fann2::Layer * layer_it = fann_get_layer(ann, layer);
	if(layer_it) {
		Fann2::Neuron * last_neuron = layer_it->last_neuron;
		for(Fann2::Neuron * neuron_it = layer_it->first_neuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_steepness = steepness;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness_output(Fann2 * ann, float steepness)
{
	Fann2::Layer * last_layer = ann->last_layer - 1;
	Fann2::Neuron * last_neuron = last_layer->last_neuron;
	for(Fann2::Neuron * neuron_it = last_layer->first_neuron; neuron_it != last_neuron; neuron_it++) {
		neuron_it->activation_steepness = steepness;
	}
}

FANN_GET_SET(enum fann_errorfunc_enum, train_error_function)
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
FANN_GET_SET(enum fann_stopfunc_enum, train_stop_function)
FANN_GET_SET(float, bit_fail_limit)
FANN_GET_SET(float, learning_momentum)
//
// FANN_TRAIN_DATA
//
// 
// Reads training data from a file.
// 
FANN_EXTERNAL struct fann_train_data * FANN_API fann_read_train_from_file(const char * configuration_file)
{
	struct fann_train_data * data = 0;
	FILE * file = fopen(configuration_file, "r");
	if(!file)
		fann_error_2(NULL, FANN_E_CANT_OPEN_CONFIG_R, configuration_file);
	else {
		data = fann_read_train_from_fd(file, configuration_file);
		fclose(file);
	}
	return data;
}
// 
// INTERNAL FUNCTION
// Save the train data structure.
// 
static int fann_save_train_internal_fd_2(struct fann_train_data * data, FILE * file, const char * filename, uint save_as_fixed, uint decimal_point)
{
	uint num_data = data->num_data;
	uint num_input = data->num_input;
	uint num_output = data->num_output;
	uint i, j;
	int retval = 0;
#ifndef FIXEDFANN
	uint multiplier = 1 << decimal_point;
#endif
	fprintf(file, "%u %u %u\n", data->num_data, data->num_input, data->num_output);
	for(i = 0; i < num_data; i++) {
		for(j = 0; j < num_input; j++) {
#ifndef FIXEDFANN
			if(save_as_fixed) {
				fprintf(file, "%d ", (int)(data->input[i][j] * multiplier));
			}
			else {
				if((ffloori(data->input[i][j] + 0.5) * 1000000) == (ffloori(data->input[i][j] * 1000000.0 + 0.5))) {
					fprintf(file, "%d ", (int)data->input[i][j]);
				}
				else {
					fprintf(file, "%f ", data->input[i][j]);
				}
			}
#else
			fprintf(file, FANNPRINTF " ", data->input[i][j]);
#endif
		}
		fprintf(file, "\n");
		for(j = 0; j < num_output; j++) {
#ifndef FIXEDFANN
			if(save_as_fixed) {
				fprintf(file, "%d ", (int)(data->output[i][j] * multiplier));
			}
			else {
				if((ffloori(data->output[i][j] + 0.5) * 1000000) == (ffloori(data->output[i][j] * 1000000.0 + 0.5))) {
					fprintf(file, "%d ", (int)data->output[i][j]);
				}
				else {
					fprintf(file, "%f ", data->output[i][j]);
				}
			}
#else
			fprintf(file, FANNPRINTF " ", data->output[i][j]);
#endif
		}
		fprintf(file, "\n");
	}
	return retval;
}
// 
// INTERNAL FUNCTION
// Save the train data structure.
// 
static int fann_save_train_internal_2(struct fann_train_data * data, const char * filename, uint save_as_fixed, uint decimal_point)
{
	int    retval = -1;
	FILE * file = fopen(filename, "w");
	if(!file)
		fann_error_2((struct fann_error *)data, FANN_E_CANT_OPEN_TD_W, filename);
	else {
		retval = fann_save_train_internal_fd_2(data, file, filename, save_as_fixed, decimal_point);
		fclose(file);
	}
	return retval;
}
// 
// Save training data to a file
// 
FANN_EXTERNAL int FANN_API fann_save_train(struct fann_train_data * data, const char * filename)
	{ return fann_save_train_internal_2(data, filename, 0, 0); }
// 
// Save training data to a file in fixed point algebra. (Good for testing a network in fixed point)
// 
FANN_EXTERNAL int FANN_API fann_save_train_to_fixed(struct fann_train_data * data, const char * filename, uint decimal_point)
	{ return fann_save_train_internal_2(data, filename, 1, decimal_point); }
// 
// deallocate the train data structure.
// 
FANN_EXTERNAL void FANN_API fann_destroy_train(struct fann_train_data * data)
{
	if(data) {
		if(data->input)
			ZFREE(data->input[0]);
		if(data->output)
			ZFREE(data->output[0]);
		ZFREE(data->input);
		ZFREE(data->output);
		ZFREE(data);
	}
}
// 
// Test a set of training data and calculate the MSE
// 
FANN_EXTERNAL float FANN_API fann_test_data(Fann2 * ann, struct fann_train_data * data)
{
	if(fann_check_input_output_sizes(ann, data) == -1)
		return 0;
	fann_reset_MSE(ann);
	for(uint i = 0; i != data->num_data; i++) {
		fann_test(ann, data->input[i], data->output[i]);
	}
	return fann_get_MSE(ann);
}

#ifndef FIXEDFANN
// 
// Internal train function
// 
float fann_train_epoch_quickprop(Fann2 * ann, struct fann_train_data * data)
{
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	fann_reset_MSE(ann);
	for(uint i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->first_layer + 1, ann->last_layer - 1);
	}
	fann_update_weights_quickprop(ann, data->num_data, 0, ann->total_connections);
	return fann_get_MSE(ann);
}
// 
// Internal train function
// 
float fann_train_epoch_irpropm(Fann2 * ann, struct fann_train_data * data)
{
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	fann_reset_MSE(ann);
	for(uint i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->first_layer + 1, ann->last_layer - 1);
	}
	fann_update_weights_irpropm(ann, 0, ann->total_connections);
	return fann_get_MSE(ann);
}
// 
// Internal train function
// 
float fann_train_epoch_sarprop(Fann2 * ann, struct fann_train_data * data)
{
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	fann_reset_MSE(ann);
	for(uint i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->first_layer + 1, ann->last_layer - 1);
	}
	fann_update_weights_sarprop(ann, ann->sarprop_epoch, 0, ann->total_connections);
	++(ann->sarprop_epoch);
	return fann_get_MSE(ann);
}
// 
// Internal train function
// 
float fann_train_epoch_batch(Fann2 * ann, struct fann_train_data * data)
{
	fann_reset_MSE(ann);
	for(uint i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->first_layer + 1, ann->last_layer - 1);
	}
	//fann_update_weights_batch(ann, data->num_data, 0, ann->total_connections);
	//static void fann_update_weights_batch(Fann2 * ann, uint num_data, uint first_weight, uint past_end)
	{
		const uint past_end = ann->total_connections;
		float * train_slopes = ann->train_slopes;
		float * weights = ann->weights;
		const float epsilon = ann->learning_rate / data->num_data;
		for(uint i = 0; i != past_end; i++) {
			weights[i] += train_slopes[i] * epsilon;
			train_slopes[i] = 0.0;
		}
	}
	return fann_get_MSE(ann);
}
// 
// Internal train function
// 
float fann_train_epoch_incremental(Fann2 * ann, struct fann_train_data * data)
{
	fann_reset_MSE(ann);
	for(uint i = 0; i != data->num_data; i++) {
		fann_train(ann, data->input[i], data->output[i], 0);
	}
	return fann_get_MSE(ann);
}
// 
// Train for one epoch with the selected training algorithm
// 
FANN_EXTERNAL float FANN_API fann_train_epoch(Fann2 * ann, struct fann_train_data * data)
{
	if(fann_check_input_output_sizes(ann, data) == -1)
		return 0;
	switch(ann->training_algorithm) {
		case FANN_TRAIN_QUICKPROP: return fann_train_epoch_quickprop(ann, data);
		case FANN_TRAIN_RPROP: return fann_train_epoch_irpropm(ann, data);
		case FANN_TRAIN_SARPROP: return fann_train_epoch_sarprop(ann, data);
		case FANN_TRAIN_BATCH: return fann_train_epoch_batch(ann, data);
		case FANN_TRAIN_INCREMENTAL: return fann_train_epoch_incremental(ann, data);
	}
	return 0;
}

FANN_EXTERNAL void FANN_API fann_train_on_data(Fann2 * ann, struct fann_train_data * data, uint max_epochs, uint epochs_between_reports, float desired_error)
{
	float error;
	uint i;
	int desired_error_reached;
#ifdef DEBUG
	printf("Training with %s\n", FANN_TRAIN_NAMES[ann->training_algorithm]);
#endif
	if(epochs_between_reports && ann->callback == NULL) {
		printf("Max epochs %8d. Desired error: %.10f.\n", max_epochs, desired_error);
	}
	for(i = 1; i <= max_epochs; i++) {
		// 
		// train
		// 
		error = fann_train_epoch(ann, data);
		desired_error_reached = fann_desired_error_reached(ann, desired_error);
		// 
		// print current output
		// 
		if(epochs_between_reports && (i % epochs_between_reports == 0 || i == max_epochs || i == 1 ||
		    desired_error_reached == 0)) {
			if(ann->callback == NULL) {
				printf("Epochs     %8d. Current error: %.10f. Bit fail %d.\n", i, error, ann->num_bit_fail);
			}
			else if(((*ann->callback)(ann, data, max_epochs, epochs_between_reports, desired_error, i)) == -1) {
				// you can break the training by returning -1
				break;
			}
		}
		if(desired_error_reached == 0)
			break;
	}
}

FANN_EXTERNAL void FANN_API fann_train_on_file(Fann2 * ann, const char * filename, uint max_epochs, uint epochs_between_reports, float desired_error)
{
	struct fann_train_data * data = fann_read_train_from_file(filename);
	if(data) {
		fann_train_on_data(ann, data, max_epochs, epochs_between_reports, desired_error);
		fann_destroy_train(data);
	}
}

#endif
// 
// shuffles training data, randomizing the order
// 
FANN_EXTERNAL void FANN_API fann_shuffle_train_data(struct fann_train_data * train_data)
{
	uint dat = 0, elem, swap;
	float temp;
	for(; dat < train_data->num_data; dat++) {
		swap = (uint)(rand() % train_data->num_data);
		if(swap != dat) {
			for(elem = 0; elem < train_data->num_input; elem++) {
				temp = train_data->input[dat][elem];
				train_data->input[dat][elem] = train_data->input[swap][elem];
				train_data->input[swap][elem] = temp;
			}
			for(elem = 0; elem < train_data->num_output; elem++) {
				temp = train_data->output[dat][elem];
				train_data->output[dat][elem] = train_data->output[swap][elem];
				train_data->output[swap][elem] = temp;
			}
		}
	}
}
// 
// INTERNAL FUNCTION calculates min and max of train data
// 
void fann_get_min_max_data(float ** data, uint num_data, uint num_elem, float * min, float * max)
{
	*min = *max = data[0][0];
	for(uint dat = 0; dat < num_data; dat++) {
		for(uint elem = 0; elem < num_elem; elem++) {
			float temp = data[dat][elem];
			if(temp < *min)
				*min = temp;
			else if(temp > *max)
				*max = temp;
		}
	}
}

FANN_EXTERNAL float FANN_API fann_get_min_train_input(struct fann_train_data * train_data)
{
	float min, max;
	fann_get_min_max_data(train_data->input, train_data->num_data, train_data->num_input, &min, &max);
	return min;
}

FANN_EXTERNAL float FANN_API fann_get_max_train_input(struct fann_train_data * train_data)
{
	float min, max;
	fann_get_min_max_data(train_data->input, train_data->num_data, train_data->num_input, &min, &max);
	return max;
}

FANN_EXTERNAL float FANN_API fann_get_min_train_output(struct fann_train_data * train_data)
{
	float min, max;
	fann_get_min_max_data(train_data->output, train_data->num_data, train_data->num_output, &min, &max);
	return min;
}

FANN_EXTERNAL float FANN_API fann_get_max_train_output(struct fann_train_data * train_data)
{
	float min, max;
	fann_get_min_max_data(train_data->output, train_data->num_data, train_data->num_output, &min, &max);
	return max;
}
// 
// INTERNAL FUNCTION Scales data to a specific range
// 
void fann_scale_data(float ** data, uint num_data, uint num_elem, float new_min, float new_max)
{
	float old_min, old_max;
	fann_get_min_max_data(data, num_data, num_elem, &old_min, &old_max);
	fann_scale_data_to_range(data, num_data, num_elem, old_min, old_max, new_min, new_max);
}
// 
// INTERNAL FUNCTION Scales data to a specific range
// 
FANN_EXTERNAL void FANN_API fann_scale_data_to_range(float ** data, uint num_data, uint num_elem,
    float old_min, float old_max, float new_min, float new_max)
{
	float old_span = old_max - old_min;
	float new_span = new_max - new_min;
	float factor = new_span / old_span;
	// printf("max %f, min %f, factor %f\n", old_max, old_min, factor);
	for(uint dat = 0; dat < num_data; dat++) {
		for(uint elem = 0; elem < num_elem; elem++) {
			const float temp = (data[dat][elem] - old_min) * factor + new_min;
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
}
// 
// Scales the inputs in the training data to the specified range
// 
FANN_EXTERNAL void FANN_API fann_scale_input_train_data(struct fann_train_data * train_data, float new_min, float new_max)
	{ fann_scale_data(train_data->input, train_data->num_data, train_data->num_input, new_min, new_max); }
// 
// Scales the inputs in the training data to the specified range
// 
FANN_EXTERNAL void FANN_API fann_scale_output_train_data(struct fann_train_data * train_data, float new_min, float new_max)
	{ fann_scale_data(train_data->output, train_data->num_data, train_data->num_output, new_min, new_max); }
// 
// Scales the inputs in the training data to the specified range
// 
FANN_EXTERNAL void FANN_API fann_scale_train_data(struct fann_train_data * train_data, float new_min, float new_max)
{
	fann_scale_data(train_data->input, train_data->num_data, train_data->num_input, new_min, new_max);
	fann_scale_data(train_data->output, train_data->num_data, train_data->num_output, new_min, new_max);
}
// 
// merges training data into a single struct.
// 
FANN_EXTERNAL struct fann_train_data * FANN_API fann_merge_train_data(struct fann_train_data * data1, const struct fann_train_data * data2)
{
	uint i;
	float * data_input, * data_output;
	struct fann_train_data * dest = static_cast<struct fann_train_data *>(SAlloc::M(sizeof(struct fann_train_data)));
	if(dest == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data1), FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	if((data1->num_input != data2->num_input) || (data1->num_output != data2->num_output)) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data1), FANN_E_TRAIN_DATA_MISMATCH);
		return NULL;
	}
	fann_init_error_data((struct fann_error *)dest);
	dest->error_log = data1->error_log;
	dest->num_data = data1->num_data+data2->num_data;
	dest->num_input = data1->num_input;
	dest->num_output = data1->num_output;
	dest->input = static_cast<float **>(SAlloc::C(dest->num_data, sizeof(float *)));
	if(dest->input == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data1), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	dest->output = static_cast<float **>(SAlloc::C(dest->num_data, sizeof(float *)));
	if(dest->output == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data1), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	data_input = static_cast<float *>(SAlloc::C(dest->num_input * dest->num_data, sizeof(float)));
	if(data_input == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data1), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_input, data1->input[0], dest->num_input * data1->num_data * sizeof(float));
	memcpy(data_input + (dest->num_input*data1->num_data), data2->input[0], dest->num_input * data2->num_data * sizeof(float));
	data_output = static_cast<float *>(SAlloc::C(dest->num_output * dest->num_data, sizeof(float)));
	if(data_output == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data1), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_output, data1->output[0], dest->num_output * data1->num_data * sizeof(float));
	memcpy(data_output + (dest->num_output*data1->num_data), data2->output[0], dest->num_output * data2->num_data * sizeof(float));
	for(i = 0; i != dest->num_data; i++) {
		dest->input[i] = data_input;
		data_input += dest->num_input;
		dest->output[i] = data_output;
		data_output += dest->num_output;
	}
	return dest;
}
// 
// return a copy of a fann_train_data struct
// 
FANN_EXTERNAL struct fann_train_data * FANN_API fann_duplicate_train_data(struct fann_train_data * data)
{
	uint i;
	float * data_input, * data_output;
	struct fann_train_data * dest = static_cast<struct fann_train_data *>(SAlloc::M(sizeof(struct fann_train_data)));
	if(dest == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	fann_init_error_data((struct fann_error *)dest);
	dest->error_log = data->error_log;

	dest->num_data = data->num_data;
	dest->num_input = data->num_input;
	dest->num_output = data->num_output;
	dest->input = static_cast<float **>(SAlloc::C(dest->num_data, sizeof(float *)));
	if(dest->input == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	dest->output = static_cast<float **>(SAlloc::C(dest->num_data, sizeof(float *)));
	if(dest->output == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	data_input = static_cast<float *>(SAlloc::C(dest->num_input * dest->num_data, sizeof(float)));
	if(data_input == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_input, data->input[0], dest->num_input * dest->num_data * sizeof(float));
	data_output = static_cast<float *>(SAlloc::C(dest->num_output * dest->num_data, sizeof(float)));
	if(data_output == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_output, data->output[0], dest->num_output * dest->num_data * sizeof(float));
	for(i = 0; i != dest->num_data; i++) {
		dest->input[i] = data_input;
		data_input += dest->num_input;
		dest->output[i] = data_output;
		data_output += dest->num_output;
	}
	return dest;
}

FANN_EXTERNAL struct fann_train_data * FANN_API fann_subset_train_data(struct fann_train_data * data, uint pos, uint length)
{
	uint i;
	float * data_input, * data_output;
	struct fann_train_data * dest = static_cast<struct fann_train_data *>(SAlloc::M(sizeof(struct fann_train_data)));
	if(dest == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	if(pos > data->num_data || pos+length > data->num_data) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_TRAIN_DATA_SUBSET, pos, length, data->num_data);
		return NULL;
	}
	fann_init_error_data((struct fann_error *)dest);
	dest->error_log = data->error_log;
	dest->num_data = length;
	dest->num_input = data->num_input;
	dest->num_output = data->num_output;
	dest->input = static_cast<float **>(SAlloc::C(dest->num_data, sizeof(float *)));
	if(dest->input == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	dest->output = static_cast<float **>(SAlloc::C(dest->num_data, sizeof(float *)));
	if(dest->output == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	data_input = static_cast<float *>(SAlloc::C(dest->num_input * dest->num_data, sizeof(float)));
	if(data_input == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_input, data->input[pos], dest->num_input * dest->num_data * sizeof(float));
	data_output = static_cast<float *>(SAlloc::C(dest->num_output * dest->num_data, sizeof(float)));
	if(data_output == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(data), FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_output, data->output[pos], dest->num_output * dest->num_data * sizeof(float));
	for(i = 0; i != dest->num_data; i++) {
		dest->input[i] = data_input;
		data_input += dest->num_input;
		dest->output[i] = data_output;
		data_output += dest->num_output;
	}
	return dest;
}

FANN_EXTERNAL uint FANN_API fann_length_train_data(const struct fann_train_data * data) { return data->num_data; }
FANN_EXTERNAL uint FANN_API fann_num_input_train_data(const struct fann_train_data * data) { return data->num_input; }
FANN_EXTERNAL uint FANN_API fann_num_output_train_data(const struct fann_train_data * data) { return data->num_output; }
// 
// Creates an empty set of training data
// 
FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train(uint num_data, uint num_input, uint num_output)
{
	float * data_input, * data_output;
	uint i;
	struct fann_train_data * data = static_cast<struct fann_train_data *>(SAlloc::M(sizeof(struct fann_train_data)));
	if(data == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	fann_init_error_data((struct fann_error *)data);
	data->num_data = num_data;
	data->num_input = num_input;
	data->num_output = num_output;
	data->input = static_cast<float **>(SAlloc::C(num_data, sizeof(float *)));
	if(data->input == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(data);
		return NULL;
	}
	data->output = static_cast<float **>(SAlloc::C(num_data, sizeof(float *)));
	if(data->output == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(data);
		return NULL;
	}
	data_input = static_cast<float *>(SAlloc::C(num_input * num_data, sizeof(float)));
	if(data_input == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(data);
		return NULL;
	}
	data_output = static_cast<float *>(SAlloc::C(num_output * num_data, sizeof(float)));
	if(data_output == NULL) {
		fann_error_2(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(data);
		return NULL;
	}
	for(i = 0; i != num_data; i++) {
		data->input[i] = data_input;
		data_input += num_input;
		data->output[i] = data_output;
		data_output += num_output;
	}
	return data;
}

FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train_pointer_array(uint num_data, uint num_input,
    float ** input, uint num_output, float ** output)
{
	struct fann_train_data * data = fann_create_train(num_data, num_input, num_output);
	if(data) {
		for(uint i = 0; i < num_data; ++i) {
			memcpy(data->input[i], input[i], num_input*sizeof(float));
			memcpy(data->output[i], output[i], num_output*sizeof(float));
		}
	}
	return data;
}

FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train_array(uint num_data, uint num_input, float * input, uint num_output, float * output)
{
	struct fann_train_data * data = fann_create_train(num_data, num_input, num_output);
	if(data) {
		for(uint i = 0; i < num_data; ++i) {
			memcpy(data->input[i], &input[i*num_input], num_input*sizeof(float));
			memcpy(data->output[i], &output[i*num_output], num_output*sizeof(float));
		}
	}
	return data;
}
// 
// Creates training data from a callback function.
// 
FANN_EXTERNAL struct fann_train_data * FANN_API fann_create_train_from_callback(uint num_data, uint num_input, uint num_output,
    void (FANN_API * user_function)(uint, uint, uint, float *, float *))
{
	uint i;
	struct fann_train_data * data = fann_create_train(num_data, num_input, num_output);
	if(data == NULL) {
		return NULL;
	}
	for(i = 0; i != num_data; i++) {
		(*user_function)(i, num_input, num_output, data->input[i], data->output[i]);
	}
	return data;
}

FANN_EXTERNAL float * FANN_API fann_get_train_input(struct fann_train_data * data, uint position)
{
	if(position >= data->num_data)
		return NULL;
	return data->input[position];
}

FANN_EXTERNAL float * FANN_API fann_get_train_output(struct fann_train_data * data, uint position)
{
	if(position >= data->num_data)
		return NULL;
	return data->output[position];
}
// 
// INTERNAL FUNCTION Reads training data from a file descriptor.
// 
struct fann_train_data * fann_read_train_from_fd(FILE * file, const char * filename)
{
	uint num_input, num_output, num_data;
	uint line = 1;
	struct fann_train_data * data = 0;
	if(fscanf(file, "%u %u %u\n", &num_data, &num_input, &num_output) != 3)
		fann_error_2(NULL, FANN_E_CANT_READ_TD, filename, line);
	else {
		line++;
		data = fann_create_train(num_data, num_input, num_output);
		if(data) {
			for(uint i = 0; i != num_data; i++) {
				uint   j;
				for(j = 0; j != num_input; j++) {
					if(fscanf(file, FANNSCANF " ", &data->input[i][j]) != 1) {
						fann_error_2(NULL, FANN_E_CANT_READ_TD, filename, line);
						fann_destroy_train(data);
						return NULL;
					}
				}
				line++;
				for(j = 0; j != num_output; j++) {
					if(fscanf(file, FANNSCANF " ", &data->output[i][j]) != 1) {
						fann_error_2(NULL, FANN_E_CANT_READ_TD, filename, line);
						fann_destroy_train(data);
						return NULL;
					}
				}
				line++;
			}
		}
	}
	return data;
}
// 
// INTERNAL FUNCTION returns 0 if the desired error is reached and -1 if it is not reached
// 
int fann_desired_error_reached(const Fann2 * ann, float desired_error)
{
	switch(ann->train_stop_function) {
		case FANN_STOPFUNC_MSE:
		    if(fann_get_MSE(ann) <= desired_error)
			    return 0;
		    break;
		case FANN_STOPFUNC_BIT:
		    if(ann->num_bit_fail <= (uint)desired_error)
			    return 0;
		    break;
	}
	return -1;
}

#ifndef FIXEDFANN
// 
// Scale data in input vector before feed it to ann based on previously calculated parameters.
// 
FANN_EXTERNAL void FANN_API fann_scale_input(Fann2 * ann, float * input_vector)
{
	uint   cur_neuron;
	if(ann->scale_mean_in == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_SCALE_NOT_PRESENT);
		return;
	}
	for(cur_neuron = 0; cur_neuron < ann->num_input; cur_neuron++)
		if(ann->scale_deviation_in[ cur_neuron ] != 0.0)
			input_vector[cur_neuron] = ((input_vector[cur_neuron] - ann->scale_mean_in[cur_neuron]) / 
				ann->scale_deviation_in[cur_neuron] - ((float)-1.0) /* This is old_min */)
			    * ann->scale_factor_in[cur_neuron] + ann->scale_new_min_in[cur_neuron];
}
// 
// Scale data in output vector before feed it to ann based on previously calculated parameters.
// 
FANN_EXTERNAL void FANN_API fann_scale_output(Fann2 * ann, float * output_vector)
{
	uint   cur_neuron;
	if(ann->scale_mean_in == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_SCALE_NOT_PRESENT);
		return;
	}
	for(cur_neuron = 0; cur_neuron < ann->num_output; cur_neuron++)
		if(ann->scale_deviation_out[ cur_neuron ] != 0.0)
			output_vector[ cur_neuron ] =
			    (
				( output_vector[ cur_neuron ] - ann->scale_mean_out[ cur_neuron ])
				/ ann->scale_deviation_out[ cur_neuron ]
				- (-1.0f) /* This is old_min */
			    )
			    * ann->scale_factor_out[ cur_neuron ]
			    + ann->scale_new_min_out[ cur_neuron ];
}
// 
// Descale data in input vector after based on previously calculated parameters.
// 
FANN_EXTERNAL void FANN_API fann_descale_input(Fann2 * ann, float * input_vector)
{
	uint   cur_neuron;
	if(ann->scale_mean_in == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_SCALE_NOT_PRESENT);
		return;
	}
	for(cur_neuron = 0; cur_neuron < ann->num_input; cur_neuron++)
		if(ann->scale_deviation_in[ cur_neuron ] != 0.0)
			input_vector[cur_neuron] = ((input_vector[cur_neuron] - ann->scale_new_min_in[cur_neuron])
				/ ann->scale_factor_in[cur_neuron]
				+ (-1.0f) /* This is old_min */
			    ) * ann->scale_deviation_in[cur_neuron] + ann->scale_mean_in[cur_neuron];
}
// 
// Descale data in output vector after get it from ann based on previously calculated parameters.
// 
FANN_EXTERNAL void FANN_API fann_descale_output(Fann2 * ann, float * output_vector)
{
	uint   cur_neuron;
	if(ann->scale_mean_in == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_SCALE_NOT_PRESENT);
		return;
	}
	for(cur_neuron = 0; cur_neuron < ann->num_output; cur_neuron++)
		if(ann->scale_deviation_out[ cur_neuron ] != 0.0)
			output_vector[cur_neuron] =((output_vector[cur_neuron] - ann->scale_new_min_out[cur_neuron])
				/ ann->scale_factor_out[cur_neuron] + (-1.0f) /* This is old_min */
			    ) * ann->scale_deviation_out[cur_neuron] + ann->scale_mean_out[cur_neuron];
}
// 
// Scale input and output data based on previously calculated parameters.
// 
FANN_EXTERNAL void FANN_API fann_scale_train(Fann2 * ann, struct fann_train_data * data)
{
	uint   cur_sample;
	if(ann->scale_mean_in == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_SCALE_NOT_PRESENT);
		return;
	}
	// Check that we have good training data. 
	if(fann_check_input_output_sizes(ann, data) == -1)
		return;
	for(cur_sample = 0; cur_sample < data->num_data; cur_sample++) {
		fann_scale_input(ann, data->input[ cur_sample ]);
		fann_scale_output(ann, data->output[ cur_sample ]);
	}
}
// 
// Scale input and output data based on previously calculated parameters.
// 
FANN_EXTERNAL void FANN_API fann_descale_train(Fann2 * ann, struct fann_train_data * data)
{
	uint   cur_sample;
	if(ann->scale_mean_in == NULL) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_SCALE_NOT_PRESENT);
		return;
	}
	/* Check that we have good training data. */
	if(fann_check_input_output_sizes(ann, data) == -1)
		return;
	for(cur_sample = 0; cur_sample < data->num_data; cur_sample++) {
		fann_descale_input(ann, data->input[ cur_sample ]);
		fann_descale_output(ann, data->output[ cur_sample ]);
	}
}

#define SCALE_RESET(what, where, default_value)                                                       \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++) \
		ann->what ## _ ## where[ cur_neuron ] = ( default_value );

#define SCALE_SET_PARAM(where)                                                                                                                                                \
	/* Calculate mean: sum(x)/length */                                                                                                                                     \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		ann->scale_mean_ ## where[ cur_neuron ] = 0.0f;                                                                                                   \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		for(cur_sample = 0; cur_sample < data->num_data; cur_sample++)                                                                \
			ann->scale_mean_ ## where[ cur_neuron ] += (float)data->where ## put[ cur_sample ][ cur_neuron ]; \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		ann->scale_mean_ ## where[ cur_neuron ] /= (float)data->num_data;                                                                 \
	/* Calculate deviation: sqrt(sum((x-mean)^2)/length) */                                                                                         \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		ann->scale_deviation_ ## where[ cur_neuron ] = 0.0f;                                                                                              \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		for(cur_sample = 0; cur_sample < data->num_data; cur_sample++)                                                                \
			ann->scale_deviation_ ## where[ cur_neuron ] +=                                                                                           \
			    /* Another local variable in macro? Oh no! */                                                                                   \
			    ((float)data->where ## put[cur_sample][cur_neuron] - ann->scale_mean_ ## where[cur_neuron]) * \
			    ((float)data->where ## put[cur_sample][cur_neuron] - ann->scale_mean_ ## where[cur_neuron]); \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		ann->scale_deviation_ ## where[ cur_neuron ] =                                                                                                    \
		    sqrtf(ann->scale_deviation_ ## where[ cur_neuron ] / (float)data->num_data);                    \
	/* Calculate factor: (new_max-new_min)/(old_max(1)-old_min(-1)) */                                                                      \
	/* Looks like we dont need whole array of factors? */                                                                                           \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		ann->scale_factor_ ## where[ cur_neuron ] =                                                                                                               \
		    ( new_ ## where ## put_max - new_ ## where ## put_min )                                                                                         \
		    /                                                                                                                                                                                       \
		    ( 1.0f - ( -1.0f ));                                                                                                                                           \
	/* Copy new minimum. */                                                                                                                                                         \
	/* Looks like we dont need whole array of new minimums? */                                                                                      \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)                                                         \
		ann->scale_new_min_ ## where[ cur_neuron ] = new_ ## where ## put_min;

FANN_EXTERNAL int FANN_API fann_set_input_scaling_params(Fann2 * ann, const struct fann_train_data * data, float new_input_min, float new_input_max)
{
	uint   cur_neuron, cur_sample;
	/* Check that we have good training data. */
	/* No need for if( !params || !ann ) */
	if(data->num_input != ann->num_input || data->num_output != ann->num_output) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_TRAIN_DATA_MISMATCH);
		return -1;
	}
	if(ann->scale_mean_in == NULL)
		fann_allocate_scale(ann);
	if(ann->scale_mean_in == NULL)
		return -1;
	if(!data->num_data) {
		SCALE_RESET(scale_mean,                in,     0.0)
		SCALE_RESET(scale_deviation,   in,     1.0)
		SCALE_RESET(scale_new_min,             in,     -1.0)
		SCALE_RESET(scale_factor,              in,     1.0)
	}
	else {
		SCALE_SET_PARAM(in);
	}
	return 0;
}

FANN_EXTERNAL int FANN_API fann_set_output_scaling_params(Fann2 * ann, const struct fann_train_data * data, float new_output_min, float new_output_max)
{
	uint   cur_neuron, cur_sample;
	/* Check that we have good training data. */
	/* No need for if( !params || !ann ) */
	if(data->num_input != ann->num_input || data->num_output != ann->num_output) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_TRAIN_DATA_MISMATCH);
		return -1;
	}
	if(ann->scale_mean_out == NULL)
		fann_allocate_scale(ann);
	if(ann->scale_mean_out == NULL)
		return -1;
	if(!data->num_data) {
		SCALE_RESET(scale_mean,                out,    0.0)
		SCALE_RESET(scale_deviation,   out,    1.0)
		SCALE_RESET(scale_new_min,             out,    -1.0)
		SCALE_RESET(scale_factor,              out,    1.0)
	}
	else {
		SCALE_SET_PARAM(out);
	}
	return 0;
}
// 
// Calculate scaling parameters for future use based on training data.
// 
FANN_EXTERNAL int FANN_API fann_set_scaling_params(Fann2 * ann, const struct fann_train_data * data, float new_input_min, float new_input_max, float new_output_min, float new_output_max)
{
	if(fann_set_input_scaling_params(ann, data, new_input_min, new_input_max) == 0)
		return fann_set_output_scaling_params(ann, data, new_output_min, new_output_max);
	else
		return -1;
}
// 
// Clears scaling parameters.
// 
FANN_EXTERNAL int FANN_API fann_clear_scaling_params(Fann2 * ann)
{
	uint   cur_neuron;
	if(ann->scale_mean_out == NULL)
		fann_allocate_scale(ann);
	if(ann->scale_mean_out == NULL)
		return -1;
	SCALE_RESET(scale_mean,                in,     0.0)
	SCALE_RESET(scale_deviation,   in,     1.0)
	SCALE_RESET(scale_new_min,             in,     -1.0)
	SCALE_RESET(scale_factor,              in,     1.0)
	SCALE_RESET(scale_mean,                out,    0.0)
	SCALE_RESET(scale_deviation,   out,    1.0)
	SCALE_RESET(scale_new_min,             out,    -1.0)
	SCALE_RESET(scale_factor,              out,    1.0)
	return 0;
}

#endif

int fann_check_input_output_sizes(Fann2 * ann, const struct fann_train_data * data)
{
	if(ann->num_input != data->num_input) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_INPUT_NO_MATCH, ann->num_input, data->num_input);
		return -1;
	}
	if(ann->num_output != data->num_output) {
		fann_error_2(reinterpret_cast<struct fann_error *>(ann), FANN_E_OUTPUT_NO_MATCH, ann->num_output, data->num_output);
		return -1;
	}
	return 0;
}
//
//
//
static const char * MakeFannTestFilePath(const char * pFn)
{
	SString & r_buf = SLS.AcquireRvlStr();
	SLS.QueryPath("testroot", r_buf);
	return r_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("ann").SetLastSlash().Cat(pFn);
}

static const char * MakeFannTestOutFilePath(const char * pFn)
{
	SString & r_buf = SLS.AcquireRvlStr();
	SLS.QueryPath("testroot", r_buf);
	return r_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("ann").SetLastSlash().Cat(pFn);
}

static void FannLogMessage(const char * pMsg)
{
	SString & r_fn = SLS.AcquireRvlStr();
	SLS.QueryPath("log", r_fn);
	r_fn.SetLastSlash().Cat("fann.log");
	SLS.LogMessage(r_fn, pMsg, 0);
}

static void train_on_steepness_file(Fann2 * ann, const char * filename, uint max_epochs, uint epochs_between_reports,
    float desired_error, float steepness_start, float steepness_step, float steepness_end)
{
	SString log_buf;
	float error;
	uint i;
	struct fann_train_data * data = fann_read_train_from_file(filename);
	if(epochs_between_reports) {
		FannLogMessage(log_buf.Printf("Max epochs %8d. Desired error: %.10f", max_epochs, desired_error));
	}
	fann_set_activation_steepness_hidden(ann, steepness_start);
	fann_set_activation_steepness_output(ann, steepness_start);
	for(i = 1; i <= max_epochs; i++) {
		/* train */
		error = fann_train_epoch(ann, data);
		/* print current output */
		if(epochs_between_reports && (i % epochs_between_reports == 0 || i == max_epochs || i == 1 || error < desired_error)) {
			FannLogMessage(log_buf.Printf("Epochs     %8d. Current error: %.10f", i, error));
		}
		if(error < desired_error) {
			steepness_start += steepness_step;
			if(steepness_start <= steepness_end) {
				FannLogMessage(log_buf.Printf("Steepness: %f", steepness_start));
				fann_set_activation_steepness_hidden(ann, steepness_start);
				fann_set_activation_steepness_output(ann, steepness_start);
			}
			else {
				break;
			}
		}
	}
	fann_destroy_train(data);
}

static int FANN_API xor_train_test_callback(const Fann2 * ann, struct fann_train_data * train, uint max_epochs, uint epochs_between_reports, float desired_error, uint epochs)
{
	SString log_buf;
	FannLogMessage(log_buf.Printf("Epochs     %8d. MSE: %.5f. Desired-MSE: %.5f", epochs, fann_get_MSE(ann), desired_error));
	return 0;
}

int TestFann2()
{
	int    ok =1;
	uint   i;
	SString log_buf;
	//int simple_train() // main
	{
		FannLogMessage("FANN simple_train");
		const uint num_input = 2;
		const uint num_output = 1;
		const uint num_layers = 3;
		const uint num_neurons_hidden = 3;
		const float desired_error = 0.001f;
		const uint max_epochs = 500000;
		const uint epochs_between_reports = 1000;
		Fann2 * ann = fann_create_standard_2(num_layers, num_input, num_neurons_hidden, num_output);
		fann_set_activation_function_hidden(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_activation_function_output(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_train_on_file(ann, MakeFannTestFilePath("xor.data"), max_epochs, epochs_between_reports, desired_error);
		fann_save(ann, MakeFannTestOutFilePath("xor_float.net"));
		fann_destroy(ann);
	}
	//xor_train();
	{
		FannLogMessage("FANN xor_train");
		const uint num_input = 2;
		const uint num_output = 1;
		const uint num_layers = 3;
		const uint num_neurons_hidden = 3;
		const float desired_error = 0.0f;
		const uint max_epochs = 1000;
		const uint epochs_between_reports = 10;
		Fann2 * ann;
		struct fann_train_data * data;

		uint decimal_point;
		FannLogMessage(log_buf.Printf("Creating network."));
		ann = fann_create_standard_2(num_layers, num_input, num_neurons_hidden, num_output);
		data = fann_read_train_from_file(MakeFannTestFilePath("xor.data"));
		fann_set_activation_steepness_hidden(ann, 1);
		fann_set_activation_steepness_output(ann, 1);
		fann_set_activation_function_hidden(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_activation_function_output(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_train_stop_function(ann, FANN_STOPFUNC_BIT);
		fann_set_bit_fail_limit(ann, 0.01f);
		fann_set_training_algorithm(ann, FANN_TRAIN_RPROP);
		fann_init_weights(ann, data);
		FannLogMessage(log_buf.Printf("Training network."));
		fann_train_on_data(ann, data, max_epochs, epochs_between_reports, desired_error);
		FannLogMessage(log_buf.Printf("Testing network. %f", fann_test_data(ann, data)));
		for(i = 0; i < fann_length_train_data(data); i++) {
			const float * calc_out = fann_run(ann, data->input[i]);
			FannLogMessage(log_buf.Printf("XOR test (%f,%f) -> %f, should be %f, difference=%f", data->input[i][0], data->input[i][1], calc_out[0], data->output[i][0], fabsf(calc_out[0] - data->output[i][0])));
		}
		FannLogMessage(log_buf.Printf("Saving network."));
		fann_save(ann, MakeFannTestOutFilePath("xor_float.net"));
		decimal_point = fann_save_to_fixed(ann, MakeFannTestOutFilePath("xor_fixed.net"));
		fann_save_train_to_fixed(data, MakeFannTestOutFilePath("xor_fixed.data"), decimal_point);
		fann_destroy_train(data);
		fann_destroy(ann);
	}
	//steepness_train();
	{
		FannLogMessage("FANN steepness_train");
		const uint num_input = 2;
		const uint num_output = 1;
		const uint num_layers = 3;
		const uint num_neurons_hidden = 3;
		const float desired_error = 0.001f;
		const uint max_epochs = 500000;
		const uint epochs_between_reports = 1000;
		float * calc_out;
		struct fann_train_data * data;
		Fann2 * ann = fann_create_standard_2(num_layers, num_input, num_neurons_hidden, num_output);
		data = fann_read_train_from_file(MakeFannTestFilePath("xor.data"));
		fann_set_activation_function_hidden(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_activation_function_output(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_training_algorithm(ann, FANN_TRAIN_QUICKPROP);
		train_on_steepness_file(ann, MakeFannTestFilePath("xor.data"), max_epochs, epochs_between_reports, desired_error, (float)1.0, (float)0.1, (float)20.0);
		fann_set_activation_function_hidden(ann, Fann2::FANN_THRESHOLD_SYMMETRIC);
		fann_set_activation_function_output(ann, Fann2::FANN_THRESHOLD_SYMMETRIC);
		for(i = 0; i != fann_length_train_data(data); i++) {
			calc_out = fann_run(ann, data->input[i]);
			FannLogMessage(log_buf.Printf("XOR test (%f, %f) -> %f, should be %f, difference=%f", data->input[i][0], data->input[i][1], calc_out[0], data->output[i][0], (float)fabsf(calc_out[0] - data->output[i][0])));
		}
		fann_save(ann, MakeFannTestOutFilePath("xor_float.net"));
		fann_destroy(ann);
		fann_destroy_train(data);
	}
	//cascade_train();
	{
		FannLogMessage("FANN cascade_train");
		Fann2 * ann;
		struct fann_train_data * train_data, * test_data;
		const float desired_error = 0.0f;
		uint max_neurons = 30;
		uint neurons_between_reports = 1;
		uint bit_fail_train, bit_fail_test;
		float mse_train, mse_test;
		float steepness;
		int multi = 0;
		Fann2::ActivationFunc activation;
		enum fann_train_enum training_algorithm = FANN_TRAIN_RPROP;
		FannLogMessage(log_buf.Printf("Reading data."));
		train_data = fann_read_train_from_file(MakeFannTestFilePath("parity8.train"));
		test_data = fann_read_train_from_file(MakeFannTestFilePath("parity8.test"));
		fann_scale_train_data(train_data, -1, 1);
		fann_scale_train_data(test_data, -1, 1);
		FannLogMessage(log_buf.Printf("Creating network."));
		ann = fann_create_shortcut_2(2, fann_num_input_train_data(train_data), fann_num_output_train_data(train_data));
		fann_set_training_algorithm(ann, training_algorithm);
		fann_set_activation_function_hidden(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_activation_function_output(ann, Fann2::FANN_LINEAR);
		fann_set_train_error_function(ann, FANN_ERRORFUNC_LINEAR);
		if(!multi) {
			/*steepness = 0.5;*/
			steepness = 1;
			fann_set_cascade_activation_steepnesses(ann, &steepness, 1);
			/*activation = Fann2::FANN_SIN_SYMMETRIC;*/
			activation = Fann2::FANN_SIGMOID_SYMMETRIC;
			fann_set_cascade_activation_functions(ann, &activation, 1);
			fann_set_cascade_num_candidate_groups(ann, 8);
		}
		if(training_algorithm == FANN_TRAIN_QUICKPROP) {
			fann_set_learning_rate(ann, 0.35f);
			fann_randomize_weights(ann, -2.0f, 2.0f);
		}
		fann_set_bit_fail_limit(ann, 0.9f);
		fann_set_train_stop_function(ann, FANN_STOPFUNC_BIT);
		fann_print_parameters(ann);
		fann_save(ann, MakeFannTestOutFilePath("cascade_train2.net"));
		FannLogMessage(log_buf.Printf("Training network."));
		fann_cascadetrain_on_data(ann, train_data, max_neurons, neurons_between_reports, desired_error);
		fann_print_connections(ann);
		mse_train = fann_test_data(ann, train_data);
		bit_fail_train = fann_get_bit_fail(ann);
		mse_test = fann_test_data(ann, test_data);
		bit_fail_test = fann_get_bit_fail(ann);
		FannLogMessage(log_buf.Printf("\nTrain error: %f, Train bit-fail: %d, Test error: %f, Test bit-fail: %d\n", mse_train, bit_fail_train, mse_test, bit_fail_test));
		for(i = 0; i < train_data->num_data; i++) {
			const float * output = fann_run(ann, train_data->input[i]);
			if((train_data->output[i][0] >= 0 && output[0] <= 0) || (train_data->output[i][0] <= 0 && output[0] >= 0)) {
				FannLogMessage(log_buf.Printf("ERROR: %f does not match %f", train_data->output[i][0], output[0]));
			}
		}
		FannLogMessage(log_buf.Printf("Saving network."));
		fann_save(ann, MakeFannTestOutFilePath("cascade_train.net"));
		fann_destroy_train(train_data);
		fann_destroy_train(test_data);
		fann_destroy(ann);
	}
	//momentums();
	{
		FannLogMessage("FANN momentums");
		const uint num_layers = 3;
		const uint num_neurons_hidden = 96;
		const float desired_error = 0.001f;
		struct fann_train_data * train_data = fann_read_train_from_file(MakeFannTestFilePath("robot.train"));
		struct fann_train_data * test_data = fann_read_train_from_file(MakeFannTestFilePath("robot.test"));
		for(float momentum = 0.0f; momentum < 0.7f; momentum += 0.1f) {
			FannLogMessage(log_buf.Printf("============= momentum = %f =============", momentum));
			Fann2 * ann = fann_create_standard_2(num_layers, train_data->num_input, num_neurons_hidden, train_data->num_output);
			fann_set_training_algorithm(ann, FANN_TRAIN_INCREMENTAL);
			fann_set_learning_momentum(ann, momentum);
			fann_train_on_data(ann, train_data, 2000, 500, desired_error);
			FannLogMessage(log_buf.Printf("MSE error on train data: %f", fann_test_data(ann, train_data)));
			FannLogMessage(log_buf.Printf("MSE error on test data : %f", fann_test_data(ann, test_data)));
			fann_destroy(ann);
		}
		fann_destroy_train(train_data);
		fann_destroy_train(test_data);
	}
	//mushroom();
	{
		FannLogMessage("FANN mushroom");
		const uint num_layers = 3;
		const uint num_neurons_hidden = 32;
		const float desired_error = 0.0001f;
		const uint max_epochs = 300;
		const uint epochs_between_reports = 10;
		Fann2 * ann;
		struct fann_train_data * train_data, * test_data;
		FannLogMessage(log_buf.Printf("Creating network."));
		train_data = fann_read_train_from_file(MakeFannTestFilePath("mushroom.train"));
		ann = fann_create_standard_2(num_layers, train_data->num_input, num_neurons_hidden, train_data->num_output);
		FannLogMessage(log_buf.Printf("Training network."));
		fann_set_activation_function_hidden(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_activation_function_output(ann, Fann2::FANN_SIGMOID);
		/*fann_set_training_algorithm(ann, FANN_TRAIN_INCREMENTAL); */
		fann_train_on_data(ann, train_data, max_epochs, epochs_between_reports, desired_error);
		FannLogMessage(log_buf.Printf("Testing network."));
		test_data = fann_read_train_from_file(MakeFannTestFilePath("mushroom.test"));
		fann_reset_MSE(ann);
		for(i = 0; i < fann_length_train_data(test_data); i++) {
			fann_test(ann, test_data->input[i], test_data->output[i]);
		}
		FannLogMessage(log_buf.Printf("MSE error on test data: %f", fann_get_MSE(ann)));
		FannLogMessage(log_buf.Printf("Saving network."));
		fann_save(ann, MakeFannTestOutFilePath("mushroom_float.net"));
		fann_destroy_train(train_data);
		fann_destroy_train(test_data);
		fann_destroy(ann);
	}
	//robot();
	{
		FannLogMessage("FANN robot");
		const uint num_layers = 3;
		const uint num_neurons_hidden = 96;
		const float desired_error = 0.001f;
		Fann2 * ann;
		struct fann_train_data * train_data, * test_data;
		FannLogMessage(log_buf.Printf("Creating network."));
		train_data = fann_read_train_from_file(MakeFannTestFilePath("robot.train"));
		ann = fann_create_standard_2(num_layers, train_data->num_input, num_neurons_hidden, train_data->num_output);
		FannLogMessage(log_buf.Printf("Training network."));
		fann_set_training_algorithm(ann, FANN_TRAIN_INCREMENTAL);
		fann_set_learning_momentum(ann, 0.4f);
		fann_train_on_data(ann, train_data, 3000, 10, desired_error);
		FannLogMessage(log_buf.Printf("Testing network."));
		test_data = fann_read_train_from_file(MakeFannTestFilePath("robot.test"));
		fann_reset_MSE(ann);
		for(i = 0; i < fann_length_train_data(test_data); i++) {
			fann_test(ann, test_data->input[i], test_data->output[i]);
		}
		FannLogMessage(log_buf.Printf("MSE error on test data: %f", fann_get_MSE(ann)));
		FannLogMessage(log_buf.Printf("Saving network."));
		fann_save(ann, MakeFannTestOutFilePath("robot_float.net"));
		fann_destroy_train(train_data);
		fann_destroy_train(test_data);
		fann_destroy(ann);
	}
	//scaling_train();
	{
		FannLogMessage("FANN scaling_train");
		const uint num_input = 3;
		const uint num_output = 1;
		const uint num_layers = 4;
		const uint num_neurons_hidden = 5;
		const float desired_error = 0.0001f;
		const uint max_epochs = 5000;
		const uint epochs_between_reports = 1000;
		struct fann_train_data * data = NULL;
		Fann2 * ann = fann_create_standard_2(num_layers, num_input, num_neurons_hidden, num_neurons_hidden, num_output);
		fann_set_activation_function_hidden(ann, Fann2::FANN_SIGMOID_SYMMETRIC);
		fann_set_activation_function_output(ann, Fann2::FANN_LINEAR);
		fann_set_training_algorithm(ann, FANN_TRAIN_RPROP);
		data = fann_read_train_from_file(MakeFannTestFilePath("scaling.data"));
		fann_set_scaling_params(ann, data, -1/* New input minimum */, 1/* New input maximum */, -1/* New output minimum */, 1/* New output maximum */);
		fann_scale_train(ann, data);
		fann_train_on_data(ann, data, max_epochs, epochs_between_reports, desired_error);
		fann_destroy_train(data);
		fann_save(ann, MakeFannTestOutFilePath("scaling.net"));
		fann_destroy(ann);
	}
	//scaling_test();
	{
		FannLogMessage("FANN scaling_test");
		FannLogMessage(log_buf.Printf("Creating network."));
		Fann2 * ann = fann_create_from_file(MakeFannTestOutFilePath("scaling.net"));
		if(!ann) {
			FannLogMessage(log_buf.Printf("Error creating ann --- ABORTING."));
		}
		else {
			fann_print_connections(ann);
			fann_print_parameters(ann);
			FannLogMessage(log_buf.Printf("Testing network."));
			struct fann_train_data * data = fann_read_train_from_file(MakeFannTestFilePath("scaling.data"));
			for(i = 0; i < fann_length_train_data(data); i++) {
				fann_reset_MSE(ann);
				fann_scale_input(ann, data->input[i]);
				float * calc_out = fann_run(ann, data->input[i]);
				fann_descale_output(ann, calc_out);
				FannLogMessage(log_buf.Printf("Result %f original %f error %f", calc_out[0], data->output[i][0], (float)fabsf(calc_out[0] - data->output[i][0])));
			}
			fann_destroy_train(data);
			fann_destroy(ann);
		}
	}
	return ok;
}

#if 0 // @construction {
#if SLTEST_RUNNING // {

static int AssertWeights(STestCase * pTc, const Fann * pNet, float min, float max, float avg)
{
	TSVector <FannConnection> connections;
	pNet->GetConnectionArray(connections);
    float min_weight = connections.at(0).Weight;
    float max_weight = connections.at(0).Weight;
    float total_weight = 0.0;
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
    pTc->SLTEST_CHECK_EQ_TOL(avg, total_weight / (float)conn_count, 0.5f);
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

SLTEST_R(FANN2)
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
		float XorInput[] = {
			0.0, 0.0,
			0.0, 1.0,
			1.0, 0.0,
			1.0, 1.0
		};
		float XorOutput[] = {
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
				net.train((float *) (const float[]) {0.0, 0.0}, (float *) (const float[]) {0.0});
				net.train((float *) (const float[]) {1.0, 0.0}, (float *) (const float[]) {1.0});
				net.train((float *) (const float[]) {0.0, 1.0}, (float *) (const float[]) {1.0});
				net.train((float *) (const float[]) {1.0, 1.0}, (float *) (const float[]) {0.0});
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
				input_file_name = MakeInputFilePath((temp_buf = test_item_name).DotCat("train"));
				test_input_file_name = MakeInputFilePath((temp_buf = test_item_name).DotCat("test"));
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
#endif // } 0 @construction