# Automated tests

Runtime APIs and editor integration are tested by Dnd5eEditor under the Dnd5e automation prefix. The suite uses Unreal's C++ automation framework, editor context, product filter, and WITH_DEV_AUTOMATION_TESTS guard. Tests are not part of the packaged runtime module. Runtime models/helpers are exercised across the module boundary; editor settings/session behavior has separate cases.

Verification on 2026-11-10, Unreal Engine 5.8.4 on Windows: the Development Editor target builds. The standard suite contains **836 cases**; the combined explicitly supplied-data run passes **865 cases**, with zero failed/not-run cases, automation warnings, or automation errors. Frozen schema/default/copy/serialization expectations cover **98 structs (85 Core, 13 JSON), 472 fields, 27 enums, 176 native tags and 25 Content extraction helpers**. Dedicated validation results are BlueprintReadOnly; their access flags and actual reflected invocation are tested. The item definition catalog and its Blueprint wrapper share one lookup contract.

## Coverage

The current combined run includes all standard cases plus 25 opt-in spell cases: eighteen primary-file conversions (969 records), availability integration (969 lookup entries / 14,448 associations), explicit file loading, 389 reprint targets, reference collection/reporting, independent scaled-token counts, and the new primary-plus-supplement completion audit. That audit validates all **969 primary / 92 supplementary records and 91 images** and compares raw source openings to typed occurrences for all **27 source-token kinds**. No opaque payloads remain in this inspected primary/supplementary input. Mechanical-description reference collection finds **1,906 occurrences**, with **0 opaque-fallback owners**; 67 original targets resolve, 68 selectors normalize and their normalized targets resolve.

The primary reader returns **1,847 warning diagnostics** and the primary-plus-supplement build returns **1,849**; these are returned unresolved-reference/preservation data, not automation log warnings or silently discarded content. Typed selectors without imported target domains/publications remain useful unresolved data. Snapshot acceptance does not establish every future/custom form, complete prose-rule semantics, destination navigation, or gameplay execution.

Batches C–H add 203 standard cases to the previously verified 543. Coverage includes all generic selector domains and feature qualifiers, document/quickref/filter grammar, nested labels and note/list/table presentation, exact extraction/copy/reset, media paths, supplementary reader output reset and atomic joins/file loading, namespace/publication isolation, source membership/hints, explicit edition policy, validation branches and shared limits, optional candidate validation and actual Blueprint invocation. The standard suite uses original synthetic fixtures only. The external inventory remains opt-in and outside Git.

