# Automated tests

Runtime APIs and editor integration are tested by Dnd5eEditor under the Dnd5e automation prefix. The suite uses Unreal's C++ automation framework, editor context, product filter, and WITH_DEV_AUTOMATION_TESTS guard. Tests are not part of the packaged runtime module. Runtime models/helpers are exercised across the module boundary; editor settings/session behavior has separate cases.

Verification on 2026-10-30, Unreal Engine 5.8 on Windows: the editor target builds and the standard suite contains **348 passing cases**. The reprint batch adds 19 cases to the previous 329: 16 selector/structure cases, identity agreement/order, reader-to-relationship lookup, and reflected relationship invocation. The existing 21 reference-failure cases now also check relationship forwarding. Runtime values remain 51 core structs and nine JSON-owned structs, with 269 reflected fields, 15 enums, 129 native tags, and 18 Content helpers. The catalog wrappers expose nine reflected operations across two runtime UObject classes. Three private editor classes implement settings, session ownership, and transient browser selection; they are not additional runtime models. The host's 22 legacy leaf redirects are checked through text import. Compilation alone is not passing-run evidence.

## Coverage

The combined verification run passed **369 cases**: all 348 standard cases, 18 opt-in per-file cases converting **969 records**, one catalog integration case joining **969 lookup entries / 14,448 associations**, one filesystem-loader case loading all 969 records, and one separately enabled reprint-closure audit verifying **389 normalized edges and loaded targets**. There were zero failed/not-run cases and no automation test warnings/errors. The primary reader audit returned **2,167 diagnostic warnings**, down from 2,556 after removing the 389 unconditional warnings for now-normalized reprints. These are API data, not silent test warnings or proof of complete semantics. Target lookup remains a separate explicit operation.

