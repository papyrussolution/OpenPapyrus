#ifndef BLOBHDR_H_DEFINED
/*************************************************************************
**
**  Copyright 1982-2001 Pervasive Software Inc. All Rights Reserved
**
*************************************************************************/
/***************************************************************************
   BLOBHDR.H - Btrieve Constants for Chunk Operations
      This file contains various Btrieve constants for use by Btrieve
      C/C++ applications.

***************************************************************************/

/*
**
** Extractors are used for both Get and Put operations.  Every Get Extractor
** has a parallel Put Extractor; the only Put Extractor without a Get
** counterpart is Truncate.
**
** There are two ways to classify all Extractors except Truncate:
**
**   1) Do they define chunks by Offset and Len, or do they instruct
**      Btrieve to define chunks based on the notion of a rectangle?
**
**   2) Do they read/write user's data directly from/to the data buffer,
**      or do they use pointers in the data buffer as source/destination?
**
*/

//#ifndef BTI_ULONG
//#define BTI_ULONG  unsigned long
//#endif

#ifndef RP
#define RP BTI_ULONG
#endif

//#ifndef BTI_BYTE
//#define BTI_BYTE unsigned char
//#endif


/* "Actions" or opcodes for ProcessIndirect() */
#define PREPROCESS_BLOBGET   0
#define PREPROCESS_BLOBPUT   1
#define POSTPROCESS_BLOBGET  2
//#define POSTPROCESS_BLOBPUT  "no postprocess for blob put"


typedef struct
{
    BTI_ULONG ChunkOffset;
    BTI_ULONG ChunkLen;
    BTI_BUFFER_PTR dataP;       /* When used w/ "direct" api, init to NULL. */
} CHUNK;

typedef struct
{
    BTI_ULONG      Signature;
    BTI_ULONG      NumChunks;
    CHUNK          Chunk[1];
} XTRACTR;

typedef struct
{
   BTI_ULONG Signature;
   BTI_ULONG NumRows;
   BTI_ULONG Offset;
   BTI_ULONG BytesPerRow;
   BTI_ULONG BtrDistanceBetweenRows;    /* Btrieve's count of bytes between */
                                      /* beginning of two consecutive rows. */
   BTI_BUFFER_PTR dataP;
   BTI_ULONG AppDistanceBetweenRows;        /* App's count of bytes between */
                                    /* beginning of two consecutive rows.   */
} BRECTANGLE;

/*============================================================================

                           Chunk PUT APIs:

============================================================================*/

/*
**  1. Update a chunk at Offset, Len bytes.  New bytes are in data buffer,
**     following the entire extractor.  Initialize the dummy Source ptr to
**     NULL.
*/


//#define  XTRACTR_DIRECT_SIGN     0x80000000L


   typedef struct {
      BTI_ULONG          Signature;        /* XTRACTR_DIRECT_SIGN */
      BTI_ULONG          NumChunks;
      CHUNK          Chunk[1];
   }  PUT_XTRACTR;


/*
**  2. Update a chunk at Offset, Len bytes.  Get new bytes by using a ptr(s)
**     in the extractor.  Use PUT_XTRACTR, but initialize the Signature
**     field to XTRACTR_INDIRECT_SIGN.  Initialize the Source ptr(s) to
**     point to the chunks.
*/

//#define  XTRACTR_INDIRECT_SIGN     0x80000001L

/*
**  3. Update a rectangle.  A series of NumRows chunks, beginning at Offset,
**     bytesPerRow number of bytes per chunk.   Between rows (chunks),
**     increment previous Offset by BtrDistanceBetweenRows to position
**     into record for next chunk.  Data is sent in the data buffer.
**     Initialize the dummy Destination ptr to NULL.  The
**     AppDistanceBetweenRows field will be ignored.
*/

// #define  RECTANGLE_DIRECT_SIGN     0x80000002L

   typedef struct {
      BTI_ULONG Signature;                         /* RECTANGLE_DIRECT_SIGN */
      BTI_ULONG NumRows;
      BTI_ULONG Offset;
      BTI_ULONG BytesPerRow;
      BTI_ULONG BtrDistanceBetweenRows; /* Btrieve's count of bytes between */
                                      /* beginning of two consecutive rows. */
      BTI_BUFFER_PTR dataP;
      BTI_ULONG AppDistanceBetweenRows;     /* App's count of bytes between */
                                         /* beginning of two consecutive rows.     */
      } PUT_RECTANGLE;


