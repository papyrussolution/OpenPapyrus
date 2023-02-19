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
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * Copyright (c) 2008, Jerome Fimes, Communications & Systemes <jerome.fimes@c-s.fr>
 * Copyright (c) 2011-2012, Centre National d'Etudes Spatiales (CNES), France
 * Copyright (c) 2012, CS Systemes d'Information, France
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef OPJ_J2K_H
#define OPJ_J2K_H
/**
   @file j2k.h
   @brief The JPEG-2000 Codestream Reader/Writer (J2K)

   The functions in J2K.C have for goal to read/write the several parts of the codestream: markers and data.
 */

/** @defgroup J2K J2K - JPEG-2000 codestream reader/writer */
/*@{*/

#define J2K_CP_CSTY_PRT 0x01
#define J2K_CP_CSTY_SOP 0x02
#define J2K_CP_CSTY_EPH 0x04
#define J2K_CCP_CSTY_PRT 0x01
#define J2K_CCP_CBLKSTY_LAZY 0x01     /**< Selective arithmetic coding bypass */
#define J2K_CCP_CBLKSTY_RESET 0x02    /**< Reset context probabilities on coding pass boundaries */
#define J2K_CCP_CBLKSTY_TERMALL 0x04  /**< Termination on each coding pass */
#define J2K_CCP_CBLKSTY_VSC 0x08      /**< Vertically stripe causal context */
#define J2K_CCP_CBLKSTY_PTERM 0x10    /**< Predictable termination */
#define J2K_CCP_CBLKSTY_SEGSYM 0x20   /**< Segmentation symbols are used */
#define J2K_CCP_CBLKSTY_HT 0x40       /**< (high throughput) HT codeblocks */
#define J2K_CCP_CBLKSTY_HTMIXED 0x80  /**< MIXED mode HT codeblocks */
#define J2K_CCP_QNTSTY_NOQNT 0
#define J2K_CCP_QNTSTY_SIQNT 1
#define J2K_CCP_QNTSTY_SEQNT 2

#define J2K_MS_SOC 0xff4f   /**< SOC marker value */
#define J2K_MS_SOT 0xff90   /**< SOT marker value */
#define J2K_MS_SOD 0xff93   /**< SOD marker value */
#define J2K_MS_EOC 0xffd9   /**< EOC marker value */
#define J2K_MS_CAP 0xff50   /**< CAP marker value */
#define J2K_MS_SIZ 0xff51   /**< SIZ marker value */
#define J2K_MS_COD 0xff52   /**< COD marker value */
#define J2K_MS_COC 0xff53   /**< COC marker value */
#define J2K_MS_CPF 0xff59   /**< CPF marker value */
#define J2K_MS_RGN 0xff5e   /**< RGN marker value */
#define J2K_MS_QCD 0xff5c   /**< QCD marker value */
#define J2K_MS_QCC 0xff5d   /**< QCC marker value */
#define J2K_MS_POC 0xff5f   /**< POC marker value */
#define J2K_MS_TLM 0xff55   /**< TLM marker value */
#define J2K_MS_PLM 0xff57   /**< PLM marker value */
#define J2K_MS_PLT 0xff58   /**< PLT marker value */
#define J2K_MS_PPM 0xff60   /**< PPM marker value */
#define J2K_MS_PPT 0xff61   /**< PPT marker value */
#define J2K_MS_SOP 0xff91   /**< SOP marker value */
#define J2K_MS_EPH 0xff92   /**< EPH marker value */
#define J2K_MS_CRG 0xff63   /**< CRG marker value */
#define J2K_MS_COM 0xff64   /**< COM marker value */
#define J2K_MS_CBD 0xff78   /**< CBD marker value */
#define J2K_MS_MCC 0xff75   /**< MCC marker value */
#define J2K_MS_MCT 0xff74   /**< MCT marker value */
#define J2K_MS_MCO 0xff77   /**< MCO marker value */

#define J2K_MS_UNK 0        /**< UNKNOWN marker value */

