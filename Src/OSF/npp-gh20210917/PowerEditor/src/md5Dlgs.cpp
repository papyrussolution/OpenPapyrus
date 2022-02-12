// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>
// @licence GNU GPL
//
#include <npp-internal.h>
#pragma hdrstop
//
//
//
//#include "md5.h"
//
// Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights reserved.
//
// License to copy and use this software is granted provided that it
// is identified as the "RSA Data Security, Inc. MD5 Message-Digest
// Algorithm" in all material mentioning or referencing this software
// or this function.
//
// License is also granted to make and use derivative works provided
// that such works are identified as "derived from the RSA Data
// Security, Inc. MD5 Message-Digest Algorithm" in all material
// mentioning or referencing the derived work.
//
// RSA Data Security, Inc. makes no representations concerning either
// the merchantability of this software or the suitability of this
// software for any particular purpose. It is provided "as is"
// without express or implied warranty of any kind.
//
// These notices must be retained in any copies of any part of this
// documentation and/or software.

// The original md5 implementation avoids external libraries.
// This version has dependency on stdio.h for file input and
// string.h for memcpy.

//
// http://www.ietf.org/ietf-ftp/IPR/RSA-MD-all

#pragma region MD5 defines
// Constants for MD5Transform routine.
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static uchar PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// F, G, H and I are basic MD5 functions.
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

// ROTATE_LEFT rotates x left n bits.
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
#define FF(a, b, c, d, x, s, ac) { (a) += F((b), (c), (d)) + (x) + (uint32)(ac); (a) = ROTATE_LEFT((a), (s)); (a) += (b); }
#define GG(a, b, c, d, x, s, ac) { (a) += G((b), (c), (d)) + (x) + (uint32)(ac); (a) = ROTATE_LEFT((a), (s)); (a) += (b); }
#define HH(a, b, c, d, x, s, ac) { (a) += H((b), (c), (d)) + (x) + (uint32)(ac); (a) = ROTATE_LEFT((a), (s)); (a) += (b); }
#define II(a, b, c, d, x, s, ac) { (a) += I((b), (c), (d)) + (x) + (uint32)(ac); (a) = ROTATE_LEFT((a), (s)); (a) += (b); }
#pragma endregion

//typedef uchar BYTE;
//typedef uchar * POINTER; // POINTER defines a generic pointer type
//typedef unsigned short int UINT2; // UINT2 defines a two byte word
//typedef unsigned long int UINT4; // UINT4 defines a four byte word

// convenient object that wraps
// the C-functions for use in C++ only
#if 0 // {
class MD5 {
private:
	struct __context_t {
		uint32 state[4]; /* state (ABCD) */
		uint32 count[2]; /* number of bits, modulo 2^64 (lsb first) */
		uchar buffer[64]; /* input buffer */
	} context;

