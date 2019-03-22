if (workspace.__dde__) {
    registerShortcut("Window Absolute Maximize", "Window Absolute Maximize", "Meta+Up", function() {
        workspace.__dde__.kwinUtils.fullmaximizeWindow(workspace.activeClient);
    })
    registerShortcut("Window Unmaximize", "Window Unmaximize", "Meta+Down", function() {
        workspace.__dde__.kwinUtils.unmaximizeWindow(workspace.activeClient);
    })
}
