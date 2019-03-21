var registeredBorders = [];

function init() {
    for (var i in registeredBorders) {
        unregisterScreenEdge(registeredBorders[i]);
    }

    registeredBorders = [];

    var borders = readConfig("BorderActivate", "").toString().split(",");
    for (var i in borders) {
        var border = parseInt(borders[i]);

        if (isFinite(border)) {
            var onBorderActive = function () {
                workspace.activeClient.closeWindow();
            }

            registerScreenEdge(border, onBorderActive)
        }
    }
}

options.configChanged.connect(init);
init();