/* UniPG>> */
#ifdef USE_JPWL
#define J2K_MS_EPC 0xff68   /**< EPC marker value (Part 11: JPEG 2000 for Wireless) */
#define J2K_MS_EPB 0xff66   /**< EPB marker value (Part 11: JPEG 2000 for Wireless) */
#define J2K_MS_ESD 0xff67   /**< ESD marker value (Part 11: JPEG 2000 for Wireless) */
#define J2K_MS_RED 0xff69   /**< RED marker value (Part 11: JPEG 2000 for Wireless) */
#endif /* USE_JPWL */
#ifdef USE_JPSEC
#define J2K_MS_SEC 0xff65    /**< SEC marker value (Part 8: Secure JPEG 2000) */
#define J2K_MS_INSEC 0xff94  /**< INSEC marker value (Part 8: Secure JPEG 2000) */
#endif /* USE_JPSEC */
/* <<UniPG */

#define J2K_MAX_POCS    32      /**< Maximum number of POCs */
/**
 * Values that specify the status of the decoding process when decoding the main header.
 * These values may be combined with a | operator.
 * */
typedef enum J2K_STATUS {
	J2K_STATE_NONE  =  0x0000,/**< a SOC marker is expected */
	J2K_STATE_MHSOC  = 0x0001,/**< a SOC marker is expected */
	J2K_STATE_MHSIZ  = 0x0002,/**< a SIZ marker is expected */
	J2K_STATE_MH     = 0x0004,/**< the decoding process is in the main header */
	J2K_STATE_TPHSOT = 0x0008, /**< the decoding process is in a tile part header and expects a SOT marker */
	J2K_STATE_TPH    = 0x0010,/**< the decoding process is in a tile part header */
	J2K_STATE_MT     = 0x0020,/**< the EOC marker has just been read */
	J2K_STATE_NEOC   = 0x0040,/**< the decoding process must not expect a EOC marker because the codestream is truncated */
	J2K_STATE_DATA   = 0x0080,/**< a tile header has been successfully read and codestream is expected */
	J2K_STATE_EOC    = 0x0100,/**< the decoding process has encountered the EOC marker */
	J2K_STATE_ERR    = 0x8000/**< the decoding process has encountered an error (FIXME warning V1 = 0x0080)*/
} J2K_STATUS;

/**
 * Type of elements storing in the MCT data
 */
typedef enum MCT_ELEMENT_TYPE {
	MCT_TYPE_INT16 = 0, /** MCT data is stored as signed shorts*/
	MCT_TYPE_INT32 = 1, /** MCT data is stored as signed integers*/
	MCT_TYPE_FLOAT = 2, /** MCT data is stored as floats*/
	MCT_TYPE_DOUBLE = 3 /** MCT data is stored as doubles*/
} J2K_MCT_ELEMENT_TYPE;

/**
 * Type of MCT array
 */
typedef enum MCT_ARRAY_TYPE {
	MCT_TYPE_DEPENDENCY = 0,
	MCT_TYPE_DECORRELATION = 1,
	MCT_TYPE_OFFSET = 2
} J2K_MCT_ARRAY_TYPE;
/**
   T2 encoding mode
 */
typedef enum T2_MODE {
	THRESH_CALC = 0, /** Function called in Rate allocation process*/
	FINAL_PASS = 1  /** Function called in Tier 2 process*/
} J2K_T2_MODE;

/**
 * Quantization stepsize
 */
typedef struct opj_stepsize {
	int32_t expn; /** exponent */
	int32_t mant; /** mantissa */
} opj_stepsize_t;

/**
   Tile-component coding parameters
 */
typedef struct opj_tccp {
	uint32_t csty; /** coding style */
	uint32_t numresolutions; /** number of resolutions */
	uint32_t cblkw; /** code-blocks width */
	uint32_t cblkh; /** code-blocks height */
	uint32_t cblksty; /** code-block coding style */
	uint32_t qmfbid; /** discrete wavelet transform identifier */
	uint32_t qntsty; /** quantisation style */
	opj_stepsize_t stepsizes[OPJ_J2K_MAXBANDS]; /** stepsizes used for quantization */
	uint32_t numgbits; /** number of guard bits */
	int32_t roishift; /** Region Of Interest shift */
	uint32_t prcw[OPJ_J2K_MAXRLVLS]; /** precinct width */
	uint32_t prch[OPJ_J2K_MAXRLVLS]; /** precinct height */
	int32_t m_dc_level_shift; /** the dc_level_shift **/
} opj_tccp_t;
/**
 * FIXME DOC
 */
