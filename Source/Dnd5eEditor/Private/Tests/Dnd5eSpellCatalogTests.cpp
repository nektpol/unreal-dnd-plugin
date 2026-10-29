#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Dnd5eJsonSpellReader.h"
#include "GameplayTagsManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

using namespace Dnd5eTests;

namespace
{
	FDnd5eSpell CatalogSpell(const TCHAR* Key, const TCHAR* Name = TEXT("Invented Spark"), const TCHAR* Source = TEXT("SYN"), const TCHAR* Namespace = TEXT("synthetic"))
	{
		FDnd5eSpell Spell;
		Spell.Header.RecordId.DatasetNamespace = Namespace;
		Spell.Header.RecordId.Category = Dnd5e::Category::Spell;
		Spell.Header.RecordId.SourceKey = Key;
		Spell.Header.DisplayName = Name;
		Spell.Header.Aliases.Add(TEXT("Invented Alias"));
		FDnd5eRecordProvenance Provenance;
		Provenance.SourceId = Source;
		Provenance.RelativeFilePath = TEXT("spells/synthetic.json");
		Provenance.JsonPointer = TEXT("/spell/0");
		Spell.Header.Provenance.Add(Provenance);
		Spell.Level = 1;
		Spell.Schools.AddTag(Dnd5e::Magic::School::Evocation);
		return Spell;
	}

