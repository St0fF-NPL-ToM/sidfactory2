#include "driver_arch_atmds3.h"

#include "runtime/editor/driver/driver_info.h"
#include "runtime/emulation/cpumemory.h"

#include "foundation/base/assert.h"

#include <algorithm>
#include <set>

namespace Editor
{
	void DriverArchitectureATMDS3::SetEmptySequence(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress)
	{
		FOUNDATION_ASSERT(inCPUMemory != nullptr);
		FOUNDATION_ASSERT(inCPUMemory->IsLocked());

		unsigned char empty_sequence_data[] = { 0x90, 0xfe, 0xff }; // dur=10,w8,end
		inCPUMemory->SetData(inAddress, empty_sequence_data, sizeof(empty_sequence_data));
	}


	void DriverArchitectureATMDS3::SetEmptyOrderList(Emulation::CPUMemory* inCPUMemory, unsigned short inAddress)
	{
		FOUNDATION_ASSERT(inCPUMemory != nullptr);
		FOUNDATION_ASSERT(inCPUMemory->IsLocked());

		unsigned char empty_orderlist_data[] = { 0, 0xfe, 0 }; // as the others do it: empty seq, loop it.
		inCPUMemory->SetData(inAddress, empty_orderlist_data, sizeof(empty_orderlist_data));
	}

	void DriverArchitectureATMDS3::PostInitSetPlaybackIndices(std::vector<PlayMarkerInfo>& inPlayMarkerInfoList, Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo)
	{
		if (!m_CommandsCreated)
			GenerateCommandTable(inCPUMemory, inDriverInfo);

		const auto& driver_common = inDriverInfo.GetDriverCommon();
		const auto& music_data = inDriverInfo.GetMusicData();

		const unsigned char track_count = music_data.m_TrackCount;

		if (track_count == inPlayMarkerInfoList.size())
		{
			const unsigned short order_list_index_address = driver_common.m_OrderListIndexAddress;
			const unsigned short sequence_index_address = driver_common.m_SequenceIndexAddress;
			const unsigned short tick_counter_address = driver_common.m_TickCounterAddress;
			const unsigned short current_sequence_address = driver_common.m_CurrentSequenceAddress;
			const unsigned short current_transpose_address = driver_common.m_CurrentTransposeAddress;
			const unsigned short current_event_duration_address = driver_common.m_CurrentSequenceEventDurationAddress;
			const unsigned short next_instrument_address = driver_common.m_NextInstrumentAddress;

			for (int i = 0; i < 7 * music_data.m_TrackCount; i += 7)
			{
				(*inCPUMemory)[order_list_index_address + i] = inPlayMarkerInfoList[i].m_OrderListIndex;
				(*inCPUMemory)[sequence_index_address + i] = inPlayMarkerInfoList[i].m_SequenceIndex;
				(*inCPUMemory)[tick_counter_address + i] = inPlayMarkerInfoList[i].m_TickCounters;
				(*inCPUMemory)[current_sequence_address + i] = inPlayMarkerInfoList[i].m_CurrentSequence;
				(*inCPUMemory)[current_transpose_address + i] = inPlayMarkerInfoList[i].m_CurrentTranspose;
				(*inCPUMemory)[current_event_duration_address + i] = inPlayMarkerInfoList[i].m_CurrentSequenceEventDuration;
				(*inCPUMemory)[driver_common.m_SequenceInUseAddress + i] = inPlayMarkerInfoList[i].m_SequenceInUse ? 1 : 0;

				if (inPlayMarkerInfoList[i].m_NextInstrumentAddress >= 0x80)
					(*inCPUMemory)[next_instrument_address + i] = inPlayMarkerInfoList[i].m_NextInstrumentAddress;
			}
		}
	}

	void DriverArchitectureATMDS3::GenerateCommandTable(Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo)
	{
		std::vector<ATMDS_cmd> cmds;
		std::set<unsigned char> sectors_parsed;
		char tmp[256];
		// parse all songs, all tracks, all used sequences …
		for (int song = 0; song < 4; song++)
		{
			for (int track = 0; track < 3; track++)
			{
				// use the song track data pointer to retrieve the track address
				const auto tp = inCPUMemory->GetWord(0x4040 + song * 8 + track * 2);

				// then get the track into easily accessible local mem
				inCPUMemory->GetData(tp, tmp, 256);

				// parse one track only regarding commands!
				int rowCmd = -1;
				auto addRC = [&](ATMDS_cmd::cmd_use cmd, unsigned char val)
				{
					if (rowCmd < 0)
						rowCmd = cmds.size(), cmds.emplace_back(cmd, val);
					else
						cmds.back().set(cmd, val);
				};
				for (int pos = 0; pos < 256; pos++)
				{
					// SPD command?
					if ((tmp[pos] > 0x60u) && (tmp[pos] < 0x80u))
						addRC(ATMDS_cmd::uses_S, tmp[pos++] & 0x1f);
					// VOL command?
					if ((tmp[pos] > 0xe0u) && (tmp[pos] < 0xf0u))
						addRC(ATMDS_cmd::uses_V, tmp[pos++] & 0x0f);
					// Sector → reset row command
					if (tmp[pos] < 0x60u)
					{
						if (sectors_parsed.count(tmp[pos]) == 0)
						{
							unsigned char ts[256]; // acquire the sector
							inCPUMemory->GetData(0x5400 + 0x100 * tmp[pos], ts, 256);
							// now, parse the sector
							for (int sr {}, sp {}; sp < 256; sp++)
							{
								if (ts[sp] < 0x60u || ts[sp] >= 0xfcu) // row-end
								{
									sr++, rowCmd = -1;
									if (ts[sp] == 0xffu)
										break; // end marker
								}
								if (ts[sp] < 0x80u) // glide
								{
									addRC(ATMDS_cmd::uses_G, ts[sp++] & 0x1f); // glide table index
									addRC(ATMDS_cmd::none, ts[sp++]); // target note
									// after the glide-fx, sp points to the respective note
								}
								if (ts[sp] == 0xf6u)
									addRC(ATMDS_cmd::uses_FP, ts[++sp]);
								if (ts[sp] == 0xf7u)
									addRC(ATMDS_cmd::uses_PP, ts[++sp]);
								if (ts[sp] == 0xf8u)
									addRC(ATMDS_cmd::uses_FF, ts[++sp]);
								if (ts[sp] == 0xf9u)
									addRC(ATMDS_cmd::uses_SR, ts[++sp]);
								if (ts[sp] == 0xfau)
									addRC(ATMDS_cmd::uses_AD, ts[++sp]);
							}
							sectors_parsed.emplace(tmp[pos]);
						}
					}
					// Song/Jump/Stop
					if (tmp[pos] >= 0xF0u)
						break;
				}
			}
		}
		// Command Table sollte nun vollständig gebaut sein ... also übertragen.
		inCPUMemory->SetData(0x4000 - 10 * 128, cmds.data(), cmds.size() * sizeof(ATMDS_cmd));
		m_CommandsCreated = true;
	}

