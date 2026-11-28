#pragma once

#include "datasource_emulation_memory.h"
#include "foundation/base/assert.h"
#include "runtime/editor/driver/driver_info.h"
#include "runtime/editor/driver/driver_state.h"
#include "runtime/editor/interfaces/ievent_definition.h"
#include "runtime/editor/global_constants.h"
#include "runtime/emulation/cpumemory.h"

namespace Emulation
{
	class CPUMemory;
}

namespace Editor
{
	class IDataSourceEvents : public DataSourceEmulationMemory
	{
	  public:
		using Event = IEvent;
		const uint16_t MaxEventCount;

		IDataSourceEvents( const IEventDefinition& inEventDefinition, Emulation::CPUMemory* inCPUMemory,
						   const Editor::DriverInfo& inDriverInfo, const Editor::DriverState& inDriverState,
						   uint16_t inSourceAddress, int inBlockSize, uint16_t inMaxLength, uint8_t inId = 0u )
			: DataSourceEmulationMemory( inCPUMemory, inSourceAddress, inBlockSize )
			, m_EventDef( inEventDefinition )
			, m_DriverInfo( inDriverInfo )
			, m_DriverState( inDriverState )
			, MaxEventCount( inMaxLength )
			, m_Id( inId )
		{
			m_Events		 = new Event[ MaxEventCount ];
			m_InternalBuffer = new unsigned char[ MaxEventCount * 2 + 1 ];
			PullDataFromSource();
		}

	  protected: // Copy-CTor can be made public in derived class if needed!
		IDataSourceEvents( const IDataSourceEvents& inOther )
			: DataSourceEmulationMemory( inOther )
			, MaxEventCount( inOther.MaxEventCount )
			, m_EventDef( inOther.m_EventDef )
			, m_DriverInfo( inOther.m_DriverInfo )
			, m_DriverState( inOther.m_DriverState )
			, m_Id( inOther.m_Id )
		{
			m_Events		 = new Event[ MaxEventCount ];
			m_InternalBuffer = new unsigned char[ MaxEventCount * 2 + 1 ];
			for ( int i = 0; i < MaxEventCount; ++i ) m_Events[ i ] = inOther.m_Events[ i ];
		}

	  public:
		virtual ~IDataSourceEvents()
		{
			delete[] m_Events;
			delete[] m_InternalBuffer;
		}


		Event& operator[]( int inIndex )
		{
			FOUNDATION_ASSERT( inIndex < MaxEventCount );
			FOUNDATION_ASSERT( m_Events != nullptr );

			return m_Events[ inIndex ];
		}
		const Event& operator[]( int inIndex ) const
		{
			FOUNDATION_ASSERT( inIndex < MaxEventCount );
			FOUNDATION_ASSERT( m_Events != nullptr );

			return m_Events[ inIndex ];
		}

		bool PushDataToSource() override
		{
			FOUNDATION_ASSERT( m_CPUMemory != nullptr );
			FOUNDATION_ASSERT( m_CPUMemory->IsLocked() );
			FOUNDATION_ASSERT( m_Data != nullptr );

			if ( m_EventDef.MayPushData( *this, m_DriverInfo, m_DriverState ) )
				return m_CPUMemory->SetData( m_SourceAddress, m_Data, m_DataSize ), true;
			else return false;
		}

		void PullDataFromSource() override
		{
			FOUNDATION_ASSERT( m_CPUMemory != nullptr );
			FOUNDATION_ASSERT( m_Data != nullptr );

			m_CPUMemory->Lock();
			m_CPUMemory->GetData( m_SourceAddress, m_Data, m_DataSize );
			m_CPUMemory->Unlock();

			Unpack();
		}

		uint8_t			 GetPackedSize() const { return m_PackedSize; }
		uint8_t			 GetLength() const { return m_Length; }

		EventsPackResult Pack()
		{
			auto res			= m_EventDef.Pack( *this, m_InternalBuffer );
			m_PackingErrorState = ( res.m_DataLength == 0 );
			return res;
		}

		EventsUnpackResult Unpack()
		{
			Clear();
			auto ur				= m_EventDef.Unpack( *this, m_Data );
			m_Length			= ur.m_Length;
			m_PackedSize		= ur.m_PackedSize;
			m_LastParameterSet	= ur.m_LastParameterSet;
			m_LastCommandSet	= ur.m_LastCommandSet;
			m_PackingErrorState = ur.m_PackingErrorState;
			return ur;
		}

		uint8_t GetLastParameterSet() const { return m_LastParameterSet; };
		uint8_t GetLastCommandSet() const { return m_LastCommandSet; }

		void	SendPackedDataToBuffer( const EventsPackResult& inPackResult )
		{
			FOUNDATION_ASSERT( inPackResult.m_DataLength <= m_DataSize );

			memset( m_Data, 0, m_DataSize );
			memcpy( m_Data, &*inPackResult.m_Data, inPackResult.m_DataLength );

			m_PackedSize = inPackResult.m_DataLength;
		}

		bool IsInErrorState() const { return m_PackingErrorState; }

		// using the event description to decide on "is end index":
		bool IsEndIndex( int inIndex ) const
		{
			return m_EventDef.IsParamSentinel( SentinelType::Loop, m_Events[ inIndex ] )
				   || m_EventDef.IsParamSentinel( SentinelType::Stop, m_Events[ inIndex ] )
				   || m_EventDef.IsParamSentinel( SentinelType::Start, m_Events[ inIndex ] )
				   || m_EventDef.IsPayloadSentinel( SentinelType::Stop, m_Events[ inIndex ] );
		}

