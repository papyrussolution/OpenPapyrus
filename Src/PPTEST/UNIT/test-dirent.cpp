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
					rCase.SLCHECK_EQ(ent->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(ent->d_namlen, static_cast<size_t>(1U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(ent), static_cast<size_t>(1U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(1, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 1;
				}
				else if(sstreq(ent->d_name, "..")) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(ent->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(ent->d_namlen, static_cast<size_t>(2U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(ent), static_cast<size_t>(2U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(2, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 2;
				}
				else if(sstreq(ent->d_name, "file")) { // Regular file 
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(ent->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(ent->d_namlen, static_cast<size_t>(4U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(ent), static_cast<size_t>(4U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(4, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 4;
				}
				else if(sstreq(ent->d_name, "dir")) { // Just a directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(ent->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(ent->d_namlen, static_cast<size_t>(3U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(ent), static_cast<size_t>(3U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(3, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 8;
				}
				else { // Other file
					slfprintf_stderr("Unexpected file %s\n", ent->d_name);
					abort();
				}
			}
			rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
			closedir(dir);
		}
	}
	// Function opendir() fails if directory doesn't exist 
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("invalid")); /* Open directory */
		rCase.SLCHECK_Z(dir);
		rCase.SLCHECK_EQ(errno, ENOENT);
	}
	// Function opendir() fails if pathname is really a file
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1/file")); /* Open directory */
		rCase.SLCHECK_Z(dir);
		rCase.SLCHECK_EQ(errno, ENOTDIR);
	}
	// Function opendir() fails if pathname is a zero-length string 
	{
		DIR * dir = opendir(""); /* Open directory */
		rCase.SLCHECK_Z(dir);
		rCase.SLCHECK_EQ(errno, ENOENT);
	}
	// Rewind of directory stream 
	{
		struct dirent * ent;
		int found = 0;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		rCase.SLCHECK_NZ(dir);
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
		rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
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
		rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
		closedir(dir);
	}
	/* Rewind with intervening change of working directory */
	{
		struct dirent * ent;
		int found = 0;
		int errorcode;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		rCase.SLCHECK_NZ(dir);
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
		rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
		// Change working directory
		errorcode = _chdir((dir_buf = pBaseDir).RmvLastSlash());
		rCase.SLCHECK_Z(errorcode);
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
		rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
		/* Restore working directory */
		errorcode = _chdir("..");
		rCase.SLCHECK_Z(errorcode);
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
					rCase.SLCHECK_EQ(ent->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(ent->d_namlen, static_cast<size_t>(8U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(ent), static_cast<size_t>(8U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(8, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 4;
				}
				else if(sstreqi_ascii(ent->d_name, "Testfile-1.2.3.dat")) {
					/* Long file name with multiple dots */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(ent->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(ent->d_namlen, static_cast<size_t>(18U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(ent), static_cast<size_t>(18U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(18, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 8;
				}
				else { // Other file
					rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name), 0);
				}
			}
			rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
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
				rCase.SLCHECK_LE(n, static_cast<size_t>(4U));
			}
			/* Make sure that we got all the files from directory */
			rCase.SLCHECK_EQ(n, static_cast<size_t>(4U));
			/* Check entries in memory */
			for(i = 0; i < 4; i++) {
				entry = &ent[i];
				/* Check each file */
				if(sstreq(entry->d_name, ".")) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(1U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(1U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(1, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 1;
				}
				else if(sstreq(entry->d_name, "..")) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(2U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(2U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(2, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 2;
				}
				else if(sstreq(entry->d_name, "file")) { /* Regular file */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(4U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(4U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(4, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 4;
				}
				else if(sstreq(entry->d_name, "dir")) { /* Just a directory */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(3U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(3U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(3, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 8;
				}
				else { // Other file
					rCase.SetInfo(temp_buf.Z().Cat("Unexpected file").Space().Cat(entry->d_name), 0);
				}
			}
			rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
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
				rCase.SLCHECK_LE(n, static_cast<size_t>(4U));
			}
			rCase.SLCHECK_EQ(n, static_cast<size_t>(4U)); // Make sure that we got all the files from directory 
			// Check entries in memory 
			for(i = 0; i < 4; i++) {
				entry = &ent[i];
				// Check each file 
				if(wcscmp(entry->d_name, L".") == 0) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(1U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(1U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(1, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 1;
				}
				else if(wcscmp(entry->d_name, L"..") == 0) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(2U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(2U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(2, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 2;
				}
				else if(wcscmp(entry->d_name, L"file") == 0) { /* Regular file */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(4U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(4U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(4, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 4;
				}
				else if(wcscmp(entry->d_name, L"dir") == 0) { // Just a directory 
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLCHECK_EQ(entry->d_type, DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLCHECK_EQ(entry->d_namlen, static_cast<size_t>(3U));
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLCHECK_EQ(_D_EXACT_NAMLEN(entry), static_cast<size_t>(3U));
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLCHECK_LT(3, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 8;
				}
				else { // Other file
					rCase.SetInfo(temp_buf.Z().Cat("Unexpected file"), 0);
				}
			}
			rCase.SLCHECK_EQ(found, 0xf); // Make sure that all files were found
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
		rCase.SLCHECK_EQ(n, 1);
		rCase.SLCHECK_NZ(sstreqi_ascii(files[0]->d_name, "README.txt")); // Make sure that the filter works
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
		rCase.SLCHECK_EQ(n, 13);
		// Make sure that we got all the names in the proper order 
		rCase.SLCHECK_NZ(sstreq(files[0]->d_name, "."));
		rCase.SLCHECK_NZ(sstreq(files[1]->d_name, ".."));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[2]->d_name, "3zero.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[3]->d_name, "666.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[4]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[5]->d_name, "README.txt"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[6]->d_name, "aaa.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[7]->d_name, "dirent.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[8]->d_name, "empty.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[9]->d_name, "sane-1.12.0.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[10]->d_name, "sane-1.2.30.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[11]->d_name, "sane-1.2.4.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[12]->d_name, "zebra.dat"));
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
		rCase.SLCHECK_EQ(n, 11);
		// Make sure that we got file names in the reverse order
		rCase.SLCHECK_NZ(sstreqi_ascii(files[0]->d_name, "zebra.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[1]->d_name, "sane-1.2.4.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[2]->d_name, "sane-1.2.30.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[3]->d_name, "sane-1.12.0.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[4]->d_name, "empty.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[5]->d_name, "dirent.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[6]->d_name, "aaa.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[7]->d_name, "README.txt"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[8]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[9]->d_name, "666.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[10]->d_name, "3zero.dat"));
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
		rCase.SLCHECK_EQ(n, -1);
		rCase.SLCHECK_Z(files);
		rCase.SLCHECK_EQ(errno, ENOENT);
	}
	// Trying to open file as a directory produces ENOTDIR error 
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3/666.dat");
		n = scandir(dir_buf, &files, NULL, alphasort);
		rCase.SLCHECK_EQ(n, -1);
		rCase.SLCHECK_Z(files);
		rCase.SLCHECK_EQ(errno, ENOTDIR);
	}
	// Sort files using versionsort()
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, no_directories, versionsort);
		rCase.SLCHECK_EQ(n, 11);
		//
		// Make sure that we got all the file names in the proper order: 1.2.4 < 1.2.30 < 1.12.0
		//
		rCase.SLCHECK_NZ(sstreqi_ascii(files[0]->d_name, "3zero.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[1]->d_name, "666.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[2]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[3]->d_name, "README.txt"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[4]->d_name, "aaa.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[5]->d_name, "dirent.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[6]->d_name, "empty.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[7]->d_name, "sane-1.2.4.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[8]->d_name, "sane-1.2.30.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[9]->d_name, "sane-1.12.0.dat"));
		rCase.SLCHECK_NZ(sstreqi_ascii(files[10]->d_name, "zebra.dat"));
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
		rCase.SLCHECK_LT(0, i);
#else
		strcpy(dirname, "/tmp/");
		i = strlen(dirname);
#endif
		// Append random characters to dirname
		for(int j = 0; j < 10; j++) {
			char c = "abcdefghijklmnopqrstuvwxyz"[rand() % 26]; /* Generate random character */
			// Append character to dirname 
			rCase.SLCHECK_LT(i, PATH_MAX);
			dirname[i++] = c;
		}
		/* Terminate directory name */
		rCase.SLCHECK_LT(i, PATH_MAX);
		dirname[i] = '\0';
		/* Create directory */
#ifdef WIN32
		ok = CreateDirectoryA(dirname, NULL);
		rCase.SLCHECK_NZ(ok);
#else
		ok = mkdir(dirname, 0700);
		rCase.SLCHECK_Z(ok); /*success*/
#endif
		/* Create one thousand files */
		rCase.SLCHECK_LT((i + 5), PATH_MAX);
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
			rCase.SLCHECK_NZ(fp);
			fclose(fp);
		}
		dirname[i] = '\0'; /* Cut out the file name part */
		/* Scan directory */
		n = scandir(dirname, &files, no_directories, alphasort);
		rCase.SLCHECK_EQ(n, 1000);
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
			rCase.SLCHECK_NZ(sstreqi_ascii(files[j]->d_name, match));
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
	rCase.SLCHECK_NZ(strverscmp("", "") == 0);
	rCase.SLCHECK_NZ(strverscmp("abc", "abc") == 0);
	rCase.SLCHECK_NZ(strverscmp("a", "b") < 0);
	rCase.SLCHECK_NZ(strverscmp("b", "a") > 0);
	/* Shorter string is smaller, other things being equal */
	rCase.SLCHECK_NZ(strverscmp("a", "aa") < 0);
	rCase.SLCHECK_NZ(strverscmp("aa", "a") > 0);
	rCase.SLCHECK_NZ(strverscmp("abcdef", "abcdefg") < 0);
	rCase.SLCHECK_NZ(strverscmp("abcdefg", "abcdef") > 0);
	/* Integers with equal length are compared as in strcmp() */
	rCase.SLCHECK_NZ(strverscmp("0", "0") == 0);
	rCase.SLCHECK_NZ(strverscmp("000", "000") == 0);
	rCase.SLCHECK_NZ(strverscmp("1", "2") < 0);
	rCase.SLCHECK_NZ(strverscmp("2", "1") > 0);
	rCase.SLCHECK_NZ(strverscmp("001", "100") < 0);
	rCase.SLCHECK_NZ(strverscmp("100", "001") > 0);
	rCase.SLCHECK_NZ(strverscmp("2020-07-01", "2020-07-02") < 0);
	rCase.SLCHECK_NZ(strverscmp("2020-07-02", "2020-07-01") > 0);
	rCase.SLCHECK_NZ(strverscmp("jan999", "jan999") == 0);
	/* Integers of different length are compared as numbers */
	rCase.SLCHECK_NZ(strverscmp("jan9", "jan10") < 0);
	rCase.SLCHECK_NZ(strverscmp("jan10", "jan9") > 0);
	rCase.SLCHECK_NZ(strverscmp("999", "1000") < 0);
	rCase.SLCHECK_NZ(strverscmp("1000", "999") > 0);
	rCase.SLCHECK_NZ(strverscmp("t12-1000", "t12-9999") < 0);
	rCase.SLCHECK_NZ(strverscmp("t12-9999", "t12-1000") > 0);
	rCase.SLCHECK_NZ(strverscmp("1000", "10001") < 0);
	rCase.SLCHECK_NZ(strverscmp("10001", "1000") > 0);
	rCase.SLCHECK_NZ(strverscmp("1000!", "10001") < 0);
	rCase.SLCHECK_NZ(strverscmp("10001", "1000!") > 0);
	rCase.SLCHECK_NZ(strverscmp("1000Z", "10001") < 0);
	rCase.SLCHECK_NZ(strverscmp("10001", "1000Z") > 0);
	/* If numbers starts with zero, then longer number is smaller */
	rCase.SLCHECK_NZ(strverscmp("00", "0") < 0);
	rCase.SLCHECK_NZ(strverscmp("0", "00") > 0);
	rCase.SLCHECK_NZ(strverscmp("a000", "a00") < 0);
	rCase.SLCHECK_NZ(strverscmp("a00", "a000") > 0);
	rCase.SLCHECK_NZ(strverscmp("0000", "000") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "0000") > 0);
	rCase.SLCHECK_NZ(strverscmp("0000", "000!") < 0);
	rCase.SLCHECK_NZ(strverscmp("000!", "0000") > 0);
	rCase.SLCHECK_NZ(strverscmp("0000", "000Z") < 0);
	rCase.SLCHECK_NZ(strverscmp("000Z", "0000") > 0);
	rCase.SLCHECK_NZ(strverscmp("0000", "000Z") < 0);
	rCase.SLCHECK_NZ(strverscmp("000Z", "0000") > 0);
	rCase.SLCHECK_NZ(strverscmp("1.01", "1.0") < 0);
	rCase.SLCHECK_NZ(strverscmp("1.0", "1.01") > 0);
	rCase.SLCHECK_NZ(strverscmp("1.01", "1.0!") < 0);
	rCase.SLCHECK_NZ(strverscmp("1.0!", "1.01") > 0);
	rCase.SLCHECK_NZ(strverscmp("1.01", "1.0~") < 0);
	rCase.SLCHECK_NZ(strverscmp("1.0~", "1.01") > 0);
	/* Number having more leading zeros is considered smaller */
	rCase.SLCHECK_NZ(strverscmp("item-0001", "item-001") < 0);
	rCase.SLCHECK_NZ(strverscmp("item-001", "item-0001") > 0);
	rCase.SLCHECK_NZ(strverscmp("item-001", "item-01") < 0);
	rCase.SLCHECK_NZ(strverscmp("item-01", "item-001") > 0);
	rCase.SLCHECK_NZ(strverscmp(".0001000", ".001") < 0);
	rCase.SLCHECK_NZ(strverscmp(".001", ".0001000") > 0);
	rCase.SLCHECK_NZ(strverscmp(".0001000", ".01") < 0);
	rCase.SLCHECK_NZ(strverscmp(".01", ".0001000") > 0);
	rCase.SLCHECK_NZ(strverscmp(".0001000", ".1") < 0);
	rCase.SLCHECK_NZ(strverscmp(".1", ".0001000") > 0);
	rCase.SLCHECK_NZ(strverscmp("1.0002", "1.0010000") < 0);
	rCase.SLCHECK_NZ(strverscmp("1.0010000", "1.0002") > 0);
	/* Number starting with zero is smaller than any number */
	rCase.SLCHECK_NZ(strverscmp("item-009", "item-1") < 0);
	rCase.SLCHECK_NZ(strverscmp("item-1", "item-009") > 0);
	rCase.SLCHECK_NZ(strverscmp("item-099", "item-2") < 0);
	rCase.SLCHECK_NZ(strverscmp("item-2", "item-099") > 0);
	/* Number vs alphabetical comparison */
	rCase.SLCHECK_NZ(strverscmp("1.001", "1.00!") < 0);
	rCase.SLCHECK_NZ(strverscmp("1.00!", "1.001") > 0);
	rCase.SLCHECK_NZ(strverscmp("1.001", "1.00x") < 0);
	rCase.SLCHECK_NZ(strverscmp("1.00x", "1.001") > 0);
	rCase.SLCHECK_NZ(strverscmp("1", "x") < 0);
	rCase.SLCHECK_NZ(strverscmp("x", "1") > 0);
	rCase.SLCHECK_NZ(strverscmp("1", "!") > 0);
	rCase.SLCHECK_NZ(strverscmp("!", "1") < 0);
	/* Handling the end of string */
	rCase.SLCHECK_NZ(strverscmp("01", "011") < 0);
	rCase.SLCHECK_NZ(strverscmp("011", "01") > 0);
	rCase.SLCHECK_NZ(strverscmp("0100", "01000") < 0);
	rCase.SLCHECK_NZ(strverscmp("01000", "0100") > 0);
	rCase.SLCHECK_NZ(strverscmp("1", "1!") < 0);
	rCase.SLCHECK_NZ(strverscmp("1!", "1") > 0);
	rCase.SLCHECK_NZ(strverscmp("1", "1z") < 0);
	rCase.SLCHECK_NZ(strverscmp("1z", "1") > 0);
	/* Ordering 000 < 00 < 01 < 010 < 09 < 0 < 1 < 9 < 10 */
	rCase.SLCHECK_NZ(strverscmp("000", "00") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "01") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "010") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "09") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "0") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "1") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "9") < 0);
	rCase.SLCHECK_NZ(strverscmp("000", "10") < 0);
	rCase.SLCHECK_NZ(strverscmp("00", "01") < 0);
	rCase.SLCHECK_NZ(strverscmp("00", "010") < 0);
	rCase.SLCHECK_NZ(strverscmp("00", "09") < 0);
	rCase.SLCHECK_NZ(strverscmp("00", "0") < 0);
	rCase.SLCHECK_NZ(strverscmp("00", "1") < 0);
	rCase.SLCHECK_NZ(strverscmp("00", "9") < 0);
	rCase.SLCHECK_NZ(strverscmp("00", "10") < 0);
	rCase.SLCHECK_NZ(strverscmp("01", "010") < 0);
	rCase.SLCHECK_NZ(strverscmp("01", "09") < 0);
	rCase.SLCHECK_NZ(strverscmp("01", "0") < 0);
	rCase.SLCHECK_NZ(strverscmp("01", "1") < 0);
	rCase.SLCHECK_NZ(strverscmp("01", "9") < 0);
	rCase.SLCHECK_NZ(strverscmp("01", "10") < 0);
	rCase.SLCHECK_NZ(strverscmp("010", "09") < 0);
	rCase.SLCHECK_NZ(strverscmp("010", "0") < 0);
	rCase.SLCHECK_NZ(strverscmp("010", "1") < 0);
	rCase.SLCHECK_NZ(strverscmp("010", "9") < 0);
	rCase.SLCHECK_NZ(strverscmp("010", "10") < 0);
	rCase.SLCHECK_NZ(strverscmp("09", "0") < 0);
	rCase.SLCHECK_NZ(strverscmp("09", "1") < 0);
	rCase.SLCHECK_NZ(strverscmp("09", "9") < 0);
	rCase.SLCHECK_NZ(strverscmp("09", "10") < 0);
	rCase.SLCHECK_NZ(strverscmp("0", "1") < 0);
	rCase.SLCHECK_NZ(strverscmp("0", "9") < 0);
	rCase.SLCHECK_NZ(strverscmp("0", "10") < 0);
	rCase.SLCHECK_NZ(strverscmp("1", "9") < 0);
	rCase.SLCHECK_NZ(strverscmp("1", "10") < 0);
	rCase.SLCHECK_NZ(strverscmp("9", "10") < 0);
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
			rCase.SLCHECK_NZ((diff1 < 0 && diff2 > 0) || (diff1 == 0 && diff2 == 0) || (diff1 > 0 && diff2 < 0));
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
		if(sstreq(entry->d_name, ".")) {
			continue;
		}
		if(sstreq(entry->d_name, "..")) {
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
		if(sstreq(entry->d_name, ".")) {
			continue;
		}
		if(sstreq(entry->d_name, "..")) {
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
	{
		const SrUedContainer_Rt * p_uedc = DS.GetUedContainer();
		SString temp_buf;
		SString ued_symb;
		TSCollection <SKnownFolderEntry> known_folder_list;
		GetKnownFolderList(known_folder_list);
		for(uint i = 0; i < known_folder_list.getCount(); i++) {
			const SKnownFolderEntry * p_entry = known_folder_list.at(i);
			if(p_entry) {
				ued_symb.Z();
				temp_buf.Z();
				if(p_uedc && p_uedc->GetSymb(p_entry->Ued, ued_symb)) {
					assert(ued_symb.NotEmpty());
					temp_buf.Cat(ued_symb).Space();
				}
				temp_buf.CatHex(p_entry->Ued).Space().Cat(p_entry->Guid, S_GUID::fmtIDL).Space().Cat(p_entry->Result).Space().Cat(p_entry->PathUtf8).CR();
				SetInfo(temp_buf);
			}
		}
	}
	{
		SECURITY_INFORMATION securInfo = DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION /*| SACL_SECURITY_INFORMATION*/;
		SString path = GetSuiteEntry()->InPath;
		uint  ok_count = 0;
		uint  fail_count = 0;
		SFileEntryPool fep;
		SFileEntryPool::Entry fep_entry;
		SBuffer secur_info;
		fep.Scan(path, "*.*", SFileEntryPool::scanfRecursive);
		const uint _c = fep.GetCount();
		for(uint i = 0; i < _c; i++) {
			fep.Get(i, &fep_entry, &path);
			if(SFile::GetSecurity(path, securInfo, secur_info)) {
				ok_count++;
			}
			else {
				fail_count++;
			}
		}

	}
	return CurrentStatus;
}
//
//
//
SLTEST_R(SCachedFileEntity)
{
	static uint GloIdx = 0;
	static ACount Counter;

	class SCachedFileEntity_TestSs : public SCachedFileEntity {
	public:
		static SCachedFileEntity_TestSs * GetGlobalObj(const char * pFileName)
		{
			SCachedFileEntity_TestSs * p_result = 0;
			if(!GloIdx) {
				ENTER_CRITICAL_SECTION
				if(!GloIdx) {
					TSClassWrapper <SCachedFileEntity_TestSs> cls;
					GloIdx = SLS.CreateGlobalObject(cls);
					p_result = static_cast<SCachedFileEntity_TestSs *>(SLS.GetGlobalObject(GloIdx));
					if(p_result) {
						if(!p_result->Init(pFileName))
							p_result = 0;
					}
				}
				LEAVE_CRITICAL_SECTION
			}
			if(GloIdx && !p_result) {
				p_result = static_cast<SCachedFileEntity_TestSs *>(SLS.GetGlobalObject(GloIdx));
			}
			return p_result;
		}
		SCachedFileEntity_TestSs() : SCachedFileEntity()
		{
		}
		virtual ~SCachedFileEntity_TestSs()
		{
		}
		virtual bool InitEntity(void * extraPtr)
		{
			bool   ok = true;
			Ss.Z();
			THROW(fileExists(GetFilePath()));
			{
				SFile f(GetFilePath(), SFile::mRead);
				SString line_buf;
				THROW(f.IsValid());
				while(f.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
					if(line_buf.NotEmpty()) {
						Ss.add(line_buf);
					}
				}
			}
			CATCHZOK
			return ok;
		}
		virtual void DestroyEntity()
		{
			Ss.Z();
		}
		void Get(StringSet & rC) const
		{
			Lck.Lock();
			rC = Ss;
			Lck.Unlock();
		}
	private:
		StringSet Ss;
	};
	class _ThreadWriter : public SlThread {
		SString FilePath;
	public:
		_ThreadWriter(const char * pFilePath) : SlThread(), FilePath(pFilePath)
		{
		}
		virtual void Run()
		{
			const int64 start_clock = clock();
			SString line_buf;
			do {
				ENTER_CRITICAL_SECTION
				{
					SFile f(FilePath, SFile::mAppend);
					assert(f.IsValid());
					if(f.IsValid()) {
						Counter.Incr();
						line_buf.Z().CatLongZ(Counter, 12);
						f.WriteLine(line_buf.CR());
					}
				}
				LEAVE_CRITICAL_SECTION
				SDelay(100);
			} while((clock() - start_clock) < 60000);
		}
	};
	class _Thread : public SlThread {
		SString FilePath;
	public:
		_Thread(const char * pFilePath) : SlThread(), FilePath(pFilePath)
		{
		}
		virtual void Run()
		{
			const int64 start_clock = clock();
			StringSet ss;
			SString temp_buf;
			uint   prev_count = 0;
			do {
				SCachedFileEntity_TestSs * p_obj = SCachedFileEntity_TestSs::GetGlobalObj(FilePath);
				if(p_obj && p_obj->Reload(false, 0)) {
					p_obj->Get(ss);
					uint c = ss.getCount();
					assert(c > 0);
					assert(c >= static_cast<uint>(Counter)-2);
					long test_v = 0;
					uint test_c = 1;
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						test_v = temp_buf.ToLong();
						assert(test_v == test_c);
						test_c++;
					}
					assert(test_c == (c+1));
					assert(!prev_count || c >= prev_count);
					prev_count = c;
				}
				SDelay(10);
			} while((clock() - start_clock) < 80000);
		}
	};

	int    ok = 1;
	SString file_path(MakeOutputFilePath("SCachedFileEntity_SsTest.txt"));
	{
		SFile f(file_path, SFile::mWrite);
		assert(f.IsValid());
		if(f.IsValid()) {
			SString line_buf;
			for(uint i = 0; i < 20; i++) {
				Counter.Incr();
				line_buf.Z().CatLongZ(Counter, 12);
				f.WriteLine(line_buf.CR());
			}
		}
	}
	HANDLE tl[128];
	uint   tc = 0;
	MEMSZERO(tl);
	{
		for(uint i = 0; i < 40; i++) {
			_Thread * p_thr = new _Thread(file_path);
			p_thr->Start();
			tl[tc++] = *p_thr;
		}
	}
	{
		for(uint i = 0; i < 4; i++) {
			_ThreadWriter * p_thr = new _ThreadWriter(file_path);
			p_thr->Start();
			tl[tc++] = *p_thr;
		}
	}
	::WaitForMultipleObjects(tc, tl, TRUE, INFINITE);
	SLS.DestroyGlobalObject(GloIdx);
	return ok;
}

SLTEST_R(SFile)
{
	int    ok = 1;
	SFile file;
	SString file_path;
	{
		// Тестирование открытия и чтения сжатых файлов gz
		// src/rsrc/data/user-agents.json.gz
		STempBuffer rd_buf(101);
		SBuffer _file_data;
		PPGetPath(PPPATH_SRCROOT, file_path);
		file_path.SetLastSlash().Cat("rsrc").SetLastSlash().Cat("data").SetLastSlash().Cat("user-agents.json.gz");
		SFile f(file_path, SFile::mReadCompressed);
		if(SLCHECK_NZ(f.IsValid())) {
			bool rd_done = false;
			int  r = 0;
			do {
				size_t actual_size = 0;
				r = f.Read(rd_buf, rd_buf.GetSize(), &actual_size);
				if(r <= 0)
					rd_done = true;
				else {
					_file_data.Write(rd_buf, actual_size);
				}
			} while(!rd_done);
			SLCHECK_NZ(r);
			if(r) {
				_file_data.WriteByte(0);
				SJson * p_js = SJson::Parse(_file_data.GetBufC());
				SLCHECK_NZ(p_js);
				delete p_js;
			}
		}
	}
	{
		// Тестирование открытия и чтения сжатых файлов bz2
	}
	//
	SString file_name(MakeOutputFilePath("open_for_write_test.txt"));
	{ // @v11.5.9
		// Тестирование функции ReadLineCsv()
		/*
			field01;поле02;field03;поле 04
			;текст;3.1415926;
			"строка с кавычками"" еще что-то";;7000;false
			1;female;"string with ;";true
			some text ;male;"string with ""quotes"" and semicolon;";last field
		*/
		SString temp_buf;
		(file_path = GetSuiteEntry()->OutPath).SetLastSlash().Cat("test-input-csv-semicol-utf8.csv");
		StringSet original_line_collection;
		original_line_collection.add("field01;поле02;field03;поле 04");
		original_line_collection.add(";текст;3.1415926;");
		original_line_collection.add("\"строка с кавычками\"\" еще что-то\";;7000;false");
		original_line_collection.add("1;female;\"string with ;\";true");
		original_line_collection.add("some text ;male;\"string with \"\"quotes\"\" and semicolon;\";last field");
		{
			// Сначала создадим файл
			SFile f_out(file_path, SFile::mWrite);
			THROW(SLCHECK_NZ(f_out.IsValid()));
			for(uint ssp = 0; original_line_collection.get(&ssp, temp_buf);) {
				f_out.WriteLine(temp_buf.CR());
			}
		}
		{
			uint    line_count = 0;
			uint    field_count = 0;
			StringSet ss;
			SFile::ReadLineCsvContext ctx(';');
			SFile f_in(file_path, SFile::mRead);
			THROW(SLCHECK_NZ(f_in.IsValid()));
			while(f_in.ReadLineCsv(ctx, ss)) {
				line_count++;
				field_count = ss.getCount();
				SLCHECK_EQ(field_count, 4U);
			}
			SLCHECK_EQ(line_count, original_line_collection.getCount());
		}
	}
	//
	// Тестирование функции IsOpenedForWriting()
	//
	// Откроем файл на запись (IsOpenedForWriting должен вернуть 1)
	//
	THROW(SLCHECK_NZ(file.Open(file_name, SFile::mWrite)));
	SLCHECK_NZ(SFile::IsOpenedForWriting(file_name));
	SLCHECK_NZ(file.WriteLine("test"));
	SLCHECK_NZ(file.Close());
	//
	// Откроем файл на чтение (IsOpenedForWriting должен вернуть 0)
	//
	THROW(SLCHECK_NZ(file.Open(file_name, SFile::mRead)));
	SLCHECK_Z(SFile::IsOpenedForWriting(file_name));
	SLCHECK_NZ(file.Close());
	//
	{
		SLCHECK_LT(SFile::WaitForWriteSharingRelease(file_name, 10000), 0);
		THROW(SLCHECK_NZ(file.Open(file_name, SFile::mWrite)));
		SLCHECK_Z(SFile::WaitForWriteSharingRelease(file_name, 1000));
		SLCHECK_NZ(file.Close());
		SLCHECK_LT(SFile::WaitForWriteSharingRelease(file_name, 1000), 0);
		//
		// @todo Не проверенным остался случай реального ожидания закрытия файла
		// поскольку для этого надо создавать отдельный асинхронный поток.
		//
	}
	{
		SLCHECK_NZ(SFile::WildcardMatch("*.*", "abc.txt"));
		SLCHECK_NZ(SFile::WildcardMatch("*.txt", "abc.txt"));
		SLCHECK_Z(SFile::WildcardMatch("*.txt", "abc.tx"));
		SLCHECK_NZ(SFile::WildcardMatch("*xyz*.t?t", "12xyz.txt"));
		SLCHECK_Z(SFile::WildcardMatch("*xyz*.t?t", "12xyz.txxt"));
		SLCHECK_NZ(SFile::WildcardMatch("*xyz*.t?t", "12xyz-foo.txt"));
		SLCHECK_NZ(SFile::WildcardMatch("??xyz*.t?t", "12xyz-foo.txt"));
		SLCHECK_Z(SFile::WildcardMatch("??xyz*.t?t", "123xyz-foo.txt"));
		SLCHECK_NZ(SFile::WildcardMatch("??xyz*.x*", "12xyz-foo.xml"));
		SLCHECK_NZ(SFile::WildcardMatch("??xyz*.x*", "12xyz-foo.xls"));
		SLCHECK_NZ(SFile::WildcardMatch("??xyz*.x*", "12xyz-foo.xlsm"));
	}
	{
		bool   debug_mark = false;
		const SString root_path(GetSuiteEntry()->InPath);
		THROW(SLCHECK_NZ(SFile::IsDir(root_path)));
		{
			// Тестирование функций распознавания типов файлов
			SFileEntryPool fep;
			SFileEntryPool::Entry fe;
			STempBuffer file_buf(SMEGABYTE(1));
			THROW(SLCHECK_NZ(file_buf.IsValid()));
			{
				(file_path = root_path).SetLastSlash().Cat("harfbuzz\\test\\shaping\\data\\in-house\\fonts\\DFONT.dfont");
				SFileFormat ff;
				int fir = ff.Identify(file_path);
				if(oneof3(fir, 2, 3, 4)) {
					SFile f_in(file_path, SFile::mRead|SFile::mBinary);
					if(SLCHECK_NZ(f_in.IsValid())) {
						size_t actual_size = 0;
						SLCHECK_NZ(f_in.ReadAll(file_buf, SKILOBYTE(128), &actual_size));
						{
							SFileFormat ff2;
							int fir2 = ff2.IdentifyBuffer(file_buf, actual_size);
							SLCHECK_NZ(oneof2(fir2, 2, 4));
							if(!CurrentStatus)
								debug_mark = true;
						}
					}
				}
			}
			{
				(file_path = root_path).SetLastSlash().Cat("harfbuzz\\test\\shaping\\data\\in-house\\fonts\\TRAK.ttf");
				SFileFormat ff;
				int fir = ff.Identify(file_path);
				if(oneof3(fir, 2, 3, 4)) {
					SFile f_in(file_path, SFile::mRead|SFile::mBinary);
					if(SLCHECK_NZ(f_in.IsValid())) {
						size_t actual_size = 0;
						SLCHECK_NZ(f_in.ReadAll(file_buf, SKILOBYTE(128), &actual_size));
						{
							SFileFormat ff2;
							int fir2 = ff2.IdentifyBuffer(file_buf, actual_size);
							SLCHECK_NZ(oneof2(fir2, 2, 4));
							if(!CurrentStatus)
								debug_mark = true;
						}
					}
				}
			}
#if 0 // (Не удается пока успешно прогнать эти тесты) {
			{
				(file_path = root_path).SetLastSlash().Cat("json\\utf16le.json");
				SFileFormat ff;
				int fir = ff.Identify(file_path);
				if(oneof3(fir, 2, 3, 4)) {
					SFile f_in(file_path, SFile::mRead|SFile::mBinary);
					if(SLCHECK_NZ(f_in.IsValid())) {
						size_t actual_size = 0;
						SLCHECK_NZ(f_in.ReadAll(file_buf, SKILOBYTE(128), &actual_size));
						{
							SFileFormat ff2;
							int fir2 = ff2.IdentifyBuffer(file_buf, actual_size);
							SLCHECK_NZ(oneof2(fir2, 2, 4));
							if(!CurrentStatus)
								debug_mark = true;
						}
					}
				}
			}
			fep.Scan(root_path, "*.*", SFileEntryPool::scanfRecursive);
			for(uint p = 0; p < fep.GetCount(); p++) {
				if(fep.Get(p, &fe, &file_path)) {
					SFileFormat ff;
					int fir = ff.Identify(file_path);
					if(oneof3(fir, 2, 3, 4)) {
						SFile f_in(file_path, SFile::mRead|SFile::mBinary);
						if(SLCHECK_NZ(f_in.IsValid())) {
							size_t actual_size = 0;
							SLCHECK_NZ(f_in.ReadAll(file_buf, SKILOBYTE(128), &actual_size));
							{
								SFileFormat ff2;
								int fir2 = ff2.IdentifyBuffer(file_buf, actual_size);
								SLCHECK_NZ(oneof2(fir2, 2, 4));
								if(!CurrentStatus)
									debug_mark = true;
							}
						}
					}
				}
			}
#endif // } 0
		}
		{ // @v12.4.8
			// Тест функции int SFileEntryPool::Scan(const char * pPath, long flags)
			SFileEntryPool fep;
			SFileEntryPool::Entry fe;
			{	
				// "D:/Papyrus/Src/PPTEST/DATA/Test Directory/Test Directory Level 2/Directory With Many Files/symb-11-foldercl.bmp"
				(file_path = GetSuiteEntry()->InPath).SetLastSlash().Cat("Test Directory").SetLastSlash().Cat("Test Directory Level 2").
					SetLastSlash().Cat("Directory With Many Files").SetLastSlash().Cat("symb-11-foldercl.bmp");
				fep.Scan(file_path, SFileEntryPool::scanfRecursive);
				SLCHECK_EQ(fep.GetCount(), 1U);
			}
			{	
				// "D:/Papyrus/Src/PPTEST/DATA/Test Directory/Test Directory Level 2/Directory With Many Files/"
				(file_path = GetSuiteEntry()->InPath).SetLastSlash().Cat("Test Directory").SetLastSlash().Cat("Test Directory Level 2").
					SetLastSlash().Cat("Directory With Many Files").SetLastSlash();
				fep.Scan(file_path, SFileEntryPool::scanfRecursive);
				SLCHECK_LE(20U, fep.GetCount());
			}
			{	
				// "D:/Papyrus/Src/PPTEST/DATA/Test Directory/Test Directory Level 2/Directory With Many Files"
				(file_path = GetSuiteEntry()->InPath).SetLastSlash().Cat("Test Directory").SetLastSlash().Cat("Test Directory Level 2").
					SetLastSlash().Cat("Directory With Many Files");
				fep.Scan(file_path, SFileEntryPool::scanfRecursive);
				SLCHECK_LE(20U, fep.GetCount());
			}
			//
			{	
				// "D:/Papyrus/Src/PPTEST/DATA/Test Directory/Test Directory Level 2/Directory With Many Files/abc-*.gif"
				(file_path = GetSuiteEntry()->InPath).SetLastSlash().Cat("Test Directory").SetLastSlash().Cat("Test Directory Level 2").
					SetLastSlash().Cat("Directory With Many Files").SetLastSlash().Cat("abc-*.gif");
				fep.Scan(file_path, SFileEntryPool::scanfRecursive);
				SLCHECK_EQ(fep.GetCount(), 65U);
			}
		}
		{
			SFileEntryPool fep;
			SFileEntryPool::Entry fe;
			SBinaryChunk bc_hash;
			fep.Scan(root_path, "*.*", SFileEntryPool::scanfRecursive);
			for(uint p = 0; p < fep.GetCount(); p++) {
				if(fep.Get(p, &fe, &file_path)) {
					uint32 crc1 = 0;
					uint32 crc2 = 0;
					{
						SFile f1(file_path, SFile::mRead|SFile::mBinary);
						THROW(SLCHECK_NZ(f1.IsValid()));
						THROW(SLCHECK_NZ(f1.CalcCRC(0, &crc1)));
					}
					{
						SFile f2(file_path, SFile::mRead|SFile::mBinary);
						THROW(SLCHECK_NZ(f2.IsValid()));
						THROW(SLCHECK_NZ(f2.CalcHash(0, SHASHF_CRC32, bc_hash)));
						SLCHECK_EQ(bc_hash.Len(), sizeof(crc2));
						crc2 = *static_cast<const uint32 *>(bc_hash.PtrC());
					}
					SLCHECK_EQ(crc1, crc2);
					if(!CurrentStatus)
						debug_mark = true;
				}
			}
		}
	}
	CATCHZOK;
	return CurrentStatus;
}

#ifdef __WIN32__
static int Win_IsFileExists(const char * pFileName)
{
	HANDLE handle = ::CreateFile(SUcSwitch(pFileName), 0/* query access only */, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE/* share mode */,
		NULL/*security attributes*/, OPEN_EXISTING/*disposition*/, FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN/* flags & attributes */, NULL/* template file */);
	if(handle != INVALID_HANDLE_VALUE) {
		::CloseHandle(handle);
		return 1;
	}
	else
		return 0;
}
#endif
/*
;
; Аргументы:
; 0 - тестовый подкаталог каталога IN
; 1 - подкаталог тестового каталого, в котором находится большое количество файлов
; 2 - количество файлов, находящихся в каталоге, описанном параметром 1
; 3 - суммарный размер файлов, находящихся в каталоге, описанном параметром 1
;
arglist=Test Directory;Test Directory Level 2\Directory With Many Files;1858;470075
benchmark=access;winfileexists
*/
SLTEST_R(Directory)
{
	int    ok = 1;
	SString path(GetSuiteEntry()->InPath);
	SString out_path(GetSuiteEntry()->OutPath);
	SString test_dir;
	SString test_dir_with_files;
	SString temp_buf;
	uint   files_count = 0;
	int64  files_size = 0;
	SStrCollection file_list;
	// @v11.3.1 {
	{
		const char * p_path_to_normalize = "D:\\Papyrus\\Src\\BuildVC2017\\..\\..\\ppy\\bin\\..\\..\\src\\pptest\\out\\lmdb-test";
		SFsPath::NormalizePath(p_path_to_normalize, SFsPath::npfCompensateDotDot, temp_buf);
		SLCHECK_EQ(temp_buf, "d:\\papyrus\\src\\pptest\\out\\lmdb-test");
		SFsPath::NormalizePath(p_path_to_normalize, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, temp_buf);
		SLCHECK_EQ(temp_buf, "D:\\Papyrus\\src\\pptest\\out\\lmdb-test");
		SFsPath::NormalizePath(p_path_to_normalize, SFsPath::npfSlash, temp_buf);
		SLCHECK_EQ(temp_buf, "d:/papyrus/src/buildvc2017/../../ppy/bin/../../src/pptest/out/lmdb-test");
		SFsPath::NormalizePath(p_path_to_normalize, SFsPath::npfCompensateDotDot|SFsPath::npfUpper, temp_buf);
		SLCHECK_EQ(temp_buf, "D:\\PAPYRUS\\SRC\\PPTEST\\OUT\\LMDB-TEST");
	}
	// } @v11.3.1 
	{
		uint   arg_no = 0;
		for(uint ap = 0, arg_no = 0; EnumArg(&ap, temp_buf); arg_no++) {
			switch(arg_no) {
				case 0: test_dir = temp_buf; break;
				case 1: test_dir_with_files = temp_buf; break;
				case 2: files_count = temp_buf.ToLong(); break;
				case 3: files_size = temp_buf.ToLong(); break;
			}
		}
	}
	(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir);
	THROW(SLCHECK_NZ(pathValid(path, 1)));
	(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat(test_dir_with_files);
	test_dir_with_files = path;
	THROW(SLCHECK_NZ(pathValid(test_dir_with_files, 1)));
	{
		int64  sz = 0;
		SDirEntry de;
		(temp_buf = test_dir_with_files).SetLastSlash().CatChar('*').Dot().CatChar('*');
		for(SDirec dir(temp_buf, 0); dir.Next(&de) > 0;) {
			if(!de.IsFolder()) {
				sz += de.Size;
				de.GetNameA(test_dir_with_files, temp_buf);
				THROW(SLCHECK_Z(::_access(temp_buf, 0)));
				THROW(SLCHECK_NZ(Win_IsFileExists(temp_buf)));
				file_list.insert(newStr(temp_buf));
			}
		}
		THROW(SLCHECK_EQ(file_list.getCount(), files_count));
		THROW(SLCHECK_EQ((long)sz, (long)files_size));
		// @v11.6.5 {
		{
			// "D:\Papyrus\Src\PPTEST\DATA\Test Directory\TDR\Тестовый каталог в кодировке cp1251\"
			// Проверяем наличие каталога с именем русскими буквами (он есть). Проверка и в кодировках utf8 и 1251 (ANSI)
			(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat("TDR\\Тестовый каталог в кодировке cp1251");
			SLCHECK_NZ(fileExists(path));
			path.Transf(CTRANSF_UTF8_TO_OUTER);
			SLCHECK_NZ(fileExists(path));
			// Проверяем наличие файла с именем русскими буквами (он есть). Проверка и в кодировках utf8 и 1251 (ANSI)
			(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat("TDR\\Тестовый каталог в кодировке cp1251\\тестовый файл в кодировке 1251.txt");
			SLCHECK_NZ(fileExists(path));
			path.Transf(CTRANSF_UTF8_TO_OUTER);
			SLCHECK_NZ(fileExists(path));
			// Вставляем дефисы вместо пробелов - такого файла нет!
			(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat("TDR\\Тестовый каталог в кодировке cp1251\\тестовый-файл-в-кодировке-1251.txt");
			SLCHECK_Z(fileExists(path));
			path.Transf(CTRANSF_UTF8_TO_OUTER);
			SLCHECK_Z(fileExists(path));
		}
		// } @v11.6.5
		//
		if(pBenchmark) {
			if(sstreqi_ascii(pBenchmark, "access")) {
				for(uint i = 0; i < file_list.getCount(); i++) {
					::_access(file_list.at(i), 0);
				}
			}
			else if(sstreqi_ascii(pBenchmark, "winfileexists")) {
				for(uint i = 0; i < file_list.getCount(); i++) {
					Win_IsFileExists(file_list.at(i));
				}
			}
		}
	}
	{
		const int64 test_file_size = 1024 * 1024;
		(temp_buf = out_path).SetLastSlash().Cat("тестовый файл с не ansi-символами.txt"); // source-file in utf-8!
		{
			SFile f_out(temp_buf, SFile::mWrite|SFile::mBinary);
			THROW(SLCHECK_NZ(f_out.IsValid()));
			{
				uint8 bin_buf[256];
				for(uint i = 0; i < test_file_size/sizeof(bin_buf); i++) {
					SObfuscateBuffer(bin_buf, sizeof(bin_buf));
					THROW(SLCHECK_NZ(f_out.Write(bin_buf, sizeof(bin_buf))));
				}
			}
		}
		{
			SFile::Stat stat;
			THROW(SLCHECK_NZ(SFile::GetStat(temp_buf, 0, &stat, 0)));
			SLCHECK_EQ(stat.Size, test_file_size);
		}
	}
	CATCHZOK
	return CurrentStatus;
}
