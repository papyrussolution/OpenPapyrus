// Windows/FileSystem.h

#ifndef __WINDOWS_FILE_SYSTEM_H
#define __WINDOWS_FILE_SYSTEM_H

namespace NWindows {
	namespace NFile {
		namespace NSystem {
			bool MyGetVolumeInformation(CFSTR rootPath, UString &volumeName, LPDWORD volumeSerialNumber, LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, UString &fileSystemName);
			UINT MyGetDriveType(CFSTR pathName);
			bool MyGetDiskFreeSpace(CFSTR rootPath, uint64 &clusterSize, uint64 &totalSize, uint64 &freeSize);
		}
	}
}

#endif
