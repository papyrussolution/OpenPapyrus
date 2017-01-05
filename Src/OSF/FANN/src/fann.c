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
#include "config.h"
#include "fann.h"

/* #define FANN_NO_SEED */

FANN_EXTERNAL Fann * FANN_API fann_create_standard(uint num_layers, ...)
{
	Fann * ann;
	va_list layer_sizes;
	int i;
	int status;
	int arg;
	uint * layers = (uint*)calloc(num_layers, sizeof(uint));
	if(layers == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
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
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		free(layers);
		return NULL;
	}
	ann = fann_create_standard_array(num_layers, layers);
	free(layers);
	return ann;
}

FANN_EXTERNAL Fann * FANN_API fann_create_standard_array(uint num_layers, const uint * layers)
{
	return fann_create_sparse_array(1, num_layers, layers);
}

FANN_EXTERNAL Fann * FANN_API fann_create_sparse(float connection_rate, uint num_layers, ...)
{
	Fann * ann;
	va_list layer_sizes;
	int i;
	int status;
	int arg;
	uint * layers = (uint*)calloc(num_layers, sizeof(uint));
	if(layers == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
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
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		free(layers);
		return NULL;
	}
	ann = fann_create_sparse_array(connection_rate, num_layers, layers);
	free(layers);
	return ann;
}

FANN_EXTERNAL Fann * FANN_API fann_create_sparse_array(float connection_rate, uint num_layers, const uint * layers)
{
	FannLayer * layer_it, * last_layer, * prev_layer;
	Fann * ann;
	FannNeuron * neuron_it, * last_neuron, * random_neuron, * bias_neuron;
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
	fann_seed_rand();
	ann = fann_allocate_structure(num_layers);
	if(ann == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	ann->connection_rate = connection_rate;
#ifdef FIXEDFANN
	multiplier = ann->multiplier;
	fann_update_stepwise(ann);
#endif
	// determine how many neurons there should be in each layer
	i = 0;
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		// we do not allocate room here, but we make sure that
		// last_neuron - first_neuron is the number of neurons
		layer_it->P_FirstNeuron = NULL;
		layer_it->P_LastNeuron = layer_it->P_FirstNeuron + layers[i++] + 1;       /* +1 for bias */
		ann->total_neurons += (uint)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron);
	}
	ann->num_output = (uint)((ann->P_LastLayer-1)->P_LastNeuron - (ann->P_LastLayer-1)->P_FirstNeuron - 1);
	ann->num_input = (uint)(ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron - 1);
	// allocate room for the actual neurons
	if(!ann->AllocateNeurons()) {
		fann_destroy(ann);
		return NULL;
	}
#ifdef DEBUG
	printf("creating network with connection rate %f\n", connection_rate);
	printf("input\n");
	printf("  layer       : %d neurons, 1 bias\n", (int)(ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron - 1));
#endif
	num_neurons_in = ann->num_input;
	for(layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer; layer_it++) {
		num_neurons_out = (uint)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron - 1);
		/*�if all neurons in each layer should be connected to at least one neuron
		 * in the previous layer, and one neuron in the next layer.
		 * and the bias node should be connected to the all neurons in the next layer.
		 * Then this is the minimum amount of neurons */
		min_connections = MAX(num_neurons_in, num_neurons_out); /* not calculating bias */
		max_connections = num_neurons_in * num_neurons_out;          /* not calculating bias */
		num_connections = MAX(min_connections, (uint)(0.5 + (connection_rate * max_connections))) + num_neurons_out;
		connections_per_neuron = num_connections / num_neurons_out;
		allocated_connections = 0;
		/* Now split out the connections on the different neurons */
		for(i = 0; i != num_neurons_out; i++) {
			layer_it->P_FirstNeuron[i].first_con = ann->total_connections + allocated_connections;
			allocated_connections += connections_per_neuron;
			layer_it->P_FirstNeuron[i].last_con = ann->total_connections + allocated_connections;
			layer_it->P_FirstNeuron[i].activation_function = FANN_SIGMOID_STEPWISE;
#ifdef FIXEDFANN
			layer_it->P_FirstNeuron[i].activation_steepness = ann->multiplier / 2;
#else
			layer_it->P_FirstNeuron[i].activation_steepness = 0.5;
#endif
			if(allocated_connections < (num_connections * (i + 1)) / num_neurons_out) {
				layer_it->P_FirstNeuron[i].last_con++;
				allocated_connections++;
			}
		}
		/* bias neuron also gets stuff */
		layer_it->P_FirstNeuron[i].first_con = ann->total_connections + allocated_connections;
		layer_it->P_FirstNeuron[i].last_con = ann->total_connections + allocated_connections;
		ann->total_connections += num_connections;
		/* used in the next run of the loop */
		num_neurons_in = num_neurons_out;
	}
	fann_allocate_connections(ann);
	if(ann->IsError(FANN_E_CANT_ALLOCATE_MEM)) {
		fann_destroy(ann);
		return NULL;
	}
	if(connection_rate >= 1) {
#ifdef DEBUG
		prev_layer_size = ann->num_input + 1;
#endif
		prev_layer = ann->P_FirstLayer;
		last_layer = ann->P_LastLayer;
		for(layer_it = ann->P_FirstLayer + 1; layer_it != last_layer; layer_it++) {
			last_neuron = layer_it->P_LastNeuron - 1;
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				tmp_con = neuron_it->last_con - 1;
				for(i = neuron_it->first_con; i != tmp_con; i++) {
					ann->weights[i] = (fann_type)fann_random_weight();
					/* these connections are still initialized for fully connected networks, to
					   allow
					 * operations to work, that are not optimized for fully connected networks.
					 */
					ann->connections[i] = prev_layer->P_FirstNeuron + (i - neuron_it->first_con);
				}
				// bias weight
				ann->weights[tmp_con] = (fann_type)fann_random_bias_weight();
				ann->connections[tmp_con] = prev_layer->P_FirstNeuron + (tmp_con - neuron_it->first_con);
			}
#ifdef DEBUG
			prev_layer_size = layer_it->P_LastNeuron - layer_it->P_FirstNeuron;
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

		/* All the connections are cleared by calloc, because we want to
		 * be able to see which connections are allready connected */

		for(layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer; layer_it++) {
			num_neurons_out = (uint)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron - 1);
			num_neurons_in = (uint)((layer_it - 1)->P_LastNeuron - (layer_it - 1)->P_FirstNeuron - 1);
			/* first connect the bias neuron */
			bias_neuron = (layer_it - 1)->P_LastNeuron - 1;
			last_neuron = layer_it->P_LastNeuron - 1;
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				ann->connections[neuron_it->first_con] = bias_neuron;
				ann->weights[neuron_it->first_con] = (fann_type)fann_random_bias_weight();
			}
			/* then connect all neurons in the input layer */
			last_neuron = (layer_it - 1)->P_LastNeuron - 1;
			for(neuron_it = (layer_it - 1)->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				/* random neuron in the output layer that has space
				 * for more connections */
				do {
					random_number = (int)(0.5 + fann_rand(0, num_neurons_out - 1));
					random_neuron = layer_it->P_FirstNeuron + random_number;
					/* checks the last space in the connections array for room */
				}
				while(ann->connections[random_neuron->last_con - 1]);
				/* find an empty space in the connection array and connect */
				for(i = random_neuron->first_con; i < random_neuron->last_con; i++) {
					if(ann->connections[i] == NULL) {
						ann->connections[i] = neuron_it;
						ann->weights[i] = (fann_type)fann_random_weight();
						break;
					}
				}
			}
			/* then connect the rest of the unconnected neurons */
			last_neuron = layer_it->P_LastNeuron - 1;
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				/* find empty space in the connection array and connect */
				for(i = neuron_it->first_con; i < neuron_it->last_con; i++) {
					/* continue if allready connected */
					if(ann->connections[i] != NULL)
						continue;
					do {
						found_connection = 0;
						random_number = (int)(0.5 + fann_rand(0, num_neurons_in - 1));
						random_neuron = (layer_it - 1)->P_FirstNeuron + random_number;
						/* check to see if this connection is allready there */
						for(j = neuron_it->first_con; j < i; j++) {
							if(random_neuron == ann->connections[j]) {
								found_connection = 1;
								break;
							}
						}
					} while(found_connection);
					/* we have found a neuron that is not allready
					 * connected to us, connect it */
					ann->connections[i] = random_neuron;
					ann->weights[i] = (fann_type)fann_random_weight();
				}
			}