	#pragma region static helper functions
	// The core of the MD5 algorithm is here.
	// MD5 basic transformation. Transforms state based on block.
	static void MD5Transform(uint32 state[4], uchar block[64])
	{
		uint32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];
		Decode(x, block, 64);
		/* Round 1 */
		FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
		FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
		FF(c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
		FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
		FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
		FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
		FF(c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
		FF(b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
		FF(a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
		FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
		FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
		FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
		FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
		FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
		FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
		FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

		/* Round 2 */
		GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
		GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
		GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
		GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
		GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
		GG(d, a, b, c, x[10], S22,  0x2441453); /* 22 */
		GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
		GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
		GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
		GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
		GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
		GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
		GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
		GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
		GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
		GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

		/* Round 3 */
		HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
		HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
		HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
		HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
		HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
		HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
		HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
		HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
		HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
		HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
		HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
		HH(b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
		HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
		HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
		HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
		HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

		/* Round 4 */
		II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
		II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
		II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
		II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
		II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
		II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
		II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
		II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
		II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
		II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
		II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
		II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
		II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
		II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
		II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
		II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;

		// Zeroize sensitive information.
		memzero((uint8 *)x, sizeof(x));
	}

	// Encodes input (uint32) into output (uchar). Assumes len is
	// a multiple of 4.
	static void Encode(uchar * output, uint32 * input, uint len)
	{
		for(uint i = 0, j = 0; j < len; i++, j += 4) {
			output[j] = (uchar)(input[i] & 0xff);
			output[j+1] = (uchar)((input[i] >> 8) & 0xff);
			output[j+2] = (uchar)((input[i] >> 16) & 0xff);
			output[j+3] = (uchar)((input[i] >> 24) & 0xff);
		}
	}
	// Decodes input (uchar) into output (uint32). Assumes len is
	// a multiple of 4.
	static void Decode(uint32 * output, uchar * input, uint len)
	{
		for(uint i = 0, j = 0; j < len; i++, j += 4)
			output[i] = ((uint32)input[j]) | (((uint32)input[j+1]) << 8) | (((uint32)input[j+2]) << 16) | (((uint32)input[j+3]) << 24);
	}
	#pragma endregion
public:
	// MAIN FUNCTIONS
	MD5()
	{
		Init();
	}
	// MD5 initialization. Begins an MD5 operation, writing a new context.
	void Init()
	{
		context.count[0] = context.count[1] = 0;
		// Load magic initialization constants.
		context.state[0] = 0x67452301;
		context.state[1] = 0xefcdab89;
		context.state[2] = 0x98badcfe;
		context.state[3] = 0x10325476;
	}
	// MD5 block update operation. Continues an MD5 message-digest
	// operation, processing another message block, and updating the
	// context.
	void Update(uchar * input/*input block*/, uint inputLen/*length of input block*/)
	{
		uint i, partLen;
		// Compute number of bytes mod 64
		uint index = (uint)((context.count[0] >> 3) & 0x3F);
		// Update number of bits
		if((context.count[0] += ((uint32)inputLen << 3)) < ((uint32)inputLen << 3))
			context.count[1]++;
		context.count[1] += ((uint32)inputLen >> 29);
		partLen = 64 - index;
		// Transform as many times as possible.
		if(inputLen >= partLen) {
			memcpy((uint8 *)&context.buffer[index], (uint8 *)input, partLen);
			MD5Transform(context.state, context.buffer);
			for(i = partLen; i + 63 < inputLen; i += 64)
				MD5Transform(context.state, &input[i]);
			index = 0;
		}
		else
			i = 0;
		// Buffer remaining input 
		memcpy((uint8 *)&context.buffer[index], (uint8 *)&input[i], inputLen-i);
	}
	// MD5 finalization. Ends an MD5 message-digest operation, writing the
	// the message digest and zeroizing the context.
	// Writes to digestRaw
	void Final()
	{
		uchar bits[8];
		uint index, padLen;
		// Save number of bits
		Encode(bits, context.count, 8);
		// Pad out to 56 mod 64.
		index = (uint)((context.count[0] >> 3) & 0x3f);
		padLen = (index < 56) ? (56 - index) : (120 - index);
		Update(PADDING, padLen);
		// Append length (before padding)
		Update(bits, 8);
		// Store state in digest
		Encode(digestRaw, context.state, 16);
		// Zeroize sensitive information.
		memzero((uint8 *)&context, sizeof(context));
		writeToString();
	}
	/// Buffer must be 32+1 (nul) = 33 chars long at least
	void writeToString()
	{
		for(int pos = 0; pos < 16; pos++)
			sprintf(digestChars+(pos*2), "%02x", digestRaw[pos]);
	}
public:
	// an MD5 digest is a 16-byte number (32 hex digits)
	BYTE digestRaw[ 16 ];
	// This version of the digest is actually
	// a "printf'd" version of the digest.
	char digestChars[ 33 ];
	/// Load a file from disk and digest it
	// Digests a file and returns the result.
	char * digestFile(const char * filename)
	{
		Init();
		int len;
		uchar buffer[1024];
		FILE * file = fopen(filename, "rb");
		if(file == NULL) {
			//printf("%s can't be opened\n", filename);
			return NULL;
		}
		else {
			while((len = static_cast<int>(fread(buffer, 1, 1024, file)) ) != 0)
				Update(buffer, len);
			Final();
			fclose(file);
		}
		return digestChars;
	}
	/// Digests a byte-array already in memory
	char * digestMemory(BYTE * memchunk, int len)
	{
		Init();
		Update(memchunk, len);
		Final();
		return digestChars;
	}
	// Digests a string and prints the result.
	char * digestString(const char * string)
	{
		Init();
		Update( (uchar *)string, static_cast<uint>(strlen(string)));
		Final();
		return digestChars;
	}
};
#endif // } 0
//
//
//
static SString & DigestFile_MD5(const char * filename, SString & rBuf)
{
	rBuf.Z();
	int len;
	uchar buffer[1024];
	FILE * file = fopen(filename, "rb");
	if(file) {
		SlHash::State hs;
		SlHash::Md5(&hs, 0, 0);
		while((len = static_cast<int>(fread(buffer, 1, 1024, file)) ) != 0) {
			//Update(buffer, len);
			SlHash::Md5(&hs, buffer, len);
		}
		binary128 h = SlHash::Md5(&hs, 0, 0);
		//Final();
		fclose(file);
		{
			for(int pos = 0; pos < sizeof(h); pos++) {
				//sprintf(pOutBuf+(pos*2), "%02x", PTR8(&h)[pos]);
				rBuf.CatHexUpper(PTR8(&h)[pos]);
			}
		}
	}
	else {
		//printf("%s can't be opened\n", filename);
	}
	return rBuf;
}


/*static*/LRESULT CALLBACK HashFromFilesDlg::HashPathEditStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	const auto dlg = (HashFromFilesDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	return (run_textEditProc(dlg->_oldHashPathEditProc, hwnd, message, wParam, lParam));
}

/*static*/LRESULT CALLBACK HashFromFilesDlg::HashResultStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	const auto dlg = (HashFromFilesDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	return (run_textEditProc(dlg->_oldHashResultProc, hwnd, message, wParam, lParam));
}

INT_PTR CALLBACK HashFromFilesDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
			{
				NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
				int fontDpiDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(13);
				HFONT hFont = ::CreateFontA(fontDpiDynamicalHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
					OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Courier New");
				const HWND hHashPathEdit = ::GetDlgItem(_hSelf, IDC_HASH_PATH_EDIT);
				const HWND hHashResult = ::GetDlgItem(_hSelf, IDC_HASH_RESULT_EDIT);
				::SendMessage(hHashPathEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
				::SendMessage(hHashResult, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
				::SetWindowLongPtr(hHashPathEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				_oldHashPathEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashPathEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashPathEditStaticProc)));
				::SetWindowLongPtr(hHashResult, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				_oldHashResultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashResult, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashResultStaticProc)));
			}
		    return TRUE;
		case WM_CTLCOLORDLG:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORSTATIC:
		    if(NppDarkMode::isEnabled()) {
			    HWND hwnd = reinterpret_cast<HWND>(lParam);
			    if(hwnd == ::GetDlgItem(_hSelf, IDC_HASH_PATH_EDIT) || hwnd == ::GetDlgItem(_hSelf, IDC_HASH_RESULT_EDIT)) {
				    return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			    }
			    else {
				    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			    }
		    }
		    break;
		case NPPM_INTERNAL_REFRESHDARKMODE:
		    NppDarkMode::autoThemeChildControls(_hSelf);
		    return TRUE;
		case WM_COMMAND:
	    {
		    switch(wParam) {
			    case IDCANCEL: display(false); return TRUE;
			    case IDOK: return TRUE;
			    case IDC_HASH_FILEBROWSER_BUTTON:
			{
				CustomFileDialog fDlg(_hSelf);
				fDlg.setExtFilter(TEXT("All types"), TEXT(".*"));
				const auto& fns = fDlg.doOpenMultiFilesDlg();
				if(!fns.empty()) {
					SString digest_buf;
					std::wstring files2check, hashResultStr;
					for(const auto & it : fns) {
						if(_ht == hashType::hash_md5) {
							WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
							const char * path = wmc.wchar2char(it.c_str(), CP_ACP);
							//MD5 md5;
							//char * md5Result = md5.digestFile(path);
							DigestFile_MD5(path, digest_buf);
							//if(md5Result) {
							if(digest_buf.NotEmpty()) {
								files2check += it;
								files2check += TEXT("\r\n");
								wchar_t * fileName = ::PathFindFileName(it.c_str());
								hashResultStr += wmc.char2wchar(/*md5Result*/digest_buf, CP_ACP);
								hashResultStr += TEXT("  ");
								hashResultStr += fileName;
								hashResultStr += TEXT("\r\n");
							}
						}
						else if(_ht == hashType::hash_sha256) {
							std::string content = getFileContent(it.c_str());
							//uint8_t sha2hash[32];
							//calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());
							binary256 __h = SlHash::Sha256(0, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());
							wchar_t sha2hashStr[65] = { '\0' };
							for(size_t i = 0; i < sizeof(__h); i++)
								wsprintf(sha2hashStr + i * 2, TEXT("%02x"), /*sha2hash*/PTR8C(&__h)[i]);
							files2check += it;
							files2check += TEXT("\r\n");
							wchar_t * fileName = ::PathFindFileName(it.c_str());
							hashResultStr += sha2hashStr;
							hashResultStr += TEXT("  ");
							hashResultStr += fileName;
							hashResultStr += TEXT("\r\n");
						}
						else {
							// unknown
						}
					}
					if(!files2check.empty() && !hashResultStr.empty()) {
						::SetDlgItemText(_hSelf, IDC_HASH_PATH_EDIT, files2check.c_str());
						::SetDlgItemText(_hSelf, IDC_HASH_RESULT_EDIT, hashResultStr.c_str());
					}
				}
			}
				return TRUE;
			    case IDC_HASH_TOCLIPBOARD_BUTTON:
					{
						int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_RESULT_EDIT), WM_GETTEXTLENGTH, 0, 0));
						if(len) {
							wchar_t * rStr = new wchar_t[len+1];
							::GetDlgItemText(_hSelf, IDC_HASH_RESULT_EDIT, rStr, len + 1);
							str2Clipboard(rStr, _hSelf);
							delete[] rStr;
						}
					}
					return TRUE;
			    default:
					break;
		    }
	    }
	}
	return FALSE;
}

