#pragma once
/**
 *		SF2: driver_arch_ATMDS3
 *		=======================
 *		Adding a non-JCH/Laxity driver architecture to SIDFactoryII.
 *
 *		ATMDS 3.2 was mostly programmed in 1998 and 1999.  Due to St0fF's psychological state he
 *		couldn't "regard the tracker as a finished product", due to a few unfixed, known bugs.
 *
 *		Anyhow, ATMDS3.2 was not as far developed regarding optimal Order and Sequence Structure
 *		compared to JCH's NewPlayer architecture.  It also did not know about a command table,
 *		that indirection step was taken in ATMDS 4's unreleased drivers.
 *
 *		So - OrderLists in ATMDS are called Tracks.  They do contain the same information, as SF2
 *		Orderlists, but also a few more.
 *		- global volume, playback speed, and current song selection, as well as a "stop" can be
 *		  encoded, which reaches far into the Commands of SF2.
 *
 *		This leads to the fact, that either this additional information gets skipped, or we'd have
 *		to keep an architecture record binding additional track commands to the following sector,
 *		thus showing that sector executing a command for volume oder speed.
 *		The concept of internal songs - ATMDS called them "tunes" - has to be finished inside SF2,
 *		first.
 *
 *		Sectors - which would be Sequences in SF2 - on the other hand do not know about a command
 *		table.  There are a few built-in commands a Sector knows, such as:
 *		- Glide, TuneAD, TuneSR, TunePP, TuneFP, TuneFCF, Switch
 *		These will need to be transmogrified into some kind of a command table.
 *
 *		Command Tables:
 *		As there are 9 extra commands, 5 are "tuners", one calls a glide-fx to a target note and
 *		the other two coming from the Track (volume/speed) are Nybbles, one could make this work
 *		creating a table like:
 *		columns: bitmask | VOL | SPD | AD | SR | PP | FP | FF | GLD | TgtNote
 *
 *		Internal State:	bool m_CommandsCreated
 *		Internal Func:  void GenerateCommandTable
 *			triggered by: PostInitSetPlaybackIndices?
 *
 *		Command Tables in emulated memory: should be positioned outside the original ATMDS data
 *		range, e.g. ending at $4000.  In "my DDK" I just set it to ($4000-10*128), ColumnMajor.
 *
 */
#include "runtime/editor/interfaces/idriver_architecture.h"

namespace Editor
{
	struct ATMDS_cmd
	{
		enum cmd_use : unsigned char
		{
			none = 0,
			uses_AD = (1 << 0),
			uses_SR = (1 << 1),
			uses_PP = (1 << 2),
			uses_FP = (1 << 3),
			uses_FF = (1 << 4),
			uses_V = (1 << 5),
			uses_S = (1 << 6),
			uses_G = (1 << 7)
		} m_uses { none };
		unsigned char m_AD, m_SR, m_PP, m_FP, m_FF, m_V, m_S, m_G, m_T;
		ATMDS_cmd(cmd_use use, unsigned char val) { set(use, val); }
		void set(cmd_use use, unsigned char val)
		{ // clang-format off
			m_uses = cmd_use(use | m_uses);
			switch( use ) {
				case uses_AD:	m_AD = val;	break;
				case uses_SR:	m_SR = val;	break;
				case uses_PP:	m_PP = val;	break;
				case uses_FP:	m_FP = val;	break;
				case uses_FF:	m_FF = val;	break;
				case uses_V:	m_V  = val; break;
				case uses_S:	m_S  = val; break;
				case uses_G:	m_G  = val; break;
				case none:		m_T  = val; break;	// abusing cmd "none" for glide target setting!
			}
		} // clang-format on
	};

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

	private:
		void GenerateCommandTable(Emulation::CPUMemory* inCPUMemory, const DriverInfo& inDriverInfo);
		std::vector<ATMDS_cmd> m_CreatedCommands;
	};
}