| Test group | Contract exercised |
| --- | --- |
| Dnd5e.Model.Contract | Every exported model: reflected field names/types, complete array element types, Blueprint visibility/editability, runtime module ownership, and default/presence semantics |
| Dnd5e.Model.CopyAndSerialization | Every model: populated reflected storage, ordinary value-copy isolation, and binary reflected serialization/load round trips, including Unicode, nested arrays, and instanced payloads |
| Dnd5e.Model.EnumContract | All public enum names, member identities/order, and Blueprint exposure |
| Dnd5e.Content.Extraction | Every wrapper extractor: correct payload preservation, independent copy-out, complete output reset, read-only failure, empty payloads, invalid/wrong/parent/child kinds, and all other public struct payload types |
| Dnd5e.Content.Annotation.DiceExpression | DiceRoll/DamageRoll inner values, syntax/expression preservation, independent copy-out, output reset, invalid kinds/storage/types, and separation of outer annotation from inner value |
| Dnd5e.Json.Contract | Supplied text/context/index preservation; independent diagnostic/result ownership; result round trips; reflected replacement of success/failure values without stale payloads |
| Dnd5e.Json.Annotations.NumericAndRollValues / Dnd5e.Content.Annotation.NumericExtractorOwnership / ChanceReferenceTraversal | Literal DC/d20/attack/percent values, signed integer boundaries and preserved literals, labels/names/outcomes, exact role/type extraction and copy/reset, malformed/overflow/extra-argument fallbacks, and chance Value.Success/Failure before Display references |
| Dnd5e.Json.Annotations.ScaledRollValues / Dnd5e.Content.Annotation.ScaledExtractorAndDisplayTraversal | Thirty supported/fallback progression cases, separate alternatives and raw expressions, exact mode/axis mapping, labels, role/type extraction/copy/reset, opaque expression strings, nested display references, and recursive failure without partial output |
| Dnd5e.Json.ExternalScaling | Optional independent decoded-source/typed-output count agreement for every selected primary file; all 14 scaledice and 148 scaledamage tokens verified in the supplied snapshot |
| Dnd5e.Json.Reader | Actual ReadSpell calls: complete synthetic mapping, strict field/type/presence and vocabulary handling, numeric token/range checks, Unicode/escaped identity, immutable requests, fresh results, source locations, metadata, fallback, reprint normalization/preservation, concentration association, duplicates, and input/depth/value limits |
| Dnd5e.Content.References / Dnd5e.Blueprint.SpellContent | All 26 current child-container routes and exact model paths, parent/Display and description/higher-level order, independent copied output, opaque leaves, excluded annotation Value payloads, malformed wrappers, atomic failure, inclusive depth/node/occurrence limits, per-call reset, reader integration, and reflected invocation/signature |
| Dnd5e.Json.ExternalReferenceCollection | Opt-in collection from every converted supplied spell, unique rooted model paths and source context, aggregate typed-reference/fallback counts; no claim about references hidden in fallback text |
| Dnd5e.Json.ReferenceReport / Dnd5e.Blueprint.SpellCatalog.ReferenceReport | Sixteen independent-stage combinations, conflicting valid original/normalized IDs, model locations and per-stage diagnostics, immutable owner/catalog, copy/snapshot ownership, replacement, structural/limit failure, empty success, and actual reader-to-report reflected invocation |
| Dnd5e.Json.ExternalReferenceReport | Opt-in full supplied-catalog reporting, per-spell collection agreement, second-lookup gating, and aggregate stage outcomes; no implicit repair or hidden-fallback coverage |
| Dnd5e.Json.ReferenceNormalization | 34 token/input cases, inclusive token-size bounds, missing-source/ambiguity policy, unchanged source/display, failure-copy preservation, exact canonical ID correction, ownership/aliasing, replacement, and actual reader-to-normalizer-to-lookup integration |
| Dnd5e.Json.Reprints.Selectors / IdentityAgreementAndOrder | Sixteen explicit/trimmed/Unicode/percent, unsupported-selector, and structural-failure cases; original selector preservation, output flags/default IDs, located warnings, stable order, and shared header/inline/reprint identity |
| Dnd5e.Catalog.Relationship.ReaderToLookup | Normalized target absent/present in selected catalog, independent copy, no implicit replacement, and no recursive traversal even with a cycle |
| Dnd5e.Json.ExternalReprints | Optional caller-asserted complete-directory audit: independent source edge count, normalized targets, exact resolution, and retained selector agreement with target name/publication |
| Dnd5e.Catalog | Exact C++/reflected ID equality and hashing, stable filtered queries, independent copy-out/reset, atomic replacement, variant/qualification filters, and incomplete availability diagnostics |
| Dnd5e.Catalog.Selector.Matching | 24 cases covering whole case-insensitive name/source matching, exact namespace, no alias/substring/trimming fallback, missing source with zero/one/many candidates, primary/missing/contributing provenance, invalid input, output reset, and immutable input/catalog values |
| Dnd5e.Catalog.Selector.AmbiguityAndExistingContracts / OutputOwnershipAndReplacement | Canonical IDs and installation order for ambiguous equivalents, unchanged case-sensitive query-source/exact-ID contracts, independent output arrays/strings, and fresh answers after replacement/clear |
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
| Dnd5e.Editor.SpellBrowser.ReportSelectionSchema / ReferenceReportInspection | Private transient/read-only report storage; production selection builds the report and cached summary against the same catalog; independent stage counts, opaque warning count, zero/failure states, populated details ticks, GC/copy isolation, and search/refresh clearing |
| Dnd5e.Editor.SpellBrowser.DetailsSelectionLifecycle | Production selection wrapper and standard object inspector: populated nested Content, eight selection/clear cycles, actual detail ticks, disabled editing, and survival through garbage collection |
| Dnd5e.Editor.SpellBrowser.WidgetSelectionAndRefresh | Production widget's actual Slate list/search/inspector controls: exact-ID selection, independent copy, strong ownership, search clearing, notification refresh retaining search, and release after close |
| Dnd5e.Editor.SpellBrowser.MenuAndTabRegistration | Stable browser tab factory and one Tools entry with the module cleanup owner |
| Dnd5e.Json.External | Optional caller-selected local-file smoke test; discovered only when an explicit Dnd5eJsonSmokeFile argument is supplied, outside the default synthetic suite |
| Dnd5e.Json.ExternalDataset | Optional explicit-directory audit: one case per spells-*.json file, converting every selected record and reporting counts/codes/locations without source passages |
| Dnd5e.Blueprint.ExtractionAPI | All helper functions retain their reflected static/callable/pure flags, const struct input, concrete output, and bool success return |
| Dnd5e.Blueprint.SpellCatalog.Reflection | Catalog/library module/base/type flags; twelve catalog/JSON function signatures, complete array element types, const/static/pure flags, output directions, FString NativeConst metadata, and native-only mutable access; the separate SpellContent case checks the thirteenth operation and its third library class |
| Dnd5e.Blueprint.SpellCatalog.MatchSelector | ProcessEvent with the new request/enum: unique canonical match, one-candidate MissingSource, NoMatch, invalid-input reset, ambiguity, recovery, and exact lookup of the returned ID |
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

