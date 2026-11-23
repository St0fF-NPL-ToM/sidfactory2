#include "datasource_sequence.h"
#include "foundation/base/assert.h"
#include "runtime/editor/driver/driver_info.h"
#include "runtime/editor/driver/driver_state.h"
#include "runtime/editor/interfaces/idriver_architecture.h"
#include "runtime/emulation/cpumemory.h"
#include <cstring>

namespace Editor
{
	const unsigned int DataSourceSequence::MaxEventCount = 1024;

	DataSourceSequence::DataSourceSequence(
		Emulation::CPUMemory* inCPUMemory,
		const Editor::DriverInfo& inDriverInfo,
		const Editor::DriverState& inDriverState,
		const unsigned char inSequenceIndex,
		unsigned short inSourceAddress,
		int inBlockSize)
		: DataSourceEmulationMemory(inCPUMemory, inSourceAddress, inBlockSize)
		, m_DriverInfo(inDriverInfo)
		, m_DriverState(inDriverState)
		, m_SequenceIndex(inSequenceIndex)
		, m_Length(0)
		, m_PackedSize(0)
		, m_PackingErrorState(false)
	{
		m_Events = new Event[MaxEventCount];
		m_InternalBuffer = new unsigned char[MaxEventCount * 3];

		ClearEvents();

		PullDataFromSource();
	}

	DataSourceSequence::DataSourceSequence(const DataSourceSequence& inOther)
		: DataSourceEmulationMemory(inOther.m_CPUMemory, inOther.m_SourceAddress, inOther.m_DataSize)
		, m_DriverInfo(inOther.m_DriverInfo)
		, m_DriverState(inOther.m_DriverState)
		, m_SequenceIndex(inOther.m_SequenceIndex)
		, m_Length(inOther.m_Length)
		, m_PackedSize(inOther.m_PackedSize)
		, m_PackingErrorState(inOther.m_PackingErrorState)
	{
		m_Events = new Event[MaxEventCount];
		m_InternalBuffer = new unsigned char[MaxEventCount * 3];

		for (int i = 0; i < MaxEventCount; ++i)
			m_Events[i] = inOther.m_Events[i];
	}

	DataSourceSequence::~DataSourceSequence()
	{
		delete[] m_Events;
		delete[] m_InternalBuffer;
	}

	//------------------------------------------------------------------------------------------------------------------

	void DataSourceSequence::operator=(const DataSourceSequence& inRhs)
	{
		for (int i = 0; i < MaxEventCount; ++i)
			m_Events[i] = inRhs.m_Events[i];

		m_Length = inRhs.m_Length;
	}

	DataSourceSequence::Event& DataSourceSequence::operator[](int inIndex)
	{
		FOUNDATION_ASSERT(inIndex >= 0);
		FOUNDATION_ASSERT(inIndex < MaxEventCount);

		return m_Events[inIndex];
	}

	const DataSourceSequence::Event& DataSourceSequence::operator[](int inIndex) const
	{
		FOUNDATION_ASSERT(inIndex >= 0);
		FOUNDATION_ASSERT(inIndex < MaxEventCount);

		return m_Events[inIndex];
	}

	//------------------------------------------------------------------------------------------------------------------

