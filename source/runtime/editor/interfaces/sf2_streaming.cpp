/**
 *  SF2 Event redefinition
 */

#include "sf2_streaming.h"

#include "foundation/base/assert.h"

namespace Editor {

	DataSourceOrderList::PackResult SF2_OrderStreaming::PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) const
	{
		int end_index = -1;
		int end_marker = 0;
		int loop_index = -1;

		// Find end marker and loop index
		for (int i = 0; i < inData.MaxEntryCount; ++i)
		{
			const unsigned char val = inData[i].m_Transposition;
			if (val == 0xff || val == 0xfe)
			{
				end_index = i;
				end_marker = val;

				if (val == 0xff)
					loop_index = inData[i].m_SequenceIndex;

				break;
			}
		}

		// If end marker wasn't found, return with no content
		if (end_index == -1)
		{
			// Orderlist was without end marker
			return DataSourceOrderList::PackResult(nullptr, 0);
		}

		unsigned char current_transposition = 0;
		int index = 0;
		int packed_loop_index = 0;

		// Pack order list
		for (int i = 0; i < end_index; ++i)
		{
			unsigned char transposition = inData[i].m_Transposition;
			unsigned char sequence_index = inData[i].m_SequenceIndex;

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

			return DataSourceOrderList::PackResult(packed_data, index);
		}

		// .. otherwise return nothing
		return DataSourceOrderList::PackResult(nullptr, 0);
	}

	DataSourceOrderList::PackedDataEventPosition SF2_OrderStreaming::GetOrderListIndex(int inIndex, const unsigned char* m_Data, const int m_PackedSize) const
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

	std::pair<int, int> SF2_OrderStreaming::UnpackOrderList(DataSourceOrderList& m_Events, unsigned char* m_Data) const
	{
		int end_marker_index = -1;
		int entry_count = 0;
		unsigned char end_marker = 0;
		unsigned char loop_index = 0;
		// RESULTS:
		int m_PackedSize {}, m_Length {};
		// Find end and possibly loop index
		for (int i = 0; i < 0x100; ++i)
		{
			unsigned char value = m_Data[i];

			if (value == 0xff)
			{
				end_marker_index = i;
				end_marker = value;
				loop_index = m_Data[i + 1];

				++entry_count;

				m_PackedSize = i + 2;

				break;
			}
			if (value == 0xfe)
			{
				end_marker_index = i;
				end_marker = value;

				++entry_count;

				m_PackedSize = i + 1;

				break;
			}
			if (value < 0x80)
				++entry_count;
		}

		if (end_marker_index == -1 || entry_count >= m_Events.MaxEntryCount)
		{
			return { m_PackedSize, m_Length = 0 };
		}

		// Set order list length
		m_Length = entry_count;

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
		return { m_PackedSize, m_Length };
	}


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
	DataSourceSequence::PackResult SF2_SequenceStreaming::PackSequence(const DataSourceSequence& inData, unsigned char* m_InternalBuffer) const
	{
		unsigned char m_LastCommandSet = 0xff;
		unsigned char m_LastInstrumentSet = 0xff;

		int packIndex = 0;
		int lastDuration = -1;

		for (unsigned int i = 0; i < inData.GetLength(); ++i)
		{
			unsigned char instrument = inData[i].m_Instrument;
			unsigned char command = inData[i].m_Command;
			unsigned char note = inData[i].m_Note;

			// Look for next event
			int duration = 0;

			for (unsigned int j = i + 1; j < inData.GetLength(); ++j)
			{
				if (inData[j].m_Instrument != 0x80 || inData[j].m_Command != 0x80)
					break;

				if (note == 0)
				{
					if (inData[j].m_Note != 0)
						break;
				}
				else
				{
					if (inData[j].m_Note != 0x7e)
						break;
				}

				duration++;

				if (duration >= 0x0f)
					break;
			}

			bool bTieNote = (instrument == 0x90);

			if (command != 0x80)
			{
				m_InternalBuffer[packIndex++] = command;
				m_LastCommandSet = command & 0x3f;
			}
			if (instrument >= 0xa0)
			{
				m_InternalBuffer[packIndex++] = instrument;
				m_LastInstrumentSet = instrument & 0x1f;
			}

			if (lastDuration != duration || bTieNote)
			{
				m_InternalBuffer[packIndex++] = static_cast<unsigned char>((duration | 0x80) | (bTieNote ? 0x10 : 0x00));
				lastDuration = duration;
			}

			m_InternalBuffer[packIndex++] = note;

			i += duration;
		}

		// Create a buffer of the exact size of the packed data and copy the data into it
		if (!(!(packIndex > 0 && packIndex < 0xff)))
		{
			// Insert end mark
			m_InternalBuffer[packIndex++] = 0x7f;

			unsigned char* packed_data = new unsigned char[packIndex];

			for (int i = 0; i < packIndex; i++)
				packed_data[i] = m_InternalBuffer[i];

			return DataSourceSequence::PackResult(packed_data, packIndex);
		}

		return DataSourceSequence::PackResult();
	}

	DataSourceSequence::PackedDataEventPosition SF2_SequenceStreaming::GetSequenceIndex(int inEventPosition, const unsigned char* m_Data, const int m_PackedSize) const
	{
		int event_position = 0;
		int index = 0;
		unsigned char current_delta_tick = 0;
		unsigned char current_instrument = 0;

		while (index < static_cast<int>(m_PackedSize))
		{
			if (event_position >= inEventPosition)
				return { true, static_cast<unsigned char>(index), static_cast<unsigned char>(event_position - inEventPosition), current_delta_tick, current_instrument, m_Data[index] == 0x7f };

			while (m_Data[index] >= 0x80)
			{
				if (m_Data[index] < 0xa0)
					current_delta_tick = m_Data[index] & 0x0f;
				else if (m_Data[index] < 0xc0)
					current_instrument = m_Data[index];

				++index;
			}

			event_position += (current_delta_tick + 1);
			++index;
		}

		return { false, 0, 0, 0, 0, false };
	}

	DataSourceSequence::UnpackResult SF2_SequenceStreaming::UnpackSequence(DataSourceSequence& m_Events, unsigned char* m_Data) const
	{
		int event_index = 0;
		int duration = 0;
		bool tie_note = false;

		DataSourceSequence::UnpackResult r {};

		for (int i = 0; i < 0x100;)
		{
			unsigned char value = m_Data[i++];

			if (value == 0x7f)
			{
				r.m_PackedSize = i;
				break;
			}

			if (value >= 0xc0) // Command
			{
				m_Events[event_index].m_Command = value;
				r.m_LastCommandSet = value & 0x3f;
				value = m_Data[i++];

				FOUNDATION_ASSERT(i < 0x100);
			}
			else
				m_Events[event_index].m_Command = 0x80;

			if (value >= 0xa0) // Instrument
			{
				m_Events[event_index].m_Instrument = value;
				r.m_LastInstrumentSet = value & 0x1f;
				value = m_Data[i++];

				FOUNDATION_ASSERT(i < 0x100);
			}
			else
				m_Events[event_index].m_Instrument = 0x80;

			if (value >= 0x80)
			{
				duration = value & 0x0f;

				tie_note = (value & 0x10) != 0;

				if (tie_note)
					m_Events[event_index].m_Instrument = 0x90;

				value = m_Data[i++];

				FOUNDATION_ASSERT(i < 0x100);
			}

			FOUNDATION_ASSERT(value < 0x80);

			m_Events[event_index++].m_Note = value;

			for (int j = 0; j < duration; ++j)
			{
				m_Events[event_index].m_Command = 0x80;
				m_Events[event_index].m_Instrument = 0x80;
				m_Events[event_index].m_Note = value != 0x00 ? 0x7e : 0x00;

				event_index++;
			}
		}

		r.m_Length = event_index;
		r.m_PackingErrorState = false;
		return r;
	}

}