The extraction tests load development-only child tags from temporary INI sources under the host Saved/Automation directory, then delete the generated files. Editor modules cannot declare native gameplay tags, so these fixtures are not native declarations or persistent project-config edits. Their in-memory dictionary nodes may remain until the editor worker exits/restarts. They prove that hierarchical descendants cannot bypass exact discriminator matching. The 137 documented exported native identities belong to the runtime module.

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

For an optional local compatibility check, select Dnd5e.Json.External and additionally pass `-Dnd5eJsonSmokeFile="<absolute-user-supplied-json-file>" -Dnd5eJsonSmokeIndex=<explicit-index>`. The harness loads the chosen file and passes its text to the supplied-text reader; that reader itself still performs no I/O. This case is discovered only when the file argument is present and is outside the default 746 cases. It checks acceptance, identity presence, and selected-record provenance without logging passages. Keep the file and reports outside the repository.

To audit every spell in explicitly selected files, pass `-Dnd5eJsonSmokeDirectory="<absolute-user-supplied-spells-directory>"` and select Dnd5e.Json.ExternalDataset, or the full Dnd5e prefix to combine the audit with the standard suite. The harness discovers spells-*.json under that directory and calls ReadSpell with every explicit index. It registers a failing case if the supplied directory has no matching files. Source acceptance includes explicit fallback/reference warnings; it does not establish typed semantics for every retained token. The default suite does not discover/read this directory without that argument.

Inspect the report's test list, success/failure counts, and errors; an editor process starting or exiting is insufficient evidence. Logs and exported reports belong under the host's Saved directory, outside the plugin repository. Native tags/configuration should be exercised in a fresh editor worker after a rebuild, rather than relying on an old Live Coding session.