typedef struct opj_mct_data {
	J2K_MCT_ELEMENT_TYPE m_element_type;
	J2K_MCT_ARRAY_TYPE m_array_type;
	uint32_t m_index;
	uint8 *           m_data;
	uint32_t m_data_size;
} opj_mct_data_t;

/**
 * FIXME DOC
 */
typedef struct opj_simple_mcc_decorrelation_data {
	uint32_t m_index;
	uint32_t m_nb_comps;
	opj_mct_data_t *     m_decorrelation_array;
	opj_mct_data_t *     m_offset_array;
	OPJ_BITFIELD m_is_irreversible : 1;
} opj_simple_mcc_decorrelation_data_t;

typedef struct opj_ppx_struct {
	uint8*   m_data;/* m_data == NULL => Zppx not read yet */
	uint32_t m_data_size;
} opj_ppx;
/**
   Tile coding parameters :
   this structure is used to store coding/decoding parameters common to all
   tiles (information like COD, COC in main header)
 */
typedef struct opj_tcp {
	/** coding style */
	uint32_t csty;
	/** progression order */
	OPJ_PROG_ORDER prg;
	/** number of layers */
	uint32_t numlayers;
	uint32_t num_layers_to_decode;
	/** multi-component transform identifier */
	uint32_t mct;
	/** rates of layers */
	float rates[100];
	/** number of progression order changes */
	uint32_t numpocs;
	/** progression order changes */
	opj_poc_t pocs[J2K_MAX_POCS];

	/** number of ppt markers (reserved size) */
	uint32_t ppt_markers_count;
	/** ppt markers data (table indexed by Zppt) */
	opj_ppx* ppt_markers;

	/** packet header store there for future use in t2_decode_packet */
	uint8 * ppt_data;
	/** used to keep a track of the allocated memory */
	uint8 * ppt_buffer;
	/** Number of bytes stored inside ppt_data*/
	uint32_t ppt_data_size;
	/** size of ppt_data*/
	uint32_t ppt_len;
	/** add fixed_quality */
	float distoratio[100];
	/** tile-component coding parameters */
	opj_tccp_t * tccps;
	/** current tile part number or -1 if first time into this tile */
	int32_t m_current_tile_part_number;
	/** number of tile parts for the tile. */
	uint32_t m_nb_tile_parts;
	/** data for the tile */
	uint8 *      m_data;
	/** size of data */
	uint32_t m_data_size;
	/** encoding norms */
	double *   mct_norms;
	/** the mct decoding matrix */
	float *   m_mct_decoding_matrix;
	/** the mct coding matrix */
	float *   m_mct_coding_matrix;
	/** mct records */
	opj_mct_data_t * m_mct_records;
	/** the number of mct records. */
	uint32_t m_nb_mct_records;
	/** the max number of mct records. */
	uint32_t m_nb_max_mct_records;
	/** mcc records */
	opj_simple_mcc_decorrelation_data_t * m_mcc_records;
	/** the number of mct records. */
	uint32_t m_nb_mcc_records;
	/** the max number of mct records. */
	uint32_t m_nb_max_mcc_records;

	/***** FLAGS *******/
	/** If cod == 1 --> there was a COD marker for the present tile */
	OPJ_BITFIELD cod : 1;
	/** If ppt == 1 --> there was a PPT marker for the present tile */
	OPJ_BITFIELD ppt : 1;
	/** indicates if a POC marker has been used O:NO, 1:YES */
	OPJ_BITFIELD POC : 1;
} opj_tcp_t;