LRESULT run_textEditProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
		case WM_GETDLGCODE:
		    return DLGC_WANTALLKEYS | ::CallWindowProc(oldEditProc, hwnd, message, wParam, lParam);
		case WM_CHAR:
		    if(wParam == 1) {    // Ctrl+A
			    ::SendMessage(hwnd, EM_SETSEL, 0, -1);
			    return TRUE;
		    }
		    break;
		default:
		    break;
	}
	return ::CallWindowProc(oldEditProc, hwnd, message, wParam, lParam);
}

void HashFromFilesDlg::setHashType(hashType hashType2set)
{
	_ht = hashType2set;
}

void HashFromFilesDlg::doDialog(bool isRTL)
{
	if(!isCreated()) {
		create(IDD_HASHFROMFILES_DLG, isRTL);
		if(_ht == hash_sha256) {
			generic_string title = TEXT("Generate SHA-256 digest from files");
			::SetWindowText(_hSelf, title.c_str());
			generic_string buttonText = TEXT("Choose files to generate SHA-256...");
			::SetDlgItemText(_hSelf, IDC_HASH_FILEBROWSER_BUTTON, buttonText.c_str());
		}
	}
	// Adjust the position in the center
	goToCenter();
};

void HashFromTextDlg::generateHash()
{
	if(oneof2(_ht, hash_md5, hash_sha256)) {
		int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
		if(len) {
			// it's important to get text from UNICODE then convert it to UTF8
			// So we get the result of UTF8 text (tested with Chinese).
			wchar_t * text = new wchar_t[len + 1];
			::GetDlgItemText(_hSelf, IDC_HASH_TEXT_EDIT, text, len + 1);
			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			const char * newText = wmc.wchar2char(text, SC_CP_UTF8);
			if(_ht == hash_md5) {
				//MD5 md5;
				//char * md5Result = md5.digestString(newText);
				SString __md5_buf;
				binary128 __md5 = SlHash::Md5(0, newText, sstrlen(newText));
				__md5_buf.CatHex(&__md5, sizeof(__md5));
				::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, /*md5Result*/__md5_buf.cptr());
			}
			else if(_ht == hash_sha256) {
				//uint8_t sha2hash[32];
				//calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
				binary256 __h = SlHash::Sha256(0, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
				wchar_t sha2hashStr[65] = { '\0' };
				for(size_t i = 0; i < sizeof(__h); i++)
					wsprintf(sha2hashStr + i * 2, TEXT("%02x"), /*sha2hash*/PTR8C(&__h)[i]);
				::SetDlgItemText(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, sha2hashStr);
			}
			delete[] text;
		}
		else
			::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, "");
	}
}

