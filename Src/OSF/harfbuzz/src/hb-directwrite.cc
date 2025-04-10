/*
 * Copyright © 2015-2019  Ebrahim Byagowi
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 */
#include "harfbuzz-internal.h"
#pragma hdrstop

#ifdef HAVE_DIRECTWRITE

#include <dwrite_1.h>
#include "hb-directwrite.h"

/* Declare object creator for dynamic support of DWRITE */
typedef HRESULT (*WINAPI t_DWriteCreateFactory)(
	DWRITE_FACTORY_TYPE factoryType,
	REFIID iid,
	IUnknown            ** factory
	);

/*
 * hb-directwrite uses new/delete syntatically but as we let users
 * to override malloc/free, we will redefine new/delete so users
 * won't need to do that by their own.
 */
void * operator new(size_t size) { return SAlloc::M(size); }
void * operator new [](size_t size) { return SAlloc::M(size); }
void operator delete(void * pointer) { SAlloc::F(pointer); }
void operator delete [](void * pointer) { SAlloc::F(pointer); }
/*
 * DirectWrite font stream helpers
 */

// This is a font loader which provides only one font (unlike its original design).
// For a better implementation which was also source of this
// and DWriteFontFileStream, have a look at to NativeFontResourceDWrite.cpp in Mozilla
class DWriteFontFileLoader : public IDWriteFontFileLoader
{
private:
	IDWriteFontFileStream * mFontFileStream;
public:
	DWriteFontFileLoader (IDWriteFontFileStream * fontFileStream)
	{
		mFontFileStream = fontFileStream;
	}

	// IUnknown interface
	IFACEMETHOD(QueryInterface) (IID const& iid, OUT void** ppObject)
	{
		return S_OK;
	}
	IFACEMETHOD_(ULONG, AddRef) () {
		return 1;
	}
	IFACEMETHOD_(ULONG, Release) () { return 1; }
	// IDWriteFontFileLoader methods
	virtual HRESULT STDMETHODCALLTYPE CreateStreamFromKey(void const* fontFileReferenceKey,
	    uint32_t fontFileReferenceKeySize,
	    OUT IDWriteFontFileStream** fontFileStream)
	{
		*fontFileStream = mFontFileStream;
		return S_OK;
	}
	virtual ~DWriteFontFileLoader() 
	{
	}
};

class DWriteFontFileStream : public IDWriteFontFileStream {
private:
	uint8 * mData;
	uint32_t mSize;
public:
	DWriteFontFileStream (uint8 * aData, uint32_t aSize)
	{
		mData = aData;
		mSize = aSize;
	}

	// IUnknown interface
	IFACEMETHOD(QueryInterface) (IID const& iid, OUT void** ppObject)
	{
		return S_OK;
	}
	IFACEMETHOD_(ULONG, AddRef) () {
		return 1;
	}
	IFACEMETHOD_(ULONG, Release) () {
		return 1;
	}