typedef struct opj_encoding_param {
	/** Maximum rate for each component. If == 0, component size limitation is not considered */
	uint32_t m_max_comp_size;
	/** Position of tile part flag in progression order*/
	int32_t m_tp_pos;
	/** fixed layer */
	int32_t * m_matrice;
	/** Flag determining tile part generation*/
	uint8 m_tp_flag;
	/** allocation by rate/distortion */
	OPJ_BITFIELD m_disto_alloc : 1;
	/** allocation by fixed layer */
	OPJ_BITFIELD m_fixed_alloc : 1;
	/** add fixed_quality */
	OPJ_BITFIELD m_fixed_quality : 1;
	/** Enabling Tile part generation*/
	OPJ_BITFIELD m_tp_on : 1;
} opj_encoding_param_t;

typedef struct opj_decoding_param {
	uint32_t m_reduce; /** if != 0, then original dimension divided by 2^(reduce); if == 0 or not used, image is decoded to the full resolution */
	uint32_t m_layer; /** if != 0, then only the first "layer" layers are decoded; if == 0 or not used, all the quality layers are decoded */
} opj_decoding_param_t;
/**
 * Coding parameters
 */
typedef struct opj_cp {
	/** Size of the image in bits*/
	/*int img_size;*/
	uint16 rsiz; /** Rsiz*/
	/** XTOsiz */
	uint32_t tx0; /* MSD see norm */
	/** YTOsiz */
	uint32_t ty0; /* MSD see norm */
	uint32_t tdx; /** XTsiz */
	uint32_t tdy; /** YTsiz */
	char * comment; /** comment */
	uint32_t tw; /** number of tiles in width */
	uint32_t th; /** number of tiles in height */
	uint32_t ppm_markers_count; /** number of ppm markers (reserved size) */
	opj_ppx* ppm_markers; /** ppm markers data (table indexed by Zppm) */
	uint8 * ppm_data; /** packet header store there for future use in t2_decode_packet */
	/** size of the ppm_data*/
	uint32_t ppm_len;
	/** size of the ppm_data*/
	uint32_t ppm_data_read;
	uint8 * ppm_data_current;
	/** packet header storage original buffer */
	uint8 * ppm_buffer;
	/** pointer remaining on the first byte of the first header if ppm is used */
	uint8 * ppm_data_first;
	/** Number of bytes actually stored inside the ppm_data */
	uint32_t ppm_data_size;
	/** use in case of multiple marker PPM (number of info already store) */
	int32_t ppm_store;
	/** use in case of multiple marker PPM (case on non-finished previous info) */
	int32_t ppm_previous;
	/** tile coding parameters */
	opj_tcp_t * tcps;
	union {
		opj_decoding_param_t m_dec;
		opj_encoding_param_t m_enc;
	} m_specific_param;
	/** TRUE if entire bit stream must be decoded, FALSE if partial bitstream decoding allowed */
	boolint strict;
	/* UniPG>> */
#ifdef USE_JPWL
	/** enables writing of EPC in MH, thus activating JPWL */
	boolint epc_on;
	/** enables writing of EPB, in case of activated JPWL */
	boolint epb_on;
	/** enables writing of ESD, in case of activated JPWL */
	boolint esd_on;
	/** enables writing of informative techniques of ESD, in case of activated JPWL */
	boolint info_on;
	/** enables writing of RED, in case of activated JPWL */
	boolint red_on;
	/** error protection method for MH (0,1,16,32,37-128) */
	int hprot_MH;
	/** tile number of header protection specification (>=0) */
	int hprot_TPH_tileno[JPWL_MAX_NO_TILESPECS];
	/** error protection methods for TPHs (0,1,16,32,37-128) */
	int hprot_TPH[JPWL_MAX_NO_TILESPECS];
	/** tile number of packet protection specification (>=0) */
	int pprot_tileno[JPWL_MAX_NO_PACKSPECS];
	/** packet number of packet protection specification (>=0) */
	int pprot_packno[JPWL_MAX_NO_PACKSPECS];
	/** error protection methods for packets (0,1,16,32,37-128) */
	int pprot[JPWL_MAX_NO_PACKSPECS];
	/** enables writing of ESD, (0/2/4 bytes) */
	int sens_size;
	/** sensitivity addressing size (0=auto/2/4 bytes) */
	int sens_addr;
	/** sensitivity range (0-3) */
	int sens_range;
	/** sensitivity method for MH (-1,0-7) */
	int sens_MH;
	/** tile number of sensitivity specification (>=0) */
	int sens_TPH_tileno[JPWL_MAX_NO_TILESPECS];
	/** sensitivity methods for TPHs (-1,0-7) */
	int sens_TPH[JPWL_MAX_NO_TILESPECS];
	/** enables JPWL correction at the decoder */
	boolint correct;
	/** expected number of components at the decoder */
	int exp_comps;
	/** maximum number of tiles at the decoder */
	uint32_t max_tiles;
#endif /* USE_JPWL */

	/******** FLAGS *********/
	/** if ppm == 1 --> there was a PPM marker*/
	OPJ_BITFIELD ppm : 1;
	/** tells if the parameter is a coding or decoding one */
	OPJ_BITFIELD m_is_decoder : 1;
	/** whether different bit depth or sign per component is allowed. Decoder only for ow */
	OPJ_BITFIELD allow_different_bit_depth_sign : 1;
	/* <<UniPG */
} opj_cp_t;

