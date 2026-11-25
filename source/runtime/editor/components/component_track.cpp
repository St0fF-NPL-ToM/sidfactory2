#include "component_track.h"

#include "runtime/editor/global_constants.h"
#include "runtime/editor/auxilarydata/auxilary_data_collection.h"
#include "runtime/editor/auxilarydata/auxilary_data_editing_preferences.h"
#include "runtime/editor/cursor_control.h"
#include "runtime/editor/display_state.h"
#include "runtime/editor/components_manager.h"
#include "runtime/editor/components/component_tracks.h"
#include "runtime/editor/components/utils/orderlist_utils.h"
#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"
#include "runtime/editor/dialog/dialog_hex_value_input.h"
#include "runtime/editor/undo/undo.h"
#include "runtime/editor/undo/undo_componentdata/undo_componentdata_tracks.h"
#include "runtime/editor/utilities/editor_utils.h"
#include "runtime/editor/utilities/datasource_utils.h"
#include "runtime/editor/debug/debug_singleton.h"
#include "runtime/editor/datacopy/copypaste.h"
#include "runtime/editor/datacopy/datacopy_sequence.h"
#include "foundation/input/mouse.h"
#include "foundation/input/keyboard.h"
#include "foundation/input/keyboard_utils.h"
#include "foundation/graphics/textfield.h"
#include "utils/keyhook.h"
#include "utils/keyhookstore.h"
#include "utils/usercolors.h"

#include <string>
#include "foundation/base/assert.h"
#include "utils/logging.h"
#include "SDL_keycode.h"

using namespace Foundation;
using namespace Utility;

namespace Editor
{
	std::string ComponentTrack::ms_NotesSharp[ 12 ] = { "C-", "C#", "D-", "D#", "E-", "F-",
														"F#", "G-", "G#", "A-", "A#", "B-" };

	std::string ComponentTrack::ms_NotesFlat[ 12 ]	= { "C-", "Db", "D-", "Eb", "E-", "F-",
														"Gb", "G-", "Ab", "A-", "Bb", "B-" };

	int			GetSequenceLength( std::vector< std::shared_ptr< DataSourceSequence > >& inDataSourceSequenceList,
								   unsigned char										 inSequenceIndex,
								   int													 inFallbackReturnSize )
	{
		if ( inSequenceIndex < MAX_SEQUENCE_COUNT ) return inDataSourceSequenceList[ inSequenceIndex ]->GetLength();

		return inFallbackReturnSize;
	}


	//----------------------------------------------------------------------------------------------------------------------------------------

	ComponentTrack::EventPosDetails::EventPosDetails()
		: m_OrderListIndex( 0 )
		, m_SequenceIndex( 0 )
	{}

	void ComponentTrack::EventPosDetails::Set( unsigned int inOrderListIndex, unsigned int inSequenceIndex )
	{
		m_OrderListIndex = inOrderListIndex;
		m_SequenceIndex	 = inSequenceIndex;
	}

	unsigned int ComponentTrack::EventPosDetails::OrderListIndex() const
	{ return m_OrderListIndex; }

	unsigned int ComponentTrack::EventPosDetails::SequenceIndex() const
	{ return m_SequenceIndex; }

	//----------------------------------------------------------------------------------------------------------------------------------------

	ComponentTrack::ComponentTrack(
		int	  inID,
		int	  inGroupID,
		Undo* inUndo,
		std::shared_ptr< DataSourceOrderList >
																	inDataSourceOrderList,
		const std::vector< std::shared_ptr< DataSourceSequence > >& inDataSourceSequenceList,
		Foundation::TextField*										inTextField,
		const EditState&											inEditState,
		const Utility::KeyHookStore&								inKeyHookStore,
		const AuxilaryDataCollection&								inAuxilaryDataCollection,
		std::function< void( bool, int, int ) >
			inStatusReportFunction,
		std::function< unsigned char() >
			inGetFirstFreeSequenceIndexFunction,
		std::function< unsigned char() >
			inGetFirstEmptySequenceIndexFunction,
		int inX,
		int inY,
		int inHeight )
		: ComponentBase( inID, inGroupID, inUndo, inTextField, inX, inY, 15, inHeight )
		, m_EditState( inEditState )
		, m_AuxilaryDataPlayMarkers( inAuxilaryDataCollection )
		, m_StatusReportFunction( inStatusReportFunction )
		, m_GetFirstFreeSequenceIndexFunction( inGetFirstFreeSequenceIndexFunction )
		, m_GetFirstEmptySequenceIndexFunction( inGetFirstEmptySequenceIndexFunction )
		, m_DataSourceOrderList( inDataSourceOrderList )
		, m_DataSourceSequenceList( inDataSourceSequenceList )
		, m_CursorPos( 0 )
		, m_EventPos( 0xffffffff )
		, m_MaxEventPos( 0 )
		, m_FirstValidOrderListIndex( 0 )
		, m_FirstValidSequenceIndex( 0 )
		, m_HasFirstValid( false )
		, m_FocusModeOrderList( false )
		, m_TakingOrderListInput( false )
		, m_SequenceDataHasChanged( false )
		, m_HasDataChangeOrderList( false )
		, m_LocalDataChange( false )
		, m_IsMuted( false )
		, m_IsMarkingArea( false )
		, m_MarkingFromEventPos( 0 )
		, m_MarkingToEventPos( 0 )
	{
		UpdateMaxEventPos();

		m_FocusRow = ComponentTrackUtils::CalculateFocusRow( m_Dimensions.m_Height >> 1, m_Dimensions.m_Height );

		ConfigureKeyHooks( inKeyHookStore );
	}

	ComponentTrack::~ComponentTrack()
	{
		m_OrderListChangedEvent.Clear();
		m_SequenceChangedEvent.Clear();
		m_SequenceSplitEvent.Clear();
		m_OrderListIndexChangedEvent.Clear();
	}

	//----------------------------------------------------------------------------------------------------------------------------------------

	void ComponentTrack::SetPosition( const Foundation::Point& inPosition )
	{
		m_Position = inPosition;
		m_Rect	   = { m_Position, m_Dimensions };
	}

	void ComponentTrack::SetHeight( int inHeight )
	{
		m_Dimensions.m_Height = inHeight;
		m_Rect				  = { m_Position, m_Dimensions };

		m_FocusRow = ComponentTrackUtils::CalculateFocusRow( m_Dimensions.m_Height >> 1, m_Dimensions.m_Height );
	}

	//----------------------------------------------------------------------------------------------------------------------------------------

	void ComponentTrack::SetHasControl( GetControlType inGetControlType, CursorControl& inCursorControl )
	{
		ComponentBase::SetHasControl( inGetControlType, inCursorControl );

		if ( m_EventPos > m_MaxEventPos ) SetEventPosition( m_MaxEventPos );
		else {
			UpdateOrderListStatusReport();
			UpdateSequenceStatusReport();
		}

		const unsigned int order_list_index = m_EventPosDetails.OrderListIndex();
		if ( m_DataSourceOrderList->IsEndIndex( order_list_index ) )
			m_OrderListIndexChangedEvent.Execute( m_HasControl, order_list_index, 0xff );
		else
			m_OrderListIndexChangedEvent.Execute( m_HasControl, order_list_index,
												  m_DataSourceOrderList->GetPayload( order_list_index ) );
	}


	void ComponentTrack::ClearHasControl( CursorControl& inCursorControl )
	{
		ComponentBase::ClearHasControl( inCursorControl );

		if ( m_IsMarkingArea ) DoCancelMarking();

		const unsigned int order_list_index = m_EventPosDetails.OrderListIndex();

		if ( m_DataSourceOrderList->IsEndIndex( order_list_index ) )
			m_OrderListIndexChangedEvent.Execute( m_HasControl, order_list_index, 0xff );
		else
			m_OrderListIndexChangedEvent.Execute( m_HasControl, order_list_index,
												  m_DataSourceOrderList->GetPayload( order_list_index ) );
	}

	//----------------------------------------------------------------------------------------------------------------------------------------

	bool ComponentTrack::ConsumeInput( const Foundation::Keyboard& inKeyboard,
									   CursorControl&			   inCursorControl,
									   ComponentsManager&		   inComponentsManager )
	{
		bool consume	   = false;
		int	 new_event_pos = m_EventPos;

		if ( m_HasControl ) {
			if ( !m_EditState.IsPreventingSequenceEdit() ) {
				for ( const auto& key_event: inKeyboard.GetKeyEventList() ) {
					if ( Utility::ConsumeInputKeyHooks( key_event,
														inKeyboard.GetModiferMask(),
														m_KeyHooks,
														KeyHookContext( { inComponentsManager, new_event_pos } ) ) )
						consume = true;

					if ( m_FocusModeOrderList ) {
						if ( inKeyboard.GetModiferMask() == 0 ) {
							char value_key = KeyboardUtils::FilterHexDigit( key_event );

							if ( value_key != 0 ) {
								ApplyOrderListValueKey( value_key );
								consume = true;
							}
						}
					} else {
						if ( inKeyboard.GetModiferMask() == 0 ) {
							if ( IsCursorAtSequenceNote() ) {
								int note_value = EditorUtils::GetNoteValue( key_event, m_EditState.GetOctave() );

								if ( note_value != -1 ) {
									new_event_pos			 = ApplySequenceNoteValue( note_value );
									m_SequenceDataHasChanged = true;
									consume					 = true;
								}
							} else {
								char value_key = KeyboardUtils::FilterHexDigit( key_event );

								if ( value_key != 0 ) {
									new_event_pos			 = ApplySequenceValueKey( value_key );
									m_SequenceDataHasChanged = true;
									consume					 = true;
								}
							}
						}
					}
				}

				if ( new_event_pos != m_EventPos ) {
					// If by any means the event position changed, we need to cancel the order list input.
					CancelOrderListInputValue();

					// Set the new event position
					SetEventPosition( new_event_pos );
				}
			}

			ApplyCursorPosition( inCursorControl );
		}

		if ( m_SequenceDataHasChanged ) {
			m_SequenceChangedEvent.Execute();
			m_SequenceDataHasChanged = false;
		}

		if ( m_HasDataChangeOrderList ) { m_OrderListChangedEvent.Execute(); }

		return consume;
	}


	bool ComponentTrack::ConsumeInput( const Foundation::Mouse& inMouse,
									   bool						inModifierKeyMask,
									   CursorControl&			inCursorControl,
									   ComponentsManager&		inComponentsManager )
	{
		if ( inMouse.IsButtonPressed( Foundation::Mouse::Button::Left ) ) {
			Foundation::Point screen_position = inMouse.GetPosition();

			if ( ContainsPosition( screen_position ) ) {
				Point	   cell_position	   = GetCellPosition( screen_position );

				const int  event_pos_at_cursor = ( cell_position.m_Y - m_Position.m_Y ) + m_TopEventPos;
				const bool event_pos_at_cursor_outside_range =
					event_pos_at_cursor < 0 || event_pos_at_cursor >= m_MaxEventPos;

				if ( !event_pos_at_cursor_outside_range ) {
					int sequence_cursor_position_x = GetCursorPositionXFromSequenceCellX( cell_position.m_X );
					if ( sequence_cursor_position_x >= 0 ) {
						if ( m_FocusModeOrderList ) SetFocusModeOrderList( false );

						m_CursorPos = sequence_cursor_position_x;

						ApplyCursorPosition( inCursorControl );
					} else if ( IsEventPosStartOfSequence( event_pos_at_cursor ) ) {
						if ( m_EditState.IsPreventingSequenceEdit() ) return false;

						int orderlist_cursor_position_x = GetCursorPositionXFromOrderListCellX( cell_position.m_X );

						if ( orderlist_cursor_position_x >= 0 ) {
							if ( !m_FocusModeOrderList ) SetFocusModeOrderList( true );

							m_CursorPos = orderlist_cursor_position_x;
							ApplyCursorPosition( inCursorControl );
						}
					}
				}

				if ( !m_EditState.IsPreventingSequenceEdit() )
					SetEventPosition( std::min( m_MaxEventPos, std::max( 0, event_pos_at_cursor ) ) );

				return true;
			}
		}

		return false;
	}


	bool ComponentTrack::ConsumeNonExclusiveInput( const Foundation::Mouse& inMouse )
	{ return false; }


