// ZipRegistry.h

#ifndef __ZIP_REGISTRY_H
#define __ZIP_REGISTRY_H

//#include "ExtractMode.h"

namespace NExtract {
	struct CInfo {
		void Save() const;
		void Load();

		NPathMode::EEnum PathMode;
		NOverwriteMode::EEnum OverwriteMode;
		bool PathMode_Force;
		bool OverwriteMode_Force;
		CBoolPair SplitDest;
		CBoolPair ElimDup;
		// CBoolPair AltStreams;
		CBoolPair NtSecurity;
		CBoolPair ShowPassword;
		UStringVector Paths;
	};

	void Save_ShowPassword(bool showPassword);
	bool Read_ShowPassword();
}
namespace NCompression {
	struct CFormatOptions {
		CFormatOptions() 
		{
			ResetForLevelChange();
		}
		void ResetForLevelChange()
		{
			BlockLogSize = NumThreads = Level = Dictionary = Order = uint32(-1);
			Method.Empty();
			// Options.Empty();
			// EncryptionMethod.Empty();
		}
		uint32 Level;
		uint32 Dictionary;
		uint32 Order;
		uint32 BlockLogSize;
		uint32 NumThreads;
		CSysString FormatID;
		UString Method;
		UString Options;
		UString EncryptionMethod;
	};

	struct CInfo {
		void Save() const;
		void Load();

		uint32 Level;
		bool ShowPassword;
		bool EncryptHeaders;
		UString ArcType;
		UStringVector ArcPaths;
		CObjectVector <CFormatOptions> Formats;
		CBoolPair NtSecurity;
		CBoolPair AltStreams;
		CBoolPair HardLinks;
		CBoolPair SymLinks;
	};
}
namespace NWorkDir {
	namespace NMode {
		enum EEnum {
			kSystem,
			kCurrent,
			kSpecified
		};
	}
	struct CInfo {
		void SetForRemovableOnlyDefault() { ForRemovableOnly = true; }
		void SetDefault()
		{
			Mode = NMode::kSystem;
			Path.Empty();
			SetForRemovableOnlyDefault();
		}
		void Save() const;
		void Load();

		NMode::EEnum Mode;
		FString Path;
		bool   ForRemovableOnly;
		uint8  Reserve[3]; // @alignment
	};
}

struct CContextMenuInfo {
	CBoolPair Cascaded;
	CBoolPair MenuIcons;
	CBoolPair ElimDup;
	uint32 Flags;
	bool   Flags_Def;
	uint8  Reserve[3]; // @alignment

	void Save() const;
	void Load();
};

#endif
