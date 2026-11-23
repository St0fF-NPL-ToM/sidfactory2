#pragma once

#include "datasource_emulation_memory.h"
#include "runtime/editor/interfaces/ievent_definition.h"
#include "idatasource.h"

#include <memory>

namespace Emulation
{
	class CPUMemory;
}

namespace Editor
{
	class IDriverArchitecture;

	class DataSourceOrderList : public DataSourceEmulationMemory
	{
	  public:
		const int MaxEntryCount = 256;

		struct Entry : public IEvent
		{
			unsigned char& m_Transposition() { return m_Param; }
			unsigned char& m_SequenceIndex() { return m_Payload; }
		};

		struct PackResult
		{
			PackResult()
				: m_Data( nullptr )
				, m_DataLength( -1 )
			{}

			PackResult( unsigned char* inData, int inDataLength )
				: m_Data( inData )
				, m_DataLength( inDataLength )
			{}

			std::shared_ptr< unsigned char > m_Data;
			int								 m_DataLength;
		};

		struct PackedDataEventPosition
		{
			bool		  m_IsValid;
			unsigned char m_Index;
			unsigned char m_Sequence;
			unsigned char m_CurrentTranspose;
		};

		DataSourceOrderList( const IDriverArchitecture* inArch, Emulation::CPUMemory* inCPUMemory,
							 unsigned short inSourceAddress, int inBlockSize );
		virtual ~DataSourceOrderList();

		Entry&					operator[]( int inIndex );
		const Entry&			operator[]( int inIndex ) const;

		bool					PushDataToSource() override;
		void					PullDataFromSource() override;

		unsigned int			GetPackedSize() const;
		unsigned int			GetLength() const;

		bool					CanIncreaseSize() const;
		void					IncreaseSize();
		void					DecreaseSize();
		void					ComputeLength();

		unsigned char			GetLoopIndex() const;
		void					SetLoopIndex( unsigned char inLoopIndex );

		PackResult				Pack() const;
		void					SendPackedDataToBuffer( const PackResult& inPackResult );

		PackedDataEventPosition GetIndexInPackedData( int inIndex ) const;

		bool					IsEndIndex( int inIndex ) const;

	  private:
		void					   ClearEntries();

		void					   Unpack();

		unsigned int			   m_Length;

		Entry*					   m_Events;
		unsigned char*			   m_InternalBuffer;
		unsigned int			   m_PackedSize;

		const IDriverArchitecture* m_Arch;
	};
} // namespace Editor
