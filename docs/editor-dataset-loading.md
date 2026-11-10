# Editor dataset loading

Dnd5eEditor adds a local dataset selection page, three Tools commands, an editor-session catalog, Message Log results, and a searchable spell browser. It uses the existing [file loader](json-spell-file-loader.md) without changing the runtime parser or catalog contract. No test Actor or Play In Editor session is needed.

## Select files

Open **Tools → Dnd5e Dataset Settings**, or **Editor Preferences → General → Dnd5e Dataset**.

| Setting | Editor storage | Meaning |
| --- | --- | --- |
| Dataset Root | `FDirectoryPath`, initially empty | Browse to the external directory containing the selected files |
| Dataset Namespace | `FString`, initially empty | Explicit logical collection identity |
| Spell Files | `TArray<FFilePath>`, initially empty | Add one entry per selected spell JSON file and use its picker |
| Has Availability Lookup | `bool = false` initially | Enable the optional companion lookup |
| Availability File | `FFilePath`, initially empty | Choose the lookup under the same dataset root |

Previously saved local values are restored when settings load, so configured values can differ from these initial defaults. Choosing a root does not select any spell files: the array needs explicit entries. Files are filesystem JSON, not Content Browser assets. There is no automatic directory scan, source/edition preference, download, or source-content copy into the plugin.

File pickers can return absolute paths. The editor converts a fresh copy into the loader's portable relative filenames, preserving order and namespace. Already-relative inputs are retained after filesystem-separator normalization. The saved picker values remain unchanged. Files outside the root, on an incompatible drive, or otherwise invalid remain rejectable by the loader; the editor does not silently select another root or drop them. An inactive availability filename does not participate in loading.

Settings changes update the remembered selection. They do not replace the loaded catalog. A changed root may make earlier file choices invalid; choose compatible files and explicitly load again.

## Local persistence

The settings class uses Config=EditorPerProjectUserSettings with five Config properties and no DefaultConfig flag. Ordinary settings persistence and the explicit load command's SaveConfig write the host's generated per-project user configuration, normally Saved/Config/<editor-platform>/EditorPerProjectUserSettings.ini. No shared default config filename is supplied by this workflow.

Remembered paths belong to that local configuration, outside the plugin repository. No personal paths are supplied as source defaults. Catalog records, original JSON, and diagnostics are not settings properties and are not saved there. Explicit manual settings export/shared-default actions are outside this workflow.

## Load and read the result

Choose **Tools → Load Dnd5e Dataset**. The action saves the current selection, converts it to a load request, and synchronously invokes the existing loader. The **Dnd5e Dataset** Message Log opens with the attempt summary and all returned diagnostics.

- Success reports the installed spell count. Warnings and Info diagnostics can accompany a successful load.
- Failure reports that the active editor catalog still contains its previous records. A first failed load leaves it empty.
- Diagnostic rows retain severity, controlled code, message, relative file label, and JSON Pointer where available.

The result listing suppresses mirroring its presentation to the general Output Log. It does not print whole spell records or source documents. Runtime calls still return diagnostics quietly; the editor displays them because the user explicitly chose the load command.

For example, loading 319 records with 823 diagnostics means one successful collection and 823 messages across those records. A record can produce multiple messages. PreservedUnsupportedContent means a construct lacks a typed mapping and its source fragment is retained. UnresolvedReference means reference/relationship storage is retained while target normalization/resolution is incomplete. Neither warning means the whole record was converted to plain text. Typed dice/damage annotations and the supported spell/Content values remain structured.

## Session ownership

An editor subsystem owns one transient UDnd5eSpellCatalogObject. It creates the object on the first explicit load and keeps the same object for subsequent attempts. The loader's candidate/atomic replacement preserves prior records on failure. The complete last result is retained separately from the currently installed record count.

Restarting Unreal restores the file selection, not the loaded records. The editor session begins unloaded until the user chooses Load. Shutdown releases the transient catalog and last-result state. Changing settings performs no dataset reads.

This editor catalog is independent of Actor/GameInstance-owned Blueprint catalogs. It does not automatically populate a playable project or package external content. Runtime consumers still choose their own catalog ownership and loading configuration.

## Browse loaded spells

Open **Tools → Dnd5e Spell Browser**. The dockable tab reads the existing editor catalog; opening it does not select files or load a dataset. Before an explicit load, it shows an empty list and a loading instruction. The summary reports visible rows out of total loaded records.

The left search box matches case-insensitive substrings of display names, ignoring leading/trailing search whitespace. Empty search shows all records in catalog order. Source labels and other fields are not included in this search. Runtime catalog exact-name queries retain their existing contract.

Each row shows display name, level, and primary publication/source ID. Level 0 represents a cantrip. Missing provenance displays Source unspecified; no source or edition is inferred. Records with matching names remain distinct rows selected by their exact record IDs.

