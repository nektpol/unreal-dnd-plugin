# JSON spell-file loader

FDnd5eJsonSpellCatalogLoader::LoadCatalog reads an explicit selection of external JSON files and delegates conversion/installation to the existing [catalog builder](spell-catalog.md#catalog-builder). UDnd5eJsonSpellCatalogLibrary::LoadSpellCatalog exposes the same operation to Blueprint using a retained catalog object. Both live in Dnd5eJson.

The loader opens caller-selected filesystem paths. It downloads nothing, performs no automatic dataset discovery, and stores no remembered paths or generated assets. The existing supplied-text ReadSpell, ReadAvailability, BuildCatalog, and BuildSpellCatalog operations continue to perform no file I/O. Core Dnd5e remains independent of JSON and file loading.

## Contents

- [Request and result](#request-and-result)
- [Calling from C++](#calling-from-c)
- [Calling from Blueprint](#calling-from-blueprint)
- [Selection and provenance](#selection-and-provenance)
- [Read limits and encoding](#read-limits-and-encoding)
- [Failure and diagnostics](#failure-and-diagnostics)
- [Verification and remaining work](#verification-and-remaining-work)

## Request and result

The exported Blueprint-visible request and native loader are declared in [Dnd5eJsonSpellCatalogLoader.h](../Source/Dnd5eJson/Public/Readers/Spells/Dnd5eJsonSpellCatalogLoader.h). FDnd5eJsonSpellCatalogLoadRequest has the following EditAnywhere/BlueprintReadWrite fields in Dnd5e|Json|Catalog:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| DatasetRoot | `FString`, empty | Explicit absolute filesystem directory containing this selection |
| DatasetNamespace | `FString`, empty | Required logical dataset identity, preserved exactly |
| SpellFiles | `TArray<FString>`, empty | Ordered portable filenames relative to DatasetRoot |
| bHasAvailabilityLookup | `bool = false` | Whether AvailabilityFile is active |
| AvailabilityFile | `FString`, empty | Relative lookup filename when active |
| SpellFluffFiles | `TArray<FString>`, empty | Explicit supplementary filenames under the same root |
| EditionPolicyId | `FString`, empty | Explicit caller classification policy |
| EditionAssignments | `TArray<FDnd5eSourceEditionAssignment>`, empty | Exact namespace/source edition assignments |
| bValidateContent | `bool`, false | Opt-in candidate Content validation |

Both operations reuse [FDnd5eJsonSpellCatalogBuildResult](spell-catalog.md#catalog-builder): Status, SpellCount, Diagnostics, UnmatchedSupplements, and ContentValidationIssues. NotAttempted remains its default authoring value; the operation returns Success or Failure. Success can include warnings or Info diagnostics. Failure returns SpellCount=0 and at least one Error while preserving the previously installed catalog, if any. The result count is not the size of a preserved old catalog after failure.

## Calling from C++

```cpp
#include "Readers/Spells/Dnd5eJsonSpellCatalogLoader.h"

FDnd5eJsonSpellCatalogLoadRequest Request;
Request.DatasetRoot = CallerSelectedAbsoluteDirectory;
Request.DatasetNamespace = TEXT("my-collection");
Request.SpellFiles = {TEXT("spells/local.json")};

FDnd5eSpellCatalog Catalog;
const auto Result = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
```

The caller supplies the directory and files. For a lookup, explicitly enable bHasAvailabilityLookup and set AvailabilityFile to its relative filename under the same root. The loader assigns the one supplied namespace to every generated document context. It does not infer a namespace, source publication, SRD membership, or rules edition from directory names.

## Calling from Blueprint

Use the existing [catalog object ownership](spell-catalog.md#blueprint-ownership-and-operations) pattern: construct UDnd5eSpellCatalogObject and retain it in a strong object-reference variable. Make a load request, connect a String array to SpellFiles, and call Load Spell Catalog from the [JSON library](../Source/Dnd5eJson/Public/Readers/Spells/Dnd5eJsonSpellCatalogLibrary.h).

This is a static BlueprintCallable execution node, not a pure node. It checks CatalogObject with IsValid before reading anything. A null/invalid target returns the same Dnd5e.Json.InvalidCatalogObject result as the supplied-text build wrapper. Otherwise it delegates to the native loader without modifying the request.

Switch on the returned Status. On Success, query the same retained object and retrieve copied spell structs through TryGetSpell. For all IDs, leave SourceId empty, Level=-1, and the other filters at defaults. SourceId is a publication selector; it does not accept a sentinel for all publications.

The new route replaces application-side document preparation when input is a file selection. Supplied-text Build Spell Catalog remains available when the application already has text. The loader does not add a Content Browser import asset or an editor file dialog.

## Selection and provenance

At least one spell file is required, with at most 1,024 primary/supplementary/active-lookup files combined. A selected file containing `{"spell":[]}` deliberately installs an empty catalog; an empty selection is an error. Every selected file contributes its entire exact spell array under the existing [accepted source/preservation policy](json-spell-reader.md#accepted-spell-subset). Files are not silently skipped.

The complete active selection is validated before any selected file is opened. DatasetNamespace follows the existing context rules. DatasetRoot must be nonempty, absolute, free of edge whitespace/control/wildcard/reserved characters or a URL scheme, and resolve to an existing directory. Filesystem separators are normalized on a copied root. Windows requires a drive-qualified or UNC root and rejects drive-relative/current-drive-relative forms. Other platforms use the engine's ordinary absolute-path handling.

Selected filenames are nonempty portable relative labels using forward slashes, with no absolute roots, backslashes, colons, controls/invalid Unicode, empty/current/parent segments, wildcards, or reserved characters `* ? < > | "`. The extension must be json, compared case-insensitively. Resolved paths must remain lexically under the normalized root. Duplicate resolved selections use FPaths::IsSamePath, including Microsoft case-insensitive semantics. An active lookup cannot reuse a selected spell path. Inactive AvailabilityFile text is ignored.

These checks use engine/platform lexical path comparison. They do not establish physical-file identity for every alias/junction/symlink or an untrusted-input filesystem sandbox. The caller chooses filesystem paths and access context.

Spell-file order is retained. Each loaded document's RelativeFilePath is the original valid selected label, and its DatasetNamespace is the explicit supplied namespace. Record JSON Pointers remain local to their source file. The absolute root is used for opening files, not copied into record IDs/provenance or saved by this API. The optional lookup receives the same namespace and its own selected relative label.

## Read limits and encoding

The loader allows **16,777,216 bytes per selected file** and **67,108,864 bytes total per load**, including an active lookup. These are additional byte caps, not a reinterpretation of the existing parser's TCHAR/depth/value/record limits. A document accepted by the text API could exceed the loader's conservative byte limit. Limits are inclusive and reset for each load; over-limit input fails without truncation.

Each file is opened through an owned archive. Its reported size is checked before allocating a byte buffer. The loader reads that checked byte count, checks archive error/position, and then uses FFileHelper::BufferToString for engine-supported text/BOM decoding. UTF-8 with/without BOM and UTF-16 BOM fixtures are tested on the current host. This does not introduce a separate strict encoding validator. An empty file reaches the existing builder as empty text and fails with EmptyInput.

All documents are prepared before one builder call. No file installs records independently. The builder validates a complete candidate and replaces only on success. Reads/builds are synchronous; use the current UObject integration on the game thread. This API supplies no asynchronous job, progress/cancellation service, cross-thread synchronization, or guarantee that externally edited files form a simultaneous filesystem snapshot.

## Failure and diagnostics

Loader-specific fixed Error codes are:

| Code | Meaning |
| --- | --- |
| Dnd5e.Json.Load.InvalidRequest | No spell files selected |
| Dnd5e.Json.Load.InvalidDatasetRoot | Invalid, relative, unresolved, or nonexistent dataset directory |
| Dnd5e.Json.Load.InvalidFilePath | Empty/unsupported filename or resolution outside the root |
| Dnd5e.Json.Load.DuplicateFile | Repeated active resolved selection |
| Dnd5e.Json.Load.FileReadFailed | Open, size, or complete-read failure |

Existing InvalidContext handles namespace/portable-label errors, ResourceLimitExceeded handles count/byte limits, and InvalidCatalogObject handles Blueprint targets. The builder's JSON/conversion/index errors and preservation/reference warnings pass through unchanged. For example, an array-shaped lookup root follows its existing RequireObject InvalidFieldValue path, while a spell-document array root uses InvalidRoot.

Filesystem errors carry the selected relative filename when one is available and an empty JsonPointer. Root/request errors have no invented source locator. Builder errors carry their existing source pointer. The loader returns these values without application logging or saved reports; the caller decides presentation.

Failure at validation, a later read, lookup conversion, normalization, duplicate record IDs, or a limit preserves the target's previous records. Querying that retained target remains valid. Explicitly loading a valid empty spell array is the supported clear operation.

## Verification and remaining work

See [Automated tests](testing.md) for current evidence, synthetic temporary-file cases, reflected invocation, and opt-in external-directory checks. Source files, local paths, and generated real-data reports stay outside the plugin repository.

The separate [editor integration](editor-dataset-loading.md) supplies file pickers, remembered per-user selection, explicit Tools loading with a Message Log, and searchable read-only spell browsing. The native loader remains independent of that UI/configuration. Rich Content rendering, asynchronous loading, provider/reference resolution, and complete typed rule semantics remain later work. This operation loads reference data and does not apply character or gameplay changes.

## Supplementary and policy inputs

SpellFluffFiles are explicitly selected, not discovered. All selected primary/supplementary/active-lookup filenames are validated and deduplicated together before reads. They share the 1024-file count and one 64 MiB byte accumulator (16 MiB per file). The loader forwards edition and validation options to the same [builder](spell-data-inputs.md); the native/Blueprint result includes unmatched successful rows and dedicated model issues. Editor preferences currently expose primary/availability fields only; these additions do not silently select supplementary files or enable validation.