| Test group | Contract exercised |
| --- | --- |
| Dnd5e.Model.Contract | Every exported model: reflected field names/types, complete array element types, Blueprint visibility/editability, runtime module ownership, and default/presence semantics |
| Dnd5e.Model.CopyAndSerialization | Every model: populated reflected storage, ordinary value-copy isolation, and binary reflected serialization/load round trips, including Unicode, nested arrays, and instanced payloads |
| Dnd5e.Model.EnumContract | All public enum names, member identities/order, and Blueprint exposure |
| Dnd5e.Content.Extraction | Every wrapper extractor: correct payload preservation, independent copy-out, complete output reset, read-only failure, empty payloads, invalid/wrong/parent/child kinds, and all other public struct payload types |
| Dnd5e.Content.Annotation.DiceExpression | DiceRoll/DamageRoll inner values, syntax/expression preservation, independent copy-out, output reset, invalid kinds/storage/types, and separation of outer annotation from inner value |
| Dnd5e.Json.Contract | Supplied text/context/index preservation; independent diagnostic/result ownership; result round trips; reflected replacement of success/failure values without stale payloads |
| Dnd5e.Json.Reader | Actual ReadSpell calls: complete synthetic mapping, strict field/type/presence and vocabulary handling, numeric token/range checks, Unicode/escaped identity, immutable requests, fresh results, source locations, metadata, fallback, reprint normalization/preservation, concentration association, duplicates, and input/depth/value limits |
| Dnd5e.Json.Reprints.Selectors / IdentityAgreementAndOrder | Sixteen explicit/trimmed/Unicode/percent, unsupported-selector, and structural-failure cases; original selector preservation, output flags/default IDs, located warnings, stable order, and shared header/inline/reprint identity |
| Dnd5e.Catalog.Relationship.ReaderToLookup | Normalized target absent/present in selected catalog, independent copy, no implicit replacement, and no recursive traversal even with a cycle |
| Dnd5e.Json.ExternalReprints | Optional caller-asserted complete-directory audit: independent source edge count, normalized targets, exact resolution, and retained selector agreement with target name/publication |
| Dnd5e.Catalog | Exact C++/reflected ID equality and hashing, stable filtered queries, independent copy-out/reset, atomic replacement, variant/qualification filters, and incomplete availability diagnostics |
| Dnd5e.Catalog.Reference.Failures | 21 cases covering missing flags/inactive IDs, invalid/control-character identity text, validation precedence, unsupported categories, exact case/whitespace behavior, empty catalogs, no name/alias fallback, output reset, diagnostic severity/location, and input/catalog immutability |
| Dnd5e.Catalog.Reference.ExactLookupAndCopy / ReplacementLifecycle / ReaderToLookup | Full nested copy isolation, exact IDs among equal names, load/clear/replacement lifecycle, and actual JSON source-explicit reference extraction to target lookup while omitted sources remain unresolved |
| Dnd5e.Json.Availability | All nine provider branches with synthetic qualifiers/context, source pointers, missing versus known-empty data, immutable requests, round trips, malformed metadata, and atomic failure |
| Dnd5e.Json.Catalog | Multi-document conversion and lookup joins, namespace boundaries, duplicate IDs, explicit clearing, inactive optional values, later failures, and document limits |
| Dnd5e.Json.ExternalCatalog | Optional supplied-directory/lookup build, all-record count, association totals, and unfiltered query; discovered only with both explicit path arguments |
| Dnd5e.Json.ExternalLoader | Optional explicitly selected spell-directory audit through the production file loader, with independent expected counts and relative provenance; no availability lookup supplied in this case |
| Dnd5e.Json.Loader.Selection | 26 Windows selection cases: namespace/root requirements, relative/drive-relative roots, labels/extensions/traversal/wildcards, platform-aware duplicates, optional lookup paths, and document limits; every failure preserves seeded records and input values |
| Dnd5e.Json.Loader.ReadBuildAndProvenance | Ordered selected files, Unicode names, case-insensitive JSON extensions, inactive lookup ignored, exact namespace, relative filenames, and per-file pointers |
| Dnd5e.Json.Loader.AtomicFailureAndClear | Missing later file, full preflight before reads, malformed later JSON, duplicate records across distinct files, empty text/directory reads, and explicit empty-array clearing |
| Dnd5e.Json.Loader.AvailabilityAndWarnings | Filesystem lookup join, missing/malformed lookup preservation, returned unsupported-content Warning and missing-entry Info |
| Dnd5e.Json.Loader.EncodingAndByteLimits | Engine UTF-8/UTF-16 BOM handling, inclusive 16 MiB per-file and 64 MiB aggregate byte boundaries, oversized inputs, lookup bytes included, and per-call budget reset |
| Dnd5e.Editor.Dataset.Schema | Editor class ownership/bases, per-project user config without DefaultConfig, all five picker/config fields, stable preference location, and transient non-config catalog/result fields |
| Dnd5e.Editor.Dataset.PathConversion | Absolute picker paths to relative filenames, preserved order/namespace/settings, inactive lookup, active lookup, and rejectable outside-root/different-drive selections |
| Dnd5e.Editor.Dataset.ConfigIsolation | Save/reload all selection fields through an explicit temporary INI; actual user-config bytes and settings CDO remain unchanged |
| Dnd5e.Editor.Dataset.SessionAndAtomicLoading | Unloaded initial state, explicit lazy catalog creation, separate owners, failure preserving the same object/records, complete result storage, and successful retry without settings changes |
| Dnd5e.Editor.Dataset.LifetimeAndDeinitialize | Reflected strong ownership across collection, deinitialization reset, and collection of the released catalog |
| Dnd5e.Editor.Dataset.MenuRegistration | Auto-registered settings object, unique Tools entries with module-matching owner, and registered Message Log listing where UI services exist |
| Dnd5e.Editor.SpellBrowser.ModelMetadataAndIdentity / ModelNameSearch / ModelRebuildAndOwnership | Catalog order, case-sensitive opaque IDs, matching-name variants, primary/missing source, cantrip level, case-insensitive trimmed name substrings, empty/no-match filters, replacement/null rebuilds, and independent display metadata |
| Dnd5e.Editor.SpellBrowser.CatalogNotifications | Success/known-empty replacement notify after state installation; failure preserves records without notifying; deinitialization observers see cleared state |
| Dnd5e.Editor.SpellBrowser.DetailsSelectionLifecycle | Production selection wrapper and standard object inspector: populated nested Content, eight selection/clear cycles, actual detail ticks, disabled editing, and survival through garbage collection |
| Dnd5e.Editor.SpellBrowser.WidgetSelectionAndRefresh | Production widget's actual Slate list/search/inspector controls: exact-ID selection, independent copy, strong ownership, search clearing, notification refresh retaining search, and release after close |
| Dnd5e.Editor.SpellBrowser.MenuAndTabRegistration | Stable browser tab factory and one Tools entry with the module cleanup owner |
| Dnd5e.Json.External | Optional caller-selected local-file smoke test; discovered only when an explicit Dnd5eJsonSmokeFile argument is supplied, outside the default synthetic suite |
| Dnd5e.Json.ExternalDataset | Optional explicit-directory audit: one case per spells-*.json file, converting every selected record and reporting counts/codes/locations without source passages |
| Dnd5e.Blueprint.ExtractionAPI | All helper functions retain their reflected static/callable/pure flags, const struct input, concrete output, and bool success return |
| Dnd5e.Blueprint.SpellCatalog.Reflection | Both classes' module/base/type flags; all nine function signatures, complete array element types, const/static/pure flags, output directions, and native-only mutable access |
| Dnd5e.Blueprint.SpellCatalog.ResolveRelationship | Reflected relationship input, enum/copy/diagnostic outputs, all failure statuses, output reset, recovery, and input immutability |
| Dnd5e.Blueprint.SpellCatalog.ResolveReference | ProcessEvent with an initialized reflected frame: enum return, full copy-out, all four failure statuses, input immutability, stale-output clearing, and recovery to diagnostic-free success |
| Dnd5e.Blueprint.SpellCatalog.BuildQueryAndCopyOut | ProcessEvent invocation of build/count/query/get using initialized reflected parameter frames; typed Content/annotations, immutable requests, exact lookup/copy-out/reset, and level versus publication constraints |
| Dnd5e.Blueprint.SpellCatalog.AtomicFailureAndDiagnostics | Invalid/null targets, fixed diagnostic codes/locations, failed JSON/core replacement preserving old records, and explicit clearing |
| Dnd5e.Blueprint.SpellCatalog.AvailabilityWarnings | Missing availability warning, supplied lookup join, provider query, and preservation warnings returned unchanged |
| Dnd5e.Blueprint.SpellCatalog.OwnershipAndGarbageCollection | Separate initially empty catalogs, independent ID copies, survival under a strong reference, and collection after that reference is released |
| Dnd5e.Blueprint.SpellCatalog.LoadFiles | LoadSpellCatalog through an initialized ProcessEvent frame, retained object installation, immutable input, and invalid target before request validation/file reads |
| Dnd5e.Tags.NativeVocabulary | Fixed public native tag strings, dictionary registration, uniqueness, root spelling, and absence of obsolete duplicate identities |
| Dnd5e.Tags.HierarchyAndRoles | Intended map ancestry; action/bonus-action siblings; shared timing, attack mode, school, and concentration scope; separate range/origin identities |
| Dnd5e.Tags.ConfiguredRedirects | Any host-configured Dnd5e leaf redirects reach registered current names through reflected text import; a fresh host may have no legacy redirects |
| Dnd5e.Spell | A composed synthetic spell: nested Content, source-order timing/duration alternatives, per-entry concentration, distinct classification roles, mixed-denomination costs, provenance, normalized/unresolved relationships, reflected round trip, and deep copy isolation |
| Dnd5e.Values.PresenceAndNumericBoundaries | int32 endpoints, explicit zero versus unspecified, double precision/literal preservation, numeric versus textual page references, and absent versus explicit zero cost |
| Dnd5e.Content.Section.OutputReset / Dnd5e.Content.CopyIndependence | Original focused regression tests, retained and rerun with the full suite |