#ifdef DEBUG
			printf("  layer       : %d neurons, 1 bias\n", num_neurons_out);
#endif
		}

		/* TODO it would be nice to have the randomly created
		 * connections sorted for smoother memory access.
		 */
	}
#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}

FANN_EXTERNAL Fann * FANN_API fann_create_shortcut(uint num_layers, ...)
{
	Fann * ann;
	int i;
	int status;
	int arg;
	va_list layer_sizes;
	uint * layers = (uint*)calloc(num_layers, sizeof(uint));
	if(layers == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
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
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		free(layers);
		return NULL;
	}
	ann = fann_create_shortcut_array(num_layers, layers);
	free(layers);
	return ann;
}

FANN_EXTERNAL Fann * FANN_API fann_create_shortcut_array(uint num_layers, const uint * layers)
{
	FannLayer * layer_it, * layer_it2, * last_layer;
	Fann * ann;
	FannNeuron * neuron_it, * neuron_it2 = 0;
	uint i;
	uint num_neurons_in, num_neurons_out;
#ifdef FIXEDFANN
	uint multiplier;
#endif
	fann_seed_rand();
	ann = fann_allocate_structure(num_layers);
	if(ann == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
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
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		// we do not allocate room here, but we make sure that
		// last_neuron - P_FirstNeuron is the number of neurons
		layer_it->P_FirstNeuron = NULL;
		layer_it->P_LastNeuron = layer_it->P_FirstNeuron + layers[i++];
		if(layer_it == ann->P_FirstLayer) {
			// there is a bias neuron in the first layer
			layer_it->P_LastNeuron++;
		}
		ann->total_neurons += (uint)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron);
	}
	ann->num_output = (uint)((ann->P_LastLayer-1)->P_LastNeuron - (ann->P_LastLayer-1)->P_FirstNeuron);
	ann->num_input = (uint)(ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron - 1);
	// allocate room for the actual neurons
	if(!ann->AllocateNeurons()) {
		fann_destroy(ann);
		return NULL;
	}
#ifdef DEBUG
	printf("creating fully shortcut connected network.\n");
	printf("input\n");
	printf("  layer       : %d neurons, 1 bias\n", (int)(ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron - 1));
#endif
	num_neurons_in = ann->num_input;
	last_layer = ann->P_LastLayer;
	for(layer_it = ann->P_FirstLayer + 1; layer_it != last_layer; layer_it++) {
		num_neurons_out = (uint)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron);
		// Now split out the connections on the different neurons
		for(i = 0; i != num_neurons_out; i++) {
			layer_it->P_FirstNeuron[i].first_con = ann->total_connections;
			ann->total_connections += num_neurons_in + 1;
			layer_it->P_FirstNeuron[i].last_con = ann->total_connections;
			layer_it->P_FirstNeuron[i].activation_function = FANN_SIGMOID_STEPWISE;
#ifdef FIXEDFANN
			layer_it->P_FirstNeuron[i].activation_steepness = ann->multiplier / 2;
#else
			layer_it->P_FirstNeuron[i].activation_steepness = 0.5;
#endif
		}
#ifdef DEBUG
		printf("  layer       : %d neurons, 0 bias\n", num_neurons_out);
#endif
		/* used in the next run of the loop */
		num_neurons_in += num_neurons_out;
	}
	fann_allocate_connections(ann);
	if(ann->IsError(FANN_E_CANT_ALLOCATE_MEM)) {
		fann_destroy(ann);
		return NULL;
	}
	// Connections are created from all neurons to all neurons in later layers
	num_neurons_in = ann->num_input + 1;
	for(layer_it = ann->P_FirstLayer + 1; layer_it != last_layer; layer_it++) {
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
			i = neuron_it->first_con;
			for(layer_it2 = ann->P_FirstLayer; layer_it2 != layer_it; layer_it2++) {
				for(neuron_it2 = layer_it2->P_FirstNeuron; neuron_it2 != layer_it2->P_LastNeuron; neuron_it2++) {
					ann->weights[i] = (fann_type)fann_random_weight();
					ann->connections[i] = neuron_it2;
					i++;
				}
			}
		}
		num_neurons_in += (uint)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron);
	}
#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}

