var registeredBorders = [];

function init() {
    for (var i in registeredBorders) {
        unregisterScreenEdge(registeredBorders[i]);
    }

    registeredBorders = [];

    if (readConfig("Enabled", "true").toString() === "false") {
        return;
    }

    var borders = readConfig("BorderActivate", "").toString().split(",");
    for (var i in borders) {
        var border = parseInt(borders[i]);

        if (isFinite(border)) {
            var onBorderActive = function () {
                // 热区快捷方式只允许关闭最大化窗口
                if (workspace.activeClient.closeable && workspace.__dde__.kwinUtils.isFullMaximized(workspace.activeClient))
                    workspace.activeClient.closeWindow();
            }

            registerScreenEdge(border, onBorderActive)
        }
    }
}

options.configChanged.connect(init);
init();

