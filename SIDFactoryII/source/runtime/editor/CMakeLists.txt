#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
    - editor subdirectory
]]
cmake_minimum_required( VERSION 4.2 )

target_include_directories( SIDFactoryII PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    auxilarydata
    components
    components/utils
    converters
    converters/cc
    converters/gt
    converters/jch
    converters/mod
    converters/null
    converters/utils
    datacopy
    datasources
    debug
    dialog
    driver
    instrument
    keys
    keys
    optimize
    overlays
    packer
    screens
    screens/statusbar
    undo
    undo/undo_componentdata
    undo/undo_datasource
    utilities
    visualizer_components
)
set( HEADERS
    components_manager.h
    cursor_control.h
    display_state.h
    edit_state.h
    editor_facility.h
    editor_types.h
    overlay_control.h

    auxilarydata/auxilary_data_collection.h
    auxilarydata/auxilary_data_editing_preferences.h
    auxilarydata/auxilary_data_hardware_preferences.h
    auxilarydata/auxilary_data_play_markers.h
    auxilarydata/auxilary_data_songs.h
    auxilarydata/auxilary_data_table_text.h
    auxilarydata/auxilary_data_utils.h
    auxilarydata/auxilary_data.h

    components/utils/orderlist_utils.h
    components/utils/text_editing_data_source_table_text.h
    components/component_base.h
    components/component_button.h
    components/component_check_button.h
    components/component_console.h
    components/component_file_selector.h
    components/component_flightrecorder.h
    components/component_hex_value_input.h
    components/component_list_selector.h
    components/component_memory_view.h
    components/component_orderlistoverview.h
    components/component_pulse_filter_visualizer.h
    components/component_string_list_selector.h
    components/component_table_row_elements_with_text.h
    components/component_table_row_elements.h
    components/component_text_input.h
    components/component_track_utils.h
    components/component_track.h
    components/component_tracks.h

    converters/converterbase.h
    converters/cc/converter_cc.h
    converters/cc/source_ct.h
    converters/gt/converter_gt.h
    converters/gt/source_sng.h
    converters/jch/converter_jch.h
    converters/mod/converter_mod.h
    converters/mod/source_mod.h
    converters/null/converter_null.h
    converters/utils/consoleostreambuffer.h
    converters/utils/misc.h
    converters/utils/sf2_interface.h
    converters/utils/source_utils.h

    datacopy/copypaste.h
    datacopy/datacopy_orderlist.h
    datacopy/datacopy_sequence.h

    datasources/datasource_directory.h
    datasources/datasource_emulation_memory.h
    datasources/datasource_flightrecorder.h
    datasources/datasource_memory_buffer_string.h
    datasources/datasource_memory_buffer.h
    datasources/datasource_memory.h
    datasources/datasource_orderlist.h
    datasources/datasource_play_markers.h
    datasources/datasource_sequence.h
    datasources/datasource_sidregistersbuffer.h
    datasources/datasource_table_column_major.h
    datasources/datasource_table_memory_view.h
    datasources/datasource_table_row_major.h
    datasources/datasource_table_text.h
    datasources/datasource_table.h
    datasources/datasource_tlist.h
    datasources/datasource_track_components.h
    datasources/idatasource.h

    debug/debug_singleton.h
    debug/debug_views.h

    dialog/dialog_base.h
    dialog/dialog_bit_input.h
    dialog/dialog_hex_value_input.h
    dialog/dialog_message_yesno.h
    dialog/dialog_message.h
    dialog/dialog_midi_out_devices.h
    dialog/dialog_move_selection_list.h
    dialog/dialog_optimize.h
    dialog/dialog_packing_options.h
    dialog/dialog_selection_list.h
    dialog/dialog_sid_file_info.h
    dialog/dialog_songs.h
    dialog/dialog_text_input.h
    dialog/dialog_utilities.h

    driver/driver_architecture_sidfactory2.h
    driver/driver_info.h
    driver/driver_state.h
    driver/driver_utils.h
    driver/idriver_architecture.h

    instrument/instrumentdata_table.h
    instrument/instrumentdata_tablemapping.h
    instrument/instrumentdata.h

    keys/keyhook_setup.h

    optimize/optimizer.h
    overlays/overlay_flightrecorder.h

    packer/packer.h
    packer/packing_utils.h

    screens/statusbar/status_bar_edit.h
    screens/statusbar/status_bar.h
    screens/screen_base.h
    screens/screen_convert.h
    screens/screen_disk.h
    screens/screen_edit_utils.h
    screens/screen_edit.h
    screens/screen_intro.h

    undo/undo_componentdata/undo_componentdata_orderlist_overview.h
    undo/undo_componentdata/undo_componentdata_table_row_elements.h
    undo/undo_componentdata/undo_componentdata_table_text.h
    undo/undo_componentdata/undo_componentdata_tracks.h
    undo/undo_componentdata/undo_componentdata.h
    undo/undo_datasource/undo_datasource.h
    undo/undo.h
    undo/undostep.h

    utilities/datasource_utils.h
    utilities/editor_utils.h
    utilities/import_utils.h

    visualizer_components/visualizer_component_base.h
    visualizer_components/visualizer_component_pulse_filter_state.h
    visualizer_components/vizualizer_component_emulation_state.h
)
set( SOURCES
    components_manager.cpp
    cursor_control.cpp
    display_state.cpp
    edit_state.cpp
    editor_facility.cpp
    overlay_control.cpp

    auxilarydata/auxilary_data_collection.cpp
    auxilarydata/auxilary_data_editing_preferences.cpp
    auxilarydata/auxilary_data_hardware_preferences.cpp
    auxilarydata/auxilary_data_play_markers.cpp
    auxilarydata/auxilary_data_songs.cpp
    auxilarydata/auxilary_data_table_text.cpp
    auxilarydata/auxilary_data.cpp

    components/utils/orderlist_utils.cpp
    components/utils/text_editing_data_source_table_text.cpp
    components/component_base.cpp
    components/component_button.cpp
    components/component_check_button.cpp
    components/component_console.cpp
    components/component_file_selector.cpp
    components/component_flightrecorder.cpp
    components/component_hex_value_input.cpp
    components/component_list_selector.cpp
    components/component_memory_view.cpp
    components/component_orderlistoverview.cpp
    components/component_pulse_filter_visualizer.cpp
    components/component_string_list_selector.cpp
    components/component_table_row_elements_with_text.cpp
    components/component_table_row_elements.cpp
    components/component_text_input.cpp
    components/component_track_utils.cpp
    components/component_track.cpp
    components/component_tracks.cpp

    converters/converterbase.cpp
    converters/cc/converter_cc.cpp
    converters/cc/source_ct.cpp
    converters/gt/converter_gt.cpp
    converters/gt/source_sng.cpp
    converters/jch/converter_jch.cpp
    converters/mod/converter_mod.cpp
    converters/mod/source_mod.cpp
    converters/null/converter_null.cpp
    converters/utils/consoleostreambuffer.cpp
    converters/utils/misc.cpp
    converters/utils/sf2_interface.cpp
    converters/utils/source_utils.cpp

    datacopy/copypaste.cpp
    datacopy/datacopy_orderlist.cpp
    datacopy/datacopy_sequence.cpp

    datasources/datasource_directory.cpp
    datasources/datasource_emulation_memory.cpp
    datasources/datasource_flightrecorder.cpp
    datasources/datasource_memory_buffer_string.cpp
    datasources/datasource_memory_buffer.cpp
    datasources/datasource_memory.cpp
    datasources/datasource_orderlist.cpp
    datasources/datasource_play_markers.cpp
    datasources/datasource_sequence.cpp
    datasources/datasource_sidregistersbuffer.cpp
    datasources/datasource_table_column_major.cpp
    datasources/datasource_table_memory_view.cpp
    datasources/datasource_table_row_major.cpp
    datasources/datasource_table_text.cpp
    datasources/datasource_table.cpp
    datasources/datasource_track_components.cpp

    debug/debug_singleton.cpp
    debug/debug_views.cpp

    dialog/dialog_base.cpp
    dialog/dialog_bit_input.cpp
    dialog/dialog_hex_value_input.cpp
    dialog/dialog_message_yesno.cpp
    dialog/dialog_message.cpp
    dialog/dialog_midi_out_devices.cpp
    dialog/dialog_move_selection_list.cpp
    dialog/dialog_optimize.cpp
    dialog/dialog_packing_options.cpp
    dialog/dialog_selection_list.cpp
    dialog/dialog_sid_file_info.cpp
    dialog/dialog_songs.cpp
    dialog/dialog_text_input.cpp
    dialog/dialog_utilities.cpp

    driver/driver_architecture_sidfactory2.cpp
    driver/driver_info.cpp
    driver/driver_state.cpp
    driver/driver_utils.cpp

    instrument/instrumentdata_table.cpp
    instrument/instrumentdata_tablemapping.cpp
    instrument/instrumentdata.cpp

    keys/keyhook_setup.cpp

    optimize/optimizer.cpp
    overlays/overlay_flightrecorder.cpp

    packer/packer.cpp
    packer/packing_utils.cpp

    screens/statusbar/status_bar_edit.cpp
    screens/statusbar/status_bar.cpp
    screens/screen_base.cpp
    screens/screen_convert.cpp
    screens/screen_disk.cpp
    screens/screen_edit_utils.cpp
    screens/screen_edit.cpp
    screens/screen_intro.cpp

    undo/undo.cpp
    undo/undostep.cpp

    utilities/datasource_utils.cpp
    utilities/editor_utils.cpp
    utilities/import_utils.cpp

    visualizer_components/visualizer_component_base.cpp
    visualizer_components/visualizer_component_pulse_filter_state.cpp
    visualizer_components/vizualizer_component_emulation_state.cpp
)
set( OTHERS )
target_sources( SIDFactoryII PUBLIC ${HEADERS} ${SOURCES} ${OTHERS} )
