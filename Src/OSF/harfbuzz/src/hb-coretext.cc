/*
 * Copyright © 2012,2013  Mozilla Foundation.
 * Copyright © 2012,2013  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Mozilla Author(s): Jonathan Kew
 * Google Author(s): Behdad Esfahbod
 */
#include "harfbuzz-internal.h"
#pragma hdrstop

#ifdef HAVE_CORETEXT

#include "hb-coretext.h"
#include "hb-aat-layout.hh"
#include <math.h>
/**
 * SECTION:hb-coretext
 * @title: hb-coretext
 * @short_description: CoreText integration
 * @include: hb-coretext.h
 *
 * Functions for using HarfBuzz with the CoreText fonts.
 **/

/* https://developer.apple.com/documentation/coretext/1508745-ctfontcreatewithgraphicsfont */
#define HB_CORETEXT_DEFAULT_FONT_SIZE 12.f

static void release_table_data(void * user_data)
{
	CFDataRef cf_data = reinterpret_cast<CFDataRef> (user_data);
	CFRelease(cf_data);
}

static hb_blob_t * _hb_cg_reference_table(hb_face_t * face CXX_UNUSED_PARAM, hb_tag_t tag, void * user_data)
{
	CGFontRef cg_font = reinterpret_cast<CGFontRef> (user_data);
	CFDataRef cf_data = CGFontCopyTableForTag(cg_font, tag);
	if(UNLIKELY(!cf_data))
		return nullptr;
	const char * data = reinterpret_cast<const char*> (CFDataGetBytePtr(cf_data));
	const size_t length = CFDataGetLength(cf_data);
	if(!data || !length) {
		CFRelease(cf_data);
		return nullptr;
	}
	return hb_blob_create(data, length, HB_MEMORY_MODE_READONLY,
		   reinterpret_cast<void *> (const_cast<__CFData *> (cf_data)),
		   release_table_data);
}

static void _hb_cg_font_release(void * data)
{
	CGFontRelease((CGFontRef)data);
}

static CTFontDescriptorRef get_last_resort_font_desc()
{
	// TODO Handle allocation failures?
	CTFontDescriptorRef last_resort = CTFontDescriptorCreateWithNameAndSize(CFSTR("LastResort"), 0);
	CFArrayRef cascade_list = CFArrayCreate(kCFAllocatorDefault,
		(const void**)&last_resort,
		1,
		&kCFTypeArrayCallBacks);
	CFRelease(last_resort);
	CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault,
		(const void**)&kCTFontCascadeListAttribute,
		(const void**)&cascade_list,
		1,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);
	CFRelease(cascade_list);

	CTFontDescriptorRef font_desc = CTFontDescriptorCreateWithAttributes(attributes);
	CFRelease(attributes);
	return font_desc;
}

static void release_data(void * info, const void * data, size_t size)
{
	assert(hb_blob_get_length((hb_blob_t*)info) == size &&
	    hb_blob_get_data((hb_blob_t*)info, nullptr) == data);

	hb_blob_destroy((hb_blob_t*)info);
}

static CGFontRef create_cg_font(hb_face_t * face)
{
	CGFontRef cg_font = nullptr;
	if(face->destroy == _hb_cg_font_release) {
		cg_font = CGFontRetain((CGFontRef)face->user_data);
	}
	else {
		hb_blob_t * blob = hb_face_reference_blob(face);
		uint blob_length;
		const char * blob_data = hb_blob_get_data(blob, &blob_length);
		if(UNLIKELY(!blob_length))
			DEBUG_MSG(CORETEXT, face, "Face has empty blob");

		CGDataProviderRef provider = CGDataProviderCreateWithData(blob, blob_data, blob_length, &release_data);
		if(LIKELY(provider)) {
			cg_font = CGFontCreateWithDataProvider(provider);
			if(UNLIKELY(!cg_font))
				DEBUG_MSG(CORETEXT, face, "Face CGFontCreateWithDataProvider() failed");
			CGDataProviderRelease(provider);
		}
	}
	return cg_font;
}