typedef struct opj_j2k_dec {
	/** locate in which part of the codestream the decoder is (main header, tile header, end) */
	uint32_t m_state;
	/**
	 * store decoding parameters common to all tiles (information like COD, COC in main header)
	 */
	opj_tcp_t * m_default_tcp;
	uint8  * m_header_data;
	uint32_t m_header_data_size;
	/** to tell the tile part length */
	uint32_t m_sot_length;
	/** Only tiles index in the correct range will be decoded.*/
	uint32_t m_start_tile_x;
	uint32_t m_start_tile_y;
	uint32_t m_end_tile_x;
	uint32_t m_end_tile_y;

	/** Index of the tile to decode (used in get_tile) */
	int32_t m_tile_ind_to_dec;
	/** Position of the last SOT marker read */
	OPJ_OFF_T m_last_sot_read_pos;

	/**
	 * Indicate that the current tile-part is assume as the last tile part of the codestream.
	 * It is useful in the case of PSot is equal to zero. The sot length will be compute in the
	 * SOD reader function. FIXME NOT USED for the moment
	 */
	boolint m_last_tile_part;

	uint32_t m_numcomps_to_decode;
	uint32_t  * m_comps_indices_to_decode;

	/** to tell that a tile can be decoded. */
	OPJ_BITFIELD m_can_decode : 1;
	OPJ_BITFIELD m_discard_tiles : 1;
	OPJ_BITFIELD m_skip_data : 1;
	/** TNsot correction : see issue 254 **/
	OPJ_BITFIELD m_nb_tile_parts_correction_checked : 1;
	OPJ_BITFIELD m_nb_tile_parts_correction : 1;
} opj_j2k_dec_t;

typedef struct opj_j2k_enc {
	/** Tile part number, regardless of poc, for each new poc, tp is reset to 1*/
	uint32_t m_current_poc_tile_part_number; /* tp_num */
	/** Tile part number currently coding, taking into account POC. m_current_tile_part_number holds the total
	   number of tile parts while encoding the last tile part.*/
	uint32_t m_current_tile_part_number; /*cur_tp_num */
	/* whether to generate TLM markers */
	boolint m_TLM;
	/* whether the Ttlmi field in a TLM marker is a byte (otherwise a uint16) */
	boolint m_Ttlmi_is_byte;
	/**
	   locate the start position of the TLM marker
	   after encoding the tilepart, a jump (in j2k_write_sod) is done to the TLM marker to store the value of its length.
	 */
	OPJ_OFF_T m_tlm_start;
	/**
	 * Stores the sizes of the tlm.
	 */
	uint8 * m_tlm_sot_offsets_buffer;
	/**
	 * The current offset of the tlm buffer.
	 */
	uint8 * m_tlm_sot_offsets_current;
	/** Total num of tile parts in whole image = num tiles* num tileparts in each tile used in TLMmarker*/
	uint32_t m_total_tile_parts; /* totnum_tp */
	/* encoded data for a tile */
	uint8 * m_encoded_tile_data;
	/* size of the encoded_data */
	uint32_t m_encoded_tile_size;
	/* encoded data for a tile */
	uint8 * m_header_tile_data;
	/* size of the encoded_data */
	uint32_t m_header_tile_data_size;
	/* whether to generate PLT markers */
	boolint m_PLT;
	/* reserved bytes in m_encoded_tile_size for PLT markers */
	uint32_t m_reserved_bytes_for_PLT;
	/** Number of components */
	uint32_t m_nb_comps;
} opj_j2k_enc_t;