The fixed model and tag manifests are compiled test expectations, reviewed against the documented API. They are not generated from runtime reflection during test execution and do not read source headers or a dataset. Intentional public API changes require reviewing/updating the expectations alongside the model documentation. Type-directed checks also link every public struct from the editor module, helping catch missing runtime exports.

Storage probes deliberately populate every reflected field, including FName diagnostic codes using controlled test identifiers and inactive optional fields, to detect dropped data. They are serialization probes, not validated D&D definitions. The separately authored synthetic spell exercises contextual composition. Neither probe pretends that an extraction helper validates a rule: correctly typed default payloads remain extractable under the documented contract.

The extraction tests load development-only child tags from temporary INI sources under the host Saved/Automation directory, then delete the generated files. Editor modules cannot declare native gameplay tags, so these fixtures are not native declarations or persistent project-config edits. Their in-memory dictionary nodes may remain until the editor worker exits/restarts. They prove that hierarchical descendants cannot bypass exact discriminator matching. The 129 documented exported native identities belong to the runtime module.

## Running the suite

Build the development host's Editor target first. Open that host project, go to Session Frontend's Automation tab, enable the Product/Project filter, select Dnd5e, and run the selected tests. Play In Editor is not required. A Functional Testing Editor shortcut is optional; the C++ suite uses the automation framework directly.