For the catalog integration case, supply both `-Dnd5eJsonSmokeDirectory="<absolute-user-supplied-spells-directory>"` and `-Dnd5eJsonAvailabilityFile="<absolute-user-supplied-lookup-file>"`, then select Dnd5e.Json.ExternalCatalog or the full prefix. In that case the harness opens files and the builder receives text. The earlier 2026-10-30 Batch B run is under the development host's Saved/Automation/Dnd5eSpellBatchB, outside Git. Earlier reports remain under their preceding batch directories, including Saved/Automation/Dnd5eReprintNormalizationVerified. The standard suite remains independent of these real-data paths.

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
| Dnd5eModelContracts.inl | Frozen expectations for 67 reviewed model schemas and 298 properties across both runtime modules |
| Dnd5eAnnotationValueTests.cpp | Batch A reader matrix, inner-value extractor ownership/failure coverage, and nested chance-outcome traversal |
| Dnd5eScaledRollTests.cpp | Scaling parser matrix, extractor/display/failure behavior, and opt-in raw-source versus typed-output audit |
| Dnd5eModelTests.cpp | Parameterized model/default/Blueprint-reflection cases, copies/serialization, and 17 enum contracts |
| Dnd5eExtractionTests.cpp | Parameterized extraction failure matrix, inner dice values, transient child-tag fixture loading, and reflected Blueprint function signatures |
| Dnd5eNativeTagContracts.inl | Frozen expected symbol/string pairs for all 137 exported native identities |
| Dnd5eTagTests.cpp | Native registration, uniqueness, hierarchy/role checks, and configured legacy text-import redirects |
| Dnd5eJsonContractTests.cpp | Cross-module JSON request/result storage tests, diagnostic codes/locations, and value replacement |
| Dnd5eJsonSpellReaderTests.cpp | Synthetic reader scenarios, rejected-field matrix, recursive Content/inline conversion, new scaling values, percentile/sentinel bounds, input/conversion budgets, and optional explicit-file/directory audits |
| Dnd5eSpellCatalogTests.cpp | Exact identity/reflection, core ownership/query behavior, supplementary availability parsing, atomic supplied-document builds, and the opt-in external catalog integration case |
| Dnd5eReferenceCollectionTests.cpp | Synthetic container/path/order/copy/failure/limit cases, reader integration, and optional supplied-directory typed-reference collection audit |
| Dnd5eReferenceReportTests.cpp | Synthetic independent-stage combinations, collection/copy/limit/replacement behavior, and optional full-catalog report audit |
| Dnd5eReferenceResolutionTests.cpp | Native reference failure matrix, deep copy isolation, replacement lifecycle, and synthetic JSON-reference extraction followed by exact catalog lookup |
| Dnd5eReprintTests.cpp | Reprint grammar/preservation/identity integration, native relationship lookup, and the separately enabled external closure audit |
| Dnd5eSpellReferenceNormalizerTests.cpp | Synthetic retained-token normalization matrix, copy/aliasing/replacement, and reader-to-exact-lookup integration |
| Dnd5eSpellSelectorTests.cpp | Native selector matrix, ambiguity/canonical ID behavior, unchanged query/lookup contracts, and output ownership/replacement |
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

## Spell-coverage verification environment

The final report is in the host's `Saved/Automation/Dnd5eSpellCoverageVerified/index.json`, outside Git, with its `Saved/Logs/Dnd5eSpellCoverageVerified.log`. The preceding final run passed all 771 cases but captured two ambient Home Screen HTTP reachability timeouts. The verification repeat disables that unrelated UI only for the command-line process with `-ini:Engine:[ConsoleVariables]:HomeScreen.EnableHomeScreen=0`; no persisted host/engine preferences or plugin checks are changed.

The explicitly enabled `Dnd5e.Json.ExternalCoverageCompletion` case reads primary and fluff documents from Dnd5eJsonSmokeDirectory, enables Content validation, checks exact attachment/unmatched accounting and image counts, compares independently scanned source-token counts with typed output, and checks opaque payload absence. `-Dnd5eJsonAuditReprints` separately enables the existing closure audit. Directory paths and external passages are never embedded in test source or committed reports.

## Reader-folder organization verification

On 2026-11-01, the ten spell-specific public headers and ten private implementation files moved under Readers/Spells. Non-include code, exported schema, module/reflection names, IDs, behavior and fixtures remain unchanged. The Editor build succeeds and all 771 existing combined cases pass again with zero failed/not-run cases or automation warnings/errors. The host-only report is Saved/Automation/Dnd5eSpellOrganization/index.json. Public source links and include examples use the new paths; no dataset or generated report is included in the plugin.

## Item foundation verification

On 2026-11-01, the Development Editor target builds and all **752 standard / 777 combined cases** pass with no failed/not-run cases or automation warnings/errors. The host-only report is Saved/Automation/Dnd5eItemFoundation/index.json with Saved/Logs/Dnd5eItemFoundation.log. No runtime correction was required for the submitted item foundation.

The foundation adds six standard cases: schema/default/Blueprint-access and populated copy/serialization contracts for FDnd5eItem and FDnd5eItemDefinitionSelector, a rarity/definition-role case, and nested selector/Content ownership plus editor presence metadata. Twelve native identities extend the existing vocabulary contract. The combined run retains all 25 explicitly supplied spell audit cases; it does not import or audit item datasets. Items have foundation storage only, with no reader, resolver, item validator, price/mass/attunement mapping or item catalog yet.

## Item values verification

