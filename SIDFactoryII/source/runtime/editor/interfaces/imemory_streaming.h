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
		virtual bool IsPackedSequenceEmpty(int seq, const Emulation::IMemoryRandomReadAccess& m_Reader) const = 0;

		virtual ~ISequenceStreaming() = default;
	};

	// IReport replaces the driver_utilities query functions basing on internal driver knowledge.
	// The driver designer may want to apply some kind of better dirty-state-tracking.
	// The class is misplaced in this header file, though, it consumes above interfaces, thus
	// for a start it may live in here ...
	class DriverReport
	{
	protected: // internal storage to cache data
		union SeqDat
		{
			uint16_t value;
			struct
			{
				uint8_t usecnt;
				uint8_t size;
			} d;
		};
		std::vector<SeqDat> m_SequenceData;
		std::vector<uint16_t> m_OrderListLengths;
		uint8_t m_MaxInsUsed, m_MaxCommandUsed;
		bool m_Dirty { true };
		const Editor::DriverInfo& m_Driver;
		const Emulation::IMemoryRandomReadAccess& m_Reader;

		DriverReport& AcquireReport()
		{
			if (m_Dirty)
			{
				m_MaxInsUsed = m_MaxCommandUsed = 0;
				m_SequenceData.resize(m_Driver.GetMusicData().m_SequenceCount);
				m_OrderListLengths.clear();
				const unsigned char song_count = m_Driver.GetAuxilaryDataCollection().GetSongs().GetSongCount();
				// Get Music data descriptor
				const DriverInfo::MusicData& music_data = m_Driver.GetMusicData();
				for (int song = 0; song < song_count; ++song)
				{
					for (int track = 0; track < music_data.m_TrackCount; ++track)
					{

					}
				}
				m_Dirty = false;
			}
			return *this;
		}

	public:
		DriverReport(const Editor::DriverInfo& inDriverInfo, const Emulation::IMemoryRandomReadAccess& inMemoryReader)
			: m_Driver(inDriverInfo)
			, m_Reader(inMemoryReader)
		{
		}
		// please make it dirty yourself!
		void MakeDirty() { m_Dirty = true; }
		// queries
		uint8_t GetHighestSequenceIndexUsed()
		{
			AcquireReport();
			for (int s = m_SequenceData.size() - 1; s >= 0; --s)
				if (m_SequenceData[s].d.usecnt)
					return s;
			return 0;
		}
		std::vector<int> GetSequenceUsageCount()
		{
			AcquireReport();
			std::vector<int> res(m_SequenceData.size());
			std::transform(m_SequenceData.begin(), m_SequenceData.end(), res.begin(), [](const SeqDat& sd)
				{ return (int)sd.d.usecnt; });
			return res;
		}
		uint8_t GetFirstUnusedSequenceIndex()
		{
			AcquireReport();
			for (int s = 0; s < m_SequenceData.size(); ++s)
				if (m_SequenceData[s].d.usecnt == 0)
					return s;
			return 0xff; // all used
		}
		uint8_t GetFirstEmptySequenceIndex()
		{
			AcquireReport();
			for (int s = 0; s < m_SequenceData.size(); ++s)
				if (m_SequenceData[s].d.usecnt == 0 && m_Driver.GetDriverArchitecture().IsPackedSequenceEmpty(s))
					return s;
			return 0xff; // all used
		}
		uint8_t GetHighestInstrumentIndexUsed() { return AcquireReport().m_MaxInsUsed; }
		uint8_t GetHighestCommandIndexUsed() { return AcquireReport().m_MaxCommandUsed; }
		const std::vector<uint16_t>& GetOrderListsLength() { return AcquireReport().m_OrderListLengths; }
		uint16_t GetSequenceLength(uint16_t inSequenceIndex) { return AcquireReport().m_SequenceData[inSequenceIndex].d.size; }
		uint16_t GetEndOfMusicDataAddress();
		uint16_t GetEndOfFileAddress();
	};
}