For an unattended run, invoke the engine's UnrealEditor-Cmd executable with your host .uproject and these arguments:

```text
-unattended -nop4 -nosplash -nullrhi -nosound -NoLiveCoding
-ExecCmds="Automation RunTests Dnd5e."
-TestExit="Automation Test Queue Empty"
-ReportExportPath="<host>/Saved/Automation/Dnd5eFullSuite"
```

Use the actual absolute report directory and quote paths containing spaces. The standard suite runs without external datasets or media; its fixtures and catalog records are original synthetic values. Tests do not download content or open fixture URL/media paths. Optional audits read only explicitly selected external files. Binary archive tests are in-memory round trips, not a new plugin import/export API or a promise of compatibility with future saved formats.

For an optional local compatibility check, select Dnd5e.Json.External and additionally pass `-Dnd5eJsonSmokeFile="<absolute-user-supplied-json-file>" -Dnd5eJsonSmokeIndex=<explicit-index>`. The harness loads the chosen file and passes its text to the supplied-text reader; that reader itself still performs no I/O. This case is discovered only when the file argument is present and is outside the default 348 cases. It checks acceptance, identity presence, and selected-record provenance without logging passages. Keep the file and reports outside the repository.

To audit every spell in explicitly selected files, pass `-Dnd5eJsonSmokeDirectory="<absolute-user-supplied-spells-directory>"` and select Dnd5e.Json.ExternalDataset, or the full Dnd5e prefix to combine the audit with the standard suite. The harness discovers spells-*.json under that directory and calls ReadSpell with every explicit index. It registers a failing case if the supplied directory has no matching files. Source acceptance includes explicit fallback/reference warnings; it does not establish typed semantics for every retained token. The default suite does not discover/read this directory without that argument.

Inspect the report's test list, success/failure counts, and errors; an editor process starting or exiting is insufficient evidence. Logs and exported reports belong under the host's Saved directory, outside the plugin repository. Native tags/configuration should be exercised in a fresh editor worker after a rebuild, rather than relying on an old Live Coding session.

For the catalog integration case, supply both `-Dnd5eJsonSmokeDirectory="<absolute-user-supplied-spells-directory>"` and `-Dnd5eJsonAvailabilityFile="<absolute-user-supplied-lookup-file>"`, then select Dnd5e.Json.ExternalCatalog or the full prefix. In that case the harness opens files and the builder receives text. The final 2026-10-30 reprint report is under the development host's Saved/Automation/Dnd5eReprintNormalizationVerified, outside Git. Earlier reference/browser/editor-loading reports remain under their preceding batch directories. The standard suite remains independent of these real-data paths.

To enable the additional Dnd5e.Json.ExternalReprints closure audit, also pass `-Dnd5eJsonAuditReprints` and select a complete directory with the SmokeDirectory argument. This separate opt-in asserts that every declared reprint in that selection can normalize and find a loaded target; it is not an acceptance requirement for a deliberately partial dataset. The audit registers only when its flag is present and fails clearly if the directory argument is missing. No publication names or passages are logged. The verified snapshot contained 389 edges across 969 spells.

