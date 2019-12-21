/*
 * buf.c: memory buffers for libxml2
 *
 * new buffer structures and entry points to simplify the maintainance
 * of libxml2 and ensure we keep good control over memory allocations
 * and stay 64 bits clean.
 * The new entry point use the xmlBufPtr opaque structure and
 * xmlBuf...() counterparts to the old xmlBuf...() functions
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

#define WITH_BUFFER_COMPAT
/**
 * xmlBuf:
 *
 * A buffer structure. The base of the structure is somehow compatible
 * with struct _xmlBuffer to limit risks on application which accessed
 * directly the input->buf->buffer structures.
 */
struct xmlBuf {
	xmlChar * content;      /* The buffer content UTF8 */
	uint compat_use; /* for binary compatibility */
	uint compat_size; /* for binary compatibility */
	xmlBufferAllocationScheme alloc; /* The realloc method */
	xmlChar * contentIO;    /* in IO mode we may have a different base */
	size_t use;             /* The buffer size used */
	size_t size;            /* The buffer size */
	xmlBuffer * buffer;    /* wrapper for an old buffer */
	int error;              /* an error code if a failure occured */
};

#ifdef WITH_BUFFER_COMPAT
/*
 * Macro for compatibility with xmlBuffer to be used after an xmlBuf
 * is updated. This makes sure the compat fields are updated too.
 */
#define UPDATE_COMPAT(buf)				    \
	if(buf->size < INT_MAX) buf->compat_size = buf->size; \
	else buf->compat_size = INT_MAX;		       \
	if(buf->use < INT_MAX) buf->compat_use = buf->use; \
	else buf->compat_use = INT_MAX;

/*
 * Macro for compatibility with xmlBuffer to be used in all the xmlBuf
 * entry points, it checks that the compat fields have not been modified
 * by direct call to xmlBuffer function from code compiled before 2.9.0 .
 */
#define CHECK_COMPAT(buf)				    \
	if(buf->size != (size_t)buf->compat_size)	     \
		if(buf->compat_size < INT_MAX)			  \
			buf->size = buf->compat_size;		       \
	if(buf->use != (size_t)buf->compat_use)		     \
		if(buf->compat_use < INT_MAX)			  \
			buf->use = buf->compat_use;

#else /* ! WITH_BUFFER_COMPAT */
#define UPDATE_COMPAT(buf)
#define CHECK_COMPAT(buf)
#endif /* WITH_BUFFER_COMPAT */

/**
 * xmlBufMemoryError:
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 * To be improved...
 */
static void FASTCALL xmlBufMemoryError(xmlBuf * buf, const char * extra)
{
	__xmlSimpleError(XML_FROM_BUFFER, XML_ERR_NO_MEMORY, NULL, NULL, extra);
	if(buf && buf->error == 0)
		buf->error = XML_ERR_NO_MEMORY;
}
/**
 * xmlBufOverflowError:
 * @extra:  extra informations
 *
 * Handle a buffer overflow error
 * To be improved...
 */
static void FASTCALL xmlBufOverflowError(xmlBuf * buf, const char * extra)
{
	__xmlSimpleError(XML_FROM_BUFFER, XML_BUF_OVERFLOW, NULL, NULL, extra);
	if(buf && buf->error == 0)
		buf->error = XML_BUF_OVERFLOW;
}
/**
 * xmlBufCreate:
 *
 * routine to create an XML buffer.
 * returns the new structure.
 */
xmlBuf * xmlBufCreate()
{
	xmlBuf * p_ret = (xmlBuf *)SAlloc::M(sizeof(xmlBuf));
	if(!p_ret) {
		xmlBufMemoryError(NULL, "creating buffer");
	}
	else {
		p_ret->compat_use = 0;
		p_ret->use = 0;
		p_ret->error = 0;
		p_ret->buffer = NULL;
		p_ret->size = xmlDefaultBufferSize;
		p_ret->compat_size = xmlDefaultBufferSize;
		p_ret->alloc = xmlBufferAllocScheme;
		p_ret->content = static_cast<xmlChar *>(SAlloc::M(p_ret->size * sizeof(xmlChar)));
		if(p_ret->content == NULL) {
			xmlBufMemoryError(p_ret, "creating buffer");
			ZFREE(p_ret);
		}
		else {
			p_ret->content[0] = 0;
			p_ret->contentIO = NULL;
		}
	}
	return p_ret;
}
/**
 * xmlBufCreateSize:
 * @size: initial size of buffer
 *
 * routine to create an XML buffer.
 * returns the new structure.
 */