On 2026-11-02, the Development Editor target builds and all **764 standard / 789 combined cases** pass with zero failed/not-run cases, automation warnings or automation errors. The local report is Saved/Automation/Dnd5eItemValues/index.json and the log is Saved/Logs/Dnd5eItemValues.log. No runtime correction was required; appending the new item fields after SupplementaryContent preserves the declared contract.

This batch adds twelve standard cases: schema/default/access and populated copy/serialization for three shared structs, three enum contracts, and three behavior cases for price presence/currency baskets, fractional mass/attunement ownership, and editor presence conditions. The item schema includes its nine new fields. Tests distinguish absent/null/zero prices, retain multiplier-only prices and separate denominations, preserve numeric source literals and units, and verify nested copies and independent primary/alternative conditions. The previous 25 opt-in spell audits remain enabled; these results do not claim item JSON conversion, structured eligibility evaluation or item dataset coverage.

## Attunement index verification

On 2026-11-03, the Development Editor target builds and all **776 standard / 801 combined cases** pass with zero failed/not-run cases or automation warnings/errors. The local report is Saved/Automation/Dnd5eAttunementIndex/index.json and the log is Saved/Logs/Dnd5eAttunementIndex.log. The reviewed runtime declarations needed no functional correction; their flat Alignment namespace and EDnd5eSize enum are the documented contract.

This batch adds twelve standard cases: schema/default/access and populated copy/serialization for four structs, the size enum contract, and three behavioral cases. The expanded attunement requirement schema covers index presence/storage. Native tag expectations add the six exact sibling Alignment identities.

The behavioral cases preserve ten distinct concrete encodings (the nine grid alignments plus Unaligned), including singleton Neutral for True Neutral, paired Neutral Good and Lawful Neutral, and separate Lawful Good/Chaotic Evil alternatives. They explicitly demonstrate why a Neutral subset test cannot distinguish these cases. They also exercise independent primary/alternative groups, selectors with explicit/omitted sources, language terms, ability minimums, both fallback levels, completeness storage, inactive index retention, explicit false versus absent boolean fields, and editor EditCondition metadata. These are storage tests; there is no alignment matcher, eligibility evaluator, item index parser or validator to test yet. Existing opt-in audits still concern spells, not item conversion.

## Item equipment verification

On 2026-11-03, the Development Editor target builds and all **792 standard / 817 combined cases** pass with zero failed/not-run cases or automation warnings/errors. The local report is Saved/Automation/Dnd5eItemEquipment/index.json and the log is Saved/Logs/Dnd5eItemEquipment.log. The submitted runtime declarations match the guide; no runtime correction was required.

This batch adds sixteen standard cases: schema/default/access and populated copy/serialization for five structs, three enum contracts, and three equipment behavior cases. The FDnd5eItem contract includes its four new optional-data fields. Native tag and extraction-helper counts are unchanged.

Behavioral tests preserve constant and dice damage expressions with separate types, normal/long distances and units, independent weapon/armor payloads, copy ownership, base versus bonus AC roles, all Dexterity modes and inactive cap storage, Strength absent/null/zero, stealth absent versus explicit false, reload/range presence, inactive payload retention and eleven editor conditions. These verify reference-data storage, not rolling, final AC, movement, ammunition use, equipment state or item JSON conversion. The existing 25 explicitly supplied spell audits remain enabled and do not establish item dataset coverage.

## Item definition verification

The Development Editor build and all **803 standard / 828 combined cases** passed on 2026-11-03, with zero failures, skipped cases or automation warnings. The local report is Saved/Automation/Dnd5eItemDefinitions/index.json.

This batch adds eight schema and copy/serialization cases for four structs, plus three behavior cases for definition ownership and source separation, unexpanded template presence and retention, and independent mastery-reference notes. The item schema includes Masteries and the native vocabulary includes Category.ItemMastery. The external audits cover spells. Definition lookup and JSON conversion are pending.

## Item definition catalog verification

The Development Editor build and all **807 standard / 832 combined cases** passed on 2026-11-03. The report records zero failures, skipped cases and automation warnings. See Saved/Automation/Dnd5eDefinitionCatalog/index.json.

Four new cases cover the match status enum, canonical ID escaping and output reset, atomic replacement and scoped lookup, and reflected getter invocation. They exercise duplicate rejection, rollback after an invalid final domain, omitted publication candidates, qualifier rejection and copy ownership. The reflected case invokes the property getter through ProcessEvent and checks the output pin flags and reset on a miss.


