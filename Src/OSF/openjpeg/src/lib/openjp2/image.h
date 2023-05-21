/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
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
#ifndef OPJ_IMAGE_H
#define OPJ_IMAGE_H
/**
   @file image.h
   @brief Implementation of operations on images (IMAGE)

   The functions in IMAGE.C have for goal to realize operations on images.
 */
struct opj_image;
struct opj_cp;

/** @defgroup IMAGE IMAGE - Implementation of operations on images */
/*@{*/

/**
 * Create an empty image
 *
 * @return returns an empty image if successful, returns NULL otherwise
 */
opj_image_t* opj_image_create0();
/**
 * Updates the components characteristics of the image from the coding parameters.
 *
 * @param p_image_header        the image header to update.
 * @param p_cp                  the coding parameters from which to update the image.
 */
void opj_image_comp_header_update(opj_image_t * p_image, const struct opj_cp* p_cp);
void opj_copy_image_header(const opj_image_t* p_image_src, opj_image_t* p_image_dest);

/*@}*/

#endif /* OPJ_IMAGE_H */