xmlBuf * FASTCALL xmlBufCreateSize(size_t size)
{
	xmlBuf * ret = (xmlBuf *)SAlloc::M(sizeof(xmlBuf));
	if(!ret) {
		xmlBufMemoryError(NULL, "creating buffer");
	}
	else {
		ret->compat_use = 0;
		ret->use = 0;
		ret->error = 0;
		ret->buffer = NULL;
		ret->alloc = xmlBufferAllocScheme;
		ret->size = (size ? size+2 : 0);     /* +1 for ending null */
		ret->compat_size = (int)ret->size;
		if(ret->size) {
			ret->content = static_cast<xmlChar *>(SAlloc::M(ret->size * sizeof(xmlChar)));
			if(ret->content == NULL) {
				xmlBufMemoryError(ret, "creating buffer");
				SAlloc::F(ret);
				return NULL;
			}
			ret->content[0] = 0;
		}
		else
			ret->content = NULL;
		ret->contentIO = NULL;
	}
	return ret;
}
/**
 * xmlBufDetach:
 * @buf:  the buffer
 *
 * Remove the string contained in a buffer and give it back to the
 * caller. The buffer is reset to an empty content.
 * This doesn't work with immutable buffers as they can't be reset.
 *
 * Returns the previous string contained by the buffer.
 */
xmlChar * FASTCALL xmlBufDetach(xmlBuf * pBuf)
{
	xmlChar * p_ret = 0;
	if(pBuf && pBuf->alloc != XML_BUFFER_ALLOC_IMMUTABLE && pBuf->buffer && !pBuf->error) {
        p_ret = pBuf->content;
        pBuf->content = NULL;
        pBuf->size = 0;
        pBuf->use = 0;
        pBuf->compat_use = 0;
        pBuf->compat_size = 0;
    }
	return p_ret;
}
/**
 * xmlBufCreateStatic:
 * @mem: the memory area
 * @size:  the size in byte
 *
 * routine to create an XML buffer from an immutable memory area.
 * The area won't be modified nor copied, and is expected to be
 * present until the end of the buffer lifetime.
 *
 * returns the new structure.
 */
xmlBuf * FASTCALL xmlBufCreateStatic(void * pMem, size_t size)
{
	xmlBuf * p_ret = 0;
	if(pMem && size) {
		p_ret = (xmlBuf *)SAlloc::M(sizeof(xmlBuf));
		if(!p_ret)
			xmlBufMemoryError(NULL, "creating buffer");
		else {
			if(size < INT_MAX) {
				p_ret->compat_use = size;
				p_ret->compat_size = size;
			}
			else {
				p_ret->compat_use = INT_MAX;
				p_ret->compat_size = INT_MAX;
			}
			p_ret->use = size;
			p_ret->size = size;
			p_ret->alloc = XML_BUFFER_ALLOC_IMMUTABLE;
			p_ret->content = (xmlChar *)pMem;
			p_ret->error = 0;
			p_ret->buffer = NULL;
		}
	}
	return p_ret;
}
/**
 * xmlBufGetAllocationScheme:
 * @buf:  the buffer
 *
 * Get the buffer allocation scheme
 *
 * Returns the scheme or -1 in case of error
 */
int FASTCALL xmlBufGetAllocationScheme(xmlBuf * buf)
{
	if(!buf) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufGetAllocationScheme: buf == NULL\n");
#endif
		return -1;
	}
	else
		return buf->alloc;
}
/**
 * xmlBufSetAllocationScheme:
 * @buf:  the buffer to tune
 * @scheme:  allocation scheme to use
 *
 * Sets the allocation scheme for this buffer
 *
 * returns 0 in case of success and -1 in case of failure
 */
int FASTCALL xmlBufSetAllocationScheme(xmlBuf * buf, xmlBufferAllocationScheme scheme)
{
	if(!buf || buf->error) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufSetAllocationScheme: buf == NULL or in error\n");
#endif
		return -1;
	}
	else if(oneof2(buf->alloc, XML_BUFFER_ALLOC_IMMUTABLE, XML_BUFFER_ALLOC_IO))
		return -1;
	else if(oneof4(scheme, XML_BUFFER_ALLOC_DOUBLEIT, XML_BUFFER_ALLOC_EXACT, XML_BUFFER_ALLOC_HYBRID, XML_BUFFER_ALLOC_IMMUTABLE)) {
		buf->alloc = scheme;
		if(buf->buffer)
			buf->buffer->alloc = scheme;
		return 0;
	}
	else {
		// 
		// Switching a buffer ALLOC_IO has the side effect of initializing
		// the contentIO field with the current content
		// 
		if(scheme == XML_BUFFER_ALLOC_IO) {
			buf->alloc = XML_BUFFER_ALLOC_IO;
			buf->contentIO = buf->content;
		}
		return -1;
	}
}
/**
 * xmlBufFree:
 * @buf:  the buffer to free
 *
 * Frees an XML buffer. It frees both the content and the structure which
 * encapsulate it.
 */