	// IDWriteFontFileStream methods
	virtual HRESULT STDMETHODCALLTYPE ReadFileFragment(void const** fragmentStart,
	    UINT64 fileOffset,
	    UINT64 fragmentSize,
	    OUT void** fragmentContext)
	{
		// We are required to do bounds checking.
		if(fileOffset + fragmentSize > mSize) return E_FAIL;

		// truncate the 64 bit fileOffset to size_t sized index into mData
		size_t index = static_cast<size_t> (fileOffset);

		// We should be alive for the duration of this.
		*fragmentStart = &mData[index];
		*fragmentContext = nullptr;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE ReleaseFileFragment(void * fragmentContext) {
	}

	virtual HRESULT STDMETHODCALLTYPE GetFileSize(OUT UINT64* fileSize)
	{
		*fileSize = mSize;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetLastWriteTime(OUT UINT64* lastWriteTime) {
		return E_NOTIMPL;
	}

	virtual ~DWriteFontFileStream() {
	}
};

/*
 * shaper face data
 */

struct hb_directwrite_face_data_t {
	HMODULE dwrite_dll;
	IDWriteFactory * dwriteFactory;
	IDWriteFontFile * fontFile;
	DWriteFontFileStream * fontFileStream;
	DWriteFontFileLoader * fontFileLoader;
	IDWriteFontFace * fontFace;
	hb_blob_t * faceBlob;
};

hb_directwrite_face_data_t * _hb_directwrite_shaper_face_data_create(hb_face_t * face)
{
	hb_directwrite_face_data_t * data = new hb_directwrite_face_data_t;
	if(UNLIKELY(!data))
		return nullptr;

#define FAIL(...) \
	HB_STMT_START { \
		DEBUG_MSG(DIRECTWRITE, nullptr, __VA_ARGS__); \
		return nullptr; \
	} HB_STMT_END

	data->dwrite_dll = LoadLibrary(TEXT("DWRITE"));
	if(UNLIKELY(!data->dwrite_dll))
		FAIL("Cannot find DWrite.DLL");

	t_DWriteCreateFactory p_DWriteCreateFactory;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

	p_DWriteCreateFactory = (t_DWriteCreateFactory)
	    GetProcAddress(data->dwrite_dll, "DWriteCreateFactory");

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

	if(UNLIKELY(!p_DWriteCreateFactory))
		FAIL("Cannot find DWriteCreateFactory().");

	HRESULT hr;

	// TODO: factory and fontFileLoader should be cached separately
	IDWriteFactory* dwriteFactory;
	hr = p_DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
		(IUnknown**)&dwriteFactory);

	if(UNLIKELY(hr != S_OK))
		FAIL("Failed to run DWriteCreateFactory().");

	hb_blob_t * blob = hb_face_reference_blob(face);
	DWriteFontFileStream * fontFileStream;
	fontFileStream = new DWriteFontFileStream((uint8*)hb_blob_get_data(blob, nullptr),
		hb_blob_get_length(blob));

	DWriteFontFileLoader * fontFileLoader = new DWriteFontFileLoader(fontFileStream);
	dwriteFactory->RegisterFontFileLoader(fontFileLoader);

	IDWriteFontFile * fontFile;
	uint64_t fontFileKey = 0;
	hr = dwriteFactory->CreateCustomFontFileReference(&fontFileKey, sizeof(fontFileKey),
		fontFileLoader, &fontFile);

	if(FAILED(hr))
		FAIL("Failed to load font file from data!");

	BOOL isSupported;
	DWRITE_FONT_FILE_TYPE fileType;
	DWRITE_FONT_FACE_TYPE faceType;
	uint32_t numberOfFaces;
	hr = fontFile->Analyze(&isSupported, &fileType, &faceType, &numberOfFaces);
	if(FAILED(hr) || !isSupported)
		FAIL("Font file is not supported.");
#undef FAIL
	IDWriteFontFace * fontFace;
	dwriteFactory->CreateFontFace(faceType, 1, &fontFile, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontFace);
	data->dwriteFactory = dwriteFactory;
	data->fontFile = fontFile;
	data->fontFileStream = fontFileStream;
	data->fontFileLoader = fontFileLoader;
	data->fontFace = fontFace;
	data->faceBlob = blob;
	return data;
}

void _hb_directwrite_shaper_face_data_destroy(hb_directwrite_face_data_t * data)
{
	if(data->fontFace)
		data->fontFace->Release();
	if(data->fontFile)
		data->fontFile->Release();
	if(data->dwriteFactory) {
		if(data->fontFileLoader)
			data->dwriteFactory->UnregisterFontFileLoader(data->fontFileLoader);
		data->dwriteFactory->Release();
	}
	if(data->fontFileLoader)
		delete data->fontFileLoader;
	if(data->fontFileStream)
		delete data->fontFileStream;
	if(data->faceBlob)
		hb_blob_destroy(data->faceBlob);
	if(data->dwrite_dll)
		FreeLibrary(data->dwrite_dll);
	if(data)
		delete data;
}

/*
 * shaper font data
 */

struct hb_directwrite_font_data_t {};

hb_directwrite_font_data_t * _hb_directwrite_shaper_font_data_create(hb_font_t * font)
{
	hb_directwrite_font_data_t * data = new hb_directwrite_font_data_t;
	if(UNLIKELY(!data))
		return nullptr;

	return data;
}

void _hb_directwrite_shaper_font_data_destroy(hb_directwrite_font_data_t * data)
{
	delete data;
}

// Most of TextAnalysis is originally written by Bas Schouten for Mozilla project
// but now is relicensed to MIT for HarfBuzz use
class TextAnalysis : public IDWriteTextAnalysisSource, public IDWriteTextAnalysisSink
{
public:

	IFACEMETHOD(QueryInterface) (IID const& iid, OUT void** ppObject)
	{
		return S_OK;
	}
	IFACEMETHOD_(ULONG, AddRef) () {
		return 1;
	}
	IFACEMETHOD_(ULONG, Release) () {
		return 1;
	}

	// A single contiguous run of characters containing the same analysis
	// results.
	struct Run {
		uint32_t mTextStart; // starting text position of this run
		uint32_t mTextLength; // number of contiguous code units covered
		uint32_t mGlyphStart; // starting glyph in the glyphs array
		uint32_t mGlyphCount; // number of glyphs associated with this run
		// text
		DWRITE_SCRIPT_ANALYSIS mScript;
		uint8 mBidiLevel;
		bool mIsSideways;

		bool ContainsTextPosition(uint32_t aTextPosition) const
		{
			return aTextPosition >= mTextStart &&
			       aTextPosition <  mTextStart + mTextLength;
		}

		Run * nextRun;
	};

public:
	TextAnalysis (const wchar_t * text, uint32_t textLength,
	    const wchar_t * localeName, DWRITE_READING_DIRECTION readingDirection)
		: mTextLength(textLength), mText(text), mLocaleName(localeName),
		mReadingDirection(readingDirection), mCurrentRun(nullptr) {
	}

	~TextAnalysis ()
	{
		// delete runs, except mRunHead which is part of the TextAnalysis object
		for(Run * run = mRunHead.nextRun; run;) {
			Run * origRun = run;
			run = run->nextRun;
			delete origRun;
		}
	}

