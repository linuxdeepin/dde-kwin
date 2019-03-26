function clientExpandToFrame(client) {
    var applied_gtk_frame = client.applyGtkFrame;

    if (!client.clientSideDecorated && !applied_gtk_frame) {
        return;
    }

    var frame = workspace.__dde__.kwinUtils.getGtkFrame(client);

    if (!frame) {
        // 未获取到frame时恢复窗口的原始大小
        if (applied_gtk_frame)
            client.geometry = applied_gtk_frame.geometry;
        clearApplyGtkFrame(client);
        return;
    }

    var geometry = client.geometry;

    if (applied_gtk_frame) {
        // 使用旧的geometry
        geometry = applied_gtk_frame.geometry;
    }

    var new_geometry = {
        x: geometry.x - frame.left,
        y: geometry.y - frame.top,
        width: geometry.width + frame.left + frame.right,
        height: geometry.height + frame.top + frame.bottom
    }

    client.geometry = new_geometry;
    // 保存此时应用的数据，当窗口的_GTK_FRAME_EXTENTS属性改变后更新窗口大小
    workspace.__dde__.setObjectProperty(client, "applyGtkFrame", {
                                            frame: frame,
                                            geometry: geometry
                                        });
}

function clearApplyGtkFrame(client) {
    if (client.applyGtkFrame) {
        workspace.__dde__.setObjectProperty(client, "applyGtkFrame", undefined);
    }
}

function onClientMaximizedStateChanged (client, h, v) {
    if (!h || !v) {
        clearApplyGtkFrame(client);
        return;
    }

    clientExpandToFrame(client);
}

function setupConnection(client) {
    client['clientMaximizedStateChanged(KWin::AbstractClient*,bool,bool)'].connect(onClientMaximizedStateChanged);
    client.fullScreenChanged.connect(function () {
        if (client.fullScreen) {
            clientExpandToFrame(client)
        } else {
            clearApplyGtkFrame(client);
        }
    })
    client.clientSideDecoratedChanged.connect(function () {
        if (client.clientSideDecorated) {
            if (client.applyGtkFrame)
                clientExpandToFrame(client)
        } else {
            clearApplyGtkFrame(client);
        }
    })

    // 处理已经最大化或全屏的窗口
    if (client.fullScreen || workspace.__dde__.kwinUtils.isFullMaximized(client)) {
        clientExpandToFrame(client)
    }
}

if (workspace.__dde__) {
    // 支持_GTK_FRAME_EXTENTS时不做任何处理
    var gtk_frame_atom = workspace.__dde__.kwinUtils.getXcbAtom("_GTK_FRAME_EXTENTS", true);

    if (!workspace.__dde__.kwinUtils.isSupportedAtom(gtk_frame_atom)) {
        workspace.clientAdded.connect(setupConnection);
        // connect all existing clients
        var clients = workspace.clientList();
        for (var i=0; i<clients.length; i++) {
            setupConnection(clients[i]);
        }
    }
} else {
    print("Not found the '__dde__' object");
}