void HashFromTextDlg::generateHashPerLine()
{
	int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
	if(len) {
		wchar_t * text = new wchar_t[len + 1];
		::GetDlgItemText(_hSelf, IDC_HASH_TEXT_EDIT, text, len + 1);
		std::wstringstream ss(text);
		std::wstring aLine;
		std::string result;
		//MD5 md5;
		SString __md5_buf;
		WcharMbcsConvertor & wmc = WcharMbcsConvertor::getInstance();
		while(std::getline(ss, aLine)) {
			// getline() detect only '\n' but not "\r\n" under windows
			// this hack is to walk around such bug
			if(aLine.back() == '\r')
				aLine = aLine.substr(0, aLine.size() - 1);
			if(aLine.empty())
				result += "\r\n";
			else {
				const char * newText = wmc.wchar2char(aLine.c_str(), SC_CP_UTF8);
				if(_ht == hash_md5) {
					//char * md5Result = md5.digestString(newText);
					//result += md5Result;
					//result += "\r\n";
					binary128 __md5 = SlHash::Md5(0, newText, sstrlen(newText));
					__md5_buf.Z().CatHex(&__md5, sizeof(__md5)).CRB();
					result += __md5_buf.cptr();
				}
				else if(_ht == hash_sha256) {
					//uint8_t sha2hash[32];
					//calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
					binary256 __h = SlHash::Sha256(0, reinterpret_cast<const uint8_t*>(newText), strlen(newText));
					char sha2hashStr[65] = { '\0' };
					for(size_t i = 0; i < sizeof(__h); i++)
						sprintf(sha2hashStr + i * 2, "%02x", /*sha2hash*/PTR8C(&__h)[i]);
					result += sha2hashStr;
					result += "\r\n";
				}
			}
		}
		delete[] text;
		::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, result.c_str());
	}
	else {
		::SetDlgItemTextA(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, "");
	}
}