	STDMETHODIMP GenerateResults(IDWriteTextAnalyzer* textAnalyzer, Run ** runHead)
	{
		// Analyzes the text using the script analyzer and returns
		// the result as a series of runs.

		HRESULT hr = S_OK;

		// Initially start out with one result that covers the entire range.
		// This result will be subdivided by the analysis processes.
		mRunHead.mTextStart = 0;
		mRunHead.mTextLength = mTextLength;
		mRunHead.mBidiLevel =
		    (mReadingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
		mRunHead.nextRun = nullptr;
		mCurrentRun = &mRunHead;

		// Call each of the analyzers in sequence, recording their results.
		if(SUCCEEDED(hr = textAnalyzer->AnalyzeScript(this, 0, mTextLength, this)))
			*runHead = &mRunHead;

		return hr;
	}

	// IDWriteTextAnalysisSource implementation

	IFACEMETHODIMP GetTextAtPosition(uint32_t textPosition,
	    OUT wchar_t const** textString,
	    OUT uint32_t* textLength)
	{
		if(textPosition >= mTextLength) {
			// No text at this position, valid query though.
			*textString = nullptr;
			*textLength = 0;
		}
		else {
			*textString = mText + textPosition;
			*textLength = mTextLength - textPosition;
		}
		return S_OK;
	}

	IFACEMETHODIMP GetTextBeforePosition(uint32_t textPosition,
	    OUT wchar_t const** textString,
	    OUT uint32_t* textLength)
	{
		if(textPosition == 0 || textPosition > mTextLength) {
			// Either there is no text before here (== 0), or this
			// is an invalid position. The query is considered valid though.
			*textString = nullptr;
			*textLength = 0;
		}
		else {
			*textString = mText;
			*textLength = textPosition;
		}
		return S_OK;
	}

	IFACEMETHODIMP_(DWRITE_READING_DIRECTION)
	GetParagraphReadingDirection() {
		return mReadingDirection;
	}

	IFACEMETHODIMP GetLocaleName(uint32_t textPosition, uint32_t* textLength,
	    wchar_t const** localeName)
	{
		return S_OK;
	}

	IFACEMETHODIMP GetNumberSubstitution(uint32_t textPosition,
	    OUT uint32_t* textLength,
	    OUT IDWriteNumberSubstitution** numberSubstitution)
	{
		// We do not support number substitution.
		*numberSubstitution = nullptr;
		*textLength = mTextLength - textPosition;

		return S_OK;
	}

	// IDWriteTextAnalysisSink implementation

	IFACEMETHODIMP SetScriptAnalysis(uint32_t textPosition, uint32_t textLength,
	    DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis)
	{
		SetCurrentRun(textPosition);
		SplitCurrentRun(textPosition);
		while(textLength > 0) {
			Run * run = FetchNextRun(&textLength);
			run->mScript = *scriptAnalysis;
		}

		return S_OK;
	}

	IFACEMETHODIMP SetLineBreakpoints(uint32_t textPosition,
	    uint32_t textLength,
	    const DWRITE_LINE_BREAKPOINT* lineBreakpoints)
	{
		return S_OK;
	}

	IFACEMETHODIMP SetBidiLevel(uint32_t textPosition, uint32_t textLength,
	    uint8 explicitLevel, uint8 resolvedLevel)
	{
		return S_OK;
	}

	IFACEMETHODIMP SetNumberSubstitution(uint32_t textPosition, uint32_t textLength,
	    IDWriteNumberSubstitution* numberSubstitution)
	{
		return S_OK;
	}

protected:
	Run * FetchNextRun(IN OUT uint32_t* textLength)
	{
		// Used by the sink setters, this returns a reference to the next run.
		// Position and length are adjusted to now point after the current run
		// being returned.

		Run * origRun = mCurrentRun;
		// Split the tail if needed (the length remaining is less than the
		// current run's size).
		if(*textLength < mCurrentRun->mTextLength)
			SplitCurrentRun(mCurrentRun->mTextStart + *textLength);
		else
			// Just advance the current run.
			mCurrentRun = mCurrentRun->nextRun;
		*textLength -= origRun->mTextLength;

		// Return a reference to the run that was just current.
		return origRun;
	}

	void SetCurrentRun(uint32_t textPosition)
	{
		// Move the current run to the given position.
		// Since the analyzers generally return results in a forward manner,
		// this will usually just return early. If not, find the
		// corresponding run for the text position.

		if(mCurrentRun && mCurrentRun->ContainsTextPosition(textPosition))
			return;

		for(Run * run = &mRunHead; run; run = run->nextRun)
			if(run->ContainsTextPosition(textPosition)) {
				mCurrentRun = run;
				return;
			}
		assert(0); // We should always be able to find the text position in one of our runs
	}

	void SplitCurrentRun(uint32_t splitPosition)
	{
		if(!mCurrentRun) {
			assert(0); // SplitCurrentRun called without current run
			// Shouldn't be calling this when no current run is set!
			return;
		}
		// Split the current run.
		if(splitPosition <= mCurrentRun->mTextStart) {
			// No need to split, already the start of a run
			// or before it. Usually the first.
			return;
		}
		Run * newRun = new Run;

		*newRun = *mCurrentRun;

		// Insert the new run in our linked list.
		newRun->nextRun = mCurrentRun->nextRun;
		mCurrentRun->nextRun = newRun;

		// Adjust runs' text positions and lengths.
		uint32_t splitPoint = splitPosition - mCurrentRun->mTextStart;
		newRun->mTextStart += splitPoint;
		newRun->mTextLength -= splitPoint;
		mCurrentRun->mTextLength = splitPoint;
		mCurrentRun = newRun;
	}

protected:
	// Input
	// (weak references are fine here, since this class is a transient
	//  stack-based helper that doesn't need to copy data)
	uint32_t mTextLength;
	const wchar_t * mText;
	const wchar_t * mLocaleName;
	DWRITE_READING_DIRECTION mReadingDirection;

	// Current processing state.
	Run * mCurrentRun;

	// Output is a list of runs starting here
	Run mRunHead;
};

/*
 * shaper
 */
static hb_bool_t _hb_directwrite_shape_full(hb_shape_plan_t * shape_plan,
    hb_font_t * font,
    hb_buffer_t * buffer,
    const hb_feature_t * features,
    uint num_features,
    float lineWidth)
{
	hb_face_t * face = font->face;
	const hb_directwrite_face_data_t * face_data = face->data.directwrite;
	IDWriteFactory * dwriteFactory = face_data->dwriteFactory;
	IDWriteFontFace * fontFace = face_data->fontFace;

	IDWriteTextAnalyzer* analyzer;
	dwriteFactory->CreateTextAnalyzer(&analyzer);

	uint scratch_size;
	hb_buffer_t::scratch_buffer_t * scratch = buffer->get_scratch_buffer(&scratch_size);
#define ALLOCATE_ARRAY(Type, name, len) \
	Type *name = (Type*)scratch; \
	do { \
		uint _consumed = idivroundup((len) * sizeof(Type), sizeof(*scratch)); \
		assert(_consumed <= scratch_size); \
		scratch += _consumed; \
		scratch_size -= _consumed; \
	} while(0)

#define utf16_index() var1.u32

	ALLOCATE_ARRAY(wchar_t, textString, buffer->len * 2);

	uint chars_len = 0;
	for(uint i = 0; i < buffer->len; i++) {
		hb_codepoint_t c = buffer->info[i].codepoint;
		buffer->info[i].utf16_index() = chars_len;
		if(LIKELY(c <= 0xFFFFu))
			textString[chars_len++] = c;
		else if(UNLIKELY(c > 0x10FFFFu))
			textString[chars_len++] = 0xFFFDu;
		else {
			textString[chars_len++] = 0xD800u + ((c - 0x10000u) >> 10);
			textString[chars_len++] = 0xDC00u + ((c - 0x10000u) & ((1u << 10) - 1));
		}
	}

	ALLOCATE_ARRAY(WORD, log_clusters, chars_len);
	/* Need log_clusters to assign features. */
	chars_len = 0;
	for(uint i = 0; i < buffer->len; i++) {
		hb_codepoint_t c = buffer->info[i].codepoint;
		uint cluster = buffer->info[i].cluster;
		log_clusters[chars_len++] = cluster;
		if(hb_in_range(c, 0x10000u, 0x10FFFFu))
			log_clusters[chars_len++] = cluster; /* Surrogates. */
	}

	// TODO: Handle TEST_DISABLE_OPTIONAL_LIGATURES

	DWRITE_READING_DIRECTION readingDirection;
	readingDirection = buffer->props.direction ?
	    DWRITE_READING_DIRECTION_RIGHT_TO_LEFT :
	    DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;

	/*
	 * There's an internal 16-bit limit on some things inside the analyzer,
	 * but we never attempt to shape a word longer than 64K characters
	 * in a single gfxShapedWord, so we cannot exceed that limit.
	 */
	uint32_t textLength = buffer->len;

	TextAnalysis analysis(textString, textLength, nullptr, readingDirection);
	TextAnalysis::Run * runHead;
	HRESULT hr;
	hr = analysis.GenerateResults(analyzer, &runHead);

#define FAIL(...) \
	HB_STMT_START { \
		DEBUG_MSG(DIRECTWRITE, nullptr, __VA_ARGS__); \
		return false; \
	} HB_STMT_END

	if(FAILED(hr))
		FAIL("Analyzer failed to generate results.");

	uint32_t maxGlyphCount = 3 * textLength / 2 + 16;
	uint32_t glyphCount;
	bool isRightToLeft = HB_DIRECTION_IS_BACKWARD(buffer->props.direction);

	const wchar_t localeName[20] = {0};
	if(buffer->props.language)
		mbstowcs((wchar_t *)localeName,
		    hb_language_to_string(buffer->props.language), 20);

	// TODO: it does work but doesn't care about ranges
	DWRITE_TYPOGRAPHIC_FEATURES typographic_features;
	typographic_features.featureCount = num_features;
	if(num_features) {
		typographic_features.features = new DWRITE_FONT_FEATURE[num_features];
		for(uint i = 0; i < num_features; ++i) {
			typographic_features.features[i].nameTag = (DWRITE_FONT_FEATURE_TAG)
			    hb_uint32_swap(features[i].tag);
			typographic_features.features[i].parameter = features[i].value;
		}
	}
	const DWRITE_TYPOGRAPHIC_FEATURES* dwFeatures;
	dwFeatures = (const DWRITE_TYPOGRAPHIC_FEATURES*)&typographic_features;
	const uint32_t featureRangeLengths[] = { textLength };
	//

	uint16_t* clusterMap;
	clusterMap = new uint16_t[textLength];
	DWRITE_SHAPING_TEXT_PROPERTIES* textProperties;
	textProperties = new DWRITE_SHAPING_TEXT_PROPERTIES[textLength];
retry_getglyphs:
	uint16_t* glyphIndices = new uint16_t[maxGlyphCount];
	DWRITE_SHAPING_GLYPH_PROPERTIES* glyphProperties;
	glyphProperties = new DWRITE_SHAPING_GLYPH_PROPERTIES[maxGlyphCount];

	hr = analyzer->GetGlyphs(textString, textLength, fontFace, false,
		isRightToLeft, &runHead->mScript, localeName,
		nullptr, &dwFeatures, featureRangeLengths, 1,
		maxGlyphCount, clusterMap, textProperties,
		glyphIndices, glyphProperties, &glyphCount);

	if(UNLIKELY(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))) {
		delete [] glyphIndices;
		delete [] glyphProperties;

		maxGlyphCount *= 2;

		goto retry_getglyphs;
	}
	if(FAILED(hr))
		FAIL("Analyzer failed to get glyphs.");

