/**
 *  SF2 Event redefinition
 */

#include "runtime/editor/interfaces/ievent_definition.h"
#include "runtime/editor/datasources/idatasource_events.h"

namespace Editor
{
	struct SF2_OrderEvent : public IEventDefinition
	{
		SF2_OrderEvent();
		EventsPackResult   Pack( const IDataSourceEvents& inData, uint8_t* ouTernalBuffer ) const noexcept override;
		EventsUnpackResult Unpack( IDataSourceEvents& outData, uint8_t* inTernalBuffer ) const noexcept override;
		EventsPackedRowPos GetRowIndex( int inIndex, uint8_t* inData, uint16_t inDataSize ) const noexcept override;
	};


	struct SF2_SequenceEvent : public IEventDefinition
	{
		SF2_SequenceEvent();
		EventsPackResult   Pack( const IDataSourceEvents& inData, uint8_t* ouTernalBuffer ) const noexcept override;
		EventsUnpackResult Unpack( IDataSourceEvents& outData, uint8_t* inTernalBuffer ) const noexcept override;
		bool MayPushData( IDataSourceEvents&, const DriverInfo&, const DriverState& ) const noexcept override;
		EventsPackedRowPos GetRowIndex( int inIndex, uint8_t* inData, uint16_t inDataSize ) const noexcept override;
	};

} // namespace Editor
