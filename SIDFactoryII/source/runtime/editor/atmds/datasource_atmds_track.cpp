#include "datasource_atmds_track.h"
#include "foundation/base/assert.h"
#include "runtime/emulation/cpumemory.h"
#include <cstring>

namespace Editor
{
	DataSourceTrackList::DataSourceTrackList(Emulation::CPUMemory* inCPUMemory, unsigned short inSourceAddress, int inBlockSize)
		: DataSourceEmulationMemory(inCPUMemory, inSourceAddress, inBlockSize)
		, m_Length(0)
		, m_PackedSize(0)
	{
		m_Events = new Entry[MaxEntryCount];
		m_InternalBuffer = new unsigned char[MaxEntryCount * 2 + 1];

		ClearEntries();

		PullDataFromSource();
	}

	DataSourceTrackList::~DataSourceTrackList()
	{
		delete[] m_Events;
		delete[] m_InternalBuffer;
	}


	DataSourceTrackList::Entry& DataSourceTrackList::operator[](int inIndex)
	{
		FOUNDATION_ASSERT(inIndex < MaxEntryCount);
		FOUNDATION_ASSERT(m_Events != nullptr);

		return m_Events[inIndex];
	}


	const DataSourceTrackList::Entry& DataSourceTrackList::operator[](int inIndex) const
	{
		FOUNDATION_ASSERT(inIndex < MaxEntryCount);
		FOUNDATION_ASSERT(m_Events != nullptr);

		return m_Events[inIndex];
	}


	bool DataSourceTrackList::PushDataToSource()
	{
		FOUNDATION_ASSERT(m_CPUMemory != nullptr);
		FOUNDATION_ASSERT(m_CPUMemory->IsLocked());
		FOUNDATION_ASSERT(m_Data != nullptr);

		m_CPUMemory->SetData(m_SourceAddress, m_Data, m_DataSize);

		return true;
	}


	unsigned int DataSourceTrackList::GetPackedSize() const
	{
		return m_PackedSize;
	}


	unsigned int DataSourceTrackList::GetLength() const
	{
		return m_Length;
	}


	bool DataSourceTrackList::CanIncreaseSize() const
	{
		return m_Length < static_cast<unsigned int>(MaxEntryCount);
	}

	void DataSourceTrackList::IncreaseSize()
	{
		FOUNDATION_ASSERT(CanIncreaseSize());

		++m_Length;
	}

	void DataSourceTrackList::DecreaseSize()
	{
		FOUNDATION_ASSERT(m_Length > 2);

		--m_Length;
	}


	void DataSourceTrackList::ComputeLength()
	{
		for (int i = 0; i < MaxEntryCount; ++i)
		{
			if (m_Events[i].m_Transposition >= 0xfe)
			{
				m_Length = i + 1;
				return;
			}
		}

		m_Length = MaxEntryCount - 1;
	}


	unsigned char DataSourceTrackList::GetLoopIndex() const
	{
		if (m_Length > 0 && m_Events[m_Length - 1].m_Flags == Entry::loops)
			return m_Events[m_Length - 1].m_EvtExtraData;
		return m_Length;
	}


	void DataSourceTrackList::SetLoopIndex(unsigned char inLoopIndex)
	{
		if (m_Length > 0)
		{
			const bool isEnd = inLoopIndex == m_Length - 1;

			if (inLoopIndex < m_Length - 1)
			{
				m_Events[m_Length - 1].m_Transposition = 0xff;
				m_Events[m_Length - 1].m_SequenceIndex = inLoopIndex;
			}
			else
			{
				FOUNDATION_ASSERT(inLoopIndex == m_Length - 1);

				if (m_Events[m_Length - 1].m_Transposition != 0xfe)
				{
					m_Events[m_Length - 1].m_Transposition = 0xfe;
					m_Events[m_Length - 1].m_SequenceIndex = static_cast<unsigned int>(m_Length - 1);
				}
				else
				{
					m_Events[m_Length - 1].m_Transposition = 0xff;
					m_Events[m_Length - 1].m_SequenceIndex = 0;
				}
			}
		}
	}