static CTFontRef create_ct_font(CGFontRef cg_font, CGFloat font_size)
{
	CTFontRef ct_font = nullptr;

	/* CoreText does not enable trak table usage / tracking when creating a CTFont
	 * using CTFontCreateWithGraphicsFont. The only way of enabling tracking seems
	 * to be through the CTFontCreateUIFontForLanguage call. */
	CFStringRef cg_postscript_name = CGFontCopyPostScriptName(cg_font);
	if(CFStringHasPrefix(cg_postscript_name, CFSTR(".SFNSText")) ||
	    CFStringHasPrefix(cg_postscript_name, CFSTR(".SFNSDisplay"))) {
#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && MAC_OS_X_VERSION_MIN_REQUIRED < 1080
#define kCTFontUIFontSystem kCTFontSystemFontType
#define kCTFontUIFontEmphasizedSystem kCTFontEmphasizedSystemFontType
#endif
		CTFontUIFontType font_type = kCTFontUIFontSystem;
		if(CFStringHasSuffix(cg_postscript_name, CFSTR("-Bold")))
			font_type = kCTFontUIFontEmphasizedSystem;

		ct_font = CTFontCreateUIFontForLanguage(font_type, font_size, nullptr);
		CFStringRef ct_result_name = CTFontCopyPostScriptName(ct_font);
		if(CFStringCompare(ct_result_name, cg_postscript_name, 0) != kCFCompareEqualTo) {
			CFRelease(ct_font);
			ct_font = nullptr;
		}
		CFRelease(ct_result_name);
	}
	CFRelease(cg_postscript_name);

	if(!ct_font)
		ct_font = CTFontCreateWithGraphicsFont(cg_font, font_size, nullptr, nullptr);

	if(UNLIKELY(!ct_font)) {
		DEBUG_MSG(CORETEXT, cg_font, "Font CTFontCreateWithGraphicsFont() failed");
		return nullptr;
	}

	/* crbug.com/576941 and crbug.com/625902 and the investigation in the latter
	 * bug indicate that the cascade list reconfiguration occasionally causes
	 * crashes in CoreText on OS X 10.9, thus let's skip this step on older
	 * operating system versions. Except for the emoji font, where _not_
	 * reconfiguring the cascade list causes CoreText crashes. For details, see
	 * crbug.com/549610 */
	// 0x00070000 stands for "kCTVersionNumber10_10", see CoreText.h
	if(&CTGetCoreTextVersion != nullptr && CTGetCoreTextVersion() < 0x00070000) {
		CFStringRef fontName = CTFontCopyPostScriptName(ct_font);
		bool isEmojiFont = CFStringCompare(fontName, CFSTR("AppleColorEmoji"), 0) == kCFCompareEqualTo;
		CFRelease(fontName);
		if(!isEmojiFont)
			return ct_font;
	}

	CFURLRef original_url = nullptr;
#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && MAC_OS_X_VERSION_MIN_REQUIRED < 1060
	ATSFontRef atsFont;
	FSRef fsref;
	OSStatus status;
	atsFont = CTFontGetPlatformFont(ct_font, NULL);
	status = ATSFontGetFileReference(atsFont, &fsref);
	if(status == noErr)
		original_url = CFURLCreateFromFSRef(NULL, &fsref);
#else
	original_url = (CFURLRef)CTFontCopyAttribute(ct_font, kCTFontURLAttribute);
#endif

	/* Create font copy with cascade list that has LastResort first; this speeds up CoreText
	 * font fallback which we don't need anyway. */
	{
		CTFontDescriptorRef last_resort_font_desc = get_last_resort_font_desc();
		CTFontRef new_ct_font = CTFontCreateCopyWithAttributes(ct_font, 0.0, nullptr, last_resort_font_desc);
		CFRelease(last_resort_font_desc);
		if(new_ct_font) {
			/* The CTFontCreateCopyWithAttributes call fails to stay on the same font
			 * when reconfiguring the cascade list and may switch to a different font
			 * when there are fonts that go by the same name, since the descriptor is
			 * just name and size.
			 *
			 * Avoid reconfiguring the cascade lists if the new font is outside the
			 * system locations that we cannot access from the sandboxed renderer
			 * process in Blink. This can be detected by the new file URL location
			 * that the newly found font points to. */
			CFURLRef new_url = nullptr;
#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && MAC_OS_X_VERSION_MIN_REQUIRED < 1060
			atsFont = CTFontGetPlatformFont(new_ct_font, NULL);
			status = ATSFontGetFileReference(atsFont, &fsref);
			if(status == noErr)
				new_url = CFURLCreateFromFSRef(NULL, &fsref);
#else
			new_url = (CFURLRef)CTFontCopyAttribute(new_ct_font, kCTFontURLAttribute);
#endif
			// Keep reconfigured font if URL cannot be retrieved (seems to be the case
			// on Mac OS 10.12 Sierra), speculative fix for crbug.com/625606
			if(!original_url || !new_url || CFEqual(original_url, new_url)) {
				CFRelease(ct_font);
				ct_font = new_ct_font;
			}
			else {
				CFRelease(new_ct_font);
				DEBUG_MSG(CORETEXT, ct_font, "Discarding reconfigured CTFont, location changed.");
			}
			if(new_url)
				CFRelease(new_url);
		}
		else
			DEBUG_MSG(CORETEXT, ct_font, "Font copy with empty cascade list failed");
	}

	if(original_url)
		CFRelease(original_url);
	return ct_font;
}

hb_coretext_face_data_t * _hb_coretext_shaper_face_data_create(hb_face_t * face)
{
	CGFontRef cg_font = create_cg_font(face);

	if(UNLIKELY(!cg_font)) {
		DEBUG_MSG(CORETEXT, face, "CGFont creation failed..");
		return nullptr;
	}

	return (hb_coretext_face_data_t*)cg_font;
}

void _hb_coretext_shaper_face_data_destroy(hb_coretext_face_data_t * data)
{
	CFRelease((CGFontRef)data);
}

/**
 * hb_coretext_face_create:
 * @cg_font: The CGFontRef to work upon
 *
 * Creates an #hb_face_t face object from the specified
 * CGFontRef.
 *
 * Return value: the new #hb_face_t face object
 *
 * Since: 0.9.10
 */
hb_face_t * hb_coretext_face_create(CGFontRef cg_font)
{
	return hb_face_create_for_tables(_hb_cg_reference_table, CGFontRetain(cg_font), _hb_cg_font_release);
}

/**
 * hb_coretext_face_get_cg_font:
 * @face: The #hb_face_t to work upon
 *
 * Fetches the CGFontRef associated with an #hb_face_t
 * face object
 *
 * Return value: the CGFontRef found
 *
 * Since: 0.9.10
 */
