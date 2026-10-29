# Automated tests

The current runtime API is tested by the Dnd5eEditor module under the Dnd5e automation prefix. The suite uses Unreal's C++ automation framework, editor context, product filter, and WITH_DEV_AUTOMATION_TESTS guard. Tests are not part of the packaged runtime module. Runtime models and helpers are exercised across the module boundary.

Verification on 2026-10-29, Unreal Engine 5.8: the editor target built successfully, then a fresh unattended worker discovered and passed **129 tests**, with zero failed/not-run tests and zero test warnings or errors. This includes the two original regressions, 45 model-contract cases, 45 model copy/serialization cases, 12 enum cases, 17 wrapper-extraction cases, inner dice extraction, Blueprint API reflection, three tag cases, two composed-spell cases, and numeric/presence boundaries. The host had 22 configured legacy leaf redirects; its redirect case checked them through text import. Compilation alone is not the passing-run evidence.

## Coverage

| Test group | Contract exercised |
| --- | --- |
| Dnd5e.Model.Contract | Every exported model: reflected field names/types, complete array element types, Blueprint visibility/editability, runtime module ownership, and default/presence semantics |
| Dnd5e.Model.CopyAndSerialization | Every model: populated reflected storage, ordinary value-copy isolation, and binary reflected serialization/load round trips, including Unicode, nested arrays, and instanced payloads |
| Dnd5e.Model.EnumContract | All public enum names, member identities/order, and Blueprint exposure |
| Dnd5e.Content.Extraction | Every wrapper extractor: correct payload preservation, independent copy-out, complete output reset, read-only failure, empty payloads, invalid/wrong/parent/child kinds, and all other public struct payload types |
| Dnd5e.Content.Annotation.DiceExpression | DiceRoll/DamageRoll inner values, syntax/expression preservation, independent copy-out, output reset, invalid kinds/storage/types, and separation of outer annotation from inner value |
| Dnd5e.Blueprint.ExtractionAPI | All helper functions retain their reflected static/callable/pure flags, const struct input, concrete output, and bool success return |
| Dnd5e.Tags.NativeVocabulary | Fixed public native tag strings, dictionary registration, uniqueness, root spelling, and absence of obsolete duplicate identities |
| Dnd5e.Tags.HierarchyAndRoles | Intended map ancestry; action/bonus-action siblings; shared timing, attack mode, school, and concentration scope; separate range/origin identities |
| Dnd5e.Tags.ConfiguredRedirects | Any host-configured Dnd5e leaf redirects reach registered current names through reflected text import; a fresh host may have no legacy redirects |
| Dnd5e.Spell | A composed synthetic spell: nested Content, source-order timing/duration alternatives, per-entry concentration, distinct classification roles, mixed-denomination costs, provenance, normalized/unresolved relationships, reflected round trip, and deep copy isolation |
| Dnd5e.Values.PresenceAndNumericBoundaries | int32 endpoints, explicit zero versus unspecified, double precision/literal preservation, numeric versus textual page references, and absent versus explicit zero cost |
| Dnd5e.Content.Section.OutputReset / Dnd5e.Content.CopyIndependence | Original focused regression tests, retained and rerun with the full suite |

The fixed model and tag manifests are compiled test expectations, reviewed against the documented API. They are not generated from runtime reflection during test execution and do not read source headers or a dataset. Intentional public API changes require reviewing/updating the expectations alongside the model documentation. Type-directed checks also link every public struct from the editor module, helping catch missing runtime exports.

Storage probes deliberately populate every reflected field, including inactive optional fields, to detect dropped data. They are serialization probes, not validated D&D definitions. The separately authored synthetic spell exercises contextual composition. Neither probe pretends that an extraction helper validates a rule: correctly typed default payloads remain extractable under the documented contract.

The extraction tests load development-only child tags from temporary INI sources under the host Saved/Automation directory, then delete the generated files. Editor modules cannot declare native gameplay tags, so these fixtures are not native declarations or persistent project-config edits. Their in-memory dictionary nodes may remain until the editor worker exits/restarts. They prove that hierarchical descendants cannot bypass exact discriminator matching. The 121 documented exported native identities still belong to the runtime module.

