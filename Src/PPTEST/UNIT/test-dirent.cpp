// TEST-DIRENT.CPP
// A.Sobolev 2023
// @codepage UTF-8
// Реализация POSIX-dirent для Windows
//
#include <pp.h>
#pragma hdrstop
#include <direct.h>
#include <dirent.h>

static int T_Dirent(STestCase & rCase, const char * pBaseDir)
{
	SString dir_buf;
	SString temp_buf;
	// Basic directory retrieval 
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); // Open directory 
		if(!dir) {
			rCase.SetInfo(temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found"), 0);
		}
		else {
			// Read entries 
			struct dirent * ent;
			int found = 0;
			while((ent = readdir(dir)) != NULL) {
				// Check each file 
				if(sstreq(ent->d_name, ".")) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 1U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 1U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(1, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 1;
				}
				else if(sstreq(ent->d_name, "..")) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 2U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 2U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(2, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 2;
				}
				else if(sstreq(ent->d_name, "file")) { // Regular file 
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 4U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 4U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(4, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 4;
				}
				else if(sstreq(ent->d_name, "dir")) { // Just a directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 3U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 3U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(3, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 8;
				}
				else { // Other file
					slfprintf_stderr("Unexpected file %s\n", ent->d_name);
					abort();
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
			closedir(dir);
		}
	}
	// Function opendir() fails if directory doesn't exist 
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("invalid")); /* Open directory */
		rCase.SLTEST_CHECK_Z(dir);
		rCase.SLTEST_CHECK_EQ(errno, ENOENT);
	}
	// Function opendir() fails if pathname is really a file
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1/file")); /* Open directory */
		rCase.SLTEST_CHECK_Z(dir);
		rCase.SLTEST_CHECK_EQ(errno, ENOTDIR);
	}
	// Function opendir() fails if pathname is a zero-length string 
	{
		DIR * dir = opendir(""); /* Open directory */
		rCase.SLTEST_CHECK_Z(dir);
		rCase.SLTEST_CHECK_EQ(errno, ENOENT);
	}
	// Rewind of directory stream 
	{
		struct dirent * ent;
		int found = 0;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		rCase.SLTEST_CHECK_NZ(dir);
		/* Read entries */
		while((ent = readdir(dir)) != NULL) {
			// Check each file 
			if(sstreq(ent->d_name, ".")) {
				found += 1; // Directory itself
			}
			else if(sstreq(ent->d_name, "..")) {
				found += 2; // Parent directory
			}
			else if(sstreq(ent->d_name, "file")) {
				found += 4; /* Regular file */
			}
			else if(sstreq(ent->d_name, "dir")) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name), 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
		rewinddir(dir); // Rewind stream and read entries again 
		found = 0;
		// Read entries
		while((ent = readdir(dir)) != NULL) {
			/* Check each file */
			if(sstreq(ent->d_name, ".")) {
				found += 1; // Directory itself
			}
			else if(sstreq(ent->d_name, "..")) {
				found += 2; // Parent directory
			}
			else if(sstreq(ent->d_name, "file")) {
				found += 4; /* Regular file */
			}
			else if(sstreq(ent->d_name, "dir")) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name), 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
		closedir(dir);
	}
	/* Rewind with intervening change of working directory */
	{
		struct dirent * ent;
		int found = 0;
		int errorcode;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		rCase.SLTEST_CHECK_NZ(dir);
		/* Read entries */
		while((ent = readdir(dir)) != NULL) {
			/* Check each file */
			if(sstreq(ent->d_name, ".")) {
				found += 1; // Directory itself
			}
			else if(sstreq(ent->d_name, "..")) {
				found += 2; // Parent directory
			}
			else if(sstreq(ent->d_name, "file")) {
				found += 4; /* Regular file */
			}
			else if(sstreq(ent->d_name, "dir")) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name), 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
		/* Change working directory */
		errorcode = chdir((dir_buf = pBaseDir).RmvLastSlash());
		rCase.SLTEST_CHECK_Z(errorcode);
		rewinddir(dir); // Rewind stream and read entries again 
		found = 0;
		/* Read entries */
		while((ent = readdir(dir)) != NULL) {
			/* Check each file */
			if(sstreq(ent->d_name, ".")) {
				found += 1; // Directory itself
			}
			else if(sstreq(ent->d_name, "..")) {
				found += 2; // Parent directory
			}
			else if(sstreq(ent->d_name, "file")) {
				found += 4; /* Regular file */
			}
			else if(sstreq(ent->d_name, "dir")) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name), 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
		/* Restore working directory */
		errorcode = chdir("..");
		rCase.SLTEST_CHECK_Z(errorcode);
		closedir(dir);
	}
	/* Long file name */
	{
		struct dirent * ent;
		int found = 0;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("2")); /* Open directory */
		if(!dir) {
			rCase.SetInfo(temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found"), 0);
		}
		else {
			/* Read entries */
			while((ent = readdir(dir)) != NULL) {
				/* Check each file */
				if(sstreq(ent->d_name, ".")) {
					found += 1; // Directory itself
				}
				else if(sstreq(ent->d_name, "..")) {
					found += 2; // Parent directory
				}
				else if(sstreqi_ascii(ent->d_name, "file.txt")) {
					/* Regular 8+3 filename */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 8U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 8U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(8, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 4;
				}
				else if(sstreqi_ascii(ent->d_name, "Testfile-1.2.3.dat")) {
					/* Long file name with multiple dots */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 18U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 18U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(18, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 8;
				}
				else { // Other file
					rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name), 0);
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
			closedir(dir);
		}
	}
	/* Basic directory retrieval with readdir_r */
	{
		struct dirent ent[10];
		struct dirent * entry;
		size_t i = 0;
		size_t n = 0;
		int found = 0;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		if(!dir) {
			rCase.SetInfo(temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found"), 0);
		}
		else {
			/* Read entries to table */
			while(readdir_r(dir, &ent[n], &entry) == /*OK*/ 0 && entry != 0) {
				n++;
				rCase.SLTEST_CHECK_LE(n, 4U);
			}
			/* Make sure that we got all the files from directory */
			rCase.SLTEST_CHECK_EQ(n, 4U);
			/* Check entries in memory */
			for(i = 0; i < 4; i++) {
				entry = &ent[i];
				/* Check each file */
				if(sstreq(entry->d_name, ".")) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 1U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 1U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(1, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 1;
				}
				else if(sstreq(entry->d_name, "..")) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 2U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 2U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(2, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 2;
				}
				else if(sstreq(entry->d_name, "file")) { /* Regular file */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 4U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 4U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(4, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 4;
				}
				else if(sstreq(entry->d_name, "dir")) { /* Just a directory */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 3U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 3U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(3, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 8;
				}
				else { // Other file
					rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(entry->d_name), 0);
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
			closedir(dir);
		}
	}
	/* Basic directory retrieval with _wreaddir_r */
#ifdef WIN32
	{
		struct _wdirent ent[10];
		struct _wdirent * entry;
		size_t i = 0;
		size_t n = 0;
		int found = 0;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("1");
		SStringU dir_buf_u;
		dir_buf_u.CopyFromMb(cpANSI, dir_buf, dir_buf.Len());
		_WDIR * dir = _wopendir(dir_buf_u/*L"tests/1"*/); /* Open directory */
		if(!dir) {
			rCase.SetInfo(temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found"), 0);
		}
		else {
			// Read entries to table 
			while(_wreaddir_r(dir, &ent[n], &entry) == /*OK*/ 0 && entry != 0) {
				n++;
				rCase.SLTEST_CHECK_LE(n, 4U);
			}
			rCase.SLTEST_CHECK_EQ(n, 4U); // Make sure that we got all the files from directory 
			// Check entries in memory 
			for(i = 0; i < 4; i++) {
				entry = &ent[i];
				// Check each file 
				if(wcscmp(entry->d_name, L".") == 0) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 1U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 1U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(1, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 1;
				}
				else if(wcscmp(entry->d_name, L"..") == 0) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 2U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 2U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(2, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 2;
				}
				else if(wcscmp(entry->d_name, L"file") == 0) { /* Regular file */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 4U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 4U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(4, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 4;
				}
				else if(wcscmp(entry->d_name, L"dir") == 0) { // Just a directory 
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 3U);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 3U);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(3, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 8;
				}
				else { // Other file
					rCase.SetInfo(temp_buf.Z().Cat("Unexpected file"), 0);
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xf); // Make sure that all files were found
			_wclosedir(dir);
		}
	}
#endif
	return rCase.GetCurrentStatus();
}

/* Filter and sort functions */
static int only_readme(const struct dirent * entry);
static int no_directories(const struct dirent * entry);
static int reverse_alpha(const struct dirent ** a, const struct dirent ** b);

static int T_ScanDir(STestCase & rCase, const char * pBaseDir)
{
	struct dirent ** files;
	int i;
	int n;
	SString dir_buf;
	SString temp_buf;
	srand((uint)time(NULL)); // Initialize random number generator 
	// Basic scan with simple filter function 
	{
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, only_readme, alphasort); /* Read directory entries */
		rCase.SLTEST_CHECK_EQ(n, 1);
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[0]->d_name, "README.txt")); // Make sure that the filter works
		// Release file names 
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	/* Basic scan with default sorting function */
	{
		/* Read directory entries in alphabetic order */
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, NULL, alphasort);
		rCase.SLTEST_CHECK_EQ(n, 13);
		// Make sure that we got all the names in the proper order 
		rCase.SLTEST_CHECK_NZ(sstreq(files[0]->d_name, "."));
		rCase.SLTEST_CHECK_NZ(sstreq(files[1]->d_name, ".."));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[2]->d_name, "3zero.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[3]->d_name, "666.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[4]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[5]->d_name, "README.txt"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[6]->d_name, "aaa.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[7]->d_name, "dirent.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[8]->d_name, "empty.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[9]->d_name, "sane-1.12.0.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[10]->d_name, "sane-1.2.30.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[11]->d_name, "sane-1.2.4.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[12]->d_name, "zebra.dat"));
		// Release file names 
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	// Custom filter AND sort function 
	{
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, no_directories, reverse_alpha); /* Read directory entries in alphabetic order */
		rCase.SLTEST_CHECK_EQ(n, 11);
		// Make sure that we got file names in the reverse order
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[0]->d_name, "zebra.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[1]->d_name, "sane-1.2.4.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[2]->d_name, "sane-1.2.30.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[3]->d_name, "sane-1.12.0.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[4]->d_name, "empty.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[5]->d_name, "dirent.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[6]->d_name, "aaa.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[7]->d_name, "README.txt"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[8]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[9]->d_name, "666.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[10]->d_name, "3zero.dat"));
		// Release file names 
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	/* Trying to read from non-existent directory leads to an error */
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("invalid");
		n = scandir(dir_buf, &files, NULL, alphasort);
		rCase.SLTEST_CHECK_EQ(n, -1);
		rCase.SLTEST_CHECK_Z(files);
		rCase.SLTEST_CHECK_EQ(errno, ENOENT);
	}
	// Trying to open file as a directory produces ENOTDIR error 
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3/666.dat");
		n = scandir(dir_buf, &files, NULL, alphasort);
		rCase.SLTEST_CHECK_EQ(n, -1);
		rCase.SLTEST_CHECK_Z(files);
		rCase.SLTEST_CHECK_EQ(errno, ENOTDIR);
	}
	// Sort files using versionsort()
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, no_directories, versionsort);
		rCase.SLTEST_CHECK_EQ(n, 11);
		//
		// Make sure that we got all the file names in the proper order: 1.2.4 < 1.2.30 < 1.12.0
		//
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[0]->d_name, "3zero.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[1]->d_name, "666.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[2]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[3]->d_name, "README.txt"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[4]->d_name, "aaa.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[5]->d_name, "dirent.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[6]->d_name, "empty.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[7]->d_name, "sane-1.2.4.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[8]->d_name, "sane-1.2.30.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[9]->d_name, "sane-1.12.0.dat"));
		rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[10]->d_name, "zebra.dat"));
		// Release file names 
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	// Scan large directory 
	{
		char dirname[PATH_MAX+1];
		int i;
		int ok;
		// Copy name of temporary directory to variable dirname
#ifdef WIN32
		i = GetTempPathA(PATH_MAX, dirname);
		rCase.SLTEST_CHECK_LT(0, i);
#else
		strcpy(dirname, "/tmp/");
		i = strlen(dirname);
#endif
		// Append random characters to dirname
		for(int j = 0; j < 10; j++) {
			char c = "abcdefghijklmnopqrstuvwxyz"[rand() % 26]; /* Generate random character */
			// Append character to dirname 
			rCase.SLTEST_CHECK_LT(i, PATH_MAX);
			dirname[i++] = c;
		}
		/* Terminate directory name */
		rCase.SLTEST_CHECK_LT(i, PATH_MAX);
		dirname[i] = '\0';
		/* Create directory */
#ifdef WIN32
		ok = CreateDirectoryA(dirname, NULL);
		rCase.SLTEST_CHECK_NZ(ok);
#else
		ok = mkdir(dirname, 0700);
		rCase.SLTEST_CHECK_Z(ok); /*success*/
#endif
		/* Create one thousand files */
		rCase.SLTEST_CHECK_LT((i + 5), PATH_MAX);
		for(int j = 0; j < 1000; j++) {
			FILE * fp;
			/* Construct file name */
			dirname[i] = '/';
			dirname[i+1] = 'z';
			dirname[i+2] = '0' + ((j / 100) % 10);
			dirname[i+3] = '0' + ((j / 10) % 10);
			dirname[i+4] = '0' + (j % 10);
			dirname[i+5] = '\0';
			/* Create file */
			fp = fopen(dirname, "w");
			rCase.SLTEST_CHECK_NZ(fp);
			fclose(fp);
		}
		dirname[i] = '\0'; /* Cut out the file name part */
		/* Scan directory */
		n = scandir(dirname, &files, no_directories, alphasort);
		rCase.SLTEST_CHECK_EQ(n, 1000);
		// Make sure that all 1000 files are read back 
		for(int j = 0; j < n; j++) {
			char match[100];
			// Construct file name 
			match[0] = 'z';
			match[1] = '0' + ((j / 100) % 10);
			match[2] = '0' + ((j / 10) % 10);
			match[3] = '0' + (j % 10);
			match[4] = '\0';
			// Make sure that file name matches that on the disk
			rCase.SLTEST_CHECK_NZ(sstreqi_ascii(files[j]->d_name, match));
		}
		// Release file names
		for(int j = 0; j < n; j++) {
			SAlloc::F(files[j]);
		}
		SAlloc::F(files);
	}
	return rCase.GetCurrentStatus();
}
//
// Only pass README.txt file 
//
static int only_readme(const struct dirent * entry) { return BIN(sstreqi_ascii(entry->d_name, "README.txt")); }
//
// Filter out directories 
//
static int no_directories(const struct dirent * entry) { return BIN(entry->d_type != DT_DIR); }
//
// Sort in reverse direction 
//
static int reverse_alpha(const struct dirent ** a, const struct dirent ** b) { return strcoll((*b)->d_name, (*a)->d_name); }
//
//
//
static int T_Strverscmp(STestCase & rCase)
{
	/* Strings without digits are compared as in strcmp() */
	rCase.SLTEST_CHECK_NZ(strverscmp("", "") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("abc", "abc") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("a", "b") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("b", "a") > 0);
	/* Shorter string is smaller, other things being equal */
	rCase.SLTEST_CHECK_NZ(strverscmp("a", "aa") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("aa", "a") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("abcdef", "abcdefg") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("abcdefg", "abcdef") > 0);
	/* Integers with equal length are compared as in strcmp() */
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "0") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "000") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "2") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("2", "1") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("001", "100") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("100", "001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("2020-07-01", "2020-07-02") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("2020-07-02", "2020-07-01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("jan999", "jan999") == 0);
	/* Integers of different length are compared as numbers */
	rCase.SLTEST_CHECK_NZ(strverscmp("jan9", "jan10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("jan10", "jan9") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("999", "1000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000", "999") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("t12-1000", "t12-9999") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("t12-9999", "t12-1000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000", "10001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("10001", "1000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000!", "10001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("10001", "1000!") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000Z", "10001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("10001", "1000Z") > 0);
	/* If numbers starts with zero, then longer number is smaller */
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "00") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("a000", "a00") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("a00", "a000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000!", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000Z") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000Z", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000Z") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000Z", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.01", "1.0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0", "1.01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.01", "1.0!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0!", "1.01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.01", "1.0~") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0~", "1.01") > 0);
	/* Number having more leading zeros is considered smaller */
	rCase.SLTEST_CHECK_NZ(strverscmp("item-0001", "item-001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-001", "item-0001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-001", "item-01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-01", "item-001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".0001000", ".001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".001", ".0001000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".0001000", ".01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".01", ".0001000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".0001000", ".1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".1", ".0001000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0002", "1.0010000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0010000", "1.0002") > 0);
	/* Number starting with zero is smaller than any number */
	rCase.SLTEST_CHECK_NZ(strverscmp("item-009", "item-1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-1", "item-009") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-099", "item-2") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-2", "item-099") > 0);
	/* Number vs alphabetical comparison */
	rCase.SLTEST_CHECK_NZ(strverscmp("1.001", "1.00!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.00!", "1.001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.001", "1.00x") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.00x", "1.001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "x") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("x", "1") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "!") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("!", "1") < 0);
	/* Handling the end of string */
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "011") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("011", "01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0100", "01000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01000", "0100") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "1!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1!", "1") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "1z") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1z", "1") > 0);
	/* Ordering 000 < 00 < 01 < 010 < 09 < 0 < 1 < 9 < 10 */
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "00") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "010") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "010") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "010") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("9", "10") < 0);
	/* Compare speed */
	{
#define LENGTH 100
#define REPEAT 1000000
		char a[LENGTH+1];
		char b[LENGTH+1];
		size_t i;
		size_t j;
		const char letters[] = "01234567890123456789abdefghjkpqrtwxyz-/.";
		size_t n = strlen(letters);
		/* Repeat test */
		for(i = 0; i < REPEAT; i++) {
			int diff1;
			int diff2;
			/* Generate two random strings of LENGTH characters */
			for(j = 0; j < LENGTH; j++) {
				a[j] = letters[rand() % n];
				b[j] = letters[rand() % n];
			}
			a[j] = '\0';
			b[j] = '\0';
			/* Compare strings in both directions */
			diff1 = strverscmp(a, b);
			diff2 = strverscmp(b, a);
			/* Must give identical result in both directions */
			rCase.SLTEST_CHECK_NZ((diff1 < 0 && diff2 > 0) || (diff1 == 0 && diff2 == 0) || (diff1 > 0 && diff2 < 0));
		}
	}
	return rCase.GetCurrentStatus();
}

