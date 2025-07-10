# Cocoa backend sources
set(COCOA_SOURCES
    cocoa_event_source.mm
    cocoa_window_manager.mm
    cocoa_window_manager_factory.mm
    cocoa_window.mm
    cocoa_monitor.mm
    cocoa_ime.mm
    cocoa_system_factory.mm
)

# Add sources to parent scope
set(SYSTEM_SOURCES ${SYSTEM_SOURCES} ${COCOA_SOURCES} PARENT_SCOPE)

# Cocoa backend requires Objective-C++
set_source_files_properties(${COCOA_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++")

# Link with Cocoa frameworks
find_library(COCOA_LIBRARY Cocoa REQUIRED)
find_library(QUARTZCORE_LIBRARY QuartzCore REQUIRED)
find_library(METAL_LIBRARY Metal REQUIRED)
find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)

set(COCOA_LIBRARIES
    ${COCOA_LIBRARY}
    ${QUARTZCORE_LIBRARY}
    ${METAL_LIBRARY}
    ${COREGRAPHICS_LIBRARY}
)

# Export libraries to parent scope
set(SYSTEM_PLATFORM_LIBS ${SYSTEM_PLATFORM_LIBS} ${COCOA_LIBRARIES} PARENT_SCOPE)