struct opj_tcd;

/**
   JPEG-2000 codestream reader/writer
 */
typedef struct opj_j2k {
	boolint m_is_decoder; /* J2K codestream is decoded*/
	/* FIXME DOC*/
	union {
		opj_j2k_dec_t m_decoder;
		opj_j2k_enc_t m_encoder;
	} m_specific_param;
	opj_image_t* m_private_image; /** pointer to the internal/private encoded / decoded image */
	opj_image_t* m_output_image; /* pointer to the output image (decoded)*/
	opj_cp_t m_cp; /** Coding parameters */
	opj_procedure_list_t *  m_procedure_list; /** the list of procedures to exec **/
	opj_procedure_list_t *  m_validation_list; /** the list of validation procedures to follow to make sure the code is valid **/
	opj_codestream_index_t * cstr_index; /** helper used to write the index file */
	uint32_t m_current_tile_number; /** number of the tile currently concern by coding/decoding */
	struct opj_tcd *    m_tcd; /** the current tile coder/decoder **/
	opj_thread_pool_t* m_tp; /** Thread pool */
	uint32_t ihdr_w; /** Image width coming from JP2 IHDR box. 0 from a pure codestream */
	uint32_t ihdr_h; /** Image height coming from JP2 IHDR box. 0 from a pure codestream */
	unsigned int dump_state; /** Set to 1 by the decoder initialization if OPJ_DPARAMETERS_DUMP_FLAG is set */
} opj_j2k_t;

/** @name Exported functions */
/*@{*/
/**
   Setup the decoder decoding parameters using user parameters.
   Decoding parameters are returned in j2k->cp.
   @param j2k J2K decompressor handle
   @param parameters decompression parameters
 */
void opj_j2k_setup_decoder(opj_j2k_t * j2k, opj_dparameters_t * parameters);
void opj_j2k_decoder_set_strict_mode(opj_j2k_t * j2k, boolint strict);
boolint opj_j2k_set_threads(opj_j2k_t * j2k, uint32_t num_threads);
/**
 * Creates a J2K compression structure
 *
 * @return Returns a handle to a J2K compressor if successful, returns NULL otherwise
 */
opj_j2k_t* opj_j2k_create_compress(void);
boolint opj_j2k_setup_encoder(opj_j2k_t * p_j2k, opj_cparameters_t * parameters, opj_image_t * image, opj_event_mgr_t * p_manager);
/**
   Converts an enum type progression order to string type
 */
const char * opj_j2k_convert_progression_order(OPJ_PROG_ORDER prg_order);
/*@}*/

/*@}*/

/**
 * Ends the decompression procedures and possibiliy add data to be read after the
 * codestream.
 */
boolint opj_j2k_end_decompress(opj_j2k_t * j2k, opj_stream_private_t * p_stream, opj_event_mgr_t * p_manager);
/**
 * Reads a jpeg2000 codestream header structure.
 *
 * @param p_stream the stream to read data from.
 * @param p_j2k the jpeg2000 codec.
 * @param p_image FIXME DOC
 * @param p_manager the user event manager.
 *
 * @return true if the box is valid.
 */
boolint opj_j2k_read_header(opj_stream_private_t * p_stream, opj_j2k_t* p_j2k, opj_image_t** p_image, opj_event_mgr_t* p_manager);
/**
 * Destroys a jpeg2000 codec.
 *
 * @param   p_j2k   the jpeg20000 structure to destroy.
 */
