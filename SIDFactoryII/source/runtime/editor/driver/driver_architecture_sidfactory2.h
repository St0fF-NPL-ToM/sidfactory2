#pragma once

#include "idriver_architecture.h"

namespace Editor
{
	class DriverArchitectureSidFactory2 final : public IDriverArchitecture
	{
	public:
		void SetEmptySequence(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress) override;
		void SetEmptyOrderList(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress) override;
		void PostInitSetPlaybackIndices(std::vector<PlayMarkerInfo>& inPlayMarkerInfoList, Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo) override;

		static unsigned char GetDescriptorType()
		{
			return 0x00;
		}
		/* 	Extensions by St0fF: while these functions remain the same between all SF2-drivers, they
			do make up for the "real differences" between SF2 and other drivers.

			Abstracting ANY tables "away" is only logical, as only track- and sequence-interpretation makes differences.
			Specialized table visualizations would be a different topic, though.
		*/
		DataSourceOrderList::PackResult PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) override;
		DataSourceOrderList::PackedDataEventPosition GetOrderListIndex(int inIndex, const unsigned char* inData, const int inDataSize) const override;
		std::pair<int, int> UnpackOrderList(DataSourceOrderList& outData, unsigned char* m_Data) const override;
		DataSourceSequence::PackResult PackSequence(const DataSourceSequence& inData) override;
		DataSourceSequence::PackedDataEventPosition GetSequenceIndex(const DataSourceSequence& inData, int inIndex) const override;
		bool UnpackSequence(DataSourceSequence& outData) const override;
	};
}
