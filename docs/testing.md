# Automated tests

The current runtime API is tested by the Dnd5eEditor module under the Dnd5e automation prefix. The suite uses Unreal's C++ automation framework, editor context, product filter, and WITH_DEV_AUTOMATION_TESTS guard. Tests are not part of the packaged runtime module. Runtime models and helpers are exercised across the module boundary.

Verification on 2026-10-29, Unreal Engine 5.8: the editor target builds and the standard suite contains **257 passing cases**. It covers 161 storage/API cases, 82 [JSON spell reader](json-spell-reader.md) cases, nine [availability/catalog](spell-catalog.md) scenarios, and five [Blueprint catalog](spell-catalog.md#blueprint-ownership-and-operations) scenarios. The models remain 51 core structs and eight JSON-owned structs, with 264 reflected fields, 14 enums, 129 native tags, and 18 Content helpers. The wrapper batch adds two exported UObject classes and six reflected operations. The host's 22 configured legacy leaf redirects are checked through text import. Compilation alone is not passing-run evidence.

## Coverage

The combined verification run passed **276 cases**: all 257 standard cases, 18 opt-in file cases converting **969 records**, and one catalog integration case joining **969 lookup entries / 14,448 associations**. There were zero failed/not-run cases and no automation test warnings/errors. The primary reader returned **2,556 diagnostic warnings** across the external records for retained/unresolved content, references, and presentation; these are inspected API data, not silent test warnings or proof of complete semantics.

| Test group | Contract exercised |
| --- | --- |
| Dnd5e.Model.Contract | Every exported model: reflected field names/types, complete array element types, Blueprint visibility/editability, runtime module ownership, and default/presence semantics |
| Dnd5e.Model.CopyAndSerialization | Every model: populated reflected storage, ordinary value-copy isolation, and binary reflected serialization/load round trips, including Unicode, nested arrays, and instanced payloads |
| Dnd5e.Model.EnumContract | All public enum names, member identities/order, and Blueprint exposure |
| Dnd5e.Content.Extraction | Every wrapper extractor: correct payload preservation, independent copy-out, complete output reset, read-only failure, empty payloads, invalid/wrong/parent/child kinds, and all other public struct payload types |
| Dnd5e.Content.Annotation.DiceExpression | DiceRoll/DamageRoll inner values, syntax/expression preservation, independent copy-out, output reset, invalid kinds/storage/types, and separation of outer annotation from inner value |
| Dnd5e.Json.Contract | Supplied text/context/index preservation; independent diagnostic/result ownership; result round trips; reflected replacement of success/failure values without stale payloads |
| Dnd5e.Json.Reader | Actual ReadSpell calls: complete synthetic mapping, strict field/type/presence and vocabulary handling, numeric token/range checks, Unicode/escaped identity, immutable requests, fresh results, source locations, metadata, fallback, unresolved reprints, concentration association, duplicates, and input/depth/value limits |
| Dnd5e.Catalog | Exact C++/reflected ID equality and hashing, stable filtered queries, independent copy-out/reset, atomic replacement, variant/qualification filters, and incomplete availability diagnostics |
| Dnd5e.Json.Availability | All nine provider branches with synthetic qualifiers/context, source pointers, missing versus known-empty data, immutable requests, round trips, malformed metadata, and atomic failure |
| Dnd5e.Json.Catalog | Multi-document conversion and lookup joins, namespace boundaries, duplicate IDs, explicit clearing, inactive optional values, later failures, and document limits |
| Dnd5e.Json.ExternalCatalog | Optional supplied-directory/lookup build, all-record count, association totals, and unfiltered query; discovered only with both explicit path arguments |
| Dnd5e.Json.External | Optional caller-selected local-file smoke test; discovered only when an explicit Dnd5eJsonSmokeFile argument is supplied, outside the default synthetic suite |
| Dnd5e.Json.ExternalDataset | Optional explicit-directory audit: one case per spells-*.json file, converting every selected record and reporting counts/codes/locations without source passages |
| Dnd5e.Blueprint.ExtractionAPI | All helper functions retain their reflected static/callable/pure flags, const struct input, concrete output, and bool success return |
| Dnd5e.Blueprint.SpellCatalog.Reflection | Both classes' module/base/type flags; all six function signatures, complete array element types, const/static/pure flags, output directions, and native-only mutable access |
| Dnd5e.Blueprint.SpellCatalog.BuildQueryAndCopyOut | ProcessEvent invocation of build/count/query/get using initialized reflected parameter frames; typed Content/annotations, immutable requests, exact lookup/copy-out/reset, and level versus publication constraints |
| Dnd5e.Blueprint.SpellCatalog.AtomicFailureAndDiagnostics | Invalid/null targets, fixed diagnostic codes/locations, failed JSON/core replacement preserving old records, and explicit clearing |
| Dnd5e.Blueprint.SpellCatalog.AvailabilityWarnings | Missing availability warning, supplied lookup join, provider query, and preservation warnings returned unchanged |
| Dnd5e.Blueprint.SpellCatalog.OwnershipAndGarbageCollection | Separate initially empty catalogs, independent ID copies, survival under a strong reference, and collection after that reference is released |
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

For an optional local compatibility check, select Dnd5e.Json.External and additionally pass `-Dnd5eJsonSmokeFile="<absolute-user-supplied-json-file>" -Dnd5eJsonSmokeIndex=<explicit-index>`. The harness loads the chosen file and passes its text to the runtime reader; the reader itself still performs no I/O. This case is discovered only when the file argument is present and is outside the default 257 cases. It checks acceptance, identity presence, and selected-record provenance without logging passages. Keep the file and reports outside the repository.

To audit every spell in explicitly selected files, pass `-Dnd5eJsonSmokeDirectory="<absolute-user-supplied-spells-directory>"` and select Dnd5e.Json.ExternalDataset, or the full Dnd5e prefix to combine the audit with the standard suite. The harness discovers spells-*.json under that directory and calls ReadSpell with every explicit index. It registers a failing case if the supplied directory has no matching files. Source acceptance includes explicit fallback/reference warnings; it does not establish typed semantics for every retained token. The default suite does not discover/read this directory without that argument.

Inspect the report's test list, success/failure counts, and errors; an editor process starting or exiting is insufficient evidence. Logs and exported reports belong under the host's Saved directory, outside the plugin repository. Native tags/configuration should be exercised in a fresh editor worker after a rebuild, rather than relying on an old Live Coding session.

For the catalog integration case, supply both `-Dnd5eJsonSmokeDirectory="<absolute-user-supplied-spells-directory>"` and `-Dnd5eJsonAvailabilityFile="<absolute-user-supplied-lookup-file>"`, then select Dnd5e.Json.ExternalCatalog or the full prefix. Only the test harness opens these selected files; production APIs receive supplied text. The 2026-10-29 wrapper/full run's report is under the development host's Saved/Automation/Dnd5eBlueprintCatalogVerified, outside Git. The standard suite remains independent of these paths and data.

## Test files and assertions

The optional external smoke case was also run separately on 2026-10-29 against one explicitly selected 2014 PHB spell in the local dataset: one test passed, with zero failed/not-run cases and no test warnings/errors. This verifies that selected record's acceptance under the limited subset, not every record or complete 2014 coverage.

All test sources live under Source/Dnd5eEditor/Private/Tests and are guarded by WITH_DEV_AUTOMATION_TESTS. Dnd5eEditor.Build.cs declares Engine, GameplayTags, and Json for direct test use. Dnd5eJson publicly depends on Core/CoreUObject/Dnd5e/Engine and privately on Json/GameplayTags. Engine supplies the public Blueprint library base; GameplayTags is a direct private dependency because conversion calls AddTag. The developer wrote the original envelopes and Blueprint wrappers; the assistant completed the reader and automated behavior tests under explicit authorization.

| File | Responsibility |
| --- | --- |
| Dnd5eTestSupport.h/.cpp | Shared in-memory fixtures, recursive default checks, storage probes, one-leaf mutation, and archive round-trip helpers |
| Dnd5eModelContracts.inl | Frozen expectations for all 59 model schemas and 264 properties across both runtime modules |
| Dnd5eModelTests.cpp | Parameterized model/default/Blueprint-reflection cases, copies/serialization, and 14 enum contracts |
| Dnd5eExtractionTests.cpp | Parameterized extraction failure matrix, inner dice values, transient child-tag fixture loading, and reflected Blueprint function signatures |
| Dnd5eNativeTagContracts.inl | Frozen expected symbol/string pairs for all 129 exported native identities |
| Dnd5eTagTests.cpp | Native registration, uniqueness, hierarchy/role checks, and configured legacy text-import redirects |
| Dnd5eJsonContractTests.cpp | Cross-module JSON request/result storage tests, diagnostic codes/locations, and value replacement |
| Dnd5eJsonSpellReaderTests.cpp | Synthetic reader scenarios, rejected-field matrix, recursive Content/inline conversion, new scaling values, percentile/sentinel bounds, input/conversion budgets, and optional explicit-file/directory audits |
| Dnd5eSpellCatalogTests.cpp | Exact identity/reflection, core ownership/query behavior, supplementary availability parsing, atomic supplied-document builds, and the opt-in external catalog integration case |
| Dnd5eBlueprintCatalogTests.cpp | Reflected wrapper contracts and ProcessEvent invocation, copy/reset behavior, atomic failure/diagnostics, availability forwarding, independent objects, and garbage-collection lifetime |
| Dnd5eSpellTests.cpp | Original synthetic composed spell, nested copy isolation, contextual role preservation, and numeric/presence boundaries |
| Dnd5eContentLibraryTests.cpp | Original two regression tests, retained unchanged |

IMPLEMENT_COMPLEX_AUTOMATION_TEST registers a group whose GetTests method lists named cases; RunTest receives the selected case name. IMPLEMENT_SIMPLE_AUTOMATION_TEST registers one scenario directly. Assertions record expected versus actual behavior in the report. A failed essential setup check returns early before unsafe access; a passing case returns without recorded errors. The fixtures construct inputs, production functions perform the operation, and assertions check the observable contract. None of the helpers is a new runtime plugin API.

To maintain coverage, review/update the model/tag manifests when intentionally changing the public schema or tag vocabulary. Add a behavioral scenario when a new importer, resolver, validator, or other capability exists. Do not regenerate expected results from live runtime reflection to make a failing contract test pass.

## Current limits

Tests cover storage/reflection/extraction, native identities, composed values, spell/Content conversion, availability joins, native catalogs, and Blueprint-facing wrapper behavior. Required known structures are checked; unmodeled Content is retained explicitly. General reference resolution, complete typed rule interpretation, full media/document coverage, and effect execution remain pending. ProcessEvent/reflection tests exercise the reflected API but do not compile or run an arbitrary Blueprint asset. The user separately verified the host Blueprint flow with 319 source-marked SRD 5.1 records, queried all IDs, and retrieved a spell; that manual result is distinct from the synthetic automated suite. Tag text-import migration checks do not establish migration of every binary Blueprint asset or parent-tag query. No compatibility with another engine version or a historical binary archive format is implied.

Dnd5eJson exposes ReadSpell, ReadAvailability, BuildCatalog, and the reflected BuildSpellCatalog library; module startup/shutdown hooks remain empty because supplied-text conversion requires no startup-owned service. Editor file selection/loading remains pending. The host prototype's explicit file-reading helper is outside the distributable plugin and this runtime suite. Add behavioral tests as loading capabilities are implemented, while keeping the standard suite independent of real datasets.
