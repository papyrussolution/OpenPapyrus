// UpdateAction.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//#include <UpdateAction.h>

namespace NUpdateArchive {
	const CActionSet k_ActionSet_Add = {
		{ NPairAction::kCopy, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCompress, NPairAction::kCompress, NPairAction::kCompress, NPairAction::kCompress }
	};
	const CActionSet k_ActionSet_Update = {
		{ NPairAction::kCopy, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCopy, NPairAction::kCompress }
	};
	const CActionSet k_ActionSet_Fresh = {{
		 NPairAction::kCopy,
		 NPairAction::kCopy,
		 NPairAction::kIgnore,
		 NPairAction::kCopy,
		 NPairAction::kCompress,
		 NPairAction::kCopy,
		 NPairAction::kCompress
	 }};
	const CActionSet k_ActionSet_Sync = {{
		 NPairAction::kCopy,
		 NPairAction::kIgnore,
		 NPairAction::kCompress,
		 NPairAction::kCopy,
		 NPairAction::kCompress,
		 NPairAction::kCopy,
		 NPairAction::kCompress,
	 }};
	const CActionSet k_ActionSet_Delete = {{
		 NPairAction::kCopy,
		 NPairAction::kIgnore,
		 NPairAction::kIgnore,
		 NPairAction::kIgnore,
		 NPairAction::kIgnore,
		 NPairAction::kIgnore,
		 NPairAction::kIgnore
	 }};

	bool FASTCALL CActionSet::IsEqualTo(const CActionSet &a) const
	{
		for(uint i = 0; i < NPairState::kNumValues; i++)
			if(StateActions[i] != a.StateActions[i])
				return false;
		return true;
	}

	bool CActionSet::NeedScanning() const
	{
		uint i;
		for(i = 0; i < NPairState::kNumValues; i++)
			if(StateActions[i] == NPairAction::kCompress)
				return true;
		for(i = 1; i < NPairState::kNumValues; i++)
			if(StateActions[i] != NPairAction::kIgnore)
				return true;
		return false;
	}
}