	void ComponentTrack::Refresh( const DisplayState& inDisplayState )
	{
		if ( m_RequireRefresh ) {
			m_RequireRefresh							  = false;

			const Color background_color				  = ToColor( UserColor::TrackBackground );
			const Color background_color_muted			  = ToColor( UserColor::TrackBackgroundMuted );
			const Color focus_line_background_color		  = ToColor( UserColor::TrackBackgroundFocusLine );
			const Color focus_line_background_color_muted = ToColor( UserColor::TrackBackgroundMutedFocusLine );
			const Color background_color_marking		  = ToColor( UserColor::TrackMarkingArea );
			const Color background_color_marking_muted	  = ToColor( UserColor::TrackMarkingAreaMuted );

			Rect		focus_line_rect					  = m_Rect;

			focus_line_rect.m_Position.m_Y += m_Rect.m_Dimensions.m_Height >> 1;
			focus_line_rect.m_Dimensions.m_Height = 1;

			m_TextField->ColorAreaBackground( m_IsMuted ? background_color_muted : background_color, m_Rect );
			m_TextField->ColorAreaBackground(
				m_IsMuted ? focus_line_background_color_muted : focus_line_background_color, focus_line_rect );
			m_TextField->ClearText( m_Rect );

			if ( m_HasFirstValid ) {
				const bool		   is_uppercase	 = inDisplayState.IsHexUppercase();

				const int		   top_event	 = m_TopEventPos;
				const int		   bottom_event	 = m_TopEventPos + m_Dimensions.m_Height;
				const unsigned int loop_index	 = static_cast< unsigned int >( m_DataSourceOrderList->GetLoopIndex() );

				int				   current_y	 = m_Position.m_Y;
				int				   current_event = top_event;

				// Draw marking
				if ( m_IsMarkingArea ) {
					int		   top							  = std::min( m_MarkingFromEventPos, m_MarkingToEventPos );
					int		   bottom						  = std::max( m_MarkingFromEventPos, m_MarkingToEventPos );

					int		   marking_top_screen_position	  = top - top_event;
					int		   marking_bottom_screen_position = ( bottom - top_event ) + 1;

					const bool out_of_screen = ( marking_top_screen_position < 0 && marking_bottom_screen_position < 0 )
											   || ( marking_top_screen_position >= m_Dimensions.m_Height
													&& marking_bottom_screen_position >= m_Dimensions.m_Height );

					if ( !out_of_screen ) {
						marking_top_screen_position = marking_top_screen_position < 0 ? 0 : marking_top_screen_position;
						marking_bottom_screen_position = marking_bottom_screen_position >= m_Dimensions.m_Height
															 ? m_Dimensions.m_Height
															 : marking_bottom_screen_position;

						Rect marking_rect			   = m_Rect;

						marking_rect.m_Position.m_Y += marking_top_screen_position;
						marking_rect.m_Dimensions.m_Height =
							marking_bottom_screen_position - marking_top_screen_position;
						marking_rect.m_Position.m_X += 5;
						marking_rect.m_Dimensions.m_Width -= 6;

						m_TextField->ColorAreaBackground(
							m_IsMuted ? background_color_marking_muted : background_color_marking, marking_rect );
					}
				}

				// Figure blank height
				if ( top_event < 0 ) {
					int blank_height = -top_event;
					current_y += blank_height;
					current_event += blank_height;
				}

				unsigned char current_instrument = 0xff;

				unsigned int  orderlist_index	 = m_FirstValidOrderListIndex;
				unsigned int  sequence_index	 = m_FirstValidSequenceIndex;

				{
					{
						const std::shared_ptr< DataSourceSequence >& sequence =
							m_DataSourceSequenceList[ m_DataSourceOrderList->GetPayload( orderlist_index ) ];

						for ( unsigned int i = 0; i < sequence_index; ++i )
							if ( sequence->HasParameter( i ) ) // sequence parameters are instruments / tie
								current_instrument = sequence->GetParameter( i );
					}

					if ( current_instrument == 0xff ) {
						// We didn't find the current instrument
						unsigned orderlist_index_highligh_check = orderlist_index;

						while ( orderlist_index_highligh_check > 0 && current_instrument == 0xff ) {
							--orderlist_index_highligh_check;

							const std::shared_ptr< DataSourceSequence >& sequence =
								m_DataSourceSequenceList[ m_DataSourceOrderList->GetPayload( orderlist_index ) ];

							if ( sequence->GetLength() > 0 ) {
								for ( int i = static_cast< int >( sequence->GetLength() ); i >= 0; --i ) {
									if ( sequence->HasParameter( i ) ) {
										current_instrument = sequence->GetParameter( i );
										break;
									}
								}
							}
						}
					}
				}

				// Draw sequence lines
				SequenceColors sequence_colors(
					{ ToColor( UserColor::SequenceError ),
					  ToColor( UserColor::SequenceInstrumentEmpty ),
					  ToColor( UserColor::SequenceInstrumentEmptyFocusLine ),
					  ToColor( UserColor::SequenceInstrumentTie ),
					  ToColor( UserColor::SequenceInstrumentTieFocusLine ),
					  ToColor( UserColor::SequenceInstrumentValue ),
					  ToColor( UserColor::SequenceInstrumentValueFocusLine ),
					  ToColor( UserColor::SequenceInstrumentValueSelectedHighlight ),
					  ToColor( UserColor::SequenceInstrumentValueSelectedHighlightFocusLine ),
					  ToColor( UserColor::SequenceCommandEmpty ),
					  ToColor( UserColor::SequenceCommandEmptyFocusLine ),
					  ToColor( UserColor::SequenceCommandValue ),
					  ToColor( UserColor::SequenceCommandValueFocusLine ),
					  ToColor( UserColor::SequenceCommandValueSelectedHighlight ),
					  ToColor( UserColor::SequenceCommandValueSelectedHighlightFocusLine ),
					  ToColor( UserColor::SequenceNoteEmpty ),
					  ToColor( UserColor::SequenceNoteEmptyFocusLine ),
					  ToColor( UserColor::SequenceNoteGateOn ),
					  ToColor( UserColor::SequenceNoteGateOnFocusLine ),
					  ToColor( UserColor::SequenceNoteValue ),
					  ToColor( UserColor::SequenceNoteValueFocusLine ),
					  ToColor( UserColor::SequenceNoteValueTied ),
					  ToColor( UserColor::SequenceNoteValueTiedFocusLine ),
					  ToColor( UserColor::SequenceNoteValueSelectedHighlight ),
					  ToColor( UserColor::SequenceNoteValueSelectedHighlightFocusLine ) } );

				const Color color_orderlist_end_loop		  = ToColor( UserColor::OrderlistEndLoop );
				const Color color_orderlist_end_stop		  = ToColor( UserColor::OrderlistEndStop );
				const Color color_orderlist_value			  = ToColor( UserColor::OrderlistValue );
				const Color color_orderlist_value_loop_marker = ToColor( UserColor::OrderlistValueLoopMarker );
				const Color color_orderlist_value_input		  = ToColor( UserColor::OrderlistValueInput );

				/* 	TODO:
					Scheint als wäre hier die richtige Stelle, wo aus Sentinels Displaydaten werden …
				 */
				const auto& order_list_entry				  = ( *m_DataSourceOrderList )[ orderlist_index ];

				while ( current_event < bottom_event ) {
					if ( sequence_index == 0 ) {
						if ( m_DataSourceOrderList->IsEndIndex( orderlist_index ) ) {
							switch ( m_DataSourceOrderList->GetSentinel( orderlist_index, Semantic::Parameter ) ) {
								case SentinelType::Loop:
									m_TextField->Print( m_Position.m_X, current_y, color_orderlist_end_loop,
														m_DataSourceOrderList->GetSentinelText( SentinelType::Loop,
																								Semantic::Parameter ) );
									break;
								case SentinelType::Stop:
									m_TextField->Print( m_Position.m_X, current_y, color_orderlist_end_stop,
														m_DataSourceOrderList->GetSentinelText( SentinelType::Stop,
																								Semantic::Parameter ) );
									break;
							}
						} else {
							if ( m_HasControl && m_TakingOrderListInput
								 && orderlist_index == m_EventPosDetails.OrderListIndex() )
							{
								m_TextField->PrintHexValue( m_Position.m_X,
															current_y,
															color_orderlist_value_input,
															is_uppercase,
															m_OrderListInputValue );
							} else {
								const bool	is_looping_point = ( orderlist_index == loop_index );
								const Color color =
									is_looping_point ? color_orderlist_value_loop_marker : color_orderlist_value;

								m_TextField->PrintHexValue( m_Position.m_X, current_y, color, is_uppercase,
															order_list_entry.m_Param );
								m_TextField->PrintHexValue( m_Position.m_X + 2, current_y, color, is_uppercase,
															order_list_entry.m_Payload );
							}
						}
					}

					if ( current_event >= m_MaxEventPos ) break;

					int transposition = static_cast< int >( order_list_entry.m_Param );
					FOUNDATION_ASSERT( transposition < 0xfe );

					const std::shared_ptr< DataSourceSequence >& sequence =
						m_DataSourceSequenceList[ order_list_entry.m_Payload ];

					DrawSequenceLine( m_Position.m_X + 5,
									  current_y,
									  sequence_colors,
									  is_uppercase,
									  sequence,
									  sequence_index,
									  transposition - 0xa0,
									  current_event == m_EventPos,
									  current_instrument );

					++current_y;
					++current_event;
					++sequence_index;

					if ( sequence_index >= sequence->GetLength() ) {
						++orderlist_index;
						sequence_index = 0;
					}
				}
			}
		}
	}


	void ComponentTrack::HandleDataChange()
	{
		if ( m_HasDataChange ) {
			if ( m_HasDataChangeOrderList ) {
				if ( m_DataSourceOrderList->PushDataToSource() ) m_HasDataChangeOrderList = false;

				SetEventPosDetails( m_EventPosDetails.OrderListIndex(), m_EventPosDetails.SequenceIndex(), true );
			}

			if ( !m_DataChangeSequenceIndexList.empty() ) {
				std::vector< unsigned char > skipped_data_pushes;
				for ( unsigned char sequence_index: m_DataChangeSequenceIndexList ) {
					const std::shared_ptr< DataSourceSequence >& sequence = m_DataSourceSequenceList[ sequence_index ];

					if ( !sequence->PushDataToSource() ) skipped_data_pushes.push_back( sequence_index );
				}

				m_DataChangeSequenceIndexList = skipped_data_pushes;
			}

			m_HasDataChange = !m_DataChangeSequenceIndexList.empty();

			if ( !m_HasDataChange || ( !m_HasDataChangeOrderList && m_LocalDataChange ) )
				AddUndoRecentModificationStep( false );

			m_LocalDataChange = false;
		}
	}


	void ComponentTrack::PullDataFromSource( const bool inFromUndo )
	{
		m_DataSourceOrderList->PullDataFromSource();
		UpdateMaxEventPos();
	}


	void ComponentTrack::PullSequenceDataFromSource()
	{
		for ( auto& sequence: m_DataSourceSequenceList ) sequence->PullDataFromSource();
	}


	void ComponentTrack::ExecuteInsertDeleteRule( const DriverInfo::TableInsertDeleteRule& inRule,
												  int									   inSourceTableID,
												  int									   inIndexPre,
												  int									   inIndexPost )
	{}


	void ComponentTrack::ExecuteAction( int inActionInput ) {}


	void ComponentTrack::ForceRefresh()
	{
		UpdateMaxEventPos();
		m_RequireRefresh = true;
	}


	//--------------------------------------------------------------------------------------

	int ComponentTrack::GetCursorPosition() const
	{ return m_CursorPos; }


	void ComponentTrack::SetCursorPosition( int inCursorPosition )
	{
		if ( m_TakingOrderListInput )
			m_CursorPos = inCursorPosition < 0 ? 0 : ( inCursorPosition > 3 ? 3 : inCursorPosition );
		else m_CursorPos = inCursorPosition < 0 ? 0 : ( inCursorPosition > 4 ? 4 : inCursorPosition );
	}


	//--------------------------------------------------------------------------------------

	int ComponentTrack::GetMaxEventPosition() const
	{ return m_MaxEventPos; }


