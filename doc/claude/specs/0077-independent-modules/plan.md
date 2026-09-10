---
spec: 0077-independent-modules
phase: plan
status: approved
updated: 2026-09-08
---

# Plan 0077 — Independent core modules

> **Phase 2 of 4 — the HOW.** The technical design that satisfies every requirement in
> [`spec.md`](./spec.md). Grounded in six read-only audits of the tree on 2026-09-08 (enum
> vocabulary, Misc utilities, app-state/licensing reaches, the Devices↔Pipeline seam, the
> Storage/Api/Ui edges, build/gates/tests); every path below was confirmed by grep.
> Gate: do not start `/ss-tasks` until a human marks this `approved`.

## Approach (one paragraph)

Six working-tree phases, each compiled and run by the maintainer before the next, landing as
one commit. Phase 0 wires the bus into the composition root by injection and moves the shared
vocabulary down: the `SerialStudio` enums become a `Q_NAMESPACE` in `Core`, the Qt-Core-only
helpers, `AppInfo.h`, the header-only third-party queues and the crypto helpers follow, which
alone removes about 200 of the 270 includes into `app/src`. Phase 1 relocates the GUI-free
`Misc/` classes into `Core`, replaces every lower-library `showMessageBox` with a Core prompt
seam the root binds to the existing Ui implementation, moves the project-editor UI block into
`Ui`, and turns notification posts into the existing `NotificationRaised` topic. Phase 2 makes
the remaining upward *facts* retained bus topics (operation mode, frame config, licence,
connection state, replay state, audio format, extension catalog, dashboard view state) with
`AppState` moving into `Pipeline` and a Core-side atomic licence flag for the hot readers.
Phase 3 resolves the hotpath seams without the bus: the frame value types and `HAL_Driver.h`
move to `Core`, an ingest-binder interface implemented by `PipelineHost` takes reader and
stream-worker ownership away from `Devices`, one `IBlockSink` base replaces the eight concrete
sink types in `BlockPublisher`, two raw-tap interfaces cover the per-chunk and per-frame taps,
and the MQTT *publisher* moves to `Storage` beside the other sinks. Phase 4 finishes the
sideways commands with five small interfaces (command executor, replay plot sink, dashboard
control, device writer, checkpoint store) and moves the eight Ui-bound API handlers into `Ui`.
Phase 5 cuts the CMake include roots and links to downward-only, flips `layer-verify.py` to
strict with an R2 check that reads the CMake declarations, converts the allowed-edge singleton
reaches to injected references, links the archives from the unit tier, adds the per-library CI
build and the publish/subscribe census, and deletes `MessageBus::instance()`.

## Named alternatives considered

- **Bus-everything.** Every cross-library reach becomes a message. Rejected: `apiCall`,
  `showMessageBox` questions, replay window loads, device writes and project-generation flows
  need a synchronous answer, and the bus rejects blocking delivery by design; the block and raw
  taps are per-block/per-frame and the bus is banned there.
- **Devices-over-Pipeline.** Let `Devices` include `Pipeline` and kill 58 edges by fiat. The
  spec chose siblings (Q2): the drivers, the benchmark root and the headless roots must build
  without the parse pipeline, and only the sibling shape makes "compile Devices alone" real.
- **Relocate-and-interface (chosen).** Vocabulary and value types down, eleven small
  interfaces owned below both parties for the hot and synchronous seams, the bus for facts and
  fire-and-forget commands, injected references for the allowed downward calls. This is the one
  shape in which the include gate can measure the outcome and the hotpath keeps its bound-pointer
  form.

## Phase map and edge accounting

Baseline: 594 upward includes over 16 edges (`scripts/layer-baseline.json`). "→0 by" names
the phase after which that edge is expected empty.

| Edge | Today | What it is | →0 by |
|---|---|---|---|
| Pipeline→App | 91 | `SerialStudio.h` 56, `AppState.h` 9, `readerwriterqueue.h` 8, `SessionContext.h` 7, `AppInfo.h` 5, licensing 4, `GRPCServer.h` 2 | P0 (enums, queue, AppInfo), P2 (AppState, licence), P3 (gRPC), P4 (SessionContext) |
| Ui→App | 63 | `SerialStudio.h` 33, `AppState.h` 9, licensing 16, `SessionContext.h` 3, `HotpathBenchmark.h` 1 | P0, P2, P3 (benchmark flag), P4 |
| Devices→App | 44 | `SerialStudio.h` 17, `AppState.h` 12, licensing/crypto 8, `miniaudio.h` 3, `GRPCServer.h` 2, queue 1 | P0, P2, P3 |
| Api→App | 37 | `SerialStudio.h` 20, `AppState.h` 8, licensing 5, `AppInfo.h` 2, `SessionContext.h` 2 | P0, P2, P4 |
| Storage→App | 35 | `SerialStudio.h` 16, `AppState.h` 9, licensing 7, `AppInfo.h` 3 | P0, P2 |
| Pipeline→Ui | 95 | `Utilities.h` 29, `Translator.h` 16, `IconEngine.h` 10, `IconRegistry.h` 9, `Dashboard.h` 5, `WidgetExtensions.h` 5, `WorkspaceManager.h` 4, `CommonFonts.h` 4, `ThemeManager.h` 4, `TimerEvents.h` 3, `AudioExport.h` 2, `JsonValidator.h` 2 | P1 (utilities, editor block, dead includes), P2 (Dashboard/WidgetExtensions topics), P3 (AudioExport sink) |
| Devices→Ui | 36 | `Utilities.h` 18, `Translator.h` 7, `TimerEvents.h` 4, `Console/Handler.h` 2, diagnostics 2, `AudioExport.h` 1, fonts/theme 2 | P1, P3 (console tap), P4 (diagnostics report) |
| Storage→Ui | 21 | `WorkspaceManager.h` 8, `Utilities.h` 6, `Dashboard.h` 4, `TimerEvents.h` 2, `PasswordHash.h` 1 | P1, P2 (view state), P4 (replay plot sink) |
| Api→Ui | 23 | Ui-bound handlers 17, `BackupManager.h` 4, `Utilities.h` 2 | P1 (prompt), P4 (handlers, checkpoint store) |
| Devices→Pipeline | 58 | ingest lifecycle 7, ConnectionManager→FrameBuilder/ProjectModel 13, driver project generation 19, MQTT publisher stack 19 | P3 |
| Pipeline→Devices | 50 | `ConnectionManager.h` 19 (4 dead), `MQTT/Publisher.h` 14 (6 dead), `PublisherScriptEditor.h` 9 (8 dead), `HAL_Driver.h` 5, `Audio.h`/`Modbus.h`/`OpcUaWire.h` 3 | P1 (dead), P3, P4 (Modbus importer) |
| Pipeline→Storage | 14 | Export/Player headers in FrameBuilder, BlockPublisher, ControlScript | P2 (replay topic), P3 (sink base) |
| Storage→Devices | 11 | `ConnectionManager.h` (payload injection, connect state, link stats), `HAL_Driver.h` (`CapturedDataPtr`), 3 dead | P1 (dead), P3 (Core types), P4 |
| Pipeline→Api | 9 | `Server.h` 2 (sink), script `apiCall` 7 | P3, P4 |
| Devices→Storage | 4 | `hotpathTxRawBytes` tap 1, `enabledChanged` connects 3 | P3 |
| Devices→Api | 3 | `Server.h` taps 2, `EnumLabels.h` 1 | P0 (labels), P3 |

## Affected subsystems & files

Grouped by phase. "→" is a `git mv`; a moved file keeps its SPDX header and its
`(file, gating)` pair; every `Q_OBJECT` header is listed in exactly one target.

### Phase 0: bus injection and the vocabulary drop

