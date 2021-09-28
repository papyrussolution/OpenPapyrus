// SORTERS.CPP
//
#include <npp-internal.h>
#pragma hdrstop

generic_string ISorter::getSortKey(const generic_string & input)
{
	if(isSortingSpecificColumns()) {
		if(input.length() < _fromColumn)
			return TEXT(""); // prevent an std::out_of_range exception
		else if(_fromColumn == _toColumn)
			return input.substr(_fromColumn); // get characters from the indicated column to the end of the line
		else
			return input.substr(_fromColumn, _toColumn - _fromColumn); // get characters between the indicated columns, inclusive
	}
	else
		return input;
}

std::vector<generic_string> LexicographicSorter::sort(std::vector<generic_string> lines)
{
	// Note that both branches here are equivalent in the sense that they always give the same answer.
	// However, if we are *not* sorting specific columns, then we get a 40% speed improvement by not calling
	// getSortKey() so many times.
	if(isSortingSpecificColumns()) {
		std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
		{
			return isDescending() ? (getSortKey(a).compare(getSortKey(b)) > 0) : (getSortKey(a).compare(getSortKey(b)) < 0);
		});
	}
	else {
		std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
		{
			return isDescending() ? (a.compare(b) > 0) : (a.compare(b) < 0);
		});
	}
	return lines;
}

std::vector<generic_string> LexicographicCaseInsensitiveSorter::sort(std::vector<generic_string> lines)
{
	// Note that both branches here are equivalent in the sense that they always give the same answer.
	// However, if we are *not* sorting specific columns, then we get a 40% speed improvement by not calling
	// getSortKey() so many times.
	if(isSortingSpecificColumns()) {
		std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
		{
			return isDescending() ? (OrdinalIgnoreCaseCompareStrings(getSortKey(a).c_str(), getSortKey(b).c_str()) > 0) : 
				(OrdinalIgnoreCaseCompareStrings(getSortKey(a).c_str(), getSortKey(b).c_str()) < 0);
		});
	}
	else {
		std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
		{
			return isDescending() ? (OrdinalIgnoreCaseCompareStrings(a.c_str(), b.c_str()) > 0) : (OrdinalIgnoreCaseCompareStrings(a.c_str(), b.c_str()) < 0);
		});
	}
	return lines;
}

std::vector<generic_string> NaturalSorter::sort(std::vector<generic_string> lines)
{
	// Note that both branches here are equivalent in the sense that they always give the same answer.
	// However, if we are *not* sorting specific columns, then we get a 40% speed improvement by not calling
	// getSortKey() so many times.
	if(isSortingSpecificColumns()) {
		std::sort(lines.begin(), lines.end(), [this](generic_string aIn, generic_string bIn)
		{
			generic_string a = getSortKey(aIn);
			generic_string b = getSortKey(bIn);
			long long compareResult = 0;
			size_t i = 0;
			while(compareResult == 0) {
				if(i >= a.length() || i >= b.length()) {
					compareResult = a.compare(min(i, a.length()), generic_string::npos, b, min(i, b.length()), generic_string::npos);
					break;
				}
				bool aChunkIsNum = a[i] >= L'0' && a[i] <= L'9';
				bool bChunkIsNum = b[i] >= L'0' && b[i] <= L'9';
			    // One is number and one is string
				if(aChunkIsNum != bChunkIsNum) {
					compareResult = a[i] - b[i];
				            // No need to update i; compareResult != 0
				}
				    // Both are numbers
				else if(aChunkIsNum) {
					size_t delta = 0;

				            // stoll crashes if number exceeds the limit for unsigned long long
				            // Maximum value for a variable of type unsigned long long |
				            // 18446744073709551615
				            // So take the max length 18 to convert the number
					const size_t maxLen = 18;
					compareResult = std::stoll(a.substr(i, maxLen)) - std::stoll(b.substr(i, maxLen), &delta);
					i += delta;
				}
				    // Both are strings
				else {
					size_t aChunkEnd = a.find_first_of(L"1234567890", i);
					size_t bChunkEnd = b.find_first_of(L"1234567890", i);
					compareResult = a.compare(i, aChunkEnd - i, b, i, bChunkEnd - i);
					i = aChunkEnd;
				}
			}
			return isDescending() ? (compareResult > 0) : (compareResult < 0);
		});
	}
	else {
		std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
		{
			long long compareResult = 0;
			size_t i = 0;
			while(compareResult == 0) {
				if(i >= a.length() || i >= b.length()) {
					compareResult = a.compare(min(i, a.length()), generic_string::npos, b, min(i, b.length()), generic_string::npos);
					break;
				}
				bool aChunkIsNum = a[i] >= L'0' && a[i] <= L'9';
				bool bChunkIsNum = b[i] >= L'0' && b[i] <= L'9';
				    // One is number and one is string
				if(aChunkIsNum != bChunkIsNum) {
					compareResult = a[i] - b[i];
				            // No need to update i; compareResult != 0
				}
				    // Both are numbers
				else if(aChunkIsNum) {
					size_t delta = 0;
				            // stoll crashes if number exceeds the limit for unsigned long long
				            // Maximum value for a variable of type unsigned long long |
				            // 18446744073709551615
				            // So take the max length 18 to convert the number
					const size_t maxLen = 18;
					compareResult = std::stoll(a.substr(i, maxLen)) - std::stoll(b.substr(i, maxLen), &delta);
					i += delta;
				}
				    // Both are strings
				else {
					size_t aChunkEnd = a.find_first_of(L"1234567890", i);
					size_t bChunkEnd = b.find_first_of(L"1234567890", i);
					compareResult = a.compare(i, aChunkEnd-i, b, i, bChunkEnd-i);
					i = aChunkEnd;
				}
			}
			return isDescending() ? (compareResult > 0) : (compareResult < 0);
		});
	}
	return lines;
}