CGFontRef hb_coretext_face_get_cg_font(hb_face_t * face)
{
	return (CGFontRef)(const void*)face->data.coretext;
}

hb_coretext_font_data_t * _hb_coretext_shaper_font_data_create(hb_font_t * font)
{
	hb_face_t * face = font->face;
	const hb_coretext_face_data_t * face_data = face->data.coretext;
	if(UNLIKELY(!face_data)) return nullptr;
	CGFontRef cg_font = (CGFontRef)(const void*)face->data.coretext;

	CGFloat font_size = (CGFloat)(font->ptem <= 0.f ? HB_CORETEXT_DEFAULT_FONT_SIZE : font->ptem);
	CTFontRef ct_font = create_ct_font(cg_font, font_size);

	if(UNLIKELY(!ct_font)) {
		DEBUG_MSG(CORETEXT, font, "CGFont creation failed..");
		return nullptr;
	}

	return (hb_coretext_font_data_t*)ct_font;
}

void _hb_coretext_shaper_font_data_destroy(hb_coretext_font_data_t * data)
{
	CFRelease((CTFontRef)data);
}

static const hb_coretext_font_data_t * hb_coretext_font_data_sync(hb_font_t * font)
{
retry:
	const hb_coretext_font_data_t *data = font->data.coretext;
	if(UNLIKELY(!data)) return nullptr;

	if(fabs(CTFontGetSize((CTFontRef)data) - (CGFloat)font->ptem) > .5) {
		/* XXX-MT-bug
		 * Note that evaluating condition above can be dangerous if another thread
		 * got here first and destructed data.  That's, as always, bad use pattern.
		 * If you modify the font (change font size), other threads must not be
		 * using it at the same time.  However, since this check is delayed to
		 * when one actually tries to shape something, this is a XXX race condition
		 * (and the only one we have that I know of) right now.  Ie. you modify the
		 * font size in one thread, then (supposedly safely) try to use it from two
		 * or more threads and BOOM!  I'm not sure how to fix this.  We want RCU.
		 */

		/* Drop and recreate. */
		/* If someone dropped it in the mean time, throw it away and don't touch it.
		 * Otherwise, destruct it. */
		if(LIKELY(font->data.coretext.cmpexch(const_cast<hb_coretext_font_data_t *> (data), nullptr)))
			_hb_coretext_shaper_font_data_destroy(const_cast<hb_coretext_font_data_t *> (data));
		else
			goto retry;
	}
	return font->data.coretext;
}

/**
 * hb_coretext_font_create:
 * @ct_font: The CTFontRef to work upon
 *
 * Creates an #hb_font_t font object from the specified
 * CTFontRef.
 *
 * Return value: the new #hb_font_t font object
 *
 * Since: 1.7.2
 **/
hb_font_t * hb_coretext_font_create(CTFontRef ct_font)
{
	CGFontRef cg_font = CTFontCopyGraphicsFont(ct_font, nullptr);
	hb_face_t * face = hb_coretext_face_create(cg_font);
	CFRelease(cg_font);
	hb_font_t * font = hb_font_create(face);
	hb_face_destroy(face);
	if(UNLIKELY(hb_object_is_immutable(font)))
		return font;
	hb_font_set_ptem(font, CTFontGetSize(ct_font));
	/* Let there be dragons here... */
	font->data.coretext.cmpexch(nullptr, (hb_coretext_font_data_t*)CFRetain(ct_font));
	return font;
}

/**
 * hb_coretext_face_get_ct_font:
 * @font: #hb_font_t to work upon
 *
 * Fetches the CTFontRef associated with the specified
 * #hb_font_t font object.
 *
 * Return value: the CTFontRef found
 *
 * Since: 0.9.10
 */
CTFontRef hb_coretext_font_get_ct_font(hb_font_t * font)
{
	const hb_coretext_font_data_t * data = hb_coretext_font_data_sync(font);
	return data ? (CTFontRef)data : nullptr;
}
/*
 * shaper
 */
struct feature_record_t {
	uint feature;
	uint setting;
};

struct active_feature_t {
	feature_record_t rec;
	uint order;
	HB_INTERNAL static int cmp(const void * pa, const void * pb) {
		const active_feature_t * a = (const active_feature_t*)pa;
		const active_feature_t * b = (const active_feature_t*)pb;
		return a->rec.feature < b->rec.feature ? -1 : a->rec.feature > b->rec.feature ? 1 :
		       a->order < b->order ? -1 : a->order > b->order ? 1 :
		       a->rec.setting < b->rec.setting ? -1 : a->rec.setting > b->rec.setting ? 1 :
		       0;
	}

	bool operator == (const active_feature_t * f) {
		return cmp(this, f) == 0;
	}
};

struct feature_event_t {
	uint index;
	bool start;
	active_feature_t feature;

	HB_INTERNAL static int cmp(const void * pa, const void * pb) {
		const feature_event_t * a = (const feature_event_t*)pa;
		const feature_event_t * b = (const feature_event_t*)pb;
		return a->index < b->index ? -1 : a->index > b->index ? 1 :
		       a->start < b->start ? -1 : a->start > b->start ? 1 :
		       active_feature_t::cmp(&a->feature, &b->feature);
	}
};

struct range_record_t {
	CTFontRef font;
	uint index_first; /* == start */
	uint index_last; /* == end - 1 */
};

