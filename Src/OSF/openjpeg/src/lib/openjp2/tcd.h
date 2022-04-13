/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2008, 2011-2012, Centre National d'Etudes Spatiales (CNES), FR
 * Copyright (c) 2012, CS Systemes d'Information, France
 * Copyright (c) 2017, IntoPIX SA <support@intopix.com>
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
#ifndef OPJ_TCD_H
#define OPJ_TCD_H
/**
   @file tcd.h
   @brief Implementation of a tile coder/decoder (TCD)

   The functions in TCD.C encode or decode each tile independently from
   each other. The functions in TCD.C are used by other functions in J2K.C.
 */

/** @defgroup TCD TCD - Implementation of a tile coder/decoder */
/*@{*/

/**
   FIXME DOC
 */
typedef struct opj_tcd_pass {
	uint32_t rate;
	double distortiondec;
	uint32_t len;
	OPJ_BITFIELD term : 1;
} opj_tcd_pass_t;

/**
   FIXME DOC
 */
typedef struct opj_tcd_layer {
	uint32_t numpasses;   /* Number of passes in the layer */
	uint32_t len;         /* len of information */
	double disto;      /* add for index (Cfr. Marcela) */
	uint8 * data;        /* data */
} opj_tcd_layer_t;

/**
   FIXME DOC
 */
typedef struct opj_tcd_cblk_enc {
	uint8* data;           /* Data */
	opj_tcd_layer_t* layers;  /* layer information */
	opj_tcd_pass_t* passes;   /* information about the passes */
	int32_t x0, y0, x1,
	    y1;       /* dimension of the code-blocks : left upper corner (x0, y0) right low corner (x1,y1) */
	uint32_t numbps;
	uint32_t numlenbits;
	uint32_t data_size;     /* Size of allocated data buffer */
	uint32_t
	    numpasses; /* number of pass already done for the code-blocks */
	uint32_t numpassesinlayers; /* number of passes in the layer */
	uint32_t totalpasses;   /* total number of passes */
} opj_tcd_cblk_enc_t;

/** Chunk of codestream data that is part of a code block */
typedef struct opj_tcd_seg_data_chunk {
	/* Point to tilepart buffer. We don't make a copy !
	   So the tilepart buffer must be kept alive
	   as long as we need to decode the codeblocks */
	uint8 * data;
	uint32_t len;             /* Usable length of data */
} opj_tcd_seg_data_chunk_t;

/** Segment of a code-block.
 * A segment represent a number of consecutive coding passes, without termination
 * of MQC or RAW between them. */
typedef struct opj_tcd_seg {
	uint32_t len;  /* Size of data related to this segment */
	/* Number of passes decoded. Including those that we skip */
	uint32_t numpasses;
	/* Number of passes actually to be decoded. To be used for code-block decoding */
	uint32_t real_num_passes;
	/* Maximum number of passes for this segment */
	uint32_t maxpasses;
	/* Number of new passes for current packed. Transitory value */
	uint32_t numnewpasses;
	/* Codestream length for this segment for current packed. Transitory value */
	uint32_t newlen;
} opj_tcd_seg_t;

/** Code-block for decoding */
typedef struct opj_tcd_cblk_dec {
	opj_tcd_seg_t* segs;        /* segments information */
	opj_tcd_seg_data_chunk_t* chunks; /* Array of chunks */
	/* position of the code-blocks : left upper corner (x0, y0) right low corner (x1,y1) */
	int32_t x0, y0, x1, y1;
	/* Mb is The maximum number of bit-planes available for the representation of
	   coefficients in any sub-band, b, as defined in Equation (E-2). See
	   Section B.10.5 of the standard */
	uint32_t Mb; /* currently used only to check if HT decoding is correct */
	/* numbps is Mb - P as defined in Section B.10.5 of the standard */
	uint32_t numbps;
	/* number of bits for len, for the current packet. Transitory value */
	uint32_t numlenbits;
	/* number of pass added to the code-blocks, for the current packet. Transitory value */
	uint32_t numnewpasses;
	/* number of segments, including those of packet we skip */
	uint32_t numsegs;
	/* number of segments, to be used for code block decoding */
	uint32_t real_num_segs;
	uint32_t m_current_max_segs; /* allocated number of segs[] items */
	uint32_t numchunks;       /* Number of valid chunks items */
	uint32_t numchunksalloc;  /* Number of chunks item allocated */
	/* Decoded code-block. Only used for subtile decoding. Otherwise tilec->data is directly updated */
	int32_t* decoded_data;
} opj_tcd_cblk_dec_t;

