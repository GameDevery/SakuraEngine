#pragma once

namespace skr {
class SystemApp;
class ISystemEventSource;
class ISystemWindowManager;
class IME;

// Factory functions for Cocoa backend
ISystemEventSource* CreateCocoaEventSource(SystemApp* app);
void ConnectCocoaComponents(ISystemWindowManager* window_manager, ISystemEventSource* event_source, IME* ime);
}