void FASTCALL xmlBufFree(xmlBuf * pBuf)
{
	if(!pBuf) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufFree: buf == NULL\n");
#endif
		return;
	}
	if(pBuf->alloc == XML_BUFFER_ALLOC_IO && pBuf->contentIO != NULL) {
		SAlloc::F(pBuf->contentIO);
	}
	else if(pBuf->content && (pBuf->alloc != XML_BUFFER_ALLOC_IMMUTABLE)) {
		SAlloc::F(pBuf->content);
	}
	SAlloc::F(pBuf);
}
/**
 * xmlBufEmpty:
 * @buf:  the buffer
 *
 * empty a buffer.
 */
void FASTCALL xmlBufEmpty(xmlBuf * pBuf)
{
	if(pBuf && !pBuf->error && pBuf->content) {
		CHECK_COMPAT(pBuf)
		pBuf->use = 0;
		if(pBuf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) {
			pBuf->content = const_cast<xmlChar *>(reinterpret_cast<const xmlChar *>("")); // @badcast
		}
		else if((pBuf->alloc == XML_BUFFER_ALLOC_IO) && pBuf->contentIO) {
			const size_t start_buf = pBuf->content - pBuf->contentIO;
			pBuf->size += start_buf;
			pBuf->content = pBuf->contentIO;
			pBuf->content[0] = 0;
		}
		else {
			pBuf->content[0] = 0;
		}
		UPDATE_COMPAT(pBuf)
	}
}
/**
 * xmlBufShrink:
 * @buf:  the buffer to dump
 * @len:  the number of xmlChar to remove
 *
 * Remove the beginning of an XML buffer.
 * NOTE that this routine behaviour differs from xmlBufferShrink()
 * as it will return 0 on error instead of -1 due to size_t being
 * used as the return type.
 *
 * Returns the number of byte removed or 0 in case of failure
 */
size_t FASTCALL xmlBufShrink(xmlBuf * buf, size_t len)
{
	if(!buf || buf->error) 
		return 0;
	CHECK_COMPAT(buf)
	if(len == 0) 
		return 0;
	if(len > buf->use) 
		return 0;
	buf->use -= len;
	if((buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) || ((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO)) {
		/*
		 * we just move the content pointer, but also make sure
		 * the perceived buffer size has shrinked accordingly
		 */
		buf->content += len;
		buf->size -= len;
		//
		// sometimes though it maybe be better to really shrink on IO buffers
		//
		if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
			const size_t start_buf = buf->content - buf->contentIO;
			if(start_buf >= buf->size) {
				memmove(buf->contentIO, &buf->content[0], buf->use);
				buf->content = buf->contentIO;
				buf->content[buf->use] = 0;
				buf->size += start_buf;
			}
		}
	}
	else {
		memmove(buf->content, &buf->content[len], buf->use);
		buf->content[buf->use] = 0;
	}
	UPDATE_COMPAT(buf)
	return len;
}
/**
 * xmlBufGrowInternal:
 * @buf:  the buffer
 * @len:  the minimum free size to allocate
 *
 * Grow the available space of an XML buffer, @len is the target value
 * Error checking should be done on buf->error since using the return
 * value doesn't work that well
 *
 * Returns 0 in case of error or the length made available otherwise
 */
