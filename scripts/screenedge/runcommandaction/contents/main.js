var registeredBorders = [];

function runCommand(program, arguments) {
    print(program, typeof(program));
    print(arguments, typeof(arguments));

    callDBus("com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager",
             "RunCommand", program, arguments);
}

function parseCommand(border) {
    var border_string = String(border)
    var program = readConfig("Border" + border_string + "Program", "").toString();
    var args = readConfig("Border" + border_string + "Args", "").toString().split(",");

    if (!program) {
        return false;
    }

    if (!(args instanceof Array)) {
        if (args instanceof String) {
            args = [args]
        }
    }

    var index = registeredBorders.push({
                                           border: border,
                                           program: program,
                                           args: args
                                       }) - 1;

    var onBorderActive = function () {
        var borderObj = registeredBorders[index];
        runCommand(borderObj.program, borderObj.args)
    }

    registerScreenEdge(border, onBorderActive);

    return true;
}

function init() {
    for (var i in registeredBorders) {
        unregisterScreenEdge(registeredBorders[i].border);
    }

    registeredBorders = [];

    var borders = readConfig("BorderActivate", "").toString().split(",");
    for (var i in borders) {
        var border = parseInt(borders[i]);

        if (isFinite(border) && !parseCommand(border)) {
            print("Invalid border set with:" + borders[i]);
        }
    }
}

options.configChanged.connect(init);
init();