	FDnd5eJsonDocument SpellDocument(const TCHAR* Name = TEXT("Invented Spark"), const TCHAR* Source = TEXT("SYN"))
	{
		const FString Fixture = TEXT(R"json({"spell":[{
			"name":"Invented Spark","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],
			"range":{"type":"point","distance":{"type":"feet","amount":20}},"components":{"v":true},
			"duration":[{"type":"instant"}],"entries":["A small invented light appears."]
		}]})json");
		TSharedPtr<FJsonObject> Root;
		const bool bParsed = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture), Root);
		check(bParsed);
		const auto Spell = Root->GetArrayField(TEXT("spell"))[0]->AsObject();
		Spell->SetStringField(TEXT("name"), Name);
		Spell->SetStringField(TEXT("source"), Source);
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("synthetic");
		Document.Context.RelativeFilePath = TEXT("spells/synthetic.json");
		FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Document.JsonText));
		return Document;
	}

	FDnd5eJsonSpellAvailabilityReadRequest AvailabilityRequest(const FString& Entry = TEXT("{}"))
	{
		FDnd5eJsonSpellAvailabilityReadRequest Request;
		Request.Document.Context.DatasetNamespace = TEXT("synthetic");
		Request.Document.Context.RelativeFilePath = TEXT("generated/lookup.json");
		Request.Document.JsonText = TEXT("{\"syn\":{\"invented spark\":") + Entry + TEXT("}}");
		Request.SpellName = TEXT("Invented Spark");
		Request.SpellSourceId = TEXT("SYN");
		return Request;
	}

	FDnd5eSpellAvailability Association(const TCHAR* Name, bool bVariant = false)
	{
		FDnd5eSpellAvailability Entry;
		Entry.Provider.Category = Dnd5e::Category::Class;
		Entry.Provider.Name = Name;
		Entry.Provider.SourceId = TEXT("SYN");
		Entry.bHasUnqualifiedDefinition = true;
		Entry.bIsVariant = bVariant;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eRecordIdHashTest, "Dnd5e.Catalog.IdentityEqualityAndHash", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eRecordIdHashTest::RunTest(const FString& Parameters)
{
	auto A = CatalogSpell(TEXT("Opaque-A")).Header.RecordId;
	auto B = A; B.SourceKey = TEXT("opaque-a");
	auto C = A; C.DatasetNamespace = TEXT("Synthetic");
	TestTrue(TEXT("A copied identity compares equal"), A == FDnd5eRecordId(A));
	TestEqual(TEXT("Equal IDs hash equally"), GetTypeHash(A), GetTypeHash(FDnd5eRecordId(A)));
	TestTrue(TEXT("Source keys and namespaces use exact case"), A != B && A != C);
	TestFalse(TEXT("Reflected identity also preserves source-key case"), FDnd5eRecordId::StaticStruct()->CompareScriptStruct(&A, &B, 0));
	TestFalse(TEXT("Reflected identity also preserves namespace case"), FDnd5eRecordId::StaticStruct()->CompareScriptStruct(&A, &C, 0));
	TestEqual(TEXT("Reflected hashing uses the same identity contract"), FDnd5eRecordId::StaticStruct()->GetStructTypeHash(&A), GetTypeHash(A));
	TMap<FDnd5eRecordId, int32> Values;
	Values.Add(A, 1); Values.Add(B, 2); Values.Add(C, 3);
	TestEqual(TEXT("Exact IDs do not inherit FString's case-insensitive map identity"), Values.Num(), 3);
	TestEqual(TEXT("Case-distinct key resolves independently"), Values.FindChecked(B), 2);
	auto EmbeddedA = A; EmbeddedA.SourceKey = FString(3, TEXT("A\0x"));
	auto EmbeddedB = A; EmbeddedB.SourceKey = FString(3, TEXT("A\0y"));
	TestTrue(TEXT("Value equality does not stop at an embedded null"), EmbeddedA != EmbeddedB);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCatalogOwnershipTest, "Dnd5e.Catalog.AtomicReplacementAndCopyOut", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCatalogOwnershipTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	auto A = CatalogSpell(TEXT("Opaque-A"));
	auto B = CatalogSpell(TEXT("opaque-a"));
	TestTrue(TEXT("Case-distinct opaque IDs are accepted"), Catalog.ReplaceSpells({A, B}, Diagnostics));
	const auto Before = Catalog.GetSpellIds();
	TestFalse(TEXT("Duplicates fail rather than overwrite"), Catalog.ReplaceSpells({A, A}, Diagnostics));
	TestEqual(TEXT("Duplicate code"), Diagnostics.Last().Code, FName(TEXT("Dnd5e.Catalog.DuplicateRecordId")));
	TestEqual(TEXT("Failed replacement preserves count"), Catalog.GetSpellCount(), 2);
	TestTrue(TEXT("Failed replacement preserves order/IDs"), Catalog.GetSpellIds() == Before);
	FDnd5eSpell Out;
	TestTrue(TEXT("Get by exact ID"), Catalog.TryGetSpell(A.Header.RecordId, Out));
	Out.Header.DisplayName = TEXT("Edited copy");
	Out.Header.Aliases.Reset();
	Catalog.TryGetSpell(A.Header.RecordId, Out);
	TestEqual(TEXT("Editing a returned value preserves catalog storage"), Out.Header.DisplayName, A.Header.DisplayName);
	TestEqual(TEXT("Nested arrays have value ownership"), Out.Header.Aliases.Num(), 1);
	auto Missing = A.Header.RecordId; Missing.SourceKey = TEXT("missing");
	TestFalse(TEXT("Missing ID is an ordinary lookup miss"), Catalog.TryGetSpell(Missing, Out));
	const FDnd5eSpell Default;
	TestTrue(TEXT("A miss resets a previous output"), Equal(FDnd5eSpell::StaticStruct(), &Out, &Default));
	auto Invalid = A; Invalid.Level = -1;
	TestFalse(TEXT("Invalid record does not replace active data"), Catalog.ReplaceSpells({Invalid}, Diagnostics));
	TestEqual(TEXT("Old data remains available"), Catalog.GetSpellCount(), 2);
	TestTrue(TEXT("An explicitly empty replacement clears the catalog"), Catalog.ReplaceSpells({}, Diagnostics));
	TestEqual(TEXT("Empty catalog has no stale entries"), Catalog.GetSpellCount(), 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCatalogQueriesTest, "Dnd5e.Catalog.FilteredQueries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCatalogQueriesTest::RunTest(const FString& Parameters)
{
	auto A = CatalogSpell(TEXT("A"));
	auto B = CatalogSpell(TEXT("B"), TEXT("Invented Spark"), TEXT("SYN2"));
	B.Level = 2;
	B.Header.Aliases.Reset();
	A.Traits.AddTag(Dnd5e::Spell::Trait::Ritual);
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Install synthetic records"), Catalog.ReplaceSpells({A, B}, Diagnostics));
	FDnd5eSpellQuery Query;
	TArray<FDnd5eRecordId> Ids;
	TestTrue(TEXT("A valid query succeeds"), Catalog.QuerySpellIds(Query, Ids, Diagnostics));
	TestEqual(TEXT("Empty filters select the loaded catalog"), Ids.Num(), 2);
	Query.Name = TEXT("invented spark");
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestEqual(TEXT("A name match returns all sources without edition guessing"), Ids.Num(), 2);
	Query.SourceId = TEXT("SYN2");
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestTrue(TEXT("Primary source filters explicitly"), Ids.Num() == 1 && Ids[0] == B.Header.RecordId);
	Query = FDnd5eSpellQuery{};
	Query.Name = TEXT("invented alias");
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestTrue(TEXT("Optional aliases participate"), Ids.Num() == 1 && Ids[0] == A.Header.RecordId);
	Query.bIncludeAliases = false;
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestEqual(TEXT("Aliases can be excluded"), Ids.Num(), 0);
	Query = FDnd5eSpellQuery{};
	Query.Schools.AddTag(UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Dnd5e.Magic.School")));
	Query.Traits.AddTag(Dnd5e::Spell::Trait::Ritual);
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestTrue(TEXT("Tag filters support parent matching and all requested roles"), Ids.Num() == 1 && Ids[0] == A.Header.RecordId);
	Query.Level = 10;
	TestFalse(TEXT("An invalid level is a failed query, not a successful miss"), Catalog.QuerySpellIds(Query, Ids, Diagnostics));
	TestEqual(TEXT("A failed query resets previous IDs"), Ids.Num(), 0);
	TestEqual(TEXT("Invalid query code"), Diagnostics.Last().Code, FName(TEXT("Dnd5e.Catalog.InvalidQuery")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAvailabilityReaderTest, "Dnd5e.Json.Availability.ProviderFamilies", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAvailabilityReaderTest::RunTest(const FString& Parameters)
{
	auto Request = AvailabilityRequest(TEXT(R"json({
		"class":{"SYN":{"Clock Adept":true}},
		"classVariant":{"SYN":{"Clock Adept":{"definedInSources":[null,"SYNEXP"]}}},
		"subclass":{"SYN":{"Clock Adept":{"SYN2":{"Hands":{"name":"Clock Hands","subSubclasses":["Copper","Silver"]}}}}},
		"race":{"SYN":{"Clockfolk":{"baseName":"Clockkin","baseSource":"SYNBASE"}}},
		"optionalfeature":{"SYN":{"Clock Option":{"featureType":["INV"]}}},
		"background":{"SYN":{"Clock Keeper":true}},"feat":{"SYN":{"Clock Gift":true}},
		"charoption":{"SYN":{"Clock Choice":true}},"reward":{"SYN":{"Clock Reward":true}}
	})json"));
	const auto Before = Request;
	const auto Result = FDnd5eJsonSpellAvailabilityReader::ReadAvailability(Request);
	if (!TestTrue(TEXT("All provider families are mapped"), Result.Status == EDnd5eJsonReadStatus::Success && Result.bHasEntry)) { return false; }
	if (!TestEqual(TEXT("One association per source leaf"), Result.Availability.Num(), 9)) { return false; }
	TestEqual(TEXT("No warning is invented for unresolved provider record identities"), Result.Diagnostics.Num(), 0);
	const auto& Variant = Result.Availability[1];
	TestTrue(TEXT("Variant qualification is preserved"), Variant.bIsVariant && Variant.bHasUnqualifiedDefinition && Variant.DefinedInSources.Num() == 1);
	TestEqual(TEXT("Defining publication retained"), Variant.DefinedInSources[0], FString(TEXT("SYNEXP")));
	const auto& Subclass = Result.Availability[2];
	TestTrue(TEXT("Subclass category"), Subclass.Provider.Category == Dnd5e::Category::Subclass);
	TestEqual(TEXT("Full subclass name"), Subclass.Provider.Name, FString(TEXT("Clock Hands")));
	TestEqual(TEXT("Short lookup name"), Subclass.Provider.ShortName, FString(TEXT("Hands")));
	TestEqual(TEXT("Parent class identity is retained"), Subclass.Provider.ClassName, FString(TEXT("Clock Adept")));
	TestEqual(TEXT("Subgroup choices retained"), Subclass.Subgroups.Num(), 2);
	TestTrue(TEXT("Race source branch maps to the species category"), Result.Availability[3].Provider.Category == Dnd5e::Category::Species);
	TestEqual(TEXT("Base-species identity context"), Result.Availability[3].Provider.BaseSourceId, FString(TEXT("SYNBASE")));
	TestEqual(TEXT("Feature-type code retained without dynamic tags"), Result.Availability[4].FeatureTypes[0], FString(TEXT("INV")));
	TestEqual(TEXT("Actual folded source/name keys form the pointer"), Result.Availability[0].Provenance.JsonPointer, FString(TEXT("/syn/invented spark/class/SYN/Clock Adept")));
	TestTrue(TEXT("Generated data does not invent a publication"), Result.Availability[0].Provenance.SourceId.IsEmpty());
	TestTrue(TEXT("Reading is immutable"), Equal(FDnd5eJsonSpellAvailabilityReadRequest::StaticStruct(), &Request, &Before));
	FDnd5eJsonSpellAvailabilityReadResult Restored;
	RoundTrip(*this, FDnd5eJsonSpellAvailabilityReadResult::StaticStruct(), &Result, &Restored);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAvailabilityPresenceTest, "Dnd5e.Json.Availability.PresenceAndAtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAvailabilityPresenceTest::RunTest(const FString& Parameters)
{
	auto Request = AvailabilityRequest();
	auto Result = FDnd5eJsonSpellAvailabilityReader::ReadAvailability(Request);
	TestTrue(TEXT("An explicit empty entry is known empty"), Result.Status == EDnd5eJsonReadStatus::Success && Result.bHasEntry && Result.Availability.IsEmpty());
	Request.Document.JsonText = TEXT("{}");
	Result = FDnd5eJsonSpellAvailabilityReader::ReadAvailability(Request);
	TestTrue(TEXT("Missing entry is distinct from known empty"), Result.Status == EDnd5eJsonReadStatus::Success && !Result.bHasEntry && Result.Availability.IsEmpty());
	TestTrue(TEXT("The missing-data note is explicit"), Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Info && Result.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.AvailabilityEntryNotFound")));
	Request = AvailabilityRequest(TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":true}},\"reward\":{\"SYN\":{\"Late Failure\":false}}}"));
	Result = FDnd5eJsonSpellAvailabilityReader::ReadAvailability(Request);
	TestTrue(TEXT("A late malformed leaf resets the whole result"), Result.Status == EDnd5eJsonReadStatus::Failure && !Result.bHasEntry && Result.Availability.IsEmpty());
	TestEqual(TEXT("Precise leaf location"), Result.Diagnostics.Last().JsonPointer, FString(TEXT("/syn/invented spark/reward/SYN/Late Failure")));
	Request = AvailabilityRequest(TEXT("{\"class\":{\"SYN\":{\"A/B~C\":true}}}"));
	Result = FDnd5eJsonSpellAvailabilityReader::ReadAvailability(Request);
	if (TestTrue(TEXT("Source selector punctuation is accepted"), Result.Status == EDnd5eJsonReadStatus::Success))
	{
		TestEqual(TEXT("Pointer escaping"), Result.Availability[0].Provenance.JsonPointer, FString(TEXT("/syn/invented spark/class/SYN/A~1B~0C")));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAvailabilityValidationTest, "Dnd5e.Json.Availability.StrictValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAvailabilityValidationTest::RunTest(const FString& Parameters)
{
	struct FCase { const TCHAR* Entry; const TCHAR* Code; };
	const FCase Cases[] = {
		{TEXT("{\"future\":{}}"), TEXT("UnsupportedRequiredValue")},
		{TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":false}}}"), TEXT("InvalidFieldValue")},
		{TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":{\"definedInSources\":[]}}}}"), TEXT("InvalidFieldValue")},
		{TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":{\"definedInSources\":[3]}}}}"), TEXT("InvalidFieldValue")},
		{TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":{\"definedInSource\":\"SYN\",\"definedInSources\":[]}}}}"), TEXT("InvalidFieldValue")},
		{TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":{\"future\":true}}}}"), TEXT("UnsupportedRequiredValue")},
		{TEXT("{\"subclass\":{\"SYN\":{\"Clock Adept\":{\"SYN2\":{\"Hands\":true}}}}}"), TEXT("InvalidFieldValue")},
		{TEXT("{\"subclass\":{\"SYN\":{\"Clock Adept\":{\"SYN2\":{\"Hands\":{}}}}}}"), TEXT("MissingRequiredField")},
		{TEXT("{\"race\":{\"SYN\":{\"Clockfolk\":{\"baseName\":\"Clockkin\"}}}}"), TEXT("MissingRequiredField")},
		{TEXT("{\"optionalfeature\":{\"SYN\":{\"Clock Option\":{\"featureType\":null}}}}"), TEXT("InvalidFieldValue")},
		{TEXT("{\"feat\":{\"SYN\":{\" Clock Gift\":true}}}"), TEXT("InvalidFieldValue")}
	};
	for (const auto& Case : Cases)
	{
		const auto Result = FDnd5eJsonSpellAvailabilityReader::ReadAvailability(AvailabilityRequest(Case.Entry));
		if (!TestTrue(TEXT("Malformed association fails atomically"), Result.Status == EDnd5eJsonReadStatus::Failure && Result.Availability.IsEmpty() && !Result.bHasEntry)) { continue; }
		TestEqual(TEXT("One fail-fast diagnostic"), Result.Diagnostics.Num(), 1);
		TestEqual(TEXT("Controlled error code"), Result.Diagnostics.Last().Code.ToString(), FString(TEXT("Dnd5e.Json.")) + Case.Code);
	}
	auto Duplicate = AvailabilityRequest();
	Duplicate.Document.JsonText = TEXT("{\"syn\":{},\"SYN\":{}}");
	const auto Result = FDnd5eJsonSpellAvailabilityReader::ReadAvailability(Duplicate);
	TestTrue(TEXT("Case-colliding lookup objects fail before map construction"), Result.Status == EDnd5eJsonReadStatus::Failure && Result.Diagnostics.Last().Code == FName(TEXT("Dnd5e.Json.DuplicateProperty")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAvailabilityQueriesTest, "Dnd5e.Catalog.AvailabilityQueriesAndUnknownData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAvailabilityQueriesTest::RunTest(const FString& Parameters)
{
	auto A = CatalogSpell(TEXT("A"));
	A.bHasAvailabilityData = true;
	A.Availability.Add(Association(TEXT("Clock Adept"), true));
	A.Availability[0].DefinedInSources.Add(TEXT("SYNEXP"));
	auto B = CatalogSpell(TEXT("B"));
	B.bHasAvailabilityData = true;
	auto C = CatalogSpell(TEXT("C"));
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	Catalog.ReplaceSpells({A, B, C}, Diagnostics);
	FDnd5eSpellQuery Query;
	Query.AvailabilityProvider.Category = Dnd5e::Category::Class;
	Query.AvailabilityProvider.Name = TEXT("clock adept");
	Query.AvailabilityProvider.SourceId = TEXT("SYN");
	TArray<FDnd5eRecordId> Ids;
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestTrue(TEXT("Known matching availability is returned"), Ids.Num() == 1 && Ids[0] == A.Header.RecordId);
	TestTrue(TEXT("Unknown data is not falsely reported as no availability"), Diagnostics.Num() == 1 && Diagnostics[0].Code == FName(TEXT("Dnd5e.Catalog.IncompleteAvailability")));
	Query.bIncludeVariantAvailability = false;
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestEqual(TEXT("Variants can be excluded"), Ids.Num(), 0);
	Query.bIncludeVariantAvailability = true;
	Query.AvailabilityDefinedInSource = TEXT("SYNEXP");
	Query.Name = A.Header.DisplayName;
	Query.DatasetNamespace = TEXT("synthetic");
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestEqual(TEXT("Definition-source constraints are queryable"), Ids.Num(), 1);
	Query = FDnd5eSpellQuery{};
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestEqual(TEXT("Ordinary queries do not require availability"), Ids.Num(), 3);
	TestEqual(TEXT("Ordinary queries need no incomplete-availability warning"), Diagnostics.Num(), 0);
	A.bHasAvailabilityData = false;
	TestFalse(TEXT("Links cannot masquerade as unsupplied data"), Catalog.ReplaceSpells({A}, Diagnostics));
	TestEqual(TEXT("The previous catalog survives invalid presence"), Catalog.GetSpellCount(), 3);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCatalogBuilderTest, "Dnd5e.Json.Catalog.BuildJoinAndAtomicReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCatalogBuilderTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonSpellCatalogBuildRequest Request;
	Request.SpellDocuments = {SpellDocument(), SpellDocument(TEXT("Other Invented Spark"), TEXT("SYN2"))};
	Request.bHasAvailabilityLookup = true;
	Request.AvailabilityLookup = AvailabilityRequest(TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":true}}}")).Document;
	FDnd5eSpellCatalog Catalog;
	auto Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	if (!TestTrue(TEXT("Multiple documents plus lookup join succeed"), Result.Status == EDnd5eJsonReadStatus::Success && Result.SpellCount == 2 && Catalog.GetSpellCount() == 2)) { return false; }
	FDnd5eSpell First, Second;
	const auto Ids = Catalog.GetSpellIds();
	Catalog.TryGetSpell(Ids[0], First); Catalog.TryGetSpell(Ids[1], Second);
	TestTrue(TEXT("Matched entry populates availability"), First.bHasAvailabilityData && First.Availability.Num() == 1);
	TestTrue(TEXT("Missing join stays unknown"), !Second.bHasAvailabilityData && Second.Availability.IsEmpty());
	TestTrue(TEXT("Missing join is diagnosed"), Result.Diagnostics.ContainsByPredicate([](const auto& Diagnostic) { return Diagnostic.Code == FName(TEXT("Dnd5e.Json.AvailabilityEntryNotFound")); }));
	const auto Before = Catalog.GetSpellIds();
	Request.SpellDocuments = {SpellDocument(), SpellDocument()};
	Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("Duplicate IDs fail the complete replacement"), Result.Status == EDnd5eJsonReadStatus::Failure && Result.SpellCount == 0);
	TestTrue(TEXT("Failure preserves the active snapshot"), Catalog.GetSpellIds() == Before);
	Request.SpellDocuments = {SpellDocument()};
	Request.AvailabilityLookup.Context.DatasetNamespace = TEXT("different");
	Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("Cross-namespace lookup joins fail explicitly"), Result.Status == EDnd5eJsonReadStatus::Failure && Result.Diagnostics.Last().Code == FName(TEXT("Dnd5e.Json.AvailabilityNamespaceMismatch")));
	Request.bHasAvailabilityLookup = false;
	Request.AvailabilityLookup.JsonText = TEXT("ignored invalid optional text");
	Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("Absent optional lookup is ignored, not guessed"), Result.Status == EDnd5eJsonReadStatus::Success && Catalog.GetSpellCount() == 1);
	Catalog.TryGetSpell(Catalog.GetSpellIds()[0], First);
	TestFalse(TEXT("Primary-only builds mark availability unknown"), First.bHasAvailabilityData);
	Request.SpellDocuments.Reset();
	Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("An empty request cannot accidentally clear active data"), Result.Status == EDnd5eJsonReadStatus::Failure && Catalog.GetSpellCount() == 1);
	auto Empty = SpellDocument(); Empty.JsonText = TEXT("{\"spell\":[]}");
	Request.SpellDocuments.Add(Empty);
	Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("An explicit empty document can replace with an empty catalog"), Result.Status == EDnd5eJsonReadStatus::Success && Catalog.GetSpellCount() == 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCatalogBuilderValidationTest, "Dnd5e.Json.Catalog.ValidationAndLimits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCatalogBuilderValidationTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	const auto Original = CatalogSpell(TEXT("original"));
	Catalog.ReplaceSpells({Original}, Diagnostics);
	FDnd5eJsonSpellCatalogBuildRequest Request;
	auto Bad = SpellDocument(); Bad.JsonText = TEXT("{\"spell\":[null]}");
	Request.SpellDocuments = {SpellDocument(), Bad};
	auto Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("A bad later document preserves active data"), Result.Status == EDnd5eJsonReadStatus::Failure && Catalog.GetSpellCount() == 1);
	FDnd5eSpell Out;
	TestTrue(TEXT("The original ID remains"), Catalog.TryGetSpell(Original.Header.RecordId, Out));
	Request.SpellDocuments.SetNum(1025);
	Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("Document count has an explicit bound"), Result.Status == EDnd5eJsonReadStatus::Failure && Result.Diagnostics.Last().Code == FName(TEXT("Dnd5e.Json.ResourceLimitExceeded")));
	Request.SpellDocuments = {SpellDocument()};
	Request.bHasAvailabilityLookup = true;
	Request.AvailabilityLookup = AvailabilityRequest(TEXT("{\"class\":{\"SYN\":{\"Clock Adept\":false}}}")).Document;
	Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	TestTrue(TEXT("A malformed lookup cannot partially replace the catalog"), Result.Status == EDnd5eJsonReadStatus::Failure && Catalog.TryGetSpell(Original.Header.RecordId, Out));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eExternalCatalogTest, "Dnd5e.Json.ExternalCatalog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eExternalCatalogTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Directory, LookupFile;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory) && FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonAvailabilityFile="), LookupFile))
	{
		Names.Add(TEXT("BuildJoinAndQuery"));
		Commands.Add(TEXT("BuildJoinAndQuery"));
	}
}
bool FDnd5eExternalCatalogTest::RunTest(const FString& Parameters)
{
	FString Directory, LookupFile;
	FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory);
	FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonAvailabilityFile="), LookupFile);
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Directory, TEXT("spells-*.json")), true, false);
	Files.Sort();
	if (!TestFalse(TEXT("Explicit directory contains spell files"), Files.IsEmpty())) { return false; }
	FDnd5eJsonSpellCatalogBuildRequest Request;
	int32 ExpectedRecords = 0;
	for (const auto& File : Files)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("external-catalog");
		Document.Context.RelativeFilePath = FPaths::Combine(TEXT("spells"), File);
		if (!TestTrue(TEXT("Read caller-selected spell document"), FFileHelper::LoadFileToString(Document.JsonText, *FPaths::Combine(Directory, File)))) { return false; }
		TSharedPtr<FJsonObject> Root;
		if (!TestTrue(TEXT("External spell document has object JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Document.JsonText), Root))) { return false; }
		const TArray<TSharedPtr<FJsonValue>>* Spells = nullptr;
		if (!TestTrue(TEXT("External document contains a spell array"), Root->TryGetArrayField(TEXT("spell"), Spells) && Spells)) { return false; }
		ExpectedRecords += Spells->Num();
		Request.SpellDocuments.Add(MoveTemp(Document));
	}
	Request.bHasAvailabilityLookup = true;
	Request.AvailabilityLookup.Context.DatasetNamespace = TEXT("external-catalog");
	Request.AvailabilityLookup.Context.RelativeFilePath = TEXT("generated/availability.json");
	if (!TestTrue(TEXT("Read caller-selected lookup"), FFileHelper::LoadFileToString(Request.AvailabilityLookup.JsonText, *LookupFile))) { return false; }
	FDnd5eSpellCatalog Catalog;
	const auto Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	for (const auto& Diagnostic : Result.Diagnostics)
	{
		if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error) { AddError(Diagnostic.Code.ToString() + TEXT(" at ") + Diagnostic.JsonPointer); }
	}
	if (!TestTrue(TEXT("Full supplied catalog builds"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TestEqual(TEXT("Every primary record is present"), Catalog.GetSpellCount(), ExpectedRecords);
	int32 Known = 0, Associations = 0;
	for (const auto& Id : Catalog.GetSpellIds())
	{
		FDnd5eSpell Spell;
		Catalog.TryGetSpell(Id, Spell);
		Known += Spell.bHasAvailabilityData ? 1 : 0;
		Associations += Spell.Availability.Num();
	}
	FDnd5eSpellQuery Query;
	TArray<FDnd5eRecordId> Ids;
	TArray<FDnd5eDiagnostic> Diagnostics;
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestEqual(TEXT("An unfiltered query returns all loaded IDs"), Ids.Num(), ExpectedRecords);
	AddInfo(FString::Printf(TEXT("Built %d spells; %d matched lookup entries; %d typed availability associations. Source passages are not logged."), Catalog.GetSpellCount(), Known, Associations));
	return !HasAnyErrors();
}

#endif