	DataSourceTrackList::PackResult DataSourceTrackList::Pack() const
	{
		int end_index = -1;
		int end_marker = 0;
		int loop_index = -1;

		// Find end marker and loop index
		for (int i = 0; i < MaxEntryCount; ++i)
		{
			const unsigned char val = m_Events[i].m_Transposition;
			if (val == 0xff || val == 0xfe)
			{
				end_index = i;
				end_marker = val;

				if (val == 0xff)
					loop_index = m_Events[i].m_SequenceIndex;

				break;
			}
		}

		// If end marker wasn't found, return with no content
		if (end_index == -1)
		{
			// Orderlist was without end marker
			return PackResult(nullptr, 0);
		}

		unsigned char current_transposition = 0;
		int index = 0;
		int packed_loop_index = 0;

		// Pack order list
		for (int i = 0; i < end_index; ++i)
		{
			unsigned char transposition = m_Events[i].m_Transposition;
			unsigned char sequence_index = m_Events[i].m_SequenceIndex;

			// If transposition changed, or a loop index, add the transposition value to the order list
			if (i == loop_index || transposition != current_transposition)
			{
				if (i == loop_index)
					packed_loop_index = index;

				m_InternalBuffer[index++] = transposition;
				current_transposition = transposition;
			}

			m_InternalBuffer[index++] = sequence_index;
		}

		// Append the end marker
		FOUNDATION_ASSERT(end_marker >= 0 && end_marker < 0x100);
		m_InternalBuffer[index++] = static_cast<unsigned char>(end_marker);

		// .. and the loop point if the end marker is a loop mark
		if (end_marker == 0xff)
		{
			FOUNDATION_ASSERT(packed_loop_index >= 0 && packed_loop_index < 0x100);
			m_InternalBuffer[index++] = static_cast<unsigned char>(packed_loop_index);
		}

		// If packed data were produced, which is within the size limitations...
		if (index > 0 && index < 0x100)
		{
			// Create a buffer for the data, copy and return
			unsigned char* packed_data = new unsigned char[index];

			for (int i = 0; i < index; ++i)
				packed_data[i] = m_InternalBuffer[i];

			return PackResult(packed_data, index);
		}

		// .. otherwise return nothing
		return PackResult(nullptr, 0);
	}


	void DataSourceTrackList::SendPackedDataToBuffer(const PackResult& inPackResult)
	{
		FOUNDATION_ASSERT(inPackResult.m_DataLength <= m_DataSize);

		memset(m_Data, 0, m_DataSize);
		memcpy(m_Data, &*inPackResult.m_Data, inPackResult.m_DataLength);

		m_PackedSize = inPackResult.m_DataLength;
	}


	DataSourceTrackList::PackedDataEventPosition DataSourceTrackList::GetIndexInPackedData(int inIndex) const
	{
		int count = 0;
		unsigned char transpose = 0;
		unsigned char sequence = 0;

		for (int i = 0; i < static_cast<int>(m_PackedSize); ++i)
		{
			if (m_Data[i] < 0x80)
			{
				sequence = m_Data[i];

				++count;
				if (count == inIndex + 1)
				{
					const unsigned char next_event = m_Data[i + 1];
					const int return_index = next_event == 0xff ? m_Data[i + 2] : (i + 1); // If the next event is a loop mark, the next next event is the loop offset into the orderlist data.

					return { true, static_cast<unsigned char>(return_index), sequence, transpose };
				}
			}
			else
			{
				int transpose_value = static_cast<int>((m_Data[i] & 0x7f) - 0x20);
				transpose = static_cast<unsigned int>(transpose_value & 0xff);
			}
		}

		return { false, 0, 0, 0 };
	}


	void DataSourceTrackList::PullDataFromSource()
	{
		FOUNDATION_ASSERT(m_CPUMemory != nullptr);
		FOUNDATION_ASSERT(m_Data != nullptr);

		m_CPUMemory->Lock();
		m_CPUMemory->GetData(m_SourceAddress, m_Data, m_DataSize);
		m_CPUMemory->Unlock();

		ClearEntries();
		Unpack();
	}

	void DataSourceTrackList::ClearEntries()
	{
		for (int i = 0; i < MaxEntryCount; ++i)
			m_Events[i] = {};
	}