static int T_DirEntUnicode(STestCase & rCase, const char * pLocale)
{
#ifdef WIN32
	wchar_t wpath[MAX_PATH+1];
	char path[MAX_PATH+1];
	DWORD i, j, k, x;
	BOOL ok;
	HANDLE fh;
	_WDIR * wdir;
	struct _wdirent * wentry;
	DIR * dir;
	struct dirent * entry;
	char buffer[100];
	FILE * fp;
	int counter = 0;
	srand(((int)time(NULL)) * 257 + ((int)GetCurrentProcessId())); /* Initialize random number generator */
	/* Set current locale */
	if(!isempty(pLocale)) {
		printf("Locale %s\n", pLocale);
		setlocale(LC_ALL, pLocale);
	}
	else {
		setlocale(LC_ALL, "");
	}
	/****** CREATE FILE WITH UNICODE FILE NAME ******/

	/* Get path to temporary directory (wide-character and ascii) */
	i = GetTempPathW(MAX_PATH, wpath);
	assert(i > 0);
	j = GetTempPathA(MAX_PATH, path);
	assert(j > 0);
	/* Append random directory name */
	for(k = 0; k < 10; k++) {
		char c = "abcdefghijklmnopqrstuvwxyz"[rand() % 26]; /* Generate random character */
		/* Append character to paths */
		assert(i < MAX_PATH &&  j < MAX_PATH);
		wpath[i++] = c;
		path[j++] = c;
	}
	/* Terminate paths */
	assert(i < MAX_PATH &&  j < MAX_PATH);
	wpath[i] = '\0';
	path[j] = '\0';
	k = i; /* Remember the end of directory name */
	/* Create directory using unicode */
	ok = CreateDirectoryW(wpath, NULL);
	if(!ok) {
		DWORD e = GetLastError();
		wprintf(L"Cannot create directory %ls (code %u)\n", wpath, e);
		abort();
	}
	/* Overwrite zero terminator with path separator */
	assert(i < MAX_PATH &&  j < MAX_PATH);
	wpath[i++] = '\\';
	/* Append a few unicode characters */
	assert(i < MAX_PATH);
	wpath[i++] = 0x6d4b;
	assert(i < MAX_PATH);
	wpath[i++] = 0x8bd5;

	/* Terminate string */
	assert(i < MAX_PATH);
	wpath[i] = '\0';
	/* Create file with unicode */
	fh = CreateFileW(wpath, /* Access */ GENERIC_READ | GENERIC_WRITE, /* Share mode */ 0, /* Security attributes */ NULL, /* Creation disposition */ CREATE_NEW,
		/* Attributes */ FILE_ATTRIBUTE_NORMAL, /* Template files */ NULL);
	assert(fh != INVALID_HANDLE_VALUE);
	/* Write some data to file */
	ok = WriteFile(/* File handle */ fh, /* Pointer to data */ "hep\n", /* Number of bytes to write */ 4, /* Number of bytes written */ NULL, /* Overlapped */ NULL);
	assert(ok);
	ok = CloseHandle(fh); /* Close file */
	assert(ok);

	/****** MAKE SURE THAT UNICODE FILE CAN BE READ BY _WREADDIR ******/

	// Zero terminate wide-character path and open directory stream 
	wpath[k] = '\0';
	wdir = _wopendir(wpath);
	if(wdir == NULL) {
		wprintf(L"Cannot open directory %ls\n", wpath);
		abort();
	}
	// Read through entries 
	counter = 0;
	while((wentry = _wreaddir(wdir)) != NULL) {
		// Skip pseudo directories 
		if(wcscmp(wentry->d_name, L".") == 0) {
			continue;
		}
		if(wcscmp(wentry->d_name, L"..") == 0) {
			continue;
		}
		// Found a file 
		counter++;
		assert(wentry->d_type == DT_REG);
		// Append file name to path 
		i = k;
		assert(i < MAX_PATH);
		wpath[i++] = '\\';
		x = 0;
		while(wentry->d_name[x] != '\0') {
			assert(i < MAX_PATH);
			wpath[i++] = wentry->d_name[x++];
		}
		assert(i < MAX_PATH);
		wpath[i] = '\0';
		// Open file for read 
		fh = CreateFileW(wpath, /* Access */ GENERIC_READ,/* Share mode */ 0, /* Security attributes */ NULL,/* Creation disposition */ OPEN_EXISTING, 
			/* Attributes */ FILE_ATTRIBUTE_NORMAL, /* Template files */ NULL);
		assert(fh != INVALID_HANDLE_VALUE);
		// Read data from file 
		ok = ReadFile(/* File handle */ fh, /* Output buffer */ buffer, /* Max number of bytes to read */ sizeof(buffer) - 1, /* Number of bytes actually read */ &x, /* Overlapped */ NULL);
		assert(ok);
		// Make sure that we got the file contents right 
		assert(x == 4);
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		ok = CloseHandle(fh); /* Close file */
		assert(ok);
	}
	assert(counter == 1);
	_wclosedir(wdir); // Close directory
	/****** MAKE SURE THAT UNICODE FILE NAME CAN BE READ BY READDIR *****/

	/* Zero terminate ascii path and open directory stream */
	k = j;
	path[k] = '\0';
	dir = opendir(path);
	if(!dir) {
		slfprintf_stderr("Cannot open directory %s\n", path);
		abort();
	}
	/* Read through entries */
	counter = 0;
	while((entry = readdir(dir)) != NULL) {
		/* Skip pseudo directories */
		if(strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if(strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		/* Found a file */
		counter++;
		assert(entry->d_type == DT_REG);
		/* Append file name to path */
		j = k;
		assert(j < MAX_PATH);
		path[j++] = '\\';
		x = 0;
		while(entry->d_name[x] != '\0') {
			assert(j < MAX_PATH);
			path[j++] = entry->d_name[x++];
		}
		assert(j < MAX_PATH);
		path[j] = '\0';
		/* Open file for read */
		fp = fopen(path, "r");
		if(!fp) {
			slfprintf_stderr("Cannot open file %s\n", path);
			abort();
		}
		/* Read data from file */
		if(fgets(buffer, sizeof(buffer), fp) == NULL) {
			slfprintf_stderr("Cannot read file %s\n", path);
			abort();
		}
		/* Make sure that we got the file contents right */
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		assert(buffer[4] == '\0');
		fclose(fp); /* Close file */
	}
	assert(counter == 1);
	closedir(dir); /* Close directory */
	/****** CREATE FILE WITH UTF-8 ******/

	/* Append UTF-8 file name (åäö.txt) to path */
	j = k;
	path[j++] = '\\';
	path[j++] = '\xc3';
	path[j++] = '\xa5';
	path[j++] = '\xc3';
	path[j++] = '\xa4';
	path[j++] = '\xc3';
	path[j++] = '\xb6';
	path[j++] = '\x2e';
	path[j++] = '\x74';
	path[j++] = '\x78';
	path[j++] = '\x74';
	assert(j < MAX_PATH);
	path[j] = '\0';
	/*
	 * Create file.
	 *
	 * Be ware that the code below creates a different file depending on
	 * the current locale!  For example, if the current locale is
	 * english_us.65001, then the file name will be "åäö.txt" (7
	 * characters).  However, if the current locale is english_us.1252,
	 * then the file name will be "ÃċÃĊÃ¶.txt" (10 characters).
	 */
	printf("Creating %s\n", path);
	fp = fopen(path, "w");
	if(!fp) {
		slfprintf_stderr("Cannot open file %s\n", path);
		abort();
	}
	fputs("hep\n", fp);
	fclose(fp);
	/* Open directory again */
	path[k] = '\0';
	dir = opendir(path);
	if(!dir) {
		slfprintf_stderr("Cannot open directory %s\n", path);
		abort();
	}
	/* Read through entries */
	counter = 0;
	while((entry = readdir(dir)) != NULL) {
		/* Skip pseudo directories */
		if(strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if(strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		/* Found a file */
		counter++;
		assert(entry->d_type == DT_REG);
		/* Append file name to path */
		j = k;
		assert(j < MAX_PATH);
		path[j++] = '\\';
		x = 0;
		while(entry->d_name[x] != '\0') {
			assert(j < MAX_PATH);
			path[j++] = entry->d_name[x++];
		}
		assert(j < MAX_PATH);
		path[j] = '\0';
		/* Print file name for debugging */
		printf("Opening \"%s\" hex ", path + k + 1);
		x = 0;
		while(entry->d_name[x] != '\0') {
			printf("0x%02x ", (uint)(entry->d_name[x++] & 0xff));
		}
		printf("\n");
		/* Open file for read */
		fp = fopen(path, "r");
		if(!fp) {
			slfprintf_stderr("Cannot open file %s\n", path);
			abort();
		}
		/* Read data from file */
		if(fgets(buffer, sizeof(buffer), fp) == NULL) {
			slfprintf_stderr("Cannot read file %s\n", path);
			abort();
		}
		/* Make sure that we got the file contents right */
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		assert(buffer[4] == '\0');
		fclose(fp); /* Close file */
	}
	assert(counter == 2);
	closedir(dir); /* Close directory */
#else
	/* Linux */
	(void)argc;
	(void)argv;
#endif
	return EXIT_SUCCESS;
}

SLTEST_R(dirent)
{
	// File type macros 
	STATIC_ASSERT(DTTOIF(DT_REG) == S_IFREG);
	STATIC_ASSERT(DTTOIF(DT_DIR) == S_IFDIR);
	STATIC_ASSERT(DTTOIF(DT_FIFO) == S_IFIFO);
	STATIC_ASSERT(DTTOIF(DT_SOCK) == S_IFSOCK);
	STATIC_ASSERT(DTTOIF(DT_CHR) == S_IFCHR);
	STATIC_ASSERT(DTTOIF(DT_BLK) == S_IFBLK);
	STATIC_ASSERT(IFTODT(S_IFREG) == DT_REG);
	STATIC_ASSERT(IFTODT(S_IFDIR) == DT_DIR);
	STATIC_ASSERT(IFTODT(S_IFIFO) == DT_FIFO);
	STATIC_ASSERT(IFTODT(S_IFSOCK) == DT_SOCK);
	STATIC_ASSERT(IFTODT(S_IFCHR) == DT_CHR);
	STATIC_ASSERT(IFTODT(S_IFBLK) == DT_BLK);
	{
		SString base_dir;
		SString path = GetSuiteEntry()->InPath;
		(base_dir = path).SetLastSlash().Cat("dirent");
		T_Strverscmp(*this);
		T_Dirent(*this, base_dir.cptr());
		T_ScanDir(*this, base_dir.cptr());
	}
	return CurrentStatus;
}