/** Precinct structure */
typedef struct opj_tcd_precinct {
	/* dimension of the precinct : left upper corner (x0, y0) right low corner (x1,y1) */
	int32_t x0, y0, x1, y1;
	uint32_t cw, ch;          /* number of code-blocks, in width and height */
	union {                     /* code-blocks information */
		opj_tcd_cblk_enc_t* enc;
		opj_tcd_cblk_dec_t* dec;
		void*               blocks;
	} cblks;

	uint32_t block_size;      /* size taken by cblks (in bytes) */
	opj_tgt_tree_t * incltree;  /* inclusion tree */
	opj_tgt_tree_t * imsbtree;  /* IMSB tree */
} opj_tcd_precinct_t;

/** Sub-band structure */
typedef struct opj_tcd_band {
	/* dimension of the subband : left upper corner (x0, y0) right low corner (x1,y1) */
	int32_t x0, y0, x1, y1;
	/* band number: for lowest resolution level (0=LL), otherwise (1=HL, 2=LH, 3=HH) */
	uint32_t bandno;
	/* precinct information */
	opj_tcd_precinct_t * precincts;
	/* size of data taken by precincts */
	uint32_t precincts_data_size;
	int32_t numbps;
	float stepsize;
} opj_tcd_band_t;

/** Tile-component resolution structure */
typedef struct opj_tcd_resolution {
	/* dimension of the resolution level : left upper corner (x0, y0) right low corner (x1,y1) */
	int32_t x0, y0, x1, y1;
	/* number of precincts, in width and height, for this resolution level */
	uint32_t pw, ph;
	/* number of sub-bands for the resolution level (1 for lowest resolution level, 3 otherwise) */
	uint32_t numbands;
	/* subband information */
	opj_tcd_band_t bands[3];

	/* dimension of the resolution limited to window of interest. Only valid if tcd->whole_tile_decoding is set */
	uint32_t win_x0;
	uint32_t win_y0;
	uint32_t win_x1;
	uint32_t win_y1;
} opj_tcd_resolution_t;

/** Tile-component structure */
typedef struct opj_tcd_tilecomp {
	/* dimension of component : left upper corner (x0, y0) right low corner (x1,y1) */
	int32_t x0, y0, x1, y1;
	/* component number */
	uint32_t compno;
	/* number of resolutions level */
	uint32_t numresolutions;
	/* number of resolutions level to decode (at max)*/
	uint32_t minimum_num_resolutions;
	/* resolutions information */
	opj_tcd_resolution_t * resolutions;
	/* size of data for resolutions (in bytes) */
	uint32_t resolutions_size;

	/* data of the component. For decoding, only valid if tcd->whole_tile_decoding is set (so exclusive of data_win
	   member) */
	int32_t * data;
	/* if true, then need to free after usage, otherwise do not free */
	boolint ownsData;
	/* we may either need to allocate this amount of data, or re-use image data and ignore this value */
	size_t data_size_needed;
	/* size of the data of the component */
	size_t data_size;

	/** data of the component limited to window of interest. Only valid for decoding and if tcd->whole_tile_decoding
	   is NOT set (so exclusive of data member) */
	int32_t * data_win;
	/* dimension of the component limited to window of interest. Only valid for decoding and  if
	   tcd->whole_tile_decoding is NOT set */
	uint32_t win_x0;
	uint32_t win_y0;
	uint32_t win_x1;
	uint32_t win_y1;

	/* add fixed_quality */
	int32_t numpix;
} opj_tcd_tilecomp_t;

/**
   FIXME DOC
 */
typedef struct opj_tcd_tile {
	/* dimension of the tile : left upper corner (x0, y0) right low corner (x1,y1) */
	int32_t x0, y0, x1, y1;
	uint32_t numcomps;        /* number of components in tile */
	opj_tcd_tilecomp_t * comps; /* Components information */
	int32_t numpix;           /* add fixed_quality */
	double distotile;      /* add fixed_quality */
	double distolayer[100]; /* add fixed_quality */
	uint32_t packno;          /* packet number */
} opj_tcd_tile_t;

/**
   FIXME DOC
 */
typedef struct opj_tcd_image {
	opj_tcd_tile_t * tiles; /* Tiles information */
}

opj_tcd_image_t;

/**
   Tile coder/decoder
 */
