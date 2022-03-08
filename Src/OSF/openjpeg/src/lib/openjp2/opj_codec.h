/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef OPJ_CODEC_H
#define OPJ_CODEC_H
/**
   @file opj_codec.h
 */

/**
 * Main codec handler used for compression or decompression.
 */
typedef struct opj_codec_private {
	/** FIXME DOC */
	union {
		/**
		 * Decompression handler.
		 */
		struct opj_decompression {
			/** Main header reading function handler */
			boolint (* opj_read_header)(struct opj_stream_private * cio,
			    void * p_codec,
			    opj_image_t ** p_image,
			    struct opj_event_mgr * p_manager);

			/** Decoding function */
			boolint (* opj_decode)(void * p_codec,
			    struct opj_stream_private * p_cio,
			    opj_image_t * p_image,
			    struct opj_event_mgr * p_manager);

			/** FIXME DOC */
			boolint (* opj_read_tile_header)(void * p_codec,
			    OPJ_UINT32 * p_tile_index,
			    OPJ_UINT32 * p_data_size,
			    OPJ_INT32 * p_tile_x0,
			    OPJ_INT32 * p_tile_y0,
			    OPJ_INT32 * p_tile_x1,
			    OPJ_INT32 * p_tile_y1,
			    OPJ_UINT32 * p_nb_comps,
			    boolint * p_should_go_on,
			    struct opj_stream_private * p_cio,
			    struct opj_event_mgr * p_manager);

			/** FIXME DOC */
			boolint (* opj_decode_tile_data)(void * p_codec,
			    OPJ_UINT32 p_tile_index,
			    uint8 * p_data,
			    OPJ_UINT32 p_data_size,
			    struct opj_stream_private * p_cio,
			    struct opj_event_mgr * p_manager);

			/** Reading function used after codestream if necessary */
			boolint (* opj_end_decompress)(void * p_codec,
			    struct opj_stream_private * cio,
			    struct opj_event_mgr * p_manager);

			/** Codec destroy function handler */
			void (* opj_destroy)(void * p_codec);

			/** Setup decoder function handler */
			void (* opj_setup_decoder)(void * p_codec, opj_dparameters_t * p_param);

			/** Set decode area function handler */
			boolint (* opj_set_decode_area)(void * p_codec,
			    opj_image_t * p_image,
			    OPJ_INT32 p_start_x,
			    OPJ_INT32 p_end_x,
			    OPJ_INT32 p_start_y,
			    OPJ_INT32 p_end_y,
			    struct opj_event_mgr * p_manager);

			/** Get tile function */
			boolint (* opj_get_decoded_tile)(void * p_codec,
			    opj_stream_private_t * p_cio,
			    opj_image_t * p_image,
			    struct opj_event_mgr * p_manager,
			    OPJ_UINT32 tile_index);

			/** Set the decoded resolution factor */
			boolint (* opj_set_decoded_resolution_factor)(void * p_codec,
			    OPJ_UINT32 res_factor,
			    opj_event_mgr_t * p_manager);

			/** Set the decoded components */
			boolint (* opj_set_decoded_components)(void * p_codec,
			    OPJ_UINT32 num_comps,
			    const OPJ_UINT32* comps_indices,
			    opj_event_mgr_t * p_manager);
		} m_decompression;

		/**
		 * Compression handler. FIXME DOC
		 */
		struct opj_compression {
			boolint (* opj_start_compress)(void * p_codec,
			    struct opj_stream_private * cio,
			    struct opj_image * p_image,
			    struct opj_event_mgr * p_manager);

			boolint (* opj_encode)(void * p_codec,
			    struct opj_stream_private * p_cio,
			    struct opj_event_mgr * p_manager);

			boolint (* opj_write_tile)(void * p_codec,
			    OPJ_UINT32 p_tile_index,
			    uint8 * p_data,
			    OPJ_UINT32 p_data_size,
			    struct opj_stream_private * p_cio,
			    struct opj_event_mgr * p_manager);

			boolint (* opj_end_compress)(void * p_codec,
			    struct opj_stream_private * p_cio,
			    struct opj_event_mgr * p_manager);

			void (* opj_destroy)(void * p_codec);

			boolint (* opj_setup_encoder)(void * p_codec,
			    opj_cparameters_t * p_param,
			    struct opj_image * p_image,
			    struct opj_event_mgr * p_manager);

			boolint (* opj_encoder_set_extra_options)(void * p_codec,
			    const char* const* p_options,
			    struct opj_event_mgr * p_manager);
		} m_compression;
	} m_codec_data;

	/** FIXME DOC*/
	void * m_codec;
	/** Event handler */
	opj_event_mgr_t m_event_mgr;
	/** Flag to indicate if the codec is used to decode or encode*/
	boolint is_decompressor;
	void (* opj_dump_codec)(void * p_codec, OPJ_INT32 info_flag,
	    FILE* output_stream);
	opj_codestream_info_v2_t* (*opj_get_codec_info)(void * p_codec);
	opj_codestream_index_t* (*opj_get_codec_index)(void * p_codec);

	/** Set number of threads */
	boolint (* opj_set_threads)(void * p_codec, OPJ_UINT32 num_threads);
}

opj_codec_private_t;

#endif /* OPJ_CODEC_H */