static size_t FASTCALL xmlBufGrowInternal(xmlBuf * pBuf, size_t len)
{
	size_t size;
	xmlChar * p_newbuf;
	if(!pBuf || pBuf->error)
        return 0;
	CHECK_COMPAT(pBuf)
	if(pBuf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
        return 0;
	if(pBuf->use + len < pBuf->size)
		return (pBuf->size - pBuf->use);

	/*
	 * Windows has a BIG problem on realloc timing, so we try to double
	 * the buffer size (if that's enough) (bug 146697)
	 * Apparently BSD too, and it's probably best for linux too
	 * On an embedded system this may be something to change
	 */
#if 1
	size = (pBuf->size > (size_t)len) ? (pBuf->size * 2) : (pBuf->use + len + 100);
#else
	size = buf->use + len + 100;
#endif
	if(pBuf->alloc == XML_BUFFER_ALLOC_IO && pBuf->contentIO) {
		size_t start_buf = pBuf->content - pBuf->contentIO;
		p_newbuf = static_cast<xmlChar *>(SAlloc::R(pBuf->contentIO, start_buf + size));
		if(!p_newbuf) {
			xmlBufMemoryError(pBuf, "growing buffer");
			return 0;
		}
		pBuf->contentIO = p_newbuf;
		pBuf->content = p_newbuf + start_buf;
	}
	else {
		p_newbuf = static_cast<xmlChar *>(SAlloc::R(pBuf->content, size));
		if(!p_newbuf) {
			xmlBufMemoryError(pBuf, "growing buffer");
			return 0;
		}
		pBuf->content = p_newbuf;
	}
	pBuf->size = size;
	UPDATE_COMPAT(pBuf)
	return (pBuf->size - pBuf->use);
}
/**
 * xmlBufGrow:
 * @buf:  the buffer
 * @len:  the minimum free size to allocate
 *
 * Grow the available space of an XML buffer, @len is the target value
 * This is been kept compatible with xmlBufferGrow() as much as possible
 *
 * Returns -1 in case of error or the length made available otherwise
 */
int FASTCALL xmlBufGrow(xmlBuf * buf, int len)
{
	size_t ret;
	if(!buf || (len < 0))
        return -1;
	else if(len == 0)
		return 0;
	else {
		ret = xmlBufGrowInternal(buf, len);
		if(buf->error != 0)
			return -1;
		else 
			return static_cast<int>(ret);
	}
}
/**
 * xmlBufInflate:
 * @buf:  the buffer
 * @len:  the minimum extra free size to allocate
 *
 * Grow the available space of an XML buffer, adding at least @len bytes
 *
 * Returns 0 if successful or -1 in case of error
 */
int FASTCALL xmlBufInflate(xmlBuf * buf, size_t len)
{
	if(!buf)
        return -1;
	else {
		xmlBufGrowInternal(buf, len + buf->size);
		return buf->error ? -1 : 0;
	}
}
/**
 * xmlBufDump:
 * @file:  the file output
 * @buf:  the buffer to dump
 *
 * Dumps an XML buffer to  a FILE *.
 * Returns the number of #xmlChar written
 */
size_t xmlBufDump(FILE * file, xmlBufPtr buf)
{
	size_t ret;
	if(!buf || buf->error) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufDump: buf == NULL or in error\n");
#endif
		return 0;
	}
	if(buf->content == NULL) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufDump: buf->content == NULL\n");
#endif
		return 0;
	}
	CHECK_COMPAT(buf)
	SETIFZ(file, stdout);
	ret = fwrite(buf->content, sizeof(xmlChar), buf->use, file);
	return ret;
}
/**
 * xmlBufContent:
 * @buf:  the buffer
 *
 * Function to extract the content of a buffer
 *
 * Returns the internal content
 */

xmlChar * FASTCALL xmlBufContent(const xmlBuf * buf)
{
	return (buf && !buf->error) ? buf->content : 0;
}
/**
 * xmlBufEnd:
 * @buf:  the buffer
 *
 * Function to extract the end of the content of a buffer
 *
 * Returns the end of the internal content or NULL in case of error
 */
xmlChar * FASTCALL xmlBufEnd(xmlBuf * buf)
{
	if(!buf || buf->error)
		return NULL;
	else {
		CHECK_COMPAT(buf)
		return &buf->content[buf->use];
	}
}
/**
 * xmlBufAddLen:
 * @buf:  the buffer
 * @len:  the size which were added at the end
 *
 * Sometime data may be added at the end of the buffer without
 * using the xmlBuf APIs that is used to expand the used space
 * and set the zero terminating at the end of the buffer
 *
 * Returns -1 in case of error and 0 otherwise
 */
int FASTCALL xmlBufAddLen(xmlBuf * buf, size_t len)
{
	if(!buf || buf->error)
		return -1;
	else {
		CHECK_COMPAT(buf)
		if(len > (buf->size - buf->use))
			return -1;
		else {
			buf->use += len;
			UPDATE_COMPAT(buf)
			if(buf->size > buf->use)
				buf->content[buf->use] = 0;
			else
				return -1;
			return 0;
		}
	}
}
/**
 * xmlBufErase:
 * @buf:  the buffer
 * @len:  the size to erase at the end
 *
 * Sometime data need to be erased at the end of the buffer
 *
 * Returns -1 in case of error and 0 otherwise
 */