| File | Change |
|------|--------|
| `core/Core/Bus/MessageBus.{h,cpp}` | Keep `instance()`/`setInstance()` for now (deleted in P5); add `@file` checklist for `Messages.h` growth; store handlers as `shared_ptr<const ErasedHandler>` (review item 6). |
| `core/Core/Bus/Messages.h` | Extend vocabulary (table in "Architecture"): `ConnectionStateChanged` gains `connecting`, `busType`; `LicenseStateChanged` gains `tier`, `trialExpired`; new retained/request topics. Field additions are safe now: zero production subscribers. |
| `app/src/SessionContext.{h,cpp}` | Slot 0: `std::unique_ptr<Core::Bus::MessageBus>` adopted first, released last; `create<T>(bus)` passes the bus to the nine module ctors. |
| `app/src/Misc/ModuleManager.cpp` | `instantiateCoreModules()` constructs the bus before `Translator`; `setInstance()` for the transitional accessor; `setupExternalConnections(bus)` on every Meyers-singleton module that needs to publish or subscribe. |
| Nine adopted modules (`AppState`, `UI::Dashboard`, `Console::Handler`, `FrameParser`, `FrameBuilder`, `ProjectModel`, `IO::PipelineHost`, `IO::ConnectionManager`, `NotificationCenter`) | Ctor takes `Core::Bus::MessageBus&`, stores `m_bus`; `Subscription` members. `ProjectModel`'s ctor closure gains no reach (bus is built before it). |
| `app/src/SerialStudio.h` → `core/Core/SerialStudio.h` (+ new `core/Core/SerialStudio.cpp`) | `namespace SerialStudio { Q_NAMESPACE ... Q_ENUM_NS }` with every enum verbatim (`BUILD_COMMERCIAL` blocks kept; `BarPanel=90`, `Extension=100` pinned); `XAxisMode`, `XAxisPolicy`, `kWidgetApiVersion*`; the Qt-Core-only helpers (`toDouble` overloads, `hexToBytes`, `resolveEscapeSequences`, `hexToString`, `stringToHex`, `escapeControlCharacters`, `normalizeIconPath`, `textEncodingName/FromName`, `groupWidgetId/FromId`, `datasetWidgetId/FromId`, `isGroupWidget`, `isDatasetWidget`, `isDashboardTool`, `dashboardWidgetPaintsTitle`, `*EligibleForWorkspace`, `searchMatches`, `dashboardWidgetIconId`); `tr()` callers use `QCoreApplication::translate("SerialStudio", …)` so the `.ts` context survives. |
| `app/src/SerialStudioFrameSupport.cpp` → `core/Pipeline/DataModel/FrameSupport.{h,cpp}` | `commercialCfg` ×2, `groupXAxisMode`, `resolveXAxisPolicy`; `encodeText`/`decodeText` (`QTextCodec`, Core5Compat) as `core/Pipeline/DataModel/TextCodec.{h,cpp}`. Moves again to `Core` in P3 with `Frame.cpp` (except `TextCodec`, which stays in Pipeline). |
| `app/src/SerialStudio.cpp` → `core/Ui/UI/SerialStudioHelpers.{h,cpp}` | Class `UI::SerialStudioHelpers : QObject`, the Ui-dependent statics: `getDashboardWidget(s)` (→ P2 resolver), `extensionGroupWidgetCount`, `dashboardWidgetIcon`, `getDatasetColor*`, `getDeviceColor*`, `getGroupColorOverride`, `isAnyPlayerOpen` (→ P2 topic), `dashboardWidgetTitle`, `textEncodings`; registered as QML singleton `SerialStudioHelpers`. |
| `app/src/Misc/ModuleManager.cpp` | `qmlRegisterUncreatableMetaObject(SerialStudio::staticMetaObject, "SerialStudio", 1, 0, "SerialStudio", …)` replaces `qmlRegisterSingletonType<SerialStudio>`; `qmlRegisterSingletonInstance("SerialStudio", 1, 0, "SerialStudioHelpers", …)`. |
| 26 QML call sites in 14 files (`CommandModel.qml`, `PaletteModel.qml`, `AddWidgetDialog.qml`, `ConstantsLibraryDialog.qml`, `DataTablesView.qml`, `GroupFolderView.qml`, `GroupsView.qml`, `SystemDatasetsView.qml`, `TableFolderView.qml`, `WorkspaceFolderView.qml`, `WorkspacesView.qml`, `DashboardCanvas.qml`, `ExternalWidgetWindow.qml`, `Taskbar.qml`, `WidgetDelegate.qml`, `WorkspaceView.qml`, `ProjectStructure.qml`, `MiniWindow.qml`, `DashboardOutputPanel.qml`) | `SerialStudio.fn(` → `SerialStudioHelpers.fn(`; the 270 enum reads stay verbatim. |
| 127 `#include "SerialStudio.h"` sites under `core/` | → `#include "Core/SerialStudio.h"`; `activated()` callers → `Core::License::activated()` (P2 finishes the licensing block; the flag file lands here). |
| `app/tests/session_context_stub.cpp:50` | Delete the `SerialStudio::staticMetaObject` stub line (Core's moc defines it). |
| `scripts/generate-property-registry.py` (+ regenerate the six artifacts) | Include literal → `Core/SerialStudio.h`; `PropertyHooks.h` literal updated in P1. |
| `scripts/registry-verify.py:428,1111` | Path constants → `core/Core/SerialStudio.h` / `core/Ui/UI/SerialStudioHelpers.cpp`. |
| `app/src/AppInfo.h` → `core/Core/AppInfo.h` | Verbatim; 9 includers under `core/` re-pointed. |
| `app/src/ThirdParty/{readerwriterqueue.h,atomicops.h,readerwritercircularbuffer.h,fast_float.h}` → `core/Core/ThirdParty/` | `REUSE.toml:119-146` paths updated; `app/CMakeLists.txt:322-325` HEADERS entries removed; `code-verify.py:1873` basename rule unchanged. |
| `app/src/ThirdParty/miniaudio.{h,cpp}` + `ss_apply_miniaudio_definitions` (`app/CMakeLists.txt:185-214`) → `core/Devices/ThirdParty/` + `core/Devices/CMakeLists.txt` | The function moves with the file so Pipeline no longer needs it (P3 removes `Audio.h` from `QuickPlotBuilder`). |
| `app/src/Licensing/SimpleCrypt.{h,cpp}` → `core/Core/Crypto/SimpleCrypt.{h,cpp}`; new `core/Core/Crypto/MachineKey.{h,cpp}` | Amended during P0: the key is computed by the commercially licensed fingerprint and cannot be lifted, so Core holds a root-published `Core::Crypto::machineKey()` (set once in `instantiateCoreModules()` from `MachineID`), the same shape as `Core::License`; `CredentialVault` and `KeyVault` read it. `MachineID` stays in `app/src`. |
| `core/Devices/MQTT/CredentialVault.{h,cpp}` → `core/Core/Crypto/CredentialVault.{h,cpp}` | Shared by InfluxDB, the MQTT publisher and `MqttHandler`. |
| `core/Api/API/EnumLabels.{h,cpp}` → `core/Core/EnumLabels.{h,cpp}`; `core/Ui/UI/LayoutPatterns.{h,cpp}` → `core/Core/LayoutPatterns.{h,cpp}` | Pure enum↔slug tables. |
| `core/Core/CMakeLists.txt`, `core/Pipeline/…`, `core/Ui/…` | Source lists; Core stays `Qt6::Core` only (`Q_NAMESPACE` needs moc, already on). |
| `core/Core/License.{h,cpp}` (new) | `Core::License`: `std::atomic<bool> activated`, `std::atomic<quint8> tier`, `trialExpired`; inline `activated()`; `set…()` called by the root only. The hot readers (MQTT driver per message, MQTT publisher per block) read this, never the bus. |

### Phase 1: utilities down, editor UI up, prompt seam, notifications

| File | Change |
|------|--------|
| `core/Ui/Misc/TimerEvents.{h,cpp}` → `core/Core/TimerEvents.{h,cpp}` | Verbatim (Qt Core only). QML context name `Cpp_Misc_TimerEvents` unchanged. |
| `core/Ui/Misc/IconRegistry.{h,cpp}` → `core/Core/IconRegistry.{h,cpp}` | Verbatim (`QDirIterator` over qrc is Qt Core); `IconEngine`/`IconRegistryLegacy` stay in Ui. |
| `core/Ui/Misc/Translator.{h,cpp}`, `Misc/LanguageTable.{h,cpp}` → `core/Core/Translator.{h,cpp}`, `core/Core/LanguageTable.{h,cpp}` | `setLayoutDirection` (Gui) removed from `setLanguage`; `Ui::ModuleManager` connects `languageChanged` → `qApp->setLayoutDirection` (already connects `retranslate`); `welcomeConsoleText` (reads `featureTier()`) moves to `core/Ui/Console/WelcomeText.{h,cpp}` and the Q_PROPERTY to `Console::Handler`; QML `Cpp_Misc_Translator.welcomeConsoleText` → `Cpp_Console_Handler.welcomeConsoleText` (one binding). |
| `core/Ui/Misc/WorkspaceManager.{h,cpp}` → `core/Core/WorkspaceManager.{h,cpp}` | `selectPath()` re-implemented over the prompt seam's `selectDirectory` so the QML slot survives verbatim. |
| `core/Ui/Misc/JsonValidator.h`, `Misc/PasswordHash.{h,cpp}`, `AI/MemoryStore.{h,cpp}` (+ `AI/Logging`, `AI/Redactor`) | → `core/Core/` (Qt Core only). `MemoryStore` moves because the assistant handler reads its constants; if the handler moves to Ui in P4 this move is unnecessary and is skipped. |
| `core/Core/Prompt/UserPrompt.h` (new), `core/Core/Prompt/IUserPrompter.h` (new) | `Core::Prompt::Icon`/`Button` enums (values mirror `QMessageBox`), `showMessageBox(...)` free function forwarding to a root-bound `IUserPrompter*` (null → `qWarning` + default button), `selectDirectory`, `revealFile`. |
| `core/Ui/Misc/Utilities.{h,cpp}` | Implements `IUserPrompter` by delegating to its existing `showMessageBox` (keeps the off-thread `invokeMethod` path at `Utilities.cpp:180-188`); root binds it. |
| 157 `showMessageBox` sites in `core/Pipeline` (18 files), `core/Devices` (10), `core/Storage` (6), `core/Api` (2); 2 `revealFile`; 2 `coloredSvgIcon` (editor files, move with the block) | `Misc::Utilities::showMessageBox(…, QMessageBox::X, …)` → `Core::Prompt::showMessageBox(…, Prompt::X, …)`. Mechanical, scripted, reviewed by diff. |
| Dead includes deleted | `Misc/Utilities.h` in 7 Pipeline + 9 Devices files; `Misc/Translator.h` in 7 `ProjectEditor*.cpp`; `Misc/WorkspaceManager.h` in 3 Storage exports; `UI/Dashboard.h` in `FrameBuilder.cpp`; `Misc/IconEngine.h` in 7 of 10 includers; `IO/ConnectionManager.h` in 4 `ProjectEditor*.cpp`; `MQTT/Publisher.h` + `PublisherScriptEditor.h` in 6 `ProjectEditor*.cpp`; `IO/HAL_Driver.h` in `Sessions/Verifier.cpp`; `IO/ConnectionManager.h` in `CSV/Export.cpp`, `MDF4/Export.cpp`. |
| Project-editor block → `core/Ui/ProjectEditor/` | `DataModel/ProjectEditor.{h,cpp}` + the 8 `Project/ProjectEditor*.cpp` (moved as the one class it is; not re-split here), `Project/ProjectEditorIcons.h`, `ProjectEditorItemIds.h`, `ProjectNavHistory.{h,cpp}`, `Project/PropertyHooks.{h,cpp}`, `DataModel/Editors/*` (except `CodeFormatter`, `ScriptTemplateCatalog`, `EditorFormatting`, which stay), `Generated/DatasetForm.cpp`, `Editors/FrameParserModel.*`, `Importers/ImporterCommon.h` (icon preview part), `DataModel/Generated/DatasetRegistry.h` stays (model). `ProjectEditor` was a `qmlRegisterType` from the root: unchanged. |
| `scripts/generate-property-registry.py`, `scripts/registry-verify.py:704`, `scripts/code-verify.py:3477-3478`, `app/tests/CMakeLists.txt` (13 `PropertyValidators.cpp` registrations) | Output/allowlist/test paths follow the move. |
| `core/Ui/ProjectEditor/ProjectEditor.{h,cpp}` + eight new sub-object classes (`EditorTree`, `EditorForms`, `EditorSelection`, `EditorCommit`, `EditorMqtt`, `EditorMultiSelect`, `EditorSummaries`, `EditorWiring`, one `.h`/`.cpp` pair each) | Maintainer direction 2026-09-08: the editor is re-formed into the spec 0070 shape while it moves. Each former `ProjectEditor*.cpp` becomes one sub-object class owned by value by the facade; method bodies move verbatim, the facade forwards, QML-facing `Q_PROPERTY`/`Q_INVOKABLE` surface stays on the facade so `qmlRegisterType<ProjectEditor>` and every QML binding are unchanged. |
| `core/Pipeline/DataModel/Project/ProjectWorkspaces.cpp`, `ProjectPresentation.cpp` | `IconRegistry::iconById` now a Core include; `WidgetExtensions::persistedTypeToken` → a `Core::SerialStudio` static (pure string). |
| `core/Pipeline/DataModel/NotificationCenter.{h,cpp}` (amended 2026-09-08: stays in Pipeline; Pipeline already links Widgets/Qml, no include edge names it from below, and moving it would grow Api→Ui until P4) | Stays a SessionContext module (tray icon, QML model). Every lower-library `postWarning/postInfo`/`invokeMethod("postWarning")` → `m_bus.publish<NotificationRaised>(…)` (topic gains `channel`, `key`); the center subscribes with `Qt::AutoConnection` (queued from the pipeline thread and drivers). `installScriptApi(lua_State*)`/`(QJSEngine*)` → `core/Pipeline/DataModel/Scripting/NotificationScriptApi.{h,cpp}`: `notify/info/warning/critical` publish the topic; `clear` becomes a `NotificationClearRequested{channel}` request the center serves. `MQTT::Publisher` connects `notificationPosted` → subscribes to the topic instead. Api `NotificationsHandler` (history/resolve/markRead) becomes a Ui-owned handler in P4. |
| `core/Ui/Misc/ProblemCenter`, `ConnectionDiagnostics`, `Diagnostics/DiagnosticsShared.h` | `Diagnostics::Bus` enum + slug helpers → `core/Core/DiagnosticsTypes.h`; `ConnectionManager.cpp:1231` `onOpenSucceeded/Failed` → `DeviceOpenAttempted{bus, ok, reason}` topic the diagnostics runner subscribes to. |

### Phase 2: upward facts as retained topics

| File | Change |
|------|--------|
| `app/src/AppState.{h,cpp}` → `core/Pipeline/AppState.{h,cpp}` | Stays a SessionContext module; publishes `OperationModeChanged` (retained) from `setOperationMode` before `frameConfigChanged`, `FrameConfigChanged` (retained), `ProjectLoaded` from `onProjectLoaded`. `IO::FrameConfig` moves to `core/Core/IO/FrameConfig.h` (P3 confirms). |
| Consumers of `AppState::operationMode()` / `operationModeChanged` in Devices (`ConnectionManager.cpp` ×10, `StreamConfigBuilder`, `UiDriverSync`, `StreamWorkerPool`, `DeviceIoRouter`), Storage (CSV/MDF4/Sessions Export+Player, `DatabaseManager`, `Verifier`, `ReplaySynthesis`), Api (`DashboardHandler`, `ProjectFileCommands`, `ProjectParserCommands`, `SourceHandler`, `WorkspacesHandler`, Mirror), Ui (`Dashboard.cpp` ×7, `Taskbar`, `Console/*`, `ProjectCheckers`, `FileOpenEventFilter`, `Painter`) | Reads → `m_bus.latest<OperationModeChanged>()` or a subscribed cached member; the two Dashboard connections on one signal (`Dashboard.cpp:271` queued, `:750` direct) become two subscriptions with the same connection types, in the same registration order. Storage/Api/Ui may still include `Pipeline/AppState.h`; Devices may not. |
| `setOperationMode()` callers in Devices (`EthernetIp.cpp:1360`, `Iec104.cpp:1436`, `Modbus.cpp:820`, `OpcUa.cpp:734`, `S7.cpp:1345`, `MQTTSparkplug.cpp:720-723`) | Folded into the P3 `LoadGeneratedProjectRequested{json, switchToProjectFile, requestId}` request served by `ProjectModel`/`AppState`; completion arrives as `GeneratedProjectLoadFinished{requestId, accepted}` (Sparkplug's set/load/restore sequence restores on that reply). Storage/Api/Ui setters call `AppState` directly (downward). |
| `core/Pipeline/DataModel/FrameBuilder/ExternalWiring.{h,cpp}` (created in P0 for the player watch), `core/Pipeline/IO/PipelineHost.cpp:159-177`, `core/Devices/IO/ConnectionManager/DeviceIoRouter.cpp:180` | `m_operationMode` / the PipelineHost atomic / a new `DeviceIoRouter` cached bool are refreshed from `subscribe<OperationModeChanged>` (auto → queued into the pipeline thread for FrameBuilder, direct on GUI for the others). Never `latest<>()` on those paths. |
| `app/src/Misc/ModuleManager.cpp`, `app/src/Licensing/LemonSqueezy.cpp` | Root hook on `activatedChanged`: `Core::License::set(...)` then `publishState<LicenseStateChanged>{activated, tier, trialExpired}`; initial publish after the licensing block constructs (before `restoreLastProject`, spec 0042). |
| Ten `activatedChanged` receivers (`FrameBuilder.cpp:195`, `ProjectModel.cpp:641`, `ConnectionManager.cpp:790`, `MDF4/Export.cpp:530`, `Sessions/Export.cpp:891`, `InfluxDB/Export.cpp:545`, `Console/Export.cpp:212`, `Dashboard.cpp:367`, `AudioExport.cpp:672`, `Terminal.cpp:129`) and every `SerialStudio::activated()`/`featureTier()` read below `app/src` | `subscribe<LicenseStateChanged>(…, replayLatest=true)` / `Core::License::activated()`. `MQTT.cpp:1250` and `Publisher.cpp:1240,1262,1282` read the atomic. `LicensingHandler` (command surface over `LemonSqueezy`/`OfflineLicense`/`Trial`) stays in `app/src` in P4 as a root-registered handler. |
| `core/Devices/IO/ConnectionManager.cpp` `notifyConnectedStateChanged()` | `publishState<ConnectionStateChanged>{sourceId, connected, paused, connecting, busType}`. Subscribers: `FrameBuilder.cpp:571-579` (queued into pipeline), `PipelineHost.cpp:168-177` (direct, GUI, writes the atomics), `ProjectModel.cpp:634`, `ControlScript.cpp:85,257`, `QuickPlotBuilder` (`busType`), `ProjectLoader.cpp:878`, `Sessions::Player`/`CSV::Player`/`MDF4::Player` (`isConnected`), ProjectEditor (`contextsRebuilt`/`deviceListRefreshed`/`driverChanged` → `DeviceCatalogChanged{}` retained). |
| `core/Storage/{CSV,MDF4,Sessions}/Player.cpp` | `publishState<ReplayPlayerStateChanged>{playerId, open}` on `openChanged`; `FrameBuilder.cpp:625-628,810-820` keeps `m_playerOpen` as a 3-bit mask refreshed by subscription (queued into pipeline); `ControlScript.cpp:95-102` subscribes; `SerialStudioHelpers::isAnyPlayerOpen` reads `latest<>()` ×3. |
| `core/Devices/IO/Drivers/Audio.cpp` | `publishState<AudioCaptureFormat>{format, sampleRate, normalized}` on open/config; `QuickPlotBuilder.cpp:215-221` reads it (deleting `Audio.h` + miniaudio from Pipeline). |
| `core/Ui/UI/WidgetExtensions.cpp` | `publishState<WidgetExtensionCatalog>{entries: {id, scope, title, replaces}}` on `catalogChanged`; `core/Pipeline/DataModel/WidgetResolution.{h,cpp}` (new) hosts `getDashboardWidget(s)`/`extensionGroupWidgetCount` over `latest<>()` for the model files (`ProjectWorkspaces`, `ProjectWorkspaceRefs`, `ProjectEntities`, `ProjectEditorSummaries` after its move, Api `WorkspacesHandler`). |
| `core/Ui/UI/Dashboard.cpp` ↔ `core/Pipeline/DataModel/{ProjectModel,Project/ProjectLoader}.cpp` | Inversion: Dashboard reads/writes `points`/`plotTimeRange` on `ProjectModel` (downward) on `ProjectLoaded` and on its own setters; ProjectModel stops calling `Dashboard::setPoints/setPlotTimeRange` and stops connecting `pointsChanged`/`widgetCountChanged` (Dashboard publishes `DashboardStructureChanged{generation, widgetCount}`; ProjectModel subscribes). |
| `core/Ui/UI/Dashboard.cpp`, `core/Storage/Sessions/{Export,Player}.cpp` | `publishState<DashboardViewState>{json}` on `viewStateChanged` (debounce stays in Export); `DashboardViewStateRestoreRequested{json}` + `DashboardViewStateClearRequested{}` requests served by Dashboard. |
| `core/Ui/UI/Dashboard.cpp:671` | `Benchmark::HotpathBenchmark::active()` → `Core::Runtime::benchmarkActive()` (a Core module-static bool the benchmark root sets), same shape as `MirrorSession::mirroring()`. |

### Phase 3: the hotpath seams (value types, ingest binder, sinks)

| File | Change |
|------|--------|
| `core/Devices/IO/HAL_Driver.h` → `core/Core/IO/HAL_Driver.h` | Verbatim (header-only, Qt Core only, already compiled bare by the ctest tier). Includers in Pipeline (`FrameReader.h`, `FrameBuilder.h`, `LatestFrameTap.h`, `StreamWorker.h`, `FrameParserPipeline.cpp`), Ui (`Console/Handler.h`, `ImageView.h`) re-pointed. |
| `core/Pipeline/DataModel/{Frame.h,Frame.cpp,FrameKeys.h,DataBlock.h,ExportSchema.h,FrameConsumer.h,FrameConsumer.cpp}` → `core/Core/DataModel/` | Verbatim except: `Frame.cpp:270` `QColor::fromString` → `DataModel::colorStringValid(s)` (Core hook, function pointer installed by the root with the `QColor` check; unset accepts any string); `Frame.cpp` includes `Core/SerialStudio.h`, `Core/AppInfo.h`; `commercialCfg` reads `Core::License::activated()`. `FrameSupport.{h,cpp}` from P0 moves with it. `TextCodec` stays in Pipeline. |
| `core/Core/IO/{FrameConfig.h,StreamConfig.h}` | `IO::FrameConfig`, `StreamConfig`, `StreamChannelConfig` value types (from `core/Pipeline/IO/FrameConfig.h`, `StreamWorker.h`). |
| `core/Core/IO/IIngestBinder.h` (new) | `attach(deviceId, HAL_Driver*, const FrameConfig&)`, `reconfigure(deviceId, const FrameConfig&)`, `detach(deviceId)`, `attachStream(deviceId, HAL_Driver*, const StreamConfig&)`, `setStreamPaused`, `injectPayload(sourceId, CapturedDataPtr)`, `injectMultiSourcePayload`, `linkStats(deviceId) -> IO::LinkStats` (moves to `core/Core/IO/LinkStats.h`). Pure virtual, Qt Core only. |
| `core/Pipeline/IO/PipelineHost.{h,cpp}` | Implements `IIngestBinder`: owns `FrameReader`s keyed by device id (created on the GUI thread, configured, `moveToThread`, registered exactly as `registerFrameReader` does today), makes the queued `HAL_Driver::dataReceived → FrameReader::processData` connect itself (`DeviceManager.cpp:218-219` moves here), keeps `readyRead → routeFrames` Direct. `IO::StreamWorkerPool` (`core/Devices/IO/ConnectionManager/StreamWorkerPool.{h,cpp}`) → `core/Pipeline/IO/StreamWorkerPool.{h,cpp}` with its queued `blockReady → FrameBuilder::ingestStreamBlock` hop intact. |
| `core/Devices/IO/DeviceManager.{h,cpp}`, `IO/ConnectionManager.{h,cpp}` + sub-objects (`StreamConfigBuilder`, `DeviceTableQuery`, `UiDriverSync`, `DeviceIoRouter`) | `ConnectionManager` ctor takes `IIngestBinder&` (root passes `PipelineHost`); `DeviceManager` drops `m_frameReader`/`m_pipeline`, calls `binder.attach/reconfigure/detach`; `StreamConfigBuilder`/`DeviceTableQuery`/`UiDriverSync` read the retained `ProjectStructureSnapshot{sources, groups, luaFastMode, frameDetection}` (published by `ProjectModel` on structure change, Core `Frame` types) instead of `ProjectModel::instance()`; `UiDriverSync` publishes `Source0ConnectionSettingsChanged{busType, settings}` which `ProjectModel` serves (it still emits `sourceConnectionChanged` for the editor, the 2026-09-04 fix); `ProjectSources.cpp:307-350` / `ProjectLoader.cpp:877-895` (Pipeline reading the UI driver) invert into `UiDriverSync` subscribing to `ProjectLoaded`/`ProjectStructureSnapshot` and applying source-0 settings itself. `ConnectionManager.cpp:590` `ControlScript::runOnConnect` → `ControlScript` subscribes to `ConnectionStateChanged`. `:681,:965` `registerQuickPlotHeaders` → `IIngestBinder::resetQuickPlotHeaders()`. |
| `core/Core/IO/IRawByteTap.h`, `core/Core/IO/IRawFrameTap.h` (new) | `IRawByteTap::onDeviceBytes(deviceId, const CapturedDataPtr&)`, `onSentBytes(deviceId, const QByteArray&)`; `IRawFrameTap::onRawFrame(deviceId, const CapturedDataPtr&)`. |
| `core/Devices/IO/ConnectionManager/DeviceIoRouter.{h,cpp}` | Holds `std::array<IRawByteTap*, 6>` bound by the root (`API::Server`, `Console::Handler`, `Sessions::Export`, `MQTT::Publisher`, gRPC, `FileTransmission` stays Devices-internal); per-chunk loop over non-null taps on the GUI thread (chunk rate, unchanged thread). |
| `core/Pipeline/IO/PipelineHost.cpp:278-296` | `IRawFrameTap* m_rawFrameTap` (nullable, bound once) replaces `MQTT::Publisher::hotpathTxRawFrame`; pointer hoisted out of the drain loop. |
| `core/Core/DataModel/IBlockSink.h` (new) | `virtual void ingestBlock(const DataBlockPtr&) = 0; virtual bool sinkActive() const = 0;` + `signal sinkActivityChanged()`; `FrameConsumer<DataBlockPtr>` derives from it. Implemented by `CSV::Export`, `MDF4::Export`, `Sessions::Export`, `InfluxDB::Export`, `API::Server`, `MQTT::Publisher`, `Widgets::AudioExport`, `GRPCServer` (`sinkActive` = today's `exportEnabled()`/`enabled()&&clientCount()`/`hasActiveSessions()`). |
| `core/Pipeline/DataModel/FrameBuilder/BlockPublisher.{h,cpp}`, `FrameBuilder.cpp:646-756` | `Sinks` becomes `{IO::PipelineHost* pipeline; std::array<IBlockSink*, kMaxSinks> sinks; IBlockSink* server; IBlockSink* grpc}` (the two observers keep their masked-lane role by slot); `bindBlockSinks(std::span<IBlockSink*>)` fed by the root; `resolveAsyncSinks()` deleted; `wireAsyncSinkHooks` connects `sinkActivityChanged` → `refreshSinkFlag` (still `Qt::DirectConnection` where it was). `refreshLatestFrameCapture` reads `server->sinkActive()`. |
| `core/Devices/MQTT/` (publisher stack: `Publisher*`, `PublisherWorker*`, `CsvExpansion*`, `SparkplugPublisher*`, `TlsConfig*`, `TlsIdentity*`, `PublisherScript*`) → `core/Storage/MQTT/` | `IO::Drivers::MQTT` + `SparkplugSession` stay in Devices. `PublisherScriptEditor.{h,cpp}` → `core/Ui/ProjectEditor/`. Storage's allowed set gains `Protocols` (`layer-verify.py` table + CMake already links it). |
| `core/Devices/IO/Drivers/OpcUaWire.h` → `core/Protocols/OpcUa/OpcUaWire.h` | Two `SerialStudio::` uses now resolve to `Core/SerialStudio.h`. |
| `core/Devices/IO/Drivers/{EthernetIp,Iec104,S7,OpcUa,Modbus,MQTT}*` project generators | Build `DataModel::Group/Dataset` (Core), `publish<LoadGeneratedProjectRequested>`; `ProjectModel::saveDialogCompleted` reach → the `GeneratedProjectLoadFinished` reply. |
| `core/Devices/IO/Drivers/{UART,USB}.cpp` | `NotificationRaised` (already P1). |
| `app/src/API/GRPC/GRPCServer.{h,cpp}` | Derives `IBlockSink` + `IRawByteTap`; root binds; `ENABLE_GRPC` include/define/link on Pipeline and Devices (`app/CMakeLists.txt:686-700`) removed. |
| `app/src/Misc/CLI.cpp:407-409`, `app/src/Benchmark/HotpathBenchmark.cpp` | Benchmark root: `bindBlockSinks({})`, `Core::Runtime::setBenchmarkActive(true)`; includes trimmed to Core/Pipeline (it constructs `FrameReader`/`StreamProcessor` only). `instantiateCoreModules()` keeps constructing `ConnectionManager` for every root (spec 0039 slots unchanged). |

### Phase 4: sideways commands, handlers, context

| File | Change |
|------|--------|
| `core/Core/Api/CommandProtocol.h` (from `core/Api/API/CommandProtocol.h`), `core/Core/Api/ICommandExecutor.h` (new) | `CommandRequest`/`CommandResponse` value types; `execute(const CommandRequest&) -> CommandResponse` (synchronous, GUI-thread contract), `hasCommand`, `commandNames`. |
| `core/Api/API/CommandHandler.{h,cpp}` | Implements `ICommandExecutor`; `initializeHandlers()` becomes explicit `registerCoreHandlers(HandlerContext&)` called by the root (no lazy side effect in `instance()`); `HandlerContext` (new, `core/Api/API/HandlerContext.h`) carries `ConnectionManager&`, `ProjectModel&`, `FrameBuilder&`, `AppState&`, the players/exports, `NotificationCenter` via bus, `ICheckpointStore&`. |
| `core/Pipeline/DataModel/Scripting/{ScriptApiCall,ControlScriptWorker,MacroWorker}.cpp`, `core/Ui/AI/ToolDispatcher.cpp` + `AI/Tools/*` | Take `ICommandExecutor&` bound by the root; the GUI-thread marshalling (`setPipelineParkedOnGui`, `BlockingQueuedConnection` onto `qApp`) stays exactly where it is. |
| Ui-bound handlers → `core/Ui/Api/Handlers/` | `AssistantHandler`, `ConsoleHandler`, `DashboardHandler`, `DiagnosticsHandler`, `ExtensionHandler`, `ProblemsHandler`, `WindowHandler`, `WorkspacesHandler`, `NotificationsHandler`; registered by `UI::ApiHandlers::registerAll(CommandRegistry&, UiHandlerContext&)` from the root after the core set. `LicensingHandler` stays in `app/src` and registers last. |
| `core/Api/API/ICheckpointStore.h` (new); `core/Ui/Misc/BackupManager` | `snapshot(label)`, `restore(path)`, `list(limit)`, `backupDirectory()`; `CommandRegistry.cpp:271`, `ProjectDatasetCommands.cpp:551`, `ProjectGroupCommands.cpp:213` use the injected store. |
| `core/Pipeline/DataModel/IReplayPlotSink.h` (new); `core/Ui/UI/Dashboard` | `points()`, `plotTimeRange()`, `seekSeries()`, `seekKey()`, `bulkLoadPlotWindow(...)`, `clearPlotData()`; Dashboard implements; root binds into `CSV::Player`, `MDF4::Player`, `Sessions::Player` (`setPlotSink`). The ~30 Hz scrub path keeps its bulk-copy shape; one virtual call per tick. |
| `core/Pipeline/DataModel/IDashboardControl.h` (new) | `clearPlotData`, `setPoints`, `setClockEnabled`…, `actionIndexForId`, `activateAction`; Dashboard implements; `DashboardApi.cpp`, `DeviceWriteApi.cpp` use the bound pointer (script API, synchronous). |
| `core/Core/IO/IDeviceWriter.h` (new) | `writeDataToDevice`, `writeAndArmReply`, `pollReplyBuffer`, `disarmReplyCapture`; `ConnectionManager` implements; bound into `DeviceWriteApi`, `ControlScriptWorker`, `FrameBuilder` (auto-execute actions), `Output::Base` (already Ui). |
| Players (`CSV/Player.cpp:450-456,1475,1511`, `MDF4/Player.cpp`, `Sessions/Player.cpp`, `ReplaySynthesis.cpp`) | `processPayload` → `IIngestBinder::injectPayload` on `PipelineHost` (downward); `isConnected` → `latest<ConnectionStateChanged>`; `disconnectDevice` → `DisconnectRequested{}`; `Sessions/Export::linkStats` → `PipelineHost::linkStats`; `Verifier::buildFrameConfig` → `IO::FrameConfigBuilder` moved to `core/Pipeline/IO/`. |
| `core/Pipeline/DataModel/Importers/ModbusMapImporter.cpp:719-726` | `publish<ModbusRegisterGroupsLoaded{json}>`; the Modbus driver subscribes and applies. |
| `app/src/SessionContext.{h,cpp}`, the eight library `instance()` forwarders (`FrameBuilder.cpp:224`, `ProjectModel.cpp:151`, `NotificationCenter.cpp:90`, `FrameParser.cpp:107`, `PipelineHost.cpp:92`, `ConnectionManager.cpp:139`, `Console/Handler.cpp:169`, `Dashboard.cpp:562`) | Each module gets a private `static X* s_instance` set by `SessionContext::adoptX()` (friend stays) and cleared in `shutdown()`; `instance()` asserts and dereferences it. No library file includes `SessionContext.h`. The five injected-context classes (`DBCImporter`, `ProtoImporter`, `MirrorPublisher`, `MirrorSession`, `BackupManager`) take the concrete modules they use by reference from the root. INV-4 holds: the adopted address never changes. |
| `core/Ui/Console/Handler` ↔ `DeviceIoRouter` | Console implements `IRawByteTap` (P3); `hotpathRxData/hotpathRxDeviceData/displaySentData` become the tap methods. |

### Phase 5: strict layering, injected references, gates, tests, docs

| File | Change |
|------|--------|
| `core/CMakeLists.txt:89-97`, `core/{Pipeline,Devices,Storage,Api,Ui}/CMakeLists.txt` | Delete the cycle loop; each target's `target_include_directories` lists only `core/<Self>` and the roots of its allowed lower layers (Core/Protocols via `core/`); `target_link_libraries` PUBLIC only its allowed lower layers plus Qt/third-party; `app/src` removed from every library. `add_subdirectory(core)` stays where it is (R1 of 0076). Executable adds `core/Pipeline` … roots for `app/src`. |
| `scripts/layer-verify.py` | `DEBT_LAYERS = ()`; baseline edges deleted; `Storage` allowed `+= Protocols`; new rule `cmake-root-violation`: parse `target_include_directories(` / `target_link_libraries(` blocks per `core/<Layer>/CMakeLists.txt` and the root file, compute the transitive PUBLIC closure, fail if it names a layer outside `LAYERS[layer]` or `app/src`. `--accept` refuses while any strict error stands (unchanged). |
| `scripts/code-verify.py`, `scripts/code_verify_rules.py` | Singleton census gains a `per_edge` block (`caller-layer→callee-layer`, using `layer-verify`'s `layer_of` and a class→header→layer index); gate: `cross_library_total` must be 0 outside `_SINGLETON_ROOT_FILES` (root list grows by `app/src/Misc/CLI.cpp`, `Benchmark/*`). New `--bus-census` (`scripts/bus-census.json`): regex over `(publish|publishState|subscribe|latest)<(?:Core::Bus::)?(\w+)>` in `core/` + `app/src`; errors: a topic in `Messages.h` with zero publishers or zero subscribers outside tests, any bus token in a `_HOTPATH_ASSERT_ALLOWED` file (the existing `bus-on-hotpath` stays as the per-line rule). |
| `core/Core/Bus/MessageBus.{h,cpp}` | Delete `instance()`/`setInstance()`; census allowlist empty. |
| Api handler lambdas (`ConnectionManager` 138, `ProjectModel` 122, `DatabaseManager` 17, `FrameBuilder` 12, players/exports ~30, `MQTT::Publisher` 3, `FrameParser` 3, `ControlScript` 2) and Ui facades (`ConnectionManager` 23, `ProjectModel` 18, players 10, `FrameBuilder` 4, `PipelineHost` 3) | Handlers capture `HandlerContext&`; Ui SessionContext modules (`Dashboard`, `Console::Handler`) take the lower modules by reference in their ctors (all constructed earlier in the pinned order); non-adopted Ui classes receive references through their existing `setupExternalConnections` or ctor from the root. `static auto& x = X::instance()` caches inside libraries are removed on those edges (INV-4 is what made them safe; a reference member is the same guarantee). |
| `app/tests/CMakeLists.txt` | Suites that recompiled `Frame.cpp`, `FrameSupport`, `PropertyValidators`, `DatasetSerialization`, `AppPlatform`, `JsWatchdog*`, `FrameReader.cpp`, `FrameConsumer.cpp`, `ReplayRowCodec`, native templates link `SerialStudio::Core` / `SerialStudio::Pipeline`; Devices/Storage/Api/Ui suites link their archive where the closure is now Core+Protocols(+Pipeline)+Qt. `session_context_stub.cpp` shrinks to what the executable-only classes still need. Header comment at `:28-36` rewritten. Residual recompile list recorded in tasks.md. |
| `.github/workflows/ci.yml` | New job `build-core-libraries` (ubuntu, restore-only Qt cache, modeled on `build-gpl3`): the unit-ci configure line, then `cmake --build build/unit-ci --target SerialStudio<Layer>` for each of the seven, in dependency order, each as its own step so the failing layer is named; not in `upload.needs`. Lint job adds `--bus-census --check`. |
| Docs | `CLAUDE.md` (Project Overview sentence, the `core/` contract row, Startup block: bus slot 0 + injection, hotpath block: the sink base and taps), `doc/claude/directory-map.md` (strict graph, new Core subdirectories, `app/src` composition-root-only list shrinks), `architecture/startup.md` (pinned order gains the bus, `AppState` in Pipeline, the ordered anchor in `scripts/doc-anchors.json` re-seeded), `architecture/dataflow.md` (ingest binder, sink base, taps, cached-flag inputs now bus subscriptions), `architecture/io.md` ("Opening a Link": reader ownership moved to `PipelineHost`), `architecture/project.md`, `architecture/ai.md`, `common-mistakes.md` (`SerialStudio.h` path, Utilities → prompt seam, `QMessageBox` enum mapping), `scripts.md` (bus census, strict layer gate), `.claude/skills/ss-new-driver/SKILL.md` (touch-points: `Core/SerialStudio.h`, `IIngestBinder`), `ss-hotpath` paths. `tests/README.md` (archives linked). `REUSE.toml`. Claim baseline re-seeded only for moved paths. |

## Architecture & data flow

**Bus ownership and injection.** `SessionContext` gains slot 0, the `MessageBus`, constructed as
the first statement of `instantiateCoreModules()` and released last in `shutdown()`. The nine
adopted modules receive `MessageBus&` in their ctors through `SessionContext::create<T>(bus)`;
every other module that publishes or subscribes (Meyers singletons such as the players and
exports, `Translator`, `WidgetExtensions`) receives it through its existing
`setupExternalConnections(...)` call from `ModuleManager`, storing a `MessageBus*` that is
non-null before any publish (all wiring precedes `restoreLastProject()`, the first publish).
Subscriptions are `Core::Bus::Subscription` members; receiver affinity decides the thread.
`MessageBus::instance()` survives P0–P4 with the singleton census counting it and is deleted in
P5 (spec Q1: the count falls uncompensated).

**Vocabulary (final `Messages.h`).** All retained unless marked request. Types are Core-only
after P0/P3, which is what makes `FrameConfig`, `Source`, `Group` legal fields.

| Topic | Owner (publisher) | Readers | Kind |
|---|---|---|---|
| `OperationModeChanged{int mode}` | `AppState` (Pipeline) | Devices, Storage, Api, Ui; `FrameBuilder`/`PipelineHost`/`DeviceIoRouter` caches | retained |
| `FrameConfigChanged{IO::FrameConfig}` | `AppState` | `ConnectionManager` (reset reader), `StreamConfigBuilder`, `ProjectParserCommands` | retained |
| `ProjectLoaded{path, title}`, `ProjectModified{modified}` | `AppState` / `ProjectModel` | Dashboard, `UiDriverSync`, Painter, Sessions | retained |
| `ProjectStructureSnapshot{sources, groups, luaFastMode, frameDetection, generation}` | `ProjectModel` | `StreamConfigBuilder`, `DeviceTableQuery`, `UiDriverSync`, the six project generators | retained |
| `LicenseStateChanged{activated, tier, trialExpired}` | root (`LemonSqueezy` hook) | ten `activatedChanged` receivers; everything else reads `Core::License` | retained |
| `ConnectionStateChanged{sourceId, connected, paused, connecting, busType}` | `ConnectionManager` | `FrameBuilder`, `PipelineHost`, `ProjectModel`, `ControlScript`, players, QuickPlot, Ui | retained |
| `DeviceCatalogChanged{}` | `ConnectionManager` (`contextsRebuilt`, `deviceListRefreshed`, `driverChanged`) | ProjectEditor (Ui) | retained |
| `ReplayPlayerStateChanged{playerId, open}` | each player (Storage) | `FrameBuilder` mask, `ControlScript`, `SerialStudioHelpers` | retained ×3 |
| `AudioCaptureFormat{format, sampleRate, normalized}` | `Drivers::Audio` | `QuickPlotBuilder` | retained |
| `WidgetExtensionCatalog{entries}` | `WidgetExtensions` (Ui) | `DataModel::WidgetResolution` (Pipeline), Api workspaces handler | retained |
| `DashboardStructureChanged{generation, widgetCount}` | `Dashboard` | `ProjectModel`, Mirror | retained |
| `DashboardViewState{json}` | `Dashboard` | `Sessions::Export` | retained |
| `RecordingSessionBoundary`, `SettingsChanged` | (existing; `FrameBuilder` keeps the direct sink call for the boundary, the topic is informational) | — | retained |
| `NotificationRaised{severity, channel, key, title, text}` | any library | `NotificationCenter` (Ui), `MQTT::Publisher` | notification |
| `NotificationClearRequested{channel}` | script API (Pipeline) | `NotificationCenter` | request |
| `LoadGeneratedProjectRequested{json, switchToProjectFile, requestId}` / `GeneratedProjectLoadFinished{requestId, accepted}` | drivers (Devices) / `ProjectModel`+`AppState` | | request / reply |
| `Source0ConnectionSettingsChanged{busType, settings}` | `UiDriverSync` (Devices) | `ProjectModel` | request |
| `DashboardViewStateRestoreRequested{json}`, `DashboardViewStateClearRequested{}` | `Sessions::Player` | `Dashboard` | request |
| `DisconnectRequested{}` | players (Storage) | `ConnectionManager` | request |
| `DeviceOpenAttempted{bus, ok, reason}` | `ConnectionManager` | `ConnectionDiagnostics` (Ui) | notification |
| `ModbusRegisterGroupsLoaded{json}` | `ModbusMapImporter` (Pipeline) | `Drivers::Modbus` | request |

Growth rule written into the `@file` block: aggregate of Core types only; no enum owned above
Core; a field change is a wire break for every reader; a topic is added only through a spec;
a request topic never blocks its publisher and never carries a reply slot.

**Interfaces (owned below both parties; the root binds every one).**

| Interface | Owner | Implemented by | Bound into | Rate |
|---|---|---|---|---|
| `IO::IIngestBinder` | Core | `PipelineHost` | `ConnectionManager`, players (`injectPayload`), `Sessions::Export` (`linkStats`) | command |
| `IO::IRawByteTap` | Core | `API::Server`, `Console::Handler`, `Sessions::Export`, `MQTT::Publisher`, `GRPCServer` | `DeviceIoRouter` | per chunk, GUI thread |
| `IO::IRawFrameTap` | Core | `MQTT::Publisher` | `PipelineHost::routeFrames` | per frame, pipeline thread |
| `DataModel::IBlockSink` | Core | eight sinks | `BlockPublisher::Sinks` | per block, pipeline thread |
| `IO::IDeviceWriter` | Core | `ConnectionManager` | script device APIs, `FrameBuilder` auto-actions | command |
| `Prompt::IUserPrompter` | Core | `Misc::Utilities` (Ui) | `Core::Prompt` free functions | command |
| `DataModel::colorStringValid` hook | Core | root (`QColor`) | `Frame.cpp` | per marker read |
| `Api::ICommandExecutor` | Core | `API::CommandHandler` | script `apiCall`, macros, AI tools | command, synchronous |
| `DataModel::IReplayPlotSink` | Pipeline | `Dashboard` | three players | ~30 Hz scrub |
| `DataModel::IDashboardControl` | Pipeline | `Dashboard` | `DashboardApi`, `DeviceWriteApi` | command |
| `API::ICheckpointStore` | Api | `BackupManager` (Ui) | `CommandRegistry`, two command files | command |

**Ingest after P3.**

```
driver (GUI/driver thread) ── HAL_Driver::dataReceived(CapturedDataPtr) ──┐  Core type
  DeviceIoRouter (Devices, GUI): for tap in taps: tap->onDeviceBytes()      │  per chunk
                                                                           ▼
PipelineHost (Pipeline) owns FrameReader[deviceId]; connect made HERE, queued to pipeline thread
  FrameReader::processData → readyRead (Direct) → routeFrames → FrameBuilder::hotpathRx*
                                                   └─ m_rawFrameTap? ->onRawFrame()  (per frame)
  BlockStager → BlockPublisher::publish: pipeline->publishBlockToDashboard;
      for sink in sinks (IBlockSink*): sink->ingestBlock(detached)         (per block)
StreamWorkerPool (now Pipeline): StreamProcessor::blockReady → FrameBuilder::ingestStreamBlock (queued)
```

Ownership inverts (Pipeline owns readers and workers; Devices asks the binder), the thread of
every hop is unchanged, and no new hop is added.

**Composition root after P5.** `instantiateCoreModules()` order: `MessageBus`, `Translator`,
[licensing], `TimerEvents`, `CommonFonts`, `WorkspaceManager`, `NotificationCenter`, … exactly
as today with the bus first; ctor arguments flow strictly from earlier entries to later ones,
which is why the pinned order needs no reordering. A new `bindInterfaces()` step between
`instantiateCoreModules()` and the `setupExternalConnections` block installs the prompter,
colour validator, block sinks, raw taps, command executor, replay plot sink, dashboard control,
device writer and checkpoint store; the headless and benchmark roots call the subset they need
(`bindBlockSinks({})` for the benchmark). `registerCoreHandlers` then `registerUiHandlers`
replace the lazy `CommandHandler::instance()` side effect.

## Hotpath & threading impact

- **Touches the hotpath? Yes**, in P3 only, and in three places: (1) `BlockPublisher::publish`
  calls `ingestBlock` through `IBlockSink*` instead of eight concrete pointers, one virtual call
  per sink per *block* (≤64 rows per block on the frame lane), on the pipeline thread, no
  allocation, the single `clone_block_trimmed` copy unchanged, `m_anyAsyncSink` still the cached
  gate; (2) `PipelineHost::routeFrames` calls `IRawFrameTap::onRawFrame` per *frame* through a
  pointer hoisted out of the drain loop, replacing today's direct call into
  `MQTT::Publisher::hotpathTxRawFrame` (three relaxed loads + `try_enqueue`); (3)
  `DeviceIoRouter` loops over ≤6 `IRawByteTap*` per *chunk* on the GUI thread, replacing six
  concrete calls. `FrameReader`, `CircularBuffer`, the span lane, `BlockStager` and `Dashboard`
  ingest are untouched. `--benchmark-hotpath` runs after P3 and after P5; the gates are the
  existing nine tiers. `routeFrames` is outside the gated measurement, so the P3 checkpoint also
  records a manual UART-at-rate run with the MQTT publisher enabled and disabled.
- **SPSC / Direct / no-alloc / slot-pool preserved?** Yes: the `dataReceived → processData`
  connect keeps `Qt::AutoConnection` (queued GUI→pipeline at chunk rate) and merely moves from
  `DeviceManager.cpp:218` into `PipelineHost`; `readyRead → routeFrames` stays explicit Direct;
  `blockReady → ingestStreamBlock` stays Queued; reader creation, `moveToThread` and
  `registerFrameReader` happen in the same order on the same thread.
- **New cross-thread signal/slot?** No new signals. Bus subscriptions that replace existing
  connections keep the existing connection types: `FrameBuilder`'s refresh slots stay
  pipeline-affine (auto → queued from GUI publishers, the documented two-thread refresh rule),
  `PipelineHost`'s atomic mirrors and `Dashboard`'s `m_streamAvailable` stay GUI-affine Direct.
  A publish from the pipeline thread (`FrameReader` warning → `NotificationRaised`) is queued to
  the GUI receiver, as the `invokeMethod` it replaces was.
- **New input to a cached hotpath flag?** The *sources* of `m_operationMode`, `m_playerOpen`,
  `m_anyAsyncSink` and the PipelineHost mirrors change from signals to subscriptions; the flags,
  their refresh functions and their threads do not. Each subscription is registered in the same
  `setupExternalConnections` step that held the connection, with `replayLatest = true` so the
  seed read at `FrameBuilder.cpp:635` is unnecessary. `refreshSinkFlag` is driven by
  `IBlockSink::sinkActivityChanged`, connected `Qt::DirectConnection` as today. New cached
  members: `DeviceIoRouter::m_consoleOnly` (from `OperationModeChanged`), `MQTT::Publisher` and
  `Drivers::MQTT` read `Core::License::activated()` (an atomic, never the bus). Dashboard
  `streamAvailable()` reads `Core::Runtime::benchmarkActive()` and `MirrorSession::mirroring()`,
  both plain statics, so the ctor-time call stays construction-free.
- **Timestamp ownership.** Unchanged: drivers stamp `CapturedData` at the boundary;
  `HAL_Driver.h` moves verbatim; no re-stamping is introduced.
- **Bus never on the hotpath.** The `bus-on-hotpath` lint stays; the new bus census fails on
  any bus token in a hotpath TU. `latest<>()` takes a mutex and is used only at command rate.

## Data model & persistence

No project-JSON, `Keys::`, Sessions schema, CSV/MDF4 writer or `widgetSettings` change. The
`SerialStudio` enum ordinals are preserved verbatim (`FFTWindow` and `DashboardWidget` are
persisted). `Frame.h`/`FrameKeys.h` move without edits. `Messages.h` is in-process only.
The Dashboard↔ProjectModel inversion for `points`/`plotTimeRange` writes the same fields the
project already stores.

## API / SDK surface

No command added, renamed or removed; replies unchanged. Registration becomes explicit
(`registerCoreHandlers` + `registerUiHandlers` from the root) and `CommandHandler::instance()`
loses its lazy `initializeHandlers()` side effect, so `Server.cpp:104,614` and
`app/src/Misc/CLI.cpp:523` must run after the root's registration step (they do; the CLI path
is verified in tasks). `EnumLabels` moves to Core with identical slugs. gRPC and the generated
SDK are untouched (`dataset.json` unchanged; regenerated artifacts differ only in include
lines).

## QML / UI

- `SerialStudio.<Enum>` reads (270 sites) keep working through the uncreatable namespace
  meta-object registered under the same QML name; 26 function calls become
  `SerialStudioHelpers.fn(...)`.
- One binding moves: `Cpp_Misc_Translator.welcomeConsoleText` → `Cpp_Console_Handler.welcomeConsoleText`.
- Every `Cpp_*` context-property name and the `qmlRegisterType` names are unchanged; the
  objects behind them keep their addresses (INV-4).
- No component, theme or layout change. `--selftest qml` (the QML instantiation suite) is the
  regression check for the registration change.

## Tradeoffs & alternatives considered

| Decision | Options | Chosen + why |
|----------|---------|--------------|
| Name of the enum vocabulary | (a) `namespace SerialStudio` in Core + rename the helper class; (b) new namespace `SS` + keep the class | (a): C++ spelling `SerialStudio::X` is unchanged at ~1500 sites and 270 QML enum reads; only 26 QML function calls change. |
| Frame value types | (a) move to Core; (b) stay in Pipeline and let Devices include Pipeline | (a): spec Q2 keeps Devices a sibling, and every Devices→Pipeline project-generation, MQTT and stream-config edge is a value-type reach. The blocker 0076 named (`SerialStudio` QObject enums) is gone after P0; the one Gui touch is a 10-line validator hook. |
| Ingest ownership | (a) `IIngestBinder` in Core, `PipelineHost` owns readers/workers; (b) `IStreamSource` and keep `DeviceManager` owning the reader | (a): (b) still needs Devices to name `FrameReader`; (a) is the same hops on the same threads with the connect made one file over, and it also removes `StreamWorkerPool`'s two-ended Pipeline reach. |
| Per-frame MQTT raw tap | (a) `IRawFrameTap*` per frame; (b) a bus topic; (c) keep a concrete `MQTT::Publisher*` | (a): (b) is banned; (c) keeps Pipeline→Storage. One indirect call per frame replaces a direct call into another archive; measured at the P3 checkpoint. |
| MQTT publisher home | (a) `core/Storage/MQTT`; (b) new `core/Sinks` library | (a): a new target is a spec non-goal; Storage already holds the network sink (InfluxDB). Storage's allowed set gains Protocols (CMake already linked it). |
| `showMessageBox` below Ui | (a) Core `IUserPrompter` seam + free function; (b) `NotificationRaised` for all 157 sites | (a): dozens are questions returning a button; (b) would change behaviour. The Ui implementation is reused, including its off-thread marshalling. |
| `NotificationCenter` | (a) move to Ui, posts become `NotificationRaised`; (b) split a Core log + Ui tray/script adapters | (a): every lower-library use is a fire-and-forget post, which is exactly the topic Messages.h already defines; the script API needs only post + clear. |
| `AppState` | (a) into Pipeline + topics; (b) stay in `app/src` + request topics for the 23 setters | (a): its state derives from `ProjectModel`/`FrameBuilder`; Storage/Api/Ui may call it directly, only Devices needs the request form, which folds into the project-generation request it already needs. |
| Licence for hot readers | (a) `Core::License` atomics set by the root + topic for change hooks; (b) `latest<LicenseStateChanged>()` | (a): `latest<>()` takes a mutex and `CommercialToken::isValid()` recomputes an HMAC; the MQTT sites run per message/block. |
| Bus reach for Meyers singletons | (a) `setupExternalConnections(bus)` from the root; (b) `MessageBus::instance()`; (c) convert them all to root-owned | (a): (b) keeps the global, (c) is a second refactor. `instance()` is deleted in P5. |
| Editor block move and the nine-TU class | move as-is; re-form into sub-objects while moving | Re-form (maintainer direction 2026-09-08): the move already rewrites every include line, and landing the block in `Ui` as one class per pair satisfies the one-class-one-target constraint instead of carrying a known violation across libraries. The bodies move verbatim so the diff stays reviewable per sub-object. |
| `SessionContext.h` reach from libraries | (a) per-module `s_instance` set at adopt; (b) an abstract context interface in Core | (a): eight one-line forwarders; (b) would put the root's ownership shape into Core. |
| Strict flip timing | flip each edge as it hits zero; flip all in P5 | P5: the CMake root/link cut is one coherent change and the R2 check needs the final graph; per-phase progress is visible in `layer-verify.py --verbose` counts recorded in tasks.md. |

## Risks & mitigations

- **Ctor-edge proof (specs 0001/0039).** Ctor signatures of the nine modules change (bus, later
  references). Mitigation: the bus is built first and takes nothing; references only flow from
  earlier to later entries; `ProjectModel`'s ctor closure gains no reach; the proof is re-run at
  P0 and P5 (`doc/claude/specs/0001-composition-root/` method), and the `composition-root-order`
  doc anchor is re-seeded.
- **Cached hotpath flags going stale (common-mistakes "Hotpath").** Every replaced connection is
  listed in the P2/P3 tasks with its receiver thread and connection type; `replayLatest = true`
  replaces the seed reads; the P2 checkpoint includes the AC8 mode/connect/pause/replay walk.
- **Order-dependent signals.** `AppState` emits `operationModeChanged` then `frameConfigChanged`
  and `ConnectionManager` relies on both arriving queued in that order; two Dashboard handlers on
  one signal differ in connection type. Mitigation: publish order and per-subscription connection
  types are preserved one-to-one and named in tasks.
- **`showMessageBox` from the pipeline thread** (script engines). The Ui implementation keeps its
  `invokeMethod` marshalling; the seam only changes the reach. Verified by the JS/Lua error
  dialogs in the AC8 walk.
- **`QColor::fromString` validator hook unset in unit tests** accepts any colour string.
  Mitigation: the marker test sets the hook to a hex check; no production path runs without the
  root installing it.
- **Double moc / pair split** on ~120 moved files. `layer-verify.py` rules `pair-split`,
  `moc-double-listed`, `core-unowned` run at every checkpoint.
- **Generated artifacts.** `generate-property-registry.py` runs after P0 and P1; CI's generator
  `--check` steps catch a stale regeneration.
- **Translation contexts.** `SerialStudio` `tr()` → `QCoreApplication::translate("SerialStudio")`
  keeps the 38-message context; `translation_manager.py` walks `core/`; `.ts` files are not
  touched by the agent (Trust Contract).
- **Benchmark and headless roots.** Both call `instantiateCoreModules()`; they now also call
  `bindInterfaces()`'s subset; the benchmark sets `Core::Runtime::setBenchmarkActive`. A missed
  bind publishes through a null sink: `SS_ASSERT` on every bound pointer at bind time.
- **Unity build regrouping** inside archives after moves: the CI unity leg is the gate; fix by
  renaming file-local helpers, never by growing exclusions.
- **Single commit, no bisect.** Per-phase `layer-verify --json`, census outputs and gate results
  are pasted into `tasks.md` at each checkpoint so a regression can be localised by phase.
- **Adjacent work found during planning, all taken in (maintainer direction 2026-09-08):**
  `ProjectEditor` split across nine TUs is re-formed in P1; the `layer-verify.py` table
  forbidding Storage→Protocols while CMake links it is fixed in P3; `generate-legacy-icons.py`'s
  stale output path and the stale `app/tests/CMakeLists.txt` header comment were fixed on
  2026-09-08 ahead of P0, together with the seven pre-existing `claim-verify.py` advisories
  (`claimBlockSlot` attribution in four skill docs, the CMake alias read as a symbol in
  `CLAUDE.md`, two `Source::` field claims in `architecture/scripting.md`).

## Test & verification plan

Per-phase checkpoint (maintainer): the unit-ci configure from `handoff.md` of 0076, then
`cmake --build build/0077 --target ss_unit_tests && ctest --test-dir build/0077`, then the
application build, then the checks listed for that phase. The agent runs every script gate
before handing a phase over.

| AC | Check | Phase |
|---|---|---|
| AC1 | `python scripts/layer-verify.py` → 0 errors, no debt edges; scratch upward include in each partition fails with `layer-upward` | P5 (counts recorded at every phase) |
| AC2 | CI job `build-core-libraries` green; local: `cmake --build build/0077 --target SerialStudioDevices` (each layer); deleting `core/Devices`'s Core root from CMake makes it fail (one-off manual proof at P5) | P5 |
| AC3 | `python scripts/code-verify.py --singleton-census --check` with `per_edge.cross_library_total == 0` and `MessageBus` count 0 | P5 |
| AC4 | `python scripts/code-verify.py --bus-census --check` → every `Messages.h` topic ≥1 publisher and ≥1 subscriber, none in a hotpath TU | P5 |
| AC5 | `serial-studio-pro --headless --benchmark-hotpath --min-fps 256000` on the release binary after P3 and P5; plus the manual MQTT-on/off rate run after P3 | P3, P5 |
| AC6 | `ctest` counts ≥ 152 unit / 11 fuzz at every checkpoint; recompile-line count in `app/tests/CMakeLists.txt` at P5 matches the residual list | every phase, P5 |
| AC7 | `pytest tests/integration tests/security tests/performance -m "not destructive"` against the running P5 build; fixture projects round-trip byte-identical (`tests/integration/test_project_*`), recordings compared with the spec-0044 verifier (`--verify-session`), API replies via `tests/utils/api_client.py` diffed against a pre-program capture the maintainer takes before P0 | P5 (integration subset after P2 and P4 too) |
| AC8 | Maintainer walk (checklist in tasks.md): mode switch ×3, licence activate/deactivate (offline file), language change incl. RTL, connect/pause/disconnect UART and MQTT, project load/save, CSV+Historian replay with seek, generate-project from a Modbus/S7 device, Modbus map import, assistant checkpoint/restore, `--selftest`, `--benchmark-hotpath` | P2, P4, P5 |
| AC9 | `claim-verify.py`, `registry-verify.py`, `documentation-verify.py`, `reuse lint`, generator `--check`s clean | every phase |
| AC10 | One commit after P5; tasks.md carries the per-phase gate log | P5 |

- **Unit (agent can run):** `pytest tests/scripts/` (JS parser cases, path literals repointed
  in `test_cpp_regressions.py`), `pytest scripts/tests/` (`test_layer_verify.py` gains the
  `cmake-root-violation` fixture, `test_code_verify.py` the bus-census fixture).
- **C++ unit tier (maintainer):** new suites `tst_license_state`, `tst_user_prompt_seam`,
  `tst_ingest_binder` (FakeDriver → PipelineHost attach/detach/inject), `tst_block_sink`
  (BlockPublisher over a counting `IBlockSink`), `tst_serialstudio_enums` (ordinal pins for
  `FFTWindow`, `DashboardWidget`); `tst_message_bus` extended for the new topics' aggregate
  shape; every existing suite kept.
- **Static:** `code-verify.py --check` on every touched file per task; `qt-cpp-review` on the
  P3 and P5 diffs; `sanitize-commit.py` before the single commit.
