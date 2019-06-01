<a name="0.0.8"></a>
## 0.0.8 (2019-06-01)


#### Features

*   allow configuration items to override the default theme ([ce46a721](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/ce46a72163ce962b57d8983a37f2ffe063ec8068))
*   add the "deepin-chameleon" window decoration for kwin ([59680022](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/596800228ece3778e6670a13fcc2c1f921758912))
*   support get the shoutcut default value ([7e18ace3](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/7e18ace36573066d28bf80e9bba3c4315658ecd1))

#### Bug Fixes

*   correct the path to find translation files ([70e563e2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/70e563e2990b78bf44ecf60ba33664874161004e))
*   ignore the RuleBook::save function of kwin ([e0ab3f6d](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/e0ab3f6d09b63b1bff0460a6b29bed204a4f3fa1))
*   execute dirname before setting the LD_PRELOAD environment ([41481b9e](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/41481b9e30f78e64c53c13a4ba8692a268cf1d9e))



<a name="0.0.7"></a>
## 0.0.7 (2019-05-23)


#### Features

*   support KWin highlightWindows effect for PreviewWindow ([1a28d4a1](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/1a28d4a1e7dcf006c24e00f355c6c9c1384483a3))
*   overwrite the window menu ([42fb49fe](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/42fb49fead923f68da81ca8e26543ac4b39c3186))
*   Support for specifying the version of kwin on build ([f27e571f](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/f27e571f5307429ce74e5f1965cc6f4d2ebee9d2))
* **multitaskingview:**  add multitaskingview ts ([7f5a9697](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/7f5a9697b48a37b6c54ff396093c19fd7ffc845b))

#### Bug Fixes

*   automatically append the wallpaper's gsetting value item ([15328ffe](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/15328ffe83acaf49b61c97875851f6bc92898c45))
*   clean LD_PRELOAD env ([340c297c](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/340c297c712fe3a7fe64c840289b9f4e89288801))
*   the window was closed incorrectly ([87d942cf](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/87d942cf363e0255dbd7ba52300fa43ea96b6150))
*   the "Window to Desktop" shoutcuts is invalid ([4d920b19](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/4d920b19e300025ac2c61ba1860f094e91f29926))
*   No warning when opening the multitasking view app in 2D window manager ([c3501e54](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/c3501e545fb73e395a420bbe68d854953a0ce21a))
*   do not print warning messages when the build and runtime versions are consistent ([4ee6f0c2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/4ee6f0c2a78e9d77eee4b4d9d27c686459d925c4))
*   keep window to viewable if it is minimized ([921ae1f5](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/921ae1f5c6049e7f25402debe29f3b972ca160e4))
*   disable theme cache of FrameSvg ([6ced80ed](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/6ced80ed5471e68b303b6456d567cf01f0f6a991))



<a name="0.0.6"></a>
## 0.0.6 (2019-05-09)


#### Bug Fixes

*   install the "kdeglobals" file ([7fe147ad](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/7fe147ad2b5c76c6009c623367632c819bb85368))
*   The compositingEnabledChanged signal is sent only when the user actively sets it ([bd0af31c](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/bd0af31c39ddaee902cd371c308fc4193e913f3d))
*   the window default cursor of the gtk application does not follow the theme ([3b467f97](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/3b467f97821a27800a907b0562e34b43a36d985c))

#### Features

*   disable window manager window menu ([e889bda0](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/e889bda07084c8ef2121b33a2ef5b6b9e693cfd9))
*   preview window give priority to use the dbus interface by kwin ([9516ab8a](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/9516ab8a5757feda8bbdea7f23a2dcfe32dcf4c1))
*   support PresentWindows ([42e18684](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/42e18684f0ec58f21b7da04e193d6f20193c1a38))
*   support set cursor theme/size for kwin ([f5c7ad01](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/f5c7ad01bd478d47a2757070973e733430928f33))



<a name="0.0.5"></a>
## 0.0.5 (2019-04-24)


#### Features

*   add kwin multitaskingview desktop file ([dc701963](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/dc70196388bebe2a613c13396067bd8da464cd76))

#### Bug Fixes

*   start kwin_no_scale when the kwin restart with crash ([f040ae65](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/f040ae65bbedcc49544741ec78a02a8cbd6b7513))



<a name="0.0.4.1"></a>
## 0.0.4.1 (2019-04-22)


#### Bug Fixes

*   hidden the titlebar all button at left area ([c2ed5794](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/c2ed579499eec71a093f1d30dfd79a5436fb27a8))



