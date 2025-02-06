#pragma once
#include "common/BroadcastInterface.h"
#include "core/CoreTypes.h"

DECLARE_BROADCAST_INTERFACE_TWOPARAMS(IDX12EndFrameInterface, OnEndFrame, u64, frameNum, u32, frameIdx);
