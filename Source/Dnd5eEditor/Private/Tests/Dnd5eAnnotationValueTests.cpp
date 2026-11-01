// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Spells/Dnd5eJsonSpellReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

using namespace Dnd5eTests;

namespace
{
	FDnd5eJsonSpellReadRequest AnnotationRequest(const FString& Content)
	{
		FDnd5eJsonSpellReadRequest Request; Request.SpellIndex = 0;
		Request.Context.DatasetNamespace = TEXT("annotation-synthetic"); Request.Context.RelativeFilePath = TEXT("spells/invented.json");
		TSharedPtr<FJsonObject> Root;
		const FString Json = TEXT(R"json({"spell":[{"name":"Invented Marker","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":[]}]})json");
		check(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root));
		Root->GetArrayField(TEXT("spell"))[0]->AsObject()->SetArrayField(TEXT("entries"), {MakeShared<FJsonValueString>(Content)});
		FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Request.JsonText));
		return Request;
	}
	struct FAnnotationCase { FString Name; FString Token; FGameplayTag Kind; int32 Amount = 0; FString Display; };
	TArray<FAnnotationCase> AnnotationCases()
	{
		TArray<FAnnotationCase> Cases;
		auto Add = [&](const TCHAR* Name, const TCHAR* Token, FGameplayTag Kind, int32 Amount, const TCHAR* Display) { Cases.Add({Name, Token, Kind, Amount, Display}); };
		Add(TEXT("DC"), TEXT("{@dc 13}"), Dnd5e::Content::Annotation::DifficultyClass, 13, TEXT("DC 13"));
		Add(TEXT("DCZero"), TEXT("{@dc 0}"), Dnd5e::Content::Annotation::DifficultyClass, 0, TEXT("DC 0"));
		Add(TEXT("DCMax"), TEXT("{@dc 2147483647}"), Dnd5e::Content::Annotation::DifficultyClass, MAX_int32, TEXT("DC 2147483647"));
		Add(TEXT("DCLabel"), TEXT("{@dc 13|a marker}"), Dnd5e::Content::Annotation::DifficultyClass, 13, TEXT("DC a marker"));
		Add(TEXT("HitSigned"), TEXT("{@hit +4}"), Dnd5e::Content::Annotation::AttackRoll, 4, TEXT("+4"));
		Add(TEXT("HitUnsigned"), TEXT("{@hit 4}"), Dnd5e::Content::Annotation::AttackRoll, 4, TEXT("+4"));
		Add(TEXT("D20Negative"), TEXT("{@d20 -2}"), Dnd5e::Content::Annotation::D20Roll, -2, TEXT("-2"));
		Add(TEXT("D20Minimum"), TEXT("{@d20 -2147483648}"), Dnd5e::Content::Annotation::D20Roll, MIN_int32, TEXT("-2147483648"));
		Add(TEXT("D20Maximum"), TEXT("{@d20 +2147483647}"), Dnd5e::Content::Annotation::D20Roll, MAX_int32, TEXT("+2147483647"));
		Add(TEXT("D20Zero"), TEXT("{@d20 -0}"), Dnd5e::Content::Annotation::D20Roll, 0, TEXT("+0"));
		Add(TEXT("D20LabelName"), TEXT("{@d20 +2|a modifier|Invented check}"), Dnd5e::Content::Annotation::D20Roll, 2, TEXT("a modifier"));
		Add(TEXT("ChanceZero"), TEXT("{@chance 0}"), Dnd5e::Content::Annotation::PercentChance, 0, TEXT("0"));
		Add(TEXT("ChanceHundred"), TEXT("{@chance 100}"), Dnd5e::Content::Annotation::PercentChance, 100, TEXT("100"));
		Add(TEXT("ChanceOutcomes"), TEXT("{@chance 35|a chance|Invented check|Success|Failure}"), Dnd5e::Content::Annotation::PercentChance, 35, TEXT("a chance"));
		Add(TEXT("ChanceEmptyLabel"), TEXT("{@chance 35||Invented check|Success|Failure}"), Dnd5e::Content::Annotation::PercentChance, 35, TEXT("35"));
		const TArray<TPair<FString, FString>> Rejected = {
			{TEXT("NegativeDC"), TEXT("{@dc -1}")}, {TEXT("DCOverflow"), TEXT("{@dc 2147483648}")},
			{TEXT("DCExpression"), TEXT("{@dc 8+PB}")}, {TEXT("DCExtra"), TEXT("{@dc 13|label|extra}")},
			{TEXT("RollOverflow"), TEXT("{@hit +2147483648}")}, {TEXT("RollUnderflow"), TEXT("{@d20 -2147483649}")},
			{TEXT("SignOnly"), TEXT("{@hit +}")}, {TEXT("Decimal"), TEXT("{@d20 1.5}")},
			{TEXT("Exponent"), TEXT("{@d20 1e2}")}, {TEXT("ChanceNegative"), TEXT("{@chance -1}")},
			{TEXT("ChanceOverHundred"), TEXT("{@chance 101}")}, {TEXT("ChanceExtra"), TEXT("{@chance 35|label|name|success|failure|extra}")},
			{TEXT("EmptyArgument"), TEXT("{@dc }")}, {TEXT("WrongCase"), TEXT("{@DC 13}")},
			{TEXT("Whitespace"), TEXT("{@hit  4}")}, {TEXT("HugeMagnitude"), TEXT("{@hit 9999999999999999999999999999}")}
		};
		for (const auto& Case : Rejected) { Cases.Add({Case.Key, Case.Value, {}}); }
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eAnnotationValueReadTest, "Dnd5e.Json.Annotations.NumericAndRollValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eAnnotationValueReadTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : AnnotationCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}
bool FDnd5eAnnotationValueReadTest::RunTest(const FString& Parameters)
{
	const auto Cases = AnnotationCases(); const auto* Case = Cases.FindByPredicate([&](const auto& Value) { return Value.Name == Parameters; });
	if (!TestNotNull(TEXT("Known annotation case"), Case)) { return false; }
	const auto Request = AnnotationRequest(Case->Token); const auto Before = Request;
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestEqual(TEXT("Supported or preserved token keeps spell readable"), Result.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eParagraphContent Paragraph;
	if (!TestTrue(TEXT("Single paragraph"), Result.Spell.Description.Num() == 1 && UDnd5eContentLibrary::TryGetParagraph(Result.Spell.Description[0], Paragraph) && Paragraph.Inlines.Num() == 1)) { return false; }
	if (!Case->Kind.IsValid())
	{
		FDnd5eUnsupportedContent Unsupported;
		TestTrue(TEXT("Complete rejected token is preserved"), UDnd5eContentLibrary::TryGetUnsupportedInline(Paragraph.Inlines[0], Unsupported) && Unsupported.SourceFragment == Case->Token);
		if (TestEqual(TEXT("One located warning"), Result.Diagnostics.Num(), 1)) { TestTrue(TEXT("Controlled fallback diagnostic"), Result.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Warning && Result.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedContent")) && Result.Diagnostics[0].JsonPointer == TEXT("/spell/0/entries/0")); }
	}
	else
	{
		FDnd5eAnnotationContent Annotation;
		if (!TestTrue(TEXT("Typed annotation"), UDnd5eContentLibrary::TryGetAnnotation(Paragraph.Inlines[0], Annotation))) { return false; }
		TestEqual(TEXT("Semantic kind"), Annotation.Kind, Case->Kind);
		TestTrue(TEXT("Original token retained"), Annotation.SourceAnnotation == Case->Token && Annotation.SourceFormat == TEXT("5etools"));
		TestTrue(TEXT("Handled literal produces no diagnostic"), Result.Diagnostics.IsEmpty());
		FDnd5eTextContent TextValue;
		TestTrue(TEXT("Normalized readable label"), Annotation.Display.Num() == 1 && UDnd5eContentLibrary::TryGetInlineText(Annotation.Display[0], TextValue) && TextValue.Text == Case->Display);
		if (Case->Kind == Dnd5e::Content::Annotation::DifficultyClass)
		{
			FDnd5eNumber Value; TestTrue(TEXT("Extract DC"), UDnd5eContentLibrary::TryGetAnnotationDifficultyClass(Annotation, Value));
			TestTrue(TEXT("Typed integer DC"), Value.Kind == EDnd5eNumberKind::Integer && Value.Integer == Case->Amount && !Value.SourceLiteral.IsEmpty());
		}
		else if (Case->Kind == Dnd5e::Content::Annotation::PercentChance)
		{
			FDnd5eChanceContentValue Value; TestTrue(TEXT("Extract chance"), UDnd5eContentLibrary::TryGetAnnotationPercentChance(Annotation, Value)); TestEqual(TEXT("Exact percent"), Value.Percent, Case->Amount);
			if (Case->Name.StartsWith(TEXT("ChanceOutcomes")) || Case->Name == TEXT("ChanceEmptyLabel")) { TestTrue(TEXT("Outcome labels and roll name retained"), Value.RollName == TEXT("Invented check") && Value.Success.Num() == 1 && Value.Failure.Num() == 1); }
		}
		else
		{
			FDnd5eD20Roll Value; TestTrue(TEXT("Extract d20 or attack value"), UDnd5eContentLibrary::TryGetAnnotationD20Roll(Annotation, Value));
			TestTrue(TEXT("D20 with signed literal modifier"), Value.DieSides == 20 && Value.Modifier.Kind == EDnd5eNumberKind::Integer && Value.Modifier.Integer == Case->Amount && Case->Token.Contains(Value.Modifier.SourceLiteral));
			if (Case->Name == TEXT("D20LabelName")) { TestEqual(TEXT("Roll name retained"), Value.RollName, FString(TEXT("Invented check"))); }
		}
	}
	TestTrue(TEXT("Request is immutable"), Equal(FDnd5eJsonSpellReadRequest::StaticStruct(), &Request, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAnnotationExtractorTest, "Dnd5e.Content.Annotation.NumericExtractorOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAnnotationExtractorTest::RunTest(const FString& Parameters)
{
	auto Check = [&]<typename T>(FGameplayTag Kind, bool (*Extract)(const FDnd5eAnnotationContent&, T&))
	{
		FDnd5eAnnotationContent Input; Input.Kind = Kind; Input.Value = MakeStorageProbe(T::StaticStruct()); const auto Before = Input;
		T Output; TestTrue(TEXT("Correct role/value extracted"), Extract(Input, Output)); TestTrue(TEXT("Copy contains full value"), Equal(T::StaticStruct(), &Output, Input.Value.GetMemory()));
		MutateOneLeaf(T::StaticStruct(), &Output); TestTrue(TEXT("Copy owns its value"), Equal(FDnd5eAnnotationContent::StaticStruct(), &Input, &Before));
		for (const auto& Model : ModelContracts())
		{
			if (Model.Type() == T::StaticStruct()) { continue; }
			Input = Before; Input.Value = MakeStorageProbe(Model.Type()); Output = MakeStorageProbe(T::StaticStruct()).template Get<T>();
			TestFalse(TEXT("Wrong payload rejected"), Extract(Input, Output)); const T Default{}; TestTrue(TEXT("Output completely reset"), Equal(T::StaticStruct(), &Output, &Default));
		}
		for (const auto InvalidKind : {FGameplayTag{}, Dnd5e::Content::Annotation::DiceRoll.GetTag(), FGameplayTag::RequestGameplayTag(TEXT("Dnd5e.Content.Annotation"))})
		{
			Input = Before; Input.Kind = InvalidKind; Output = MakeStorageProbe(T::StaticStruct()).template Get<T>(); const T Default{};
			TestTrue(TEXT("Wrong/parent role rejected and reset"), !Extract(Input, Output) && Equal(T::StaticStruct(), &Output, &Default));
		}
		Input = Before; Input.Value.Reset(); TestFalse(TEXT("Empty storage rejected"), Extract(Input, Output));
	};
	Check(Dnd5e::Content::Annotation::DifficultyClass, &UDnd5eContentLibrary::TryGetAnnotationDifficultyClass);
	Check(Dnd5e::Content::Annotation::AttackRoll, &UDnd5eContentLibrary::TryGetAnnotationD20Roll);
	Check(Dnd5e::Content::Annotation::D20Roll, &UDnd5eContentLibrary::TryGetAnnotationD20Roll);
	Check(Dnd5e::Content::Annotation::PercentChance, &UDnd5eContentLibrary::TryGetAnnotationPercentChance);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eChanceReferenceTraversalTest, "Dnd5e.Content.Annotation.ChanceReferenceTraversal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eChanceReferenceTraversalTest::RunTest(const FString& Parameters)
{
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(AnnotationRequest(TEXT("{@chance 35|{@spell Invented Label|SYN}|Invented roll|{@spell Invented Success|SYN}|{@spell Invented Failure|SYN}}")));
	if (!TestEqual(TEXT("Nested chance parses"), Result.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	TArray<FDnd5eContentReferenceOccurrence> Out; TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Chance outcomes are traversable"), UDnd5eSpellContentLibrary::CollectSpellReferences(Result.Spell, Out, Diagnostics));
	if (TestEqual(TEXT("Both outcomes and display references found"), Out.Num(), 3))
	{
		const FString Base = TEXT("/Description/0/Payload/Inlines/0/Payload");
		TestEqual(TEXT("Success before failure/display"), Out[0].ContentPath, Base + TEXT("/Value/Success/0/Payload"));
		TestEqual(TEXT("Failure next"), Out[1].ContentPath, Base + TEXT("/Value/Failure/0/Payload"));
		TestEqual(TEXT("Display last"), Out[2].ContentPath, Base + TEXT("/Display/0/Payload"));
	}
	FDnd5eAnnotationContent Invalid; Invalid.Kind = Dnd5e::Content::Annotation::PercentChance; Invalid.Value = FInstancedStruct::Make<FDnd5eNumber>();
	FDnd5eParagraphContent Body; Body.Inlines = {Inline(Dnd5e::Content::Inline::Annotation, Invalid)}; auto Spell = Result.Spell; Spell.Description = {Block(Dnd5e::Content::Block::Paragraph, Body)};
	TestTrue(TEXT("Mismatched chance value fails atomically"), !UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics) && Out.IsEmpty() && Diagnostics.Num() == 1);
	return !HasAnyErrors();
}

#endif