	void ComponentTrack::SetEventPosition( int inEventPos, bool inForceOrderListIndexChangeEvent )
	{
		FOUNDATION_ASSERT( inEventPos >= 0 );

		m_EventPos				 = inEventPos;
		m_TopEventPos			 = inEventPos - m_FocusRow.m_RowsAbove;

		bool found_event_pos	 = false;
		bool found_top_event_pos = false;

		if ( m_TopEventPos <= 0 ) {
			m_FirstValidOrderListIndex = 0;
			m_FirstValidSequenceIndex  = 0;

			found_top_event_pos		   = true;
			m_HasFirstValid			   = true;
		}

		if ( m_TopEventPos > m_MaxEventPos ) {
			m_HasFirstValid = false;
		} else {
			int event_pos = 0;

			for ( unsigned int i = 0; i < m_DataSourceOrderList->GetLength(); ++i ) {
				unsigned char sequence_index  = m_DataSourceOrderList->GetSequenceId( i );
				int			  sequence_length = GetSequenceLength( m_DataSourceSequenceList, sequence_index, 1 );

				int			  next_event_pos  = event_pos + sequence_length;

				if ( !found_top_event_pos && m_TopEventPos >= event_pos && m_TopEventPos < next_event_pos ) {
					found_top_event_pos		   = true;

					m_FirstValidOrderListIndex = i;
					m_FirstValidSequenceIndex  = m_TopEventPos - event_pos;
				}

				if ( !found_event_pos && m_EventPos >= event_pos && m_EventPos < next_event_pos ) {
					found_event_pos = true;
					SetEventPosDetails( i, m_EventPos - event_pos, inForceOrderListIndexChangeEvent );

					break;
				}

				event_pos = next_event_pos;
			}

			m_HasFirstValid = true;
		}

		m_RequireRefresh = true;

		UpdateOrderListStatusReport();
		UpdateSequenceStatusReport();
	}

	int ComponentTrack::GetEventPosition() const
	{ return m_EventPos; }

	int ComponentTrack::GetEventPositionAtTopOfCurrentSequence() const
	{
		int event_pos = 0;

		for ( unsigned int i = 0; i < m_DataSourceOrderList->GetLength(); ++i ) {
			if ( m_DataSourceOrderList->IsEndIndex( i ) ) break;

			const std::shared_ptr< DataSourceSequence >& sequence =
				m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId( i ) ];

			const int next_event_pos = event_pos + sequence->GetLength();

			if ( m_EventPos >= event_pos && m_EventPos < next_event_pos ) break;

			event_pos = next_event_pos;
		}