int FASTCALL xmlBufErase(xmlBuf * buf, size_t len)
{
	if(!buf || buf->error)
		return -1;
	else {
		CHECK_COMPAT(buf)
		if(len > buf->use)
			return -1;
		else {
			buf->use -= len;
			buf->content[buf->use] = 0;
			UPDATE_COMPAT(buf)
			return 0;
		}
	}
}
/**
 * xmlBufLength:
 * @buf:  the buffer
 *
 * Function to get the length of a buffer
 *
 * Returns the length of data in the internal content
 */
size_t FASTCALL xmlBufLength(xmlBuf * buf)
{
	if(!buf || buf->error)
		return 0;
	else {
		CHECK_COMPAT(buf)
		return buf->use;
	}
}
// 
// @buf:  the buffer
// 
// Function to get the length of a buffer
// Returns the length of data in the internal content
// 
size_t FASTCALL xmlBufUse(xmlBuf * buf)
{
	if(!buf || buf->error)
		return 0;
	else {
		CHECK_COMPAT(buf)
		return buf->use;
	}
}
/**
 * xmlBufAvail:
 * @buf:  the buffer
 *
 * Function to find how much free space is allocated but not
 * used in the buffer. It does not account for the terminating zero
 * usually needed
 *
 * Returns the amount or 0 if none or an error occured
 */
size_t FASTCALL xmlBufAvail(xmlBuf * buf)
{
	if(!buf || buf->error)
		return 0;
	else {
		CHECK_COMPAT(buf)
		return (buf->size - buf->use);
	}
}
/**
 * xmlBufIsEmpty:
 * @buf:  the buffer
 *
 * Tell if a buffer is empty
 *
 * Returns 0 if no, 1 if yes and -1 in case of error
 */
int FASTCALL xmlBufIsEmpty(xmlBuf * buf)
{
	if(!buf || buf->error)
		return -1;
	else {
		CHECK_COMPAT(buf)
		return (buf->use == 0);
	}
}
/**
 * xmlBufResize:
 * @buf:  the buffer to resize
 * @size:  the desired size
 *
 * Resize a buffer to accommodate minimum size of @size.
 *
 * Returns  0 in case of problems, 1 otherwise
 */