	bool DataSourceSequence::PushDataToSource()
	{
		FOUNDATION_ASSERT(m_CPUMemory != nullptr);
		FOUNDATION_ASSERT(m_CPUMemory->IsLocked());
		FOUNDATION_ASSERT(m_Data != nullptr);

		const bool may_push_sequence_data = [&]()
		{
			const unsigned short current_sequence_index_address = m_DriverInfo.GetDriverCommon().m_CurrentSequenceAddress;
			const unsigned short current_sequence_cursor_position_address = m_DriverInfo.GetDriverCommon().m_SequenceIndexAddress;

			if (m_DriverState.GetPlayState() == DriverState::PlayState::Playing)
			{
				for (int i = 0; i < m_DriverInfo.GetMusicData().m_TrackCount; ++i)
				{
					const unsigned char current_sequence_index = m_CPUMemory->GetByte(current_sequence_index_address + i);
					const unsigned char current_sequence_cursor_position = m_CPUMemory->GetByte(current_sequence_cursor_position_address + i);

					if (current_sequence_index == m_SequenceIndex && current_sequence_cursor_position != 0)
					{
						int change_from = 256;

						for (int j = 0; j < m_DataSize; ++j)
						{
							const char new_data = m_Data[j];
							const char old_data = (*m_CPUMemory)[m_SourceAddress + j];

							if ((new_data & 0xe0) == 0x80 || (old_data & 0xe0) == 0x80)
							{
								if (new_data != old_data)
								{
									change_from = j;
									break;
								}
							}
						}

						return current_sequence_cursor_position < change_from;
					}
				}
			}

			return true;
		}();

		if (may_push_sequence_data)
		{
			m_CPUMemory->SetData(m_SourceAddress, m_Data, m_DataSize);
			return true;
		}

		return false;
	}


	unsigned int DataSourceSequence::GetPackedSize() const
	{
		return m_PackedSize;
	}

	unsigned int DataSourceSequence::GetLength() const
	{
		return m_Length;
	}

	void DataSourceSequence::SetLength(unsigned int inLength)
	{
		FOUNDATION_ASSERT(inLength <= MaxEventCount);
		m_Length = inLength;
	}


	//------------------------------------------------------------------------------------------------------------------

	void DataSourceSequence::PullDataFromSource()
	{
		FOUNDATION_ASSERT(m_CPUMemory != nullptr);
		FOUNDATION_ASSERT(m_Data != nullptr);

		m_CPUMemory->Lock();
		m_CPUMemory->GetData(m_SourceAddress, m_Data, m_DataSize);
		m_CPUMemory->Unlock();

		ClearEvents();
		Unpack();
	}


	void DataSourceSequence::ClearEvents()
	{
		m_LastCommandSet = 0xff;
		m_LastInstrumentSet = 0xff;

		for (int i = 0; i < MaxEventCount; ++i)
			m_Events[i].Clear();
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

	void DataSourceSequence::Unpack()
	{
		ClearEvents();
		auto ur = m_DriverInfo.GetDriverArchitecture()->UnpackSequence(*this, m_Data);
		m_Length = ur.m_Length;
		m_PackedSize = ur.m_PackedSize;
		m_LastInstrumentSet = ur.m_LastInstrumentSet;
		m_LastCommandSet = ur.m_LastCommandSet;
		m_PackingErrorState = ur.m_PackingErrorState;
	}


	unsigned char DataSourceSequence::GetLastInstrumentSet() const
	{
		return m_LastInstrumentSet;
	}


	unsigned char DataSourceSequence::GetLastCommandSet() const
	{
		return m_LastCommandSet;
	}


	DataSourceSequence::PackResult DataSourceSequence::Pack()
	{
		auto res = m_DriverInfo.GetDriverArchitecture()->PackSequence(*this, m_InternalBuffer);
		m_PackingErrorState = (res.m_DataLength == 0);
		return res;
	}

	void DataSourceSequence::SendPackedDataToBuffer(const PackResult& inPackResult)
	{
		FOUNDATION_ASSERT(inPackResult.m_DataLength <= m_DataSize);

		memset(m_Data, 0, m_DataSize);
		memcpy(m_Data, &*inPackResult.m_Data, inPackResult.m_DataLength);

		m_PackedSize = inPackResult.m_DataLength;
	}


	bool DataSourceSequence::IsInErrorState() const
	{
		return m_PackingErrorState;
	}


	DataSourceSequence::PackedDataEventPosition DataSourceSequence::GetEventPositionInPackedData(int inEventPosition) const
	{
		return m_DriverInfo.GetDriverArchitecture()->GetSequenceIndex(inEventPosition, m_Data, m_PackedSize);
	}
}