typedef struct opj_tcd {
	/** Position of the tilepart flag in Progression order*/
	int32_t tp_pos;
	/** Tile part number*/
	uint32_t tp_num;
	/** Current tile part number*/
	uint32_t cur_tp_num;
	/** Total number of tileparts of the current tile*/
	uint32_t cur_totnum_tp;
	/** Current Packet iterator number */
	uint32_t cur_pino;
	/** info on each image tile */
	opj_tcd_image_t * tcd_image;
	/** image header */
	opj_image_t * image;
	/** coding parameters */
	opj_cp_t * cp;
	/** coding/decoding parameters common to all tiles */
	opj_tcp_t * tcp;
	/** current encoded/decoded tile */
	uint32_t tcd_tileno;
	/** tell if the tcd is a decoder. */
	OPJ_BITFIELD m_is_decoder : 1;
	/** Thread pool */
	opj_thread_pool_t* thread_pool;
	/** Coordinates of the window of interest, in grid reference space */
	uint32_t win_x0;
	uint32_t win_y0;
	uint32_t win_x1;
	uint32_t win_y1;
	/** Only valid for decoding. Whether the whole tile is decoded, or just the region in
	   win_x0/win_y0/win_x1/win_y1 */
	boolint whole_tile_decoding;
	/* Array of size image->numcomps indicating if a component must be decoded. NULL if all components must be
	   decoded */
	boolint* used_component;
} opj_tcd_t;

/**
 * Structure to hold information needed to generate some markers.
 * Used by encoder.
 */
typedef struct opj_tcd_marker_info {
	/** In: Whether information to generate PLT markers in needed */
	boolint need_PLT;

	/** OUT: Number of elements in p_packet_size[] array */
	uint32_t packet_count;

	/** OUT: Array of size packet_count, such that p_packet_size[i] is
	 *       the size in bytes of the ith packet */
	uint32_t* p_packet_size;
} opj_tcd_marker_info_t;

/** @name Exported functions */
/*@{*/
/**
   Dump the content of a tcd structure
 */
/*void tcd_dump(FILE *fd, opj_tcd_t *tcd, opj_tcd_image_t *img);*/ /* TODO MSD shoul use the new v2 structures */

/**
   Create a new TCD handle
   @param p_is_decoder FIXME DOC
   @return Returns a new TCD handle if successful returns NULL otherwise
 */
opj_tcd_t* opj_tcd_create(boolint p_is_decoder);

/**
   Destroy a previously created TCD handle
   @param tcd TCD handle to destroy
 */
void opj_tcd_destroy(opj_tcd_t * tcd);

/**
 * Create a new opj_tcd_marker_info_t* structure
 * @param need_PLT Whether information is needed to generate PLT markers.
 */
opj_tcd_marker_info_t* opj_tcd_marker_info_create(boolint need_PLT);

/**
   Destroy a previously created opj_tcd_marker_info_t* structure
   @param p_tcd_marker_info Structure to destroy
 */
void opj_tcd_marker_info_destroy(opj_tcd_marker_info_t * p_tcd_marker_info);

/**
 * Initialize the tile coder and may reuse some memory.
 * @param   p_tcd       TCD handle.
 * @param   p_image     raw image.
 * @param   p_cp        coding parameters.
 * @param   p_tp        thread pool
 *
 * @return true if the encoding values could be set (false otherwise).
 */
boolint opj_tcd_init(opj_tcd_t * p_tcd,
    opj_image_t * p_image,
    opj_cp_t * p_cp,
    opj_thread_pool_t* p_tp);

/**
 * Allocates memory for decoding a specific tile.
 *
 * @param   p_tcd       the tile decoder.
 * @param   p_tile_no   the index of the tile received in sequence. This not necessarily lead to the
 * tile at index p_tile_no.
 * @param p_manager the event manager.
 *
 * @return  true if the remaining data is sufficient.
 */
boolint opj_tcd_init_decode_tile(opj_tcd_t * p_tcd, uint32_t p_tile_no,
    opj_event_mgr_t* p_manager);

void opj_tcd_makelayer_fixed(opj_tcd_t * tcd, uint32_t layno,
    uint32_t final);

void opj_tcd_rateallocate_fixed(opj_tcd_t * tcd);

void opj_tcd_makelayer(opj_tcd_t * tcd,
    uint32_t layno,
    double thresh,
    uint32_t final);

boolint opj_tcd_rateallocate(opj_tcd_t * tcd,
    uint8 * dest,
    uint32_t * p_data_written,
    uint32_t len,
    opj_codestream_info_t * cstr_info,
    opj_event_mgr_t * p_manager);

/**
 * Gets the maximum tile size that will be taken by the tile once decoded.
 */
uint32_t opj_tcd_get_decoded_tile_size(opj_tcd_t * p_tcd,
    boolint take_into_account_partial_decoding);

