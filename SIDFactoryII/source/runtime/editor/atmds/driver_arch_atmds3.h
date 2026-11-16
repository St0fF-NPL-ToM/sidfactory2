#pragma once

#include "idriver_architecture.h"

namespace Editor
{
	class DriverArchitectureATMDS3 final : public IDriverArchitecture
	{
	public:
		void SetEmptySequence(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress) override;
		void SetEmptyOrderList(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress) override;
		void PostInitSetPlaybackIndices(std::vector<PlayMarkerInfo>& inPlayMarkerInfoList, Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo) override;

		static unsigned char GetDescriptorType()
		{
			return 0xac;
		}
	};
}