Select Dnd5e.Json.ExternalLoader with the explicit SmokeDirectory argument to exercise actual production file reads. The harness enumerates spells-*.json in that selected directory and supplies their relative names/root to LoadCatalog; this is opt-in test discovery, not production automatic dataset discovery. The independent count pass and actual loader both read the selected files. The loader case omits availability. It also supports a directory containing a caller-prepared subset without changing plugin fixtures.

A separate 2026-10-30 opt-in loader run passed its one case against the local source-marked SRD 5.1 subset and installed all 319 selected records. Its report is under the host Saved/Automation/Dnd5eFileLoaderSrd51. The user subsequently confirmed the new LoadSpellCatalog Blueprint graph works after supplying the explicit SpellFiles array. That manual result is distinct from both the native audit and synthetic ProcessEvent coverage.

File-loader fixtures are original synthetic documents under unique generated children of the host Saved/Automation/Dnd5eLoaderFixtures directory. They include temporary boundary-size files. Cleanup checks the resolved target stays under that fixture parent and is not the parent before recursively removing its own child. Tests neither modify the selected external datasets nor save their passages into the repository.

Editor cases use a separate generated child under Saved/Automation/Dnd5eEditorFixtures. The config case supplies an explicit temporary filename and SaveConfig's bAllowCopyToDefaultObject=false; the engine's default true could otherwise change the real settings CDO. It unloads the temporary cache entry before reloading from disk and checks both actual preference-file bytes and the real CDO snapshot afterward. Personal paths are compared privately in memory, not copied into fixtures or printed on success. Settings/session tests do not click the live load command or save the actual preferences.

Browser notification fixtures use guarded unique children under Saved/Automation/Dnd5eBrowserFixtures. The actual widget test temporarily installs an independent synthetic catalog through the editor subsystem's reflected Catalog property, retains and restores the original catalog object, and never changes its records, last load result, or remembered settings. The details regression uses the production editor selection class rather than a duplicate test-only UObject. Tests exercise real control callbacks and PropertyEditor ticks, including nested Content; they do not simulate mouse gestures, every painted expansion, or unloading their own module.

The registration case passed against the real settings/menu/listing registrations in the verified worker. In a non-rendering environment with no level-editor Tools menu it reports that limitation explicitly. It never unloads the module containing its executing test code. Shutdown callbacks/owner cleanup are reviewed; native dialogs and restart persistence also rely on the user's manual editor check.

## Test files and assertions

The optional external smoke case was also run separately on 2026-10-29 against one explicitly selected 2014 PHB spell in the local dataset: one test passed, with zero failed/not-run cases and no test warnings/errors. This verifies that selected record's acceptance under the limited subset, not every record or complete 2014 coverage.

All test sources live under Source/Dnd5eEditor/Private/Tests and are guarded by WITH_DEV_AUTOMATION_TESTS. Dnd5eEditor.Build.cs declares Engine, GameplayTags, and Json for direct test use. Dnd5eJson publicly depends on Core/CoreUObject/Dnd5e/Engine and privately on Json/GameplayTags. Engine supplies the public Blueprint library base; GameplayTags is a direct private dependency because conversion calls AddTag. The developer wrote the original envelopes and Blueprint wrappers; the assistant completed the reader and automated behavior tests under explicit authorization.

