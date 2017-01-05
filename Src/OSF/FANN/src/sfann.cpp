// SFANN.CPP
// Copyright (C) 2003-2016 Steffen Nissen (steffen.fann@gmail.com)
//
// #include "config.h"
// #undef PACKAGE  // Name of package
// #undef VERSION  // Version number of package
// #undef X86_64   // Define for the x86_64 CPU famyly
#include "fann.h"
//#include "fann_data.h"
//#include "string.h"

#ifdef _MSC_VER
	#define vsnprintf _vsnprintf
	#define snprintf _snprintf
#endif

// define path max if not defined
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

FANN_EXTERNAL FILE * FANN_API fann_default_error_log = (FILE*)-1;

// #define FANN_NO_SEED
// #define DEBUGTRAIN

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
	// fann_type * cascade_activation_steepnesses;
	fann_type * cascade_candidate_scores; // @transient?
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
		cascade_candidate_scores = (fann_type*)malloc(GetCascadeNumCandidates() * sizeof(fann_type));
		THROW_V(cascade_candidate_scores, FANN_E_CANT_ALLOCATE_MEM);
		memcpy(cascade_candidate_scores, rS.cascade_candidate_scores, GetCascadeNumCandidates() * sizeof(fann_type));
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
		copy->cascade_candidate_scores = (fann_type*)malloc(copy->GetCascadeNumCandidates() * sizeof(fann_type));
		if(copy->cascade_candidate_scores == NULL) {
			fann_error((FannError*)pOrig, FANN_E_CANT_ALLOCATE_MEM);
			fann_destroy(copy);
			return NULL;
		}
		memcpy(copy->cascade_candidate_scores, pOrig->cascade_candidate_scores, copy->GetCascadeNumCandidates() * sizeof(fann_type));
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
		printf("Cascade activation functions[%d]      :   %s\n", i,
		    FANN_ACTIVATIONFUNC_NAMES[ann->cascade_activation_functions[i]]);
	for(i = 0; i < ann->cascade_activation_steepnesses_count; i++)
		printf("Cascade activation steepnesses[%d]    :%8.3f\n", i, ann->cascade_activation_steepnesses[i]);
	printf("Cascade candidate groups             :%4d\n", ann->cascade_num_candidate_groups);
	printf("Cascade no. of candidates            :%4d\n", ann->GetCascadeNumCandidates());
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
void fann_update_stepwise(Fann * ann)
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
	for(uint i = 0; i < ann->total_connections_allocated; i++) {
		if(i == ann->total_connections)
			printf("* ");
		printf("%f ", ann->weights[i]);
	}
	printf("\n\n");
}

/* Cascade training directly on the training data.
   The connected_neurons pointers are not valid during training,
   but they will be again after training.
 */