## Item definition JSON verification

The Development Editor build and all **812 standard / 838 combined cases** passed on 2026-11-08. The report records zero failures, skipped cases and automation warnings. See Saved/Automation/Dnd5eDefinitionReaderFinal/index.json.

New tests cover conversion of the three definition domains, forward type copies, explicit entry overrides, metadata and unresolved reprint preservation, invalid field locations, duplicate rejection and rollback through the Blueprint library. The result schema has contract and copy/serialization coverage.

The opt-in external definition test takes `-Dnd5eJsonDefinitionFile=<path-to-items-base.json>`. The inspected local snapshot contains 67 type, 27 property and 8 mastery rows. The test checks all 102 definitions and both inherited type descriptions. It logs errors by pointer and message, while fixtures and dataset input remain local.


## Base item JSON verification

The Development Editor build and all **814 standard / 841 combined cases** passed on 2026-11-10 with zero failures, skipped cases or automation warnings. The local report is Saved/Automation/Dnd5eBaseItem/index.json.

The new cases cover item identity escaping and reset, conversion through the Blueprint library's C++ entry point, fractional prices and weight, source-qualified selectors and notes, retained equipment input, price presence and rejection of case-folded duplicates. With `-Dnd5eJsonDefinitionFile` supplied, the same local snapshot also exercises all 230 base-item rows.


## Base-item equipment verification

The Development Editor build and all **816 standard / 843 combined cases** passed on 2026-11-10. The local report is Saved/Automation/Dnd5eBaseEquipmentFinal/index.json.

The synthetic cases cover literal and dice damage expressions, both range distances, reload count, medium armor, shield AC, Strength presence and explicit false stealth. They check that converted fields are consumed, unsupported damage codes and AC overrides remain raw, and a reversed range in a later row discards the entire result.

The existing opt-in base-item audit now checks the typed counts for the inspected 230-row snapshot. It expects 102 weapon categories, 101 primary and 17 alternate damage expressions, 52 ranges, 16 reload counts, 27 AC values, six Strength thresholds and 15 stealth fields.


## Ordinary-item verification

The Development Editor build and all **818 standard / 845 combined cases** passed on 2026-11-10 with zero failures, skipped cases and automation warnings. The local report is Saved/Automation/Dnd5eOrdinaryItemFinal/index.json. The ordinary-item conversion cases use synthetic rows.

The synthetic cases exercise the Blueprint library entry point, selection by source index, omitted type, rarity and damage mappings, single-distance range, readable primary and alternative attunement, and raw index retention. Failure cases check out-of-range indices, both dependency fields and a malformed attunement value. A later valid row can be read independently.

### Structured ordinary-item attunement indexes

The Development Editor build and all **847 combined cases** passed on 2026-11-10, including **820 standard cases** and 27 opt-in audits. The run reported zero failures, not-run cases, or automation warnings. The local report is Saved/Automation/Dnd5eStructuredAttunementFinal/index.json.

The two new synthetic cases cover independent primary and alternative indexes, group boundaries, source-qualified and source-less selectors, explicit Boolean presence, Intelligence, alignment, size, creature type and language. Fallback assertions cover unsupported members, malformed groups and envelopes, scalar fragment serialization, escaped diagnostic paths and empty versus absent indexes. The existing ordinary-item conversion test now verifies that readable conditions and a typed background index coexist. No external ordinary-item conversion audit runs as part of this suite.

### Direct base-item links

The Development Editor build and all **850 combined cases** passed on 2026-11-10, including **823 standard cases** and 27 opt-in audits. The run reported zero failed, not-run, or warning cases. The local report is Saved/Automation/Dnd5eBaseItemReference/index.json.

Three synthetic cases cover canonical normalization against base rows, explicit publication, malformed or self links, and reflected Blueprint lookup. The lookup tests cover unique, missing, cross-namespace and duplicate matches, default output on failure, and output value isolation. The existing item schema and copy/serialization tests cover the two reflected fields. The base-item link cases use synthetic documents.

### Modifier-free ordinary-item copy assembly

The Development Editor build and all **854 combined cases** passed on 2026-11-10, including **826 standard cases** and 28 opt-in audits. The report under Saved/Automation/Dnd5eItemCopyAssembly records zero failures, not-run cases or automation warnings.

