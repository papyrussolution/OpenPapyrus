/* $Header: /cvs/maptools/cvsroot/libtiff/libtiff/tif_extension.c,v 1.8 2015-12-06 11:13:43 erouault Exp $ */
/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
/*
 * TIFF Library.
 *
 * Various routines support external extension of the tag set, and other
 * application extension capabilities.
 */
#include "tiffiop.h"
#pragma hdrstop

int TIFFGetTagListCount(TIFF * tif)
{
	TIFFDirectory* td = &tif->tif_dir;
	return td->td_customValueCount;
}

uint32 TIFFGetTagListEntry(TIFF * tif, int tag_index)
{
	TIFFDirectory* td = &tif->tif_dir;
	if(tag_index < 0 || tag_index >= td->td_customValueCount)
		return (uint32)(-1);
	else
		return td->td_customValues[tag_index].info->field_tag;
}
/*
** This provides read/write access to the TIFFTagMethods within the TIFF
** structure to application code without giving access to the private
** TIFF structure.
*/
TIFFTagMethods * TIFFAccessTagMethods(TIFF * tif)
{
	return &(tif->tif_tagmethods);
}

void * TIFFGetClientInfo(TIFF * tif, const char * name)
{
	TIFFClientInfoLink * psLink = tif->tif_clientinfo;
	while(psLink && !sstreq(psLink->name, name))
		psLink = psLink->next;
	return psLink ? psLink->data : NULL;
}

void TIFFSetClientInfo(TIFF * tif, void * data, const char * name)
{
	TIFFClientInfoLink * psLink = tif->tif_clientinfo;
	// Do we have an existing link with this name?  If so, just set it.
	while(psLink && !sstreq(psLink->name, name))
		psLink = psLink->next;
	if(psLink) {
		psLink->data = data;
	}
	else {
		/*
		** Create a new link.
		*/
		psLink = (TIFFClientInfoLink*)SAlloc::M(sizeof(TIFFClientInfoLink));
		assert(psLink != NULL);
		psLink->next = tif->tif_clientinfo;
		psLink->name = (char *)SAlloc::M((tmsize_t)(strlen(name)+1));
		assert(psLink->name != NULL);
		strcpy(psLink->name, name);
		psLink->data = data;
		tif->tif_clientinfo = psLink;
	}
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