FANN_EXTERNAL void FANN_API fann_cascadetrain_on_data(Fann * ann, FannTrainData * data,
    uint max_neurons, uint neurons_between_reports, float desired_error)
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
		// print current error
		if(neurons_between_reports && (i % neurons_between_reports == 0 || i == max_neurons || i == 1 || desired_error_reached == 0)) {
			if(ann->callback == NULL) {
				printf("Neurons     %3d. Current error: %.6f. Total error:%8.4f. Epochs %5d. Bit fail %3d",
				    i-1, error, ann->MSE_value, total_epochs, ann->num_bit_fail);
				if((ann->P_LastLayer-2) != ann->P_FirstLayer) {
					printf(". candidate steepness %.2f. function %s",
					    (ann->P_LastLayer-2)->P_FirstNeuron->activation_steepness,
					    FANN_ACTIVATIONFUNC_NAMES[(ann->P_LastLayer-2)->P_FirstNeuron->activation_function]);
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
			/* Unable to initialize room for candidates */
			break;
		}
		/* train new candidates */
		total_epochs += fann_train_candidates(ann, data);
		/* this installs the best candidate */
		fann_install_candidate(ann);
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

FANN_EXTERNAL void FANN_API fann_cascadetrain_on_file(Fann * ann, const char * filename,
    uint max_neurons, uint neurons_between_reports, float desired_error)
{
	FannTrainData * data = fann_read_train_from_file(filename);
	if(data) {
		fann_cascadetrain_on_data(ann, data, max_neurons, neurons_between_reports, desired_error);
		fann_destroy_train(data);
	}
}

int fann_train_outputs(Fann * ann, FannTrainData * data, float desired_error)
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
		/* Improvement since start of train */
		error_improvement = initial_error - error;
		/* After any significant change, set a new goal and
		 * allow a new quota of epochs to reach it */
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

float fann_train_outputs_epoch(Fann * ann, FannTrainData * data)
{
	fann_reset_MSE(ann);
	for(uint i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_update_slopes_batch(ann, ann->P_LastLayer - 1, ann->P_LastLayer-1);
	}
	switch(ann->training_algorithm) {
		case FANN_TRAIN_RPROP:
		    fann_update_weights_irpropm(ann, (ann->P_LastLayer-1)->P_FirstNeuron->first_con, ann->total_connections);
		    break;
		case FANN_TRAIN_SARPROP:
		    fann_update_weights_sarprop(ann, ann->sarprop_epoch, (ann->P_LastLayer-1)->P_FirstNeuron->first_con,
		    ann->total_connections);
		    ++(ann->sarprop_epoch);
		    break;
		case FANN_TRAIN_QUICKPROP:
		    fann_update_weights_quickprop(ann, data->num_data, (ann->P_LastLayer-1)->P_FirstNeuron->first_con,
		    ann->total_connections);
		    break;
		case FANN_TRAIN_BATCH:
		case FANN_TRAIN_INCREMENTAL:
		    fann_error((FannError*)ann, FANN_E_CANT_USE_TRAIN_ALG);
	}
	return fann_get_MSE(ann);
}

int fann_reallocate_connections(Fann * ann, uint total_connections)
{
	/* The connections are allocated, but the pointers inside are
	 * first moved in the end of the cascade training session.
	 */

#ifdef CASCADE_DEBUG
	printf("realloc from %d to %d\n", ann->total_connections_allocated, total_connections);
#endif
	ann->connections = (FannNeuron**)realloc(ann->connections, total_connections * sizeof(FannNeuron *));
	if(ann->connections == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->weights = (fann_type*)realloc(ann->weights, total_connections * sizeof(fann_type));
	if(ann->weights == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->train_slopes = (fann_type*)realloc(ann->train_slopes, total_connections * sizeof(fann_type));
	if(ann->train_slopes == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->prev_steps = (fann_type*)realloc(ann->prev_steps, total_connections * sizeof(fann_type));
	if(ann->prev_steps == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->prev_train_slopes = (fann_type*)realloc(ann->prev_train_slopes, total_connections * sizeof(fann_type));
	if(ann->prev_train_slopes == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	ann->total_connections_allocated = total_connections;
	return 0;
}

int fann_reallocate_neurons(Fann * pAnn, uint totalNeurons)
{
	FannLayer * p_layer_it;
	uint   num_neurons = 0;
	uint   num_neurons_so_far = 0;
	FannNeuron * p_neurons = (FannNeuron*)realloc(pAnn->P_FirstLayer->P_FirstNeuron, totalNeurons * sizeof(FannNeuron));
	pAnn->total_neurons_allocated = totalNeurons;
	if(p_neurons == NULL) {
		fann_error(&pAnn->Err, FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	// Also allocate room for more train_errors
	pAnn->train_errors = (fann_type*)realloc(pAnn->train_errors, totalNeurons * sizeof(fann_type));
	if(pAnn->train_errors == NULL) {
		fann_error(&pAnn->Err, FANN_E_CANT_ALLOCATE_MEM);
		return -1;
	}
	if(p_neurons != pAnn->P_FirstLayer->P_FirstNeuron) {
		// Then the memory has moved, also move the pointers
#ifdef CASCADE_DEBUG_FULL
		printf("Moving neuron pointers\n");
#endif
		// Move pointers from layers to neurons
		for(p_layer_it = pAnn->P_FirstLayer; p_layer_it != pAnn->P_LastLayer; p_layer_it++) {
			num_neurons = (uint)(p_layer_it->P_LastNeuron - p_layer_it->P_FirstNeuron);
			p_layer_it->P_FirstNeuron = p_neurons + num_neurons_so_far;
			p_layer_it->P_LastNeuron = p_layer_it->P_FirstNeuron + num_neurons;
			num_neurons_so_far += num_neurons;
		}
	}
	return 0;
}

void initialize_candidate_weights(Fann * ann, uint first_con, uint last_con, float scale_factor)
{
	uint bias_weight = (uint)(first_con + (ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron) - 1);
	fann_type prev_step = (ann->training_algorithm == FANN_TRAIN_RPROP) ? ann->rprop_delta_zero : 0;
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

int fann_initialize_candidates(Fann * ann)
{
	/* The candidates are allocated after the normal neurons and connections,
	 * but there is an empty place between the real neurons and the candidate neurons,
	 * so that it will be possible to make room when the chosen candidate are copied in
	 * on the desired place.
	 */
	uint neurons_to_allocate, connections_to_allocate;
	uint num_candidates = ann->GetCascadeNumCandidates();
	uint num_neurons = ann->total_neurons + num_candidates + 1;
	uint num_hidden_neurons = ann->total_neurons - ann->num_input - ann->num_output;
	uint candidate_connections_in = ann->total_neurons - ann->num_output;
	uint candidate_connections_out = ann->num_output;

	/* the number of connections going into a and out of a candidate is
	 * ann->total_neurons */
	uint num_connections = ann->total_connections + (ann->total_neurons * (num_candidates + 1));
	uint first_candidate_connection = ann->total_connections + ann->total_neurons;
	uint first_candidate_neuron = ann->total_neurons + 1;
	uint connection_it, i, j, k, candidate_index;
	FannNeuron * neurons;
	float scale_factor;
	/* First make sure that there is enough room, and if not then allocate a
	 * bit more so that we do not need to allocate more room each time.
	 */
	if(num_neurons > ann->total_neurons_allocated) {
		/* Then we need to allocate more neurons
		 * Allocate half as many neurons as already exist (at least ten)
		 */
		neurons_to_allocate = num_neurons + num_neurons / 2;
		if(neurons_to_allocate < num_neurons + 10) {
			neurons_to_allocate = num_neurons + 10;
		}
		if(fann_reallocate_neurons(ann, neurons_to_allocate) == -1) {
			return -1;
		}
	}
	if(num_connections > ann->total_connections_allocated) {
		// Then we need to allocate more connections. Allocate half as many connections as already exist (at least enough for ten neurons)
		connections_to_allocate = num_connections + num_connections / 2;
		if(connections_to_allocate < (num_connections + ann->total_neurons * 10)) {
			connections_to_allocate = num_connections + ann->total_neurons * 10;
		}
		if(fann_reallocate_connections(ann, connections_to_allocate) == -1) {
			return -1;
		}
	}

	/* Some code to do semi Widrow + Nguyen initialization */
	scale_factor = (float)(2.0 * pow(0.7f * (float)num_hidden_neurons, 1.0f / (float)ann->num_input));
	if(scale_factor > 8)
		scale_factor = 8;
	else if(scale_factor < 0.5)
		scale_factor = 0.5;
	//
	// Set the neurons
	//
	connection_it = first_candidate_connection;
	neurons = ann->P_FirstLayer->P_FirstNeuron;
	candidate_index = first_candidate_neuron;
	for(i = 0; i < ann->cascade_activation_functions_count; i++) {
		for(j = 0; j < ann->cascade_activation_steepnesses_count; j++) {
			for(k = 0; k < ann->cascade_num_candidate_groups; k++) {
				// TODO candidates should actually be created both in
				// the last layer before the output layer, and in a new layer.
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
				initialize_candidate_weights(ann, neurons[candidate_index].first_con,
				    neurons[candidate_index].last_con+candidate_connections_out, scale_factor);
				candidate_index++;
			}
		}
	}
	/* Now randomize the weights and zero out the arrays that needs zeroing out.
	 */
	/*
	   #ifdef CASCADE_DEBUG_FULL
	   printf("random cand weight [%d ... %d]\n", first_candidate_connection, num_connections - 1);
	   #endif

	   for(i = first_candidate_connection; i < num_connections; i++)
	   {

	       //ann->weights[i] = fann_random_weight();
	       ann->weights[i] = fann_rand(-2.0,2.0);
	       ann->train_slopes[i] = 0;
	       ann->prev_steps[i] = 0;
	       ann->prev_train_slopes[i] = initial_slope;
	   }
	 */

	return 0;
}

int fann_train_candidates(Fann * ann, FannTrainData * data)
{
	fann_type best_cand_score = 0.0;
	fann_type target_cand_score = 0.0;
	fann_type backslide_cand_score = -1.0e20f;
	uint i;
	uint max_epochs = ann->cascade_max_cand_epochs;
	uint min_epochs = ann->cascade_min_cand_epochs;
	uint stagnation = max_epochs;
	if(ann->cascade_candidate_scores == NULL) {
		ann->cascade_candidate_scores = (fann_type*)malloc(ann->GetCascadeNumCandidates() * sizeof(fann_type));
		if(ann->cascade_candidate_scores == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
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
			// printf("best_cand_score=%f, target_cand_score=%f, backslide_cand_score=%f, stagnation=%d\n",
			// best_cand_score, target_cand_score, backslide_cand_score, stagnation);
#endif
			target_cand_score = best_cand_score * (1.0f + ann->cascade_candidate_change_fraction);
			backslide_cand_score = best_cand_score * (1.0f - ann->cascade_candidate_change_fraction);
			stagnation = i + ann->cascade_candidate_stagnation_epochs;
		}
		// No improvement in allotted period, so quit
		if(i >= stagnation && i >= min_epochs) {
#ifdef CASCADE_DEBUG
			printf("Stagnation with %d epochs, best candidate score %f, real score: %f\n", i + 1,
			    ann->MSE_value - best_cand_score, best_cand_score);
#endif
			return i + 1;
		}
	}

#ifdef CASCADE_DEBUG
	printf("Max epochs %d reached, best candidate score %f, real score: %f\n", max_epochs,
	    ann->MSE_value - best_cand_score, best_cand_score);
#endif
	return max_epochs;
}

void fann_update_candidate_slopes(Fann * ann)
{
	FannNeuron * neurons = ann->P_FirstLayer->P_FirstNeuron;
	FannNeuron * first_cand = neurons + ann->total_neurons + 1;
	FannNeuron * last_cand = first_cand + ann->GetCascadeNumCandidates();
	uint i, j, num_connections;
	uint num_output = ann->num_output;
	fann_type max_sum, cand_sum, activation, derived, error_value, diff, cand_score;
	fann_type * weights, * cand_out_weights, * cand_slopes, * cand_out_slopes;
	fann_type * output_train_errors = ann->train_errors + (ann->total_neurons - ann->num_output);
	for(FannNeuron * cand_it = first_cand; cand_it < last_cand; cand_it++) {
		cand_score = ann->cascade_candidate_scores[cand_it - first_cand];
		error_value = 0.0;
		// code more or less stolen from fann_run to fast forward pass
		cand_sum = 0.0;
		num_connections = cand_it->last_con - cand_it->first_con;
		weights = ann->weights + cand_it->first_con;
		// unrolled loop start
		i = num_connections & 3;        /* same as modulo 4 */
		switch(i) {
			case 3: cand_sum += weights[2] * neurons[2].value;
			case 2: cand_sum += weights[1] * neurons[1].value;
			case 1: cand_sum += weights[0] * neurons[0].value;
			case 0: break;
		}
		for(; i != num_connections; i += 4) {
			cand_sum += weights[i] * neurons[i].value + weights[i+1] * neurons[i+1].value + weights[i+2] * neurons[i+2].value + weights[i+3] * neurons[i+3].value;
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
		/* The output weights is located right after the input weights in
		 * the weight array.
		 */
		cand_out_weights = weights + num_connections;
		cand_out_slopes = ann->train_slopes + cand_it->first_con + num_connections;
		for(j = 0; j < num_output; j++) {
			diff = (activation * cand_out_weights[j]) - output_train_errors[j];
#ifdef CASCADE_DEBUG_FULL
			/* printf("diff = %f = (%f * %f) - %f;\n", diff, activation, cand_out_weights[j],
			  output_train_errors[j]); */
#endif
			cand_out_slopes[j] -= 2.0f * diff * activation;
#ifdef CASCADE_DEBUG_FULL
			/* printf("cand_out_slopes[%d] <= %f += %f * %f;\n", j, cand_out_slopes[j], diff, activation);
			  */
#endif
			error_value += diff * cand_out_weights[j];
			cand_score -= (diff * diff);
#ifdef CASCADE_DEBUG_FULL
			/* printf("cand_score[%d][%d] = %f -= (%f * %f)\n", cand_it - first_cand, j, cand_score, diff,
			  diff); */

			printf("cand[%d]: error=%f, activation=%f, diff=%f, slope=%f\n", cand_it - first_cand,
			    output_train_errors[j], (activation * cand_out_weights[j]), diff,
			    -2.0 * diff * activation);
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

void fann_update_candidate_weights(Fann * ann, uint num_data)
{
	FannNeuron * first_cand = (ann->P_LastLayer-1)->P_LastNeuron + 1; /* there is an empty neuron
		between the actual neurons and the candidate neuron */
	FannNeuron * last_cand = first_cand + ann->GetCascadeNumCandidates() - 1;
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
		    fann_error((FannError*)ann, FANN_E_CANT_USE_TRAIN_ALG);
		    break;
	}
}

fann_type fann_train_candidates_epoch(Fann * ann, FannTrainData * data)
{
	uint i, j;
	uint best_candidate;
	fann_type best_score;
	uint num_cand = ann->GetCascadeNumCandidates();
	fann_type * output_train_errors = ann->train_errors + (ann->total_neurons - ann->num_output);
	FannNeuron * output_neurons = (ann->P_LastLayer-1)->P_FirstNeuron;
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

			/*
			 * output_train_errors[j] = (data->output[i][j] - ann->output[j])/2;
			 * output_train_errors[j] = ann->output[j] - data->output[i][j];
			 */
			output_train_errors[j] = (data->output[i][j] - ann->output[j]);
			switch(output_neurons[j].activation_function) {
				case FANN_LINEAR_PIECE_SYMMETRIC:
				case FANN_SIGMOID_SYMMETRIC:
				case FANN_SIGMOID_SYMMETRIC_STEPWISE:
				case FANN_THRESHOLD_SYMMETRIC:
				case FANN_ELLIOT_SYMMETRIC:
				case FANN_GAUSSIAN_SYMMETRIC:
				case FANN_SIN_SYMMETRIC:
				case FANN_COS_SYMMETRIC:
				    output_train_errors[j] /= 2.0;
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
		fann_update_candidate_slopes(ann);
	}
	fann_update_candidate_weights(ann, data->num_data);
	/* find the best candidate score */
	best_candidate = 0;
	best_score = ann->cascade_candidate_scores[best_candidate];
	for(i = 1; i < num_cand; i++) {
		/*FannNeuron *cand = ann->P_FirstLayer->P_FirstNeuron + ann->total_neurons + 1 + i;
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
	printf("Best candidate[%d]: with score %f, real score: %f\n", best_candidate,
	    ann->MSE_value - best_score, best_score);
#endif

	return best_score;
}
//
// add a layer at the position pointed to by *layer
//
FannLayer * fann_add_layer(Fann * ann, FannLayer * layer)
{
	int layer_pos = (int)(layer - ann->P_FirstLayer);
	int num_layers = (int)(ann->P_LastLayer - ann->P_FirstLayer + 1);
	int i;
	/* allocate the layer */
	FannLayer * layers = (FannLayer*)realloc(ann->P_FirstLayer, num_layers * sizeof(FannLayer));
	if(layers == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	// copy layers so that the free space is at the right location
	for(i = num_layers - 1; i >= layer_pos; i--) {
		layers[i] = layers[i - 1];
	}
	// the newly allocated layer is empty
	layers[layer_pos].P_FirstNeuron = layers[layer_pos + 1].P_FirstNeuron;
	layers[layer_pos].P_LastNeuron = layers[layer_pos + 1].P_FirstNeuron;
	// Set the ann pointers correctly
	ann->P_FirstLayer = layers;
	ann->P_LastLayer = layers + num_layers;
#ifdef CASCADE_DEBUG_FULL
	printf("add layer at pos %d\n", layer_pos);
#endif
	return layers + layer_pos;
}

void fann_set_shortcut_connections(Fann * ann)
{
	uint   num_connections = 0;
	FannNeuron ** neuron_pointers = ann->connections;
	FannNeuron * neurons = ann->P_FirstLayer->P_FirstNeuron;
	for(FannLayer * layer_it = ann->P_FirstLayer + 1; layer_it != ann->P_LastLayer; layer_it++) {
		for(FannNeuron * neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
			neuron_pointers += num_connections;
			num_connections = neuron_it->last_con - neuron_it->first_con;
			for(uint i = 0; i != num_connections; i++) {
				neuron_pointers[i] = neurons + i;
			}
		}
	}
}

void fann_add_candidate_neuron(Fann * ann, FannLayer * layer)
{
	uint num_connections_in = (uint)(layer->P_FirstNeuron - ann->P_FirstLayer->P_FirstNeuron);
	uint num_connections_out = (uint)((ann->P_LastLayer-1)->P_LastNeuron - (layer + 1)->P_FirstNeuron);
	uint num_connections_move = num_connections_out + num_connections_in;
	uint candidate_con, candidate_output_weight;
	int i;
	FannLayer * layer_it;
	FannNeuron * neuron_it, * neuron_place, * candidate;
	/* We know that there is enough room for the new neuron
	 * (the candidates are in the same arrays), so move
	 * the last neurons to make room for this neuron.
	 */
	/* first move the pointers to neurons in the layer structs */
	for(layer_it = ann->P_LastLayer - 1; layer_it != layer; layer_it--) {
#ifdef CASCADE_DEBUG_FULL
		printf("move neuron pointers in layer %d, first(%d -> %d), last(%d -> %d)\n",
		    layer_it - ann->P_FirstLayer,
		    layer_it->P_FirstNeuron - ann->P_FirstLayer->P_FirstNeuron,
		    layer_it->P_FirstNeuron - ann->P_FirstLayer->P_FirstNeuron + 1,
		    layer_it->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron,
		    layer_it->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron + 1);
#endif
		layer_it->P_FirstNeuron++;
		layer_it->P_LastNeuron++;
	}
	// also move the last neuron in the layer that needs the neuron added
	layer->P_LastNeuron++;
	// this is the place that should hold the new neuron
	neuron_place = layer->P_LastNeuron - 1;
#ifdef CASCADE_DEBUG_FULL
	printf("num_connections_in=%d, num_connections_out=%d\n", num_connections_in,
	    num_connections_out);
#endif
	candidate = ann->P_FirstLayer->P_FirstNeuron + ann->cascade_best_candidate;
	// the output weights for the candidates are located after the input weights
	candidate_output_weight = candidate->last_con;
	// move the actual output neurons and the indexes to the connection arrays
	for(neuron_it = (ann->P_LastLayer-1)->P_LastNeuron - 1; neuron_it != neuron_place; neuron_it--) {
#ifdef CASCADE_DEBUG_FULL
		printf("move neuron %d -> %d\n", neuron_it - ann->P_FirstLayer->P_FirstNeuron - 1,
		    neuron_it - ann->P_FirstLayer->P_FirstNeuron);
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
		// move the indexes to weights
		neuron_it->last_con += num_connections_move;
		num_connections_move--;
		neuron_it->first_con += num_connections_move;
		// set the new weight to the newly allocated neuron
		ann->weights[neuron_it->last_con - 1] =
		    (ann->weights[candidate_output_weight]) * ann->cascade_weight_multiplier;
		candidate_output_weight++;
	}
	// Now inititalize the actual neuron
	neuron_place->value = 0;
	neuron_place->sum = 0;
	neuron_place->activation_function = candidate->activation_function;
	neuron_place->activation_steepness = candidate->activation_steepness;
	neuron_place->last_con = (neuron_place + 1)->first_con;
	neuron_place->first_con = neuron_place->last_con - num_connections_in;
#ifdef CASCADE_DEBUG_FULL
	printf("neuron[%d] = weights[%d ... %d] activation: %s, steepness: %f\n",
	    neuron_place - ann->P_FirstLayer->P_FirstNeuron, neuron_place->first_con,
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
	// Change some of main variables
	ann->total_neurons++;
	ann->total_connections += num_connections_in + num_connections_out;
}

void fann_install_candidate(Fann * ann)
{
	FannLayer * p_layer = fann_add_layer(ann, ann->P_LastLayer-1);
	fann_add_candidate_neuron(ann, p_layer);
}

#endif /* FIXEDFANN */

FANN_EXTERNAL uint FANN_API fann_get_cascade_num_candidates(const Fann * pAnn)
{
	return pAnn->GetCascadeNumCandidates();
}

FANN_GET_SET(float, cascade_output_change_fraction)
FANN_GET_SET(uint, cascade_output_stagnation_epochs)
FANN_GET_SET(float, cascade_candidate_change_fraction)
FANN_GET_SET(uint, cascade_candidate_stagnation_epochs)
FANN_GET_SET(uint, cascade_num_candidate_groups)
FANN_GET_SET(fann_type, cascade_weight_multiplier)
FANN_GET_SET(fann_type, cascade_candidate_limit)
FANN_GET_SET(uint, cascade_max_out_epochs)
FANN_GET_SET(uint, cascade_max_cand_epochs)
FANN_GET_SET(uint, cascade_min_out_epochs)
FANN_GET_SET(uint, cascade_min_cand_epochs)

FANN_GET(uint, cascade_activation_functions_count)
FANN_GET(enum fann_activationfunc_enum *, cascade_activation_functions)

FANN_EXTERNAL void FANN_API fann_set_cascade_activation_functions(Fann * ann,
    enum fann_activationfunc_enum * cascade_activation_functions, uint cascade_activation_functions_count)
{
	if(ann->cascade_activation_functions_count != cascade_activation_functions_count) {
		ann->cascade_activation_functions_count = cascade_activation_functions_count;
		/* reallocate mem */
		ann->cascade_activation_functions = (enum fann_activationfunc_enum*)realloc(ann->cascade_activation_functions,
		    ann->cascade_activation_functions_count * sizeof(enum fann_activationfunc_enum));
		if(ann->cascade_activation_functions == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	memmove(ann->cascade_activation_functions, cascade_activation_functions,
	    ann->cascade_activation_functions_count * sizeof(enum fann_activationfunc_enum));
}

FANN_GET(uint, cascade_activation_steepnesses_count)
FANN_GET(fann_type *, cascade_activation_steepnesses)

FANN_EXTERNAL void FANN_API fann_set_cascade_activation_steepnesses(Fann * ann,
    fann_type * cascade_activation_steepnesses, uint cascade_activation_steepnesses_count)
{
	if(ann->cascade_activation_steepnesses_count != cascade_activation_steepnesses_count) {
		ann->cascade_activation_steepnesses_count = cascade_activation_steepnesses_count;
		/* reallocate mem */
		ann->cascade_activation_steepnesses = (fann_type*)realloc(ann->cascade_activation_steepnesses,
		    ann->cascade_activation_steepnesses_count * sizeof(fann_type));
		if(ann->cascade_activation_steepnesses == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	memmove(ann->cascade_activation_steepnesses, cascade_activation_steepnesses,
	    ann->cascade_activation_steepnesses_count * sizeof(fann_type));
}
//
// FANN_ERROR
//
//
// resets the last error number
//
FANN_EXTERNAL void FANN_API fann_reset_errno(FannError * errdat)
{
	errdat->errno_f = FANN_E_NO_ERROR;
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
FANN_EXTERNAL enum fann_errno_enum FANN_API fann_get_errno(FannError * errdat)
{
	return errdat->errno_f;
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
FANN_EXTERNAL void FANN_API fann_set_error_log(FannError * errdat, FILE * log_file)
{
	if(errdat == NULL)
		fann_default_error_log = log_file;
	else
		errdat->error_log = log_file;
}

/* prints the last error to stderr
 */
FANN_EXTERNAL void FANN_API fann_print_error(FannError * errdat)
{
	if(errdat->errno_f != FANN_E_NO_ERROR && errdat->Msg.NotEmpty()) {
		fprintf(stderr, "FANN Error %d: %s", errdat->errno_f, errdat->Msg.cptr());
	}
}
//
// INTERNAL FUNCTION
// Populate the error information
//
void fann_error(FannError * errdat, const enum fann_errno_enum errno_f, ...)
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
		case FANN_E_CANT_OPEN_CONFIG_R:
		    vsnprintf(errstr, errstr_max, "Unable to open configuration file \"%s\" for reading.\n", ap);
		    break;
		case FANN_E_CANT_OPEN_CONFIG_W:
		    vsnprintf(errstr, errstr_max, "Unable to open configuration file \"%s\" for writing.\n", ap);
		    break;
		case FANN_E_WRONG_CONFIG_VERSION:
		    vsnprintf(errstr, errstr_max,
		    "Wrong version of configuration file, aborting read of configuration file \"%s\".\n",
		    ap);
		    break;
		case FANN_E_CANT_READ_CONFIG:
		    vsnprintf(errstr, errstr_max, "Error reading \"%s\" from configuration file \"%s\".\n", ap);
		    break;
		case FANN_E_CANT_READ_NEURON:
		    vsnprintf(errstr, errstr_max, "Error reading neuron info from configuration file \"%s\".\n", ap);
		    break;
		case FANN_E_CANT_READ_CONNECTIONS:
		    vsnprintf(errstr, errstr_max, "Error reading connections from configuration file \"%s\".\n", ap);
		    break;
		case FANN_E_WRONG_NUM_CONNECTIONS:
		    vsnprintf(errstr, errstr_max, "ERROR connections_so_far=%d, total_connections=%d\n", ap);
		    break;
		case FANN_E_CANT_OPEN_TD_W:
		    vsnprintf(errstr, errstr_max, "Unable to open train data file \"%s\" for writing.\n", ap);
		    break;
		case FANN_E_CANT_OPEN_TD_R:
		    vsnprintf(errstr, errstr_max, "Unable to open train data file \"%s\" for writing.\n", ap);
		    break;
		case FANN_E_CANT_READ_TD:
		    vsnprintf(errstr, errstr_max, "Error reading info from train data file \"%s\", line: %d.\n", ap);
		    break;
		case FANN_E_CANT_ALLOCATE_MEM:
		    STRNSCPY(errstr, "Unable to allocate memory.\n");
		    break;
		case FANN_E_CANT_TRAIN_ACTIVATION:
		    STRNSCPY(errstr, "Unable to train with the selected activation function.\n");
		    break;
		case FANN_E_CANT_USE_ACTIVATION:
		    STRNSCPY(errstr, "Unable to use the selected activation function.\n");
		    break;
		case FANN_E_TRAIN_DATA_MISMATCH:
		    STRNSCPY(errstr, "Training data must be of equivalent structure.\n");
		    break;
		case FANN_E_CANT_USE_TRAIN_ALG:
		    STRNSCPY(errstr, "Unable to use the selected training algorithm.\n");
		    break;
		case FANN_E_TRAIN_DATA_SUBSET:
		    vsnprintf(errstr, errstr_max, "Subset from %d of length %d not valid in training set of length %d.\n", ap);
		    break;
		case FANN_E_INDEX_OUT_OF_BOUND:
		    vsnprintf(errstr, errstr_max, "Index %d is out of bound.\n", ap);
		    break;
		case FANN_E_SCALE_NOT_PRESENT:
		    strcpy(errstr, "Scaling parameters not present.\n");
		    break;
		case FANN_E_INPUT_NO_MATCH:
		    vsnprintf(errstr, errstr_max, "The number of input neurons in the ann (%d) and data (%d) don't match\n", ap);
		    break;
		case FANN_E_OUTPUT_NO_MATCH:
		    vsnprintf(errstr, errstr_max, "The number of output neurons in the ann (%d) and data (%d) don't match\n", ap);
		    break;
		case FANN_E_WRONG_PARAMETERS_FOR_CREATE:
		    strcpy(errstr, "The parameters for create_standard are wrong, either too few parameters provided or a negative/very high value provided.\n");
		    break;
	}
	va_end(ap);
	if(errdat) {
		errdat->Msg = errstr;
		error_log = errdat->error_log;
	}
	if(error_log == (FILE*)-1) { /* This is the default behavior and will give stderr */
		fprintf(stderr, "FANN Error %d: %s", errno_f, errstr);
	}
	else if(error_log != NULL) {
		fprintf(error_log, "FANN Error %d: %s", errno_f, errstr);
	}
}
//
// INTERNAL FUNCTION
// Initialize an error data strcuture
//
void fann_init_error_data(FannError * errdat)
{
	errdat->Msg = 0;
	errdat->errno_f = FANN_E_NO_ERROR;
	errdat->error_log = fann_default_error_log;
}
//
// FANN_IO
//
//
// Create a network from a configuration file.
//
FANN_EXTERNAL Fann * FANN_API fann_create_from_file(const char * configuration_file)
{
	Fann * ann = 0;
	FILE * conf = fopen(configuration_file, "r");
	if(!conf) {
		fann_error(NULL, FANN_E_CANT_OPEN_CONFIG_R, configuration_file);
	}
	else {
		ann = fann_create_from_fd(conf, configuration_file);
		fclose(conf);
	}
	return ann;
}
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
		fann_error((FannError*)ann, FANN_E_CANT_OPEN_CONFIG_W, configuration_file);
		return -1;
	}
	else {
		int retval = fann_save_internal_fd(ann, conf, configuration_file, save_as_fixed);
		fclose(conf);
		return retval;
	}
}
//
// INTERNAL FUNCTION
// Used to save the network to a file descriptor.
//
int fann_save_internal_fd(Fann * ann, FILE * conf, const char * configuration_file, uint save_as_fixed)
{
	FannLayer * layer_it;
	int calculated_decimal_point = 0;
	FannNeuron * neuron_it, * first_neuron;
	fann_type * weights;
	FannNeuron ** connected_neurons;
	uint i = 0;
#ifndef FIXEDFANN
	// variabels for use when saving floats as fixed point variabels
	uint decimal_point = 0;
	uint fixed_multiplier = 0;
	fann_type max_possible_value = 0;
	uint bits_used_for_max = 0;
	fann_type current_max_value = 0;
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
				for(i = neuron_it->first_con; i != neuron_it->last_con; i++) {
					current_max_value += fann_abs(ann->weights[i]);
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
#ifdef DEBUG
		printf("calculated_decimal_point=%d, decimal_point=%u, bits_used_for_max=%u\n", calculated_decimal_point, decimal_point, bits_used_for_max);
#endif
		fprintf(conf, "decimal_point=%u\n", decimal_point); // save the decimal_point on a seperate line
	}
#else
	// save the decimal_point on a seperate line
	fprintf(conf, "decimal_point=%u\n", ann->decimal_point);
#endif
	// Save network parameters
	fprintf(conf, "num_layers=%d\n", (int)(ann->P_LastLayer - ann->P_FirstLayer));
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
		fprintf(conf, "bit_fail_limit=%u\n", (int)floor((ann->bit_fail_limit * fixed_multiplier) + 0.5));
		fprintf(conf, "cascade_candidate_limit=%u\n", (int)floor((ann->cascade_candidate_limit * fixed_multiplier) + 0.5));
		fprintf(conf, "cascade_weight_multiplier=%u\n", (int)floor((ann->cascade_weight_multiplier * fixed_multiplier) + 0.5));
	}
	else
#endif
	{
		fprintf(conf, "bit_fail_limit=\"FANNPRINTF \"\n", ann->bit_fail_limit);
		fprintf(conf, "cascade_candidate_limit=\"FANNPRINTF \"\n", ann->cascade_candidate_limit);
		fprintf(conf, "cascade_weight_multiplier=\"FANNPRINTF \"\n", ann->cascade_weight_multiplier);
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
			fprintf(conf, "%u ", (int)floor((ann->cascade_activation_steepnesses[i] * fixed_multiplier) + 0.5));
		else
#endif
		fprintf(conf, FANNPRINTF " ", ann->cascade_activation_steepnesses[i]);
	}
	fprintf(conf, "\n");
	fprintf(conf, "layer_sizes=");
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		// the number of neurons in the layers (in the last layer, there is always one too many neurons, because of an unused bias)
		fprintf(conf, "%d ", (int)(layer_it->P_LastNeuron - layer_it->P_FirstNeuron));
	}
	fprintf(conf, "\n");
#ifndef FIXEDFANN
	/* 2.1 */
	#define SCALE_SAVE(what, where) fprintf(conf, # what "_" # where "="); \
		for(i = 0; i < ann->num_ ## where ## put; i++)						  \
			fprintf(conf, "%f ", ann->what ## _ ## where[ i ]);				  \
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
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		/* the neurons */
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != layer_it->P_LastNeuron; neuron_it++) {
#ifndef FIXEDFANN
			if(save_as_fixed) {
				fprintf(conf, "(%u, %u, %u) ", neuron_it->last_con - neuron_it->first_con,
				    neuron_it->activation_function, (int)floor((neuron_it->activation_steepness * fixed_multiplier) + 0.5));
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
	first_neuron = ann->P_FirstLayer->P_FirstNeuron;

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

Fann * fann_create_from_fd_1_1(FILE * conf, const char * configuration_file);

#define fann_scanf(type, name, val) \
	{ \
		if(fscanf(conf, name "="type "\n", val) != 1) { \
			fann_error(NULL, FANN_E_CANT_READ_CONFIG, name, configuration_file); \
			fann_destroy(ann); \
			return NULL; \
		} \
	}

#define fann_skip(name)	\
	{ \
		if(fscanf(conf, name) != 0) { \
			fann_error(NULL, FANN_E_CANT_READ_CONFIG, name, configuration_file); \
			fann_destroy(ann); \
			return NULL; \
		} \
	}
//
// INTERNAL FUNCTION
// Create a network from a configuration file descriptor.
//
Fann * fann_create_from_fd(FILE * conf, const char * configuration_file)
{
	uint num_layers, layer_size, input_neuron, i, num_connections;
	uint tmpVal;
#ifdef FIXEDFANN
	uint decimal_point, multiplier;
#else
	uint scale_included;
#endif
	FannNeuron * p_first_neuron;
	FannNeuron * neuron_it;
	FannNeuron * last_neuron;
	FannNeuron ** connected_neurons;
	fann_type * weights;
	FannLayer * layer_it;
	Fann * ann = NULL;
	char * read_version = (char *)calloc(strlen(FANN_CONF_VERSION "\n"), 1);
	if(read_version == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	if(fread(read_version, 1, strlen(FANN_CONF_VERSION "\n"), conf) == 1) {
		fann_error(NULL, FANN_E_CANT_READ_CONFIG, "FANN_VERSION", configuration_file);
		return NULL;
	}
	/* compares the version information */
	if(strncmp(read_version, FANN_CONF_VERSION "\n", strlen(FANN_CONF_VERSION "\n")) != 0) {
#ifdef FIXEDFANN
		if(strncmp(read_version, "FANN_FIX_1.1\n", strlen("FANN_FIX_1.1\n")) == 0) {
#else
		if(strncmp(read_version, "FANN_FLO_1.1\n", strlen("FANN_FLO_1.1\n")) == 0) {
#endif
			free(read_version);
			return fann_create_from_fd_1_1(conf, configuration_file);
		}

#ifndef FIXEDFANN
		/* Maintain compatibility with 2.0 version that doesnt have scale parameters. */
		if(strncmp(read_version, "FANN_FLO_2.0\n", strlen("FANN_FLO_2.0\n")) != 0 &&
		    strncmp(read_version, "FANN_FLO_2.1\n", strlen("FANN_FLO_2.1\n")) != 0)
#else
		if(strncmp(read_version, "FANN_FIX_2.0\n", strlen("FANN_FIX_2.0\n")) != 0 &&
		    strncmp(read_version, "FANN_FIX_2.1\n", strlen("FANN_FIX_2.1\n")) != 0)
#endif
		{
			free(read_version);
			fann_error(NULL, FANN_E_WRONG_CONFIG_VERSION, configuration_file);
			return NULL;
		}
	}
	free(read_version);
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
	ann->cascade_activation_functions = (enum fann_activationfunc_enum*)realloc(ann->cascade_activation_functions,
	    ann->cascade_activation_functions_count * sizeof(enum fann_activationfunc_enum));
	if(ann->cascade_activation_functions == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(ann);
		return NULL;
	}
	fann_skip("cascade_activation_functions=");
	for(i = 0; i < ann->cascade_activation_functions_count; i++) {
		if(fscanf(conf, "%u ", &tmpVal) != 1) {
			fann_error(NULL, FANN_E_CANT_READ_CONFIG, "cascade_activation_functions", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		ann->cascade_activation_functions[i] = (enum fann_activationfunc_enum)tmpVal;
	}
	fann_scanf("%u", "cascade_activation_steepnesses_count", &ann->cascade_activation_steepnesses_count);
	/* reallocate mem */
	ann->cascade_activation_steepnesses = (fann_type*)realloc(ann->cascade_activation_steepnesses, ann->cascade_activation_steepnesses_count * sizeof(fann_type));
	if(ann->cascade_activation_steepnesses == NULL) {
		fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy(ann);
		return NULL;
	}
	fann_skip("cascade_activation_steepnesses=");
	for(i = 0; i < ann->cascade_activation_steepnesses_count; i++) {
		if(fscanf(conf, FANNSCANF " ", &ann->cascade_activation_steepnesses[i]) != 1) {
			fann_error(NULL, FANN_E_CANT_READ_CONFIG, "cascade_activation_steepnesses", configuration_file);
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
	// determine how many neurons there should be in each layer
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		if(fscanf(conf, "%u ", &layer_size) != 1) {
			fann_error((FannError*)ann, FANN_E_CANT_READ_CONFIG, "layer_sizes", configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		// we do not allocate room here, but we make sure that last_neuron - first_neuron is the number of neurons
		layer_it->P_FirstNeuron = NULL;
		layer_it->P_LastNeuron = layer_it->P_FirstNeuron + layer_size;
		ann->total_neurons += layer_size;
#ifdef DEBUG
		if(ann->network_type == FANN_NETTYPE_SHORTCUT && layer_it != ann->P_FirstLayer) {
			printf("  layer       : %d neurons, 0 bias\n", layer_size);
		}
		else {
			printf("  layer       : %d neurons, 1 bias\n", layer_size - 1);
		}
#endif
	}
	ann->num_input = (uint)(ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron - 1);
	ann->num_output = (uint)((ann->P_LastLayer-1)->P_LastNeuron - (ann->P_LastLayer-1)->P_FirstNeuron);
	if(ann->network_type == FANN_NETTYPE_LAYER) {
		// one too many (bias) in the output layer
		ann->num_output--;
	}
#ifndef FIXEDFANN
#define SCALE_LOAD(what, where)											      \
	fann_skip(# what "_" # where "=");									\
	for(i = 0; i < ann->num_ ## where ## put; i++) { \
		if(fscanf(conf, "%f ", (float*)&ann->what ## _ ## where[ i ]) != 1) { \
			fann_error((FannError*)ann, FANN_E_CANT_READ_CONFIG, # what "_" # where, configuration_file); \
			fann_destroy(ann);												\
			return NULL;													\
		}																	\
	}
	if(fscanf(conf, "scale_included=%u\n", &scale_included) == 1 && scale_included == 1) {
		fann_allocate_scale(ann);
		SCALE_LOAD(scale_mean, in)
		SCALE_LOAD(scale_deviation, in)
		SCALE_LOAD(scale_new_min, in)
		SCALE_LOAD(scale_factor, in)
		SCALE_LOAD(scale_mean, out)
		SCALE_LOAD(scale_deviation, out)
		SCALE_LOAD(scale_new_min, out)
		SCALE_LOAD(scale_factor, out)
	}
#undef SCALE_LOAD
#endif
	// allocate room for the actual neurons
	if(!ann->AllocateNeurons()) {
		fann_destroy(ann);
		return NULL;
	}
	last_neuron = (ann->P_LastLayer-1)->P_LastNeuron;
	fann_skip("neurons (num_inputs, activation_function, activation_steepness)=");
	for(neuron_it = ann->P_FirstLayer->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
		if(fscanf(conf, "(%u, %u, " FANNSCANF ") ", &num_connections, &tmpVal, &neuron_it->activation_steepness) != 3) {
			fann_error((FannError*)ann, FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		neuron_it->activation_function = (enum fann_activationfunc_enum)tmpVal;
		neuron_it->first_con = ann->total_connections;
		ann->total_connections += num_connections;
		neuron_it->last_con = ann->total_connections;
	}
	fann_allocate_connections(ann);
	if(ann->IsError(FANN_E_CANT_ALLOCATE_MEM)) {
		fann_destroy(ann);
		return NULL;
	}
	connected_neurons = ann->connections;
	weights = ann->weights;
	p_first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	fann_skip("connections (connected_to_neuron, weight)=");
	for(i = 0; i < ann->total_connections; i++) {
		if(fscanf(conf, "(%u, " FANNSCANF ") ", &input_neuron, &weights[i]) != 2) {
			fann_error((FannError*)ann, FANN_E_CANT_READ_CONNECTIONS, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		connected_neurons[i] = p_first_neuron + input_neuron;
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
Fann * fann_create_from_fd_1_1(FILE * conf, const char * configuration_file)
{
	uint num_layers, layer_size, input_neuron, i, network_type, num_connections;
	uint activation_function_hidden, activation_function_output;
#ifdef FIXEDFANN
	uint decimal_point, multiplier;
#endif
	fann_type activation_steepness_hidden, activation_steepness_output;
	float learning_rate, connection_rate;
	FannNeuron * first_neuron, * neuron_it, * last_neuron, ** connected_neurons;
	fann_type * weights;
	FannLayer * layer_it;
	Fann * ann;
#ifdef FIXEDFANN
	if(fscanf(conf, "%u\n", &decimal_point) != 1) {
		fann_error(NULL, FANN_E_CANT_READ_CONFIG, "decimal_point", configuration_file);
		return NULL;
	}
	multiplier = 1 << decimal_point;
#endif
	if(fscanf(conf, "%u %f %f %u %u %u " FANNSCANF " " FANNSCANF "\n", &num_layers, &learning_rate,
		    &connection_rate, &network_type, &activation_function_hidden,
		    &activation_function_output, &activation_steepness_hidden,
		    &activation_steepness_output) != 8) {
		fann_error(NULL, FANN_E_CANT_READ_CONFIG, "parameters", configuration_file);
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
	/* determine how many neurons there should be in each layer */
	for(layer_it = ann->P_FirstLayer; layer_it != ann->P_LastLayer; layer_it++) {
		if(fscanf(conf, "%u ", &layer_size) != 1) {
			fann_error((FannError*)ann, FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		// we do not allocate room here, but we make sure that last_neuron - first_neuron is the number of neurons
		layer_it->P_FirstNeuron = NULL;
		layer_it->P_LastNeuron = layer_it->P_FirstNeuron + layer_size;
		ann->total_neurons += layer_size;
#ifdef DEBUG
		if(ann->network_type == FANN_NETTYPE_SHORTCUT && layer_it != ann->P_FirstLayer) {
			printf("  layer       : %d neurons, 0 bias\n", layer_size);
		}
		else{
			printf("  layer       : %d neurons, 1 bias\n", layer_size - 1);
		}
#endif
	}

	ann->num_input = (uint)(ann->P_FirstLayer->P_LastNeuron - ann->P_FirstLayer->P_FirstNeuron - 1);
	ann->num_output = (uint)((ann->P_LastLayer-1)->P_LastNeuron - (ann->P_LastLayer-1)->P_FirstNeuron);
	if(ann->network_type == FANN_NETTYPE_LAYER) {
		// one too many (bias) in the output layer
		ann->num_output--;
	}
	// allocate room for the actual neurons
	if(!ann->AllocateNeurons()) {
		fann_destroy(ann);
		return NULL;
	}
	last_neuron = (ann->P_LastLayer-1)->P_LastNeuron;
	for(neuron_it = ann->P_FirstLayer->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
		if(fscanf(conf, "%u ", &num_connections) != 1) {
			fann_error((FannError*)ann, FANN_E_CANT_READ_NEURON, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		neuron_it->first_con = ann->total_connections;
		ann->total_connections += num_connections;
		neuron_it->last_con = ann->total_connections;
	}
	fann_allocate_connections(ann);
	if(ann->IsError(FANN_E_CANT_ALLOCATE_MEM)) {
		fann_destroy(ann);
		return NULL;
	}
	connected_neurons = ann->connections;
	weights = ann->weights;
	first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	for(i = 0; i < ann->total_connections; i++) {
		if(fscanf(conf, "(%u " FANNSCANF ") ", &input_neuron, &weights[i]) != 2) {
			fann_error((FannError*)ann, FANN_E_CANT_READ_CONNECTIONS, configuration_file);
			fann_destroy(ann);
			return NULL;
		}
		connected_neurons[i] = first_neuron + input_neuron;
	}
	fann_set_activation_steepness_hidden(ann, activation_steepness_hidden);
	fann_set_activation_steepness_output(ann, activation_steepness_output);
	fann_set_activation_function_hidden(ann, (enum fann_activationfunc_enum)activation_function_hidden);
	fann_set_activation_function_output(ann, (enum fann_activationfunc_enum)activation_function_output);
#ifdef DEBUG
	printf("output\n");
#endif
	return ann;
}
//
// FANN_TRAIN
//
#ifndef FIXEDFANN
/* INTERNAL FUNCTION
   Calculates the derived of a value, given an activation function
   and a steepness
 */
fann_type fann_activation_derived(uint activation_function, fann_type steepness, fann_type value, fann_type sum)
{
	switch(activation_function) {
		case FANN_LINEAR:
		case FANN_LINEAR_PIECE:
		case FANN_LINEAR_PIECE_SYMMETRIC:
		    return (fann_type)fann_linear_derive(steepness, value);
		case FANN_SIGMOID:
		case FANN_SIGMOID_STEPWISE:
		    value = fann_clip(value, 0.01f, 0.99f);
		    return (fann_type)fann_sigmoid_derive(steepness, value);
		case FANN_SIGMOID_SYMMETRIC:
		case FANN_SIGMOID_SYMMETRIC_STEPWISE:
		    value = fann_clip(value, -0.98f, 0.98f);
		    return (fann_type)fann_sigmoid_symmetric_derive(steepness, value);
		case FANN_GAUSSIAN:
		    /* value = fann_clip(value, 0.01f, 0.99f); */
		    return (fann_type)fann_gaussian_derive(steepness, value, sum);
		case FANN_GAUSSIAN_SYMMETRIC:
		    /* value = fann_clip(value, -0.98f, 0.98f); */
		    return (fann_type)fann_gaussian_symmetric_derive(steepness, value, sum);
		case FANN_ELLIOT:
		    value = fann_clip(value, 0.01f, 0.99f);
		    return (fann_type)fann_elliot_derive(steepness, value, sum);
		case FANN_ELLIOT_SYMMETRIC:
		    value = fann_clip(value, -0.98f, 0.98f);
		    return (fann_type)fann_elliot_symmetric_derive(steepness, value, sum);
		case FANN_SIN_SYMMETRIC:
		    return (fann_type)fann_sin_symmetric_derive(steepness, sum);
		case FANN_COS_SYMMETRIC:
		    return (fann_type)fann_cos_symmetric_derive(steepness, sum);
		case FANN_SIN:
		    return (fann_type)fann_sin_derive(steepness, sum);
		case FANN_COS:
		    return (fann_type)fann_cos_derive(steepness, sum);
		case FANN_THRESHOLD:
		    fann_error(NULL, FANN_E_CANT_TRAIN_ACTIVATION);
	}
	return 0;
}

/* INTERNAL FUNCTION
   Calculates the activation of a value, given an activation function
   and a steepness
 */
fann_type fann_activation(Fann * ann, uint activation_function, fann_type steepness, fann_type value)
{
	value = fann_mult(steepness, value);
	fann_activation_switch(activation_function, value, value);
	return value;
}
//
// Trains the network with the backpropagation algorithm.
//
FANN_EXTERNAL void FANN_API fann_train(Fann * ann, fann_type * input, fann_type * desired_output)
{
	fann_run(ann, input);
	fann_compute_MSE(ann, desired_output);
	fann_backpropagate_MSE(ann);
	fann_update_weights(ann);
}

FANN_EXTERNAL void FANN_API fann_train_with_output(Fann * ann, fann_type * input, fann_type * desired_output, fann_type * pResult)
{
	fann_type * p_result = fann_run(ann, input);
	if(pResult) {
		memcpy(pResult, p_result, ann->num_output * sizeof(fann_type));
	}
	fann_compute_MSE(ann, desired_output);
	fann_backpropagate_MSE(ann);
	fann_update_weights(ann);
}

#endif

/* INTERNAL FUNCTION
   Helper function to update the MSE value and return a diff which takes symmetric functions into account
 */
fann_type fann_update_MSE(Fann * ann, FannNeuron* neuron, fann_type neuron_diff)
{
	float neuron_diff2;
	switch(neuron->activation_function)
	{
		case FANN_LINEAR_PIECE_SYMMETRIC:
		case FANN_THRESHOLD_SYMMETRIC:
		case FANN_SIGMOID_SYMMETRIC:
		case FANN_SIGMOID_SYMMETRIC_STEPWISE:
		case FANN_ELLIOT_SYMMETRIC:
		case FANN_GAUSSIAN_SYMMETRIC:
		case FANN_SIN_SYMMETRIC:
		case FANN_COS_SYMMETRIC:
		    neuron_diff /= (fann_type)2.0;
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
	neuron_diff2 = (neuron_diff / (float)ann->multiplier) * (neuron_diff / (float)ann->multiplier);
#else
	neuron_diff2 = (float)(neuron_diff * neuron_diff);
#endif
	ann->MSE_value += neuron_diff2;
	/*printf("neuron_diff %f = (%f - %f)[/2], neuron_diff2=%f, sum=%f, MSE_value=%f, num_MSE=%d\n", neuron_diff,
	  *desired_output, neuron_value, neuron_diff2, last_layer_begin->sum, ann->MSE_value, ann->num_MSE); */
	if(fann_abs(neuron_diff) >= ann->bit_fail_limit) {
		ann->num_bit_fail++;
	}
	return neuron_diff;
}

/* Tests the network.
 */
FANN_EXTERNAL fann_type * FANN_API fann_test(Fann * ann, fann_type * input, fann_type * desired_output)
{
	fann_type neuron_value;
	fann_type * output_begin = fann_run(ann, input);
	fann_type * output_it;
	const fann_type * output_end = output_begin + ann->num_output;
	fann_type neuron_diff;
	FannNeuron * output_neuron = (ann->P_LastLayer-1)->P_FirstNeuron;
	/* calculate the error */
	for(output_it = output_begin; output_it != output_end; output_it++) {
		neuron_value = *output_it;
		neuron_diff = (*desired_output - neuron_value);
		neuron_diff = fann_update_MSE(ann, output_neuron, neuron_diff);
		desired_output++;
		output_neuron++;
		ann->num_MSE++;
	}
	return output_begin;
}

/* get the mean square error.
 */
FANN_EXTERNAL float FANN_API fann_get_MSE(Fann * ann)
{
	if(ann->num_MSE) {
		return ann->MSE_value / (float)ann->num_MSE;
	}
	else {
		return 0;
	}
}

FANN_EXTERNAL uint FANN_API fann_get_bit_fail(Fann * ann)
{
	return ann->num_bit_fail;
}

/* reset the mean square error.
 */
FANN_EXTERNAL void FANN_API fann_reset_MSE(Fann * ann)
{
/*printf("resetMSE %d %f\n", ann->num_MSE, ann->MSE_value);*/
	ann->num_MSE = 0;
	ann->MSE_value = 0;
	ann->num_bit_fail = 0;
}

#ifndef FIXEDFANN

/* INTERNAL FUNCTION
    compute the error at the network output
        (usually, after forward propagation of a certain input vector, fann_run)
        the error is a sum of squares for all the output units
        also increments a counter because MSE is an average of such errors

        After this train_errors in the output layer will be set to:
        neuron_value_derived * (desired_output - neuron_value)
 */
void fann_compute_MSE(Fann * ann, fann_type * desired_output)
{
	fann_type neuron_value, neuron_diff, * error_it = 0, * error_begin = 0;
	FannNeuron * last_layer_begin = (ann->P_LastLayer-1)->P_FirstNeuron;
	const FannNeuron * last_layer_end = last_layer_begin + ann->num_output;
	const FannNeuron * first_neuron = ann->P_FirstLayer->P_FirstNeuron;

	/* if no room allocated for the error variabels, allocate it now */
	if(ann->train_errors == NULL) {
		ann->train_errors = (fann_type*)calloc(ann->total_neurons, sizeof(fann_type));
		if(ann->train_errors == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	else {
		/* clear the error variabels */
		memzero(ann->train_errors, (ann->total_neurons) * sizeof(fann_type));
	}
	error_begin = ann->train_errors;
#ifdef DEBUGTRAIN
	printf("\ncalculate errors\n");
#endif
	/* calculate the error and place it in the output layer */
	error_it = error_begin + (last_layer_begin - first_neuron);
	for(; last_layer_begin != last_layer_end; last_layer_begin++) {
		neuron_value = last_layer_begin->value;
		neuron_diff = *desired_output - neuron_value;
		neuron_diff = fann_update_MSE(ann, last_layer_begin, neuron_diff);
		if(ann->train_error_function) {                 /* TODO make switch when more functions */
			if(neuron_diff < -.9999999)
				neuron_diff = -17.0;
			else if(neuron_diff > .9999999)
				neuron_diff = 17.0;
			else
				neuron_diff = (fann_type)log((1.0 + neuron_diff) / (1.0 - neuron_diff));
		}
		*error_it = fann_activation_derived(last_layer_begin->activation_function, last_layer_begin->activation_steepness, neuron_value,
		    last_layer_begin->sum) * neuron_diff;
		desired_output++;
		error_it++;
		ann->num_MSE++;
	}
}
/*
	INTERNAL FUNCTION
   Propagate the error backwards from the output layer.

   After this the train_errors in the hidden layers will be:
   neuron_value_derived * sum(outgoing_weights * connected_neuron)
*/
void fann_backpropagate_MSE(Fann * ann)
{
	fann_type tmp_error;
	uint i;
	FannLayer * layer_it;
	FannNeuron * neuron_it, * last_neuron;
	FannNeuron ** connections;
	fann_type * error_begin = ann->train_errors;
	fann_type * error_prev_layer;
	fann_type * weights;
	const FannNeuron * first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	const FannLayer * second_layer = ann->P_FirstLayer + 1;
	FannLayer * last_layer = ann->P_LastLayer;

	/* go through all the layers, from last to first.
	 * And propagate the error backwards */
	for(layer_it = last_layer - 1; layer_it > second_layer; --layer_it) {
		last_neuron = layer_it->P_LastNeuron;
		/* for each connection in this layer, propagate the error backwards */
		if(ann->connection_rate >= 1) {
			if(ann->network_type == FANN_NETTYPE_LAYER) {
				error_prev_layer = error_begin + ((layer_it - 1)->P_FirstNeuron - first_neuron);
			}
			else {
				error_prev_layer = error_begin;
			}
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - first_neuron];
				weights = ann->weights + neuron_it->first_con;
				for(i = neuron_it->last_con - neuron_it->first_con; i--; ) {
					/*printf("i = %d\n", i);
					 * printf("error_prev_layer[%d] = %f\n", i, error_prev_layer[i]);
					 * printf("weights[%d] = %f\n", i, weights[i]); */
					error_prev_layer[i] += tmp_error * weights[i];
				}
			}
		}
		else {
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - first_neuron];
				weights = ann->weights + neuron_it->first_con;
				connections = ann->connections + neuron_it->first_con;
				for(i = neuron_it->last_con - neuron_it->first_con; i--; ) {
					error_begin[connections[i] - first_neuron] += tmp_error * weights[i];
				}
			}
		}
		/* then calculate the actual errors in the previous layer */
		error_prev_layer = error_begin + ((layer_it - 1)->P_FirstNeuron - first_neuron);
		last_neuron = (layer_it - 1)->P_LastNeuron;
		for(neuron_it = (layer_it - 1)->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
			*error_prev_layer *= fann_activation_derived(neuron_it->activation_function, neuron_it->activation_steepness, neuron_it->value, neuron_it->sum);
			error_prev_layer++;
		}
	}
}

/* INTERNAL FUNCTION
   Update weights for incremental training
 */
void fann_update_weights(Fann * ann)
{
	FannNeuron * neuron_it, * last_neuron, * prev_neurons;
	fann_type tmp_error, delta_w, * weights;
	FannLayer * layer_it;
	uint i;
	uint num_connections;
	/* store some variabels local for fast access */
	const float learning_rate = ann->learning_rate;
	const float learning_momentum = ann->learning_momentum;
	FannNeuron * p_first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	FannLayer * p_first_layer = ann->P_FirstLayer;
	const FannLayer * last_layer = ann->P_LastLayer;
	fann_type * error_begin = ann->train_errors;
	fann_type * deltas_begin, * weights_deltas;
	/* if no room allocated for the deltas, allocate it now */
	if(ann->prev_weights_deltas == NULL) {
		ann->prev_weights_deltas = (fann_type*)calloc(ann->total_connections_allocated, sizeof(fann_type));
		if(ann->prev_weights_deltas == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
#ifdef DEBUGTRAIN
	printf("\nupdate weights\n");
#endif
	deltas_begin = ann->prev_weights_deltas;
	prev_neurons = p_first_neuron;
	for(layer_it = (p_first_layer + 1); layer_it != last_layer; layer_it++) {
#ifdef DEBUGTRAIN
		printf("layer[%d]\n", layer_it - first_layer);
#endif
		last_neuron = layer_it->P_LastNeuron;
		if(ann->connection_rate >= 1) {
			if(ann->network_type == FANN_NETTYPE_LAYER) {
				prev_neurons = (layer_it - 1)->P_FirstNeuron;
			}
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - p_first_neuron] * learning_rate;
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
			for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - p_first_neuron] * learning_rate;
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
// INTERNAL FUNCTION
// Update slopes for batch training
// layer_begin = ann->P_FirstLayer+1 and layer_end = ann->last_layer-1 will update all slopes.
//
void fann_update_slopes_batch(Fann * ann, FannLayer * layer_begin, FannLayer * layer_end)
{
	FannNeuron * neuron_it, * last_neuron, * prev_neurons, ** connections;
	fann_type tmp_error;
	uint i, num_connections;
	// store some variabels local for fast access
	FannNeuron * first_neuron = ann->P_FirstLayer->P_FirstNeuron;
	fann_type * error_begin = ann->train_errors;
	fann_type * slope_begin, * neuron_slope;
	// if no room allocated for the slope variabels, allocate it now
	if(ann->train_slopes == NULL) {
		ann->train_slopes = (fann_type*)calloc(ann->total_connections_allocated, sizeof(fann_type));
		if(ann->train_slopes == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	SETIFZ(layer_begin, (ann->P_FirstLayer + 1));
	SETIFZ(layer_end, (ann->P_LastLayer - 1));
	slope_begin = ann->train_slopes;
#ifdef DEBUGTRAIN
	printf("\nupdate slopes\n");
#endif
	prev_neurons = first_neuron;
	for(; layer_begin <= layer_end; layer_begin++) {
#ifdef DEBUGTRAIN
		printf("layer[%d]\n", layer_begin - ann->P_FirstLayer);
#endif
		last_neuron = layer_begin->P_LastNeuron;
		if(ann->connection_rate >= 1) {
			if(ann->network_type == FANN_NETTYPE_LAYER) {
				prev_neurons = (layer_begin - 1)->P_FirstNeuron;
			}
			for(neuron_it = layer_begin->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
				tmp_error = error_begin[neuron_it - first_neuron];
				neuron_slope = slope_begin + neuron_it->first_con;
				num_connections = neuron_it->last_con - neuron_it->first_con;
				for(i = 0; i != num_connections; i++) {
					neuron_slope[i] += tmp_error * prev_neurons[i].value;
				}
			}
		}
		else {
			for(neuron_it = layer_begin->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
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
void fann_clear_train_arrays(Fann * ann)
{
	uint i;
	fann_type delta_zero;
	// if no room allocated for the slope variabels, allocate it now (calloc clears mem)
	if(ann->train_slopes == NULL) {
		ann->train_slopes = (fann_type*)calloc(ann->total_connections_allocated, sizeof(fann_type));
		if(ann->train_slopes == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	else {
		memzero(ann->train_slopes, (ann->total_connections_allocated) * sizeof(fann_type));
	}
	/* if no room allocated for the variabels, allocate it now */
	if(ann->prev_steps == NULL) {
		ann->prev_steps = (fann_type*)malloc(ann->total_connections_allocated * sizeof(fann_type));
		if(ann->prev_steps == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	if(ann->training_algorithm == FANN_TRAIN_RPROP) {
		delta_zero = ann->rprop_delta_zero;
		for(i = 0; i < ann->total_connections_allocated; i++)
			ann->prev_steps[i] = delta_zero;
	}
	else {
		memzero(ann->prev_steps, (ann->total_connections_allocated) * sizeof(fann_type));
	}
	/* if no room allocated for the variabels, allocate it now */
	if(ann->prev_train_slopes == NULL) {
		ann->prev_train_slopes = (fann_type*)calloc(ann->total_connections_allocated, sizeof(fann_type));
		if(ann->prev_train_slopes == NULL) {
			fann_error((FannError*)ann, FANN_E_CANT_ALLOCATE_MEM);
			return;
		}
	}
	else {
		memzero(ann->prev_train_slopes, (ann->total_connections_allocated) * sizeof(fann_type));
	}
}

/* INTERNAL FUNCTION
   Update weights for batch training
 */
void fann_update_weights_batch(Fann * ann, uint num_data, uint first_weight, uint past_end)
{
	fann_type * train_slopes = ann->train_slopes;
	fann_type * weights = ann->weights;
	const float epsilon = ann->learning_rate / num_data;
	for(uint i = first_weight; i != past_end; i++) {
		weights[i] += train_slopes[i] * epsilon;
		train_slopes[i] = 0.0;
	}
}
//
// INTERNAL FUNCTION
// The quickprop training algorithm
//
void fann_update_weights_quickprop(Fann * ann, uint num_data, uint first_weight, uint past_end)
{
	fann_type * train_slopes = ann->train_slopes;
	fann_type * weights = ann->weights;
	fann_type * prev_steps = ann->prev_steps;
	fann_type * prev_train_slopes = ann->prev_train_slopes;
	fann_type w, prev_step, slope, prev_slope, next_step;
	float epsilon = ann->learning_rate / num_data;
	float decay = ann->quickprop_decay;     /*-0.0001;*/
	float mu = ann->quickprop_mu;   /*1.75; */
	float shrink_factor = (float)(mu / (1.0 + mu));
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
				next_step += prev_step * slope / (prev_slope - slope);  /* Else, use quadratic estimate. */
		}
		else if(prev_step < -0.001) {
			/* If last step was negative...  */
			if(slope < 0.0) /*  Add in linear term if current slope is still negative. */
				next_step += epsilon * slope;
			/* If current slope is close to or more neg than prev slope... */
			if(slope < (shrink_factor * prev_slope))
				next_step += mu * prev_step;    /* Take maximum size negative step. */
			else
				next_step += prev_step * slope / (prev_slope - slope);  /* Else, use quadratic estimate. */
		}
		else /* Last step was zero, so use only linear term. */
			next_step += epsilon * slope;
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
		prev_steps[i] = next_step;
		w += next_step;
		if(w > 1500)
			weights[i] = 1500;
		else if(w < -1500)
			weights[i] = -1500;
		else
			weights[i] = w;
		// weights[i] = w;
		prev_train_slopes[i] = slope;
		train_slopes[i] = 0.0;
	}
}
//
// INTERNAL FUNCTION
// The iRprop- algorithm
//
void fann_update_weights_irpropm(Fann * ann, uint first_weight, uint past_end)
{
	fann_type * train_slopes = ann->train_slopes;
	fann_type * weights = ann->weights;
	fann_type * prev_steps = ann->prev_steps;
	fann_type * prev_train_slopes = ann->prev_train_slopes;
	fann_type prev_step, slope, prev_slope, next_step, same_sign;
	float increase_factor = ann->rprop_increase_factor;     /*1.2; */
	float decrease_factor = ann->rprop_decrease_factor;     /*0.5; */
	float delta_min = ann->rprop_delta_min; /*0.0; */
	float delta_max = ann->rprop_delta_max; /*50.0; */
	uint i = first_weight;
	for(; i != past_end; i++) {
		prev_step = MAX(prev_steps[i], (fann_type)0.0001); // prev_step may not be zero because then the training will stop
		slope = train_slopes[i];
		prev_slope = prev_train_slopes[i];
		same_sign = prev_slope * slope;
		if(same_sign >= 0.0)
			next_step = MIN(prev_step * increase_factor, delta_max);
		else {
			next_step = MAX(prev_step * decrease_factor, delta_min);
			slope = 0;
		}
		if(slope < 0) {
			weights[i] -= next_step;
			SETMAX(weights[i], -1500);
		}
		else {
			weights[i] += next_step;
			SETMIN(weights[i], 1500);
		}
		/*if(i == 2){
		 * printf("weight=%f, slope=%f, next_step=%f, prev_step=%f\n", weights[i], slope, next_step, prev_step);
		 * } */

		/* update global data arrays */
		prev_steps[i] = next_step;
		prev_train_slopes[i] = slope;
		train_slopes[i] = 0.0;
	}
}
//
// INTERNAL FUNCTION
// The SARprop- algorithm
//
void fann_update_weights_sarprop(Fann * ann, uint epoch, uint first_weight, uint past_end)
{
	fann_type * train_slopes = ann->train_slopes;
	fann_type * weights = ann->weights;
	fann_type * prev_steps = ann->prev_steps;
	fann_type * prev_train_slopes = ann->prev_train_slopes;
	fann_type prev_step, slope, prev_slope, next_step = 0, same_sign;
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
		prev_step = MAX(prev_steps[i], (fann_type)0.000001); // prev_step may not be zero because then the training will stop
		/* calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)*/
		slope = -train_slopes[i] - weights[i] * (fann_type)fann_exp2(-T * epoch + weight_decay_shift);
		/* TODO: is prev_train_slopes[i] 0.0 in the beginning? */
		prev_slope = prev_train_slopes[i];
		same_sign = prev_slope * slope;
		if(same_sign > 0.0) {
			next_step = MIN(prev_step * increase_factor, delta_max);
			/* TODO: are the signs inverted? see differences between SARPROP paper and iRprop */
			if(slope < 0.0)
				weights[i] += next_step;
			else
				weights[i] -= next_step;
		}
		else if(same_sign < 0.0) {
			if(prev_step < step_error_threshold_factor * MSE)
				next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (fann_type)fann_exp2(-T * epoch + step_error_shift);
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
		/*if(i == 2){
		 * printf("weight=%f, slope=%f, next_step=%f, prev_step=%f\n", weights[i], slope, next_step, prev_step);
		 * } */

		/* update global data arrays */
		prev_steps[i] = next_step;
		prev_train_slopes[i] = slope;
		train_slopes[i] = 0.0;
	}
}

#endif

FANN_GET_SET(enum fann_train_enum, training_algorithm)
FANN_GET_SET(float, learning_rate)

FANN_EXTERNAL void FANN_API fann_set_activation_function_hidden(Fann * ann, enum fann_activationfunc_enum activation_function)
{
	FannNeuron * last_neuron, * neuron_it;
	FannLayer * last_layer = ann->P_LastLayer - 1;    /* -1 to not update the output layer */
	for(FannLayer * layer_it = ann->P_FirstLayer + 1; layer_it != last_layer; layer_it++) {
		last_neuron = layer_it->P_LastNeuron;
		for(neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_function = activation_function;
		}
	}
}

FANN_EXTERNAL FannLayer* FANN_API fann_get_layer(Fann * ann, int layer)
{
	if(layer <= 0 || layer >= (ann->P_LastLayer - ann->P_FirstLayer)) {
		fann_error((FannError*)ann, FANN_E_INDEX_OUT_OF_BOUND, layer);
		return NULL;
	}
	return ann->P_FirstLayer + layer;
}

FANN_EXTERNAL FannNeuron* FANN_API fann_get_neuron_layer(Fann * ann, FannLayer* layer, int neuron)
{
	if(neuron >= (layer->P_LastNeuron - layer->P_FirstNeuron)) {
		fann_error((FannError*)ann, FANN_E_INDEX_OUT_OF_BOUND, neuron);
		return NULL;
	}
	else
		return layer->P_FirstNeuron + neuron;
}

FANN_EXTERNAL FannNeuron* FANN_API fann_get_neuron(Fann * ann, uint layer, int neuron)
{
	FannLayer * layer_it = fann_get_layer(ann, layer);
	return layer_it ? fann_get_neuron_layer(ann, layer_it, neuron) : 0;
}

FANN_EXTERNAL enum fann_activationfunc_enum FANN_API fann_get_activation_function(Fann * ann, int layer, int neuron)
{
	FannNeuron* neuron_it = fann_get_neuron(ann, layer, neuron);
	if(neuron_it == NULL) {
		return (enum fann_activationfunc_enum)-1; /* layer or neuron out of bounds */
	}
	else {
		return neuron_it->activation_function;
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_function(Fann * ann,
    enum fann_activationfunc_enum activation_function, int layer, int neuron)
{
	FannNeuron * neuron_it = fann_get_neuron(ann, layer, neuron);
	if(neuron_it)
		neuron_it->activation_function = activation_function;
}

FANN_EXTERNAL void FANN_API fann_set_activation_function_layer(Fann * ann,
    enum fann_activationfunc_enum activation_function, int layer)
{
	FannLayer * layer_it = fann_get_layer(ann, layer);
	if(layer_it) {
		FannNeuron * last_neuron = layer_it->P_LastNeuron;
		for(FannNeuron * neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_function = activation_function;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_function_output(Fann * ann, enum fann_activationfunc_enum activation_function)
{
	FannNeuron * last_neuron, * neuron_it;
	FannLayer * last_layer = ann->P_LastLayer - 1;
	last_neuron = last_layer->P_LastNeuron;
	for(neuron_it = last_layer->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
		neuron_it->activation_function = activation_function;
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness_hidden(Fann * ann, fann_type steepness)
{
	FannLayer * last_layer = ann->P_LastLayer - 1; // -1 to not update the output layer
	for(FannLayer * layer_it = ann->P_FirstLayer + 1; layer_it != last_layer; layer_it++) {
		FannNeuron * last_neuron = layer_it->P_LastNeuron;
		for(FannNeuron * neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_steepness = steepness;
		}
	}
}

FANN_EXTERNAL fann_type FANN_API fann_get_activation_steepness(Fann * ann, int layer, int neuron)
{
	FannNeuron * neuron_it = fann_get_neuron(ann, layer, neuron);
	if(neuron_it == NULL) {
		return -1; // layer or neuron out of bounds
	}
	else {
		return neuron_it->activation_steepness;
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness(Fann * ann, fann_type steepness, int layer, int neuron)
{
	FannNeuron * neuron_it = fann_get_neuron(ann, layer, neuron);
	if(neuron_it)
		neuron_it->activation_steepness = steepness;
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness_layer(Fann * ann, fann_type steepness, int layer)
{
	FannLayer * layer_it = fann_get_layer(ann, layer);
	if(layer_it) {
		FannNeuron * last_neuron = layer_it->P_LastNeuron;
		for(FannNeuron * neuron_it = layer_it->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
			neuron_it->activation_steepness = steepness;
		}
	}
}

FANN_EXTERNAL void FANN_API fann_set_activation_steepness_output(Fann * ann, fann_type steepness)
{
	FannLayer * last_layer = ann->P_LastLayer - 1;
	FannNeuron * last_neuron = last_layer->P_LastNeuron;
	for(FannNeuron * neuron_it = last_layer->P_FirstNeuron; neuron_it != last_neuron; neuron_it++) {
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
FANN_GET_SET(fann_type, bit_fail_limit)
FANN_GET_SET(float, learning_momentum)
//
// FANN_TRAIN_DATA
//
//
// Reads training data from a file.
//
FANN_EXTERNAL FannTrainData * FANN_API fann_read_train_from_file(const char * configuration_file)
{
	FannTrainData * data = 0;
	FILE * file = fopen(configuration_file, "r");
	if(!file) {
		fann_error(NULL, FANN_E_CANT_OPEN_CONFIG_R, configuration_file);
	}
	else {
		data = fann_read_train_from_fd(file, configuration_file);
		fclose(file);
	}
	return data;
}
/*
 * Save training data to a file
 */
FANN_EXTERNAL int FANN_API fann_save_train(FannTrainData * data, const char * filename)
{
	return fann_save_train_internal(data, filename, 0, 0);
}

/*
 * Save training data to a file in fixed point algebra. (Good for testing
 * a network in fixed point)
 */
FANN_EXTERNAL int FANN_API fann_save_train_to_fixed(FannTrainData * data, const char * filename, uint decimal_point)
{
	return fann_save_train_internal(data, filename, 1, decimal_point);
}

/*
 * deallocate the train data structure.
 */
FANN_EXTERNAL void FANN_API fann_destroy_train(FannTrainData * data)
{
	if(data == NULL)
		return;
	if(data->input != NULL)
		ZFREE(data->input[0]);
	if(data->output != NULL)
		ZFREE(data->output[0]);
	ZFREE(data->input);
	ZFREE(data->output);
	ZFREE(data);
}

/*
 * Test a set of training data and calculate the MSE
 */
FANN_EXTERNAL float FANN_API fann_test_data(Fann * ann, FannTrainData * data)
{
	uint i;
	if(fann_check_input_output_sizes(ann, data) == -1)
		return 0;
	fann_reset_MSE(ann);
	for(i = 0; i != data->num_data; i++) {
		fann_test(ann, data->input[i], data->output[i]);
	}
	return fann_get_MSE(ann);
}

#ifndef FIXEDFANN

/*
 * Internal train function
 */
float fann_train_epoch_quickprop(Fann * ann, FannTrainData * data)
{
	uint i;
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	fann_reset_MSE(ann);
	for(i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->P_FirstLayer + 1, ann->P_LastLayer-1);
	}
	fann_update_weights_quickprop(ann, data->num_data, 0, ann->total_connections);
	return fann_get_MSE(ann);
}

/*
 * Internal train function
 */
float fann_train_epoch_irpropm(Fann * ann, FannTrainData * data)
{
	uint i;
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	fann_reset_MSE(ann);
	for(i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->P_FirstLayer + 1, ann->P_LastLayer-1);
	}
	fann_update_weights_irpropm(ann, 0, ann->total_connections);
	return fann_get_MSE(ann);
}
/*
 * Internal train function
 */
float fann_train_epoch_sarprop(Fann * ann, FannTrainData * data)
{
	uint i;
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	fann_reset_MSE(ann);
	for(i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->P_FirstLayer + 1, ann->P_LastLayer-1);
	}
	fann_update_weights_sarprop(ann, ann->sarprop_epoch, 0, ann->total_connections);
	++(ann->sarprop_epoch);
	return fann_get_MSE(ann);
}

/*
 * Internal train function
 */
float fann_train_epoch_batch(Fann * ann, FannTrainData * data)
{
	uint i;
	fann_reset_MSE(ann);
	for(i = 0; i < data->num_data; i++) {
		fann_run(ann, data->input[i]);
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_slopes_batch(ann, ann->P_FirstLayer + 1, ann->P_LastLayer-1);
	}
	fann_update_weights_batch(ann, data->num_data, 0, ann->total_connections);
	return fann_get_MSE(ann);
}
/*
 * Internal train function
 */
float fann_train_epoch_incremental(Fann * ann, FannTrainData * data)
{
	fann_reset_MSE(ann);
	for(uint i = 0; i != data->num_data; i++) {
		fann_train(ann, data->input[i], data->output[i]);
	}
	return fann_get_MSE(ann);
}

/*
 * Train for one epoch with the selected training algorithm
 */
FANN_EXTERNAL float FANN_API fann_train_epoch(Fann * ann, FannTrainData * data)
{
	if(fann_check_input_output_sizes(ann, data) == -1)
		return 0;
	switch(ann->training_algorithm) {
		case FANN_TRAIN_QUICKPROP:   return fann_train_epoch_quickprop(ann, data);
		case FANN_TRAIN_RPROP:       return fann_train_epoch_irpropm(ann, data);
		case FANN_TRAIN_SARPROP:     return fann_train_epoch_sarprop(ann, data);
		case FANN_TRAIN_BATCH:       return fann_train_epoch_batch(ann, data);
		case FANN_TRAIN_INCREMENTAL: return fann_train_epoch_incremental(ann, data);
	}
	return 0;
}

FANN_EXTERNAL void FANN_API fann_train_on_data(Fann * ann, FannTrainData * data,
    uint max_epochs, uint epochs_between_reports, float desired_error)
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
		/*
		 * train
		 */
		error = fann_train_epoch(ann, data);
		desired_error_reached = fann_desired_error_reached(ann, desired_error);
		/*
		 * print current output
		 */
		if(epochs_between_reports && (i % epochs_between_reports == 0 || i == max_epochs || i == 1 || desired_error_reached == 0)) {
			if(ann->callback == NULL) {
				printf("Epochs     %8d. Current error: %.10f. Bit fail %d.\n", i, error, ann->num_bit_fail);
			}
			else if(((*ann->callback)(ann, data, max_epochs, epochs_between_reports, desired_error, i)) == -1) {
				/*
				 * you can break the training by returning -1
				 */
				break;
			}
		}
		if(desired_error_reached == 0)
			break;
	}
}

FANN_EXTERNAL void FANN_API fann_train_on_file(Fann * ann, const char * filename,
    uint max_epochs, uint epochs_between_reports, float desired_error)
{
	FannTrainData * data = fann_read_train_from_file(filename);
	if(data) {
		fann_train_on_data(ann, data, max_epochs, epochs_between_reports, desired_error);
		fann_destroy_train(data);
	}
}

#endif

/*
 * shuffles training data, randomizing the order
 */
FANN_EXTERNAL void FANN_API fann_shuffle_train_data(FannTrainData * train_data)
{
	for(uint dat = 0; dat < train_data->num_data; dat++) {
		uint   swap = (uint)(rand() % train_data->num_data);
		if(swap != dat) {
			uint   elem;
			for(elem = 0; elem < train_data->num_input; elem++) {
				const fann_type temp = train_data->input[dat][elem];
				train_data->input[dat][elem] = train_data->input[swap][elem];
				train_data->input[swap][elem] = temp;
			}
			for(elem = 0; elem < train_data->num_output; elem++) {
				const fann_type temp = train_data->output[dat][elem];
				train_data->output[dat][elem] = train_data->output[swap][elem];
				train_data->output[swap][elem] = temp;
			}
		}
	}
}
/*
 * INTERNAL FUNCTION calculates min and max of train data
 */
void fann_get_min_max_data(fann_type ** data, uint num_data, uint num_elem, fann_type * min, fann_type * max)
{
	fann_type temp;
	uint dat, elem;
	*min = *max = data[0][0];
	for(dat = 0; dat < num_data; dat++) {
		for(elem = 0; elem < num_elem; elem++) {
			temp = data[dat][elem];
			if(temp < *min)
				*min = temp;
			else if(temp > *max)
				*max = temp;
		}
	}
}

FANN_EXTERNAL fann_type FANN_API fann_get_min_train_input(FannTrainData * train_data)
{
	fann_type min, max;
	fann_get_min_max_data(train_data->input, train_data->num_data, train_data->num_input, &min, &max);
	return min;
}

FANN_EXTERNAL fann_type FANN_API fann_get_max_train_input(FannTrainData * train_data)
{
	fann_type min, max;
	fann_get_min_max_data(train_data->input, train_data->num_data, train_data->num_input, &min, &max);
	return max;
}

FANN_EXTERNAL fann_type FANN_API fann_get_min_train_output(FannTrainData * train_data)
{
	fann_type min, max;
	fann_get_min_max_data(train_data->output, train_data->num_data, train_data->num_output, &min, &max);
	return min;
}

FANN_EXTERNAL fann_type FANN_API fann_get_max_train_output(FannTrainData * train_data)
{
	fann_type min, max;
	fann_get_min_max_data(train_data->output, train_data->num_data, train_data->num_output, &min, &max);
	return max;
}

/*
 * INTERNAL FUNCTION Scales data to a specific range
 */
void fann_scale_data(fann_type ** data, uint num_data, uint num_elem, fann_type new_min, fann_type new_max)
{
	fann_type old_min, old_max;
	fann_get_min_max_data(data, num_data, num_elem, &old_min, &old_max);
	fann_scale_data_to_range(data, num_data, num_elem, old_min, old_max, new_min, new_max);
}

/*
 * INTERNAL FUNCTION Scales data to a specific range
 */
FANN_EXTERNAL void FANN_API fann_scale_data_to_range(fann_type ** data, uint num_data, uint num_elem,
    fann_type old_min, fann_type old_max, fann_type new_min, fann_type new_max)
{
	const fann_type old_span = old_max - old_min;
	const fann_type new_span = new_max - new_min;
	const fann_type factor = new_span / old_span;
	// printf("max %f, min %f, factor %f\n", old_max, old_min, factor);
	for(uint dat = 0; dat < num_data; dat++) {
		for(uint elem = 0; elem < num_elem; elem++) {
			const fann_type temp = (data[dat][elem] - old_min) * factor + new_min;
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

/*
 * Scales the inputs in the training data to the specified range
 */
FANN_EXTERNAL void FANN_API fann_scale_input_train_data(FannTrainData * train_data, fann_type new_min, fann_type new_max)
{
	fann_scale_data(train_data->input, train_data->num_data, train_data->num_input, new_min, new_max);
}
/*
 * Scales the inputs in the training data to the specified range
 */
FANN_EXTERNAL void FANN_API fann_scale_output_train_data(FannTrainData * train_data, fann_type new_min, fann_type new_max)
{
	fann_scale_data(train_data->output, train_data->num_data, train_data->num_output, new_min, new_max);
}
/*
 * Scales the inputs in the training data to the specified range
 */
FANN_EXTERNAL void FANN_API fann_scale_train_data(FannTrainData * train_data, fann_type new_min, fann_type new_max)
{
	fann_scale_data(train_data->input, train_data->num_data, train_data->num_input, new_min, new_max);
	fann_scale_data(train_data->output, train_data->num_data, train_data->num_output, new_min, new_max);
}

/*
 * merges training data into a single struct.
 */
FANN_EXTERNAL FannTrainData * FANN_API fann_merge_train_data(FannTrainData * data1, FannTrainData * data2)
{
	uint i;
	fann_type * data_input, * data_output;
	FannTrainData * dest = (FannTrainData*)malloc(sizeof(FannTrainData));
	if(dest == NULL) {
		fann_error((FannError*)data1, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	if((data1->num_input != data2->num_input) || (data1->num_output != data2->num_output)) {
		fann_error((FannError*)data1, FANN_E_TRAIN_DATA_MISMATCH);
		return NULL;
	}
	fann_init_error_data((FannError*)dest);
	dest->error_log = data1->error_log;
	dest->num_data = data1->num_data+data2->num_data;
	dest->num_input = data1->num_input;
	dest->num_output = data1->num_output;
	dest->input = (fann_type**)calloc(dest->num_data, sizeof(fann_type *));
	if(dest->input == NULL) {
		fann_error((FannError*)data1, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	dest->output = (fann_type**)calloc(dest->num_data, sizeof(fann_type *));
	if(dest->output == NULL) {
		fann_error((FannError*)data1, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	data_input = (fann_type*)calloc(dest->num_input * dest->num_data, sizeof(fann_type));
	if(data_input == NULL) {
		fann_error((FannError*)data1, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_input, data1->input[0], dest->num_input * data1->num_data * sizeof(fann_type));
	memcpy(data_input + (dest->num_input*data1->num_data),
	    data2->input[0], dest->num_input * data2->num_data * sizeof(fann_type));
	data_output = (fann_type*)calloc(dest->num_output * dest->num_data, sizeof(fann_type));
	if(data_output == NULL) {
		fann_error((FannError*)data1, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_output, data1->output[0], dest->num_output * data1->num_data * sizeof(fann_type));
	memcpy(data_output + (dest->num_output*data1->num_data),
	    data2->output[0], dest->num_output * data2->num_data * sizeof(fann_type));
	for(i = 0; i != dest->num_data; i++) {
		dest->input[i] = data_input;
		data_input += dest->num_input;
		dest->output[i] = data_output;
		data_output += dest->num_output;
	}
	return dest;
}

/*
 * return a copy of a fann_train_data struct
 */
FANN_EXTERNAL FannTrainData * FANN_API fann_duplicate_train_data(FannTrainData * data)
{
	uint i;
	fann_type * data_input, * data_output;
	FannTrainData * dest = (FannTrainData*)malloc(sizeof(FannTrainData));
	if(dest == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	fann_init_error_data((FannError*)dest);
	dest->error_log = data->error_log;
	dest->num_data = data->num_data;
	dest->num_input = data->num_input;
	dest->num_output = data->num_output;
	dest->input = (fann_type**)calloc(dest->num_data, sizeof(fann_type *));
	if(dest->input == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	dest->output = (fann_type**)calloc(dest->num_data, sizeof(fann_type *));
	if(dest->output == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	data_input = (fann_type*)calloc(dest->num_input * dest->num_data, sizeof(fann_type));
	if(data_input == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_input, data->input[0], dest->num_input * dest->num_data * sizeof(fann_type));
	data_output = (fann_type*)calloc(dest->num_output * dest->num_data, sizeof(fann_type));
	if(data_output == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_output, data->output[0], dest->num_output * dest->num_data * sizeof(fann_type));
	for(i = 0; i != dest->num_data; i++) {
		dest->input[i] = data_input;
		data_input += dest->num_input;
		dest->output[i] = data_output;
		data_output += dest->num_output;
	}
	return dest;
}

FANN_EXTERNAL FannTrainData * FANN_API fann_subset_train_data(FannTrainData * data, uint pos, uint length)
{
	uint i;
	fann_type * data_input, * data_output;
	FannTrainData * dest = (FannTrainData*)malloc(sizeof(FannTrainData));
	if(dest == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	if(pos > data->num_data || pos+length > data->num_data) {
		fann_error((FannError*)data, FANN_E_TRAIN_DATA_SUBSET, pos, length, data->num_data);
		return NULL;
	}
	fann_init_error_data((FannError*)dest);
	dest->error_log = data->error_log;
	dest->num_data = length;
	dest->num_input = data->num_input;
	dest->num_output = data->num_output;
	dest->input = (fann_type**)calloc(dest->num_data, sizeof(fann_type *));
	if(dest->input == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	dest->output = (fann_type**)calloc(dest->num_data, sizeof(fann_type *));
	if(dest->output == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	data_input = (fann_type*)calloc(dest->num_input * dest->num_data, sizeof(fann_type));
	if(data_input == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_input, data->input[pos], dest->num_input * dest->num_data * sizeof(fann_type));
	data_output = (fann_type*)calloc(dest->num_output * dest->num_data, sizeof(fann_type));
	if(data_output == NULL) {
		fann_error((FannError*)data, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(dest);
		return NULL;
	}
	memcpy(data_output, data->output[pos], dest->num_output * dest->num_data * sizeof(fann_type));
	for(i = 0; i != dest->num_data; i++) {
		dest->input[i] = data_input;
		data_input += dest->num_input;
		dest->output[i] = data_output;
		data_output += dest->num_output;
	}
	return dest;
}

FANN_EXTERNAL uint FANN_API fann_length_train_data(FannTrainData * data)
{
	return data->num_data;
}

FANN_EXTERNAL uint FANN_API fann_num_input_train_data(FannTrainData * data)
{
	return data->num_input;
}

FANN_EXTERNAL uint FANN_API fann_num_output_train_data(FannTrainData * data)
{
	return data->num_output;
}
//
// INTERNAL FUNCTION
// Save the train data structure.
//
int fann_save_train_internal(FannTrainData * data, const char * filename, uint save_as_fixed, uint decimal_point)
{
	int retval = 0;
	FILE * file = fopen(filename, "w");
	if(!file) {
		fann_error((FannError*)data, FANN_E_CANT_OPEN_TD_W, filename);
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
int fann_save_train_internal_fd(FannTrainData * data, FILE * file, const char * filename,
    uint save_as_fixed, uint decimal_point)
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
			else{
				if(((int)floor(data->input[i][j] + 0.5) * 1000000) == ((int)floor(data->input[i][j] * 1000000.0 + 0.5))) {
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
			else{
				if(((int)floor(data->output[i][j] + 0.5) * 1000000) == ((int)floor(data->output[i][j] * 1000000.0 + 0.5))) {
					fprintf(file, "%d ", (int)data->output[i][j]);
				}
				else{
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
// Creates an empty set of training data
//
FANN_EXTERNAL FannTrainData * FANN_API fann_create_train(uint num_data, uint num_input, uint num_output)
{
	fann_type * data_input, * data_output;
	uint i;
	FannTrainData * data = (FannTrainData*)malloc(sizeof(FannTrainData));
	if(data == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		return NULL;
	}
	fann_init_error_data((FannError*)data);
	data->num_data = num_data;
	data->num_input = num_input;
	data->num_output = num_output;
	data->input = (fann_type**)calloc(num_data, sizeof(fann_type *));
	if(data->input == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(data);
		return NULL;
	}
	data->output = (fann_type**)calloc(num_data, sizeof(fann_type *));
	if(data->output == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(data);
		return NULL;
	}
	data_input = (fann_type*)calloc(num_input * num_data, sizeof(fann_type));
	if(data_input == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
		fann_destroy_train(data);
		return NULL;
	}
	data_output = (fann_type*)calloc(num_output * num_data, sizeof(fann_type));
	if(data_output == NULL) {
		fann_error(NULL, FANN_E_CANT_ALLOCATE_MEM);
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

FANN_EXTERNAL FannTrainData * FANN_API fann_create_train_pointer_array(uint num_data,
    uint num_input, fann_type ** input, uint num_output, fann_type ** output)
{
	FannTrainData * data = fann_create_train(num_data, num_input, num_output);
	if(data) {
		for(uint i = 0; i < num_data; ++i) {
			memcpy(data->input[i], input[i], num_input*sizeof(fann_type));
			memcpy(data->output[i], output[i], num_output*sizeof(fann_type));
		}
	}
	return data;
}

FANN_EXTERNAL FannTrainData * FANN_API fann_create_train_array(uint num_data,
    uint num_input, fann_type * input, uint num_output, fann_type * output)
{
	FannTrainData * data = fann_create_train(num_data, num_input, num_output);
	if(data) {
		for(uint i = 0; i < num_data; ++i) {
			memcpy(data->input[i], &input[i*num_input], num_input*sizeof(fann_type));
			memcpy(data->output[i], &output[i*num_output], num_output*sizeof(fann_type));
		}
	}
	return data;
}

/*
 * Creates training data from a callback function.
 */
FANN_EXTERNAL FannTrainData * FANN_API fann_create_train_from_callback(uint num_data,
    uint num_input, uint num_output, void (FANN_API * user_function)(uint, uint, uint, fann_type *, fann_type *))
{
	FannTrainData * data = fann_create_train(num_data, num_input, num_output);
	if(data) {
		for(uint i = 0; i != num_data; i++) {
			(*user_function)(i, num_input, num_output, data->input[i], data->output[i]);
		}
	}
	return data;
}

FANN_EXTERNAL fann_type * FANN_API fann_get_train_input(FannTrainData * data, uint position)
{
	return (position >= data->num_data) ? NULL : data->input[position];
}

FANN_EXTERNAL fann_type * FANN_API fann_get_train_output(FannTrainData * data, uint position)
{
	return (position >= data->num_data) ? NULL : data->output[position];
}

/*
 * INTERNAL FUNCTION Reads training data from a file descriptor.
 */
FannTrainData * fann_read_train_from_fd(FILE * file, const char * filename)
{
	FannTrainData * data = 0;
	uint num_input, num_output, num_data, i, j;
	uint line = 1;
	if(fscanf(file, "%u %u %u\n", &num_data, &num_input, &num_output) != 3) {
		fann_error(NULL, FANN_E_CANT_READ_TD, filename, line);
	}
	else {
		line++;
		data = fann_create_train(num_data, num_input, num_output);
		if(data) {
			for(i = 0; i != num_data; i++) {
				for(j = 0; j != num_input; j++) {
					if(fscanf(file, FANNSCANF " ", &data->input[i][j]) != 1) {
						fann_error(NULL, FANN_E_CANT_READ_TD, filename, line);
						fann_destroy_train(data);
						return NULL;
					}
				}
				line++;
				for(j = 0; j != num_output; j++) {
					if(fscanf(file, FANNSCANF " ", &data->output[i][j]) != 1) {
						fann_error(NULL, FANN_E_CANT_READ_TD, filename, line);
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
/*
 * INTERNAL FUNCTION returns 0 if the desired error is reached and -1 if it is not reached
 */
int fann_desired_error_reached(Fann * ann, float desired_error)
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
/*
 * Scale data in input vector before feed it to ann based on previously calculated parameters.
 */
FANN_EXTERNAL void FANN_API fann_scale_input(Fann * ann, fann_type * input_vector)
{
	uint cur_neuron;
	if(ann->scale_mean_in == NULL) {
		fann_error((FannError*)ann, FANN_E_SCALE_NOT_PRESENT);
		return;
	}
	for(cur_neuron = 0; cur_neuron < ann->num_input; cur_neuron++)
		input_vector[ cur_neuron ] = ((input_vector[cur_neuron] - ann->scale_mean_in[cur_neuron]) /
			ann->scale_deviation_in[cur_neuron] - ((fann_type)-1.0) /* This is old_min */
		    )
		    * ann->scale_factor_in[cur_neuron]
		    + ann->scale_new_min_in[cur_neuron];
}

/*
 * Scale data in output vector before feed it to ann based on previously calculated parameters.
 */
FANN_EXTERNAL void FANN_API fann_scale_output(Fann * ann, fann_type * output_vector)
{
	if(ann->scale_mean_in == NULL) {
		fann_error((FannError*)ann, FANN_E_SCALE_NOT_PRESENT);
	}
	else {
		for(uint cur_neuron = 0; cur_neuron < ann->num_output; cur_neuron++) {
			output_vector[cur_neuron] = ((output_vector[cur_neuron] - ann->scale_mean_out[cur_neuron])
				/ ann->scale_deviation_out[cur_neuron] - ((fann_type)-1.0) /* This is old_min */
				) * ann->scale_factor_out[cur_neuron] + ann->scale_new_min_out[cur_neuron];
		}
	}
}
//
// Descale data in input vector after based on previously calculated parameters.
//
FANN_EXTERNAL void FANN_API fann_descale_input(Fann * ann, fann_type * input_vector)
{
	if(ann->scale_mean_in == NULL) {
		fann_error((FannError*)ann, FANN_E_SCALE_NOT_PRESENT);
	}
	else {
		for(uint cur_neuron = 0; cur_neuron < ann->num_input; cur_neuron++) {
			input_vector[cur_neuron] = ((input_vector[cur_neuron] - ann->scale_new_min_in[cur_neuron]) / ann->scale_factor_in[cur_neuron]
				+ ((fann_type)-1.0)             /* This is old_min */
				) * ann->scale_deviation_in[cur_neuron] + ann->scale_mean_in[cur_neuron];
		}
	}
}
/*
 * Descale data in output vector after get it from ann based on previously calculated parameters.
 */
FANN_EXTERNAL void FANN_API fann_descale_output(Fann * ann, fann_type * output_vector)
{
	if(ann->scale_mean_in == NULL) {
		fann_error((FannError*)ann, FANN_E_SCALE_NOT_PRESENT);
	}
	else {
		for(uint cur_neuron = 0; cur_neuron < ann->num_output; cur_neuron++) {
			output_vector[cur_neuron] = ((output_vector[cur_neuron] - ann->scale_new_min_out[cur_neuron])
				/ ann->scale_factor_out[cur_neuron]
				+ ((fann_type)-1.0)             /* This is old_min */
				) * ann->scale_deviation_out[cur_neuron] + ann->scale_mean_out[cur_neuron];
		}
	}
}
/*
 * Scale input and output data based on previously calculated parameters.
 */
FANN_EXTERNAL void FANN_API fann_scale_train(Fann * ann, FannTrainData * data)
{
	if(ann->scale_mean_in == NULL) {
		fann_error((FannError*)ann, FANN_E_SCALE_NOT_PRESENT);
	}
	else {
		// Check that we have good training data.
		if(fann_check_input_output_sizes(ann, data) != -1) {
			for(uint cur_sample = 0; cur_sample < data->num_data; cur_sample++) {
				fann_scale_input(ann, data->input[ cur_sample ]);
				fann_scale_output(ann, data->output[ cur_sample ]);
			}
		}
	}
}
//
// Scale input and output data based on previously calculated parameters.
//
FANN_EXTERNAL void FANN_API fann_descale_train(Fann * ann, FannTrainData * data)
{
	if(ann->scale_mean_in == NULL) {
		fann_error((FannError*)ann, FANN_E_SCALE_NOT_PRESENT);
	}
	else {
		// Check that we have good training data
		if(fann_check_input_output_sizes(ann, data) != -1) {
			for(uint cur_sample = 0; cur_sample < data->num_data; cur_sample++) {
				fann_descale_input(ann, data->input[ cur_sample ]);
				fann_descale_output(ann, data->output[ cur_sample ]);
			}
		}
	}
}

#define SCALE_RESET(what, where, default_value)							      \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++) \
		ann->what ## _ ## where[ cur_neuron ] = ( default_value );

#define SCALE_SET_PARAM(where)																		      \
        /* Calculate mean: sum(x)/length */																	\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_mean_ ## where[ cur_neuron ] = 0.0f;													  \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		for(cur_sample = 0; cur_sample < data->num_data; cur_sample++)								      \
			ann->scale_mean_ ## where[ cur_neuron ] += (float)data->where ## put[ cur_sample ][ cur_neuron ]; \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_mean_ ## where[ cur_neuron ] /= (float)data->num_data;								  \
        /* Calculate deviation: sqrt(sum((x-mean)^2)/length) */												\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_deviation_ ## where[ cur_neuron ] = 0.0f;												  \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		for(cur_sample = 0; cur_sample < data->num_data; cur_sample++)								      \
			ann->scale_deviation_ ## where[ cur_neuron ] +=												  \
			    /* Another local variable in macro? Oh no! */										    \
			    (																						    \
			    (float)data->where ## put[ cur_sample ][ cur_neuron ]							      \
			    - ann->scale_mean_ ## where[ cur_neuron ]											      \
			    )																						    \
			    *																						    \
			    (																						    \
			    (float)data->where ## put[ cur_sample ][ cur_neuron ]							      \
			    - ann->scale_mean_ ## where[ cur_neuron ]											      \
			    );																						    \
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_deviation_ ## where[ cur_neuron ] =													  \
		    sqrtf(ann->scale_deviation_ ## where[ cur_neuron ] / (float)data->num_data);		    \
        /* Calculate factor: (new_max-new_min)/(old_max(1)-old_min(-1)) */									\
        /* Looks like we dont need whole array of factors? */												\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_factor_ ## where[ cur_neuron ] =														  \
		    ( new_ ## where ## put_max - new_ ## where ## put_min )											    \
		    /																							    \
		    ( 1.0f - ( -1.0f ) );																	    \
        /* Copy new minimum. */																				\
        /* Looks like we dont need whole array of new minimums? */											\
	for(cur_neuron = 0; cur_neuron < ann->num_ ## where ## put; cur_neuron++)							  \
		ann->scale_new_min_ ## where[ cur_neuron ] = new_ ## where ## put_min;

FANN_EXTERNAL int FANN_API fann_set_input_scaling_params(Fann * ann,
    const FannTrainData * data, float new_input_min, float new_input_max)
{
	uint cur_neuron, cur_sample;
	// Check that we have good training data.
	// No need for if( !params || !ann )
	if(data->num_input != ann->num_input || data->num_output != ann->num_output) {
		fann_error((FannError*)ann, FANN_E_TRAIN_DATA_MISMATCH);
		return -1;
	}
	if(ann->scale_mean_in == NULL)
		fann_allocate_scale(ann);
	if(ann->scale_mean_in == NULL)
		return -1;
	if(!data->num_data) {
		SCALE_RESET(scale_mean,        in,  0.0)
		SCALE_RESET(scale_deviation,   in,  1.0)
		SCALE_RESET(scale_new_min,     in, -1.0)
		SCALE_RESET(scale_factor,      in,  1.0)
	}
	else {
		SCALE_SET_PARAM(in);
	}
	return 0;
}

FANN_EXTERNAL int FANN_API fann_set_output_scaling_params(Fann * ann,
    const FannTrainData * data, float new_output_min, float new_output_max)
{
	uint cur_neuron, cur_sample;
	// Check that we have good training data.
	// No need for if( !params || !ann )
	if(data->num_input != ann->num_input || data->num_output != ann->num_output) {
		fann_error((FannError*)ann, FANN_E_TRAIN_DATA_MISMATCH);
		return -1;
	}
	if(ann->scale_mean_out == NULL)
		fann_allocate_scale(ann);
	if(ann->scale_mean_out == NULL)
		return -1;
	if(!data->num_data) {
		SCALE_RESET(scale_mean,      out,  0.0)
		SCALE_RESET(scale_deviation, out,  1.0)
		SCALE_RESET(scale_new_min,   out, -1.0)
		SCALE_RESET(scale_factor,    out,  1.0)
	}
	else {
		SCALE_SET_PARAM(out);
	}
	return 0;
}
//
// Calculate scaling parameters for future use based on training data.
//
FANN_EXTERNAL int FANN_API fann_set_scaling_params(Fann * ann,
    const FannTrainData * data, float new_input_min, float new_input_max, float new_output_min, float new_output_max)
{
	if(fann_set_input_scaling_params(ann, data, new_input_min, new_input_max) == 0)
		return fann_set_output_scaling_params(ann, data, new_output_min, new_output_max);
	else
		return -1;
}
//
// Clears scaling parameters.
//
FANN_EXTERNAL int FANN_API fann_clear_scaling_params(Fann * ann)
{
	uint cur_neuron;
	if(ann->scale_mean_out == NULL)
		fann_allocate_scale(ann);
	if(ann->scale_mean_out == NULL)
		return -1;
	SCALE_RESET(scale_mean,      in,   0.0)
	SCALE_RESET(scale_deviation, in,   1.0)
	SCALE_RESET(scale_new_min,   in,  -1.0)
	SCALE_RESET(scale_factor,    in,   1.0)
	SCALE_RESET(scale_mean,      out,  0.0)
	SCALE_RESET(scale_deviation, out,  1.0)
	SCALE_RESET(scale_new_min,   out, -1.0)
	SCALE_RESET(scale_factor,    out,  1.0)
	return 0;
}

#endif

int fann_check_input_output_sizes(Fann * ann, FannTrainData * data)
{
	if(ann->num_input != data->num_input) {
		fann_error((FannError*)ann, FANN_E_INPUT_NO_MATCH, ann->num_input, data->num_input);
		return -1;
	}
	else if(ann->num_output != data->num_output) {
		fann_error((FannError*)ann, FANN_E_OUTPUT_NO_MATCH, ann->num_output, data->num_output);
		return -1;
	}
	else
		return 0;
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

FANN_EXTERNAL float FANN_API fann_train_epoch_batch_parallel(Fann * ann, FannTrainData * data, const uint threadnumb)
{
	/*vector<Fann *> ann_vect(threadnumb);*/
	Fann** ann_vect = (Fann**)malloc(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	fann_reset_MSE(ann);
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
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	//parallel update of the weights
	{
		const uint num_data = data->num_data;
		const uint first_weight = 0;
		const uint past_end = ann->total_connections;
		fann_type * weights = ann->weights;
		const fann_type epsilon = ann->learning_rate / num_data;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				fann_type temp_slopes = 0.0;
				uint k;
				fann_type * train_slopes;
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->train_slopes;
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
	free(ann_vect);
	return fann_get_MSE(ann);
}

FANN_EXTERNAL float FANN_API fann_train_epoch_irpropm_parallel(Fann * ann, FannTrainData * data, const uint threadnumb)
{
	Fann** ann_vect = (Fann**)malloc(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	//#define THREADNUM 1
	fann_reset_MSE(ann);
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
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	{
		fann_type * weights = ann->weights;
		fann_type * prev_steps = ann->prev_steps;
		fann_type * prev_train_slopes = ann->prev_train_slopes;
		fann_type next_step;
		const float increase_factor = ann->rprop_increase_factor; //1.2;
		const float decrease_factor = ann->rprop_decrease_factor; //0.5;
		const float delta_min = ann->rprop_delta_min; //0.0;
		const float delta_max = ann->rprop_delta_max; //50.0;
		const uint first_weight = 0;
		const uint past_end = ann->total_connections;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				fann_type prev_slope, same_sign;
				const fann_type prev_step = MAX(prev_steps[i], (fann_type)0.0001); // prev_step may not be zero because
					// then the training will stop
				fann_type temp_slopes = 0.0;
				uint k;
				fann_type * train_slopes;
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->train_slopes;
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
					if(weights[i] < -1500)
						weights[i] = -1500;
				}
				else {
					weights[i] += next_step;
					if(weights[i] > 1500)
						weights[i] = 1500;
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
	free(ann_vect);
	return fann_get_MSE(ann);
}

FANN_EXTERNAL float FANN_API fann_train_epoch_quickprop_parallel(Fann * ann, FannTrainData * data, const uint threadnumb)
{
	Fann** ann_vect = (Fann**)malloc(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	//#define THREADNUM 1
	fann_reset_MSE(ann);
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
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	{
		fann_type * weights = ann->weights;
		fann_type * prev_steps = ann->prev_steps;
		fann_type * prev_train_slopes = ann->prev_train_slopes;
		const uint first_weight = 0;
		const uint past_end = ann->total_connections;

		fann_type w = 0.0, next_step;

		const float epsilon = ann->learning_rate / data->num_data;
		const float decay = ann->quickprop_decay; /*-0.0001;*/
		const float mu = ann->quickprop_mu; /*1.75; */
		const float shrink_factor = (float)(mu / (1.0 + mu));

		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(w, next_step)
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				fann_type temp_slopes = 0.0;
				uint k;
				fann_type * train_slopes;
				fann_type prev_step, prev_slope;

				w = weights[i];
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->train_slopes;
					temp_slopes += train_slopes[i];
					train_slopes[i] = 0.0;
				}
				temp_slopes += decay * w;

				prev_step = prev_steps[i];
				prev_slope = prev_train_slopes[i];

				next_step = 0.0;

				/* The step must always be in direction opposite to the slope. */
				if(prev_step > 0.001) {
					/* If last step was positive...  */
					if(temp_slopes > 0.0)         /*  Add in linear term if current slope is still
						                        positive. */
						next_step += epsilon * temp_slopes;

					/*If current slope is close to or larger than prev slope...  */
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
	free(ann_vect);
	return fann_get_MSE(ann);
}

FANN_EXTERNAL float FANN_API fann_train_epoch_sarprop_parallel(Fann * ann, FannTrainData * data, const uint threadnumb)
{
	Fann** ann_vect = (Fann**)malloc(threadnumb * sizeof(Fann*));
	int i = 0, j = 0;
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	//#define THREADNUM 1
	fann_reset_MSE(ann);
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
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
	{
		fann_type * weights = ann->weights;
		fann_type * prev_steps = ann->prev_steps;
		fann_type * prev_train_slopes = ann->prev_train_slopes;
		const uint first_weight = 0;
		const uint past_end = ann->total_connections;
		const uint epoch = ann->sarprop_epoch;

		fann_type next_step;

		/* These should be set from variables */
		const float increase_factor = ann->rprop_increase_factor; /*1.2; */
		const float decrease_factor = ann->rprop_decrease_factor; /*0.5; */
		/* TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993) */
		const float delta_min = 0.000001f;
		const float delta_max = ann->rprop_delta_max; /*50.0; */
		const float weight_decay_shift = ann->sarprop_weight_decay_shift; /* ld 0.01 = -6.644 */
		const float step_error_threshold_factor = ann->sarprop_step_error_threshold_factor; /* 0.1 */
		const float step_error_shift = ann->sarprop_step_error_shift; /* ld 3 = 1.585 */
		const float T = ann->sarprop_temperature;
		float MSE, RMSE;
		//merge of MSEs
		for(i = 0; i<(int)threadnumb; ++i) {
			ann->MSE_value += ann_vect[i]->MSE_value;
			ann->num_MSE += ann_vect[i]->num_MSE;
		}
		MSE = fann_get_MSE(ann);
		RMSE = sqrtf(MSE);
		// for all weights; TODO: are biases included?
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
			for(i = first_weight; i < (int)past_end; i++) {
				/* TODO: confirm whether 1x10^-6 == delta_min is really better */
				const fann_type prev_step  = MAX(prev_steps[i], (fann_type)0.000001); // prev_step may not be zero because then the training will stop
				// calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)
				fann_type prev_slope, same_sign;
				fann_type temp_slopes = 0.0;
				uint k;
				fann_type * train_slopes;
				for(k = 0; k<threadnumb; ++k) {
					train_slopes = ann_vect[k]->train_slopes;
					temp_slopes += train_slopes[i];
					train_slopes[i] = 0.0;
				}
				temp_slopes = -temp_slopes - weights[i] * (fann_type)fann_exp2(-T * epoch + weight_decay_shift);
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
						next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (fann_type)fann_exp2(-T * epoch + step_error_shift);
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
	++(ann->sarprop_epoch);
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
	free(ann_vect);
	return fann_get_MSE(ann);
}

FANN_EXTERNAL float FANN_API fann_train_epoch_incremental_mod(Fann * ann, FannTrainData * data)
{
	fann_reset_MSE(ann);
	for(uint i = 0; i != data->num_data; i++) {
		fann_train(ann, data->input[i], data->output[i]);
	}
	return fann_get_MSE(ann);
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

float train_epoch_batch_parallel(Fann *ann, FannTrainData *data, const uint threadnumb)
{
	fann_reset_MSE(ann);
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
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    //parallel update of the weights
	{
		const uint num_data=data->num_data;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;
		fann_type *weights = ann->weights;
		const fann_type epsilon = ann->learning_rate / num_data;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0; k < threadnumb; ++k) {
						train_slopes=ann_vect[k]->train_slopes;
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
	return fann_get_MSE(ann);
}


float train_epoch_irpropm_parallel(Fann *ann, FannTrainData *data, const uint threadnumb)
{
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
	//#define THREADNUM 1
	fann_reset_MSE(ann);
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;
	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{

		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++)
		{
			ann_vect[i]=fann_copy(ann);
		}

    //parallel computing of the updates


        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++)
		{
			j=omp_get_thread_num();
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}

	{
    	fann_type *weights = ann->weights;
    	fann_type *prev_steps = ann->prev_steps;
    	fann_type *prev_train_slopes = ann->prev_train_slopes;

    	fann_type next_step;

    	const float increase_factor = ann->rprop_increase_factor;	//1.2;
    	const float decrease_factor = ann->rprop_decrease_factor;	//0.5;
    	const float delta_min = ann->rprop_delta_min;	//0.0;
    	const float delta_max = ann->rprop_delta_max;	//50.0;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;

		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++)
				{

		    		const fann_type prev_step = MAX(prev_steps[i], (fann_type) 0.0001);	// prev_step may not be zero because then the training will stop

		    		fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0;k<threadnumb;++k)
					{
						train_slopes=ann_vect[k]->train_slopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}

		    		const fann_type prev_slope = prev_train_slopes[i];

		    		const fann_type same_sign = prev_slope * temp_slopes;

		    		if(same_sign >= 0.0)
		    			next_step = MIN(prev_step * increase_factor, delta_max);
		    		else
		    		{
		    			next_step = MAX(prev_step * decrease_factor, delta_min);
		    			temp_slopes = 0;
		    		}

		    		if(temp_slopes < 0)
		    		{
		    			weights[i] -= next_step;
		    			if(weights[i] < -1500)
		    				weights[i] = -1500;
		    		}
		    		else
		    		{
		    			weights[i] += next_step;
		    			if(weights[i] > 1500)
		    				weights[i] = 1500;
		    		}

		    		// update global data arrays
		    		prev_steps[i] = next_step;
		    		prev_train_slopes[i] = temp_slopes;

				}
			}
	}

	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i)
	{
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return fann_get_MSE(ann);
}


float train_epoch_quickprop_parallel(Fann *ann, FannTrainData *data, const uint threadnumb)
{

	if(ann->prev_train_slopes == NULL)
	{
		fann_clear_train_arrays(ann);
	}

	//#define THREADNUM 1
	fann_reset_MSE(ann);

	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;

	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{

		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++)
		{
			ann_vect[i]=fann_copy(ann);
		}

    //parallel computing of the updates

        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++)
		{
			j=omp_get_thread_num();
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}

    {
    	fann_type *weights = ann->weights;
    	fann_type *prev_steps = ann->prev_steps;
    	fann_type *prev_train_slopes = ann->prev_train_slopes;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;

    	fann_type w=0.0, next_step;

    	const float epsilon = ann->learning_rate / data->num_data;
    	const float decay = ann->quickprop_decay;	/*-0.0001;*/
    	const float mu = ann->quickprop_mu;	/*1.75; */
    	const float shrink_factor = (float) (mu / (1.0 + mu));

		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(w, next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++)
				{

					w = weights[i];

					fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0;k<threadnumb;++k)
					{
						train_slopes=ann_vect[k]->train_slopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes+= decay * w;

					const fann_type prev_step = prev_steps[i];
					const fann_type prev_slope = prev_train_slopes[i];

					next_step = 0.0;


					/* The step must always be in direction opposite to the slope. */
					if(prev_step > 0.001)
					{
						/* If last step was positive...  */
						if(temp_slopes > 0.0) /*  Add in linear term if current slope is still positive. */
							next_step += epsilon * temp_slopes;

						/*If current slope is close to or larger than prev slope...  */
						if(temp_slopes > (shrink_factor * prev_slope))
							next_step += mu * prev_step;	/* Take maximum size negative step. */
						else
							next_step += prev_step * temp_slopes / (prev_slope - temp_slopes);	/* Else, use quadratic estimate. */
					}
					else if(prev_step < -0.001)
					{
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
	for(i=0;i<(int)threadnumb;++i)
	{
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return fann_get_MSE(ann);
}


float train_epoch_sarprop_parallel(Fann *ann, FannTrainData *data, const uint threadnumb)
{
	if(ann->prev_train_slopes == NULL)
	{
		fann_clear_train_arrays(ann);
	}

	//#define THREADNUM 1
	fann_reset_MSE(ann);

	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;

	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{

		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++)
		{
			ann_vect[i]=fann_copy(ann);
		}

    //parallel computing of the updates

        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++)
		{
			j=omp_get_thread_num();
			fann_run(ann_vect[j], data->input[i]);
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}

    {
    	fann_type *weights = ann->weights;
    	fann_type *prev_steps = ann->prev_steps;
    	fann_type *prev_train_slopes = ann->prev_train_slopes;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;
		const uint epoch=ann->sarprop_epoch;

    	fann_type next_step;

    	/* These should be set from variables */
    	const float increase_factor = ann->rprop_increase_factor;	/*1.2; */
    	const float decrease_factor = ann->rprop_decrease_factor;	/*0.5; */
    	/* TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993) */
    	const float delta_min = 0.000001f;
    	const float delta_max = ann->rprop_delta_max;	/*50.0; */
    	const float weight_decay_shift = ann->sarprop_weight_decay_shift; /* ld 0.01 = -6.644 */
    	const float step_error_threshold_factor = ann->sarprop_step_error_threshold_factor; /* 0.1 */
    	const float step_error_shift = ann->sarprop_step_error_shift; /* ld 3 = 1.585 */
    	const float T = ann->sarprop_temperature;


    	//merge of MSEs
    	for(i=0;i<(int)threadnumb;++i)
    	{
    		ann->MSE_value+= ann_vect[i]->MSE_value;
    		ann->num_MSE+=ann_vect[i]->num_MSE;
    	}

    	const float MSE = fann_get_MSE(ann);
    	const float RMSE = sqrtf(MSE);

    	/* for all weights; TODO: are biases included? */
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++)
				{
					/* TODO: confirm whether 1x10^-6 == delta_min is really better */
					const fann_type prev_step  = MAX(prev_steps[i], (fann_type) 0.000001);	/* prev_step may not be zero because then the training will stop */

					/* calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)*/

					fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0;k<threadnumb;++k)
					{
						train_slopes=ann_vect[k]->train_slopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes= -temp_slopes - weights[i] * (fann_type)fann_exp2(-T * epoch + weight_decay_shift);

					next_step=0.0;

					/* TODO: is prev_train_slopes[i] 0.0 in the beginning? */
					const fann_type prev_slope = prev_train_slopes[i];

					const fann_type same_sign = prev_slope * temp_slopes;

					if(same_sign > 0.0)
					{
						next_step = MIN(prev_step * increase_factor, delta_max);
						/* TODO: are the signs inverted? see differences between SARPROP paper and iRprop */
						if (temp_slopes < 0.0)
							weights[i] += next_step;
						else
							weights[i] -= next_step;
					}
					else if(same_sign < 0.0)
					{
						#ifndef RAND_MAX
						#define	RAND_MAX	0x7fffffff
						#endif
						if(prev_step < step_error_threshold_factor * MSE)
							next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (fann_type)fann_exp2(-T * epoch + step_error_shift);
						else
							next_step = MAX(prev_step * decrease_factor, delta_min);

						temp_slopes = 0.0;
					}
					else
					{
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

	++(ann->sarprop_epoch);

	//already computed before
	/*//merge of MSEs
	for(i=0;i<threadnumb;++i)
	{
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
	}*/
	//destroy the copies of the ann
	for(i=0; i<(int)threadnumb; i++)
	{
		fann_destroy(ann_vect[i]);
	}
	return fann_get_MSE(ann);
}

float train_epoch_incremental_mod(Fann *ann, FannTrainData *data)
{
	uint i;

	fann_reset_MSE(ann);

	for(i = 0; i != data->num_data; i++)
	{
		fann_train(ann, data->input[i], data->output[i]);
	}

	return fann_get_MSE(ann);
}

//the following versions returns also the outputs via the predicted_outputs parameter

float train_epoch_batch_parallel(Fann *ann, FannTrainData *data, const uint threadnumb,vector< vector<fann_type> >& predicted_outputs)
{
	fann_reset_MSE(ann);
	predicted_outputs.resize(data->num_data,vector<fann_type> (data->num_output));
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;

	//generate copies of the ann
	omp_set_dynamic(0);
	omp_set_num_threads(threadnumb);
	#pragma omp parallel private(j)
	{

		#pragma omp for schedule(static)
		for(i=0; i<(int)threadnumb; i++)
		{
			ann_vect[i]=fann_copy(ann);
		}

    //parallel computing of the updates
        #pragma omp for schedule(static)
		for(i = 0; i < (int)data->num_data; i++) {
			j=omp_get_thread_num();
			fann_type* temp_predicted_output=fann_run(ann_vect[j], data->input[i]);
			for(uint k=0;k<data->num_output;++k) {
				predicted_outputs[i][k]=temp_predicted_output[k];
			}
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    //parallel update of the weights
	{
		const uint num_data=data->num_data;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;
		fann_type *weights = ann->weights;
		const fann_type epsilon = ann->learning_rate / num_data;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->train_slopes;
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
	return fann_get_MSE(ann);
}


float train_epoch_irpropm_parallel(Fann *ann, FannTrainData *data, const uint threadnumb, vector< vector<fann_type> >& predicted_outputs)
{
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
		fann_reset_MSE(ann);
		predicted_outputs.resize(data->num_data,vector<fann_type> (data->num_output));
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
				fann_type* temp_predicted_output=fann_run(ann_vect[j], data->input[i]);
				for(uint k=0;k<data->num_output;++k) {
					predicted_outputs[i][k]=temp_predicted_output[k];
				}
				fann_compute_MSE(ann_vect[j], data->output[i]);
				fann_backpropagate_MSE(ann_vect[j]);
				fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
			}
	}
	{
    	fann_type *weights = ann->weights;
    	fann_type *prev_steps = ann->prev_steps;
    	fann_type *prev_train_slopes = ann->prev_train_slopes;
    	fann_type next_step;
    	const float increase_factor = ann->rprop_increase_factor;	//1.2;
    	const float decrease_factor = ann->rprop_decrease_factor;	//0.5;
    	const float delta_min = ann->rprop_delta_min;	//0.0;
    	const float delta_max = ann->rprop_delta_max;	//50.0;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
		    		const fann_type prev_step = MAX(prev_steps[i], (fann_type) 0.0001);	// prev_step may not be zero because then the training will stop
		    		fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->train_slopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
		    		const fann_type prev_slope = prev_train_slopes[i];
		    		const fann_type same_sign = prev_slope * temp_slopes;
		    		if(same_sign >= 0.0)
		    			next_step = MIN(prev_step * increase_factor, delta_max);
		    		else {
		    			next_step = MAX(prev_step * decrease_factor, delta_min);
		    			temp_slopes = 0;
		    		}
		    		if(temp_slopes < 0) {
		    			weights[i] -= next_step;
		    			if(weights[i] < -1500)
		    				weights[i] = -1500;
		    		}
		    		else {
		    			weights[i] += next_step;
		    			if(weights[i] > 1500)
		    				weights[i] = 1500;
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
	return fann_get_MSE(ann);
}

float train_epoch_quickprop_parallel(Fann *ann, FannTrainData *data, const uint threadnumb, vector< vector<fann_type> >& predicted_outputs)
{
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
		fann_reset_MSE(ann);
		predicted_outputs.resize(data->num_data,vector<fann_type> (data->num_output));
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
				fann_type* temp_predicted_output=fann_run(ann_vect[j], data->input[i]);
				for(uint k=0;k<data->num_output;++k) {
					predicted_outputs[i][k]=temp_predicted_output[k];
				}
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    {
    	fann_type *weights = ann->weights;
    	fann_type *prev_steps = ann->prev_steps;
    	fann_type *prev_train_slopes = ann->prev_train_slopes;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;
    	fann_type w=0.0, next_step;
    	const float epsilon = ann->learning_rate / data->num_data;
    	const float decay = ann->quickprop_decay;	/*-0.0001;*/
    	const float mu = ann->quickprop_mu;	/*1.75; */
    	const float shrink_factor = (float) (mu / (1.0 + mu));
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(w, next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					w = weights[i];
					fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->train_slopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes+= decay * w;

					const fann_type prev_step = prev_steps[i];
					const fann_type prev_slope = prev_train_slopes[i];
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
	return fann_get_MSE(ann);
}

float train_epoch_sarprop_parallel(Fann *ann, FannTrainData *data, const uint threadnumb, vector< vector<fann_type> >& predicted_outputs)
{
	if(ann->prev_train_slopes == NULL) {
		fann_clear_train_arrays(ann);
	}
		fann_reset_MSE(ann);
		predicted_outputs.resize(data->num_data,vector<fann_type> (data->num_output));
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
				fann_type* temp_predicted_output=fann_run(ann_vect[j], data->input[i]);
				for(uint k=0;k<data->num_output;++k) {
					predicted_outputs[i][k]=temp_predicted_output[k];
				}
			fann_compute_MSE(ann_vect[j], data->output[i]);
			fann_backpropagate_MSE(ann_vect[j]);
			fann_update_slopes_batch(ann_vect[j], ann_vect[j]->P_FirstLayer + 1, ann_vect[j]->P_LastLayer - 1);
		}
	}
    {
    	fann_type *weights = ann->weights;
    	fann_type *prev_steps = ann->prev_steps;
    	fann_type *prev_train_slopes = ann->prev_train_slopes;
		const uint first_weight=0;
		const uint past_end=ann->total_connections;
		const uint epoch=ann->sarprop_epoch;
    	fann_type next_step;
    	/* These should be set from variables */
    	const float increase_factor = ann->rprop_increase_factor;	/*1.2; */
    	const float decrease_factor = ann->rprop_decrease_factor;	/*0.5; */
    	/* TODO: why is delta_min 0.0 in iRprop? SARPROP uses 1x10^-6 (Braun and Riedmiller, 1993) */
    	const float delta_min = 0.000001f;
    	const float delta_max = ann->rprop_delta_max;	/*50.0; */
    	const float weight_decay_shift = ann->sarprop_weight_decay_shift; /* ld 0.01 = -6.644 */
    	const float step_error_threshold_factor = ann->sarprop_step_error_threshold_factor; /* 0.1 */
    	const float step_error_shift = ann->sarprop_step_error_shift; /* ld 3 = 1.585 */
    	const float T = ann->sarprop_temperature;
    	//merge of MSEs
    	for(i=0;i<(int)threadnumb;++i) {
    		ann->MSE_value+= ann_vect[i]->MSE_value;
    		ann->num_MSE+=ann_vect[i]->num_MSE;
    	}
    	const float MSE = fann_get_MSE(ann);
    	const float RMSE = (float)sqrt(MSE);
    	/* for all weights; TODO: are biases included? */
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(next_step)
		{
			#pragma omp for schedule(static)
				for(i=first_weight; i < (int)past_end; i++) {
					/* TODO: confirm whether 1x10^-6 == delta_min is really better */
					const fann_type prev_step  = MAX(prev_steps[i], (fann_type) 0.000001);	/* prev_step may not be zero because then the training will stop */
					/* calculate SARPROP slope; TODO: better as new error function? (see SARPROP paper)*/
					fann_type temp_slopes=0.0;
					uint k;
					fann_type *train_slopes;
					for(k=0;k<threadnumb;++k) {
						train_slopes=ann_vect[k]->train_slopes;
						temp_slopes+= train_slopes[i];
						train_slopes[i]=0.0;
					}
					temp_slopes= -temp_slopes - weights[i] * (fann_type)fann_exp2(-T * epoch + weight_decay_shift);
					next_step=0.0;
					/* TODO: is prev_train_slopes[i] 0.0 in the beginning? */
					const fann_type prev_slope = prev_train_slopes[i];
					const fann_type same_sign = prev_slope * temp_slopes;
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
							next_step = prev_step * decrease_factor + (float)rand() / RAND_MAX * RMSE * (fann_type)fann_exp2(-T * epoch + step_error_shift);
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
	++(ann->sarprop_epoch);
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
	return fann_get_MSE(ann);
}


float train_epoch_incremental_mod(Fann *ann, FannTrainData *data, vector< vector<fann_type> >& predicted_outputs)
{
	predicted_outputs.resize(data->num_data,vector<fann_type> (data->num_output));
	fann_reset_MSE(ann);
	for(uint i = 0; i < data->num_data; ++i) {
		fann_type* temp_predicted_output=fann_run(ann, data->input[i]);
		for(uint k=0;k<data->num_output;++k) {
			predicted_outputs[i][k]=temp_predicted_output[k];
		}
		fann_compute_MSE(ann, data->output[i]);
		fann_backpropagate_MSE(ann);
		fann_update_weights(ann);
	}
	return fann_get_MSE(ann);
}

float test_data_parallel(Fann *ann, FannTrainData *data, const uint threadnumb)
{
	if(fann_check_input_output_sizes(ann, data) == -1)
		return 0;
	fann_reset_MSE(ann);
	vector<Fann *> ann_vect(threadnumb);
	int i=0,j=0;

		//generate copies of the ann
		omp_set_dynamic(0);
		omp_set_num_threads(threadnumb);
		#pragma omp parallel private(j)
		{

			#pragma omp for schedule(static)
			for(i=0; i<(int)threadnumb; i++)
			{
				ann_vect[i]=fann_copy(ann);
			}

			//parallel computing of the updates

	        #pragma omp for schedule(static)
			for(i = 0; i < (int)data->num_data; ++i) {
				j=omp_get_thread_num();
				fann_test(ann_vect[j], data->input[i],data->output[i]);
			}
		}
	//merge of MSEs
	for(i=0;i<(int)threadnumb;++i) {
		ann->MSE_value+= ann_vect[i]->MSE_value;
		ann->num_MSE+=ann_vect[i]->num_MSE;
		fann_destroy(ann_vect[i]);
	}
	return fann_get_MSE(ann);
}

float test_data_parallel(Fann *ann, FannTrainData *data, const uint threadnumb, vector< vector<fann_type> >& predicted_outputs)
{
	if(fann_check_input_output_sizes(ann, data) == -1)
		return 0;
	predicted_outputs.resize(data->num_data,vector<fann_type> (data->num_output));
	fann_reset_MSE(ann);
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
				fann_type* temp_predicted_output=fann_test(ann_vect[j], data->input[i],data->output[i]);
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
	return fann_get_MSE(ann);
}
}
#endif // DISABLE_PARALLEL_FANN
//
// TESTINT
//
// FANN_TEST.CPP
//
#include <vector>
//#include "fann_test.h"
#include "gtest/gtest.h"
#include "doublefann.h"
#include "fann_cpp.h"

using namespace FANN;

class FannTest : public testing::Test {
protected:
    neural_net net;
    training_data data;
    void AssertCreateAndCopy(neural_net &net, unsigned int numLayers, const unsigned int *layers, unsigned int neurons,
		unsigned int connections);
    void AssertCreate(neural_net &net, unsigned int numLayers, const unsigned int *layers,
		unsigned int neurons, unsigned int connections);
    void AssertWeights(neural_net &net, fann_type min, fann_type max, fann_type avg);
    virtual void SetUp();
    virtual void TearDown();
};

//#include "fann_test_data.h"

#include "gtest/gtest.h"

#include "doublefann.h"
#include "fann_cpp.h"
#include "fann_test.h"

class FannTestData : public FannTest {
protected:
    unsigned int numData;
    unsigned int numInput;
    unsigned int numOutput;
    fann_type inputValue;
    fann_type outputValue;
    fann_type **inputData;
    fann_type **outputData;
    virtual void SetUp();
    virtual void TearDown();
    void AssertTrainData(FANN::training_data &trainingData, unsigned int numData, unsigned int numInput,
		unsigned int numOutput, fann_type inputValue, fann_type outputValue);
    void InitializeTrainDataStructure(unsigned int numData, unsigned int numInput, unsigned int numOutput,
		fann_type inputValue, fann_type outputValue, fann_type **inputData, fann_type **outputData);
};

//#include "fann_test_train.h"
//#include "fann_test.h"

class FannTestTrain : public FannTest {
protected:
    fann_type xorInput[8] = {
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0};
    fann_type xorOutput[4] = {
		0.0,
		1.0,
		1.0,
		0.0};
    virtual void SetUp();
    virtual void TearDown();
};

#include "gtest/gtest.h"

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

void FannTest::AssertCreate(neural_net &net, unsigned int numLayers, const unsigned int *layers,
	unsigned int neurons, unsigned int connections)
{
    EXPECT_EQ(numLayers, net.get_num_layers());
    EXPECT_EQ(layers[0], net.get_num_input());
    EXPECT_EQ(layers[numLayers - 1], net.get_num_output());
    unsigned int *layers_res = new unsigned int[numLayers];
    net.get_layer_array(layers_res);
    for(unsigned int i = 0; i < numLayers; i++) {
        EXPECT_EQ(layers[i], layers_res[i]);
    }
    delete layers_res;
    EXPECT_EQ(neurons, net.get_total_neurons());
    EXPECT_EQ(connections, net.get_total_connections());
    AssertWeights(net, -0.09, 0.09, 0.0);
}

void FannTest::AssertCreateAndCopy(neural_net &net, unsigned int numLayers, const unsigned int *layers, unsigned int neurons,
	unsigned int connections)
{
    AssertCreate(net, numLayers, layers, neurons, connections);
    neural_net net_copy(net);
    AssertCreate(net_copy, numLayers, layers, neurons, connections);
}

void FannTest::AssertWeights(neural_net &net, fann_type min, fann_type max, fann_type avg)
{
    connection *connections = new connection[net.get_total_connections()];
    net.get_connection_array(connections);

    fann_type minWeight = connections[0].weight;
    fann_type maxWeight = connections[0].weight;
    fann_type totalWeight = 0.0;
    for(int i = 1; i < net.get_total_connections(); ++i) {
        if(connections[i].weight < minWeight)
            minWeight = connections[i].weight;
        if(connections[i].weight > maxWeight)
            maxWeight = connections[i].weight;
        totalWeight += connections[i].weight;
    }
    EXPECT_NEAR(min, minWeight, 0.05);
    EXPECT_NEAR(max, maxWeight, 0.05);
    EXPECT_NEAR(avg, totalWeight / (fann_type) net.get_total_connections(), 0.5);
}

TEST_F(FannTest, CreateStandardThreeLayers)
{
    neural_net net(LAYER, 3, 2, 3, 4);
    AssertCreateAndCopy(net, 3, (const unsigned int[]) {2, 3, 4}, 11, 25);
}

TEST_F(FannTest, CreateStandardThreeLayersUsingCreateMethod)
{
    ASSERT_TRUE(net.create_standard(3, 2, 3, 4));
    unsigned int layers[] = {2, 3, 4};
    AssertCreateAndCopy(net, 3, layers, 11, 25);
}

TEST_F(FannTest, CreateStandardFourLayersArray)
{
    unsigned int layers[] = {2, 3, 4, 5};
    neural_net net(LAYER, 4, layers);
    AssertCreateAndCopy(net, 4, layers, 17, 50);
}

TEST_F(FannTest, CreateStandardFourLayersArrayUsingCreateMethod)
{
    unsigned int layers[] = {2, 3, 4, 5};
    ASSERT_TRUE(net.create_standard_array(4, layers));
    AssertCreateAndCopy(net, 4, layers, 17, 50);
}

TEST_F(FannTest, CreateStandardFourLayersVector)
{
    vector<unsigned int> layers{2, 3, 4, 5};
    neural_net net(LAYER, layers.begin(), layers.end());
    AssertCreateAndCopy(net, 4, layers.data(), 17, 50);
}

TEST_F(FannTest, CreateSparseFourLayers)
{
    neural_net net(0.5, 4, 2, 3, 4, 5);
    AssertCreateAndCopy(net, 4, (const unsigned int[]){2, 3, 4, 5}, 17, 31);
}

TEST_F(FannTest, CreateSparseFourLayersUsingCreateMethod)
{
    ASSERT_TRUE(net.create_sparse(0.5f, 4, 2, 3, 4, 5));
    AssertCreateAndCopy(net, 4, (const unsigned int[]){2, 3, 4, 5}, 17, 31);
}

TEST_F(FannTest, CreateSparseArrayFourLayers)
{
    unsigned int layers[] = {2, 3, 4, 5};
    neural_net net(0.5f, 4, layers);
    AssertCreateAndCopy(net, 4, layers, 17, 31);
}

TEST_F(FannTest, CreateSparseArrayFourLayersUsingCreateMethod)
{
    unsigned int layers[] = {2, 3, 4, 5};
    ASSERT_TRUE(net.create_sparse_array(0.5f, 4, layers));
    AssertCreateAndCopy(net, 4, layers, 17, 31);
}

TEST_F(FannTest, CreateSparseArrayWithMinimalConnectivity)
{
    unsigned int layers[] = {2, 2, 2};
    neural_net net(0.01f, 3, layers);
    AssertCreateAndCopy(net, 3, layers, 8, 8);
}

TEST_F(FannTest, CreateShortcutFourLayers)
{
    neural_net net(SHORTCUT, 4, 2, 3, 4, 5);
    AssertCreateAndCopy(net, 4, (const unsigned int[]){2, 3, 4, 5}, 15, 83);
    EXPECT_EQ(SHORTCUT, net.get_network_type());
}

TEST_F(FannTest, CreateShortcutFourLayersUsingCreateMethod)
{
    ASSERT_TRUE(net.create_shortcut(4, 2, 3, 4, 5));
    AssertCreateAndCopy(net, 4, (const unsigned int[]){2, 3, 4, 5}, 15, 83);
    EXPECT_EQ(SHORTCUT, net.get_network_type());
}

TEST_F(FannTest, CreateShortcutArrayFourLayers)
{
    unsigned int layers[] = {2, 3, 4, 5};
    neural_net net(SHORTCUT, 4, layers);
    AssertCreateAndCopy(net, 4, layers, 15, 83);
    EXPECT_EQ(SHORTCUT, net.get_network_type());
}

TEST_F(FannTest, CreateShortcutArrayFourLayersUsingCreateMethod)
{
    unsigned int layers[] = {2, 3, 4, 5};
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
    AssertCreateAndCopy(netToBeLoaded, 3, (const unsigned int[]){2, 3, 4}, 11, 25);
}

TEST_F(FannTest, CreateFromFileUsingCreateMethod)
{
    ASSERT_TRUE(net.create_standard(3, 2, 3, 4));
    neural_net inputNet(LAYER, 3, 2, 3, 4);
    ASSERT_TRUE(inputNet.save("tmpfile"));
    ASSERT_TRUE(net.create_from_file("tmpfile"));
    AssertCreateAndCopy(net, 3, (const unsigned int[]){2, 3, 4}, 11, 25);
}

TEST_F(FannTest, RandomizeWeights) {
    neural_net net(LAYER, 2, 20, 10);
    net.randomize_weights(-1.0, 1.0);
    AssertWeights(net, -1.0, 1.0, 0);
}
//
// FANN_TEST_DATA.CPP
//
void FannTestData::SetUp() {
    FannTest::SetUp();
    numData = 2;
    numInput = 3;
    numOutput = 1;
    inputValue = 1.1;
    outputValue = 2.2;
    inputData = new fann_type *[numData];
    outputData = new fann_type *[numData];
    InitializeTrainDataStructure(numData, numInput, numOutput, inputValue, outputValue, inputData, outputData);
}

void FannTestData::TearDown()
{
    FannTest::TearDown();
    delete(inputData);
    delete(outputData);
}

void FannTestData::InitializeTrainDataStructure(unsigned int numData,
	unsigned int numInput, unsigned int numOutput, fann_type inputValue, fann_type outputValue,
	fann_type **inputData, fann_type **outputData)
{
    for (unsigned int i = 0; i < numData; i++) {
        inputData[i] = new fann_type[numInput];
        outputData[i] = new fann_type[numOutput];
        for (unsigned int j = 0; j < numInput; j++)
            inputData[i][j] = inputValue;
        for (unsigned int j = 0; j < numOutput; j++)
            outputData[i][j] = outputValue;
    }
}

void FannTestData::AssertTrainData(training_data &trainingData, unsigned int numData, unsigned int numInput,
	unsigned int numOutput, fann_type inputValue, fann_type outputValue)
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
    fann_type input[] = {inputValue, inputValue, inputValue, inputValue, inputValue, inputValue};
    fann_type output[] = {outputValue, outputValue};
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

void callBack(unsigned int pos, unsigned int numInput, unsigned int numOutput, fann_type *input, fann_type *output)
{
    for(unsigned int i = 0; i < numInput; i++)
        input[i] = (fann_type) 1.2;
    for(unsigned int i = 0; i < numOutput; i++)
        output[i] = (fann_type) 2.3;
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
    fann_type input[] = {0.0, 1.0, 0.5, 0.0, 1.0, 0.5};
    fann_type output[] = {0.0, 1.0};
    data.set_train_data(2, 3, input, 1, output);
    data.scale_output_train_data(-1.0, 2.0);
    EXPECT_DOUBLE_EQ(0.0, data.get_min_input());
    EXPECT_DOUBLE_EQ(1.0, data.get_max_input());
    EXPECT_DOUBLE_EQ(-1.0, data.get_min_output());
    EXPECT_DOUBLE_EQ(2.0, data.get_max_output());
}

TEST_F(FannTestData, ScaleInputData)
{
    fann_type input[] = {0.0, 1.0, 0.5, 0.0, 1.0, 0.5};
    fann_type output[] = {0.0, 1.0};
    data.set_train_data(2, 3, input, 1, output);
    data.scale_input_train_data(-1.0, 2.0);
    EXPECT_DOUBLE_EQ(-1.0, data.get_min_input());
    EXPECT_DOUBLE_EQ(2.0, data.get_max_input());
    EXPECT_DOUBLE_EQ(0.0, data.get_min_output());
    EXPECT_DOUBLE_EQ(1.0, data.get_max_output());
}

TEST_F(FannTestData, ScaleData)
{
    fann_type input[] = {0.0, 1.0, 0.5, 0.0, 1.0, 0.5};
    fann_type output[] = {0.0, 1.0};
    data.set_train_data(2, 3, input, 1, output);
    data.scale_train_data(-1.0, 2.0);
    for(unsigned int i = 0; i < 2; i++) {
        fann_type *train_input = data.get_train_input(i);
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
        net.train((fann_type*) (const fann_type[]) {0.0, 0.0}, (fann_type*) (const fann_type[]) {0.0});
        net.train((fann_type*) (const fann_type[]) {1.0, 0.0}, (fann_type*) (const fann_type[]) {1.0});
        net.train((fann_type*) (const fann_type[]) {0.0, 1.0}, (fann_type*) (const fann_type[]) {1.0});
        net.train((fann_type*) (const fann_type[]) {1.0, 1.0}, (fann_type*) (const fann_type[]) {0.0});
    }
    EXPECT_LT(net.get_MSE(), 0.01);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
