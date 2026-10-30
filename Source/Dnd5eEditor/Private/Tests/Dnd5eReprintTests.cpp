// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Dnd5eJsonSpellReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

using namespace Dnd5eTests;

namespace
{
	TSharedRef<FJsonObject> ReprintSpell(const FString& Name = TEXT("Invented Origin"), const FString& Source = TEXT("SYN-A"))
	{
		TSharedPtr<FJsonObject> Object;
		const FString Json = TEXT(R"json({"name":"Invented Origin","source":"SYN-A","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":["An original synthetic description."]})json");
		check(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object));
		Object->SetStringField(TEXT("name"), Name); Object->SetStringField(TEXT("source"), Source);
		return Object.ToSharedRef();
	}

	FDnd5eJsonSpellReadRequest ReprintRequest(const TSharedRef<FJsonObject>& Object)
	{
		FDnd5eJsonSpellReadRequest Request;
		Request.Context.DatasetNamespace = TEXT("reprint-synthetic");
		Request.Context.RelativeFilePath = TEXT("spells/reprints.json");
		Request.SpellIndex = 0;
		auto Root = MakeShared<FJsonObject>();
		Root->SetArrayField(TEXT("spell"), {MakeShared<FJsonValueObject>(Object)});
		FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Request.JsonText));
		return Request;
	}

	struct FReprintCase
	{
		FString Name, Uid;
		bool bNormalized;
		FString ExpectedKey;
		FString StructuralJson;
	};

	TArray<FReprintCase> ReprintCases()
	{
		return {
			{TEXT("Explicit"), TEXT("Invented Beacon|SYN-B"), true, TEXT("5etools:spell:v1:Invented%20Beacon|SYN-B"), TEXT("")},
			{TEXT("TrimmedParts"), TEXT("  Invented Beacon | SYN-B  "), true, TEXT("5etools:spell:v1:Invented%20Beacon|SYN-B"), TEXT("")},
			{TEXT("Unicode"), TEXT("Invented Ω|SYN-B"), true, TEXT("5etools:spell:v1:Invented%20%CE%A9|SYN-B"), TEXT("")},
			{TEXT("LiteralPercent"), TEXT("Invented %20|SYN/%"), true, TEXT("5etools:spell:v1:Invented%20%2520|SYN%2F%25"), TEXT("")},
			{TEXT("MissingSeparator"), TEXT("Invented Beacon"), false, TEXT(""), TEXT("")},
			{TEXT("EmptySource"), TEXT("Invented Beacon|"), false, TEXT(""), TEXT("")},
			{TEXT("BlankSource"), TEXT("Invented Beacon|  "), false, TEXT(""), TEXT("")},
			{TEXT("EmptyName"), TEXT("|SYN-B"), false, TEXT(""), TEXT("")},
			{TEXT("ExtraPart"), TEXT("Invented Beacon|SYN-B|label"), false, TEXT(""), TEXT("")},
			{TEXT("TrailingSeparator"), TEXT("Invented Beacon|SYN-B|"), false, TEXT(""), TEXT("")},
			{TEXT("ControlName"), TEXT("Invented\nBeacon|SYN-B"), false, TEXT(""), TEXT("")},
			{TEXT("ControlSource"), TEXT("Invented Beacon|SYN\tB"), false, TEXT(""), TEXT("")},
			{TEXT("NotArray"), TEXT(""), false, TEXT(""), TEXT("true")},
			{TEXT("NumberEntry"), TEXT(""), false, TEXT(""), TEXT("[17]")},
			{TEXT("ObjectEntry"), TEXT(""), false, TEXT(""), TEXT("[{\"uid\":\"Invented Beacon|SYN-B\"}]")},
			{TEXT("EmptyEntry"), TEXT(""), false, TEXT(""), TEXT("[\"\"]")}
		};
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eReprintGrammarTest, "Dnd5e.Json.Reprints.Selectors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eReprintGrammarTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : ReprintCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}

bool FDnd5eReprintGrammarTest::RunTest(const FString& Parameters)
{
	const auto Cases = ReprintCases();
	const auto* Case = Cases.FindByPredicate([&](const auto& Value) { return Value.Name == Parameters; });
	if (!TestNotNull(TEXT("Reprint fixture exists"), Case)) { return false; }
	auto Object = ReprintSpell();
	if (Case->StructuralJson.IsEmpty()) { Object->SetArrayField(TEXT("reprintedAs"), {MakeShared<FJsonValueString>(Case->Uid)}); }
	else
	{
		TSharedPtr<FJsonObject> Wrapper;
		const FString Json = TEXT("{\"value\":") + Case->StructuralJson + TEXT("}");
		if (!TestTrue(TEXT("Parse structural fixture"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Wrapper))) { return false; }
		Object->SetField(TEXT("reprintedAs"), Wrapper->TryGetField(TEXT("value")));
	}
	const auto Request = ReprintRequest(Object);
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!Case->StructuralJson.IsEmpty())
	{
		const FDnd5eSpell Default;
		TestTrue(TEXT("Existing structural failure remains atomic"), Result.Status == EDnd5eJsonReadStatus::Failure && Equal(FDnd5eSpell::StaticStruct(), &Result.Spell, &Default));
		if (TestEqual(TEXT("One structural error is returned"), Result.Diagnostics.Num(), 1))
		{
			TestTrue(TEXT("The offending field/entry is located"), Result.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Error && Result.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.InvalidFieldValue")) && Result.Diagnostics[0].JsonPointer == (Case->Name == TEXT("NotArray") ? TEXT("/spell/0/reprintedAs") : TEXT("/spell/0/reprintedAs/0")));
		}
		return !HasAnyErrors();
	}
	if (!TestTrue(TEXT("Descriptive selector conversion succeeds"), Result.Status == EDnd5eJsonReadStatus::Success) || !TestEqual(TEXT("The relationship is retained once"), Result.Spell.Relationships.Num(), 1)) { return false; }
	const auto& Relationship = Result.Spell.Relationships[0];
	TestTrue(TEXT("Original kind/format/string evidence is retained"), Relationship.Kind == Dnd5e::Record::Relationship::ReprintedAs && Relationship.SourceFormat == TEXT("5etools") && Relationship.SourceReference == Case->Uid);
	TestEqual(TEXT("Normalization presence matches the supported grammar"), Relationship.bHasTargetRecord, Case->bNormalized);
	if (Case->bNormalized)
	{
		TestTrue(TEXT("Canonical versioned identity is constructed without lookup"), Relationship.TargetRecordId.SourceKey == Case->ExpectedKey && Relationship.TargetRecordId.DatasetNamespace == Request.Context.DatasetNamespace && Relationship.TargetRecordId.Category == Dnd5e::Category::Spell);
		TestTrue(TEXT("Normalized selectors no longer produce an unresolved warning"), Result.Diagnostics.IsEmpty());
	}
	else
	{
		TestTrue(TEXT("Unsupported selector leaves a completely default ID"), Relationship.TargetRecordId == FDnd5eRecordId{});
		if (TestEqual(TEXT("Retained selector produces one warning"), Result.Diagnostics.Num(), 1))
		{
			const auto& Diagnostic = Result.Diagnostics[0];
			TestTrue(TEXT("Warning code/severity and referring location are correct"), Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning && Diagnostic.Code == FName(TEXT("Dnd5e.Json.UnresolvedReference")) && Diagnostic.JsonPointer == TEXT("/spell/0/reprintedAs/0") && Diagnostic.RelativeFilePath == Request.Context.RelativeFilePath);
		}
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eReprintIdentityTest, "Dnd5e.Json.Reprints.IdentityAgreementAndOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eReprintIdentityTest::RunTest(const FString& Parameters)
{
	const FString Name = TEXT("Invented Ω/100%"), Source = TEXT("SYN/%");
	const FString Uid = Name + TEXT("|") + Source;
	auto Referrer = ReprintSpell();
	Referrer->SetArrayField(TEXT("reprintedAs"), {MakeShared<FJsonValueString>(Uid), MakeShared<FJsonValueString>(TEXT("Missing source")), MakeShared<FJsonValueString>(TEXT("Other|SYN-C"))});
	Referrer->SetArrayField(TEXT("entries"), {MakeShared<FJsonValueString>(TEXT("See {@spell ") + Uid + TEXT("}."))});
	const auto From = FDnd5eJsonSpellReader::ReadSpell(ReprintRequest(Referrer));
	const auto To = FDnd5eJsonSpellReader::ReadSpell(ReprintRequest(ReprintSpell(Name, Source)));
	if (!TestTrue(TEXT("Both original fixtures convert"), From.Status == EDnd5eJsonReadStatus::Success && To.Status == EDnd5eJsonReadStatus::Success) || !TestEqual(TEXT("All relationship positions survive"), From.Spell.Relationships.Num(), 3)) { return false; }
	const auto& Id = To.Spell.Header.RecordId;
	TestEqual(TEXT("UTF-8 escaping is unchanged by the shared constructor"), Id.SourceKey, FString(TEXT("5etools:spell:v1:Invented%20%CE%A9%2F100%25|SYN%2F%25")));
	TestTrue(TEXT("Header and reprint use identical IDs"), From.Spell.Relationships[0].TargetRecordId == Id);
	TestTrue(TEXT("A failed middle selector does not leave a prior ID or discard later entries"), !From.Spell.Relationships[1].bHasTargetRecord && From.Spell.Relationships[1].TargetRecordId == FDnd5eRecordId{} && From.Spell.Relationships[2].bHasTargetRecord);
	FDnd5eParagraphContent Paragraph;
	if (!TestTrue(TEXT("Read the inline reference"), UDnd5eContentLibrary::TryGetParagraph(From.Spell.Description[0], Paragraph))) { return false; }
	bool bSame = false;
	for (const auto& InlineValue : Paragraph.Inlines)
	{
		FDnd5eReferenceContent Reference;
		if (UDnd5eContentLibrary::TryGetReference(InlineValue, Reference)) { bSame = Reference.bHasTargetRecordId && Reference.TargetRecordId == Id; }
	}
	TestTrue(TEXT("Inline references use that same constructor"), bSame);
	TestTrue(TEXT("Only the unsupported middle reprint needs a warning"), From.Diagnostics.Num() == 1 && From.Diagnostics[0].JsonPointer == TEXT("/spell/0/reprintedAs/1"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eReprintLookupTest, "Dnd5e.Catalog.Relationship.ReaderToLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eReprintLookupTest::RunTest(const FString& Parameters)
{
	auto Object = ReprintSpell();
	Object->SetArrayField(TEXT("reprintedAs"), {MakeShared<FJsonValueString>(TEXT("Invented Beacon|SYN-B"))});
	const auto From = FDnd5eJsonSpellReader::ReadSpell(ReprintRequest(Object));
	const auto To = FDnd5eJsonSpellReader::ReadSpell(ReprintRequest(ReprintSpell(TEXT("Invented Beacon"), TEXT("SYN-B"))));
	if (!TestTrue(TEXT("Read both synthetic records"), From.Status == EDnd5eJsonReadStatus::Success && To.Status == EDnd5eJsonReadStatus::Success && From.Spell.Relationships.Num() == 1)) { return false; }
	const auto Relationship = From.Spell.Relationships[0];
	const auto Before = Relationship;
	FDnd5eSpellCatalog Catalog;
	FDnd5eSpell Output = To.Spell;
	TArray<FDnd5eDiagnostic> Diagnostics;
	Catalog.ReplaceSpells({From.Spell}, Diagnostics);
	TestEqual(TEXT("A normalized target can be absent from a selected subset"), Catalog.ResolveSpellRelationship(Relationship, Output, Diagnostics), EDnd5eReferenceResolveStatus::TargetNotFound);
	auto Target = To.Spell;
	FDnd5eRecordRelationship Back;
	Back.Kind = Dnd5e::Record::Relationship::ReprintedAs; Back.bHasTargetRecord = true; Back.TargetRecordId = From.Spell.Header.RecordId;
	Target.Relationships.Add(Back);
	if (!TestTrue(TEXT("Install both records without merging them"), Catalog.ReplaceSpells({From.Spell, Target}, Diagnostics))) { return false; }
	TestTrue(TEXT("Lookup returns only the selected edge's target, without following the cycle"), Catalog.ResolveSpellRelationship(Relationship, Output, Diagnostics) == EDnd5eReferenceResolveStatus::Resolved && Diagnostics.IsEmpty() && Equal(FDnd5eSpell::StaticStruct(), &Output, &Target) && Catalog.GetSpellCount() == 2);
	Output.Description.Reset();
	Catalog.ResolveSpellRelationship(Relationship, Output, Diagnostics);
	TestTrue(TEXT("Returned storage is independent"), Equal(FDnd5eSpell::StaticStruct(), &Output, &Target));
	TestTrue(TEXT("Original relationship evidence remains unchanged"), Equal(FDnd5eRecordRelationship::StaticStruct(), &Relationship, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eExternalReprintsTest, "Dnd5e.Json.ExternalReprints", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eExternalReprintsTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("Dnd5eJsonAuditReprints"))) { Names.Add(TEXT("AllDeclaredTargets")); Commands.Add(TEXT("AllDeclaredTargets")); }
}

bool FDnd5eExternalReprintsTest::RunTest(const FString& Parameters)
{
	FString Directory;
	if (!TestTrue(TEXT("Closure audit requires an explicitly selected complete spell directory"), FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory))) { return false; }
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Directory, TEXT("spells-*.json")), true, false);
	Files.Sort();
	if (!TestFalse(TEXT("Selected directory contains spell files"), Files.IsEmpty())) { return false; }
	FDnd5eJsonSpellCatalogBuildRequest Request;
	int32 ExpectedReprints = 0;
	for (const auto& File : Files)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("external-reprints"); Document.Context.RelativeFilePath = TEXT("spells/") + File;
		if (!TestTrue(TEXT("Read selected input"), FFileHelper::LoadFileToString(Document.JsonText, *FPaths::Combine(Directory, File)))) { return false; }
		TSharedPtr<FJsonObject> Root;
		if (!TestTrue(TEXT("Parse independent expected relationship count"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Document.JsonText), Root))) { return false; }
		for (const auto& Value : Root->GetArrayField(TEXT("spell")))
		{
			const TArray<TSharedPtr<FJsonValue>>* Reprints = nullptr;
			if (Value->AsObject()->TryGetArrayField(TEXT("reprintedAs"), Reprints)) { ExpectedReprints += Reprints->Num(); }
		}
		Request.SpellDocuments.Add(MoveTemp(Document));
	}
	FDnd5eSpellCatalog Catalog;
	if (!TestTrue(TEXT("Convert selected closure-audit documents"), FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog).Status == EDnd5eJsonReadStatus::Success)) { return false; }
	int32 Checked = 0;
	for (const auto& Id : Catalog.GetSpellIds())
	{
		FDnd5eSpell SourceSpell;
		Catalog.TryGetSpell(Id, SourceSpell);
		for (const auto& Relationship : SourceSpell.Relationships)
		{
			if (Relationship.Kind != Dnd5e::Record::Relationship::ReprintedAs) { continue; }
			++Checked;
			FDnd5eSpell Target;
			TArray<FDnd5eDiagnostic> Diagnostics;
			if (!TestTrue(TEXT("Every selector in the explicitly asserted closed dataset normalizes and resolves"), Relationship.bHasTargetRecord && Catalog.ResolveSpellRelationship(Relationship, Target, Diagnostics) == EDnd5eReferenceResolveStatus::Resolved && Diagnostics.IsEmpty())) { return false; }
			TArray<FString> Parts; Relationship.SourceReference.ParseIntoArray(Parts, TEXT("|"), false);
			TestTrue(TEXT("Resolved target agrees with the retained selector's exact name and source"), Parts.Num() == 2 && Target.Header.DisplayName == Parts[0].TrimStartAndEnd() && !Target.Header.Provenance.IsEmpty() && Target.Header.Provenance[0].SourceId == Parts[1].TrimStartAndEnd());
		}
	}
	TestEqual(TEXT("All source-declared reprint edges were checked"), Checked, ExpectedReprints);
	AddInfo(FString::Printf(TEXT("Verified %d reprint edges across %d loaded spells; source passages are not logged."), Checked, Catalog.GetSpellCount()));
	return !HasAnyErrors();
}

#endif