FANN_EXTERNAL fann_type * FANN_API fann_run(Fann * ann, fann_type * input)
{
	FannNeuron * neuron_it, * last_neuron, * neurons, ** neuron_pointers;
	uint i, num_connections, num_input, num_output;
	fann_type neuron_sum, * output;
	fann_type * weights;
	FannLayer * layer_it, * last_layer;
	uint activation_function;
	fann_type steepness;
	/* store some variabels local for fast access */
	FannNeuron * first_neuron = ann->P_FirstLayer->P_FirstNeuron;
#ifdef FIXEDFANN
	int multiplier = ann->multiplier;
	uint decimal_point = ann->decimal_point;
	/* values used for the stepwise linear sigmoid function */
	fann_type r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;
	fann_type v1 = 0, v2 = 0, v3 = 0, v4 = 0, v5 = 0, v6 = 0;
	fann_type last_steepness = 0;
	uint last_activation_function = 0;
#else
	fann_type max_sum = 0;
#endif
	//
	// first set the input
	//
	num_input = ann->num_input;
	for(i = 0; i != num_input; i++) {
#ifdef FIXEDFANN
		if(fann_abs(input[i]) > multiplier) {
			printf("Warning input number %d is out of range -%d - %d with value %d, integer overflow may occur.\n", i, multiplier, multiplier, input[i]);
		}
#endif
		first_neuron[i].value = input[i];
	}
	//
	// Set the bias neuron in the input layer
	//
#ifdef FIXEDFANN
	(ann->P_FirstLayer->P_LastNeuron - 1)->value = multiplier;
#else
	(ann->P_FirstLayer->P_LastNeuron - 1)->value = 1;
#endif
	last_layer = ann->P_LastLayer;
	for(layer_it = ann->P_FirstLayer + 1; layer_it != last_layer; layer_it++) {
		last_neuron = layer_it->P_LastNeuron;
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
			if(neuron_it->first_con == neuron_it->last_con) {
				/* bias neurons */
#ifdef FIXEDFANN
				neuron_it->value = multiplier;
#else
				neuron_it->value = 1;
#endif
				continue;
			}
			activation_function = neuron_it->activation_function;
			steepness = neuron_it->activation_steepness;
			neuron_sum = 0;
			num_connections = neuron_it->last_con - neuron_it->first_con;
			weights = ann->weights + neuron_it->first_con;
			if(ann->connection_rate >= 1) {
				if(ann->network_type == FANN_NETTYPE_SHORTCUT) {
					neurons = ann->P_FirstLayer->P_FirstNeuron;
				}
				else {
					neurons = (layer_it - 1)->P_FirstNeuron;
				}
				// unrolled loop start
				i = num_connections & 3; // same as modulo 4
				switch(i) {
					case 3: neuron_sum += fann_mult(weights[2], neurons[2].value);
					case 2: neuron_sum += fann_mult(weights[1], neurons[1].value);
					case 1: neuron_sum += fann_mult(weights[0], neurons[0].value);
					case 0: break;
				}
				for(; i != num_connections; i += 4) {
					neuron_sum += fann_mult(weights[i], neurons[i].value) +
						fann_mult(weights[i + 1], neurons[i + 1].value) +
						fann_mult(weights[i + 2], neurons[i + 2].value) +
						fann_mult(weights[i + 3], neurons[i + 3].value);
				}
				/* unrolled loop end */

				/*
				 * for(i = 0;i != num_connections; i++){
				 * printf("%f += %f*%f, ", neuron_sum, weights[i], neurons[i].value);
				 * neuron_sum += fann_mult(weights[i], neurons[i].value);
				 * }
				 */
			}
			else{
				neuron_pointers = ann->connections + neuron_it->first_con;
				i = num_connections & 3;        /* same as modulo 4 */
				switch(i) {
					case 3: neuron_sum += fann_mult(weights[2], neuron_pointers[2]->value);
					case 2: neuron_sum += fann_mult(weights[1], neuron_pointers[1]->value);
					case 1: neuron_sum += fann_mult(weights[0], neuron_pointers[0]->value);
					case 0: break;
				}
				for(; i != num_connections; i += 4) {
					neuron_sum +=
					    fann_mult(weights[i], neuron_pointers[i]->value) +
					    fann_mult(weights[i + 1], neuron_pointers[i + 1]->value) +
					    fann_mult(weights[i + 2], neuron_pointers[i + 2]->value) +
					    fann_mult(weights[i + 3], neuron_pointers[i + 3]->value);
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
				    neuron_it->value =
				    (fann_type)fann_stepwise(v1, v2, v3, v4, v5, v6, r1, r2, r3, r4, r5, r6, 0,
				    multiplier, neuron_sum);
				    break;
				case FANN_SIGMOID_SYMMETRIC:
				case FANN_SIGMOID_SYMMETRIC_STEPWISE:
				    neuron_it->value = (fann_type)fann_stepwise(v1, v2, v3, v4, v5, v6, r1, r2, r3, r4, r5, r6,
						-multiplier, multiplier, neuron_sum);
				    break;
				case FANN_THRESHOLD:
				    neuron_it->value = (fann_type)((neuron_sum < 0) ? 0 : multiplier);
				    break;
				case FANN_THRESHOLD_SYMMETRIC:
				    neuron_it->value = (fann_type)((neuron_sum < 0) ? -multiplier : multiplier);
				    break;
				case FANN_LINEAR:
				    neuron_it->value = neuron_sum;
				    break;
				case FANN_LINEAR_PIECE:
				    neuron_it->value =
				    (fann_type)((neuron_sum < 0) ? 0 : (neuron_sum > multiplier) ? multiplier : neuron_sum);
				    break;
				case FANN_LINEAR_PIECE_SYMMETRIC:
				    neuron_it->value =
				    (fann_type)((neuron_sum < -multiplier) ? -multiplier : (neuron_sum >
					    multiplier) ? multiplier : neuron_sum);
				    break;
				case FANN_ELLIOT:
				case FANN_ELLIOT_SYMMETRIC:
				case FANN_GAUSSIAN:
				case FANN_GAUSSIAN_SYMMETRIC:
				case FANN_GAUSSIAN_STEPWISE:
				case FANN_SIN_SYMMETRIC:
				case FANN_COS_SYMMETRIC:
				    fann_error((FannError*)ann, FANN_E_CANT_USE_ACTIVATION);
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
	neurons = (ann->P_LastLayer-1)->P_FirstNeuron;
	for(i = 0; i != num_output; i++) {
		output[i] = neurons[i].value;
	}
	return ann->output;
}

FANN_EXTERNAL void FANN_API fann_randomize_weights(Fann * ann, fann_type min_weight, fann_type max_weight)
{
	fann_type * weights = ann->weights;
	fann_type * last_weight = weights + ann->total_connections;
	for(; weights != last_weight; weights++) {
		*weights = (fann_type)(fann_rand(min_weight, max_weight));
	}
#ifndef FIXEDFANN
	if(ann->prev_train_slopes != NULL) {
		fann_clear_train_arrays(ann);
	}
#endif
}

int Fann::IsEqual(const Fann & rS, long flags) const
{
	int    yes = 1;
#define CMPF(f) THROW(f == rS.f)
#define CMPA(a) { for(uint i = 0; i < SIZEOFARRAY(a); i++) { THROW(a[i] == rS.a[i]); } }
	THROW(Err.IsEqual(rS.Err));
	CMPF(learning_rate);
	CMPF(learning_momentum);
	CMPF(connection_rate);
	CMPF(network_type);
	CMPF(total_neurons);
	CMPF(num_input);
	CMPF(num_output);
	CMPF(training_algorithm);
#ifdef FIXEDFANN
	CMPF(decimal_point);
	CMPF(multiplier);
	CMPA(sigmoid_results);
	CMPA(sigmoid_values);
	CMPA(sigmoid_symmetric_results);
	CMPA(sigmoid_symmetric_values);
#endif
	CMPF(total_connections);
	CMPF(num_MSE);
	CMPF(MSE_value);
	CMPF(num_bit_fail);
	CMPF(bit_fail_limit);
	CMPF(train_error_function);
	CMPF(train_stop_function);

	CMPF(cascade_output_change_fraction);
	CMPF(cascade_output_stagnation_epochs);
	CMPF(cascade_candidate_change_fraction);
	CMPF(cascade_candidate_stagnation_epochs);
	CMPF(cascade_best_candidate);
	CMPF(cascade_candidate_limit);
	CMPF(cascade_weight_multiplier);
	CMPF(cascade_max_out_epochs);
	CMPF(cascade_max_cand_epochs);
	CMPF(cascade_min_out_epochs);
	CMPF(cascade_min_cand_epochs);

	CMPF(cascade_activation_functions_count);
	CMPF(cascade_activation_steepnesses_count);
	CMPF(cascade_num_candidate_groups);
	CMPF(total_neurons_allocated);
	CMPF(total_connections_allocated);
	CMPF(quickprop_decay);
	CMPF(quickprop_mu);
	CMPF(rprop_increase_factor);
	CMPF(rprop_decrease_factor);
	CMPF(rprop_delta_min);
	CMPF(rprop_delta_max);
	CMPF(rprop_delta_zero);
	CMPF(sarprop_weight_decay_shift);
	CMPF(sarprop_step_error_threshold_factor);
	CMPF(sarprop_step_error_shift);
	CMPF(sarprop_temperature);
	CMPF(sarprop_epoch);
	{
		for(uint i = 0; i < cascade_activation_functions_count; i++) {
			CMPF(cascade_activation_functions[i]);
		}
	}
	{
		for(uint i = 0; i < cascade_activation_steepnesses_count; i++) {
			CMPF(cascade_activation_steepnesses[i]);
		}
	}

#if 0 // {
	FannLayer * P_FirstLayer;
	FannLayer * P_LastLayer;
	fann_type * weights;
	FannNeuron ** connections;
	fann_type * train_errors;
	fann_type * output;
	fann_callback_type callback;
	void * user_data;
	// enum fann_activationfunc_enum * cascade_activation_functions;
	fann_type * cascade_activation_steepnesses;
	fann_type * cascade_candidate_scores;
	fann_type * train_slopes;
	fann_type * prev_steps;
	fann_type * prev_train_slopes;
	fann_type * prev_weights_deltas;
#ifndef FIXEDFANN
	float * scale_mean_in;
	float * scale_deviation_in;
	float * scale_new_min_in;
	float * scale_factor_in;
	float * scale_mean_out;
	float * scale_deviation_out;
	float * scale_new_min_out;
	float * scale_factor_out;
#endif
#endif // } 0
	CATCH
		yes = 0;
	ENDCATCH
#undef CMPF
	return yes;
}

int Fann::Copy(const Fann & rS)
{
	EXCEPTVAR(*(int *)Err.errno_f);
	int    ok = 1;
	//uint   _num_layers = (uint)(rS.last_layer - rS.P_FirstLayer);
	uint   _layer_size;
	uint   i;
	uint   _input_neuron;
	FannLayer * p_orig_layer_it = 0;
	FannLayer * p_copy_layer_it = 0;
	FannNeuron * p_last_neuron = 0;
	FannNeuron * p_orig_neuron_it = 0;
	FannNeuron * p_copy_neuron_it = 0;
	FannNeuron * p_orig_first_neuron = 0;
	FannNeuron * p_copy_first_neuron = 0;
	Err = rS.Err;
	learning_rate = rS.learning_rate;
	learning_momentum = rS.learning_momentum;
	connection_rate = rS.connection_rate;
	network_type = rS.network_type;
	num_MSE = rS.num_MSE;
	MSE_value = rS.MSE_value;
	num_bit_fail = rS.num_bit_fail;
	bit_fail_limit = rS.bit_fail_limit;
	train_error_function = rS.train_error_function;
	train_stop_function = rS.train_stop_function;
	training_algorithm = rS.training_algorithm;
	callback = rS.callback;
	user_data = rS.user_data;
#ifndef FIXEDFANN // {
	cascade_output_change_fraction = rS.cascade_output_change_fraction;
	cascade_output_stagnation_epochs = rS.cascade_output_stagnation_epochs;
	cascade_candidate_change_fraction = rS.cascade_candidate_change_fraction;
	cascade_candidate_stagnation_epochs = rS.cascade_candidate_stagnation_epochs;
	cascade_best_candidate = rS.cascade_best_candidate;
	cascade_candidate_limit = rS.cascade_candidate_limit;
	cascade_weight_multiplier = rS.cascade_weight_multiplier;
	cascade_max_out_epochs = rS.cascade_max_out_epochs;
	cascade_max_cand_epochs = rS.cascade_max_cand_epochs;
	// copy cascade activation functions
	cascade_activation_functions_count = rS.cascade_activation_functions_count;
	cascade_activation_functions = (enum fann_activationfunc_enum*)realloc(cascade_activation_functions,
	    cascade_activation_functions_count * sizeof(enum fann_activationfunc_enum));
	THROW_V(cascade_activation_functions, FANN_E_CANT_ALLOCATE_MEM);
	memcpy(cascade_activation_functions, rS.cascade_activation_functions,
	    cascade_activation_functions_count * sizeof(enum fann_activationfunc_enum));
	// copy cascade activation steepnesses
	cascade_activation_steepnesses_count = rS.cascade_activation_steepnesses_count;
	cascade_activation_steepnesses = (fann_type*)realloc(cascade_activation_steepnesses,
	    cascade_activation_steepnesses_count * sizeof(fann_type));
	THROW_V(cascade_activation_steepnesses, FANN_E_CANT_ALLOCATE_MEM);
	memcpy(cascade_activation_steepnesses, rS.cascade_activation_steepnesses,
	    cascade_activation_steepnesses_count * sizeof(fann_type));
	cascade_num_candidate_groups = rS.cascade_num_candidate_groups;
	// copy candidate scores, if used
	if(rS.cascade_candidate_scores == NULL) {
		cascade_candidate_scores = NULL;
	}
	else {
		cascade_candidate_scores = (fann_type*)malloc(fann_get_cascade_num_candidates(this) * sizeof(fann_type));
		THROW_V(cascade_candidate_scores, FANN_E_CANT_ALLOCATE_MEM);
		memcpy(cascade_candidate_scores, rS.cascade_candidate_scores, fann_get_cascade_num_candidates(this) * sizeof(fann_type));
	}
#endif // } FIXEDFANN
	quickprop_decay = rS.quickprop_decay;
	quickprop_mu = rS.quickprop_mu;
	rprop_increase_factor = rS.rprop_increase_factor;
	rprop_decrease_factor = rS.rprop_decrease_factor;
	rprop_delta_min = rS.rprop_delta_min;
	rprop_delta_max = rS.rprop_delta_max;
	rprop_delta_zero = rS.rprop_delta_zero;
	// user_data is not deep copied.  user should use fann_copy_with_user_data() for that
	user_data = rS.user_data;
#ifdef FIXEDFANN
	decimal_point = rS.decimal_point;
	multiplier = rS.multiplier;
	memcpy(sigmoid_results, rS.sigmoid_results, 6*sizeof(fann_type));
	memcpy(sigmoid_values, rS.sigmoid_values, 6*sizeof(fann_type));
	memcpy(sigmoid_symmetric_results, rS.sigmoid_symmetric_results, 6*sizeof(fann_type));
	memcpy(sigmoid_symmetric_values, rS.sigmoid_symmetric_values, 6*sizeof(fann_type));
#endif
	// copy layer sizes, prepare for fann_allocate_neurons
	for(p_orig_layer_it = rS.P_FirstLayer, p_copy_layer_it = P_FirstLayer; p_orig_layer_it != rS.P_LastLayer; p_orig_layer_it++, p_copy_layer_it++) {
		_layer_size = (uint)(p_orig_layer_it->P_LastNeuron - p_orig_layer_it->P_FirstNeuron);
		p_copy_layer_it->P_FirstNeuron = NULL;
		p_copy_layer_it->P_LastNeuron = p_copy_layer_it->P_FirstNeuron + _layer_size;
		total_neurons += _layer_size;
	}
	num_input = rS.num_input;
	num_output = rS.num_output;
	// copy scale parameters, when used
#ifndef FIXEDFANN
	if(rS.scale_mean_in) {
		fann_allocate_scale(this);
		for(i = 0; i < rS.num_input; i++) {
			scale_mean_in[i] = rS.scale_mean_in[i];
			scale_deviation_in[i] = rS.scale_deviation_in[i];
			scale_new_min_in[i] = rS.scale_new_min_in[i];
			scale_factor_in[i] = rS.scale_factor_in[i];
		}
		for(i = 0; i < rS.num_output; i++) {
			scale_mean_out[i] = rS.scale_mean_out[i];
			scale_deviation_out[i] = rS.scale_deviation_out[i];
			scale_new_min_out[i] = rS.scale_new_min_out[i];
			scale_factor_out[i] = rS.scale_factor_out[i];
		}
	}
#endif
	// copy the neurons
	THROW(AllocateNeurons());
	_layer_size = (uint)((rS.P_LastLayer-1)->P_LastNeuron - (rS.P_LastLayer-1)->P_FirstNeuron);
	memcpy(output, rS.output, _layer_size * sizeof(fann_type));
	p_last_neuron = (rS.P_LastLayer-1)->P_LastNeuron;
	for(p_orig_neuron_it = rS.P_FirstLayer->P_FirstNeuron, p_copy_neuron_it = P_FirstLayer->P_FirstNeuron;
	    p_orig_neuron_it != p_last_neuron; p_orig_neuron_it++, p_copy_neuron_it++) {
		memcpy(p_copy_neuron_it, p_orig_neuron_it, sizeof(FannNeuron));
	}
	// copy the connections
	total_connections = rS.total_connections;
	fann_allocate_connections(this);
	THROW(!IsError(FANN_E_CANT_ALLOCATE_MEM));
	p_orig_first_neuron = rS.P_FirstLayer->P_FirstNeuron;
	p_copy_first_neuron = P_FirstLayer->P_FirstNeuron;
	for(i = 0; i < rS.total_connections; i++) {
		weights[i] = rS.weights[i];
		_input_neuron = (uint)(rS.connections[i] - p_orig_first_neuron);
		connections[i] = p_copy_first_neuron + _input_neuron;
	}
	if(rS.train_slopes) {
		train_slopes = (fann_type*)malloc(total_connections_allocated * sizeof(fann_type));
		THROW_V(train_slopes, FANN_E_CANT_ALLOCATE_MEM);
		memcpy(train_slopes, rS.train_slopes, total_connections_allocated * sizeof(fann_type));
	}
	if(rS.prev_steps) {
		prev_steps = (fann_type*)malloc(total_connections_allocated * sizeof(fann_type));
		THROW_V(prev_steps, FANN_E_CANT_ALLOCATE_MEM);
		memcpy(prev_steps, rS.prev_steps, total_connections_allocated * sizeof(fann_type));
	}
	if(rS.prev_train_slopes) {
		prev_train_slopes = (fann_type*)malloc(total_connections_allocated * sizeof(fann_type));
		THROW_V(prev_train_slopes, FANN_E_CANT_ALLOCATE_MEM);
		memcpy(prev_train_slopes, rS.prev_train_slopes, total_connections_allocated * sizeof(fann_type));
	}
	if(rS.prev_weights_deltas) {
		prev_weights_deltas = (fann_type*)malloc(total_connections_allocated * sizeof(fann_type));
		THROW_V(prev_weights_deltas, FANN_E_CANT_ALLOCATE_MEM);
		memcpy(prev_weights_deltas, rS.prev_weights_deltas, total_connections_allocated * sizeof(fann_type));
	}
	CATCH
		fann_error(&Err, Err.errno_f);
	ENDCATCH
	return ok;
}
//
// deep copy of the fann structure
//
FANN_EXTERNAL Fann * FANN_API fann_copy(const Fann * pOrig)
{
	Fann * p_copy = 0;
	if(pOrig) {
		p_copy = new Fann(pOrig->GetNumLayers());
		if(p_copy->IsError()) {
			ZDELETE(p_copy);
		}
		else {
			p_copy->Copy(*pOrig);
		}
	}
	return p_copy;
#if 0 // old code {
	uint   num_layers = (uint)(pOrig->last_layer - pOrig->P_FirstLayer);
	FannLayer * orig_layer_it;
	FannLayer * copy_layer_it;
	uint   layer_size;
	FannNeuron * last_neuron;
	FannNeuron * orig_neuron_it;
	FannNeuron * copy_neuron_it;
	uint   i;
	FannNeuron * orig_first_neuron;
	FannNeuron * copy_first_neuron;
	uint   input_neuron;
	Fann * copy = fann_allocate_structure(num_layers);
	if(copy == NULL) {
		fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	copy->Err = pOrig->Err;
	copy->learning_rate = pOrig->learning_rate;
	copy->learning_momentum = pOrig->learning_momentum;
	copy->connection_rate = pOrig->connection_rate;
	copy->network_type = pOrig->network_type;
	copy->num_MSE = pOrig->num_MSE;
	copy->MSE_value = pOrig->MSE_value;
	copy->num_bit_fail = pOrig->num_bit_fail;
	copy->bit_fail_limit = pOrig->bit_fail_limit;
	copy->train_error_function = pOrig->train_error_function;
	copy->train_stop_function = pOrig->train_stop_function;
	copy->training_algorithm = pOrig->training_algorithm;
	copy->callback = pOrig->callback;
	copy->user_data = pOrig->user_data;
#ifndef FIXEDFANN
	copy->cascade_output_change_fraction = pOrig->cascade_output_change_fraction;
	copy->cascade_output_stagnation_epochs = pOrig->cascade_output_stagnation_epochs;
	copy->cascade_candidate_change_fraction = pOrig->cascade_candidate_change_fraction;
	copy->cascade_candidate_stagnation_epochs = pOrig->cascade_candidate_stagnation_epochs;
	copy->cascade_best_candidate = pOrig->cascade_best_candidate;
	copy->cascade_candidate_limit = pOrig->cascade_candidate_limit;
	copy->cascade_weight_multiplier = pOrig->cascade_weight_multiplier;
	copy->cascade_max_out_epochs = pOrig->cascade_max_out_epochs;
	copy->cascade_max_cand_epochs = pOrig->cascade_max_cand_epochs;
	// copy cascade activation functions
	copy->cascade_activation_functions_count = pOrig->cascade_activation_functions_count;
	copy->cascade_activation_functions = (enum fann_activationfunc_enum*)realloc(copy->cascade_activation_functions,
	    copy->cascade_activation_functions_count * sizeof(enum fann_activationfunc_enum));
	if(copy->cascade_activation_functions == NULL) {
		fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(copy);
		return NULL;
	}
	memcpy(copy->cascade_activation_functions, pOrig->cascade_activation_functions,
	    copy->cascade_activation_functions_count * sizeof(enum fann_activationfunc_enum));
	// copy cascade activation steepnesses
	copy->cascade_activation_steepnesses_count = pOrig->cascade_activation_steepnesses_count;
	copy->cascade_activation_steepnesses = (fann_type*)realloc(copy->cascade_activation_steepnesses,
	    copy->cascade_activation_steepnesses_count * sizeof(fann_type));
	if(copy->cascade_activation_steepnesses == NULL) {
		fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(copy);
		return NULL;
	}
	memcpy(copy->cascade_activation_steepnesses, pOrig->cascade_activation_steepnesses,
	    copy->cascade_activation_steepnesses_count * sizeof(fann_type));
	copy->cascade_num_candidate_groups = pOrig->cascade_num_candidate_groups;
	// copy candidate scores, if used
	if(pOrig->cascade_candidate_scores == NULL) {
		copy->cascade_candidate_scores = NULL;
	}
	else {
		copy->cascade_candidate_scores = (fann_type*)malloc(fann_get_cascade_num_candidates(copy) * sizeof(fann_type));
		if(copy->cascade_candidate_scores == NULL) {
			fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->cascade_candidate_scores, pOrig->cascade_candidate_scores, fann_get_cascade_num_candidates(copy) *
		    sizeof(fann_type));
	}
#endif /* FIXEDFANN */
	copy->quickprop_decay = pOrig->quickprop_decay;
	copy->quickprop_mu = pOrig->quickprop_mu;
	copy->rprop_increase_factor = pOrig->rprop_increase_factor;
	copy->rprop_decrease_factor = pOrig->rprop_decrease_factor;
	copy->rprop_delta_min = pOrig->rprop_delta_min;
	copy->rprop_delta_max = pOrig->rprop_delta_max;
	copy->rprop_delta_zero = pOrig->rprop_delta_zero;
	// user_data is not deep copied.  user should use fann_copy_with_user_data() for that
	copy->user_data = pOrig->user_data;
#ifdef FIXEDFANN
	copy->decimal_point = pOrig->decimal_point;
	copy->multiplier = pOrig->multiplier;
	memcpy(copy->sigmoid_results, pOrig->sigmoid_results, 6*sizeof(fann_type));
	memcpy(copy->sigmoid_values, pOrig->sigmoid_values, 6*sizeof(fann_type));
	memcpy(copy->sigmoid_symmetric_results, pOrig->sigmoid_symmetric_results, 6*sizeof(fann_type));
	memcpy(copy->sigmoid_symmetric_values, pOrig->sigmoid_symmetric_values, 6*sizeof(fann_type));
#endif
	// copy layer sizes, prepare for fann_allocate_neurons
	for(orig_layer_it = pOrig->P_FirstLayer, copy_layer_it = copy->P_FirstLayer; orig_layer_it != pOrig->last_layer; orig_layer_it++, copy_layer_it++) {
		layer_size = (uint)(orig_layer_it->P_LastNeuron - orig_layer_it->P_FirstNeuron);
		copy_layer_it->P_FirstNeuron = NULL;
		copy_layer_it->P_LastNeuron = copy_layer_it->P_FirstNeuron + layer_size;
		copy->total_neurons += layer_size;
	}
	copy->num_input = pOrig->num_input;
	copy->num_output = pOrig->num_output;
	// copy scale parameters, when used
#ifndef FIXEDFANN
	if(pOrig->scale_mean_in != NULL) {
		fann_allocate_scale(copy);
		for(i = 0; i < pOrig->num_input; i++) {
			copy->scale_mean_in[i] = pOrig->scale_mean_in[i];
			copy->scale_deviation_in[i] = pOrig->scale_deviation_in[i];
			copy->scale_new_min_in[i] = pOrig->scale_new_min_in[i];
			copy->scale_factor_in[i] = pOrig->scale_factor_in[i];
		}
		for(i = 0; i < pOrig->num_output; i++) {
			copy->scale_mean_out[i] = pOrig->scale_mean_out[i];
			copy->scale_deviation_out[i] = pOrig->scale_deviation_out[i];
			copy->scale_new_min_out[i] = pOrig->scale_new_min_out[i];
			copy->scale_factor_out[i] = pOrig->scale_factor_out[i];
		}
	}
#endif
	// copy the neurons
	if(!copy->AllocateNeurons()) {
		fann_destroy(copy);
		return NULL;
	}
	layer_size = (uint)((pOrig->last_layer-1)->P_LastNeuron - (pOrig->last_layer-1)->P_FirstNeuron);
	memcpy(copy->output, pOrig->output, layer_size * sizeof(fann_type));

	last_neuron = (pOrig->last_layer - 1)->P_LastNeuron;
	for(orig_neuron_it = pOrig->first_layer->P_FirstNeuron, copy_neuron_it = copy->first_layer->P_FirstNeuron;
	    orig_neuron_it != last_neuron; orig_neuron_it++, copy_neuron_it++) {
		memcpy(copy_neuron_it, orig_neuron_it, sizeof(FannNeuron));
	}
	// copy the connections
	copy->total_connections = pOrig->total_connections;
	fann_allocate_connections(copy);
	if(copy->IsError(FANN_E_CANT_ALLOCATE_MEM)) {
		fann_destroy(copy);
		return NULL;
	}
	orig_first_neuron = pOrig->first_layer->P_FirstNeuron;
	copy_first_neuron = copy->first_layer->P_FirstNeuron;
	for(i = 0; i < pOrig->total_connections; i++) {
		copy->weights[i] = pOrig->weights[i];
		input_neuron = (uint)(pOrig->connections[i] - orig_first_neuron);
		copy->connections[i] = copy_first_neuron + input_neuron;
	}
	if(pOrig->train_slopes) {
		copy->train_slopes = (fann_type*)malloc(copy->total_connections_allocated * sizeof(fann_type));
		if(copy->train_slopes == NULL) {
			fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->train_slopes, pOrig->train_slopes, copy->total_connections_allocated * sizeof(fann_type));
	}
	if(pOrig->prev_steps) {
		copy->prev_steps = (fann_type*)malloc(copy->total_connections_allocated * sizeof(fann_type));
		if(copy->prev_steps == NULL) {
			fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->prev_steps, pOrig->prev_steps, copy->total_connections_allocated * sizeof(fann_type));
	}
	if(pOrig->prev_train_slopes) {
		copy->prev_train_slopes = (fann_type*)malloc(copy->total_connections_allocated * sizeof(fann_type));
		if(copy->prev_train_slopes == NULL) {
			fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->prev_train_slopes, pOrig->prev_train_slopes, copy->total_connections_allocated * sizeof(fann_type));
	}
	if(pOrig->prev_weights_deltas) {
		copy->prev_weights_deltas = (fann_type*)malloc(copy->total_connections_allocated * sizeof(fann_type));
		if(copy->prev_weights_deltas == NULL) {
			fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->prev_weights_deltas, pOrig->prev_weights_deltas, copy->total_connections_allocated * sizeof(fann_type));
	}
	return copy;
#endif // } old code
}

FANN_EXTERNAL void FANN_API fann_print_connections(Fann * ann)
{
	FannLayer * layer_it;
	FannNeuron * neuron_it;
	uint   i;
	int    value;
	uint   num_neurons = fann_get_total_neurons(ann) - fann_get_num_output(ann);
	char * neurons = (char*)malloc(num_neurons + 1);
	if(neurons == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
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
			for(i = neuron_it->first_con; i < neuron_it->last_con; i++) {
				if(ann->weights[i] < 0) {
#ifdef FIXEDFANN
					value = (int)((ann->weights[i] / (double)ann->multiplier) - 0.5);
#else
					value = (int)((ann->weights[i]) - 0.5);
#endif
					if(value < -25)
						value = -25;
					neurons[ann->connections[i] - ann->P_FirstLayer->P_FirstNeuron] = (char)('a' - value);
				}
				else{
#ifdef FIXEDFANN
					value = (int)((ann->weights[i] / (double)ann->multiplier) + 0.5);
#else
					value = (int)((ann->weights[i]) + 0.5);
#endif
					if(value > 25)
						value = 25;
					neurons[ann->connections[i] - ann->P_FirstLayer->P_FirstNeuron] = (char)('A' + value);
				}
			}
			printf("L %3d / N %4d %s\n", (int)(layer_it - ann->P_FirstLayer),
			    (int)(neuron_it - ann->P_FirstLayer->P_FirstNeuron), neurons);
		}
	}
	free(neurons);
}
//
// Initialize the weights using Widrow + Nguyen's algorithm.
//
FANN_EXTERNAL void FANN_API fann_init_weights(Fann * ann, FannTrainData * train_data)
{
	fann_type smallest_inp, largest_inp;
	uint dat = 0, elem, num_connect, num_hidden_neurons;
	FannLayer * layer_it;
	FannNeuron * neuron_it, * last_neuron, * bias_neuron;
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
	num_hidden_neurons = (uint)(ann->total_neurons - (ann->num_input + ann->num_output + (ann->P_LastLayer - ann->P_FirstLayer)));
	scale_factor = (float)(pow((double)(0.7f * (double)num_hidden_neurons),
		(double)(1.0f / (double)ann->num_input)) / (double)(largest_inp - smallest_inp));
#ifdef DEBUG
	printf("Initializing weights with scale factor %f\n", scale_factor);
#endif
	bias_neuron = ann->P_FirstLayer->P_LastNeuron - 1;
	for(layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer; layer_it++) {
		last_neuron = layer_it->P_LastNeuron;
		if(ann->network_type == FANN_NETTYPE_LAYER) {
			bias_neuron = (layer_it - 1)->P_LastNeuron - 1;
		}
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
			for(num_connect = neuron_it->first_con; num_connect < neuron_it->last_con; num_connect++) {
				if(bias_neuron == ann->connections[num_connect]) {
#ifdef FIXEDFANN
					ann->weights[num_connect] = (fann_type)fann_rand(-scale_factor, scale_factor * multiplier);
#else
					ann->weights[num_connect] = (fann_type)fann_rand(-scale_factor, scale_factor);
#endif
				}
				else {
#ifdef FIXEDFANN
					ann->weights[num_connect] = (fann_type)fann_rand(0, scale_factor * multiplier);
#else
					ann->weights[num_connect] = (fann_type)fann_rand(0, scale_factor);
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

FANN_EXTERNAL void FANN_API fann_print_parameters(Fann * ann)
{
	FannLayer * layer_it;
#ifndef FIXEDFANN
	uint i;
#endif
	printf("Input layer                          :%4d neurons, 1 bias\n", ann->num_input);
	for(layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer-1; layer_it++) {
		if(ann->network_type == FANN_NETTYPE_SHORTCUT) {
			printf("  Hidden layer                       :%4d neurons, 0 bias\n",
			    (int)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron));
		}
		else {
			printf("  Hidden layer                       :%4d neurons, 1 bias\n",
			    (int)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron - 1));
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

FANN_EXTERNAL uint FANN_API fann_get_total_neurons(Fann * ann)
{
	if(ann->network_type) {
		return ann->total_neurons;
	}
	else {
		return (ann->total_neurons - 1); // -1, because there is always an unused bias neuron in the last layer
	}
}

FANN_GET(uint, total_connections)

FANN_EXTERNAL enum fann_nettype_enum FANN_API fann_get_network_type(Fann * ann)
{
	// Currently two types: LAYER = 0, SHORTCUT = 1
	// Enum network_types must be set to match the return values
	return ann->network_type;
}

FANN_EXTERNAL float FANN_API fann_get_connection_rate(Fann * ann)
{
	return ann->connection_rate;
}

FANN_EXTERNAL uint FANN_API fann_get_num_layers(Fann * pAnn)
{
	return pAnn ? pAnn->GetNumLayers() : 0;
}

FANN_EXTERNAL void FANN_API fann_get_layer_array(Fann * ann, uint * layers)
{
	for(FannLayer * layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		uint count = (uint)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron);
		// Remove the bias from the count of neurons
		switch(fann_get_network_type(ann)) {
			case FANN_NETTYPE_LAYER: {
			    --count;
			    break;
		    }
			case FANN_NETTYPE_SHORTCUT: {
			    /* The bias in the first layer is reused for all layers */
			    if(layer_it == ann->P_FirstLayer)
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

FANN_EXTERNAL void FANN_API fann_get_bias_array(Fann * ann, uint * bias)
{
	for(FannLayer * layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; ++layer_it, ++bias) {
		switch(fann_get_network_type(ann)) {
			case FANN_NETTYPE_LAYER:
			    // Report one bias in each layer except the last
			    *bias = (layer_it != (ann->P_LastLayer-1)) ? 1 : 0;
			    break;
			case FANN_NETTYPE_SHORTCUT:
			    // The bias in the first layer is reused for all layers
			    *bias = (layer_it == ann->P_FirstLayer) ? 1 : 0;
			    break;
			default:
			    // Unknown network type, assume no bias present
			    *bias = 0;
			    break;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_get_connection_array(Fann * ann, struct fann_connection * connections)
{
	FannLayer * layer_it;
	FannNeuron * neuron_it;
	uint idx;
	uint source_index = 0;
	uint destination_index = 0;
	FannNeuron * first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	/* The following assumes that the last unused bias has no connections */
	/* for each layer */
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		/* for each neuron */
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
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

FANN_EXTERNAL void FANN_API fann_set_weight_array(Fann * ann, struct fann_connection * connections, uint num_connections)
{
	for(uint idx = 0; idx < num_connections; idx++) {
		fann_set_weight(ann, connections[idx].from_neuron, connections[idx].to_neuron, connections[idx].weight);
	}
}

FANN_EXTERNAL void FANN_API fann_set_weight(Fann * ann, uint from_neuron, uint to_neuron, fann_type weight)
{
	FannLayer * layer_it;
	FannNeuron * neuron_it;
	uint idx;
	uint source_index = 0;
	uint destination_index = 0;
	FannNeuron * first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	/* Find the connection, simple brute force search through the network
	   for one or more connections that match to minimize datastructure dependencies.
	   Nothing is done if the connection does not already exist in the network. */

	/* for each layer */
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		/* for each neuron */
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
			/* for each connection */
			for(idx = neuron_it->first_con; idx < neuron_it->last_con; idx++) {
				/* If the source and destination neurons match, assign the weight */
				if(((int)from_neuron == ann->connections[source_index] - first_neuron) && (to_neuron == destination_index)) {
					ann->weights[source_index] = weight;
				}
				source_index++;
			}
			destination_index++;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_get_weights(Fann * ann, fann_type * weights)
{
	memcpy(weights, ann->weights, sizeof(fann_type)*ann->total_connections);
}

FANN_EXTERNAL void FANN_API fann_set_weights(Fann * ann, fann_type * weights)
{
	memcpy(ann->weights, weights, sizeof(fann_type)*ann->total_connections);
}

FANN_GET_SET(void *, user_data)

#ifdef FIXEDFANN

FANN_GET(uint, decimal_point)
FANN_GET(uint, multiplier)

/* INTERNAL FUNCTION
   Adjust the steepwise functions (if used)
 */
void fann_update_stepwise(struct fann * ann)
{
	//
	// Calculate the parameters for the stepwise linear
	// sigmoid function fixed point.
	// Using a rewritten sigmoid function.
	// results 0.005, 0.05, 0.25, 0.75, 0.95, 0.995
	//
	ann->sigmoid_results[0] = MAX((fann_type)(ann->multiplier / 200.0 + 0.5), 1);
	ann->sigmoid_results[1] = MAX((fann_type)(ann->multiplier / 20.0 + 0.5), 1);
	ann->sigmoid_results[2] = MAX((fann_type)(ann->multiplier / 4.0 + 0.5), 1);
	ann->sigmoid_results[3] = MIN(ann->multiplier - (fann_type)(ann->multiplier / 4.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_results[4] = MIN(ann->multiplier - (fann_type)(ann->multiplier / 20.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_results[5] = MIN(ann->multiplier - (fann_type)(ann->multiplier / 200.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_symmetric_results[0] = MAX((fann_type)((ann->multiplier / 100.0) - ann->multiplier - 0.5), (fann_type)(1 - (fann_type)ann->multiplier));
	ann->sigmoid_symmetric_results[1] = MAX((fann_type)((ann->multiplier / 10.0) - ann->multiplier - 0.5), (fann_type)(1 - (fann_type)ann->multiplier));
	ann->sigmoid_symmetric_results[2] = MAX((fann_type)((ann->multiplier / 2.0) - ann->multiplier - 0.5), (fann_type)(1 - (fann_type)ann->multiplier));
	ann->sigmoid_symmetric_results[3] = MIN(ann->multiplier - (fann_type)(ann->multiplier / 2.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_symmetric_results[4] = MIN(ann->multiplier - (fann_type)(ann->multiplier / 10.0 + 0.5), ann->multiplier - 1);
	ann->sigmoid_symmetric_results[5] = MIN(ann->multiplier - (fann_type)(ann->multiplier / 100.0 + 1.0), ann->multiplier - 1);
	for(uint i = 0; i < 6; i++) {
		ann->sigmoid_values[i] = (fann_type)(((log(ann->multiplier / (float)ann->sigmoid_results[i] - 1) * (float)ann->multiplier) / -2.0) * (float)ann->multiplier);
		ann->sigmoid_symmetric_values[i] = (fann_type)(((log((ann->multiplier - (float)ann->sigmoid_symmetric_results[i]) / ((float)ann->sigmoid_symmetric_results[i] + ann->multiplier)) * (float)ann->multiplier) / -2.0) * (float)ann->multiplier);
	}
}

#endif

FannError::FannError()
{
	errno_f = FANN_E_NO_ERROR;
	error_log = fann_default_error_log;
}

FannError & FannError::Copy(const FannError & rS)
{
	errno_f = rS.errno_f;
	error_log = rS.error_log;
	Msg = rS.Msg;
	return *this;
}

Fann::Fann(uint numLayers)
{
	learning_rate = 0.7f;
	learning_momentum = 0.0;
	total_neurons = 0;
	total_connections = 0;
	num_input = 0;
	num_output = 0;
	train_errors = NULL;
	train_slopes = NULL;
	prev_steps = NULL;
	prev_train_slopes = NULL;
	prev_weights_deltas = NULL;
	training_algorithm = FANN_TRAIN_RPROP;
	num_MSE = 0;
	MSE_value = 0;
	num_bit_fail = 0;
	bit_fail_limit = (fann_type)0.35;
	network_type = FANN_NETTYPE_LAYER;
	train_error_function = FANN_ERRORFUNC_TANH;
	train_stop_function = FANN_STOPFUNC_MSE;
	callback = NULL;
	user_data = NULL; /* User is responsible for deallocation */
	weights = NULL;
	connections = NULL;
	output = NULL;
#ifndef FIXEDFANN
	scale_mean_in = NULL;
	scale_deviation_in = NULL;
	scale_new_min_in = NULL;
	scale_factor_in = NULL;
	scale_mean_out = NULL;
	scale_deviation_out = NULL;
	scale_new_min_out = NULL;
	scale_factor_out = NULL;
#endif
	// variables used for cascade correlation (reasonable defaults)
	cascade_output_change_fraction = 0.01f;
	cascade_candidate_change_fraction = 0.01f;
	cascade_output_stagnation_epochs = 12;
	cascade_candidate_stagnation_epochs = 12;
	cascade_num_candidate_groups = 2;
	cascade_weight_multiplier = (fann_type)0.4;
	cascade_candidate_limit = (fann_type)1000.0;
	cascade_max_out_epochs = 150;
	cascade_max_cand_epochs = 150;
	cascade_min_out_epochs = 50;
	cascade_min_cand_epochs = 50;
	cascade_candidate_scores = NULL;
	cascade_activation_functions_count = 10;
	cascade_activation_functions = (enum fann_activationfunc_enum*)calloc(cascade_activation_functions_count, sizeof(enum fann_activationfunc_enum));
	if(cascade_activation_functions == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
	}
	else {
		cascade_activation_functions[0] = FANN_SIGMOID;
		cascade_activation_functions[1] = FANN_SIGMOID_SYMMETRIC;
		cascade_activation_functions[2] = FANN_GAUSSIAN;
		cascade_activation_functions[3] = FANN_GAUSSIAN_SYMMETRIC;
		cascade_activation_functions[4] = FANN_ELLIOT;
		cascade_activation_functions[5] = FANN_ELLIOT_SYMMETRIC;
		cascade_activation_functions[6] = FANN_SIN_SYMMETRIC;
		cascade_activation_functions[7] = FANN_COS_SYMMETRIC;
		cascade_activation_functions[8] = FANN_SIN;
		cascade_activation_functions[9] = FANN_COS;
		cascade_activation_steepnesses_count = 4;
		cascade_activation_steepnesses = (fann_type*)calloc(cascade_activation_steepnesses_count, sizeof(fann_type));
		if(cascade_activation_steepnesses == NULL) {
			ZFREE(cascade_activation_functions);
			fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		}
		else {
			cascade_activation_steepnesses[0] = (fann_type)0.25;
			cascade_activation_steepnesses[1] = (fann_type)0.5;
			cascade_activation_steepnesses[2] = (fann_type)0.75;
			cascade_activation_steepnesses[3] = (fann_type)1.0;
			// Variables for use with with Quickprop training (reasonable defaults)
			quickprop_decay = -0.0001f;
			quickprop_mu = 1.75;
			// Variables for use with with RPROP training (reasonable defaults)
			rprop_increase_factor = 1.2f;
			rprop_decrease_factor = 0.5;
			rprop_delta_min = 0.0;
			rprop_delta_max = 50.0;
			rprop_delta_zero = 0.1f;
			// Variables for use with SARPROP training (reasonable defaults)
			sarprop_weight_decay_shift = -6.644f;
			sarprop_step_error_threshold_factor = 0.1f;
			sarprop_step_error_shift = 1.385f;
			sarprop_temperature = 0.015f;
			sarprop_epoch = 0;
			fann_init_error_data((FannError*)this);
		#ifdef FIXEDFANN
			// these values are only boring defaults, and should really
			// never be used, since the real values are always loaded from a file.
			decimal_point = 8;
			multiplier = 256;
		#endif
			// allocate room for the layers
			P_FirstLayer = (FannLayer *)calloc(numLayers, sizeof(FannLayer));
			if(P_FirstLayer == NULL) {
				fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
			}
			else
				P_LastLayer = P_FirstLayer + numLayers;
		}
	}
}

Fann::~Fann()
{
	ZFREE(weights);
	ZFREE(connections);
	ZFREE(P_FirstLayer->P_FirstNeuron);
	ZFREE(P_FirstLayer);
	ZFREE(output);
	ZFREE(train_errors);
	ZFREE(train_slopes);
	ZFREE(prev_train_slopes);
	ZFREE(prev_steps);
	ZFREE(prev_weights_deltas);
	//ZFREE(errstr);
	ZFREE(cascade_activation_functions);
	ZFREE(cascade_activation_steepnesses);
	ZFREE(cascade_candidate_scores);
#ifndef FIXEDFANN
	ZFREE(scale_mean_in);
	ZFREE(scale_deviation_in);
	ZFREE(scale_new_min_in);
	ZFREE(scale_factor_in);
	ZFREE(scale_mean_out);
	ZFREE(scale_deviation_out);
	ZFREE(scale_new_min_out);
	ZFREE(scale_factor_out);
#endif
	//ZFREE(ann);
}

FANN_EXTERNAL void FANN_API fann_destroy(Fann * pAnn)
{
	ZDELETE(pAnn);
}
//
// INTERNAL FUNCTION
// Allocates the main structure and sets some default values.
//
Fann * fann_allocate_structure(uint num_layers)
{
	if(num_layers < 2) {
#ifdef DEBUG
		printf("less than 2 layers - ABORTING.\n");
#endif
		return NULL;
	}
	else {
		Fann * p_ann = new Fann(num_layers);
		if(p_ann->IsError())
			ZDELETE(p_ann);
		return p_ann;
	}
}
//
// INTERNAL FUNCTION
// Allocates room for the scaling parameters.
//
int fann_allocate_scale(Fann * ann)
{
	/* todo this should only be allocated when needed */
#ifndef FIXEDFANN
	uint i = 0;
#define SCALE_ALLOCATE(what, where, default_value)				      \
	ann->what ## _ ## where = (float*)calloc(ann->num_ ## where ## put, sizeof( float )); \
	if(ann->what ## _ ## where == NULL) { \
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);				      \
		fann_destroy(ann);							      \
		return 1;														\
	}																	\
	for(i = 0; i < ann->num_ ## where ## put; i++)						  \
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

int Fann::AllocateNeurons()
{
	int    ok = 1;
	EXCEPTVAR(*(int *)Err.errno_f);
	uint _num_neurons_so_far = 0;
	uint _num_neurons = 0;
	// all the neurons is allocated in one long array (calloc clears mem)
	FannNeuron * p_neurons = (FannNeuron*)calloc(total_neurons, sizeof(FannNeuron));
	total_neurons_allocated = total_neurons;
	THROW_V(p_neurons, FANN_E_CANT_ALLOCATE_MEM);
	for(FannLayer * p_layer_it = P_FirstLayer; p_layer_it != P_LastLayer; p_layer_it++) {
		_num_neurons = (uint)(p_layer_it->P_LastNeuron - p_layer_it->P_FirstNeuron);
		p_layer_it->P_FirstNeuron = p_neurons + _num_neurons_so_far;
		p_layer_it->P_LastNeuron = p_layer_it->P_FirstNeuron + _num_neurons;
		_num_neurons_so_far += _num_neurons;
	}
	THROW_V(output = (fann_type*)calloc(_num_neurons, sizeof(fann_type)), FANN_E_CANT_ALLOCATE_MEM);
	CATCH
		fann_error(&Err, Err.errno_f);
		ok = 0;
	ENDCATCH
	return ok;
}
//
// INTERNAL FUNCTION
// Allocates room for the neurons.
//
/*void fann_allocate_neurons(Fann * pAnn)
{
	pAnn->AllocateNeurons();
}*/
//
// INTERNAL FUNCTION
// Allocate room for the connections.
//
void fann_allocate_connections(Fann * ann)
{
	ann->weights = (fann_type*)calloc(ann->total_connections, sizeof(fann_type));
	if(ann->weights == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}
	ann->total_connections_allocated = ann->total_connections;
	/* TODO make special cases for all places where the connections
	 * is used, so that it is not needed for fully connected networks.
	 */
	ann->connections = (FannNeuron**)calloc(ann->total_connections_allocated, sizeof(FannNeuron *));
	if(ann->connections == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return;
	}
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
		gettimeofday(&t, NULL);
		foo = t.tv_usec;
#ifdef DEBUG
		printf("unable to open /dev/urandom\n");
#endif
	}
	else{
		if(fread(&foo, sizeof(foo), 1, fp) != 1) {
			gettimeofday(&t, NULL);
			foo = t.tv_usec;
#ifdef DEBUG
			printf("unable to read from /dev/urandom\n");
#endif
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