<a name="0.0.4"></a>
## 0.0.4 (2019-04-22)


#### Features

*   diasble minimal for dialog type windows ([a1959eb2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/a1959eb2b774107d052c875a81d01661c4b45085))



<a name="0.0.3.3"></a>
## 0.0.3.3 (2019-04-17)


#### Bug Fixes

*   add depends for kwin ([023d7899](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/023d789963383dd938259bc2e297948aa2a7c9d7))
*   The Mischievous objects are sometimes not initialized ([4979fd77](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/4979fd774049311f94254e4d0dfb25dafcde8c52))
*   disable everything zone action of kwin with default ([acc68714](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/acc6871445690b6adcdad1330834831fb427cd2a))



<a name="0.0.3.2"></a>
## 0.0.3.2 (2019-04-12)


#### Features

*   support wait kwin started on startdde ([3112f3bb](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/3112f3bbb484704abc6a59b847a99631e470434c))

#### Bug Fixes

*   transform all sign keys ([c77a822b](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/c77a822bf3ca5086a45d8a23e969474f606d492b))
*   undefined symbol "_ZN4KWin9Workspace15quickTileWindowE6QFlagsINS_13QuickTileFlagEE" on kwin 5.8.6 ([00e155fa](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/00e155fa2058fe05d97df3f53dc82862ed29c1cc))



<a name="0.0.3.1"></a>
## 0.0.3.1 (2019-04-10)


#### Bug Fixes

*   build failed on libkf5windowsystem-dev 5.28.0 ([7c6f94fd](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/7c6f94fdd67442c90a849001b9ec657fd737c9f6))



<a name="0.0.3"></a>
## 0.0.3 (2019-04-09)


#### Bug Fixes

*   can not enable compositing for kwin ([0610e555](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/0610e55507fb77f21b665a077736e19f8184256f))
*   delete old action before setting the zone ([a8994862](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/a89948622c3a892ad095578bd4267e686d53c379))
*   zone setting is invalid ([fcdb4cfe](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/fcdb4cfeaadf5c0bb83557a30f85f65bd340c105))
*   build package failed ([1bdb85b3](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/1bdb85b32fb6218f0e568d762e2e633dacb5470b))
*   depends packages on debian stable ([41464c0c](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/41464c0c35e8f981afdf2f85883c640bee51f187))

#### Features

*   support set the zone enabled property ([e27a92be](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/e27a92be5c4ac6f485bd7623a561748e9721d55c))
*   add a new tabbox theme ([709b73e3](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/709b73e3472d19303c6aaf4a897934b865f60ccb))
*   implement show workspace/windows interfaces ([875d0654](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/875d0654133cbcece6470cc13951be4d08749f37))
*   add property "compositingEnabled" for the com.deepin.wm dbus interface ([1eeb7fad](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/1eeb7fadcad7743c6d24f72419e273d412240de3))
*   support set window decoration theme ([a2de87f2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/a2de87f27d6cfddcc67eec1bd2db3b80c655fe68))
*   add show workspaces manager view interface ([4540714d](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/4540714d242b29349b3982bb775365e276801e58))



<a name="0.0.2"></a>
## 0.0.2 (2019-04-04)


#### Features

*   set default tasks switch manager ([e9f92dcb](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/e9f92dcb38a5574fc0f05e06d72c632c8f8aface))
*   implemention of TileActiveWindow ([669aa446](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/669aa44675a6d9c07952fe8a15fa03503d8b31b2))
*   implemention two actions of PerformAction ([d2cb55e2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/d2cb55e2d7ea5827571a8aed30a22bbb54fa3972))
*   implemention of BeginToMoveActiveWindow ([f43b2230](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/f43b2230686aa9b9a951f93059e9badb212711fd))

#### Bug Fixes

*   build failed on Qt 5.7.x ([df8feedf](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/df8feedf852332d0ac5fe3546b52fc47a9723d70))



<a name="0.0.1"></a>
## 0.0.1 (2019-04-02)


#### Bug Fixes

*   files not be installed ([72b7f0e4](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/72b7f0e4a498c5e04bf2c391472c5dff36c1079d))
*   Invalid return statement ([983db12e](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/983db12e1fd6a9530bb267bb35b968f27acee3b7))
*   "unaximizeWindow" => "unmaximizeWindow" ([272e465b](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/272e465bf6bfa2154131f3b864800e509d3ec60e))
*   install the script files ([33ade5d7](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/33ade5d789504f994198eef5342ae2d0f3fa5505))

#### Features

