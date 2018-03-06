// UpdateProduce.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//#include "UpdateProduce.h"

using namespace NUpdateArchive;

static const char * const kUpdateActionSetCollision = "Internal collision in update action set";

CUpdatePair2::CUpdatePair2() : NewData(false), NewProps(false), UseArcProps(false), IsAnti(false), DirIndex(-1), ArcIndex(-1),
	NewNameIndex(-1), IsMainRenameItem(false)
{
}

void FASTCALL CUpdatePair2::SetAs_NoChangeArcItem(int arcIndex)
{
	NewData = NewProps = false;
	UseArcProps = true;
	IsAnti = false;
	ArcIndex = arcIndex;
}

bool CUpdatePair2::ExistOnDisk() const { return DirIndex != -1; }
bool CUpdatePair2::ExistInArchive() const { return ArcIndex != -1; }

void UpdateProduce(const CRecordVector<CUpdatePair> &updatePairs, const CActionSet &actionSet,
    CRecordVector<CUpdatePair2> &operationChain, IUpdateProduceCallback * callback)
{
	FOR_VECTOR(i, updatePairs) {
		const CUpdatePair &pair = updatePairs[i];
		CUpdatePair2 up2;
		up2.DirIndex = pair.DirIndex;
		up2.ArcIndex = pair.ArcIndex;
		up2.NewData = up2.NewProps = true;
		up2.UseArcProps = false;
		switch(actionSet.StateActions[(uint)pair.State]) {
			case NPairAction::kIgnore:
			    if(pair.ArcIndex >= 0 && callback)
				    callback->ShowDeleteFile(pair.ArcIndex);
			    continue;
			case NPairAction::kCopy:
			    if(pair.State == NPairState::kOnlyOnDisk)
				    throw kUpdateActionSetCollision;
			    if(pair.State == NPairState::kOnlyInArchive) {
				    if(pair.HostIndex >= 0) {
					    /*
					       ignore alt stream if
					        1) no such alt stream in Disk
					        2) there is Host file in disk
					     */
					    if(updatePairs[pair.HostIndex].DirIndex >= 0)
						    continue;
				    }
			    }
			    up2.NewData = up2.NewProps = false;
			    up2.UseArcProps = true;
			    break;

			case NPairAction::kCompress:
			    if(pair.State == NPairState::kOnlyInArchive || pair.State == NPairState::kNotMasked)
				    throw kUpdateActionSetCollision;
			    break;
			case NPairAction::kCompressAsAnti:
			    up2.IsAnti = true;
			    up2.UseArcProps = (pair.ArcIndex >= 0);
			    break;
		}
		operationChain.Add(up2);
	}
	operationChain.ReserveDown();
}

