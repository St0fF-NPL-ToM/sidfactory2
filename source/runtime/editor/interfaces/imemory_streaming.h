#pragma once
/* 	Extensions by St0fF: while these functions remain the same between all SF2-drivers, they
	do make up for the "real differences" between SF2 and other drivers.

	Abstracting ANY tables "away" is only logical, as only track- and sequence-interpretation makes differences.
	Specialized table visualizations would be a different topic, though.
*/

#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"
#include "runtime/emulation/imemoryrandomreadaccess.h"
#include "runtime/editor/driver/driver_info.h"
#include "runtime/editor/auxilarydata/auxilary_data_collection.h"
#include "runtime/editor/auxilarydata/auxilary_data_songs.h"
#include <algorithm>
#include <vector>

namespace Editor
{
	// First interface is for Order Lists. Every Driver has its own in-memory representation
	// and packing algorithm (or not). The Order Event Definition already explained
	// to the editor, which events are acceptable / which will be produced.
	struct IOrderStreaming
	{
		virtual DataSourceOrderList::PackResult PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) const = 0;
		virtual DataSourceOrderList::PackedDataEventPosition GetOrderListIndex(int inIndex, const unsigned char* m_Data, const int m_PackedSize) const = 0;
		virtual std::pair<int, int> UnpackOrderList(DataSourceOrderList& outData, unsigned char* m_Data) const = 0;
		virtual bool IsPackedOrderEmpty(int song, int track, const Emulation::IMemoryRandomReadAccess& m_Reader) const = 0;
		// Orderlist-Stream
		virtual ~IOrderStreaming() = default;
	};

	struct ISequenceStreaming
	{
		virtual DataSourceSequence::PackResult PackSequence(const DataSourceSequence& inData, unsigned char* m_InternalBuffer) const = 0;
		virtual bool MayPushSequenceData(const DataSourceSequence& inData) const = 0;
		virtual DataSourceSequence::PackedDataEventPosition GetSequenceIndex(int inEventPosition, const unsigned char* m_Data, const int m_PackedSize) const = 0;
		virtual DataSourceSequence::UnpackResult UnpackSequence(DataSourceSequence& outData, unsigned char* m_Data) const = 0;
		virtual bool IsPackedSequenceEmpty(int seq) const = 0;

		virtual ~ISequenceStreaming() = default;
	};
}