void opj_j2k_destroy(opj_j2k_t * p_j2k);
/**
 * Destroys a codestream index structure.
 *
 * @param   p_cstr_ind  the codestream index parameter to destroy.
 */
void j2k_destroy_cstr_index(opj_codestream_index_t * p_cstr_ind);
/**
 * Decode tile data.
 * @param   p_j2k       the jpeg2000 codec.
 * @param   p_tile_index
 * @param p_data       FIXME DOC
 * @param p_data_size  FIXME DOC
 * @param   p_stream            the stream to write data to.
 * @param   p_manager   the user event manager.
 */
boolint opj_j2k_decode_tile(opj_j2k_t * p_j2k, uint32_t p_tile_index, uint8 * p_data, uint32_t p_data_size, opj_stream_private_t * p_stream, opj_event_mgr_t * p_manager);
/**
 * Reads a tile header.
 * @param   p_j2k       the jpeg2000 codec.
 * @param   p_tile_index FIXME DOC
 * @param   p_data_size FIXME DOC
 * @param   p_tile_x0 FIXME DOC
 * @param   p_tile_y0 FIXME DOC
 * @param   p_tile_x1 FIXME DOC
 * @param   p_tile_y1 FIXME DOC
 * @param   p_nb_comps FIXME DOC
 * @param   p_go_on FIXME DOC
 * @param   p_stream            the stream to write data to.
 * @param   p_manager   the user event manager.
 */
boolint opj_j2k_read_tile_header(opj_j2k_t * p_j2k,
    uint32_t * p_tile_index,
    uint32_t * p_data_size,
    int32_t * p_tile_x0,
    int32_t * p_tile_y0,
    int32_t * p_tile_x1,
    int32_t * p_tile_y1,
    uint32_t * p_nb_comps,
    boolint * p_go_on,
    opj_stream_private_t * p_stream,
    opj_event_mgr_t * p_manager);

/** Sets the indices of the components to decode.
 *
 * @param p_j2k         the jpeg2000 codec.
 * @param numcomps      Number of components to decode.
 * @param comps_indices Array of num_compts indices (numbering starting at 0)
 *                      corresponding to the components to decode.
 * @param p_manager     Event manager
 *
 * @return TRUE in case of success.
 */
boolint opj_j2k_set_decoded_components(opj_j2k_t * p_j2k, uint32_t numcomps, const uint32_t* comps_indices, opj_event_mgr_t * p_manager);
/**
 * Sets the given area to be decoded. This function should be called right after opj_read_header and before any tile
 *header reading.
 *
 * @param   p_j2k           the jpeg2000 codec.
 * @param   p_image     FIXME DOC
 * @param   p_start_x       the left position of the rectangle to decode (in image coordinates).
 * @param   p_start_y       the up position of the rectangle to decode (in image coordinates).
 * @param   p_end_x         the right position of the rectangle to decode (in image coordinates).
 * @param   p_end_y         the bottom position of the rectangle to decode (in image coordinates).
 * @param   p_manager       the user event manager
 *
 * @return  true            if the area could be set.
 */
boolint opj_j2k_set_decode_area(opj_j2k_t * p_j2k, opj_image_t* p_image, int32_t p_start_x, int32_t p_start_y, int32_t p_end_x, int32_t p_end_y, opj_event_mgr_t * p_manager);
/**
 * Creates a J2K decompression structure.
 *
 * @return a handle to a J2K decompressor if successful, NULL otherwise.
 */
opj_j2k_t* opj_j2k_create_decompress(void);
/**
 * Dump some elements from the J2K decompression structure .
 *
 *@param p_j2k              the jpeg2000 codec.
 *@param flag               flag to describe what elements are dump.
 *@param out_stream         output stream where dump the elements.
 *
 */
void j2k_dump(opj_j2k_t* p_j2k, int32_t flag, FILE* out_stream);
/**
 * Dump an image header structure.
 *
 *@param image          the image header to dump.
 *@param dev_dump_flag      flag to describe if we are in the case of this function is use outside j2k_dump function
 *@param out_stream         output stream where dump the elements.
 */
