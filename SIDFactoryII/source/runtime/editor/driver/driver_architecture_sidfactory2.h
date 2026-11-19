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
		DataSourceOrderList::PackResult PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) const override;
		DataSourceOrderList::PackedDataEventPosition GetOrderListIndex(int inIndex, const unsigned char* inData, const int inDataSize) const override;
		std::pair<int, int> UnpackOrderList(DataSourceOrderList& outData, unsigned char* m_Data) const override;
		//------------------------------------------------------------------------------------------------------------------
		// Data format
		//
		// 00 		= Note off
		// 01 - 6f 	= Notes
		// 70 - 7d 	= Reserved
		// 7e 		= Note on
		// 7f		= End of sequence
		// 80 - 8f	= Duration
		// 90 - 9f	= Duration, note is tied! (That's the one coming this tick)
		// a0 - bf	= Set instrument ($00 - $1f)
		// c0 - ff	= Set command ($00 - $3f)
		//------------------------------------------------------------------------------------------------------------------
		DataSourceSequence::PackResult PackSequence(const DataSourceSequence& inData, unsigned char* m_InternalBuffer) const override;
		DataSourceSequence::PackedDataEventPosition GetSequenceIndex(int inEventPosition, const unsigned char* m_Data, const int m_PackedSize) const override;
		DataSourceSequence::UnpackResult UnpackSequence(DataSourceSequence& outData, unsigned char* m_Data) const override;
	};
}