	float* glyphAdvances = new float[maxGlyphCount];
	DWRITE_GLYPH_OFFSET* glyphOffsets = new DWRITE_GLYPH_OFFSET[maxGlyphCount];

	/* The -2 in the following is to compensate for possible
	 * alignment needed after the WORD array.  sizeof (WORD) == 2. */
	uint glyphs_size = (scratch_size * sizeof(int) - 2)
	    / (sizeof(WORD) +
	    sizeof(DWRITE_SHAPING_GLYPH_PROPERTIES) +
	    sizeof(int) +
	    sizeof(DWRITE_GLYPH_OFFSET) +
	    sizeof(uint32_t));
	ALLOCATE_ARRAY(uint32_t, vis_clusters, glyphs_size);

#undef ALLOCATE_ARRAY

	int fontEmSize = font->face->get_upem();
	if(fontEmSize < 0) fontEmSize = -fontEmSize;

	if(fontEmSize < 0) fontEmSize = -fontEmSize;
	double x_mult = (double)font->x_scale / fontEmSize;
	double y_mult = (double)font->y_scale / fontEmSize;

	hr = analyzer->GetGlyphPlacements(textString, clusterMap, textProperties,
		textLength, glyphIndices, glyphProperties,
		glyphCount, fontFace, fontEmSize,
		false, isRightToLeft, &runHead->mScript, localeName,
		&dwFeatures, featureRangeLengths, 1,
		glyphAdvances, glyphOffsets);