	DataSourceOrderList::PackResult DriverArchitectureATMDS3::PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) const
	{
		return {};
	}

	DataSourceOrderList::PackedDataEventPosition DriverArchitectureATMDS3::GetOrderListIndex(int inIndex, const unsigned char* m_Data, const int m_PackedSize) const
	{
		int pos {}, repeat {};
		DataSourceOrderList::PackedDataEventPosition ev { true, 0, 0xffu, 0xa0 };
		for (;; pos++)
		{
			if (m_Data[pos] < 0x60u)
			{
				ev.m_Sequence = m_Data[pos];
				++repeat;
				while (repeat--)
					if (inIndex == ev.m_Index)
						return ev;
					else
						ev.m_Index++;
				repeat = 0;
			}
			else if (m_Data[pos] >= 0x80u)
			{
				if (m_Data[pos] < 0xc0u) // transposition
					ev.m_CurrentTranspose = m_Data[pos] < 0xa0u ? m_Data[pos] + 0x20u : 0xa0u - (m_Data[pos] & 0x1fu);
				if (m_Data[pos] < 0xe0u) // repetition
					repeat = m_Data[pos] & 0x1fu;
				if (m_Data[pos] >= 0xf0u) // end of track in some way
				{
					if (inIndex > ev.m_Index)
					{
						ev.m_IsValid = false;
						ev.m_Sequence = 0xff;
					}
					return ev;
				}
			}
		}
	}

	std::pair<int, int> DriverArchitectureATMDS3::UnpackOrderList(DataSourceOrderList& outData, unsigned char* m_Data) const
	{
		int pos {}, repeat {}, row {}, rowpos {};
		unsigned char transpose {};
		DataSourceOrderList::Entry ce {};
		std::vector<int> row_begin;
		for (;; pos++)
		{
			if (m_Data[pos] < 0x60u)
			{
				ce.m_SequenceIndex = m_Data[pos];
				do
				{
					outData[row++] = ce;
					row_begin.push_back(rowpos);
				} while (repeat--);
				ce = { .m_Transposition = transpose, .m_SequenceIndex = 0xffu };
				rowpos = pos + 1;
				repeat = 0;
			}
			else if (m_Data[pos] >= 0x80u)
			{
				if (m_Data[pos] < 0xc0u) // transposition
					transpose = ce.m_Transposition = m_Data[pos] < 0xa0u ? m_Data[pos] + 0x20u : 0xa0u - (m_Data[pos] & 0x1fu);
				if (m_Data[pos] < 0xe0u) // repetition
					repeat = m_Data[pos] & 0x1fu;
				if (m_Data[pos] >= 0xf0u) // end of track in some way
				{
					if (m_Data[pos] == 0xfeu) // loop → we've recorded row pos for matching indices
					{
						auto it = std::find(row_begin.begin(), row_begin.end(), m_Data[++pos]) - row_begin.begin();
						ce.m_Transposition = 0xfe, ce.m_SequenceIndex = it;
					}
					else // stop → seems like JCH understands that command somehow
						// song will be interpreted as stop.
						ce.m_Transposition = 0xffu;
					outData[row++] = ce;
					break;
				}
			}
		}
		return { ++pos, row };
	}

	DataSourceSequence::PackResult DriverArchitectureATMDS3::PackSequence(const DataSourceSequence& inData, unsigned char* m_InternalBuffer) const { }

	DataSourceSequence::PackedDataEventPosition DriverArchitectureATMDS3::GetSequenceIndex(int inEventPosition, const unsigned char* m_Data, const int m_PackedSize) const { }

	DataSourceSequence::UnpackResult DriverArchitectureATMDS3::UnpackSequence(DataSourceSequence& outData, unsigned char* m_Data) const { }


}