Select a row to inspect its expandable Spell value on the right, including Header, tags, shared values, and structured Content payloads. This is a read-only reflected inspector, not a formatted spell sheet. Its property search is separate from the name search. It presents retained fallback fields as stored; inspecting a record adds no rule interpretation and does not execute expressions or navigate references/media.

A successful dataset load refreshes the list and clears selection/details while retaining the search text. A failed load sends no catalog-change notification, preserving the prior catalog and current view. Changing the name search also clears selection/details. Closing and reopening the tab shows the current catalog with a fresh search and selection; restarting Unreal still begins with an unloaded catalog even if its tab layout is restored.

The details view displays an independent copy inside a transient editor-only UObject. The browser retains that object through a strong UObject pointer, disables property editing, and releases it when the selection clears or the view closes. This does not create a Content Browser asset, save converted records, or modify the catalog. Repeated Tools invocations focus the existing tab.

## Implementation boundary

The settings and subsystem headers are private editor implementation in [Settings](../Source/Dnd5eEditor/Private/Settings/Dnd5eEditorDatasetSettings.h) and [Subsystems](../Source/Dnd5eEditor/Private/Subsystems/Dnd5eEditorDatasetSubsystem.h). Module menu/result code is in [Dnd5eEditor.cpp](../Source/Dnd5eEditor/Private/Dnd5eEditor.cpp). These classes are not a new public runtime API.

Dnd5eEditor directly declares DeveloperSettings, EditorSubsystem, Settings, ToolMenus, MessageLog, Slate, SlateCore, PropertyEditor, and InputCore in addition to its existing dependencies. The runtime modules acquire none of them. UDeveloperSettings supplies automatic settings registration; ToolMenus uses deferred startup registration and owner-based cleanup. Shutdown releases the browser tab content, closes its tab, and unregisters its factory, menu owner/callback, and Message Log listing without loading UI modules during teardown.

The list model, Slate widget, and transient selection class are private [browser implementation](../Source/Dnd5eEditor/Private/Browser/SDnd5eSpellBrowser.h), not new public runtime models. The object details view inspects UDnd5eSpellBrowserSelection.Spell and its detached ReferenceReport; the original direct struct-details approach reproduced a nested-Content crash on UE 5.8.4 and is no longer used. The precise engine-side lifetime defect was not established. The current object-view path is covered by repeated updates and garbage-collection tests. Rich Content rendering, asynchronous loading, provider/reference resolution, and complete rule semantics remain separate work.

## Reference report inspection

Selecting a spell computes a detached [reference report](spell-reference-report.md) against the same editor-owned catalog. The private selection object retains both Spell and ReferenceReport as transient reflected values, and the existing object inspector displays both read-only. Expand the Reference Report category/Entries to inspect model paths, original and normalized references, stage statuses, and stage diagnostics.

The summary shows typed occurrence count, original targets found, selectors normalized, normalized targets found, and collection-level diagnostic count. These are independent counters; both lookup routes can resolve different targets. Per-entry diagnostics remain under their respective stages. A zero-reference result is distinct from no selection or a collection failure, and opaque-fallback diagnostics mean typed counts do not include every potential reference in preserved content.

Reports and summary counts are computed once per row selection. The text attribute returns cached text; it does not scan the catalog or rebuild a report each frame. Search changes and catalog-refresh notifications clear both selected data and summary, requiring reselection for a new snapshot. Reports do not trigger navigation, catalog rewriting, source-default selection, or rule execution.

## Verification

The user verified editor loading of 319 source-marked SRD records, selection persistence after restart, and successful spell selection after the details-view correction. The current 858-case standard / 891-case combined [automation run](testing.md) retains coverage of browser metadata/exact identity, name searching, refresh notifications, the production widget's selection/search/details flow, copy isolation, garbage collection, and menu/tab registration. Report-specific tests additionally drive populated read-only inspection, cached summary text, zero/failure states, private property flags/defaults, GC/copy isolation, and search/refresh clearing. Existing config tests check that the actual user-config file and settings default object remain unchanged. The new runtime reference lookup does not add automatic browser navigation.

Automated coverage drives the real Slate controls and details updates without simulating screen clicks, docking gestures, or native file dialogs; it does not unload its own executing test module. Interactive picker/persistence and corrected spell selection have manual evidence. Docking appearance, scrolling/expansion presentation, and teardown during module unload are not claimed as fully simulated UI coverage. The runtime source-mapping limits remain those documented by the reader and loader.

## Additional runtime request options

The runtime native/Blueprint [supplementary, edition-policy, and validation request fields](spell-data-inputs.md) are implemented. This editor settings page still exposes the five primary/availability selection fields above. It does not automatically supply supplementary files, classify editions, or enable Content validation.