void j2k_dump_image_header(opj_image_t* image, boolint dev_dump_flag, FILE* out_stream);
/**
 * Dump a component image header structure.
 *
 *@param comp       the component image header to dump.
 *@param dev_dump_flag      flag to describe if we are in the case of this function is use outside j2k_dump function
 *@param out_stream         output stream where dump the elements.
 */
void j2k_dump_image_comp_header(opj_image_comp_t* comp, boolint dev_dump_flag, FILE* out_stream);
/**
 * Get the codestream info from a JPEG2000 codec.
 *
 *@param    p_j2k               the component image header to dump.
 *
 *@return   the codestream information extract from the jpg2000 codec
 */
opj_codestream_info_v2_t* j2k_get_cstr_info(opj_j2k_t* p_j2k);
/**
 * Get the codestream index from a JPEG2000 codec.
 *
 *@param    p_j2k               the component image header to dump.
 *
 *@return   the codestream index extract from the jpg2000 codec
 */
opj_codestream_index_t* j2k_get_cstr_index(opj_j2k_t* p_j2k);

/**
 * Decode an image from a JPEG-2000 codestream
 * @param j2k J2K decompressor handle
 * @param p_stream  FIXME DOC
 * @param p_image   FIXME DOC
 * @param p_manager FIXME DOC
 * @return FIXME DOC
 */
boolint opj_j2k_decode(opj_j2k_t * j2k, opj_stream_private_t * p_stream, opj_image_t * p_image, opj_event_mgr_t * p_manager);
boolint opj_j2k_get_tile(opj_j2k_t * p_j2k, opj_stream_private_t * p_stream, opj_image_t* p_image, opj_event_mgr_t * p_manager, uint32_t tile_index);
boolint opj_j2k_set_decoded_resolution_factor(opj_j2k_t * p_j2k, uint32_t res_factor, opj_event_mgr_t * p_manager);
/**
 * Specify extra options for the encoder.
 *
 * @param  p_j2k        the jpeg2000 codec.
 * @param  p_options    options
 * @param  p_manager    the user event manager
 *
 * @see opj_encoder_set_extra_options() for more details.
 */
boolint opj_j2k_encoder_set_extra_options(opj_j2k_t * p_j2k, const char* const* p_options, opj_event_mgr_t * p_manager);
/**
 * Writes a tile.
 * @param   p_j2k       the jpeg2000 codec.
 * @param p_tile_index FIXME DOC
 * @param p_data FIXME DOC
 * @param p_data_size FIXME DOC
 * @param   p_stream            the stream to write data to.
 * @param   p_manager   the user event manager.
 */
boolint opj_j2k_write_tile(opj_j2k_t * p_j2k, uint32_t p_tile_index, uint8 * p_data, uint32_t p_data_size, opj_stream_private_t * p_stream, opj_event_mgr_t * p_manager);
/**
 * Encodes an image into a JPEG-2000 codestream
 */
boolint opj_j2k_encode(opj_j2k_t * p_j2k, opj_stream_private_t * cio, opj_event_mgr_t * p_manager);
/**
 * Starts a compression scheme, i.e. validates the codec parameters, writes the header.
 *
 * @param   p_j2k       the jpeg2000 codec.
 * @param   p_stream            the stream object.
 * @param   p_image FIXME DOC
 * @param   p_manager   the user event manager.
 *
 * @return true if the codec is valid.
 */
boolint opj_j2k_start_compress(opj_j2k_t * p_j2k, opj_stream_private_t * p_stream, opj_image_t * p_image, opj_event_mgr_t * p_manager);
/**
 * Ends the compression procedures and possibiliy add data to be read after the
 * codestream.
 */
boolint opj_j2k_end_compress(opj_j2k_t * p_j2k, opj_stream_private_t * cio, opj_event_mgr_t * p_manager);
boolint opj_j2k_setup_mct_encoding(opj_tcp_t * p_tcp, opj_image_t * p_image);

#endif /* OPJ_J2K_H */