hb_bool_t _hb_coretext_shape(hb_shape_plan_t * shape_plan,
    hb_font_t * font,
    hb_buffer_t * buffer,
    const hb_feature_t * features,
    uint num_features)
{
	hb_face_t * face = font->face;
	CGFontRef cg_font = (CGFontRef)(const void*)face->data.coretext;
	CTFontRef ct_font = (CTFontRef)hb_coretext_font_data_sync(font);

	CGFloat ct_font_size = CTFontGetSize(ct_font);
	CGFloat x_mult = (CGFloat)font->x_scale / ct_font_size;
	CGFloat y_mult = (CGFloat)font->y_scale / ct_font_size;

	/* Attach marks to their bases, to match the 'ot' shaper.
	 * Adapted from a very old version of hb-ot-shape:hb_form_clusters().
	 * Note that this only makes us be closer to the 'ot' shaper,
	 * but by no means the same.  For example, if there's
	 * B1 M1 B2 M2, and B1-B2 form a ligature, M2's cluster will
	 * continue pointing to B2 even though B2 was merged into B1's
	 * cluster... */
	if(buffer->cluster_level == HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES) {
		hb_unicode_funcs_t * unicode = buffer->unicode;
		uint count = buffer->len;
		hb_glyph_info_t * info = buffer->info;
		for(uint i = 1; i < count; i++)
			if(HB_UNICODE_GENERAL_CATEGORY_IS_MARK(unicode->general_category(info[i].codepoint)))
				buffer->merge_clusters(i - 1, i + 1);
	}

	hb_vector_t<feature_record_t> feature_records;
	hb_vector_t<range_record_t> range_records;

	/*
	 * Set up features.
	 * (copied + modified from code from hb-uniscribe.cc)
	 */
	if(num_features) {
		/* Sort features by start/end events. */
		hb_vector_t<feature_event_t> feature_events;
		for(uint i = 0; i < num_features; i++) {
			active_feature_t feature;

#if MAC_OS_X_VERSION_MIN_REQUIRED < 101000
			const hb_aat_feature_mapping_t * mapping = hb_aat_layout_find_feature_mapping(features[i].tag);
			if(!mapping)
				continue;

			feature.rec.feature = mapping->aatFeatureType;
			feature.rec.setting = features[i].value ? mapping->selectorToEnable : mapping->selectorToDisable;
#else
			feature.rec.feature = features[i].tag;
			feature.rec.setting = features[i].value;
#endif
			feature.order = i;

			feature_event_t * event;

			event = feature_events.push();
			event->index = features[i].start;
			event->start = true;
			event->feature = feature;

			event = feature_events.push();
			event->index = features[i].end;
			event->start = false;
			event->feature = feature;
		}
		feature_events.qsort();
		/* Add a strategic final event. */
		{
			active_feature_t feature;
			feature.rec.feature = HB_TAG_NONE;
			feature.rec.setting = 0;
			feature.order = num_features + 1;

			feature_event_t * event = feature_events.push();
			event->index = 0; /* This value does magic. */
			event->start = false;
			event->feature = feature;
		}

		/* Scan events and save features for each range. */
		hb_vector_t<active_feature_t> active_features;
		uint last_index = 0;
		for(uint i = 0; i < feature_events.length; i++) {
			feature_event_t * event = &feature_events[i];
			if(event->index != last_index) {
				/* Save a snapshot of active features and the range. */
				range_record_t * range = range_records.push();

				if(active_features.length) {
					CFMutableArrayRef features_array = CFArrayCreateMutable(kCFAllocatorDefault, 0,
						&kCFTypeArrayCallBacks);
					/* TODO sort and resolve conflicting features? */
					/* active_features.qsort (); */
					for(uint j = 0; j < active_features.length; j++) {
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101000
						CFStringRef keys[] = {
							kCTFontFeatureTypeIdentifierKey,
							kCTFontFeatureSelectorIdentifierKey
						};
						CFNumberRef values[] = {
							CFNumberCreate(kCFAllocatorDefault,
							    kCFNumberIntType,
							    &active_features[j].rec.feature),
							CFNumberCreate(kCFAllocatorDefault,
							    kCFNumberIntType,
							    &active_features[j].rec.setting)
						};
#else
						char tag[5] = {HB_UNTAG(active_features[j].rec.feature)};
						CFTypeRef keys[] = {
							kCTFontOpenTypeFeatureTag,
							kCTFontOpenTypeFeatureValue
						};
						CFTypeRef values[] = {
							CFStringCreateWithCString(kCFAllocatorDefault, tag, kCFStringEncodingASCII),
							CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &active_features[j].rec.setting)
						};
#endif
						static_assert((SIZEOFARRAY(keys) == SIZEOFARRAY(values)), "");
						CFDictionaryRef dict = CFDictionaryCreate(kCFAllocatorDefault,
							(const void**)keys,
							(const void**)values,
							ARRAY_LENGTH(keys),
							&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
						for(uint i = 0; i < ARRAY_LENGTH(values); i++)
							CFRelease(values[i]);
						CFArrayAppendValue(features_array, dict);
						CFRelease(dict);
					}
					CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault,
						(const void**)&kCTFontFeatureSettingsAttribute,
						(const void**)&features_array,
						1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
					CFRelease(features_array);

					CTFontDescriptorRef font_desc = CTFontDescriptorCreateWithAttributes(attributes);
					CFRelease(attributes);

					range->font = CTFontCreateCopyWithAttributes(ct_font, 0.0, nullptr, font_desc);
					CFRelease(font_desc);
				}
				else {
					range->font = nullptr;
				}

				range->index_first = last_index;
				range->index_last  = event->index - 1;

				last_index = event->index;
			}

			if(event->start) {
				active_features.push(event->feature);
			}
			else {
				active_feature_t * feature = active_features.find(&event->feature);
				if(feature)
					active_features.remove(feature - active_features.arrayZ);
			}
		}
	}

	uint scratch_size;
	hb_buffer_t::scratch_buffer_t * scratch = buffer->get_scratch_buffer(&scratch_size);

#define ALLOCATE_ARRAY(Type, name, len, on_no_room) \
	Type *name = (Type*)scratch; \
	do { \
		uint _consumed = idivroundup((len) * sizeof(Type), sizeof(*scratch)); \
		if(UNLIKELY(_consumed > scratch_size)) \
		{ \
			on_no_room; \
			assert(0); \
		} \
		scratch += _consumed; \
		scratch_size -= _consumed; \
	} while(0)

	ALLOCATE_ARRAY(UniChar, pchars, buffer->len * 2, ((void)nullptr) /*nothing*/);
	uint chars_len = 0;
	for(uint i = 0; i < buffer->len; i++) {
		hb_codepoint_t c = buffer->info[i].codepoint;
		if(LIKELY(c <= 0xFFFFu))
			pchars[chars_len++] = c;
		else if(UNLIKELY(c > 0x10FFFFu))
			pchars[chars_len++] = 0xFFFDu;
		else {
			pchars[chars_len++] = 0xD800u + ((c - 0x10000u) >> 10);
			pchars[chars_len++] = 0xDC00u + ((c - 0x10000u) & ((1u << 10) - 1));
		}
	}

	ALLOCATE_ARRAY(uint, log_clusters, chars_len, ((void)nullptr) /*nothing*/);
	chars_len = 0;
	for(uint i = 0; i < buffer->len; i++) {
		hb_codepoint_t c = buffer->info[i].codepoint;
		uint cluster = buffer->info[i].cluster;
		log_clusters[chars_len++] = cluster;
		if(hb_in_range(c, 0x10000u, 0x10FFFFu))
			log_clusters[chars_len++] = cluster; /* Surrogates. */
	}

#define FAIL(...) \
	HB_STMT_START { \
		DEBUG_MSG(CORETEXT, nullptr, __VA_ARGS__); \
		ret = false; \
		goto fail; \
	} HB_STMT_END

	bool ret = true;
	CFStringRef string_ref = nullptr;
	CTLineRef line = nullptr;

	if(false) {
resize_and_retry:
		DEBUG_MSG(CORETEXT, buffer, "Buffer resize");
		/* string_ref uses the scratch-buffer for backing store, and line references
		 * string_ref (via attr_string).  We must release those before resizing buffer. */
		assert(string_ref);
		assert(line);
		CFRelease(string_ref);
		CFRelease(line);
		string_ref = nullptr;
		line = nullptr;

		/* Get previous start-of-scratch-area, that we use later for readjusting
		 * our existing scratch arrays. */
		uint old_scratch_used;
		hb_buffer_t::scratch_buffer_t * old_scratch;
		old_scratch = buffer->get_scratch_buffer(&old_scratch_used);
		old_scratch_used = scratch - old_scratch;

		if(UNLIKELY(!buffer->ensure(buffer->allocated * 2)))
			FAIL("Buffer resize failed");

		/* Adjust scratch, pchars, and log_cluster arrays.  This is ugly, but really the
		 * cleanest way to do without completely restructuring the rest of this shaper. */
		scratch = buffer->get_scratch_buffer(&scratch_size);
		pchars = reinterpret_cast<UniChar *> (((char *)scratch + ((char *)pchars - (char *)old_scratch)));
		log_clusters = reinterpret_cast<uint *> (((char *)scratch + ((char *)log_clusters - (char *)old_scratch)));
		scratch += old_scratch_used;
		scratch_size -= old_scratch_used;
	}
	{
		string_ref = CFStringCreateWithCharactersNoCopy(nullptr,
			pchars, chars_len,
			kCFAllocatorNull);
		if(UNLIKELY(!string_ref))
			FAIL("CFStringCreateWithCharactersNoCopy failed");

		/* Create an attributed string, populate it, and create a line from it, then release attributed string.
		   */
		{
			CFMutableAttributedStringRef attr_string = CFAttributedStringCreateMutable(kCFAllocatorDefault,
				chars_len);
			if(UNLIKELY(!attr_string))
				FAIL("CFAttributedStringCreateMutable failed");
			CFAttributedStringReplaceString(attr_string, CFRangeMake(0, 0), string_ref);
			if(HB_DIRECTION_IS_VERTICAL(buffer->props.direction)) {
				CFAttributedStringSetAttribute(attr_string, CFRangeMake(0, chars_len),
				    kCTVerticalFormsAttributeName, kCFBooleanTrue);
			}

			if(buffer->props.language) {
/* What's the iOS equivalent of this check?
 * The symbols was introduced in iOS 7.0.
 * At any rate, our fallback is safe and works fine. */
#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && MAC_OS_X_VERSION_MIN_REQUIRED < 1090
#define kCTLanguageAttributeName CFSTR("NSLanguage")
#endif
				CFStringRef lang = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
					hb_language_to_string(buffer->props.language),
					kCFStringEncodingUTF8,
					kCFAllocatorNull);
				if(UNLIKELY(!lang)) {
					CFRelease(attr_string);
					FAIL("CFStringCreateWithCStringNoCopy failed");
				}
				CFAttributedStringSetAttribute(attr_string, CFRangeMake(0, chars_len),
				    kCTLanguageAttributeName, lang);
				CFRelease(lang);
			}
			CFAttributedStringSetAttribute(attr_string, CFRangeMake(0, chars_len),
			    kCTFontAttributeName, ct_font);

			if(num_features && range_records.length) {
				uint start = 0;
				range_record_t * last_range = &range_records[0];
				for(uint k = 0; k < chars_len; k++) {
					range_record_t * range = last_range;
					while(log_clusters[k] < range->index_first)
						range--;
					while(log_clusters[k] > range->index_last)
						range++;
					if(range != last_range) {
						if(last_range->font)
							CFAttributedStringSetAttribute(attr_string, CFRangeMake(start, k - start),
							    kCTFontAttributeName, last_range->font);

						start = k;
					}

					last_range = range;
				}
				if(start != chars_len && last_range->font)
					CFAttributedStringSetAttribute(attr_string, CFRangeMake(start, chars_len - start),
					    kCTFontAttributeName, last_range->font);
			}
			/* Enable/disable kern if requested.
			 *
			 * Note: once kern is disabled, reenabling it doesn't currently seem to work in CoreText.
			 */
			if(num_features) {
				uint zeroint = 0;
				CFNumberRef zero = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &zeroint);
				for(uint i = 0; i < num_features; i++) {
					const hb_feature_t &feature = features[i];
					if(feature.tag == HB_TAG('k', 'e', 'r', 'n') &&
					    feature.start < chars_len && feature.start < feature.end) {
						CFRange feature_range = CFRangeMake(feature.start,
							hb_min(feature.end, chars_len) - feature.start);
						if(feature.value)
							CFAttributedStringRemoveAttribute(attr_string, feature_range, kCTKernAttributeName);
						else
							CFAttributedStringSetAttribute(attr_string,
							    feature_range,
							    kCTKernAttributeName,
							    zero);
					}
				}
				CFRelease(zero);
			}

			int level = HB_DIRECTION_IS_FORWARD(buffer->props.direction) ? 0 : 1;
			CFNumberRef level_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &level);
