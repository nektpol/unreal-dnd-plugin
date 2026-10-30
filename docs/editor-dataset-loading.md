# Editor dataset loading

Dnd5eEditor adds a local dataset selection page, two Tools commands, an editor-session catalog, and Message Log results. It uses the existing [file loader](json-spell-file-loader.md) without changing the runtime parser or catalog contract. No test Actor or Play In Editor session is needed.

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

## Implementation boundary

The settings and subsystem headers are private editor implementation in [Settings](../Source/Dnd5eEditor/Private/Settings/Dnd5eEditorDatasetSettings.h) and [Subsystems](../Source/Dnd5eEditor/Private/Subsystems/Dnd5eEditorDatasetSubsystem.h). Module menu/result code is in [Dnd5eEditor.cpp](../Source/Dnd5eEditor/Private/Dnd5eEditor.cpp). These classes are not a new public runtime API.

Dnd5eEditor directly declares DeveloperSettings, EditorSubsystem, Settings, ToolMenus, MessageLog, Slate, and SlateCore in addition to its existing dependencies. The runtime modules acquire none of them. UDeveloperSettings supplies automatic settings registration; ToolMenus uses deferred startup registration and owner-based cleanup. The module unregisters its callback, menu owner, and Message Log listing during shutdown without loading UI modules during teardown.

The current interface selects and loads data and presents diagnostics. A dedicated searchable record list and read-only details panel remain the next editor feature. Asynchronous loading, provider/reference resolution, and complete rule semantics are separate work.

## Verification

The user verified editor loading of 319 source-marked SRD records and selection persistence after restart. [Automated tests](testing.md) cover editor schemas, path conversion, isolated config save/reload, transient ownership, failed-load preservation, deinitialization, settings/menu registration, and Message Log registration. Config tests check that the actual user-config file and settings default object remain unchanged.

Automated coverage does not simulate native file dialogs or unload its own executing test module. Interactive picker/persistence behavior is supported by the manual check; shutdown cleanup is reviewed and menu-entry ownership is verified. The runtime source-mapping limits remain those documented by the reader and loader.