## Running the suite

Build the development host's Editor target first. Open that host project, go to Session Frontend's Automation tab, enable the Product/Project filter, select Dnd5e, and run the selected tests. Play In Editor is not required. A Functional Testing Editor shortcut is optional; the C++ suite uses the automation framework directly.

For an unattended run, invoke the engine's UnrealEditor-Cmd executable with your host .uproject and these arguments:

```text
-unattended -nop4 -nosplash -nullrhi -nosound -NoLiveCoding
-ExecCmds="Automation RunTests Dnd5e."
-TestExit="Automation Test Queue Empty"
-ReportExportPath="<host>/Saved/Automation/Dnd5eFullSuite"
```

Use the actual absolute report directory and quote paths containing spaces. The host can run without external datasets or media; all fixtures are original synthetic values. No test downloads content, resolves a real catalog record, or opens a fixture URL/media path. Binary archive tests are in-memory round trips, not a new plugin import/export API or a promise of compatibility with future saved formats.

Inspect the report's test list, success/failure counts, and errors; an editor process starting or exiting is insufficient evidence. Logs and exported reports belong under the host's Saved directory, outside the plugin repository. Native tags/configuration should be exercised in a fresh editor worker after a rebuild, rather than relying on an old Live Coding session.

## Test files and assertions

All new sources live under Source/Dnd5eEditor/Private/Tests and are guarded by WITH_DEV_AUTOMATION_TESTS. Dnd5eEditor.Build.cs declares the Engine and GameplayTags dependencies used directly by the tests; no runtime-module dependencies or production code changed.

| File | Responsibility |
| --- | --- |
| Dnd5eTestSupport.h/.cpp | Shared in-memory fixtures, recursive default checks, storage probes, one-leaf mutation, and archive round-trip helpers |
| Dnd5eModelContracts.inl | Frozen expectations for all 45 model schemas and 202 properties |
| Dnd5eModelTests.cpp | Parameterized model/default/Blueprint-reflection cases, copies/serialization, and 12 enum contracts |
| Dnd5eExtractionTests.cpp | Parameterized extraction failure matrix, inner dice values, transient child-tag fixture loading, and reflected Blueprint function signatures |
| Dnd5eNativeTagContracts.inl | Frozen expected symbol/string pairs for all 121 exported native identities |
| Dnd5eTagTests.cpp | Native registration, uniqueness, hierarchy/role checks, and configured legacy text-import redirects |
| Dnd5eSpellTests.cpp | Original synthetic composed spell, nested copy isolation, contextual role preservation, and numeric/presence boundaries |
| Dnd5eContentLibraryTests.cpp | Original two regression tests, retained unchanged |

IMPLEMENT_COMPLEX_AUTOMATION_TEST registers a group whose GetTests method lists named cases; RunTest receives the selected case name. IMPLEMENT_SIMPLE_AUTOMATION_TEST registers one scenario directly. Assertions record expected versus actual behavior in the report. A failed essential setup check returns early before unsafe access; a passing case returns without recorded errors. The fixtures construct inputs, production functions perform the operation, and assertions check the observable contract. None of the helpers is a new runtime plugin API.

To maintain coverage, review/update the model/tag manifests when intentionally changing the public schema or tag vocabulary. Add a behavioral scenario when a new importer, resolver, validator, or other capability exists. Do not regenerate expected results from live runtime reflection to make a failing contract test pass.

## Current limits

Tests cover implemented storage, reflection, extraction, native identities, and the composed value contract. JSON parsing, ID normalization, catalog loading/resolution, recursive semantic validation, complete rule interpretation, media loading, and effect execution are unimplemented and are not certified by these tests. Blueprint reflection checks do not replace an end-to-end Blueprint graph/editor UI test. Tag text-import migration checks do not establish migration of every existing binary Blueprint asset or parent-tag query. No compatibility with another engine version or a historical binary archive format is implied.

The Dnd5eJson and editor integration modules currently have scaffolding rather than an importer API to test. Add behavioral tests when those capabilities are implemented; do not manufacture tests for hypothetical systems or require a real dataset merely to run the present suite.