	if(FAILED(hr))
		FAIL("Analyzer failed to get glyph placements.");

	IDWriteTextAnalyzer1* analyzer1;
	analyzer->QueryInterface(&analyzer1);

	if(analyzer1 && lineWidth) {
		DWRITE_JUSTIFICATION_OPPORTUNITY* justificationOpportunities =
		    new DWRITE_JUSTIFICATION_OPPORTUNITY[maxGlyphCount];
		hr = analyzer1->GetJustificationOpportunities(fontFace, fontEmSize, runHead->mScript,
			textLength, glyphCount, textString,
			clusterMap, glyphProperties,
			justificationOpportunities);

		if(FAILED(hr))
			FAIL("Analyzer failed to get justification opportunities.");

		float* justifiedGlyphAdvances = new float[maxGlyphCount];
		DWRITE_GLYPH_OFFSET* justifiedGlyphOffsets = new DWRITE_GLYPH_OFFSET[glyphCount];
		hr = analyzer1->JustifyGlyphAdvances(lineWidth, glyphCount, justificationOpportunities,
			glyphAdvances, glyphOffsets, justifiedGlyphAdvances,
			justifiedGlyphOffsets);

		if(FAILED(hr)) FAIL("Analyzer failed to get justify glyph advances.");

		DWRITE_SCRIPT_PROPERTIES scriptProperties;
		hr = analyzer1->GetScriptProperties(runHead->mScript, &scriptProperties);
		if(FAILED(hr)) FAIL("Analyzer failed to get script properties.");
		uint32_t justificationCharacter = scriptProperties.justificationCharacter;

		// if a script justificationCharacter is not space, it can have GetJustifiedGlyphs
		if(justificationCharacter != 32) {
			uint16_t* modifiedClusterMap = new uint16_t[textLength];
retry_getjustifiedglyphs:
			uint16_t* modifiedGlyphIndices = new uint16_t[maxGlyphCount];
			float* modifiedGlyphAdvances = new float[maxGlyphCount];
			DWRITE_GLYPH_OFFSET* modifiedGlyphOffsets = new DWRITE_GLYPH_OFFSET[maxGlyphCount];
			uint32_t actualGlyphsCount;
			hr = analyzer1->GetJustifiedGlyphs(fontFace, fontEmSize, runHead->mScript,
				textLength, glyphCount, maxGlyphCount,
				clusterMap, glyphIndices, glyphAdvances,
				justifiedGlyphAdvances, justifiedGlyphOffsets,
				glyphProperties, &actualGlyphsCount,
				modifiedClusterMap, modifiedGlyphIndices,
				modifiedGlyphAdvances, modifiedGlyphOffsets);

			if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
				maxGlyphCount = actualGlyphsCount;
				delete [] modifiedGlyphIndices;
				delete [] modifiedGlyphAdvances;
				delete [] modifiedGlyphOffsets;

				maxGlyphCount = actualGlyphsCount;

				goto retry_getjustifiedglyphs;
			}
			if(FAILED(hr))
				FAIL("Analyzer failed to get justified glyphs.");

			delete [] clusterMap;
			delete [] glyphIndices;
			delete [] glyphAdvances;
			delete [] glyphOffsets;

			glyphCount = actualGlyphsCount;
			clusterMap = modifiedClusterMap;
			glyphIndices = modifiedGlyphIndices;
			glyphAdvances = modifiedGlyphAdvances;
			glyphOffsets = modifiedGlyphOffsets;

			delete [] justifiedGlyphAdvances;
			delete [] justifiedGlyphOffsets;
		}
		else {
			delete [] glyphAdvances;
			delete [] glyphOffsets;
			glyphAdvances = justifiedGlyphAdvances;
			glyphOffsets = justifiedGlyphOffsets;
		}
		delete [] justificationOpportunities;
	}
	/* Ok, we've got everything we need, now compose output buffer,
	 * very, *very*, carefully! */
	/* Calculate visual-clusters.  That's what we ship. */
	for(uint i = 0; i < glyphCount; i++)
		vis_clusters[i] = (uint32_t)-1;
	for(uint i = 0; i < buffer->len; i++) {
		uint32_t * p = &vis_clusters[log_clusters[buffer->info[i].utf16_index()]];
		*p = hb_min(*p, buffer->info[i].cluster);
	}
	for(uint i = 1; i < glyphCount; i++)
		if(vis_clusters[i] == (uint32_t)-1)
			vis_clusters[i] = vis_clusters[i - 1];
#undef utf16_index
	if(UNLIKELY(!buffer->ensure(glyphCount)))
		FAIL("Buffer in error");
#undef FAIL
	/* Set glyph infos */
	buffer->len = 0;
	for(uint i = 0; i < glyphCount; i++) {
		hb_glyph_info_t * info = &buffer->info[buffer->len++];
		info->codepoint = glyphIndices[i];
		info->cluster = vis_clusters[i];
		/* The rest is crap.  Let's store position info there for now. */
		info->mask = glyphAdvances[i];
		info->var1.i32 = glyphOffsets[i].advanceOffset;
		info->var2.i32 = glyphOffsets[i].ascenderOffset;
	}
	/* Set glyph positions */
	buffer->clear_positions();
	for(uint i = 0; i < glyphCount; i++) {
		hb_glyph_info_t * info = &buffer->info[i];
		hb_glyph_position_t * pos = &buffer->pos[i];
		/* TODO vertical */
		pos->x_advance = x_mult * (int32_t)info->mask;
		pos->x_offset = x_mult * (isRightToLeft ? -info->var1.i32 : info->var1.i32);
		pos->y_offset = y_mult * info->var2.i32;
	}
	if(isRightToLeft) 
		hb_buffer_reverse(buffer);
	delete [] clusterMap;
	delete [] glyphIndices;
	delete [] textProperties;
	delete [] glyphProperties;
	delete [] glyphAdvances;
	delete [] glyphOffsets;
	if(num_features)
		delete [] typographic_features.features;
	/* Wow, done! */
	return true;
}

