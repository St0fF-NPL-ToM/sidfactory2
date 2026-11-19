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

		DataSourceOrderList::PackResult PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) const override;
		DataSourceOrderList::PackedDataEventPosition GetOrderListIndex(int inIndex, const unsigned char* m_Data, const int m_PackedSize) const override;
		std::pair<int, int> UnpackOrderList(DataSourceOrderList& outData, unsigned char* m_Data) const override;
		DataSourceSequence::PackResult PackSequence(const DataSourceSequence& inData, unsigned char* m_InternalBuffer) const override;
		DataSourceSequence::PackedDataEventPosition GetSequenceIndex(int inEventPosition, const unsigned char* m_Data, const int m_PackedSize) const override;
		DataSourceSequence::UnpackResult UnpackSequence(DataSourceSequence& outData, unsigned char* m_Data) const override;

		static unsigned char GetDescriptorType()
		{
			return 0xac;
		}
	};
}
