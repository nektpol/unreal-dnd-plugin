#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eCoverageTestSupport.h"
#include "Readers/Spells/Dnd5eJsonSpellReferenceNormalizer.h"
using namespace Dnd5eTests;
using namespace Dnd5eCoverageTests;

namespace
{
	struct FTokenCase { FString Name; FString Source; FString Kind; };
	TArray<FTokenCase> Tokens()
	{
		TArray<FTokenCase> Cases;
		for (const TCHAR* Tag : {TEXT("spell"), TEXT("item"), TEXT("creature"), TEXT("condition"), TEXT("status"), TEXT("action"), TEXT("skill"), TEXT("sense"), TEXT("hazard"), TEXT("variantrule"), TEXT("race"), TEXT("feat"), TEXT("background")})
		{
			Cases.Add({FString(Tag) + TEXT("Explicit"), FString(TEXT("{@")) + Tag + TEXT(" Invented|SYN|label}"), TEXT("Reference")});
			Cases.Add({FString(Tag) + TEXT("Omitted"), FString(TEXT("{@")) + Tag + TEXT(" Invented}"), TEXT("Reference")});
		}
		Cases.Append({
			{TEXT("Feature"), TEXT("{@classFeature Invented|Invented Class||3}"), TEXT("Reference")},
			{TEXT("FeatureFull"), TEXT("{@classFeature Invented|Invented Class|CLS|20|SYN|label}"), TEXT("Reference")},
			{TEXT("FeatureZero"), TEXT("{@classFeature Invented|Class||0}"), TEXT("Unsupported")},
			{TEXT("FeatureOver"), TEXT("{@classFeature Invented|Class||21}"), TEXT("Unsupported")},
			{TEXT("FeatureBraces"), TEXT("{@classFeature Invented|{Class}||3}"), TEXT("Unsupported")},
			{TEXT("ReferenceExtra"), TEXT("{@item Invented|SYN|label|extra}"), TEXT("Unsupported")},
			{TEXT("ReferenceBlank"), TEXT("{@item |SYN}"), TEXT("Unsupported")},
			{TEXT("Book"), TEXT("{@book label|SYNBOOK|0|section|1}"), TEXT("DocumentLink")},
			{TEXT("Adventure"), TEXT("{@adventure label|SYNADV}"), TEXT("DocumentLink")},
			{TEXT("BookNegative"), TEXT("{@book label|SYNBOOK|-1}"), TEXT("Unsupported")},
			{TEXT("BookOverflow"), TEXT("{@book label|SYNBOOK|2147483648}"), TEXT("Unsupported")},
			{TEXT("BookExtra"), TEXT("{@book label|SYNBOOK|0|section|1|extra}"), TEXT("Unsupported")},
			{TEXT("Quick"), TEXT("{@quickref Invented|SYN|2|header|label}"), TEXT("QuickReference")},
			{TEXT("QuickOmitted"), TEXT("{@quickref Invented}"), TEXT("QuickReference")},
			{TEXT("QuickNegative"), TEXT("{@quickref Invented|SYN|-1}"), TEXT("Unsupported")},
			{TEXT("Filter"), TEXT("{@filter label|spells|level=1;2|class=Invented=Class}"), TEXT("QueryLink")},
			{TEXT("FilterEmptyValue"), TEXT("{@filter label|spells|field=}"), TEXT("QueryLink")},
			{TEXT("FilterMissingEquals"), TEXT("{@filter label|spells|field}"), TEXT("Unsupported")},
			{TEXT("FilterEmptyField"), TEXT("{@filter label|spells|=value}"), TEXT("Unsupported")},
			{TEXT("Note"), TEXT("{@note Invented {@b label}.}"), TEXT("Formatted")}
		});
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eCoverageTokensTest, "Dnd5e.Json.Coverage.InlineKinds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eCoverageTokensTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const { for (const auto& C : Tokens()) { Names.Add(C.Name); Commands.Add(C.Name); } }
bool FDnd5eCoverageTokensTest::RunTest(const FString& Parameters)
{
	const auto Cases = Tokens(); const auto* C = Cases.FindByPredicate([&](const auto& V) { return V.Name == Parameters; }); if (!C) { return false; }
	const auto Input = Token(C->Source); const auto Before = Input; const auto Result = Read(Input); FDnd5eInlineContent Value;
	if (!TestTrue(TEXT("Token converts without loss"), Result.Status == EDnd5eJsonReadStatus::Success && FirstInline(Result.Spell, Value))) { return false; }
	TestEqual(TEXT("Exact semantic inline kind"), Value.Kind.ToString(), FString(TEXT("Dnd5e.Content.Inline.")) + C->Kind);
	if (C->Kind == TEXT("Reference"))
	{
		const auto& Ref = Value.Payload.Get<FDnd5eReferenceContent>();
		TestTrue(TEXT("Whole source and typed selector retained"), Ref.bHasTargetSelector && Ref.SourceReference == C->Source && Ref.SourceFormat == TEXT("5etools"));
		const bool Explicit = C->Name.EndsWith(TEXT("Explicit")) || C->Name == TEXT("FeatureFull");
		TestEqual(TEXT("No invented publication"), Ref.TargetSelector.bHasSource, Explicit);
		TestEqual(TEXT("Only explicit spells receive v1 IDs"), Ref.bHasTargetRecordId, C->Name == TEXT("spellExplicit"));
		if (C->Name.StartsWith(TEXT("Feature"))) { const auto* Feature = Ref.TargetSelector.Qualifier.GetPtr<FDnd5eClassFeatureSelector>(); TestTrue(TEXT("Typed feature qualifier"), Feature && Feature->Level == (Explicit ? 20 : 3)); }
	}
	else if (C->Kind == TEXT("Unsupported")) { TestEqual(TEXT("Whole fallback token"), Value.Payload.Get<FDnd5eUnsupportedContent>().SourceFragment, C->Source); }
	else if (C->Kind == TEXT("DocumentLink")) { const auto& V = Value.Payload.Get<FDnd5eDocumentLinkContent>(); TestTrue(TEXT("Document locator stays a document"), V.DocumentId.StartsWith(TEXT("SYN")) && V.SourceReference == C->Source && V.bHasChapter == (C->Name == TEXT("Book"))); }
	else if (C->Kind == TEXT("QuickReference")) { const auto& V = Value.Payload.Get<FDnd5eQuickReferenceContent>(); TestTrue(TEXT("Quickref source omission and locator"), V.bHasSource == (C->Name == TEXT("Quick")) && V.Name == TEXT("Invented")); }
	else if (C->Kind == TEXT("QueryLink")) { const auto& V = Value.Payload.Get<FDnd5eQueryLinkContent>(); TestEqual(TEXT("Target page retained"), V.TargetPage, FString(TEXT("spells"))); TestEqual(TEXT("First equals only"), V.Clauses.Last().RawValue, FString(C->Name == TEXT("Filter") ? TEXT("Invented=Class") : TEXT(""))); }
	else { const auto& V = Value.Payload.Get<FDnd5eFormattedContent>(); TestTrue(TEXT("Note differs from plain italics"), V.Formatting.HasTagExact(Dnd5e::Content::Formatting::Note) && V.Formatting.HasTagExact(Dnd5e::Content::Formatting::Italic) && V.SourceReference == C->Source); }
	TestTrue(TEXT("Source document not mutated"), Equal(FDnd5eJsonDocument::StaticStruct(), &Input, &Before));
	TestTrue(TEXT("Parsed Content validates"), UDnd5eContentValidationLibrary::ValidateSpellContent(Result.Spell).bValid);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCoveragePresentationTest, "Dnd5e.Json.Coverage.PresentationAndNestedDestinations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCoveragePresentationTest::RunTest(const FString& Parameters)
{
	const auto Result = Read(Document(TEXT(R"json([{"type":"list","style":"list-hang-notitle","items":["Body"]},{"type":"table","colStyles":["col-1-2 text-center no-wrap"],"rows":[["Cell"]]},"{@book {@spell Invented Beacon|SYN|nested}|SYNBOOK|0}"])json")));
	if (!TestEqual(TEXT("Presentation fixture converts"), Result.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	const auto& List = Result.Spell.Description[0].Payload.Get<FDnd5eListContent>(); const auto& Table = Result.Spell.Description[1].Payload.Get<FDnd5eTableContent>();
	TestTrue(TEXT("Hanging labels have no ordinary marker"), List.Presentation.HasTagExact(Dnd5e::Content::Presentation::HangingList) && List.MarkerStyle == EDnd5eListMarkerStyle::None);
	TestTrue(TEXT("Fractional layout weight retained"), Table.Columns[0].WidthWeight == 1.2f && Table.Columns[0].Presentation.HasTagExact(Dnd5e::Content::Presentation::NoWrap) && Table.Columns[0].Alignment == EDnd5eContentAlignment::Center);
	TArray<FDnd5eContentReferenceOccurrence> References; TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Nested destination labels remain traversable"), UDnd5eSpellContentLibrary::CollectSpellReferences(Result.Spell, References, Diagnostics) && References.Num() == 1);
	const auto Mixed = Read(Document(TEXT(R"json([{"type":"table","colStyles":["no-wrap invented-class"],"rows":[["Cell"]]}])json")));
	if (TestEqual(TEXT("Unknown presentation retained"), Mixed.Status, EDnd5eJsonReadStatus::Success)) { TSharedPtr<FJsonObject> Evidence; TestTrue(TEXT("Mixed known/unknown style evidence retained"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Mixed.Spell.Description[0].Metadata.AdditionalMetadataJson), Evidence) && Evidence.IsValid()); }
	return !HasAnyErrors();
}
#endif
