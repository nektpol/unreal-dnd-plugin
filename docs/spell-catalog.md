# Spell availability and catalog

The plugin can build an in-memory spell catalog from supplied JSON documents, optionally join a supplied spell-availability lookup, and answer read-only queries. The caller supplies all text and chooses when to replace the catalog. These APIs perform no filesystem or network access, select no default publication/edition, and apply no character or gameplay changes.

The core [FDnd5eSpellCatalog](../Source/Dnd5e/Public/Catalog/Dnd5eSpellCatalog.h) and its [Blueprint object wrapper](../Source/Dnd5e/Public/Catalog/Dnd5eSpellCatalogObject.h) live in Dnd5e. The [availability reader](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellAvailabilityReader.h), [native builder](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellCatalogBuilder.h), and [Blueprint build library](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonSpellCatalogLibrary.h) live in Dnd5eJson. The single-record readers remain native C++ APIs; catalog construction, replacement, and queries are also callable from Blueprint. Editor dataset-selection workflows remain pending.

## Contents

- [Build and query](#build-and-query)
- [Blueprint ownership and operations](#blueprint-ownership-and-operations)
- [JSON document value](#json-document-value)
- [Availability reader](#availability-reader)
- [Catalog builder](#catalog-builder)
- [Core catalog](#core-catalog)
- [Spell query](#spell-query)
- [Availability source mapping](#availability-source-mapping)
- [Limits and diagnostics](#limits-and-diagnostics)
- [Verification and remaining work](#verification-and-remaining-work)

## Build and query

```cpp
#include "Readers/Dnd5eJsonSpellCatalogBuilder.h"

FDnd5eJsonSpellCatalogBuildRequest Request;
FDnd5eJsonDocument Document;
Document.JsonText = SuppliedSpellJson;
Document.Context.DatasetNamespace = TEXT("my-collection");
Document.Context.RelativeFilePath = TEXT("spells/local.json");
Request.SpellDocuments.Add(Document);

// Set these only when the caller supplies an availability lookup.
Request.bHasAvailabilityLookup = true;
Request.AvailabilityLookup.JsonText = SuppliedLookupJson;
Request.AvailabilityLookup.Context.DatasetNamespace = TEXT("my-collection");
Request.AvailabilityLookup.Context.RelativeFilePath = TEXT("generated/availability.json");

FDnd5eSpellCatalog Catalog;
const auto Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
if (Result.Status == EDnd5eJsonReadStatus::Success)
{
    FDnd5eSpellQuery Query;
    Query.Level = 1;
    TArray<FDnd5eRecordId> Ids;
    TArray<FDnd5eDiagnostic> Diagnostics;
    if (Catalog.QuerySpellIds(Query, Ids, Diagnostics))
    {
        for (const auto& Id : Ids)
        {
            FDnd5eSpell Spell;
            Catalog.TryGetSpell(Id, Spell);
            // Use this independent copy in the consuming application.
        }
    }
}
```

The application handles both build and query diagnostics. A successful operation can return warnings. Omitting the lookup leaves availability unknown; it does not prevent ordinary name/level/school queries.

For explicitly selected filenames rather than supplied text, use the separate [JSON spell-file loader](json-spell-file-loader.md). Its LoadSpellCatalog Blueprint node reads bounded external files and delegates to this builder. The text operations described here still open no files.

## Blueprint ownership and operations

UDnd5eSpellCatalogObject is an exported UCLASS(BlueprintType, Transient) derived from UObject. It owns one private native FDnd5eSpellCatalog and introduces no duplicate reflected spell storage. Each constructed object has its own initially empty catalog. The consumer chooses the lifetime and scope; there is no automatically shared singleton or default dataset.

Construct the native class with Construct Object from Class and retain the result in a Blueprint object-reference variable on an appropriate owner. Outer supplies context/containment; it does not replace a strong owner reference. Native owners normally retain a UPROPERTY TObjectPtr<UDnd5eSpellCatalogObject>. An Actor can own a test catalog; a consumer needing a collection across map changes can choose its GameInstance or another suitable owner.

| Reflected function | Exposure | Behavior |
| --- | --- | --- |
| ReplaceSpells(Records, Diagnostics) | BlueprintCallable, instance | Core validated replacement; returns bool and diagnostics, preserves previous records on failure |
| TryGetSpell(Id, Spell) const | BlueprintCallable, explicitly not pure | Exact lookup, independent copied spell output; resets output on miss |
| QuerySpellIds(Query, Ids, Diagnostics) const | BlueprintCallable, explicitly not pure | Query validity bool, ordered `TArray<FDnd5eRecordId>` output, diagnostics |
| GetSpellCount() const | BlueprintPure | Current int32 count |
| GetSpellIds() const | BlueprintCallable, explicitly not pure | Independent `TArray<FDnd5eRecordId>` containing all installed IDs |

Querying and copying are explicit execution nodes even though they are const. This avoids repeated scans or copying large Content trees through pure-node reevaluation. The cheap count getter is pure. Const struct/array reference parameters are inputs; non-const reference parameters are output pins. GetMutableCatalog is an unreflected C++ integration method used by the JSON wrapper, not a Blueprint node.

UDnd5eJsonSpellCatalogLibrary::BuildSpellCatalog(CatalogObject, Request) is a static BlueprintCallable function returning the existing FDnd5eJsonSpellCatalogBuildResult. It uses the target's native catalog and forwards the builder result unchanged. A null or invalid object returns Failure, SpellCount=0, and one Error with code Dnd5e.Json.InvalidCatalogObject and empty source location. The library owns no catalog and never clears a valid target before building.

For supplied text, the Blueprint flow is:

1. Construct and retain a catalog object.
2. Obtain document text through the consuming application's explicit loading flow. JsonText holds the content, not a file path.
3. Make a document with namespace/relative label, put it in a `TArray<FDnd5eJsonDocument>`, and make the build request.
4. Call BuildSpellCatalog and inspect Status, count, and diagnostics. Continue dependent work on Success.
5. Query the same retained object, then pass a returned ID to TryGetSpell and inspect the copied struct.

For an unfiltered query leave strings/containers empty and set Level=-1. SourceId is a publication selector, so putting `-1` there searches for that literal publication and produces no matches. A valid empty query result is distinct from query failure. Without a supplied availability lookup, ordinary queries still work; availability constraints can return an incomplete-results warning.

Use these object operations synchronously on the game thread for the current API. The object's lifetime follows Unreal garbage collection; catalog replacement guarantees failure preservation, not thread synchronization. This is transient in-memory data, not a saved asset, replicated catalog, file loader, or asynchronous service. Dnd5eJson publicly declares Engine for its public UBlueprintFunctionLibrary base; the core still has no JSON dependency.

## JSON document value

[FDnd5eJsonDocument](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonDocument.h) groups supplied text with its source context. Both fields use default construction:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| JsonText | `FString`, empty | Complete caller-supplied document text |
| Context | `FDnd5eJsonReadContext`, default-constructed | DatasetNamespace and optional portable RelativeFilePath label |

Context uses the same [validation policy](json-spell-reader.md#calling-the-reader) as ReadSpell. A path is a diagnostic label, never an instruction to open a file. [EDnd5eJsonReadStatus](../Source/Dnd5eJson/Public/Readers/Dnd5eJsonReadStatus.h) is shared by all JSON results: NotAttempted, Success, Failure. Its enum identities/order remain unchanged after moving its declaration into a common header.

## Availability reader

`FDnd5eJsonSpellAvailabilityReader::ReadAvailability` reads one selected spell's associations from supplied lookup text. It does not modify a spell or catalog.

FDnd5eJsonSpellAvailabilityReadRequest:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Document | `FDnd5eJsonDocument`, default-constructed | Lookup text and context |
| SpellName | `FString`, empty | Explicit spell-name selector |
| SpellSourceId | `FString`, empty | Explicit spell-publication selector |

FDnd5eJsonSpellAvailabilityReadResult:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Status | `EDnd5eJsonReadStatus = EDnd5eJsonReadStatus::NotAttempted` | Producer outcome |
| bHasEntry | `bool = false` | The selected lookup entry exists and was converted |
| Availability | `TArray<FDnd5eSpellAvailability>`, empty | Converted source associations |
| Diagnostics | `TArray<FDnd5eDiagnostic>`, empty | Returned information, warnings, and errors |

A missing source/name entry returns Success, bHasEntry=false, an empty array, and an Info diagnostic. An explicit empty entry returns Success, bHasEntry=true, and an empty array. Malformed selected data returns Failure with false/empty output even if earlier associations converted. All calls leave the request unchanged. Unselected entries are syntax/preflight checked but not semantically converted.

The canonical provider/association fields and their presence semantics are in [Spell model](spell-model.md#spell-availability).

## Catalog builder

`FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog)` converts all entries of every supplied document's exact `spell` array, optionally joins availability, and installs the result only after successful conversion and core catalog checks. It reuses the [spell reader's accepted subset](json-spell-reader.md#accepted-spell-subset), identity construction, Content conversion, and preservation policy. Each input document and the optional lookup are parsed once per build.

FDnd5eJsonSpellCatalogBuildRequest:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| SpellDocuments | `TArray<FDnd5eJsonDocument>`, empty | Ordered documents whose spell arrays are converted completely |
| bHasAvailabilityLookup | `bool = false` | Whether AvailabilityLookup is supplied for this build |
| AvailabilityLookup | `FDnd5eJsonDocument`, default-constructed | Optional generated source-association lookup |

FDnd5eJsonSpellCatalogBuildResult:

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| Status | `EDnd5eJsonReadStatus = EDnd5eJsonReadStatus::NotAttempted` | Producer outcome |
| SpellCount | `int32 = 0` | Installed record count on Success; zero on Failure |
| Diagnostics | `TArray<FDnd5eDiagnostic>`, empty | Accumulated conversion/build diagnostics |

An empty request fails. To deliberately clear a catalog through the builder, supply a valid document containing `{"spell":[]}`. If bHasAvailabilityLookup=false, its inactive value is ignored. When true, the lookup and every spell document must have exactly the same DatasetNamespace; the builder does not apply a lookup to another collection. Without a lookup, documents can carry different explicit namespaces.

Duplicate exact IDs, malformed selected records, invalid selected lookup data, and limits fail the entire build. The existing catalog remains usable; SpellCount=0 does not describe that preserved catalog's size. Warnings/Info alone do not prevent installation. A missing lookup entry leaves the corresponding spell's bHasAvailabilityData=false. The builder does not append into an existing catalog or resolve reference targets as a side effect.

## Core catalog

FDnd5eSpellCatalog owns copies of normalized spell records, an exact-ID index, and their supplied order. It has no JSON dependency, saved-file format, global singleton, automatic loader, or startup service.

| Method | Behavior |
| --- | --- |
| ReplaceSpells(Records, Diagnostics) | Reset diagnostics, validate a candidate, then replace atomically on success; an empty array clears the catalog |
| TryGetSpell(Id, Spell) const | Exact-ID lookup; copy out on success, reset Spell to its default on failure |
| QuerySpellIds(Query, Ids, Diagnostics) const | Reset both outputs, validate the query, return matching IDs in installation order |
| GetSpellCount() const | Current record count |
| GetSpellIds() const | Independent copy of all IDs in installation order |

ReplaceSpells requires a complete Spell-category ID, nonblank/control-free namespace/source key/display name, level 0–9, unique exact identities, and consistent availability presence/provider qualification. Subclass links also need parent-class name/source and short name. These are catalog indexing checks; they do not recursively validate all Content or certify complete rule/reference semantics. Default authoring spells with Level=-1 cannot be installed as complete records.

[ID equality and hashing](shared-models.md#record-id) preserve exact namespace/source-key case, including Unreal reflected equality. Name search is a separate operation. The caller owns synchronization for a catalog shared across threads; atomic replacement here describes failure behavior and does not provide concurrent-reader synchronization.

## Spell query

FDnd5eSpellQuery fields are declared in [Dnd5eSpellCatalog.h](../Source/Dnd5e/Public/Catalog/Dnd5eSpellCatalog.h). All constraints combine with AND; empty strings/containers are wildcards.

| Field | C++ type and default | Meaning |
| --- | --- | --- |
| DatasetNamespace | `FString`, empty | Exact case-sensitive namespace |
| SourceId | `FString`, empty | Exact case-sensitive publication in Header.Provenance[0] |
| Name | `FString`, empty | Whole display name, case-insensitive; optional aliases also checked |
| Level | `int32 = INDEX_NONE` | -1 means no level constraint; 0–9 select a supplied spell level |
| Schools | `FGameplayTagContainer`, empty | Require every supplied school tag using hierarchical HasAll |
| Traits | `FGameplayTagContainer`, empty | Require every supplied trait tag using hierarchical HasAll |
| bIncludeAliases | `bool = true` | Allow Name to match Header.Aliases |
| bRequireAvailability | `bool = false` | Require at least one matching supplied association |
| AvailabilityProvider | `FDnd5eSpellAvailabilityProvider`, default-constructed | Provider constraints; invalid category/empty fields are wildcards |
| AvailabilitySubgroup | `FString`, empty | Whole subgroup name, case-insensitive |
| AvailabilityDefinedInSource | `FString`, empty | Exact member of the association's DefinedInSources |
| bIncludeVariantAvailability | `bool = true` | Include classVariant links when an availability constraint is active |

Provider Category uses hierarchical tag matching. Provider names, ShortName, ClassName, and BaseName use whole case-insensitive matching; SourceId, ClassSourceId, and BaseSourceId are case-sensitive. There is no substring/fuzzy matching, default source, edition preference, or implicit merging of reprints.

Supplying a provider/subgroup/definition-source filter activates availability filtering even if bRequireAvailability=false. All availability constraints must match the same association. bIncludeVariantAvailability alone does not activate filtering. An unqualified declaration is not treated as a declaration defined in every possible publication.

Unknown availability on an otherwise matching spell excludes it from an availability-filtered answer and produces one IncompleteAvailability warning for the query. Known-empty availability produces no such warning and no match. Ordinary queries need no availability data and emit no incompleteness warning for its absence. Query success with an empty result is valid; invalid Level values fail and leave outputs empty except for diagnostics.

## Availability source mapping

The inspected companion file is `data/generated/gendata-spell-source-lookup.json`. Its shape is spell-source key → spell-name key → provider branches. Spell selectors match through Unreal's case-insensitive string comparison; original lookup keys remain in JSON Pointer/identity evidence. This does not define full JavaScript Unicode-lowercasing equivalence. Preflight rejects case-colliding keys instead of selecting one arbitrarily.

| Source branch | Provider category | Conversion |
| --- | --- | --- |
| class | Category::Class | Publication → class name → true or definition-source metadata |
| classVariant | Category::Class | Same shape, with bIsVariant=true |
| subclass | Category::Subclass | Class publication/name → subclass publication/short name → full name and optional subSubclasses |
| race | Category::Species | Publication/name → true or baseName/baseSource metadata |
| optionalfeature | Category::OptionalFeature | Publication/name → true or featureType string-array metadata |
| background / feat / charoption / reward | Corresponding category tags | Publication/name → true |

Class metadata accepts exactly one of definedInSource (nonempty string) or definedInSources (string/null array). Strings become DefinedInSources; a null records an unqualified declaration through bHasUnqualifiedDefinition. True leaves also set that flag. An empty qualification list with no unqualified declaration fails. Subclass full names stay separate from lookup short names and parent context; subSubclasses remain an ordered subgroup array on one association. Raw featureType codes stay strings, without dynamically registering tags.

False leaves, unsupported metadata, unknown selected branches, wrong types, and invalid required selector text fail. Output follows fixed branch order (class, classVariant, subclass, race, optionalfeature, background, feat, charoption, reward), with case-sensitive sorted object keys and source-ordered qualifier arrays. Provider selectors retain source identity without inventing normalized IDs for domains that have no reader yet. Associations establish neither loaded provider records nor character eligibility, casting grants, prerequisites, or their execution.

## Limits and diagnostics

Each supplied document uses the [reader's input/preflight limits](json-spell-reader.md#limits-and-diagnostics). Availability conversion allows 100,000 associations/qualifier strings per selected entry. Builds additionally allow at most 1,024 spell documents, 67,108,864 total TCHAR input units including an active lookup, 100,000 spell records, and 1,000,000 converted Content/availability values across records. Exceeding a limit fails without truncating or installing partial output. Core ReplaceSpells independently caps records at 100,000.

The existing Dnd5e.Json codes remain in use. Additional fixed codes are:

| Code | Severity | Meaning |
| --- | --- | --- |
| Dnd5e.Json.AvailabilityEntryNotFound | Info | Selected spell lacks a supplied lookup entry; availability remains unknown |
| Dnd5e.Json.MissingSpellDocuments | Error | No spell document supplied |
| Dnd5e.Json.AvailabilityNamespaceMismatch | Error | Lookup and spell-document namespaces differ |
| Dnd5e.Json.InvalidCatalogObject | Error | Blueprint build target is null or invalid; parsing has not started |
| Dnd5e.Catalog.InvalidSpell | Error | Required catalog identity/name/level check failed |
| Dnd5e.Catalog.DuplicateRecordId | Error | Repeated exact ID |
| Dnd5e.Catalog.InvalidAvailability | Error | Inconsistent presence or incomplete provider/qualification |
| Dnd5e.Catalog.InvalidQuery | Error | Level filter outside -1 or 0–9 |
| Dnd5e.Catalog.ResourceLimitExceeded | Error | Core record limit exceeded |
| Dnd5e.Catalog.IncompleteAvailability | Warning | Otherwise matching records have unknown availability |

Availability provenance retains the lookup label and escaped provider JSON Pointer. Its SourceId remains empty because the generated lookup has no single publication to assert; Provider.SourceId identifies the linked provider's publication. IdentityFieldsJson retains spell lookup keys, branch, and provider/parent lookup selectors. No automatic logging is performed.

## Verification and remaining work

The [automation suite](testing.md) covers all new value contracts, exact identity/reflected equality, copy-out, atomic failures, availability shapes/presence/qualification, joins, and query behavior. The explicitly selected external snapshot build produced 969 spells with 969 matched lookup entries and 14,448 associations. No source records, passages, lookup file, or real-data reports are bundled in the repository.

Editor selection/settings/inspection, provider-record resolution, omitted-source/ambiguous reference policy, media/fluff loading, explicit edition policy, and complete typed rule semantics remain pending. Blueprint construction/query wrappers and the separate [explicit file loader](json-spell-file-loader.md) are implemented. A consuming project's earlier prototype file-reading helper remains outside the plugin API. Catalog availability is a reference-data index and does not implement a character's spell selection or casting system.