hb_bool_t _hb_directwrite_shape(hb_shape_plan_t * shape_plan,
    hb_font_t * font,
    hb_buffer_t * buffer,
    const hb_feature_t * features,
    uint num_features)
{
	return _hb_directwrite_shape_full(shape_plan, font, buffer,
		   features, num_features, 0);
}

CXX_UNUSED_PARAM static bool _hb_directwrite_shape_experimental_width(hb_font_t * font,
    hb_buffer_t * buffer,
    const hb_feature_t * features,
    uint num_features,
    float width)
{
	static const char * shapers = "directwrite";
	hb_shape_plan_t * shape_plan;
	shape_plan = hb_shape_plan_create_cached(font->face, &buffer->props,
		features, num_features, &shapers);
	hb_bool_t res = _hb_directwrite_shape_full(shape_plan, font, buffer,
		features, num_features, width);

	buffer->unsafe_to_break_all();

	return res;
}

struct _hb_directwrite_font_table_context {
	IDWriteFontFace * face;
	void * table_context;
};

static void _hb_directwrite_table_data_release(void * data)
{
	_hb_directwrite_font_table_context * context = (_hb_directwrite_font_table_context*)data;
	context->face->ReleaseFontTable(context->table_context);
	delete context;
}

static hb_blob_t * _hb_directwrite_reference_table(hb_face_t * face CXX_UNUSED_PARAM, hb_tag_t tag, void * user_data)
{
	IDWriteFontFace * dw_face = ((IDWriteFontFace*)user_data);
	const void * data;
	uint32_t length;
	void * table_context;
	BOOL exists;
	if(!dw_face || FAILED(dw_face->TryGetFontTable(hb_uint32_swap(tag), &data,
	    &length, &table_context, &exists)))
		return nullptr;

	if(!data || !exists || !length) {
		dw_face->ReleaseFontTable(table_context);
		return nullptr;
	}
	_hb_directwrite_font_table_context * context = new _hb_directwrite_font_table_context;
	context->face = dw_face;
	context->table_context = table_context;
	return hb_blob_create((const char *)data, length, HB_MEMORY_MODE_READONLY, context, _hb_directwrite_table_data_release);
}

static void _hb_directwrite_font_release(void * data)
{
	if(data)
		((IDWriteFontFace*)data)->Release();
}

/**
 * hb_directwrite_face_create:
 * @font_face: a DirectWrite IDWriteFontFace object.
 *
 * Return value: #hb_face_t object corresponding to the given input
 *
 * Since: 2.4.0
 **/
hb_face_t * hb_directwrite_face_create(IDWriteFontFace * font_face)
{
	if(font_face)
		font_face->AddRef();
	return hb_face_create_for_tables(_hb_directwrite_reference_table, font_face,
		   _hb_directwrite_font_release);
}

/**
 * hb_directwrite_face_get_font_face:
 * @face: a #hb_face_t object
 *
 * Return value: DirectWrite IDWriteFontFace object corresponding to the given input
 *
 * Since: 2.5.0
 **/
IDWriteFontFace * hb_directwrite_face_get_font_face(hb_face_t * face)
{
	return face->data.directwrite->fontFace;
}

#endif