int FASTCALL xmlBufResize(xmlBuf * buf, size_t size)
{
	uint newSize;
	xmlChar* rebuf = NULL;
	size_t start_buf;
	if(!buf || buf->error)
		return 0;
	CHECK_COMPAT(buf)
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
        return 0;
	/* Don't resize if we don't have to */
	if(size < buf->size)
		return 1;
	/* figure out new size */
	switch(buf->alloc) {
		case XML_BUFFER_ALLOC_IO:
		case XML_BUFFER_ALLOC_DOUBLEIT:
		    /*take care of empty case*/
		    newSize = (buf->size ? buf->size*2 : size + 10);
		    while(size > newSize) {
			    if(newSize > UINT_MAX / 2) {
				    xmlBufMemoryError(buf, "growing buffer");
				    return 0;
			    }
			    newSize *= 2;
		    }
		    break;
		case XML_BUFFER_ALLOC_EXACT:
		    newSize = size+10;
		    break;
		case XML_BUFFER_ALLOC_HYBRID:
		    if(buf->use < BASE_BUFFER_SIZE)
			    newSize = size;
		    else {
			    newSize = buf->size * 2;
			    while(size > newSize) {
				    if(newSize > UINT_MAX / 2) {
					    xmlBufMemoryError(buf, "growing buffer");
					    return 0;
				    }
				    newSize *= 2;
			    }
		    }
		    break;

		default:
		    newSize = size+10;
		    break;
	}
	if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
		start_buf = buf->content - buf->contentIO;
		if(start_buf > newSize) {
			// move data back to start 
			memmove(buf->contentIO, buf->content, buf->use);
			buf->content = buf->contentIO;
			buf->content[buf->use] = 0;
			buf->size += start_buf;
		}
		else {
			rebuf = static_cast<xmlChar *>(SAlloc::R(buf->contentIO, start_buf + newSize));
			if(rebuf == NULL) {
				xmlBufMemoryError(buf, "growing buffer");
				return 0;
			}
			buf->contentIO = rebuf;
			buf->content = rebuf + start_buf;
		}
	}
	else {
		if(buf->content == NULL) {
			rebuf = static_cast<xmlChar *>(SAlloc::M(newSize));
		}
		else if(buf->size - buf->use < 100) {
			rebuf = static_cast<xmlChar *>(SAlloc::R(buf->content, newSize));
		}
		else {
			/*
			 * if we are reallocating a buffer far from being full, it's
			 * better to make a new allocation and copy only the used range
			 * and free the old one.
			 */
			rebuf = static_cast<xmlChar *>(SAlloc::M(newSize));
			if(rebuf) {
				memcpy(rebuf, buf->content, buf->use);
				SAlloc::F(buf->content);
				rebuf[buf->use] = 0;
			}
		}
		if(rebuf == NULL) {
			xmlBufMemoryError(buf, "growing buffer");
			return 0;
		}
		buf->content = rebuf;
	}
	buf->size = newSize;
	UPDATE_COMPAT(buf)
	return 1;
}
/**
 * xmlBufAdd:
 * @buf:  the buffer to dump
 * @str:  the #xmlChar string
 * @len:  the number of #xmlChar to add
 *
 * Add a string range to an XML buffer. if len == -1, the length of
 * str is recomputed.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufAdd(xmlBuf * buf, const xmlChar * str, int len)
{
	if(!str || !buf || buf->error)
		return -1;
	CHECK_COMPAT(buf)
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
		return -1;
	if(len < -1) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufAdd: len < 0\n");
#endif
		return -1;
	}
	if(len < 0)
		len = static_cast<int>(sstrlen(str));
	if(len > 0) {
		const uint need_size = buf->use + len + 2;
		if((need_size > buf->size) && !xmlBufResize(buf, need_size)) {
			xmlBufMemoryError(buf, "growing buffer");
			return XML_ERR_NO_MEMORY;
		}
		memmove(&buf->content[buf->use], str, len*sizeof(xmlChar));
		buf->use += len;
		buf->content[buf->use] = 0;
		UPDATE_COMPAT(buf)
	}
	return 0;
}
/**
 * xmlBufAddHead:
 * @buf:  the buffer
 * @str:  the #xmlChar string
 * @len:  the number of #xmlChar to add
 *
 * Add a string range to the beginning of an XML buffer.
 * if len == -1, the length of @str is recomputed.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlBufAddHead(xmlBufPtr buf, const xmlChar * str, int len)
{
	uint needSize;
	if(!buf || buf->error)
		return -1;
	CHECK_COMPAT(buf)
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
		return -1;
	if(!str) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufAddHead: str == NULL\n");
#endif
		return -1;
	}
	if(len < -1) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufAddHead: len < 0\n");
#endif
		return -1;
	}
	if(len == 0)
		return 0;
	if(len < 0)
		len = sstrlen(str);
	if(len <= 0)
		return -1;
	if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
		size_t start_buf = buf->content - buf->contentIO;
		if(start_buf > (uint)len) {
			// 
			// We can add it in the space previously shrinked
			// 
			buf->content -= len;
			memmove(&buf->content[0], str, len);
			buf->use += len;
			buf->size += len;
			UPDATE_COMPAT(buf)
			return 0;
		}
	}
	needSize = buf->use + len + 2;
	if(needSize > buf->size) {
		if(!xmlBufResize(buf, needSize)) {
			xmlBufMemoryError(buf, "growing buffer");
			return XML_ERR_NO_MEMORY;
		}
	}
	memmove(&buf->content[len], &buf->content[0], buf->use);
	memmove(&buf->content[0], str, len);
	buf->use += len;
	buf->content[buf->use] = 0;
	UPDATE_COMPAT(buf)
	return 0;
}
/**
 * xmlBufCat:
 * @buf:  the buffer to add to
 * @str:  the #xmlChar string
 *
 * Append a zero terminated string to an XML buffer.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufCat(xmlBuf * buf, const xmlChar * str)
{
	if(!buf || buf->error)
		return -1;
	else {
		CHECK_COMPAT(buf)
		return (str && buf->alloc != XML_BUFFER_ALLOC_IMMUTABLE) ? xmlBufAdd(buf, str, -1) : -1;
	}
}
/**
 * xmlBufCCat:
 * @buf:  the buffer to dump
 * @str:  the C char string
 *
 * Append a zero terminated C string to an XML buffer.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufCCat(xmlBuf * buf, const char * str)
{
	const char * cur;
	if(!buf || buf->error)
		return -1;
	CHECK_COMPAT(buf)
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) return -1;
	if(!str) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufCCat: str == NULL\n");
#endif
		return -1;
	}
	for(cur = str; *cur != 0; cur++) {
		if((buf->use + 10) >= buf->size) {
			if(!xmlBufResize(buf, (buf->use+10))) {
				xmlBufMemoryError(buf, "growing buffer");
				return XML_ERR_NO_MEMORY;
			}
		}
		buf->content[buf->use++] = *cur;
	}
	buf->content[buf->use] = 0;
	UPDATE_COMPAT(buf)
	return 0;
}
/**
 * xmlBufWriteCHAR:
 * @buf:  the XML buffer
 * @string:  the string to add
 *
 * routine which manages and grows an output buffer. This one adds
 * xmlChars at the end of the buffer.
 *
 * Returns 0 if successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufWriteCHAR(xmlBuf * buf, const xmlChar * string) 
{
	if(!buf || buf->error)
		return -1;
	CHECK_COMPAT(buf)
	return (buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) ? -1 : xmlBufCat(buf, string);
}
/**
 * xmlBufWriteChar:
 * @buf:  the XML buffer output
 * @string:  the string to add
 *
 * routine which manage and grows an output buffer. This one add
 * C chars at the end of the array.
 *
 * Returns 0 if successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufWriteChar(xmlBuf * buf, const char * string) 
{
	if(!buf || buf->error)
		return -1;
	CHECK_COMPAT(buf)
	return (buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) ? -1 : xmlBufCCat(buf, string);
}
/**
 * xmlBufWriteQuotedString:
 * @buf:  the XML buffer output
 * @string:  the string to add
 *
 * routine which manage and grows an output buffer. This one writes
 * a quoted or double quoted #xmlChar string, checking first if it holds
 * quote or double-quotes internally
 *
 * Returns 0 if successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufWriteQuotedString(xmlBufPtr buf, const xmlChar * string)
{
	const xmlChar * cur, * base;
	if(!buf || buf->error)
		return -1;
	CHECK_COMPAT(buf)
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
		return -1;
	if(xmlStrchr(string, '\"')) {
		if(xmlStrchr(string, '\'')) {
#ifdef DEBUG_BUFFER
			xmlGenericError(0, "xmlBufWriteQuotedString: string contains quote and double-quotes !\n");
#endif
			xmlBufCCat(buf, "\"");
			base = cur = string;
			while(*cur != 0) {
				if(*cur == '"') {
					if(base != cur)
						xmlBufAdd(buf, base, cur - base);
					xmlBufAdd(buf, reinterpret_cast<const xmlChar *>("&quot;"), 6);
					cur++;
					base = cur;
				}
				else {
					cur++;
				}
			}
			if(base != cur)
				xmlBufAdd(buf, base, cur - base);
			xmlBufCCat(buf, "\"");
		}
		else {
			xmlBufCCat(buf, "\'");
			xmlBufCat(buf, string);
			xmlBufCCat(buf, "\'");
		}
	}
	else {
		xmlBufCCat(buf, "\"");
		xmlBufCat(buf, string);
		xmlBufCCat(buf, "\"");
	}
	return 0;
}

/**
 * xmlBufFromBuffer:
 * @buffer: incoming old buffer to convert to a new one
 *
 * Helper routine to switch from the old buffer structures in use
 * in various APIs. It creates a wrapper xmlBufPtr which will be
 * used for internal processing until the xmlBufBackToBuffer() is
 * issued.
 *
 * Returns a new xmlBufPtr unless the call failed and NULL is returned
 */
