#include "runtime/editor/utilities/datasource_utils.h"
#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"
#include "foundation/base/assert.h"

namespace Editor
{
	namespace DataSourceUtils
	{
		void ClearSequences( std::vector< std::shared_ptr< DataSourceSequence > >& inSequenceDataSources )
		{
			bool is_first = true;
			for ( auto& sequence: inSequenceDataSources ) {
				sequence->ClearEvents();
				sequence->SetLength( is_first ? 0x10 : 0x01 );

				is_first					   = false;

				EventsPackResult packed_result = sequence->Pack();

				if ( packed_result.m_DataLength < 0x100 && packed_result.m_Data != nullptr )
					sequence->SendPackedDataToBuffer( packed_result );

				sequence->PushDataToSource();
			}
		}


		void CopySequence( const std::shared_ptr< DataSourceSequence >& inSource, unsigned int inOffset,
						   unsigned int inLength, const std::shared_ptr< DataSourceSequence >& inDestination )
		{
			FOUNDATION_ASSERT( inSource != nullptr );
			FOUNDATION_ASSERT( inDestination != nullptr );
			FOUNDATION_ASSERT( inSource->GetLength() <= inOffset + inLength );
			FOUNDATION_ASSERT( inDestination->GetLength() <= inLength );

			for ( unsigned int i = 0; i < inLength; ++i ) ( *inDestination )[ i ] = ( *inSource )[ i + inOffset ];
		}


		void ExpandSequences( std::vector< std::shared_ptr< DataSourceSequence > >& inSequenceDataSources )
		{
			for ( auto& sequence: inSequenceDataSources ) {
				ExpandSequence( sequence );
				EventsPackResult packed_result = sequence->Pack();

				if ( packed_result.m_DataLength < 0x100 && packed_result.m_Data != nullptr )
					sequence->SendPackedDataToBuffer( packed_result );

				sequence->PushDataToSource();
			}
		}


		void ExpandSequence( const std::shared_ptr< DataSourceSequence >& inSequence )
		{
			DataSourceSequence& sequence	  = *inSequence;
			DataSourceSequence	sequence_copy = DataSourceSequence( sequence );

			const int			length		  = sequence_copy.GetLength();
			inSequence->SetLength( length * 2 );

			int j = 0;
			for ( int i = 0; i < length; ++i ) {
				const DataSourceSequence::Event& event = sequence_copy[ i ];
				sequence[ j + 0 ]					   = event;
				sequence.ClearEvent( j + 1 );
				sequence.SetSentinel( j + 1,
									  sequence.GetSentinel( i, Semantic::Payload ) != SentinelType::Empty
										  ? SentinelType::Mark
										  : SentinelType::Empty,
									  Semantic::Payload );
				j += 2;
			}
		}


		void ShrinkSequence( const std::shared_ptr< DataSourceSequence >& inSequence )
		{
			DataSourceSequence& sequence	  = *inSequence;
			DataSourceSequence	sequence_copy = DataSourceSequence( sequence );

			const int			length		  = sequence_copy.GetLength();
			inSequence->SetLength( length / 2 );

			int j = 0;
			for ( int i = 0; i < length; i += 2 ) {
				const DataSourceSequence::Event& event = sequence_copy[ i ];
				sequence[ j ]						   = event;

				++j;
			}
		}


		void ClearOrderlist( std::vector< std::shared_ptr< DataSourceOrderList > >& inOrderListDataSources )
		{
			for ( auto& order_list: inOrderListDataSources ) {
				order_list->Flush();
				order_list->ComputeLength();

				EventsPackResult packed_result = order_list->Pack();

				if ( packed_result.m_DataLength < MAX_ORDER_PACKED_SIZE ) order_list->SendPackedDataToBuffer( packed_result );

				order_list->PushDataToSource();
			}
		}
	} // namespace DataSourceUtils
} // namespace Editor