Three synthetic cases cover the preview and typed handoff, child-to-ancestor origins, whole-field overrides, null removal, explicit null retention, preserve rules, malformed or unsupported directives, missing or ambiguous targets, cycles and the 32-hop limit. The preview result's Blueprint access flags and callable node are also verified.

Pass `-Dnd5eJsonItemCopyFile="<absolute-path-to-items.json>"` to enable the inspected-snapshot audit. It checks all 13 modifier-free copy chains among the snapshot's 67 declared copies. The opt-in test reads the supplied file locally and exercises raw assembly. The ordinary reader's typed handoff is covered by the synthetic case.

### Ordinary-item copy modifiers

The Development Editor build and all **856 combined cases** passed on 2026-11-10, including **828 standard cases** and 28 opt-in audits. The report under Saved/Automation/Dnd5eItemCopyModifiersFinal records zero failures, not-run cases or automation warnings.

Two synthetic tests verify all four supported operations, directive and dependency order, the -1 append sentinel, insertion clamping, exact property comparison, array creation, origin updates, and atomic failure. The existing `-Dnd5eJsonItemCopyFile` audit now exercises all 67 declared copies, including the 54 with modifiers. The audit checks raw assembly against the supplied snapshot. Typed conversion of a modified preview is covered by the synthetic test.

### Direct typed item copy reading

The Development Editor build and all **858 combined cases** passed on 2026-11-10, including **830 standard cases** and 28 opt-in audits. The report under Saved/Automation/Dnd5eTypedItemCopyReader records zero failures, not-run cases or automation warnings.

Two synthetic cases exercise Blueprint access, direct conversion, nested Content source overrides, original and escaped JSON pointers, inherited page and membership attribution, modifier anchors, and atomic failures at inherited values or modifier locations. The existing `-Dnd5eJsonItemCopyFile` audit verifies both raw assembly and direct typed conversion for all 67 declared copies in the supplied snapshot. It checks the original selected row and lineage consistency. The spell suite also exercises the shared Content and diagnostic helpers with ordinary provenance.

### Magic variant matching

The Development Editor build and all **861 combined cases** passed on 2026-11-10, including **832 standard cases** and 29 opt-in audits. The report under Saved/Automation/Dnd5eItemVariantMatcher records zero failures, not-run cases or automation warnings.

Two synthetic tests cover Blueprint access, exact predicate types and casing, alternative requirement groups, independent exclusions, edition policies, bundled bases, and file-specific diagnostics with atomic failure. Pass `-Dnd5eJsonVariantFile="<absolute-path-to-magicvariants.json>"` and `-Dnd5eJsonDefinitionFile="<absolute-path-to-items-base.json>"` to audit the inspected local snapshot. Its 225 standalone templates and 230 base rows produce 3,508 eligible pairs under SourceDefault and 2,873 under PreferClassic. The audit counts the five template copies separately.

### Specific magic item variant generation

The Development Editor build and all **863 combined cases** passed on 2026-11-10, including **834 standard cases** and 29 opt-in audits. The report under Saved/Automation/Dnd5eItemVariantGeneration records zero failures, not-run cases or automation warnings.

Two synthetic tests cover generated name and pair identity, base cleanup, literal entries, property notes, linked loot, the ordinary reader handoff, original row locations, and atomic failures. The snapshot audit selects the first eligible base under SourceDefault for each of the 221 standalone templates that has a match. It verifies 130 generated previews and 91 located deferrals. Four standalone templates have no eligible base under that policy. This audit samples one eligible pair per template.

### Magic item variant entry substitutions

The Development Editor build and all **865 combined cases** passed on 2026-11-10, including **836 standard cases** and 29 opt-in audits. The report under Saved/Automation/Dnd5eItemVariantEntries records zero failures, not-run cases or automation warnings.

Two synthetic tests verify nested entry substitution, original base names, exact article and damage display spelling, existing markup, numeric bonus literals, unchanged base entries and the typed preview handoff. Failure checks cover unknown tokens, modifiers, incomplete syntax, missing inputs and the per-string substitution limit. The existing snapshot audit now verifies 189 generated previews and 32 expected deferrals for the first eligible base under SourceDefault for each of the 221 matching standalone templates.