		// This is true in case there is no "loop" sentinel active as parameter
		// and the payload value itself is in Range and not a Sentinel.
		bool	HasPayload( int inIndex ) const { return m_EventDef.HasPayload( m_Events[ inIndex ] ); }
		bool	HasCommand( int inIndex ) const { return m_EventDef.HasCommand( m_Events[ inIndex ] ); }
		bool	HasParameter( int inIndex ) const { return m_EventDef.HasParameter( m_Events[ inIndex ] ); }

		// Before reading the payload, you should actually check it is a payload.
		// Otherwise you might not be able to distinguish the returned value from a Sentinel.
		uint8_t GetPayload( int inIndex ) const
		{ return m_Events[ inIndex ].m_Payload - m_EventDef.m_Payload.m_Value.m_Offset; }
		uint8_t GetCommand( int inIndex ) const
		{ return m_Events[ inIndex ].m_Command - m_EventDef.m_Command.m_Value.m_Offset; }
		uint8_t GetParameter( int inIndex ) const
		{ return m_Events[ inIndex ].m_Param - m_EventDef.m_Param.m_Value.m_Offset; }

		// Get the type of sentinel an Event contains at the specified semantic
		// … also returns SentinelType::NoSentinel in case there is none.
		SentinelType GetSentinel( int inIndex, Semantic sem ) const noexcept
		{ return m_EventDef.WhichSentinel( f( inIndex, sem ), d( sem ) ); }

		void SetSentinel( int inIndex, SentinelType t, Semantic s ) noexcept
		{ r( inIndex, s ) = d( s ).m_Sentinels.GetSentinel( t ); }

		int	 GetValue( int inIndex, Semantic s ) const noexcept { return f( inIndex, s ) - GetOffset( s ); }
		void SetValue( uint8_t value, int inIndex, Semantic s ) noexcept { r( inIndex, s ) = value + GetOffset( s ); }

		uint8_t		GetMinimum( Semantic s ) const noexcept { return m_EventDef.GetMinimum( s ); }
		uint8_t		GetMaximum( Semantic s ) const noexcept { return m_EventDef.GetMaximum( s ); }
		uint8_t		GetOffset( Semantic s ) const noexcept { return m_EventDef.GetOffset( s ); }
		uint8_t		GetDefault( Semantic s ) const noexcept { return m_EventDef.GetDefault( s ); }

		// Clear a single event
		Event&		ClearEvent( int inIndex ) { return m_EventDef.ClearEvent( m_Events[ inIndex ] ); }
		// Clear the whole data source
		void		ClearEvents() { Clear(); }

		// In case a constant ID was given, this will be returned, here.  Otherwise, the value defaults to 0.
		uint8_t		GetId() const noexcept { return m_Id; }

		// Needed by helper functions:
		// → access to the emulated memory for making sure, the driver is not destabilized …
		uint8_t		Peek( uint16_t inAddress ) const noexcept { return m_CPUMemory->GetByte( inAddress ); }
		// → access to the internal data store
		uint8_t		GetData( int index ) const noexcept { return m_Data[ index ]; }
		int			GetDataSize() const noexcept { return m_DataSize; }

		// Query a Sentinel for its Output Text (returns nullptr if undefined)
		const char* GetSentinelText( SentinelType t, Semantic s ) const noexcept
		{ return m_EventDef.GetSentinelText( t, s ); };

		const SentinelDef&		  GetAllSentinels( Semantic s ) const noexcept { return d( s ).m_Sentinels; }
		// this could become necessary for other drivers, where packing a sequence takes further
		// details of the music data set as a whole into account.
		const Editor::DriverInfo& GetDriverInfo() const noexcept { return m_DriverInfo; }

	  protected:
		void Clear()
		{
			for ( int i{}; i < MaxEventCount; ++i ) m_EventDef.ClearEvent( m_Events[ i ] );
		}
		const FieldDefinition& d( Semantic s ) const noexcept { return m_EventDef.field( s ); }
		const uint8_t&		   f( int inIndex, Semantic s ) const noexcept
		{
			switch ( s ) { // clang-format off
				case Semantic::Payload: return m_Events[ inIndex ].m_Payload;
				case Semantic::Command: return m_Events[ inIndex ].m_Command;
				default: 				return m_Events[ inIndex ].m_Param;
			} // clang-format on
		}
		uint8_t& r( int inIndex, Semantic s ) noexcept
		{
			switch ( s ) { // clang-format off
				case Semantic::Payload: return m_Events[ inIndex ].m_Payload;
				case Semantic::Command: return m_Events[ inIndex ].m_Command;
				default: 				return m_Events[ inIndex ].m_Param;
			} // clang-format on
		}

		unsigned				   m_Length{};
		Event*					   m_Events;

		unsigned char*			   m_InternalBuffer;
		unsigned				   m_PackedSize;

		uint8_t					   m_Id;
		uint8_t					   m_LastParameterSet;
		uint8_t					   m_LastCommandSet;

		bool					   m_PackingErrorState{ false };

		const Editor::DriverInfo&  m_DriverInfo;
		const Editor::DriverState& m_DriverState;

	  public:
		const IEventDefinition& m_EventDef;
	};
} // namespace Editor
