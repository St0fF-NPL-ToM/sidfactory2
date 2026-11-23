#include "component_track_utils.h"

#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"

#include "foundation/base/assert.h"

namespace Editor
{
	namespace ComponentTrackUtils
	{
		FocusRow CalculateFocusRow( int inFocusRow, int inHeight )
		{ return FocusRow( { inFocusRow, inFocusRow, inHeight - inFocusRow - 1 } ); }


		unsigned int GetMaxEventPosition( const std::shared_ptr< DataSourceOrderList >&				  inOrderList,
										  const std::vector< std::shared_ptr< DataSourceSequence > >& inSequenceList )
		{ return GetEventPosOf( inOrderList->GetLength(), 0, inOrderList, inSequenceList ); }


		unsigned int GetEventPosOf( unsigned int												inOrderListIndex,
									unsigned int												inSequencePosition,
									const std::shared_ptr< DataSourceOrderList >&				inOrderList,
									const std::vector< std::shared_ptr< DataSourceSequence > >& inSequenceList )
		{
			FOUNDATION_ASSERT( inOrderList != nullptr );
			FOUNDATION_ASSERT( inOrderListIndex < inOrderList->GetLength() );
			FOUNDATION_ASSERT( inSequenceList.size() > 0 );

			unsigned int max_event_position = 0;

			for ( unsigned int i = 0; i < inOrderListIndex; ++i )
				if ( inOrderList->HasSequenceId( i ) )
					max_event_position += inSequenceList[ inOrderList->GetSequenceId( i ) ]->GetLength();

			return max_event_position + inSequencePosition;
		}
	} // namespace ComponentTrackUtils
} // namespace Editor
