// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Dnd5eJsonSpellCatalogLibrary.h"
#include "Readers/Dnd5eJsonSpellReader.h"
#include "Catalog/Dnd5eSpellCatalogObject.h"
#include "UObject/StrongObjectPtr.h"

using namespace Dnd5eTests;

namespace
{
	using FNormalizeStatus = EDnd5eJsonSpellReferenceNormalizeStatus;
	FDnd5eSpell NormalizationRecord(const TCHAR* Key = TEXT("opaque/canonical"))
	{
		auto Spell = SyntheticSpell();
		Spell.Header.RecordId.DatasetNamespace = TEXT("normalize-synthetic");
		Spell.Header.RecordId.Category = Dnd5e::Category::Spell;
		Spell.Header.RecordId.SourceKey = Key;
		Spell.Header.DisplayName = TEXT("Invented Beacon");
		Spell.Header.Provenance.Reset();
		FDnd5eRecordProvenance Source; Source.SourceId = TEXT("SYN-B"); Spell.Header.Provenance.Add(Source);
		return Spell;
	}
	FDnd5eReferenceContent NormalizationInput(const FString& Token)
	{
		FDnd5eReferenceContent Reference;
		Reference.SourceFormat = TEXT("5etools"); Reference.SourceReference = Token;
		Reference.Display = {Text(TEXT("Preserved display"))};
		Reference.bHasTargetRecordId = true;
		Reference.TargetRecordId = NormalizationRecord(TEXT("stale/id")).Header.RecordId;
		return Reference;
	}
	struct FNormalizationCase
	{
		FString Name;
		FString Token;
		FNormalizeStatus Status;
		FString Namespace = TEXT("normalize-synthetic");
		FString Format = TEXT("5etools");
		int32 RecordCount = 1;
	};
	TArray<FNormalizationCase> NormalizationCases()
	{
		TArray<FNormalizationCase> Cases = {
			{TEXT("Exact"), TEXT("{@spell Invented Beacon|SYN-B}"), FNormalizeStatus::Normalized},
			{TEXT("CaseFolded"), TEXT("{@spell invented beacon|syn-b}"), FNormalizeStatus::Normalized},
			{TEXT("Trimmed"), TEXT("{@spell  Invented Beacon | SYN-B }"), FNormalizeStatus::Normalized},
			{TEXT("Display"), TEXT("{@spell invented beacon|syn-b|Different label}"), FNormalizeStatus::Normalized},
			{TEXT("EmptyDisplay"), TEXT("{@spell invented beacon|syn-b|}"), FNormalizeStatus::Normalized},
			{TEXT("MissingSource"), TEXT("{@spell Invented Beacon}"), FNormalizeStatus::MissingSource},
			{TEXT("EmptySource"), TEXT("{@spell Invented Beacon|}"), FNormalizeStatus::MissingSource},
			{TEXT("BlankSource"), TEXT("{@spell Invented Beacon|   }"), FNormalizeStatus::MissingSource},
			{TEXT("MissingSourceWithDisplay"), TEXT("{@spell Invented Beacon||syn-b}"), FNormalizeStatus::MissingSource},
			{TEXT("NoMatchName"), TEXT("{@spell Absent|syn-b}"), FNormalizeStatus::NoMatch},
			{TEXT("NoMatchSource"), TEXT("{@spell Invented Beacon|absent}"), FNormalizeStatus::NoMatch},
			{TEXT("InvalidName"), TEXT("{@spell |syn-b}"), FNormalizeStatus::InvalidSelector},
			{TEXT("NameControl"), TEXT("{@spell Invented\nBeacon|syn-b}"), FNormalizeStatus::InvalidSelector},
			{TEXT("SourceControl"), TEXT("{@spell Invented Beacon|syn\nb}"), FNormalizeStatus::InvalidSelector},
			{TEXT("OtherTag"), TEXT("{@item Invented Beacon|syn-b}"), FNormalizeStatus::UnsupportedReference},
			{TEXT("UppercaseTag"), TEXT("{@SPELL Invented Beacon|syn-b}"), FNormalizeStatus::UnsupportedReference},
			{TEXT("NoClosingBrace"), TEXT("{@spell Invented Beacon|syn-b"), FNormalizeStatus::UnsupportedReference},
			{TEXT("NoOpeningMarker"), TEXT("Invented Beacon|syn-b}"), FNormalizeStatus::UnsupportedReference},
			{TEXT("EmptyToken"), TEXT(""), FNormalizeStatus::UnsupportedReference},
			{TEXT("ExtraPart"), TEXT("{@spell Invented Beacon|syn-b|label|extra}"), FNormalizeStatus::UnsupportedReference},
			{TEXT("NestedName"), TEXT("{@spell {Invented Beacon}|syn-b}"), FNormalizeStatus::UnsupportedReference},
			{TEXT("NestedSource"), TEXT("{@spell Invented Beacon|{syn-b}}"), FNormalizeStatus::UnsupportedReference},
			{TEXT("TrailingText"), TEXT("{@spell Invented Beacon|syn-b} trailing"), FNormalizeStatus::UnsupportedReference},
			{TEXT("EmbeddedToken"), TEXT("before {@spell Invented Beacon|syn-b}"), FNormalizeStatus::UnsupportedReference}
		};
		auto Value = Cases[0]; Value.Name = TEXT("WrongFormat"); Value.Format = TEXT("5eTools"); Value.Status = FNormalizeStatus::UnsupportedReference; Cases.Add(Value);
		Value = Cases[0]; Value.Name = TEXT("InvalidNamespace"); Value.Namespace = TEXT(""); Value.Status = FNormalizeStatus::InvalidSelector; Cases.Add(Value);
		Value.Name = TEXT("NamespaceControl"); Value.Namespace = TEXT("bad\nnamespace"); Cases.Add(Value);
		Value = Cases[0]; Value.Name = TEXT("NamespaceIsolation"); Value.Namespace = TEXT("Normalize-synthetic"); Value.Status = FNormalizeStatus::NoMatch; Cases.Add(Value);
		Value = Cases[0]; Value.Name = TEXT("EmptyCatalog"); Value.RecordCount = 0; Value.Status = FNormalizeStatus::NoMatch; Cases.Add(Value);
		Value = Cases[5]; Value.Name = TEXT("MissingSourceZero"); Value.RecordCount = 0; Cases.Add(Value);
		Value = Cases[5]; Value.Name = TEXT("MissingSourceMany"); Value.RecordCount = 2; Cases.Add(Value);
		Value = Cases[0]; Value.Name = TEXT("Ambiguous"); Value.RecordCount = 2; Value.Status = FNormalizeStatus::Ambiguous; Cases.Add(Value);
		const FString Prefix = TEXT("{@spell Invented Beacon|syn-b|");
		Value = Cases[0]; Value.Name = TEXT("TokenAtLimit"); Value.Token = Prefix + FString::ChrN(65536 - Prefix.Len() - 1, TEXT('x')) + TEXT("}"); Cases.Add(Value);
		Value.Name = TEXT("TokenOverLimit"); Value.Token.InsertAt(Value.Token.Len() - 1, TEXT('x')); Value.Status = FNormalizeStatus::UnsupportedReference; Cases.Add(Value);
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eNormalizationMatrixTest, "Dnd5e.Json.ReferenceNormalization.Cases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eNormalizationMatrixTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : NormalizationCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}
bool FDnd5eNormalizationMatrixTest::RunTest(const FString& Parameters)
{
	const auto Cases = NormalizationCases();
	const auto* Case = Cases.FindByPredicate([&](const auto& Item) { return Item.Name == Parameters; });
	if (!TestNotNull(TEXT("Known normalization case"), Case)) { return false; }
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eSpell> Records;
	if (Case->RecordCount > 0) { Records.Add(NormalizationRecord()); }
	if (Case->RecordCount > 1) { Records.Add(NormalizationRecord(TEXT("opaque/second"))); }
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Install valid fixture"), Catalog.ReplaceSpells(Records, Diagnostics))) { return false; }
	auto Input = NormalizationInput(Case->Token); Input.SourceFormat = Case->Format;
	const auto Before = Input;
	auto Output = NormalizationInput(TEXT("stale output")); Diagnostics.AddDefaulted(2);
	const auto Status = FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(Catalog, Case->Namespace, Input, Output, Diagnostics);
	TestEqual(TEXT("Documented outcome"), Status, Case->Status);
	TestTrue(TEXT("Input preserved"), Equal(FDnd5eReferenceContent::StaticStruct(), &Input, &Before));
	auto Expected = Before;
	if (Status == FNormalizeStatus::Normalized)
	{
		Expected.bHasTargetRecordId = true; Expected.TargetRecordId = Records[0].Header.RecordId;
		TestTrue(TEXT("Success clears diagnostics"), Diagnostics.IsEmpty());
		FDnd5eSpell Resolved;
		TestEqual(TEXT("Canonical reference resolves"), Catalog.ResolveSpellReference(Output, Resolved, Diagnostics), EDnd5eReferenceResolveStatus::Resolved);
		TestEqual(TEXT("Resolved canonical name"), Resolved.Header.DisplayName, FString(TEXT("Invented Beacon")));
	}
	else if (TestEqual(TEXT("One fresh failure diagnostic"), Diagnostics.Num(), 1))
	{
		FName ExpectedCode = TEXT("Dnd5e.Json.SpellReference.Unsupported");
		if (Status == FNormalizeStatus::MissingSource) { ExpectedCode = TEXT("Dnd5e.Catalog.Selector.MissingSource"); }
		if (Status == FNormalizeStatus::InvalidSelector) { ExpectedCode = TEXT("Dnd5e.Catalog.Selector.Invalid"); }
		if (Status == FNormalizeStatus::NoMatch) { ExpectedCode = TEXT("Dnd5e.Catalog.Selector.NoMatch"); }
		if (Status == FNormalizeStatus::Ambiguous) { ExpectedCode = TEXT("Dnd5e.Catalog.Selector.Ambiguous"); }
		TestEqual(TEXT("Stable code"), Diagnostics[0].Code, ExpectedCode);
		TestEqual(TEXT("Severity"), Diagnostics[0].Severity, Status == FNormalizeStatus::InvalidSelector ? EDnd5eDiagnosticSeverity::Error : EDnd5eDiagnosticSeverity::Warning);
		TestTrue(TEXT("No invented location"), Diagnostics[0].RelativeFilePath.IsEmpty() && Diagnostics[0].JsonPointer.IsEmpty());
	}
	TestTrue(TEXT("Only successful normalization changes target fields"), Equal(FDnd5eReferenceContent::StaticStruct(), &Output, &Expected));
	TestEqual(TEXT("Catalog count preserved"), Catalog.GetSpellCount(), Records.Num());
	for (const auto& Record : Records)
	{
		FDnd5eSpell Again;
		TestTrue(TEXT("Catalog record preserved"), Catalog.TryGetSpell(Record.Header.RecordId, Again) && Equal(FDnd5eSpell::StaticStruct(), &Again, &Record));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eNormalizationOwnershipTest, "Dnd5e.Json.ReferenceNormalization.OwnershipAndReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eNormalizationOwnershipTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	const auto Record = NormalizationRecord();
	TestTrue(TEXT("Install"), Catalog.ReplaceSpells({Record}, Diagnostics));
	auto Input = NormalizationInput(TEXT("{@spell invented beacon|syn-b}")); Input.bHasTargetRecordId = false; Input.TargetRecordId = {};
	const auto Original = Input;
	TestEqual(TEXT("Aliased success"), FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(Catalog, TEXT("normalize-synthetic"), Input, Input, Diagnostics), FNormalizeStatus::Normalized);
	TestTrue(TEXT("ID now supplied"), Input.bHasTargetRecordId && Input.TargetRecordId == Record.Header.RecordId);
	Input.Display[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed output");
	TestTrue(TEXT("Copied display owns its payload"), !Equal(FDnd5eReferenceContent::StaticStruct(), &Input, &Original) && Original.Display[0].Payload.Get<FDnd5eTextContent>().Text == TEXT("Preserved display"));
	const auto BeforeFailure = Input;
	TestTrue(TEXT("Clear catalog"), Catalog.ReplaceSpells({}, Diagnostics));
	TestEqual(TEXT("Aliased failure"), FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(Catalog, TEXT("normalize-synthetic"), Input, Input, Diagnostics), FNormalizeStatus::NoMatch);
	TestTrue(TEXT("Failure preserves existing target too"), Equal(FDnd5eReferenceContent::StaticStruct(), &Input, &BeforeFailure));
	const auto Replacement = NormalizationRecord(TEXT("replacement/canonical"));
	TestTrue(TEXT("Replace"), Catalog.ReplaceSpells({Replacement}, Diagnostics));
	TestEqual(TEXT("Later success"), FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(Catalog, TEXT("normalize-synthetic"), Input, Input, Diagnostics), FNormalizeStatus::Normalized);
	TestTrue(TEXT("Replacement key returned and diagnostics reset"), Input.TargetRecordId == Replacement.Header.RecordId && Diagnostics.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eNormalizationReaderIntegrationTest, "Dnd5e.Json.ReferenceNormalization.ReaderToExactLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eNormalizationReaderIntegrationTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonSpellReadRequest Request;
	Request.SpellIndex = 0;
	Request.Context.DatasetNamespace = TEXT("normalize-synthetic"); Request.Context.RelativeFilePath = TEXT("invented.json");
	Request.JsonText = TEXT(R"json({"spell":[{"name":"Invented Caller","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":["{@spell invented beacon|syn-b|A different label}"]}]})json");
	const auto Read = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestEqual(TEXT("Read source document"), Read.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eParagraphContent Paragraph;
	if (!TestTrue(TEXT("Read a paragraph"), Read.Spell.Description.Num() == 1 && UDnd5eContentLibrary::TryGetParagraph(Read.Spell.Description[0], Paragraph) && Paragraph.Inlines.Num() == 1)) { return false; }
	FDnd5eReferenceContent Reference;
	if (!TestTrue(TEXT("Extract retained reference"), UDnd5eContentLibrary::TryGetReference(Paragraph.Inlines[0], Reference))) { return false; }
	FDnd5eSpellCatalog Catalog; TArray<FDnd5eDiagnostic> Diagnostics;
	const auto Target = NormalizationRecord();
	TestTrue(TEXT("Install opaque target"), Catalog.ReplaceSpells({Target}, Diagnostics));
	FDnd5eSpell Spell;
	TestEqual(TEXT("Reader-declared ID is not this opaque catalog key"), Catalog.ResolveSpellReference(Reference, Spell, Diagnostics), EDnd5eReferenceResolveStatus::TargetNotFound);
	FDnd5eReferenceContent Normalized;
	TestEqual(TEXT("Adapter reconciles retained selector"), FDnd5eJsonSpellReferenceNormalizer::NormalizeReference(Catalog, Request.Context.DatasetNamespace, Reference, Normalized, Diagnostics), FNormalizeStatus::Normalized);
	TestTrue(TEXT("Raw token and display retained"), Reference.SourceReference == Normalized.SourceReference && Equal(FDnd5eInlineContent::StaticStruct(), &Reference.Display[0], &Normalized.Display[0]));
	TestEqual(TEXT("Reconciled reference resolves exact loaded ID"), Catalog.ResolveSpellReference(Normalized, Spell, Diagnostics), EDnd5eReferenceResolveStatus::Resolved);
	return !HasAnyErrors();
}

#endif