		return event_pos;
	}

	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::SetFocusModeOrderList( bool inFocusModeOrderList )
	{
		if ( m_IsMarkingArea ) DoCancelMarking();

		m_FocusModeOrderList = inFocusModeOrderList;

		if ( m_FocusModeOrderList ) m_CursorPos = 0;
		else {
			m_CursorPos = 4;

			if ( m_TakingOrderListInput ) CancelOrderListInputValue();
		}

		// If the track has control, set the event pos to the beginning of the current sequence
		if ( m_HasControl && m_FocusModeOrderList ) {
			int event_pos = GetEventPositionAtTopOfCurrentSequence();
			SetEventPosition( event_pos, true );
		}

		UpdateOrderListStatusReport();
		UpdateSequenceStatusReport();
	}


	bool ComponentTrack::GetFocusModeOrderList() const
	{ return m_FocusModeOrderList; }


	bool ComponentTrack::IsTakingOrderListInput() const
	{ return m_TakingOrderListInput; }

	//--------------------------------------------------------------------------------------------------

	bool ComponentTrack::IsMuted() const
	{ return m_IsMuted; }

	void ComponentTrack::SetMuted( bool inMuted )
	{
		m_IsMuted		 = inMuted;
		m_RequireRefresh = true;
	}

	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::SetOrderlistLoopPoint()
	{
		if ( m_EventPos < m_MaxEventPos ) {
			EventPosDetails event_pos_details = GetEventPosDetails( m_EventPos );
			if ( event_pos_details.OrderListIndex() >= 0 ) {
				FOUNDATION_ASSERT( event_pos_details.OrderListIndex() < 0x100 );

				m_DataSourceOrderList->SetLoopIndex(
					static_cast< unsigned char >( event_pos_details.OrderListIndex() ) );

				OnOrderListChanged();

				m_HasDataChangeOrderList = true;
				m_HasDataChange			 = true;
				m_RequireRefresh		 = true;
			}
		} else {
			FOUNDATION_ASSERT( m_EventPos == m_MaxEventPos );

			m_DataSourceOrderList->SetLoopIndex(
				static_cast< unsigned char >( m_DataSourceOrderList->GetLength() - 1 ) );

			OnOrderListChanged();

			m_HasDataChangeOrderList = true;
			m_HasDataChange			 = true;
			m_RequireRefresh		 = true;
		}
	}


	int ComponentTrack::GetLoopEventPosition() const
	{
		const int loop_index = m_DataSourceOrderList->GetLoopIndex();

		if ( loop_index == 0 ) return 0;

		int event_position = 0;
		for ( int i = 0; i < loop_index; ++i ) {
			FOUNDATION_ASSERT( !m_DataSourceOrderList->IsEndIndex( i ) );
			const int sequence_length =
				m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId( i ) ]->GetLength();
			event_position += sequence_length;
		}

		return event_position;
	}


	//--------------------------------------------------------------------------------------------------

	bool ComponentTrack::ConsumeHasInputCausedSequenceDataChange()
	{
		const bool return_value	 = m_SequenceDataHasChanged;
		m_SequenceDataHasChanged = false;

		return return_value;
	}

	//--------------------------------------------------------------------------------------------------

	bool ComponentTrack::ComputePlaybackStateFromEventPosition(
		int inEventPos, std::vector< IDriverArchitecture::PlayMarkerInfo >& outOrderListIndices ) const
	{
		EventPosDetails								 event_pos_details = GetEventPosDetails( inEventPos );

		// Find position in packed order list
		DataSourceOrderList::PackedDataEventPosition order_list_event_position =
			m_DataSourceOrderList->GetIndexInPackedData( event_pos_details.OrderListIndex() );

		if ( order_list_event_position.m_IsValid ) {
			// Find index in packed sequence data and remaining ticks to next event in packed sequence data
			if ( !m_DataSourceOrderList->IsEndIndex( event_pos_details.OrderListIndex() ) ) {
				DataSourceSequence::PackedDataEventPosition event_position =
					m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId(
												  event_pos_details.OrderListIndex() ) ]
						->GetEventPositionInPackedData( event_pos_details.SequenceIndex() );

				if ( event_position.m_IsValid ) {
					if ( event_position.m_CurrentInstrument == 0 ) {
						// We didn't find the current instrument
						unsigned orderlist_index_highligh_check = event_pos_details.OrderListIndex();

						while ( orderlist_index_highligh_check > 0 && event_position.m_CurrentInstrument == 0x00 ) {
							--orderlist_index_highligh_check;

							const std::shared_ptr< DataSourceSequence >& sequence =
								m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId(
									orderlist_index_highligh_check ) ];

							if ( sequence->GetLength() > 0 ) {
								for ( int i = static_cast< int >( sequence->GetLength() ); i >= 0; --i ) {
									const DataSourceSequence::Event& event = ( *sequence )[ i ];
									if ( sequence->GetSentinel( i, Semantic::Parameter ) == SentinelType::NoSentinel ) {
										event_position.m_CurrentInstrument = event.m_Param;
										break;
									}
								}
							}
						}
					}
					outOrderListIndices.push_back(
						{ static_cast< unsigned char >( order_list_event_position.m_Index ),
						  order_list_event_position.m_Sequence, order_list_event_position.m_CurrentTranspose,
						  event_position.m_Index, event_position.m_CurrentDurationValue,
						  event_position.m_RemainingTicks, event_position.m_CurrentInstrument,
						  !event_position.m_NextIsEndOfSequence } );

					return true;
				}
			}
		}
		return false;
	}


	ComponentTrack::OrderListIndexChangedEvent& ComponentTrack::GetOrderListIndexChangedEvent()
	{ return m_OrderListIndexChangedEvent; }

	ComponentTrack::SequenceSplitEvent& ComponentTrack::GetSequenceSplitEvent()
	{ return m_SequenceSplitEvent; }

	ComponentTrack::SequenceChangedEvent& ComponentTrack::GetSequenceChangedEvent()
	{ return m_SequenceChangedEvent; }

	ComponentTrack::OrderListChangedEvent& ComponentTrack::GetOrderListChangedEvent()
	{ return m_OrderListChangedEvent; }

	ComponentTrack::ComputeMaxEventPosEvent& ComponentTrack::GetComputeMaxEventPosEvent()
	{ return m_ComputedMaxEventPosEvent; }

	//--------------------------------------------------------------------------------------------------
	// Undo handlers
	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::SetUndoHandlers( std::function< void( UndoComponentDataTableTracks& ) > inAddUndoStepHandler,
										  std::function< void( const UndoComponentDataTableTracks&, CursorControl& ) >
											  inOnUndoHandler )
	{
		m_AddUndoStepHandler = inAddUndoStepHandler;
		m_OnUndoHandler		 = inOnUndoHandler;
	}


	//--------------------------------------------------------------------------------------------------
	// Order list input
	//--------------------------------------------------------------------------------------------------

	bool ComponentTrack::BeginOrderListInput()
	{
		FOUNDATION_ASSERT( m_DataSourceOrderList != nullptr );
		FOUNDATION_ASSERT( m_DataSourceSequenceList.size() > 0 );
		FOUNDATION_ASSERT( !m_TakingOrderListInput );

		const auto orderlist_index = m_EventPosDetails.OrderListIndex();
		if ( m_DataSourceOrderList->GetSentinel( orderlist_index, Semantic::Parameter ) == SentinelType::NoSentinel ) {
			const auto& orderlist_entry = ( *m_DataSourceOrderList )[ orderlist_index ];
			m_OrderListInputValue =
				( static_cast< unsigned short >( orderlist_entry.m_Param ) << 8 ) | orderlist_entry.m_Payload;
			return m_TakingOrderListInput = true;
		}

		return false;
	}


	void ComponentTrack::ApplyOrderListValueKey( char inValueKey )
	{
		if ( m_TakingOrderListInput || BeginOrderListInput() ) {
			unsigned char value = EditorUtils::ConvertSingleCharHexValueToValue( inValueKey );

			switch ( m_CursorPos ) {
				case 0: value = value < 0x08 ? 0x08 : ( value > 0x0b ? 0x0b : value ); break;
				case 2: value = value > 0x07 ? 0x07 : value; break;
			}

			int shift = 4 * ( 3 - m_CursorPos );

			m_OrderListInputValue &= ~( 0x0f << shift );
			m_OrderListInputValue |= value << shift;

			DoKeyRight( false );

			m_RequireRefresh = true;
		}
	}


	void ComponentTrack::ApplyOrderListInputValue()
	{
		FOUNDATION_ASSERT( m_DataSourceOrderList != nullptr );
		FOUNDATION_ASSERT( m_DataSourceSequenceList.size() > 0 );
		FOUNDATION_ASSERT( m_TakingOrderListInput );

		AddUndoStep();

		DataSourceOrderList::Event& orderlist_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() ];

		orderlist_entry.m_Param						= static_cast< unsigned char >( m_OrderListInputValue >> 8 );
		orderlist_entry.m_Payload					= static_cast< unsigned char >( m_OrderListInputValue & 0xff );

		m_TakingOrderListInput						= false;

		OnOrderListChanged();
	}


	void ComponentTrack::CancelOrderListInputValue()
	{
		if ( m_TakingOrderListInput ) {
			m_TakingOrderListInput = false;
			m_RequireRefresh	   = true;
		}
	}


	//--------------------------------------------------------------------------------------------------
	// Draw sequence entry
	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::DrawSequenceLine( int											inX,
										   int											inY,
										   const SequenceColors&						inColors,
										   const bool									inIsHexUppercase,
										   const std::shared_ptr< DataSourceSequence >& inSequence,
										   unsigned int									inSequenceIndex,
										   int											inTransposition,
										   const bool									inIsFocusLine,
										   unsigned char&								ioCurrentInstrument )
	{
		const bool in_error_state = inSequence->IsInErrorState();

		// active sentinels:
		const auto par			  = inSequence->GetSentinel( inSequenceIndex, Semantic::Parameter );
		const auto cmd			  = inSequence->GetSentinel( inSequenceIndex, Semantic::Command );
		const auto val			  = inSequence->GetSentinel( inSequenceIndex, Semantic::Payload );

		if ( par == SentinelType::NoSentinel ) ioCurrentInstrument = inSequence->GetParameter( inSequenceIndex );

		const bool	is_selected_instrument = m_EditState.IsSelectedInstrument( ioCurrentInstrument );
		const bool	is_selected_command = cmd == SentinelType::NoSentinel
										  && m_EditState.IsSelectedCommand( inSequence->GetCommand( inSequenceIndex ) );

		const Point instrument_pos		= Point( inX + 0, inY );
		const Point command_pos			= Point( inX + 3, inY );
		const Point note_pos			= Point( inX + 6, inY );

		// Instrument column
		{
			const Color empty =
				in_error_state ? inColors.m_ErrorState
							   : ( inIsFocusLine ? inColors.m_InstrumentEmptyFocusLine : inColors.m_InstrumentEmpty );
			const Color tie_note = in_error_state ? inColors.m_ErrorState
												  : ( inIsFocusLine ? inColors.m_InstrumentTieNoteFocusLine
																	: inColors.m_InstrumentTieNote );
			const Color value =
				in_error_state
					? inColors.m_ErrorState
					: ( is_selected_instrument
							? ( inIsFocusLine ? inColors.m_InstrumentValueSelectedHighlightFocusLine
											  : inColors.m_InstrumentValueSelectedHighlight )
							: ( inIsFocusLine ? inColors.m_InstrumentValueFocusLine : inColors.m_InstrumentValue ) );

			if ( par == SentinelType::NoSentinel )
				m_TextField->Print( instrument_pos, value,
									ComponentTrack::ToHexValueString( ioCurrentInstrument, inIsHexUppercase ) );
			else if ( par == SentinelType::Mark || par == SentinelType::Empty )
				m_TextField->Print( instrument_pos, par == SentinelType::Mark ? tie_note : empty,
									inSequence->GetSentinelText( par, Semantic::Parameter ) );
			else m_TextField->Print( instrument_pos, inColors.m_ErrorState, "??" );
		}

		// Command column
		{
			const Color empty = in_error_state
									? inColors.m_ErrorState
									: ( inIsFocusLine ? inColors.m_CommandEmptyFocusLine : inColors.m_CommandEmpty );
			const Color value =
				in_error_state
					? inColors.m_ErrorState
					: ( is_selected_command
							? ( inIsFocusLine ? inColors.m_CommandValueSelectedHighlightFocusLine
											  : inColors.m_CommandValueSelectedHighlight )
							: ( inIsFocusLine ? inColors.m_CommandValueFocusLine : inColors.m_CommandValue ) );

			if ( cmd == SentinelType::Empty )
				m_TextField->Print( command_pos, empty,
									inSequence->GetSentinelText( SentinelType::Empty, Semantic::Command ) );
			else if ( cmd == SentinelType::NoSentinel )
				m_TextField->Print(
					command_pos, value,
					ComponentTrack::ToHexValueString( inSequence->GetValue( inSequenceIndex, Semantic::Command ),
													  inIsHexUppercase ) );
			else m_TextField->Print( command_pos, inColors.m_ErrorState, "??" );
		}

		// Note column
		{
			const bool	is_tie_note = par == SentinelType::Mark;

			const Color empty		= in_error_state
										  ? inColors.m_ErrorState
										  : ( inIsFocusLine ? inColors.m_NoteEmptyFocusLine : inColors.m_NoteEmpty );
			const Color gate_on		= in_error_state
										  ? inColors.m_ErrorState
										  : ( inIsFocusLine ? inColors.m_NoteGateOnFocusLine : inColors.m_NoteGateOn );
			const Color value =
				in_error_state
					? inColors.m_ErrorState
					: ( is_tie_note
							? ( inIsFocusLine ? inColors.m_NoteValueTiedFocusLine : inColors.m_NoteValueTied )
							: ( is_selected_instrument
									? ( inIsFocusLine ? inColors.m_NoteValueSelectedHighlightFocusLine
													  : inColors.m_NoteValueSelectedHighlight )
									: ( inIsFocusLine ? inColors.m_NoteValueFocusLine : inColors.m_NoteValue ) ) );

			int transposed_note = ( *inSequence )[ inSequenceIndex ].m_Payload + inTransposition;
			if ( val == SentinelType::NoSentinel && transposed_note >= 0x01 && transposed_note < 0x60 ) {
				unsigned char note	 = static_cast< unsigned char >( transposed_note % 12 );
				unsigned char octave = static_cast< unsigned char >( transposed_note / 12 );

				const AuxilaryDataEditingPreferences::NotationMode notation_mode =
					m_AuxilaryDataPlayMarkers.GetEditingPreferences().GetNotationMode();

				const std::string& note_string =
					( notation_mode == AuxilaryDataEditingPreferences::NotationMode::Sharp )
						? ComponentTrack::ms_NotesSharp[ note ]
						: ComponentTrack::ms_NotesFlat[ note ];

				m_TextField->Print( note_pos, value, note_string + std::to_string( octave ) );
			} else if ( auto txt = inSequence->GetSentinelText( val, Semantic::Payload ) ) {
				m_TextField->Print( note_pos, val == SentinelType::Empty ? empty : gate_on, txt );
			} else m_TextField->Print( note_pos, inColors.m_ErrorState, "???" );
		}
	}


	bool ComponentTrack::IsCursorAtSequenceInstrument() const
	{ return ( !m_FocusModeOrderList && m_CursorPos < 2 ); }


	bool ComponentTrack::IsCursorAtSequenceCommand() const
	{ return ( !m_FocusModeOrderList && m_CursorPos >= 2 && m_CursorPos < 4 ); }


	bool ComponentTrack::IsCursorAtSequenceNote() const
	{ return ( !m_FocusModeOrderList && m_CursorPos >= 4 ); }


	unsigned int ComponentTrack::GetSequenceSize() const
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sid = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );

			return m_DataSourceSequenceList[ sid ]->GetLength();
		}
		return 0;
	}


	int ComponentTrack::ApplySequenceValueKey( char inValueKey )
	{
		FOUNDATION_ASSERT( !IsCursorAtSequenceNote() );

		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const std::shared_ptr< DataSourceSequence >& sequence_data_source =
				m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() ) ];
			const auto				   sequence_index = m_EventPosDetails.SequenceIndex();

			// Why the heck two sources of truth?
			DataSourceSequence::Event& event		  = ( *sequence_data_source )[ m_EventPosDetails.SequenceIndex() ];

			unsigned char			   value		  = EditorUtils::ConvertSingleCharHexValueToValue( inValueKey );
			bool					   change_made	  = false;

			if ( IsCursorAtSequenceInstrument() ) {
				if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

				unsigned char oi = sequence_data_source->HasParameter( sequence_index )
									   ? sequence_data_source->GetParameter( sequence_index )
									   : 0,
							  instrument{ oi };

				if ( ( m_CursorPos & 1 ) == 0 ) {
					if ( value > ( sequence_data_source->GetMaximum( Semantic::Parameter ) >> 4 ) ) return m_EventPos;

					instrument = ( instrument & 0x0f ) | ( value << 4 );
				} else instrument = ( instrument & 0xf0 ) | value;

				if ( oi != instrument ) {
					sequence_data_source->SetValue( instrument, sequence_index, Semantic::Parameter );
					change_made = true;
				}
			} else if ( IsCursorAtSequenceCommand() ) {
				if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

				unsigned char oc = sequence_data_source->HasCommand( sequence_index )
									   ? sequence_data_source->GetCommand( sequence_index )
									   : 0,
							  command{ oc };

				if ( ( m_CursorPos & 1 ) == 0 ) {
					if ( value > ( sequence_data_source->GetMaximum( Semantic::Command ) >> 4 ) ) return m_EventPos;

					command = ( command & 0x0f ) | ( value << 4 );
				} else command = ( command & 0xf0 ) | value;

				if ( oc != command || !sequence_data_source->HasCommand( sequence_index ) ) {
					sequence_data_source->SetValue( command, sequence_index, Semantic::Command );
					change_made = true;
				}
			}

			if ( change_made ) {
				DoKeyRight( false );
				OnSequenceChanged( sequence_data_source->GetId() );
			}
		}

		return m_EventPos;
	}


	int ComponentTrack::ApplySequenceNoteValue( int inNoteValue )
	{
		FOUNDATION_ASSERT( IsCursorAtSequenceNote() );

		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const std::shared_ptr< DataSourceSequence >& sequence_data_source =
				m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() ) ];
			const auto sequence_index = m_EventPosDetails.SequenceIndex();

			const int  transpose	  = m_DataSourceOrderList->GetParameter( m_EventPosDetails.OrderListIndex() );
			int		   note_value	  = inNoteValue - transpose;

			if ( note_value > 0 && note_value < 0x60 ) {
				if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

				if ( sequence_data_source->GetPayload( sequence_index ) != note_value ) {
					sequence_data_source->SetValue( note_value, sequence_index, Semantic::Payload );
					OnSequenceChanged( sequence_data_source->GetId() );
				}

				UpdateSequenceStatusReport();

				return DoKeyDown();
			}
		}

		return m_EventPos;
	}


	int ComponentTrack::ApplySequenceHoldNoteValue()
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();

			if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

			// for use with trackers that know more than "hold" or "pause": switch thru the options
			const auto& as	= sequence_data_source->GetAllSentinels( Semantic::Payload );
			const auto	cs	= sequence_data_source->GetSentinel( sequence_index, Semantic::Payload );
			auto		nxt = ( as.SentinelPos( cs ) + 1 ) % as.m_Count;
			while ( as.m_Types[ nxt ] == SentinelType::NoSentinel || as.m_Types[ nxt ] == SentinelType::Loop
					|| as.m_Types[ nxt ] == SentinelType::Stop )
				nxt = ( nxt + 1 ) % as.m_Count;

			sequence_data_source->SetSentinel( sequence_index, as.m_Types[ nxt ], Semantic::Payload );
			OnSequenceChanged( sequence_id );

			return DoKeyDown();
		}

		return m_EventPos;
	}


	void ComponentTrack::ToggleSequenceHoldNoteValueInMarkedArea()
	{
		AddUndoStep();

		std::vector< unsigned char > modified_sequence_indices = ForEachEventInMarkedRange(
			[]( DataSourceSequence::Event& inEvent, int index )
			{
				// this will stay a SF2 specialty for now!  Also I think this command is only needed
				// because SF2 does this: please select gate mask state explicitly.
				// If you simply give a "switch something" command, it's not nicely editable in case the
				// instrument keeps the gate on all the time itself, but I believe it packs better.  Also
				// it saves you from manually resetting the global voice gate state on the next tock.
				if ( inEvent.m_Payload == 0x00 || inEvent.m_Payload == 0x7e ) inEvent.m_Payload ^= 0x7e;
			} );

		for ( auto modified_sequence_index: modified_sequence_indices ) OnSequenceChanged( modified_sequence_index );
	}


	bool ComponentTrack::ApplyTranspose( char inDelta )
	{
		if ( m_IsMarkingArea ) return ApplyTransposeInMarkedArea( inDelta );

		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();
			const auto									 sequence_length	  = sequence_data_source->GetLength();

			// Validate if transposition works out correctly
			for ( int i = 0; i < sequence_length; ++i ) {
				if ( sequence_data_source->HasPayload( i ) ) {
					int transposed = sequence_data_source->GetPayload( i ) + inDelta;
					if ( transposed <= 0 || transposed >= 0x60 ) return false;
				}
			}
			// Do transpose
			if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

			for ( int i = 0; i < sequence_length; ++i )
				if ( sequence_data_source->HasPayload( i ) )
					sequence_data_source->SetValue( sequence_data_source->GetPayload( i ) + inDelta, i,
													Semantic::Payload );
			OnSequenceChanged( sequence_id );

			return true;
		}

		return false;
	}


	bool ComponentTrack::ApplyTransposeInMarkedArea( char inDelta )
	{
		FOUNDATION_ASSERT( m_IsMarkingArea );

		bool	   valid = true;

		const auto mi	 = m_DataSourceSequenceList[ 0 ]->GetMinimum( Semantic::Payload );
		const auto ma	 = m_DataSourceSequenceList[ 0 ]->GetMaximum( Semantic::Payload );
		const auto mu	 = m_DataSourceSequenceList[ 0 ]->GetOffset( Semantic::Payload );

		ForEachEventInMarkedRange(
			[ & ]( DataSourceSequence::Event& inEvent, int index )
			{
				if ( inEvent.m_Payload >= mi + mu && inEvent.m_Payload <= ma + mu ) {
					char transposed_note = static_cast< char >( inEvent.m_Payload ) + inDelta;
					if ( transposed_note > ma + mu || transposed_note < mi + mu ) valid = false;
				}
			} );

		if ( valid ) {
			AddUndoStep();

			std::vector< unsigned char > modified_sequences = ForEachEventInMarkedRange(
				[ & ]( DataSourceSequence::Event& inEvent, int index )
				{
					if ( inEvent.m_Payload >= mi + mu && inEvent.m_Payload <= ma + mu )
						inEvent.m_Payload = static_cast< unsigned char >( inEvent.m_Payload + inDelta );
				} );

			for ( auto modified_sequence: modified_sequences ) OnSequenceChanged( modified_sequence );
		}

		return valid;
	}


	void ComponentTrack::ToggleSequenceHoldNoteUntilEvent( bool inDown )
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();
			const auto									 sequence_length	  = sequence_data_source->GetLength();

			if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

			int	 event_pos_index = sequence_index;
			auto sen			 = sequence_data_source->GetSentinel( event_pos_index, Semantic::Payload );
			if ( sen == SentinelType::NoSentinel ) {
				event_pos_index += inDown ? 1 : -1;
				if ( event_pos_index < 0 || event_pos_index >= static_cast< int >( sequence_data_source->GetLength() ) )
					return;
				sen = sequence_data_source->GetSentinel( event_pos_index, Semantic::Payload );
			}

			const bool set_gate = ( sen == SentinelType::Empty );

			bool	   changed	= false;

			int		   i		= event_pos_index;

			while ( i >= 0 && i < static_cast< int >( sequence_data_source->GetLength() ) ) {
				const auto si = sequence_data_source->GetSentinel( i, Semantic::Payload );
				if ( set_gate ) {
					if ( si != SentinelType::Empty ) break;
					sequence_data_source->SetSentinel( i, SentinelType::Mark, Semantic::Payload );
					changed = true;
				} else {
					if ( si != SentinelType::Mark ) break;
					sequence_data_source->SetSentinel( i, SentinelType::Empty, Semantic::Payload );
					changed = true;
				}

				i += inDown ? 1 : -1;
			}

			if ( changed ) OnSequenceChanged( sequence_id );
		}
	}


	void ComponentTrack::ToggleSequenceTieNote()
	{
		if ( m_IsMarkingArea ) return ToggleSequenceTieNoteInMarkedArea();

		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();

			AddUndoStep();

			SentinelType ns{ SentinelType::NoSentinel };
			switch ( sequence_data_source->GetSentinel( sequence_index, Semantic::Parameter ) ) {
				case SentinelType::Mark: ns = SentinelType::Empty; break;
				default: // another sentinel type or an instrument number should result in enabling tie
					ns = SentinelType::Mark;
					break;
			}
			sequence_data_source->SetSentinel( sequence_index, ns, Semantic::Parameter );

			OnSequenceChanged( sequence_id );
		}
	}


	void ComponentTrack::ToggleSequenceTieNoteInMarkedArea()
	{
		AddUndoStep();

		const auto					 as = m_DataSourceSequenceList[ 0 ]->GetAllSentinels( Semantic::Parameter );
		const auto					 se = as.GetSentinel( SentinelType::Empty );
		const auto					 st = as.GetSentinel( SentinelType::Mark );
		const auto					 mi = m_DataSourceSequenceList[ 0 ]->GetMinimum( Semantic::Payload );
		const auto					 ma = m_DataSourceSequenceList[ 0 ]->GetMaximum( Semantic::Payload );
		const auto					 mu = m_DataSourceSequenceList[ 0 ]->GetOffset( Semantic::Payload );

		std::vector< unsigned char > modified_sequence_indices = ForEachEventInMarkedRange(
			[ & ]( DataSourceSequence::Event& inEvent, int index )
			{
				if ( inEvent.m_Payload >= mi + mu && inEvent.m_Payload <= ma + mu ) {
					if ( inEvent.m_Param == se ) inEvent.m_Param = st;
					else if ( inEvent.m_Param == st ) inEvent.m_Param = se;
				}
			} );

		for ( auto modified_sequence_index: modified_sequence_indices ) OnSequenceChanged( modified_sequence_index );
	}

	int ComponentTrack::EraseSequenceLine( bool inValueOnlyAtCursor )
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();

			if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

			bool change_made = false;

			if ( !inValueOnlyAtCursor || IsCursorAtSequenceInstrument() ) {
				if ( sequence_data_source->GetSentinel( sequence_index, Semantic::Parameter ) != SentinelType::Empty ) {
					change_made |= true;
					sequence_data_source->SetSentinel( sequence_index, SentinelType::Empty, Semantic::Parameter );
				}
			}

			if ( !inValueOnlyAtCursor || IsCursorAtSequenceCommand() ) {
				if ( sequence_data_source->GetSentinel( sequence_index, Semantic::Command ) != SentinelType::Empty ) {
					change_made |= true;
					sequence_data_source->SetSentinel( sequence_index, SentinelType::Empty, Semantic::Command );
				}
			}

			if ( !inValueOnlyAtCursor || IsCursorAtSequenceNote() ) {
				if ( sequence_data_source->GetSentinel( sequence_index, Semantic::Payload ) != SentinelType::Empty ) {
					change_made |= true;
					sequence_data_source->SetSentinel( sequence_index, SentinelType::Empty, Semantic::Payload );
				}
			}

			if ( change_made ) OnSequenceChanged( sequence_id );

			if ( !inValueOnlyAtCursor || IsCursorAtSequenceNote() ) return DoKeyDown();
		}

		return m_EventPos;
	}


	void ComponentTrack::EraseSequenceLinesInMarkedArea( bool inValueOnlyAtCursor )
	{
		FOUNDATION_ASSERT( m_IsMarkingArea );

		const auto& ed = m_DataSourceSequenceList[ 0 ]->m_EventDef;

		AddUndoStep();

		std::vector< unsigned char > altered_sequence_indices = ForEachEventInMarkedRange(
			[ & ]( DataSourceSequence::Event& inEvent, int index )
			{
				if ( !inValueOnlyAtCursor ) ed.ClearEvent( inEvent );
				else if ( IsCursorAtSequenceInstrument() )
					inEvent.m_Param = ed.m_Param.m_Sentinels.GetSentinel( SentinelType::Empty );
				else if ( IsCursorAtSequenceCommand() )
					inEvent.m_Command = ed.m_Command.m_Sentinels.GetSentinel( SentinelType::Empty );
				else inEvent.m_Payload = ed.m_Payload.m_Sentinels.GetSentinel( SentinelType::Empty );
			} );

		for ( auto altered_sequence_index: altered_sequence_indices ) OnSequenceChanged( altered_sequence_index );
	}


	int ComponentTrack::DeleteSequenceLine( bool inChangeSequenceSize )
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();

			if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

			const unsigned int sequence_data_source_length = sequence_data_source->GetLength();

			if ( sequence_data_source_length > 0 ) {
				for ( unsigned int i = sequence_index; i < sequence_data_source_length - 1; ++i )
					( *sequence_data_source )[ i ] = ( *sequence_data_source )[ i + 1 ];

				sequence_data_source->ClearEvent( sequence_data_source_length - 1 );

				if ( inChangeSequenceSize && sequence_data_source_length > 1 ) {
					sequence_data_source->SetLength( sequence_data_source_length - 1 );
					UpdateMaxEventPos();
				}

				OnSequenceChanged( sequence_id );

				if ( inChangeSequenceSize && sequence_data_source_length > 1 ) {
					OnOrderListChanged();
					return ComponentTrackUtils::GetEventPosOf( m_EventPosDetails.OrderListIndex(),
															   m_EventPosDetails.SequenceIndex(),
															   m_DataSourceOrderList,
															   m_DataSourceSequenceList );
				}
			}
		}

		return m_EventPos;
	}

	int ComponentTrack::InsertSequenceLine( bool inChangeSequenceSize )
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();

			const unsigned int sequence_data_source_length					  = sequence_data_source->GetLength();

			if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

			if ( sequence_data_source_length > 1 || inChangeSequenceSize ) {
				bool changed_size = false;
				if ( inChangeSequenceSize && sequence_data_source_length < MAX_SEQUENCE_EVENTS ) {
					sequence_data_source->SetLength( sequence_data_source_length + 1 );
					changed_size = true;
				}

				for ( unsigned int i = sequence_data_source_length;
					  i > static_cast< unsigned int >( m_EventPosDetails.SequenceIndex() );
					  --i )
					( *sequence_data_source )[ i ] = ( *sequence_data_source )[ i - 1 ];

				sequence_data_source->ClearEvent( m_EventPosDetails.SequenceIndex() );

				if ( changed_size ) UpdateMaxEventPos();

				OnSequenceChanged( sequence_id );

				if ( changed_size ) {
					OnOrderListChanged();
					return ComponentTrackUtils::GetEventPosOf( m_EventPosDetails.OrderListIndex(),
															   m_EventPosDetails.SequenceIndex(),
															   m_DataSourceOrderList,
															   m_DataSourceSequenceList );
				}
			}
		}

		return m_EventPos;
	}


	int ComponentTrack::ResizeSequence( int inLength )
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 sequence_index		  = m_EventPosDetails.SequenceIndex();

			if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

			const unsigned int sequence_data_source_length = sequence_data_source->GetLength();

			sequence_data_source->SetLength( inLength );

			for ( int i = sequence_data_source_length; i < inLength; ++i ) sequence_data_source->ClearEvent( i );

			UpdateMaxEventPos();

			OnSequenceChanged( sequence_id );

			return ComponentTrackUtils::GetEventPosOf( m_EventPosDetails.OrderListIndex(),
													   m_EventPosDetails.SequenceIndex(),
													   m_DataSourceOrderList,
													   m_DataSourceSequenceList );
		}

		return m_EventPos;
	}


	int ComponentTrack::ResizeAndReplaceData( const DataCopySequence* inSequenceData )
	{
		const int source_length = inSequenceData->GetEventCount();
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];

			sequence_data_source->SetLength( source_length );

			for ( int i = 0; i < source_length; ++i ) ( *sequence_data_source )[ i ] = ( *inSequenceData )[ i ];

			UpdateMaxEventPos();

			OnSequenceChanged( sequence_id );
			return ComponentTrackUtils::GetEventPosOf( m_EventPosDetails.OrderListIndex(),
													   m_EventPosDetails.SequenceIndex(),
													   m_DataSourceOrderList,
													   m_DataSourceSequenceList );
		}

		return m_EventPos;
	}


	int ComponentTrack::PasteSequenceEventData( const DataCopySequenceEvents* inSequenceEventData, bool inInsert )
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 destination_index	  = m_EventPosDetails.SequenceIndex();

			if ( inInsert ) InsertSequenceLines( inSequenceEventData->GetEventCount() );

			const int sequence_size = sequence_data_source->GetLength();

			for ( int i = 0; i < static_cast< int >( inSequenceEventData->GetEventCount() ); ++i ) {
				if ( i + destination_index >= sequence_size ) break;

				( *sequence_data_source )[ i + destination_index ] = ( *inSequenceEventData )[ i ];
			}

			UpdateMaxEventPos();

			OnSequenceChanged( sequence_id );
			return ComponentTrackUtils::GetEventPosOf( m_EventPosDetails.OrderListIndex(),
													   m_EventPosDetails.SequenceIndex(),
													   m_DataSourceOrderList,
													   m_DataSourceSequenceList );
		}

		return m_EventPos;
	}


	int ComponentTrack::InsertSequenceLines( int inLineCount )
	{
		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
			const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ sequence_id ];
			const auto									 destination_index	  = m_EventPosDetails.SequenceIndex();

			const unsigned int sequence_data_source_length					  = sequence_data_source->GetLength();

			if ( sequence_data_source_length + inLineCount <= MAX_SEQUENCE_EVENTS ) {
				sequence_data_source->SetLength( sequence_data_source_length + inLineCount );

				int dest_index = sequence_data_source_length + inLineCount - 1;

				FOUNDATION_ASSERT( sequence_data_source_length > 0 );
				for ( int i = sequence_data_source_length - 1; i >= static_cast< int >( destination_index );
					  --i, --dest_index )
					( *sequence_data_source )[ dest_index ] = ( *sequence_data_source )[ i ];

				for ( unsigned int i = destination_index; i < destination_index + inLineCount; ++i )
					sequence_data_source->ClearEvent( i );

				UpdateMaxEventPos();

				OnSequenceChanged( sequence_id );

				return ComponentTrackUtils::GetEventPosOf( m_EventPosDetails.OrderListIndex(),
														   m_EventPosDetails.SequenceIndex(),
														   m_DataSourceOrderList,
														   m_DataSourceSequenceList );
			}
		}

		return m_EventPos;
	}

	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::UpdateMaxEventPos()
	{ m_MaxEventPos = ComponentTrackUtils::GetMaxEventPosition( m_DataSourceOrderList, m_DataSourceSequenceList ); }


	void ComponentTrack::SetEventPosDetails( unsigned int inOrderListIndex, unsigned int inSequenceIndex,
											 bool inForceOrderListIndexChangeEvent )
	{
		unsigned int previous_order_list_index = m_EventPosDetails.OrderListIndex();

		m_EventPosDetails.Set( inOrderListIndex, inSequenceIndex );

		if ( inOrderListIndex != previous_order_list_index || inForceOrderListIndexChangeEvent ) {
			if ( m_DataSourceOrderList->IsEndIndex( inOrderListIndex ) )
				m_OrderListIndexChangedEvent.Execute( m_HasControl, inOrderListIndex, 0xff );
			else
				m_OrderListIndexChangedEvent.Execute( m_HasControl, inOrderListIndex,
													  m_DataSourceOrderList->GetSequenceId( inOrderListIndex ) );
		}
	}

	//--------------------------------------------------------------------------------------------------
	// Information report (function callback)
	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::UpdateSequenceStatusReport()
	{
		if ( m_HasControl && m_StatusReportFunction != nullptr && !m_FocusModeOrderList ) {
			if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
				const auto s = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
				const std::shared_ptr< DataSourceSequence >& sequence_data_source = m_DataSourceSequenceList[ s ];
				m_StatusReportFunction( true, s, sequence_data_source->GetPackedSize() );
			} else m_StatusReportFunction( true, -1, -1 );
		}
	}

	void ComponentTrack::UpdateOrderListStatusReport()
	{
		if ( m_HasControl && m_StatusReportFunction != nullptr && m_FocusModeOrderList )
			m_StatusReportFunction( false, 0, ( *m_DataSourceOrderList ).GetPackedSize() );
	}

	//--------------------------------------------------------------------------------------------------
	// Cursor
	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::ApplyCursorPosition( CursorControl& inCursorControl )
	{
		int actual_cursor_y = m_Position.m_Y + m_FocusRow.m_Row;
		int actual_cursor_x = m_Position.m_X + [ & ]()
		{
			if ( m_FocusModeOrderList ) return m_CursorPos;
			else {
				const int sequence_offset = 5;
				if ( m_CursorPos < 2 ) return sequence_offset + m_CursorPos;
				if ( m_CursorPos < 4 ) return sequence_offset + m_CursorPos + 1;

				return sequence_offset + m_CursorPos + 2;
			}
		}();

		inCursorControl.SetPosition( CursorControl::Position( { actual_cursor_x, actual_cursor_y } ) );
	}


	int ComponentTrack::GetCursorPositionXFromSequenceCellX( int inCellX ) const
	{
		int local_sequence_cell_left_x = inCellX - ( m_Position.m_X + 5 );

		if ( local_sequence_cell_left_x < 0 ) return -1;

		if ( local_sequence_cell_left_x < 3 ) return std::max( 0, std::min( 1, local_sequence_cell_left_x ) );

		if ( local_sequence_cell_left_x < 6 ) return std::max( 2, std::min( 3, local_sequence_cell_left_x - 1 ) );

		return 4;
	}


	int ComponentTrack::GetCursorPositionXFromOrderListCellX( int inCellX ) const
	{
		int local_order_list_cell_left_x = inCellX - m_Position.m_X;

		if ( local_order_list_cell_left_x < 0 ) return -1;

		if ( local_order_list_cell_left_x < 3 ) return local_order_list_cell_left_x;

		return 3;
	}


	//--------------------------------------------------------------------------------------------------
	// Key input logic
	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::DoKeyLeft( bool inControlIsDown )
	{
		if ( m_FocusModeOrderList ) {
			if ( m_CursorPos > 0 ) --m_CursorPos;
		} else {
			if ( inControlIsDown ) {
				m_CursorPos &= ~1;

				if ( m_CursorPos > 0 ) m_CursorPos -= 2;
				else m_CursorPos = 4;
			} else if ( m_CursorPos > 0 ) --m_CursorPos;
		}
	}


	void ComponentTrack::DoKeyRight( bool inControlIsDown )
	{
		if ( m_FocusModeOrderList ) {
			if ( m_CursorPos < 3 ) m_CursorPos++;
		} else {
			if ( inControlIsDown ) {
				m_CursorPos &= ~1;

				if ( m_CursorPos < 4 ) m_CursorPos += 2;
				else m_CursorPos = 0;
			} else if ( m_CursorPos < 4 ) m_CursorPos++;
		}
	}


	int ComponentTrack::DoKeyDown()
	{
		if ( !m_FocusModeOrderList ) {
			if ( m_EventPos < m_MaxEventPos ) return m_EventPos + 1;
		} else {
			if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
				const unsigned char sequence_index =
					m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
				const int sequence_length = m_DataSourceSequenceList[ sequence_index ]->GetLength();

				return m_EventPos + sequence_length;
			}
		}

		return m_EventPos;
	}


	int ComponentTrack::DoKeyUp()
	{
		if ( !m_FocusModeOrderList ) {
			if ( m_EventPos > 0 ) return m_EventPos - 1;
		} else {
			if ( m_EventPosDetails.OrderListIndex() > 0 ) {
				const unsigned char sequence_index =
					m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
				const int sequence_length = m_DataSourceSequenceList[ sequence_index ]->GetLength();

				return m_EventPos - sequence_length;
			}
		}

		return m_EventPos;
	}


	int ComponentTrack::DoKeyPageDown()
	{
		if ( !m_FocusModeOrderList ) {
			if ( m_EventPos < m_MaxEventPos ) {
				int event_pos = m_EventPos + page_up_down_step;
				return ( event_pos > m_MaxEventPos ) ? m_MaxEventPos : event_pos;
			}
		} else {
			int order_list_index = m_EventPosDetails.OrderListIndex();
			int event_pos		 = m_EventPos;

			for ( int i = 0; i < 4; ++i ) {
				if ( m_DataSourceOrderList->IsEndIndex( order_list_index ) ) break;

				const unsigned char sequence_index	= m_DataSourceOrderList->GetSequenceId( order_list_index );
				const int			sequence_length = m_DataSourceSequenceList[ sequence_index ]->GetLength();

				event_pos += sequence_length;
				++order_list_index;
			}

			return event_pos;
		}

		return m_EventPos;
	}


	int ComponentTrack::DoKeyPageUp()
	{
		if ( !m_FocusModeOrderList ) {
			if ( m_EventPos > 0 ) {
				int event_pos = m_EventPos - page_up_down_step;
				return ( event_pos < 0 ) ? 0 : event_pos;
			}
		} else {
			if ( m_EventPosDetails.OrderListIndex() > 0 ) {
				int order_list_index = m_EventPosDetails.OrderListIndex();
				int event_pos		 = m_EventPos;

				for ( int i = 0; i < 4; ++i ) {
					if ( order_list_index > 0 ) {
						--order_list_index;

						const unsigned char sequence_index	= m_DataSourceOrderList->GetSequenceId( order_list_index );
						const int			sequence_length = m_DataSourceSequenceList[ sequence_index ]->GetLength();

						event_pos -= sequence_length;
					}
				}

				return event_pos;
			}
		}

		return m_EventPos;
	}


	int ComponentTrack::DoInsert( bool inIsControlDown )
	{
		if ( m_FocusModeOrderList ) {
			AddUndoStep();

			auto& orderlist_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() ];
			if ( m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
				if ( m_EventPosDetails.OrderListIndex() == 0 ) return m_EventPos;

				orderlist_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() - 1 ];
			}

			if ( InsertSequenceIndexInOrderListAtIndex( m_DataSourceOrderList, m_EventPosDetails.OrderListIndex(),
														orderlist_entry ) )
				OnOrderListChanged();

			return m_EventPos;
		} else {
			if ( m_IsMarkingArea ) DoCancelMarking();

			return InsertSequenceLine( inIsControlDown );
		}
	}


	int ComponentTrack::DoDelete( bool inIsControlDown )
	{
		if ( m_FocusModeOrderList ) {
			if ( m_DataSourceOrderList->GetLength() > 2
				 && !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) )
			{
				AddUndoStep();

				for ( int i = m_EventPosDetails.OrderListIndex();
					  i < static_cast< int >( m_DataSourceOrderList->GetLength() ) - 1;
					  ++i )
					( *m_DataSourceOrderList )[ i ] = ( *m_DataSourceOrderList )[ i + 1 ];

				m_DataSourceOrderList->DecreaseSize();

				OnOrderListChanged();
			}

			return m_EventPos;
		} else {
			if ( m_IsMarkingArea ) {
				if ( !inIsControlDown ) {
					EraseSequenceLinesInMarkedArea( false );
					DoCancelMarking();

					return m_EventPos;
				}

				DoCancelMarking();
			}

			return DeleteSequenceLine( inIsControlDown );
		}
	}

	int ComponentTrack::DoBackspace( bool inIsControlDown )
	{
		if ( m_FocusModeOrderList ) {
			if ( m_EventPosDetails.OrderListIndex() > 0 ) {
				int event_pos = DoKeyUp();
				SetEventPosDetails( m_EventPosDetails.OrderListIndex() - 1, m_EventPosDetails.SequenceIndex() );
				DoDelete( inIsControlDown );

				return event_pos;
			}
		} else {
			if ( m_EventPosDetails.SequenceIndex() > 0 ) {
				if ( m_IsMarkingArea ) {
					EraseSequenceLinesInMarkedArea( false );
					DoCancelMarking();

					return m_EventPos;
				}

				int event_pos = DoKeyUp();
				SetEventPosDetails( m_EventPosDetails.OrderListIndex(), m_EventPosDetails.SequenceIndex() - 1, true );
				DoDelete( inIsControlDown );

				return event_pos;
			}
		}

		return m_EventPos;
	}


	int ComponentTrack::DoInsertFirstFreeSequence( const std::function< unsigned char() > inFindFreeSequence )
	{
		if ( m_FocusModeOrderList ) {
			AddUndoStep();

			auto& orderlist_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() ];

			if ( m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
				if ( m_EventPosDetails.OrderListIndex() == 0 ) return m_EventPos;

				orderlist_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() - 1 ];
			}

			unsigned char first_free_sequence_index = inFindFreeSequence();

			if ( first_free_sequence_index < 0x80 ) orderlist_entry.m_Payload = first_free_sequence_index;

			if ( InsertSequenceIndexInOrderListAtIndex( m_DataSourceOrderList, m_EventPosDetails.OrderListIndex(),
														orderlist_entry ) )
				OnOrderListChanged();
		}

		return m_EventPos;
	}

	void ComponentTrack::DoSplitSequence()
	{
		if ( !m_FocusModeOrderList ) {
			if ( m_IsMarkingArea ) DoCancelMarking();

			auto& orderlist_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() ];

			if ( m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) return;

			const int index_in_current_sequence = m_EventPosDetails.SequenceIndex();
			if ( index_in_current_sequence == 0 ) return;

			unsigned char first_free_sequence_index = m_GetFirstEmptySequenceIndexFunction();
			if ( first_free_sequence_index >= 0x80 ) return;

			AddUndoStep();

			unsigned char current_sequence_index = orderlist_entry.m_Payload;
			auto&		  source				 = m_DataSourceSequenceList[ current_sequence_index ];
			auto&		  destination			 = m_DataSourceSequenceList[ first_free_sequence_index ];

			int			  new_sequence_length	 = source->GetLength() - index_in_current_sequence;
			destination->SetLength( new_sequence_length );

			DataSourceUtils::CopySequence( source, index_in_current_sequence, new_sequence_length, destination );
			source->SetLength( index_in_current_sequence );

			OnSequenceChanged( current_sequence_index );
			OnSequenceChanged( first_free_sequence_index );

			m_SequenceSplitEvent.Execute( current_sequence_index, first_free_sequence_index );
		}
	}

	void ComponentTrack::DoDuplicateSequence( const bool inReplaceOriginal )
	{
		if ( m_FocusModeOrderList ) {
			const unsigned int order_index	   = m_EventPosDetails.OrderListIndex();
			auto&			   orderlist_entry = ( *m_DataSourceOrderList )[ order_index ];

			if ( m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) return;

			// determine the empty, unused sequence to use for the duplicate
			const unsigned char duplicate_sequence_index = m_GetFirstEmptySequenceIndexFunction();
			if ( duplicate_sequence_index >= 0x80 ) return;

			AddUndoStep();

			// copy current sequence to the new sequence
			const unsigned char current_sequence_index = orderlist_entry.m_Payload;
			const auto&			source				   = m_DataSourceSequenceList[ current_sequence_index ];
			const auto&			destination			   = m_DataSourceSequenceList[ duplicate_sequence_index ];
			const int			new_sequence_length	   = source->GetLength();

			destination->SetLength( new_sequence_length );
			DataSourceUtils::CopySequence( source, 0, new_sequence_length, destination );
			OnSequenceChanged( duplicate_sequence_index );

			if ( inReplaceOriginal ) {
				// replace the original with the duplicate
				orderlist_entry.m_Payload				  = duplicate_sequence_index;
				( *m_DataSourceOrderList )[ order_index ] = orderlist_entry;
				OnOrderListChanged();
			} else {
				// insert the duplicate in the order list right after the original
				auto new_entry		= orderlist_entry;
				new_entry.m_Payload = duplicate_sequence_index;

				if ( InsertSequenceIndexInOrderListAtIndex( m_DataSourceOrderList, order_index + 1, new_entry ) )
					OnOrderListChanged();
			}
		}
	}


	void ComponentTrack::DoTestExpandSequence()
	{
		if ( !( m_FocusModeOrderList || m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) ) {
			auto& orderlist_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() ];

			AddUndoStep();

			unsigned char current_sequence_index = orderlist_entry.m_Payload;
			auto&		  source				 = m_DataSourceSequenceList[ current_sequence_index ];

			DataSourceUtils::ExpandSequence( source );
			OnSequenceChanged( current_sequence_index );
		}
	}


	void ComponentTrack::DoResizeSequence( ComponentsManager& inComponentsManager )
	{
		if ( !( m_FocusModeOrderList || m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) ) {
			if ( m_IsMarkingArea ) DoCancelMarking();

			auto& order_list_entry = ( *m_DataSourceOrderList )[ m_EventPosDetails.OrderListIndex() ];

			auto  dialog_ok		   = [ this ]( unsigned int inValue )
			{
				int event_pos = ResizeSequence( inValue );

				// Set the new event position
				if ( m_EventPos != event_pos ) SetEventPosition( event_pos );

				m_OrderListChangedEvent.Execute();
			};

			auto			   dialog_cancel = [ this ]() {};

			const unsigned int current_size	 = GetSequenceSize();
			std::string		   message =
				"Enter size of sequence [" + ToHexValueString( order_list_entry.m_Payload, false ) + "]";

			inComponentsManager.StartDialog( std::make_shared< DialogHexValueInput >(
				"Resize sequence", message, 30, 3, current_size, MAX_SEQUENCE_EVENTS, dialog_ok, dialog_cancel ) );
		}
	}


	void ComponentTrack::DoInsertLinesInSequence( ComponentsManager& inComponentsManager )
	{
		if ( !m_FocusModeOrderList ) {
			if ( m_IsMarkingArea ) DoCancelMarking();

			auto dialog_ok = [ this ]( unsigned int inValue )
			{
				AddUndoStep();

				int event_pos = InsertSequenceLines( inValue );

				// Set the new event position
				if ( m_EventPos != event_pos ) SetEventPosition( event_pos );

				m_OrderListChangedEvent.Execute();
			};

			auto		 dialog_cancel = [ this ]() {};

			unsigned int max_value =
				MAX_SEQUENCE_EVENTS - m_DataSourceSequenceList[ m_EventPosDetails.OrderListIndex() ]->GetLength();

			inComponentsManager.StartDialog(
				std::make_shared< DialogHexValueInput >( "Add lines to sequence",
														 "Enter number of lines to add to sequence",
														 30,
														 3,
														 0,
														 max_value,
														 dialog_ok,
														 dialog_cancel ) );
		}
	}


	void ComponentTrack::DoCopyFullSequenceData()
	{
		FOUNDATION_ASSERT( m_FocusModeOrderList );

		if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
			const std::shared_ptr< DataSourceSequence >& sequence_data_source =
				m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() ) ];
			CopyPaste::Instance().SetData( new DataCopySequence( *sequence_data_source ) );
		}
	}


	void ComponentTrack::DoCopyMarkedSequenceData()
	{
		FOUNDATION_ASSERT( !m_FocusModeOrderList );

		int	 top				= m_IsMarkingArea ? std::min( m_MarkingFromEventPos, m_MarkingToEventPos ) : m_EventPos;
		int	 bottom				= m_IsMarkingArea ? std::max( m_MarkingFromEventPos, m_MarkingToEventPos ) : m_EventPos;

		// Find sequence and index of top position
		int	 find_event_pos		= top;
		int	 event_pos			= 0;

		int	 orderlist_index	= 0;
		int	 sequence_event_pos = 0;

		bool found				= false;

		for ( unsigned int i = 0; i < m_DataSourceOrderList->GetLength(); ++i ) {
			unsigned char								 sequence_index = ( *m_DataSourceOrderList )[ i ].m_Payload;

			const std::shared_ptr< DataSourceSequence >& sequence		= m_DataSourceSequenceList[ sequence_index ];

			int											 next_event_pos = event_pos + sequence->GetLength();

			if ( find_event_pos >= event_pos && find_event_pos < next_event_pos ) {
				orderlist_index	   = i;
				sequence_event_pos = find_event_pos - event_pos;

				found			   = true;
				break;
			}

			event_pos = next_event_pos;
		}

		if ( found ) {
			std::vector< DataSourceSequence::Event > events_copy;

			for ( int event_pos = top; event_pos <= bottom; ++event_pos ) {
				if ( m_DataSourceOrderList->IsEndIndex( orderlist_index ) ) break;
				const auto&	  orderlist_entry						  = ( *m_DataSourceOrderList )[ orderlist_index ];

				unsigned char sequence_index						  = orderlist_entry.m_Payload;
				const std::shared_ptr< DataSourceSequence >& sequence = m_DataSourceSequenceList[ sequence_index ];

				DataSourceSequence::Event					 event_to_copy = ( *sequence )[ sequence_event_pos ];
				events_copy.push_back( event_to_copy );

				// Logging
				std::string output = std::to_string( event_to_copy.m_Param ) + ": "
									 + std::to_string( event_to_copy.m_Command ) + " - "
									 + std::to_string( event_to_copy.m_Payload ) + "\n";
				Logging::instance().Info( output.c_str() );

				++sequence_event_pos;
				if ( sequence_event_pos >= static_cast< int >( sequence->GetLength() ) ) {
					++orderlist_index;
					sequence_event_pos = 0;
				}
			}

			if ( !events_copy.empty() ) CopyPaste::Instance().SetData( new DataCopySequenceEvents( events_copy ) );
		}
	}


	void ComponentTrack::DoPaste( bool inCtrlPressed )
	{
		if ( CopyPaste::Instance().HasData< DataCopySequence >() ) {
			const auto* data = CopyPaste::Instance().GetData< DataCopySequence >();

			if ( !inCtrlPressed ) {
				AddUndoStep();

				int event_pos = ResizeAndReplaceData( data );

				// Set the new event position
				if ( m_EventPos != event_pos ) SetEventPosition( event_pos );

				OnOrderListChanged();
			}
		}

		if ( CopyPaste::Instance().HasData< DataCopySequenceEvents >() && !m_FocusModeOrderList ) {
			const auto* data = CopyPaste::Instance().GetData< DataCopySequenceEvents >();
			AddUndoStep();

			int event_pos = PasteSequenceEventData( data, inCtrlPressed );

			// Set the new event position
			if ( m_EventPos != event_pos ) SetEventPosition( event_pos );

			OnOrderListChanged();
		}
	}


	void ComponentTrack::DoSetInstrumentIndexValue( unsigned char inValue )
	{
		FOUNDATION_ASSERT( inValue >= 0 && inValue < 0x20 );

		if ( !m_FocusModeOrderList ) {
			if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
				const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
				const std::shared_ptr< DataSourceSequence >& sequence_data_source =
					m_DataSourceSequenceList[ sequence_id ];
				const auto sequence_index = m_EventPosDetails.SequenceIndex();

				if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

				if ( sequence_data_source->GetSentinel( sequence_index, Semantic::Parameter ) == SentinelType::Empty )
					sequence_data_source->SetValue( inValue, sequence_index, Semantic::Parameter );
				else sequence_data_source->SetSentinel( sequence_index, SentinelType::Empty, Semantic::Parameter );

				OnSequenceChanged( sequence_id );
			}
		}
	}


	void ComponentTrack::DoSetCommandIndexValue( unsigned char inValue )
	{
		FOUNDATION_ASSERT( inValue >= 0 && inValue < 0x40 );

		if ( !m_FocusModeOrderList ) {
			if ( !m_DataSourceOrderList->IsEndIndex( m_EventPosDetails.OrderListIndex() ) ) {
				const auto sequence_id = m_DataSourceOrderList->GetSequenceId( m_EventPosDetails.OrderListIndex() );
				const std::shared_ptr< DataSourceSequence >& sequence_data_source =
					m_DataSourceSequenceList[ sequence_id ];
				const auto sequence_index = m_EventPosDetails.SequenceIndex();

				if ( !sequence_data_source->IsInErrorState() ) AddUndoStep();

				auto cs = sequence_data_source->GetSentinel( sequence_index, Semantic::Command );
				if ( cs == SentinelType::Empty )
					sequence_data_source->SetValue( inValue, sequence_index, Semantic::Command );
				else sequence_data_source->SetSentinel( sequence_index, SentinelType::Empty, Semantic::Command );

				OnSequenceChanged( sequence_id );
			}
		}
	}


	void ComponentTrack::DoBeginMarking( int inBeginMarkingEventPos )
	{
		FOUNDATION_ASSERT( !m_IsMarkingArea );

		m_MarkingFromEventPos = inBeginMarkingEventPos;
		m_IsMarkingArea		  = true;
	}

	void ComponentTrack::DoCancelMarking()
	{
		FOUNDATION_ASSERT( m_IsMarkingArea );

		m_IsMarkingArea = false;
	}

	std::shared_ptr< DataSourceOrderList > ComponentTrack::GetDataSourceOrderList()
	{ return m_DataSourceOrderList; }

	//--------------------------------------------------------------------------------------------------
	// Data change
	//--------------------------------------------------------------------------------------------------

	void ComponentTrack::OnOrderListChanged()
	{
		UpdateMaxEventPos();

		// Pack orderlist
		auto packed_result = m_DataSourceOrderList->Pack();

		if ( packed_result.m_DataLength < MAX_ORDER_PACKED_SIZE )
			m_DataSourceOrderList->SendPackedDataToBuffer( packed_result );

		UpdateOrderListStatusReport();

		m_HasDataChange			 = true;
		m_HasDataChangeOrderList = true;
		m_RequireRefresh		 = true;
	}

	void ComponentTrack::OnSequenceChanged( unsigned char inSequenceIndex )
	{
		const std::shared_ptr< DataSourceSequence >& inSequence	   = m_DataSourceSequenceList[ inSequenceIndex ];
		auto										 packed_result = inSequence->Pack();

		m_RequireRefresh										   = true;

		if ( packed_result.m_DataLength < MAX_SEQUENCE_PACKED_SIZE && packed_result.m_Data != nullptr ) {
			inSequence->SendPackedDataToBuffer( packed_result );

			m_HasDataChange = true;
			m_DataChangeSequenceIndexList.push_back( inSequenceIndex );
		}

		m_SequenceDataHasChanged = true;

		UpdateSequenceStatusReport();
	}

	//--------------------------------------------------------------------------------------------------

	ComponentTrack::EventPosDetails ComponentTrack::GetEventPosDetails( int inEventPos ) const
	{
		EventPosDetails details;

		int				event_pos = 0;

		if ( inEventPos >= m_MaxEventPos && m_MaxEventPos > 0 ) {
			const int loop_event_pos = GetLoopEventPosition();
			const int loop_size		 = m_MaxEventPos - loop_event_pos;

			if ( loop_size > 0 ) {
				int loop_count = ( inEventPos - loop_event_pos ) / loop_size;
				inEventPos -= loop_count * loop_size;
			}
		}

		for ( unsigned int i = 0; i < m_DataSourceOrderList->GetLength(); ++i ) {
			unsigned char								 sequence_index = m_DataSourceOrderList->GetSequenceId( i );
			const std::shared_ptr< DataSourceSequence >& sequence		= m_DataSourceSequenceList[ sequence_index ];

			int											 next_event_pos = event_pos + sequence->GetLength();

			if ( inEventPos >= event_pos && inEventPos < next_event_pos ) {
				details.Set( i, static_cast< unsigned int >( inEventPos - event_pos ) );

				break;
			}

			event_pos = next_event_pos;
		}

		return details;
	}


	bool ComponentTrack::IsEventPosStartOfSequence( int inEventPos ) const
	{
		if ( inEventPos < 0 && inEventPos >= m_MaxEventPos ) return false;

		const auto details = GetEventPosDetails( inEventPos );

		return details.SequenceIndex() == 0;
	}


	void ComponentTrack::AddUndoStep()
	{
		std::shared_ptr< UndoComponentDataTableTracks > undo_data = std::make_shared< UndoComponentDataTableTracks >();

		m_AddUndoStepHandler( *undo_data );

		undo_data->m_CursorPos					   = m_CursorPos;
		undo_data->m_MaxEventPos				   = m_MaxEventPos;
		undo_data->m_EventPosDetailsOrderListIndex = m_EventPosDetails.OrderListIndex();
		undo_data->m_EventPosDetailsSequenceIndex  = m_EventPosDetails.SequenceIndex();
		undo_data->m_EventPosOrderListIndex		   = m_EventPosOrderListIndex;
		undo_data->m_EventPosSequenceIndex		   = m_EventPosSequenceIndex;
		undo_data->m_IsMarkingArea				   = m_IsMarkingArea;
		undo_data->m_MarkingFromEventPos		   = m_MarkingFromEventPos;
		undo_data->m_MarkingToEventPos			   = m_MarkingToEventPos;
		;

		m_Undo->AddUndo( undo_data,
						 [ this ]( const UndoComponentData& inData, CursorControl& inCursorControl )
						 { this->OnUndo( inData, inCursorControl ); } );

		m_LocalDataChange = true;
	}


	void ComponentTrack::AddUndoRecentModificationStep( bool inLockCPU )
	{
		std::shared_ptr< UndoComponentDataTableTracks > undo_data = std::make_shared< UndoComponentDataTableTracks >();

		m_AddUndoStepHandler( *undo_data );

		undo_data->m_CursorPos					   = m_CursorPos;
		undo_data->m_MaxEventPos				   = m_MaxEventPos;
		undo_data->m_EventPosDetailsOrderListIndex = m_EventPosDetails.OrderListIndex();
		undo_data->m_EventPosDetailsSequenceIndex  = m_EventPosDetails.SequenceIndex();
		undo_data->m_EventPosOrderListIndex		   = m_EventPosOrderListIndex;
		undo_data->m_EventPosSequenceIndex		   = m_EventPosSequenceIndex;
		undo_data->m_IsMarkingArea				   = m_IsMarkingArea;
		undo_data->m_MarkingFromEventPos		   = m_MarkingFromEventPos;
		undo_data->m_MarkingToEventPos			   = m_MarkingToEventPos;

		m_Undo->AddMostRecentEdit( inLockCPU,
								   undo_data,
								   [ this ]( const UndoComponentData& inData, CursorControl& inCursorControl )
								   { this->OnUndo( inData, inCursorControl ); } );
	}


	void ComponentTrack::OnUndo( const UndoComponentData& inData, CursorControl& inCursorControl )
	{
		const UndoComponentDataTableTracks& undo_data = static_cast< const UndoComponentDataTableTracks& >( inData );

		if ( m_TakingOrderListInput ) CancelOrderListInputValue();

		m_CursorPos				 = undo_data.m_CursorPos;
		m_MaxEventPos			 = undo_data.m_MaxEventPos;
		m_EventPosOrderListIndex = undo_data.m_EventPosOrderListIndex;
		m_EventPosSequenceIndex	 = undo_data.m_EventPosSequenceIndex;

		SetEventPosDetails( undo_data.m_EventPosDetailsOrderListIndex, undo_data.m_EventPosDetailsSequenceIndex );

		m_IsMarkingArea		  = undo_data.m_IsMarkingArea;
		m_MarkingFromEventPos = undo_data.m_MarkingFromEventPos;
		m_MarkingToEventPos	  = undo_data.m_MarkingToEventPos;

		m_OnUndoHandler( undo_data, inCursorControl );

		PullSequenceDataFromSource();
		PullDataFromSource( true );

		GetComputeMaxEventPosEvent().Execute();

		m_RequireRefresh = true;
	}


	void ComponentTrack::ConfigureKeyHooks( const Utility::KeyHookStore& inKeyHookStore )
	{
		using namespace Utility;

		m_KeyHooks.clear();

		m_KeyHooks.push_back( { "Key.Track.CursorUp", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( m_IsMarkingArea ) DoCancelMarking();

										inKeyHookContext.m_NewEventPos = DoKeyUp();
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.CursorDown", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( m_IsMarkingArea ) DoCancelMarking();

										inKeyHookContext.m_NewEventPos = DoKeyDown();
										return true;
									}

									return false;
								} } );

		m_KeyHooks.push_back( { "Key.Track.MarkingCursorUp", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( !m_FocusModeOrderList ) {
											if ( !m_IsMarkingArea ) DoBeginMarking( m_EventPos );

											int new_event_pos			   = DoKeyUp();

											m_MarkingToEventPos			   = new_event_pos;
											inKeyHookContext.m_NewEventPos = new_event_pos;

											return true;
										}
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.MarkingCursorDown", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( !m_FocusModeOrderList ) {
											if ( !m_IsMarkingArea ) DoBeginMarking( m_EventPos );

											int new_event_pos			   = DoKeyDown();

											m_MarkingToEventPos			   = new_event_pos;
											inKeyHookContext.m_NewEventPos = new_event_pos;

											return true;
										}
									}

									return false;
								} } );

		m_KeyHooks.push_back( { "Key.Track.ToggleGateUntilNextEvent", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_FocusModeOrderList ) {
										ToggleSequenceHoldNoteUntilEvent( true );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.ToggleGateSincePreviousEvent", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_FocusModeOrderList ) {
										ToggleSequenceHoldNoteUntilEvent( false );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.PageUp", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										inKeyHookContext.m_NewEventPos = DoKeyPageUp();
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.PageDown", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										inKeyHookContext.m_NewEventPos = DoKeyPageDown();
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.TransposeSemiToneDown", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										ApplyTranspose( -1 );

										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.TransposeOctaveDown", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										ApplyTranspose( -12 );

										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.TransposeSemiToneUp", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										ApplyTranspose( 1 );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.TransposeOctaveUp", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										ApplyTranspose( 12 );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.InsertEventsInSequence", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										DoInsertLinesInSequence( inKeyHookContext.m_ComponentsManager );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.ResizeSequence", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										DoResizeSequence( inKeyHookContext.m_ComponentsManager );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.EraseEventUnderCursor", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( m_IsMarkingArea ) EraseSequenceLinesInMarkedArea( true );
										else inKeyHookContext.m_NewEventPos = EraseSequenceLine( true );

										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.EraseEventLine", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( m_IsMarkingArea ) EraseSequenceLinesInMarkedArea( false );
										else inKeyHookContext.m_NewEventPos = EraseSequenceLine( false );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.ToggleGateUnderCursor", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( m_IsMarkingArea ) ToggleSequenceHoldNoteValueInMarkedArea();
										else inKeyHookContext.m_NewEventPos = ApplySequenceHoldNoteValue();
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.CursorLeft", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									DoKeyLeft( false );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.CursorRight", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									DoKeyRight( false );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.CursorLeftColumn", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									DoKeyLeft( true );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.CursorRightColumn", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									DoKeyRight( true );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.Insert", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoInsert( false );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.InsertAndGrow", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoInsert( true );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.Delete", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoDelete( false );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.InsertAndShrink", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoDelete( true );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.Backspace", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoBackspace( false );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.BackspaceAndShrink", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoBackspace( true );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.ShiftBackspace", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoInsert( false );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.ShiftBackspaceAndGrow", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									inKeyHookContext.m_NewEventPos = DoInsert( true );
									return true;
								} } );
		m_KeyHooks.push_back( { "Key.Track.Home", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										inKeyHookContext.m_NewEventPos = 0;
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.End", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										inKeyHookContext.m_NewEventPos = m_MaxEventPos;
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.ApplyOrderlistInput", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( m_TakingOrderListInput ) {
										ApplyOrderListInputValue();
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.CancelOrderlistInput", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( m_TakingOrderListInput ) {
										CancelOrderListInputValue();
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.ToggleTieNote", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_FocusModeOrderList ) {
										ToggleSequenceTieNote();
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.Copy", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										if ( m_FocusModeOrderList ) DoCopyFullSequenceData();
										else DoCopyMarkedSequenceData();

										m_RequireRefresh = true;

										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.Paste", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										DoPaste( false );

										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.InsertPaste", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_TakingOrderListInput ) {
										DoPaste( true );

										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back(
			{ "Key.Track.InsertFirstFreeSequence", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
			  {
				  if ( m_FocusModeOrderList && !m_TakingOrderListInput ) {
					  inKeyHookContext.m_NewEventPos = DoInsertFirstFreeSequence( m_GetFirstFreeSequenceIndexFunction );
					  return true;
				  }

				  return false;
			  } } );
		m_KeyHooks.push_back( { "Key.Track.InsertFirstEmptySequence", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( m_FocusModeOrderList && !m_TakingOrderListInput ) {
										inKeyHookContext.m_NewEventPos =
											DoInsertFirstFreeSequence( m_GetFirstEmptySequenceIndexFunction );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.SplitSequenceAtEventPosition", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_FocusModeOrderList ) {
										DoSplitSequence();
										UpdateMaxEventPos();
										SetEventPosition( m_EventPos );
										inKeyHookContext.m_NewEventPos = m_EventPos;
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.DuplicateAndReplaceSequence", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( m_FocusModeOrderList && !m_TakingOrderListInput ) {
										DoDuplicateSequence( true );
										SetEventPosition( m_EventPos );
										inKeyHookContext.m_NewEventPos = m_EventPos;
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.DuplicateAndAppendSequence", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( m_FocusModeOrderList && !m_TakingOrderListInput ) {
										DoDuplicateSequence( false );
										m_EventPos = DoKeyDown();
										SetEventPosition( m_EventPos );
										inKeyHookContext.m_NewEventPos = m_EventPos;
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.SetSelectedInstrumentIndexValue", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_FocusModeOrderList ) {
										DoSetInstrumentIndexValue( m_EditState.GetSelectedInstrument() );
										return true;
									}

									return false;
								} } );
		m_KeyHooks.push_back( { "Key.Track.SetSelectedCommandIndexValue", inKeyHookStore,
								[ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( !m_FocusModeOrderList ) {
										DoSetCommandIndexValue( m_EditState.GetSelectedCommand() );
										return true;
									}

									return false;
								} } );

		/*		m_KeyHooks.push_back({ "Key.Track.DoubleSequenceSize", SDLK_b, Keyboard::Control,
		   [&](KeyHookContext& inKeyHookContext)
				{
					if (!m_FocusModeOrderList)
					{
						DoTestExpandSequence();
						UpdateMaxEventPos();
						SetEventPosition(m_EventPos);
						inKeyHookContext.m_NewEventPos = m_EventPos;
						return true;
					}

					return false;
				} });
		*/
		m_KeyHooks.push_back( { "Key.Track.Debug", inKeyHookStore, [ & ]( KeyHookContext& inKeyHookContext )
								{
									if ( m_FocusModeOrderList )
										DebugSingleton::GetInstance()->SetDebugAddress(
											m_DataSourceOrderList->GetSourceAddress() );
									else
										DebugSingleton::GetInstance()->SetDebugAddress(
											m_DataSourceSequenceList[ m_DataSourceOrderList->GetSequenceId(
																		  m_EventPosDetails.OrderListIndex() ) ]
												->GetSourceAddress() );
									return true;
								} } );
	}


	std::string ComponentTrack::ToHexValueString( unsigned char inValue, const bool inUppercase )
	{
		auto make_character = [ &inUppercase ]( unsigned char inValue ) -> char
		{
			if ( inValue > 0x0f ) return 'x';
			if ( inValue < 10 ) return '0' + inValue;

			if ( inUppercase ) return 'A' + inValue - 10;

			return 'a' + inValue - 10;
		};

		return std::string( { make_character( inValue >> 4 ), make_character( inValue & 0x0f ) } );
	}
} // namespace Editor