xmlBuf * FASTCALL xmlBufFromBuffer(xmlBuffer * buffer)
{
	xmlBuf * ret = 0;
	if(buffer) {
		ret = static_cast<xmlBuf *>(SAlloc::M(sizeof(xmlBuf)));
		if(!ret) {
			xmlBufMemoryError(NULL, "creating buffer");
		}
		else {
			ret->use = buffer->use;
			ret->size = buffer->size;
			ret->compat_use = buffer->use;
			ret->compat_size = buffer->size;
			ret->error = 0;
			ret->buffer = buffer;
			ret->alloc = buffer->alloc;
			ret->content = buffer->content;
			ret->contentIO = buffer->contentIO;
		}
	}
	return ret;
}

/**
 * xmlBufBackToBuffer:
 * @buf: new buffer wrapping the old one
 *
 * Function to be called once internal processing had been done to
 * update back the buffer provided by the user. This can lead to
 * a failure in case the size accumulated in the xmlBuf is larger
 * than what an xmlBuffer can support on 64 bits (INT_MAX)
 * The xmlBufPtr @buf wrapper is deallocated by this call in any case.
 *
 * Returns the old xmlBufferPtr unless the call failed and NULL is returned
 */
xmlBuffer * FASTCALL xmlBufBackToBuffer(xmlBuf * buf)
{
	xmlBuffer * ret;
	if(!buf || buf->error)
		return NULL;
	CHECK_COMPAT(buf)
	if(buf->buffer == NULL) {
		xmlBufFree(buf);
		return NULL;
	}
	ret = buf->buffer;
	/*
	 * What to do in case of error in the buffer ???
	 */
	if(buf->use > INT_MAX) {
		/*
		 * Worse case, we really allocated and used more than the
		 * maximum allowed memory for an xmlBuffer on this architecture.
		 * Keep the buffer but provide a truncated size value.
		 */
		xmlBufOverflowError(buf, "Used size too big for xmlBuffer");
		ret->use = INT_MAX;
		ret->size = INT_MAX;
	}
	else if(buf->size > INT_MAX) {
		/*
		 * milder case, we allocated more than the maximum allowed memory
		 * for an xmlBuffer on this architecture, but used less than the limit.
		 * Keep the buffer but provide a truncated size value.
		 */
		xmlBufOverflowError(buf, "Allocated size too big for xmlBuffer");
		ret->size = INT_MAX;
	}
	ret->use = (int)buf->use;
	ret->size = (int)buf->size;
	ret->alloc = buf->alloc;
	ret->content = buf->content;
	ret->contentIO = buf->contentIO;
	SAlloc::F(buf);
	return ret;
}
/**
 * xmlBufMergeBuffer:
 * @buf: an xmlBufPtr
 * @buffer: the buffer to consume into @buf
 *
 * The content of @buffer is appended to @buf and @buffer is freed
 *
 * Returns -1 in case of error, 0 otherwise, in any case @buffer is freed
 */
