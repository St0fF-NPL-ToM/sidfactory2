#pragma once

#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"
#include <vector>

namespace Emulation
{
	class CPUMemory;
}

namespace Editor
{
	class DriverInfo;
	class IDriverArchitecture
	{
	public:
		struct PlayMarkerInfo
		{
			unsigned char m_OrderListIndex;
			unsigned char m_CurrentSequence;
			unsigned char m_CurrentTranspose;
			unsigned char m_SequenceIndex;
			unsigned char m_CurrentSequenceEventDuration;
			unsigned char m_TickCounters;
			unsigned char m_NextInstrumentAddress;
			bool m_SequenceInUse;
		};

		virtual ~IDriverArchitecture() { }

		virtual void SetEmptySequence(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress) = 0;
		virtual void SetEmptyOrderList(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress) = 0;
		virtual void PostInitSetPlaybackIndices(std::vector<PlayMarkerInfo>& inPlayMarkerInfoList, Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo) = 0;

		/* 	Extensions by St0fF: while these functions remain the same between all SF2-drivers, they
			do make up for the "real differences" between SF2 and other drivers.

			Abstracting ANY tables "away" is only logical, as only track- and sequence-interpretation makes differences.
			Specialized table visualizations would be a different topic, though.
		*/
		virtual DataSourceOrderList::PackResult PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) = 0;
		virtual DataSourceOrderList::PackedDataEventPosition GetOrderListIndex(int inIndex, const unsigned char* inData, const int inDataSize) const = 0;
		virtual std::pair<int, int> UnpackOrderList(DataSourceOrderList& outData, unsigned char* m_Data) const = 0;
		virtual DataSourceSequence::PackResult PackSequence(const DataSourceSequence& inData) = 0;
		virtual DataSourceSequence::PackedDataEventPosition GetSequenceIndex(const DataSourceSequence& inData, int inIndex) const = 0;
		virtual std::pair<int, int> UnpackSequence(DataSourceSequence& outData) const = 0;
	};
}
