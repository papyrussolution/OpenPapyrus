/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if you have the <openjpeg-2.0/openjpeg.h> header file. */
/* #undef HAVE_OPENJPEG_2_0_OPENJPEG_H */

/* Define to 1 if you have the <openjpeg-2.1/openjpeg.h> header file. */
/* #undef HAVE_OPENJPEG_2_1_OPENJPEG_H */

/* Define to 1 if you have the <openjpeg-2.2/openjpeg.h> header file. */
/* #undef HAVE_OPENJPEG_2_2_OPENJPEG_H */

/* Define to 1 if you have the `fmemopen' function. */
/* #undef HAVE_FMEMOPEN */


/* Define to 1 if you have giflib. */
/* #undef HAVE_LIBGIF */

/* Define to 1 if you have libopenjp2. */
/* #undef HAVE_LIBJP2K */

/* Define to 1 if you have jpeg. */
/* #undef HAVE_LIBJPEG */

/* Define to 1 if you have libpng. */
/* #undef HAVE_LIBPNG */

/* Define to 1 if you have libtiff. */
/* #undef HAVE_LIBTIFF */

/* Define to 1 if you have libwebp. */
/* #undef HAVE_LIBWEBP */

/* Define to 1 if you have zlib. */
/* #undef HAVE_LIBZ */

#ifdef HAVE_OPENJPEG_2_0_OPENJPEG_H
#define LIBJP2K_HEADER <openjpeg-2.0/openjpeg.h>
#endif

#ifdef HAVE_OPENJPEG_2_1_OPENJPEG_H
#define LIBJP2K_HEADER <openjpeg-2.1/openjpeg.h>
#endif

#ifdef HAVE_OPENJPEG_2_2_OPENJPEG_H
#define LIBJP2K_HEADER <openjpeg-2.2/openjpeg.h>
#endif
