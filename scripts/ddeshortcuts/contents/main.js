//if (workspace.__dde__) {
    registerShortcut("Window Absolute Maximize", "Window Absolute Maximize", "Meta+Up", function() {
        workspace.activeClient.setMaximize(true, true);
    })
    registerShortcut("Window Unmaximize", "Window Unmaximize", "Meta+Down", function() {
        workspace.activeClient.setMaximize(false, false);
    })
//}