/*static*/LRESULT CALLBACK HashFromTextDlg::HashTextEditStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	const auto dlg = (HashFromTextDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	return (run_textEditProc(dlg->_oldHashTextEditProc, hwnd, message, wParam, lParam));
}

/*static*/LRESULT CALLBACK HashFromTextDlg::HashResultStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	const auto dlg = (HashFromTextDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	return (run_textEditProc(dlg->_oldHashResultProc, hwnd, message, wParam, lParam));
}

INT_PTR CALLBACK HashFromTextDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
	    {
		    NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
		    int fontDpiDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(13);
		    HFONT hFont = ::CreateFontA(fontDpiDynamicalHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
			    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			    DEFAULT_PITCH | FF_DONTCARE, "Courier New");

		    const HWND hHashTextEdit = ::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT);
		    const HWND hHashResult = ::GetDlgItem(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT);

		    ::SendMessage(hHashTextEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
		    ::SendMessage(hHashResult, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

		    ::SetWindowLongPtr(hHashTextEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		    _oldHashTextEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashTextEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashTextEditStaticProc)));
		    ::SetWindowLongPtr(hHashResult, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		    _oldHashResultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hHashResult, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HashResultStaticProc)));
	    }
		    return TRUE;

		case WM_CTLCOLOREDIT:
		    if(NppDarkMode::isEnabled()) {
			    HWND hwnd = reinterpret_cast<HWND>(lParam);
			    if(hwnd == ::GetDlgItem(_hSelf, IDC_HASH_TEXT_EDIT)) {
				    return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			    }
			    else {
				    return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			    }
		    }
		    break;
		case WM_CTLCOLORDLG:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORSTATIC:
		    if(NppDarkMode::isEnabled()) {
			    HWND hwnd = reinterpret_cast<HWND>(lParam);
			    if(hwnd == ::GetDlgItem(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT)) {
				    return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			    }
			    else {
				    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			    }
		    }
		    break;
		case WM_PRINTCLIENT:
		    if(NppDarkMode::isEnabled()) {
			    return TRUE;
		    }
		    break;
		case NPPM_INTERNAL_REFRESHDARKMODE:
		    NppDarkMode::autoThemeChildControls(_hSelf);
		    return TRUE;
		case WM_COMMAND:
	    {
		    if(HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_HASH_TEXT_EDIT) {
			    if(isCheckedOrNot(IDC_HASH_EACHLINE_CHECK)) {
				    generateHashPerLine();
			    }
			    else {
				    generateHash();
			    }
		    }
		    switch(wParam) {
			    case IDCANCEL:
					display(false);
					return TRUE;
			    case IDOK: return TRUE;
			    case IDC_HASH_EACHLINE_CHECK:
					if(isCheckedOrNot(IDC_HASH_EACHLINE_CHECK)) {
						generateHashPerLine();
					}
					else {
						generateHash();
					}
					return TRUE;
			    case IDC_HASH_FROMTEXT_TOCLIPBOARD_BUTTON:
					{
						int len = static_cast<int>(::SendMessage(::GetDlgItem(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT), WM_GETTEXTLENGTH, 0, 0));
						if(len) {
							wchar_t * rStr = new wchar_t[len+1];
							::GetDlgItemText(_hSelf, IDC_HASH_RESULT_FOMTEXT_EDIT, rStr, len + 1);
							str2Clipboard(rStr, _hSelf);
							delete[] rStr;
						}
					}
					return TRUE;
			    default:
					break;
		    }
	    }
	}
	return FALSE;
}

void HashFromTextDlg::setHashType(hashType hashType2set)
{
	_ht = hashType2set;
}

void HashFromTextDlg::doDialog(bool isRTL)
{
	if(!isCreated()) {
		create(IDD_HASHFROMTEXT_DLG, isRTL);
		if(_ht == hash_sha256) {
			generic_string title = TEXT("Generate SHA-256 digest");
			::SetWindowText(_hSelf, title.c_str());
		}
	}
	// Adjust the position in the center
	goToCenter();
};