#if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && MAC_OS_X_VERSION_MIN_REQUIRED < 1060
			extern const CFStringRef kCTTypesetterOptionForcedEmbeddingLevel;
#endif
			CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault,
				(const void**)&kCTTypesetterOptionForcedEmbeddingLevel,
				(const void**)&level_number,
				1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CFRelease(level_number);
			if(UNLIKELY(!options)) {
				CFRelease(attr_string);
				FAIL("CFDictionaryCreate failed");
			}
			CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedStringAndOptions(attr_string, options);
			CFRelease(options);
			CFRelease(attr_string);
			if(UNLIKELY(!typesetter))
				FAIL("CTTypesetterCreateWithAttributedStringAndOptions failed");

			line = CTTypesetterCreateLine(typesetter, CFRangeMake(0, 0));
			CFRelease(typesetter);
			if(UNLIKELY(!line))
				FAIL("CTTypesetterCreateLine failed");
		}

		CFArrayRef glyph_runs = CTLineGetGlyphRuns(line);
		uint num_runs = CFArrayGetCount(glyph_runs);
		DEBUG_MSG(CORETEXT, nullptr, "Num runs: %d", num_runs);

		buffer->len = 0;
		uint32_t status_and = ~0, status_or = 0;
		double advances_so_far = 0;
		/* For right-to-left runs, CoreText returns the glyphs positioned such that
		 * any trailing whitespace is to the left of (0,0).  Adjust coordinate system
		 * to fix for that.  Test with any RTL string with trailing spaces.
		 * https://crbug.com/469028
		 */
		if(HB_DIRECTION_IS_BACKWARD(buffer->props.direction)) {
			advances_so_far -= CTLineGetTrailingWhitespaceWidth(line);
			if(HB_DIRECTION_IS_VERTICAL(buffer->props.direction))
				advances_so_far = -advances_so_far;
		}

		const CFRange range_all = CFRangeMake(0, 0);

		for(uint i = 0; i < num_runs; i++) {
			CTRunRef run = static_cast<CTRunRef>(CFArrayGetValueAtIndex(glyph_runs, i));
			CTRunStatus run_status = CTRunGetStatus(run);
			status_or  |= run_status;
			status_and &= run_status;
			DEBUG_MSG(CORETEXT, run, "CTRunStatus: %x", run_status);
			double run_advance = CTRunGetTypographicBounds(run, range_all, nullptr, nullptr, nullptr);
			if(HB_DIRECTION_IS_VERTICAL(buffer->props.direction))
				run_advance = -run_advance;
			DEBUG_MSG(CORETEXT, run, "Run advance: %g", run_advance);

			/* CoreText does automatic font fallback (AKA "cascading") for  characters
			 * not supported by the requested font, and provides no way to turn it off,
			 * so we must detect if the returned run uses a font other than the requested
			 * one and fill in the buffer with .notdef glyphs instead of random glyph
			 * indices from a different font.
			 */
			CFDictionaryRef attributes = CTRunGetAttributes(run);
			CTFontRef run_ct_font = static_cast<CTFontRef>(CFDictionaryGetValue(attributes, kCTFontAttributeName));
			if(!CFEqual(run_ct_font, ct_font)) {
				/* The run doesn't use our main font instance.  We have to figure out
				 * whether font fallback happened, or this is just CoreText giving us
				 * another CTFont using the same underlying CGFont.  CoreText seems
				 * to do that in a variety of situations, one of which being vertical
				 * text, but also perhaps for caching reasons.
				 *
				 * First, see if it uses any of our subfonts created to set font features...
				 *
				 * Next, compare the CGFont to the one we used to create our fonts.
				 * Even this doesn't work all the time.
				 *
				 * Finally, we compare PS names, which I don't think are unique...
				 *
				 * Looks like if we really want to be sure here we have to modify the
				 * font to change the name table, similar to what we do in the uniscribe
				 * backend.
				 *
				 * However, even that wouldn't work if we were passed in the CGFont to
				 * construct a hb_face to begin with.
				 *
				 * See: https://github.com/harfbuzz/harfbuzz/pull/36
				 *
				 * Also see: https://bugs.chromium.org/p/chromium/issues/detail?id=597098
				 */
				bool matched = false;
				for(uint i = 0; i < range_records.length; i++)
					if(range_records[i].font && CFEqual(run_ct_font, range_records[i].font)) {
						matched = true;
						break;
					}
				if(!matched) {
					CGFontRef run_cg_font = CTFontCopyGraphicsFont(run_ct_font, nullptr);
					if(run_cg_font) {
						matched = CFEqual(run_cg_font, cg_font);
						CFRelease(run_cg_font);
					}
				}
				if(!matched) {
					CFStringRef font_ps_name = CTFontCopyName(ct_font, kCTFontPostScriptNameKey);
					CFStringRef run_ps_name = CTFontCopyName(run_ct_font, kCTFontPostScriptNameKey);
					CFComparisonResult result = CFStringCompare(run_ps_name, font_ps_name, 0);
					CFRelease(run_ps_name);
					CFRelease(font_ps_name);
					if(result == kCFCompareEqualTo)
						matched = true;
				}
				if(!matched) {
					CFRange range = CTRunGetStringRange(run);
					DEBUG_MSG(CORETEXT, run, "Run used fallback font: %ld..%ld",
					    range.location, range.location + range.length);
					if(!buffer->ensure_inplace(buffer->len + range.length))
						goto resize_and_retry;
					hb_glyph_info_t * info = buffer->info + buffer->len;

					hb_codepoint_t notdef = 0;
					hb_direction_t dir = buffer->props.direction;
					hb_position_t x_advance, y_advance, x_offset, y_offset;
					hb_font_get_glyph_advance_for_direction(font, notdef, dir, &x_advance, &y_advance);
					hb_font_get_glyph_origin_for_direction(font, notdef, dir, &x_offset, &y_offset);
					hb_position_t advance = x_advance + y_advance;
					x_offset = -x_offset;
					y_offset = -y_offset;

					uint old_len = buffer->len;
					for(CFIndex j = range.location; j < range.location + range.length; j++) {
						UniChar ch = CFStringGetCharacterAtIndex(string_ref, j);
						if(hb_in_range<UniChar> (ch, 0xDC00u, 0xDFFFu) && range.location < j) {
							ch = CFStringGetCharacterAtIndex(string_ref, j - 1);
							if(hb_in_range<UniChar> (ch, 0xD800u, 0xDBFFu))
								/* This is the second of a surrogate pair.  Don't need
								   .notdef
								 * for this one. */
								continue;
						}
						if(buffer->unicode->is_default_ignorable(ch))
							continue;

						info->codepoint = notdef;
						info->cluster = log_clusters[j];

						info->mask = advance;
						info->var1.i32 = x_offset;
						info->var2.i32 = y_offset;

						info++;
						buffer->len++;
					}
					if(HB_DIRECTION_IS_BACKWARD(buffer->props.direction))
						buffer->reverse_range(old_len, buffer->len);
					advances_so_far += run_advance;
					continue;
				}
			}

			uint num_glyphs = CTRunGetGlyphCount(run);
			if(num_glyphs == 0)
				continue;

			if(!buffer->ensure_inplace(buffer->len + num_glyphs))
				goto resize_and_retry;

			hb_glyph_info_t * run_info = buffer->info + buffer->len;

			/* Testing used to indicate that CTRunGetGlyphsPtr, etc (almost?) always
			 * succeed, and so copying data to our own buffer will be rare.  Reports
			 * have it that this changed in OS X 10.10 Yosemite, and nullptr is returned
			 * frequently.  At any rate, we can test that codepath by setting USE_PTR
			 * to false. */

#define USE_PTR true

#define SCRATCH_SAVE() \
	uint scratch_size_saved = scratch_size; \
	hb_buffer_t::scratch_buffer_t * scratch_saved = scratch

#define SCRATCH_RESTORE() \
	scratch_size = scratch_size_saved; \
	scratch = scratch_saved

			{ /* Setup glyphs */
				SCRATCH_SAVE();
				const CGGlyph* glyphs = USE_PTR ? CTRunGetGlyphsPtr(run) : nullptr;
				if(!glyphs) {
					ALLOCATE_ARRAY(CGGlyph, glyph_buf, num_glyphs, goto resize_and_retry);
					CTRunGetGlyphs(run, range_all, glyph_buf);
					glyphs = glyph_buf;
				}
				const CFIndex* string_indices = USE_PTR ? CTRunGetStringIndicesPtr(run) : nullptr;
				if(!string_indices) {
					ALLOCATE_ARRAY(CFIndex, index_buf, num_glyphs, goto resize_and_retry);
					CTRunGetStringIndices(run, range_all, index_buf);
					string_indices = index_buf;
				}
				hb_glyph_info_t * info = run_info;
				for(uint j = 0; j < num_glyphs; j++) {
					info->codepoint = glyphs[j];
					info->cluster = log_clusters[string_indices[j]];
					info++;
				}
				SCRATCH_RESTORE();
			}
			{
				/* Setup positions.
				 * Note that CoreText does not return advances for glyphs.  As such,
				 * for all but last glyph, we use the delta position to next glyph as
				 * advance (in the advance direction only), and for last glyph we set
				 * whatever is needed to make the whole run's advance add up. */
				SCRATCH_SAVE();
				const CGPoint* positions = USE_PTR ? CTRunGetPositionsPtr(run) : nullptr;
				if(!positions) {
					ALLOCATE_ARRAY(CGPoint, position_buf, num_glyphs, goto resize_and_retry);
					CTRunGetPositions(run, range_all, position_buf);
					positions = position_buf;
				}
				hb_glyph_info_t * info = run_info;
				if(HB_DIRECTION_IS_HORIZONTAL(buffer->props.direction)) {
					hb_position_t x_offset = (positions[0].x - advances_so_far) * x_mult;
					for(uint j = 0; j < num_glyphs; j++) {
						double advance;
						if(LIKELY(j + 1 < num_glyphs))
							advance = positions[j + 1].x - positions[j].x;
						else /* last glyph */
							advance = run_advance - (positions[j].x - positions[0].x);
						info->mask = advance * x_mult;
						info->var1.i32 = x_offset;
						info->var2.i32 = positions[j].y * y_mult;
						info++;
					}
				}
				else {
					hb_position_t y_offset = (positions[0].y - advances_so_far) * y_mult;
					for(uint j = 0; j < num_glyphs; j++) {
						double advance;
						if(LIKELY(j + 1 < num_glyphs))
							advance = positions[j + 1].y - positions[j].y;
						else /* last glyph */
							advance = run_advance - (positions[j].y - positions[0].y);
						info->mask = advance * y_mult;
						info->var1.i32 = positions[j].x * x_mult;
						info->var2.i32 = y_offset;
						info++;
					}
				}
				SCRATCH_RESTORE();
				advances_so_far += run_advance;
			}
#undef SCRATCH_RESTORE
#undef SCRATCH_SAVE
#undef USE_PTR
#undef ALLOCATE_ARRAY

			buffer->len += num_glyphs;
		}

		/* Mac OS 10.6 doesn't have kCTTypesetterOptionForcedEmbeddingLevel,
		 * or if it does, it doesn't respect it.  So we get runs with wrong
		 * directions.  As such, disable the assert...  It wouldn't crash, but
		 * cursoring will be off...
		 *
		 * https://crbug.com/419769
		 */
		if(false) {
			/* Make sure all runs had the expected direction. */
			CXX_UNUSED_PARAM bool backward = HB_DIRECTION_IS_BACKWARD(buffer->props.direction);
			assert(bool (status_and & kCTRunStatusRightToLeft) == backward);
			assert(bool (status_or  & kCTRunStatusRightToLeft) == backward);
		}

		buffer->clear_positions();

		uint count = buffer->len;
		hb_glyph_info_t * info = buffer->info;
		hb_glyph_position_t * pos = buffer->pos;
		if(HB_DIRECTION_IS_HORIZONTAL(buffer->props.direction))
			for(uint i = 0; i < count; i++) {
				pos->x_advance = info->mask;
				pos->x_offset = info->var1.i32;
				pos->y_offset = info->var2.i32;

				info++, pos++;
			}
		else
			for(uint i = 0; i < count; i++) {
				pos->y_advance = info->mask;
				pos->x_offset = info->var1.i32;
				pos->y_offset = info->var2.i32;

				info++, pos++;
			}

		/* Fix up clusters so that we never return out-of-order indices;
		 * if core text has reordered glyphs, we'll merge them to the
		 * beginning of the reordered cluster.  CoreText is nice enough
		 * to tell us whenever it has produced nonmonotonic results...
		 * Note that we assume the input clusters were nonmonotonic to
		 * begin with.
		 *
		 * This does *not* mean we'll form the same clusters as Uniscribe
		 * or the native OT backend, only that the cluster indices will be
		 * monotonic in the output buffer. */
		if(count > 1 && (status_or & kCTRunStatusNonMonotonic)) {
			hb_glyph_info_t * info = buffer->info;
			if(HB_DIRECTION_IS_FORWARD(buffer->props.direction)) {
				uint cluster = info[count - 1].cluster;
				for(uint i = count - 1; i > 0; i--) {
					cluster = hb_min(cluster, info[i - 1].cluster);
					info[i - 1].cluster = cluster;
				}
			}
			else {
				uint cluster = info[0].cluster;
				for(uint i = 1; i < count; i++) {
					cluster = hb_min(cluster, info[i].cluster);
					info[i].cluster = cluster;
				}
			}
		}
	}
	buffer->unsafe_to_break_all();
#undef FAIL
fail:
	if(string_ref)
		CFRelease(string_ref);
	if(line)
		CFRelease(line);
	for(uint i = 0; i < range_records.length; i++)
		if(range_records[i].font)
			CFRelease(range_records[i].font);
	return ret;
}

#endif
