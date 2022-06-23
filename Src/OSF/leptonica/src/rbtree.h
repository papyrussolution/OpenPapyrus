/*====================================================================*
   -  Copyright (C) 2001 Leptonica.  All rights reserved.
   -
   -  Redistribution and use in source and binary forms, with or without
   -  modification, are permitted provided that the following conditions
   -  are met:
   -  1. Redistributions of source code must retain the above copyright
   -     notice, this list of conditions and the following disclaimer.
   -  2. Redistributions in binary form must reproduce the above
   -     copyright notice, this list of conditions and the following
   -     disclaimer in the documentation and/or other materials
   -     provided with the distribution.
*====================================================================*/

/*
 * Modified from the excellent code here:
 *     http://en.literateprograms.org/Red-black_tree_(C)?oldid=19567
 * which has been placed in the public domain under the Creative Commons
 * CC0 1.0 waiver (http://creativecommons.org/publicdomain/zero/1.0/).
 *
 * When the key is generated from a hash (e.g., string --> uint64),
 * there is always the possibility of having collisions, but to make
 * the collision probability very low requires using a large hash.
 * For that reason, the key types are 64 bit quantities, which will result
 * in a negligible probabililty of collisions for millions of hashed values.
 * Using 8 byte keys instead of 4 byte keys requires a little more
 * storage, but the simplification in being able to ignore collisions
 * with the red-black trees for most applications is worth it.
 */

#ifndef  LEPTONICA_RBTREE_H
#define  LEPTONICA_RBTREE_H

/*! The three valid key types for red-black trees, maps and sets. */
/*! RBTree Key Type */
enum {
	L_INT_TYPE = 1,
	L_UINT_TYPE = 2,
	L_FLOAT_TYPE = 3
};

/*!
 * Storage for keys and values for red-black trees, maps and sets.
 * <pre>
 * Note:
 *   (1) Keys and values of the valid key types are all 64-bit
 *   (2) (void *) can be used for values but not for keys.
 * </pre>
 */
union Rb_Type {
	l_int64 itype;
	l_uint64 utype;
	double ftype;
	void      * ptype;
};

typedef union Rb_Type RB_TYPE;

struct L_Rbtree {
	struct L_Rbtree_Node  * root;
	int32 keytype;
};

typedef struct L_Rbtree L_RBTREE;
typedef struct L_Rbtree L_AMAP;  /* hide underlying implementation for map */
typedef struct L_Rbtree L_ASET;  /* hide underlying implementation for set */

struct L_Rbtree_Node {
	union Rb_Type key;
	union Rb_Type value;
	struct L_Rbtree_Node  * left;
	struct L_Rbtree_Node  * right;
	struct L_Rbtree_Node  * parent;
	int32 color;
};

typedef struct L_Rbtree_Node L_RBTREE_NODE;
typedef struct L_Rbtree_Node L_AMAP_NODE;  /* hide tree implementation */
typedef struct L_Rbtree_Node L_ASET_NODE;  /* hide tree implementation */

#endif  /* LEPTONICA_RBTREE_H */