/*
**  4. Update a rectangle - same as above except that the data is read from
**     an address specified by the Source ptr in the incoming data buffer,
**     following the rectangle definition.  Also, AppDistanceBetweenRows
**     is specified as a value to use to increment the Destination ptr
**     between rows (chunks).  Use the PUT_RECTANGLE struct, but init
**     the Signature field to RECTANGLE_INDIRECT_SIGN.
*/


//#define  RECTANGLE_INDIRECT_SIGN     0x80000003L

/*
**  5. Truncate at Offset.
*/

// #define TRUNC_SIGN 0x80000004L

   typedef struct {
      BTI_ULONG Signature;        /* TRUNC_SIGN */
      BTI_ULONG ChunkOffset;
   }  PUT_TRUNC;


/*
**  6. Put Next.  Combine with each option above, by biasing the signature
**     with NEXT_IN_BLOB.  In each case, the Offset chunk is irrelevant,
**     because it is computed by Btrieve, based on current position in
**     record.  For the first two put options, if more than one chunk is
**     used, each chunk's offset is computed by Btrieve.  For the "put a
**     rectangle" options, the first Offset is computed from the current
**     position in the record; computation of subsequent "row's"
**     offsets is based on BtrDistanceBetweenRows.
*/

//   #define NEXT_IN_BLOB 0x40000000L

/*
**  7. Append.  Combine with the first four options above, by biasing the
**     signature with APPEND_TO_BLOB.  Behaves like Put Next in that Btrieve
**     computes the same offset chunks as are computed with Put Next.  The
**     difference is that value used for the offset is one byte beyond the end
**     of record.  May not be used with the Put Next bias.
*/

//   #define APPEND_TO_BLOB 0x20000000L


/*============================================================================

                           Chunk Get APIs:

============================================================================*/

/*
**
** 1. Get a chunk(s) at Offset, Len bytes.  Data returned is placed into the
**    data buffer.  Remember to initialize Chunk[x].dataP to NULL
**    and set the Signature field to XTRACTR_DIRECT_SIGN.
*/


   typedef struct {
      RP             RecordAddress;
      BTI_ULONG      Signature;
      BTI_ULONG      NumChunks;
      CHUNK          Chunk[1];
   }  GET_XTRACTR;

/*
**
** 2. Get a chunk(s) at Offset, Len bytes.  Data returned is placed into a
**    buffer whose address is given by a pointer sent by the application.
**    Use the GET_XTRACTR.  Set the Signature field to XTRACTR_INDIRECT_SIGN.
*/


/*
** 3. Get a rectangle.  A series of NumRows chunks, beginning at Offset,
**    bytesPerRow number of bytes per chunk.   Between rows (chunks),
**    increment previous Offset by BtrDistanceBetweenRows to position
**    into record for next chunk.  Data is returned in the data buffer.
**    Initialize the dummy Destination ptr to NULL.  The
**    AppDistanceBetweenRows field will be ignored.  Set the Signature
**    field to RECTANGLE_DIRECT_SIGN.
*/

   typedef struct {
      RP    RecordAddress;
      BTI_ULONG Signature;
      BTI_ULONG NumRows;
      BTI_ULONG Offset;
      BTI_ULONG BytesPerRow;
      BTI_ULONG BtrDistanceBetweenRows; /* Btrieve's count of bytes between */
                                      /* beginning of two consecutive rows. */
      BTI_BUFFER_PTR dataP;
      BTI_ULONG AppDistanceBetweenRows;     /* App's count of bytes between */
                                      /* beginning of two consecutive rows. */
      } GET_RECTANGLE;

/*
**
** 4. Get a rectangle - same as above except that the data is returned to an
**        address specified by the Destination pointer in the incoming data
**        buffer, following the rectangle definition.  Also,
**        AppDistanceBetweenRows is specified as a value to use to increment
**        the Destination pointer between rows.  Set the Signature field to
**        RECTANGLE_INDIRECT_SIGN.
*/

/*
**
** 5. Get Next Chunk.  Combine with each option above, by biasing the
**    signature with NEXT_IN_BLOB.  In each case, the Offset chunk is
**    irrelevant.  See details under Put Next Chunk, above.
**
*/


//#define CHUNK_NOBIAS_MASK ~(NEXT_IN_BLOB | APPEND_TO_BLOB | PARTS_OF_KEY)
#define INDIRECT_BIT       0x00000001L
#define RECTANGLE_BIT      0x00000002L
#define TRUNC_BIT          0x00000004L

#define BLOBHDR_H_DEFINED
#endif