int FASTCALL xmlBufMergeBuffer(xmlBuf * buf, xmlBuffer * buffer)
{
	int ret = 0;
	if(!buf || buf->error) {
		xmlBufferFree(buffer);
		return -1;
	}
	CHECK_COMPAT(buf)
	if((buffer != NULL) && (buffer->content != NULL) && (buffer->use > 0)) {
		ret = xmlBufAdd(buf, buffer->content, buffer->use);
	}
	xmlBufferFree(buffer);
	return ret;
}
/**
 * xmlBufResetInput:
 * @buf: an xmlBufPtr
 * @input: an xmlParserInputPtr
 *
 * Update the input to use the current set of pointers from the buffer.
 *
 * Returns -1 in case of error, 0 otherwise
 */
int FASTCALL xmlBufResetInput(xmlBuf * buf, xmlParserInput * input)
{
	if(!input || (buf == NULL) || (buf->error))
		return -1;
	CHECK_COMPAT(buf)
	input->base = input->cur = buf->content;
	input->end = &buf->content[buf->use];
	return 0;
}
/**
 * xmlBufGetInputBase:
 * @buf: an xmlBufPtr
 * @input: an xmlParserInputPtr
 *
 * Get the base of the @input relative to the beginning of the buffer
 *
 * Returns the size_t corresponding to the displacement
 */
size_t FASTCALL xmlBufGetInputBase(xmlBuf * buf, const xmlParserInput * input)
{
	size_t base;
	if(!input || (buf == NULL) || (buf->error))
		return -1;
	CHECK_COMPAT(buf)
	base = input->base - buf->content;
	//
	// We could do some pointer arythmetic checks but that's probably sufficient.
	//
	if(base > buf->size) {
		xmlBufOverflowError(buf, "Input reference outside of the buffer");
		base = 0;
	}
	return (base);
}
/**
 * xmlBufSetInputBaseCur:
 * @buf: an xmlBufPtr
 * @input: an xmlParserInputPtr
 * @base: the base value relative to the beginning of the buffer
 * @cur: the cur value relative to the beginning of the buffer
 *
 * Update the input to use the base and cur relative to the buffer
 * after a possible reallocation of its content
 *
 * Returns -1 in case of error, 0 otherwise
 */
int FASTCALL xmlBufSetInputBaseCur(xmlBufPtr buf, xmlParserInput * input, size_t base, size_t cur)
{
	if(!input || !buf || buf->error)
		return -1;
	CHECK_COMPAT(buf)
	input->base = &buf->content[base];
	input->cur = input->base + cur;
	input->end = &buf->content[buf->use];
	return 0;
}

#define bottom_buf
//#include "elfgcchack.h"