	void DataSourceTrackList::Unpack()
	{
		int end_marker_index = -1;
		int entry_count = 0;
		unsigned char end_marker = 0;
		unsigned char loop_index = 0;

		// finally some St0fFcode.
		int i {}, e {};
		bool inside { true };
		while (inside)
		{ // parse "row by row" - ATMDS 3.2 Track Parser
			// Tracks (comparably OrderLists in SF2) have much more options in ATMDS than in SF2.
			// Some of these options are considered nonsense by today's standards, but in the 90's
			// I thought it was a good idea …
			//	Track-Commands:
			//		Transpose: 	TRP+ / TRP-			as known from SF2, but optional per Row.
			//		Repeat:		REP#XX				repeats the row's sector N time (while 0 also
			//										 means repeat 0 times, e.g. play once!)
			//		Volume:		VOL#X 				as the name suggest, sets the master SID volume
			//										prior to playing the requested sector
			//		Song:		SNG#X 				Switches to Song#X
			//		Loop:		LOOP#X				Loops to position X inside this track (needs to
			// 										 set a pointer to the beginning of a row!)
			//		Stop:		STOP				Stops all order and sequence processing on this
			//										Channel.
			//		Speed:		SPD#X 				Set the tick-Speed, e.g. song tempo
			//		Sector:		xy
			//
			auto& row = m_Events[e];
			if (m_Data[i] == 0xff) // STOP found
			{
				// previous event will be the finisher
				m_Events[e > 0 ? e - 1 : e].m_Flags = Entry::stops;
				inside = false;
			}
			if (m_Data[i] == 0xfe) // LOOP found
			{
				// previous event will be the looper, but we'll have to find the e to the i
				m_Events[e > 0 ? e - 1 : e].m_Flags = Entry::loops;
				unsigned char lp = m_Data[++i], k = 0;
				for (int j = 0; k < lp; ++j)
					if (m_Data[j] < 0x60)
						++k;
				m_Events[e > 0 ? e - 1 : e].m_EvtExtraData = k;
				inside = false;
			}
			if (m_Data[i] >= 0xf0) // SNG#X found
			{
				// previous event will be the spawner
				m_Events[e > 0 ? e - 1 : e].m_Flags = Entry::song;
				m_Events[e > 0 ? e - 1 : e].m_EvtExtraData = m_Data[i] & 3;
				inside = false;
			}
			if (m_Data[i] >= 0xe0) // VOL#X found
			{
				row.m_Flags = Entry::volume;
				row.m_EvtExtraData = m_Data[i++] & 15;
			}
			if (m_Data[i] >= 0xc0) // REP#X found
				row.m_Repetition = m_Data[i++] & 31;
			if (m_Data[i] >= 0xa0) // TRP-X found
				row.m_Transposition = -(m_Data[i++] & 31);
			if (m_Data[i] >= 0x80) // TRP+X found
				row.m_Transposition = m_Data[i++] & 31;
			if (m_Data[i] >= 0x60) // SPD+X found
			{
				row.m_Flags = Entry::tempo;
				row.m_EvtExtraData = m_Data[i++] & 31;
			}
			if (m_Data[i] < 0x60)
			{ // Sector ID → last Item of Row!
				row.m_SequenceIndex = m_Data[i++];
				++e;
			}
		}
		m_PackedSize = i + 1;
		m_Length = e + 1;

		// Unpack from data source
		unsigned char current_transposition = 0;
		int unpacked_loop_index = 0;
		int index = 0;

		for (int i = 0; i < end_marker_index; ++i)
		{
			// Get the packed value
			unsigned char value = m_Data[i];

			// If the current index of the packed data is the loop point, store the index of the unpacked data
			if (i == loop_index)
				unpacked_loop_index = index;

			// If the value is a transpotion value, update the current transposition value
			if (value >= 0x80)
				current_transposition = value;
			else
			{
				//... otherwise, store the entry
				m_Events[index].m_Transposition = current_transposition;
				m_Events[index].m_SequenceIndex = value;

				++index;
			}
		}

		// Append endmarker, and loop index
		FOUNDATION_ASSERT(unpacked_loop_index >= 0 && unpacked_loop_index < 0x100);

		m_Events[index].m_Transposition = end_marker;
		m_Events[index].m_SequenceIndex = static_cast<unsigned char>(unpacked_loop_index);
	}
}