| File | Responsibility |
| --- | --- |
| Dnd5eTestSupport.h/.cpp | Shared in-memory fixtures, recursive default checks, storage probes, one-leaf mutation, and archive round-trip helpers |
| Dnd5eModelContracts.inl | Frozen expectations for all 60 model schemas and 269 properties across both runtime modules |
| Dnd5eModelTests.cpp | Parameterized model/default/Blueprint-reflection cases, copies/serialization, and 15 enum contracts |
| Dnd5eExtractionTests.cpp | Parameterized extraction failure matrix, inner dice values, transient child-tag fixture loading, and reflected Blueprint function signatures |
| Dnd5eNativeTagContracts.inl | Frozen expected symbol/string pairs for all 129 exported native identities |
| Dnd5eTagTests.cpp | Native registration, uniqueness, hierarchy/role checks, and configured legacy text-import redirects |
| Dnd5eJsonContractTests.cpp | Cross-module JSON request/result storage tests, diagnostic codes/locations, and value replacement |
| Dnd5eJsonSpellReaderTests.cpp | Synthetic reader scenarios, rejected-field matrix, recursive Content/inline conversion, new scaling values, percentile/sentinel bounds, input/conversion budgets, and optional explicit-file/directory audits |
| Dnd5eSpellCatalogTests.cpp | Exact identity/reflection, core ownership/query behavior, supplementary availability parsing, atomic supplied-document builds, and the opt-in external catalog integration case |
| Dnd5eReferenceResolutionTests.cpp | Native reference failure matrix, deep copy isolation, replacement lifecycle, and synthetic JSON-reference extraction followed by exact catalog lookup |
| Dnd5eReprintTests.cpp | Reprint grammar/preservation/identity integration, native relationship lookup, and the separately enabled external closure audit |
| Dnd5eBlueprintCatalogTests.cpp | Reflected wrapper contracts and ProcessEvent invocation, copy/reset behavior, atomic failure/diagnostics, availability forwarding, independent objects, and garbage-collection lifetime |
| Dnd5eJsonSpellLoaderTests.cpp | Synthetic filesystem selection/read/limit cases, reflected loading, guarded temporary-fixture cleanup, and the opt-in selected-directory loader audit |
| Dnd5eEditorDatasetTests.cpp | Private editor schemas, picker conversion, isolated config save/reload, session atomicity, GC/deinitialization, and available settings/menu/listing registration |
| Dnd5eSpellBrowserDetailsTests.cpp | Production selection wrapper, standard object inspector, repeated nested-Content updates, editing disabled, and GC retention |
| Dnd5eSpellBrowserTests.cpp | Metadata/search/rebuild models, notifications, actual Slate control selection/refresh/copy/close behavior, and menu/tab registration |
| Dnd5eSpellTests.cpp | Original synthetic composed spell, nested copy isolation, contextual role preservation, and numeric/presence boundaries |
| Dnd5eContentLibraryTests.cpp | Original two regression tests, retained unchanged |

IMPLEMENT_COMPLEX_AUTOMATION_TEST registers a group whose GetTests method lists named cases; RunTest receives the selected case name. IMPLEMENT_SIMPLE_AUTOMATION_TEST registers one scenario directly. Assertions record expected versus actual behavior in the report. A failed essential setup check returns early before unsafe access; a passing case returns without recorded errors. The fixtures construct inputs, production functions perform the operation, and assertions check the observable contract. None of the helpers is a new runtime plugin API.

To maintain coverage, review/update the model/tag manifests when intentionally changing the public schema or tag vocabulary. Add a behavioral scenario when a new importer, resolver, validator, or other capability exists. Do not regenerate expected results from live runtime reflection to make a failing contract test pass.

## Current limits

Tests cover storage/reflection/extraction, native identities, composed values, spell/Content conversion, availability joins, native catalogs, normalized spell-reference lookup, and Blueprint-facing wrapper behavior. Required known structures are checked; unmodeled Content is retained explicitly. Omitted-source/ambiguous selector normalization, other-domain/general reference resolution, complete typed rule interpretation, full media/document coverage, and effect execution remain pending. ProcessEvent/reflection tests exercise the reflected API but do not compile or run an arbitrary Blueprint asset. The user separately verified the host Blueprint flow with 319 source-marked SRD 5.1 records, queried all IDs, and retrieved a spell; that manual result is distinct from the synthetic automated suite. Tag text-import migration checks do not establish migration of every binary Blueprint asset or parent-tag query. No compatibility with another engine version or a historical binary archive format is implied.

Dnd5eJson exposes supplied-text readers/building plus the [native/Blueprint file loader](json-spell-file-loader.md). Runtime module hooks remain empty; Dnd5eEditor registers the [settings/menu/results interface and searchable read-only spell browser](editor-dataset-loading.md) without loading datasets at startup. The user verified 319 editor-loaded records with 823 returned diagnostics, settings retained after restart, and successful selection using the corrected object-details view. The earlier host helper remains outside the distributable plugin and runtime suite.
