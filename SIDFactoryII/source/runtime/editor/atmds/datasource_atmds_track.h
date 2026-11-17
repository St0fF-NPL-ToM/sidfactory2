#pragma once

#include "../datasources/datasource_emulation_memory.h"
#include "../datasources/idatasource.h"

#include <memory>

namespace Emulation
{
	class CPUMemory;
}

namespace Editor
{
	class DataSourceTrackList : public DataSourceEmulationMemory
	{
	public:
		const int MaxEntryCount = 256;

		struct Entry
		{
			enum
			{
				none,
				loops,
				stops,
				song,
				volume,
				tempo
			} m_Flags { none };
			unsigned char m_Transposition;
			unsigned char m_Repetition;
			unsigned char m_SequenceIndex;
			unsigned char m_EvtExtraData;
		};

		struct PackResult
		{
			PackResult()
				: m_Data(nullptr)
				, m_DataLength(-1)
			{
			}

			PackResult(unsigned char* inData, int inDataLength)
				: m_Data(inData)
				, m_DataLength(inDataLength)
			{
			}

			std::shared_ptr<unsigned char> m_Data;
			int m_DataLength;
		};

		struct PackedDataEventPosition
		{
			bool m_IsValid;
			unsigned char m_Index;
			unsigned char m_Sequence;
			unsigned char m_CurrentTranspose;
		};

		DataSourceTrackList(Emulation::CPUMemory* inCPUMemory, unsigned short inSourceAddress, int inBlockSize);
		virtual ~DataSourceTrackList();

		Entry& operator[](int inIndex);
		const Entry& operator[](int inIndex) const;

		bool PushDataToSource() override;
		void PullDataFromSource() override;

		unsigned int GetPackedSize() const;
		unsigned int GetLength() const;

		bool CanIncreaseSize() const;
		void IncreaseSize();
		void DecreaseSize();
		void ComputeLength();

		unsigned char GetLoopIndex() const;
		void SetLoopIndex(unsigned char inLoopIndex);

		PackResult Pack() const;
		void SendPackedDataToBuffer(const PackResult& inPackResult);

		PackedDataEventPosition GetIndexInPackedData(int inIndex) const;

	private:
		void ClearEntries();

		void Unpack();

		unsigned int m_Length;

		Entry* m_Events;
		unsigned char* m_InternalBuffer;
		unsigned int m_PackedSize;
	};
}