/**
 * Encodes a tile from the raw image into the given buffer.
 * @param   p_tcd           Tile Coder handle
 * @param   p_tile_no       Index of the tile to encode.
 * @param   p_dest          Destination buffer
 * @param   p_data_written  pointer to an int that is incremented by the number of bytes really written on p_dest
 * @param   p_len           Maximum length of the destination buffer
 * @param   p_cstr_info     Codestream information structure
 * @param   p_marker_info   Marker information structure
 * @param   p_manager       the user event manager
 * @return  true if the coding is successful.
 */
boolint opj_tcd_encode_tile(opj_tcd_t * p_tcd,
    uint32_t p_tile_no,
    uint8 * p_dest,
    uint32_t * p_data_written,
    uint32_t p_len,
    struct opj_codestream_info * p_cstr_info,
    opj_tcd_marker_info_t* p_marker_info,
    opj_event_mgr_t * p_manager);

/**
   Decode a tile from a buffer into a raw image
   @param tcd TCD handle
   @param win_x0 Upper left x of region to decode (in grid coordinates)
   @param win_y0 Upper left y of region to decode (in grid coordinates)
   @param win_x1 Lower right x of region to decode (in grid coordinates)
   @param win_y1 Lower right y of region to decode (in grid coordinates)
   @param numcomps_to_decode  Size of the comps_indices array, or 0 if decoding all components.
   @param comps_indices   Array of numcomps values representing the indices
                       of the components to decode (relative to the
                       codestream, starting at 0). Or NULL if decoding all components.
   @param src Source buffer
   @param len Length of source buffer
   @param tileno Number that identifies one of the tiles to be decoded
   @param cstr_info  FIXME DOC
   @param manager the event manager.
 */
boolint opj_tcd_decode_tile(opj_tcd_t * tcd,
    uint32_t win_x0,
    uint32_t win_y0,
    uint32_t win_x1,
    uint32_t win_y1,
    uint32_t numcomps_to_decode,
    const uint32_t * comps_indices,
    uint8 * src,
    uint32_t len,
    uint32_t tileno,
    opj_codestream_index_t * cstr_info,
    opj_event_mgr_t * manager);

/**
 * Copies tile data from the system onto the given memory block.
 */
boolint opj_tcd_update_tile_data(opj_tcd_t * p_tcd,
    uint8 * p_dest,
    uint32_t p_dest_length);

/**
 * Get the size in bytes of the input buffer provided before encoded.
 * This must be the size provided to the p_src_length argument of
 * opj_tcd_copy_tile_data()
 */
size_t opj_tcd_get_encoder_input_buffer_size(opj_tcd_t * p_tcd);

/**
 * Initialize the tile coder and may reuse some meory.
 *
 * @param   p_tcd       TCD handle.
 * @param   p_tile_no   current tile index to encode.
 * @param p_manager the event manager.
 *
 * @return true if the encoding values could be set (false otherwise).
 */
boolint opj_tcd_init_encode_tile(opj_tcd_t * p_tcd,
    uint32_t p_tile_no, opj_event_mgr_t* p_manager);

/**
 * Copies tile data from the given memory block onto the system.
 *
 * p_src_length must be equal to opj_tcd_get_encoder_input_buffer_size()
 */
boolint opj_tcd_copy_tile_data(opj_tcd_t * p_tcd,
    uint8 * p_src,
    size_t p_src_length);

/**
 * Allocates tile component data
 *
 *
 */
boolint opj_alloc_tile_component_data(opj_tcd_tilecomp_t * l_tilec);

/** Returns whether a sub-band is empty (i.e. whether it has a null area)
 * @param band Sub-band handle.
 * @return TRUE whether the sub-band is empty.
 */
boolint opj_tcd_is_band_empty(opj_tcd_band_t* band);

/** Reinitialize a segment */
void opj_tcd_reinit_segment(opj_tcd_seg_t* seg);

/** Returns whether a sub-band region contributes to the area of interest
 * tcd->win_x0,tcd->win_y0,tcd->win_x1,tcd->win_y1.
 *
 * @param tcd    TCD handle.
 * @param compno Component number
 * @param resno  Resolution number
 * @param bandno Band number (*not* band index, ie 0, 1, 2 or 3)
 * @param x0     Upper left x in subband coordinates
 * @param y0     Upper left y in subband coordinates
 * @param x1     Lower right x in subband coordinates
 * @param y1     Lower right y in subband coordinates
 * @return TRUE whether the sub-band region contributs to the area of
 *                  interest.
 */
boolint opj_tcd_is_subband_area_of_interest(opj_tcd_t * tcd, uint32_t compno, uint32_t resno, uint32_t bandno,
    uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
/*@}*/
/*@}*/
#endif /* OPJ_TCD_H */