*   implemention of SwitchApplication ([2cb3cb4a](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/2cb3cb4accdcb12cdd0bb10223b2a20970c70f7a))
*   compatible with deepin-wm ([51afee74](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/51afee74af4f09e0c3044af6e941050403cbf124))
*   impletion of GetAllAccels ([d292bb03](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/d292bb03246852cd7feff61f1cc8f7260149dd66))
*   add dbus interface of old deepin wm ([22a57226](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/22a57226240c7d3db4719542f575f8d320790176))
*   add kwin script ([fadc9bc6](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/fadc9bc651a9e86a08bd2a47bf9fe65c997bc11c))
*   add the "dde-launcher" script ([879f1c38](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/879f1c3800e59946993efe4929ba09811e7e03c0))
*   add unmaximize global shortcut for kwin ([15474fca](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/15474fcadb1e299b0fc8313483122ceceffafffd))
*   add kwin-xcb platform plugin ([bbeb4ba0](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/bbeb4ba04667e87070ab358f826fa00981e8c48a))
*   screen edget support "close active window"/"run custom command" ([a70fa4c2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/a70fa4c27e423da17e7c19a7c136569decee81e4))
*   set shortcut Meta+Down of "Window Unmaximize" ([270bacc9](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/270bacc9b5e27f6c3124ba55aab9ccec2d51f641))
*   add decoration themes(deepin/deepin-dark) and set "deepin" to default ([e0f925c2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/e0f925c2423766a47dc2b83a1f4b44f8f1cf5ec9))
*   do force not strict geometry for dde-launcher ([7b4c8783](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/7b4c8783a158b6416a2584ef787b90a49d5908ce))
*   add kwin_no_scale file to /usr/bin ([c4e6b14f](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/c4e6b14f8cdc8bc1b36bf1f3be3dc80eff38f54a))
*   change kwin default global shortcuts to be consistent with deepin-wm ([8a219847](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/8a2198476890c2167f68116594e973f8e9bafba7))



<a name="0.0.1"></a>
## 0.0.1 (2019-04-02)


#### Features

*   implemention of SwitchApplication ([2cb3cb4a](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/2cb3cb4accdcb12cdd0bb10223b2a20970c70f7a))
*   compatible with deepin-wm ([51afee74](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/51afee74af4f09e0c3044af6e941050403cbf124))
*   impletion of GetAllAccels ([d292bb03](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/d292bb03246852cd7feff61f1cc8f7260149dd66))
*   add dbus interface of old deepin wm ([22a57226](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/22a57226240c7d3db4719542f575f8d320790176))
*   add kwin script ([fadc9bc6](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/fadc9bc651a9e86a08bd2a47bf9fe65c997bc11c))
*   add the "dde-launcher" script ([879f1c38](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/879f1c3800e59946993efe4929ba09811e7e03c0))
*   add unmaximize global shortcut for kwin ([15474fca](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/15474fcadb1e299b0fc8313483122ceceffafffd))
*   add kwin-xcb platform plugin ([bbeb4ba0](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/bbeb4ba04667e87070ab358f826fa00981e8c48a))
*   screen edget support "close active window"/"run custom command" ([a70fa4c2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/a70fa4c27e423da17e7c19a7c136569decee81e4))
*   set shortcut Meta+Down of "Window Unmaximize" ([270bacc9](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/270bacc9b5e27f6c3124ba55aab9ccec2d51f641))
*   add decoration themes(deepin/deepin-dark) and set "deepin" to default ([e0f925c2](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/e0f925c2423766a47dc2b83a1f4b44f8f1cf5ec9))
*   do force not strict geometry for dde-launcher ([7b4c8783](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/7b4c8783a158b6416a2584ef787b90a49d5908ce))
*   add kwin_no_scale file to /usr/bin ([c4e6b14f](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/c4e6b14f8cdc8bc1b36bf1f3be3dc80eff38f54a))
*   change kwin default global shortcuts to be consistent with deepin-wm ([8a219847](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/8a2198476890c2167f68116594e973f8e9bafba7))

#### Bug Fixes

*   files not be installed ([72b7f0e4](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/72b7f0e4a498c5e04bf2c391472c5dff36c1079d))
*   Invalid return statement ([983db12e](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/983db12e1fd6a9530bb267bb35b968f27acee3b7))
*   "unaximizeWindow" => "unmaximizeWindow" ([272e465b](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/272e465bf6bfa2154131f3b864800e509d3ec60e))
*   install the script files ([33ade5d7](https://github.com/linuxdeepin/dde-kwin/tree/master/commit/33ade5d789504f994198eef5342ae2d0f3fa5505))



