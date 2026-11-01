// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Spells/Dnd5eJsonSpellReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

using namespace Dnd5eTests;

namespace
{
	FDnd5eJsonSpellReadRequest ScaledRequest(const FString& Content)
	{
		FDnd5eJsonSpellReadRequest Request; Request.SpellIndex = 0;
		Request.Context.DatasetNamespace = TEXT("scaling-synthetic"); Request.Context.RelativeFilePath = TEXT("spells/invented.json");
		TSharedPtr<FJsonObject> Root;
		const FString Json = TEXT(R"json({"spell":[{"name":"Invented Scale","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":[]}]})json");
		check(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root));
		Root->GetArrayField(TEXT("spell"))[0]->AsObject()->SetArrayField(TEXT("entries"), {MakeShared<FJsonValueString>(Content)});
		FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Request.JsonText)); return Request;
	}
	struct FScaleCase
	{
		FString Name; FString Token; TArray<int32> Choices; TArray<FString> Bases;
		FString Progression; FString Increment; FString Mode; FString Display; bool bDamage = false;
	};
	TArray<FScaleCase> ScaleCases()
	{
		TArray<FScaleCase> Cases = {
			{TEXT("Dice"), TEXT("{@scaledice 2d6|2-4|1d6}"), {2,3,4}, {TEXT("2d6")}, TEXT("2-4"), TEXT("1d6"), TEXT(""), TEXT("1d6")},
			{TEXT("DamageAlternatives"), TEXT("{@scaledamage 2d6;3d6|2-4,6|1d6||invented label}"), {2,3,4,6}, {TEXT("2d6"), TEXT("3d6")}, TEXT("2-4,6"), TEXT("1d6"), TEXT(""), TEXT("invented label"), true},
			{TEXT("Psi"), TEXT("{@scaledice 1d6|1-3|1d4|psi}"), {1,2,3}, {TEXT("1d6")}, TEXT("1-3"), TEXT("1d4"), TEXT("psi"), TEXT("1d4")},
			{TEXT("Single"), TEXT("{@scaledice 1|9|2}"), {9}, {TEXT("1")}, TEXT("9"), TEXT("2"), TEXT(""), TEXT("2")},
			{TEXT("SortedUnique"), TEXT("{@scaledice 1d4|6,2-4,3|1d4}"), {2,3,4,6}, {TEXT("1d4")}, TEXT("6,2-4,3"), TEXT("1d4"), TEXT(""), TEXT("1d4")},
			{TEXT("WhitespacePreserved"), TEXT("{@scaledice  1d4 ; 2d4 | 2 - 4 , 6 | 1d4 }"), {2,3,4,6}, {TEXT(" 1d4 "), TEXT(" 2d4 ")}, TEXT(" 2 - 4 , 6 "), TEXT(" 1d4 "), TEXT(""), TEXT(" 1d4 ")},
			{TEXT("EmptyLabelUsesIncrement"), TEXT("{@scaledice 1d4|1-2|1d4||}"), {1,2}, {TEXT("1d4")}, TEXT("1-2"), TEXT("1d4"), TEXT(""), TEXT("1d4")},
			{TEXT("UnevaluatedExpressions"), TEXT("{@scaledice 2d6+PB|1,3|1d6+1}"), {1,3}, {TEXT("2d6+PB")}, TEXT("1,3"), TEXT("1d6+1"), TEXT(""), TEXT("1d6+1")}
		};
		const TArray<TPair<FString,FString>> Invalid = {
			{TEXT("Zero"), TEXT("0")}, {TEXT("Ten"), TEXT("10")}, {TEXT("Negative"), TEXT("-2")}, {TEXT("Reversed"), TEXT("4-2")},
			{TEXT("EmptyMaximum"), TEXT("2-")}, {TEXT("EmptyPiece"), TEXT("2,,4")}, {TEXT("TrailingComma"), TEXT("2,")},
			{TEXT("MultipleRanges"), TEXT("2-4-6")}, {TEXT("Plus"), TEXT("+2")}, {TEXT("Decimal"), TEXT("2.5")},
			{TEXT("Exponent"), TEXT("2e0")}, {TEXT("LeadingZero"), TEXT("01")}, {TEXT("Overflow"), TEXT("999999999999999999")}
		};
		for (const auto& Case : Invalid) { Cases.Add({Case.Key, FString(TEXT("{@scaledice 1d4|")) + Case.Value + TEXT("|1d4}")}); }
		const TArray<TPair<FString,FString>> InvalidTokens = {
			{TEXT("MissingPart"), TEXT("{@scaledice 1d4|1-3}")}, {TEXT("ExtraPart"), TEXT("{@scaledice 1d4|1-3|1d4|psi|label|extra}")},
			{TEXT("EmptyBase"), TEXT("{@scaledice |1-3|1d4}")}, {TEXT("EmptyAlternative"), TEXT("{@scaledice 1d4;|1-3|1d4}")},
			{TEXT("EmptyProgression"), TEXT("{@scaledice 1d4||1d4}")}, {TEXT("EmptyIncrement"), TEXT("{@scaledice 1d4|1-3| }")},
			{TEXT("UnknownMode"), TEXT("{@scaledice 1d4|1-3|1d4|unknown}")}, {TEXT("WrongModeCase"), TEXT("{@scaledice 1d4|1-3|1d4|PSI}")},
			{TEXT("WrongTagCase"), TEXT("{@SCALEDICE 1d4|1-3|1d4}")}
		};
		for (const auto& Case : InvalidTokens) { Cases.Add({Case.Key, Case.Value}); }
		return Cases;
	}
	void CountSourceScaling(const TSharedPtr<FJsonValue>& Value, int32& Dice, int32& Damage)
	{
		if (Value->Type == EJson::String)
		{
			const FString Text = Value->AsString();
			for (const auto& Pair : TArray<TPair<FString,int32*>>{{TEXT("{@scaledice "), &Dice}, {TEXT("{@scaledamage "), &Damage}})
			{
				int32 Position = 0; while ((Position = Text.Find(Pair.Key, ESearchCase::CaseSensitive, ESearchDir::FromStart, Position)) != INDEX_NONE) { ++*Pair.Value; Position += Pair.Key.Len(); }
			}
		}
		else if (Value->Type == EJson::Array) { for (const auto& Child : Value->AsArray()) { CountSourceScaling(Child, Dice, Damage); } }
		else if (Value->Type == EJson::Object) { for (const auto& Pair : Value->AsObject()->Values) { CountSourceScaling(Pair.Value, Dice, Damage); } }
	}
	void CountTypedScaling(FAutomationTestBase& Test, const UScriptStruct* Type, const void* Data, int32& Dice, int32& Damage)
	{
		if (Type == FDnd5eAnnotationContent::StaticStruct())
		{
			const auto& Annotation = *static_cast<const FDnd5eAnnotationContent*>(Data);
			if (Annotation.Kind == Dnd5e::Content::Annotation::ScaledDiceRoll || Annotation.Kind == Dnd5e::Content::Annotation::ScaledDamageRoll)
			{
				FDnd5eScaledRoll Out; Test.TestTrue(TEXT("Every counted annotation has a typed scaling value"), UDnd5eContentLibrary::TryGetAnnotationScaledRoll(Annotation, Out));
				if (Annotation.Kind == Dnd5e::Content::Annotation::ScaledDiceRoll) { ++Dice; } else { ++Damage; }
			}
		}
		if (Type == FInstancedStruct::StaticStruct())
		{
			const auto& Value = *static_cast<const FInstancedStruct*>(Data);
			if (Value.IsValid()) { CountTypedScaling(Test, Value.GetScriptStruct(), Value.GetMemory(), Dice, Damage); } return;
		}
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			if (const auto* Struct = CastField<FStructProperty>(*It)) { CountTypedScaling(Test, Struct->Struct, Struct->ContainerPtrToValuePtr<void>(Data), Dice, Damage); }
			else if (const auto* Array = CastField<FArrayProperty>(*It))
			{
				if (const auto* Element = CastField<FStructProperty>(Array->Inner))
				{
					FScriptArrayHelper Values(Array, Array->ContainerPtrToValuePtr<void>(Data));
					for (int32 Index = 0; Index < Values.Num(); ++Index) { CountTypedScaling(Test, Element->Struct, Values.GetRawPtr(Index), Dice, Damage); }
				}
			}
		}
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eScaledReaderTest, "Dnd5e.Json.Annotations.ScaledRollValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eScaledReaderTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : ScaleCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}
bool FDnd5eScaledReaderTest::RunTest(const FString& Parameters)
{
	const auto Cases = ScaleCases(); const auto* Case = Cases.FindByPredicate([&](const auto& Value) { return Value.Name == Parameters; });
	if (!TestNotNull(TEXT("Known scaling case"), Case)) { return false; }
	const auto Request = ScaledRequest(Case->Token); const auto Before = Request;
	const auto Read = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestEqual(TEXT("Supported or retained syntax reads"), Read.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eParagraphContent Body;
	if (!TestTrue(TEXT("One inline payload"), Read.Spell.Description.Num() == 1 && UDnd5eContentLibrary::TryGetParagraph(Read.Spell.Description[0], Body) && Body.Inlines.Num() == 1)) { return false; }
	if (Case->Choices.IsEmpty())
	{
		FDnd5eUnsupportedContent Unsupported;
		TestTrue(TEXT("Complete token retained on unsupported grammar"), UDnd5eContentLibrary::TryGetUnsupportedInline(Body.Inlines[0], Unsupported) && Unsupported.SourceFragment == Case->Token);
		TestTrue(TEXT("One controlled fallback warning, not duplicated dispatch"), Read.Diagnostics.Num() == 1 && Read.Diagnostics[0].Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedContent")) && Read.Diagnostics[0].JsonPointer == TEXT("/spell/0/entries/0"));
	}
	else
	{
		FDnd5eAnnotationContent Annotation; FDnd5eScaledRoll Value;
		if (!TestTrue(TEXT("Extract typed scaling annotation"), UDnd5eContentLibrary::TryGetAnnotation(Body.Inlines[0], Annotation) && UDnd5eContentLibrary::TryGetAnnotationScaledRoll(Annotation, Value))) { return false; }
		TestTrue(TEXT("Kind and whole token preserved"), Annotation.Kind == (Case->bDamage ? Dnd5e::Content::Annotation::ScaledDamageRoll : Dnd5e::Content::Annotation::ScaledDiceRoll) && Annotation.SourceFormat == TEXT("5etools") && Annotation.SourceAnnotation == Case->Token);
		TestTrue(TEXT("Sorted unique choices and original progression"), Value.InputValues == Case->Choices && Value.SourceProgression == Case->Progression);
		TestTrue(TEXT("Source mode and semantic axis"), Value.SourceRenderMode == Case->Mode && Value.InputKind == (Case->Mode.IsEmpty() ? Dnd5e::Scaling::Input::SpellSlotLevel : Dnd5e::Scaling::Input::PsiPoints));
		TArray<FString> Bases; for (const auto& Base : Value.BaseExpressions) { Bases.Add(Base.Expression); TestEqual(TEXT("Source expression syntax"), Base.Syntax, FString(TEXT("5etools"))); }
		TestTrue(TEXT("Alternatives kept separately"), Bases == Case->Bases);
		TestTrue(TEXT("Increment remains unevaluated"), Value.Increment.Expression == Case->Increment && Value.Increment.Syntax == TEXT("5etools"));
		FDnd5eTextContent Display; TestTrue(TEXT("Explicit/default label"), Annotation.Display.Num() == 1 && UDnd5eContentLibrary::TryGetInlineText(Annotation.Display[0], Display) && Display.Text == Case->Display);
		TestTrue(TEXT("No unsupported warning for handled form"), Read.Diagnostics.IsEmpty());
	}
	TestTrue(TEXT("Request immutable"), Equal(FDnd5eJsonSpellReadRequest::StaticStruct(), &Request, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eScaledExtractorTest, "Dnd5e.Content.Annotation.ScaledExtractorAndDisplayTraversal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eScaledExtractorTest::RunTest(const FString& Parameters)
{
	FDnd5eAnnotationContent Annotation; Annotation.Value = MakeStorageProbe(FDnd5eScaledRoll::StaticStruct());
	for (const auto Kind : {Dnd5e::Content::Annotation::ScaledDiceRoll.GetTag(), Dnd5e::Content::Annotation::ScaledDamageRoll.GetTag()})
	{
		Annotation.Kind = Kind; const auto Before = Annotation; FDnd5eScaledRoll Out;
		TestTrue(TEXT("Both roles share exact typed extraction"), UDnd5eContentLibrary::TryGetAnnotationScaledRoll(Annotation, Out) && Equal(FDnd5eScaledRoll::StaticStruct(), &Out, Annotation.Value.GetMemory()));
		MutateOneLeaf(FDnd5eScaledRoll::StaticStruct(), &Out); TestTrue(TEXT("Independent copy"), Equal(FDnd5eAnnotationContent::StaticStruct(), &Annotation, &Before));
	}
	for (const auto& Model : ModelContracts())
	{
		if (Model.Type() == FDnd5eScaledRoll::StaticStruct()) { continue; }
		Annotation.Value = MakeStorageProbe(Model.Type()); auto Out = MakeStorageProbe(FDnd5eScaledRoll::StaticStruct()).Get<FDnd5eScaledRoll>(); const FDnd5eScaledRoll Default;
		TestTrue(TEXT("Wrong payload rejected and reset"), !UDnd5eContentLibrary::TryGetAnnotationScaledRoll(Annotation, Out) && Equal(FDnd5eScaledRoll::StaticStruct(), &Out, &Default));
	}
	Annotation.Value = MakeStorageProbe(FDnd5eScaledRoll::StaticStruct());
	for (const auto Kind : {FGameplayTag{}, Dnd5e::Content::Annotation::DiceRoll.GetTag(), FGameplayTag::RequestGameplayTag(TEXT("Dnd5e.Content.Annotation"))})
	{
		Annotation.Kind = Kind; FDnd5eScaledRoll Out; TestFalse(TEXT("Other/parent role rejected"), UDnd5eContentLibrary::TryGetAnnotationScaledRoll(Annotation, Out));
	}
	Annotation.Kind = Dnd5e::Content::Annotation::ScaledDiceRoll; Annotation.Value.Reset(); FDnd5eScaledRoll Out;
	TestFalse(TEXT("Empty storage rejected"), UDnd5eContentLibrary::TryGetAnnotationScaledRoll(Annotation, Out));
	auto Read = FDnd5eJsonSpellReader::ReadSpell(ScaledRequest(TEXT("{@scaledice 2d6|1-3|1d6||{@spell Invented Target|SYN}}")));
	if (!TestEqual(TEXT("Nested display parses through shared scanner"), Read.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	auto& Body = Read.Spell.Description[0].Payload.GetMutable<FDnd5eParagraphContent>();
	Body.Inlines[0].Payload.GetMutable<FDnd5eAnnotationContent>().Value.GetMutable<FDnd5eScaledRoll>().Increment.Expression = TEXT("{@spell Opaque Expression|SYN}");
	TArray<FDnd5eContentReferenceOccurrence> Occurrences; TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Only Content display references traverse; expression strings remain opaque"), UDnd5eSpellContentLibrary::CollectSpellReferences(Read.Spell, Occurrences, Diagnostics) && Occurrences.Num() == 1 && Occurrences[0].ContentPath == TEXT("/Description/0/Payload/Inlines/0/Payload/Display/0/Payload"));
	FString Deep = TEXT("label"); for (int32 Index = 0; Index < 65; ++Index) { Deep = TEXT("{@b ") + Deep + TEXT("}"); }
	Read = FDnd5eJsonSpellReader::ReadSpell(ScaledRequest(TEXT("{@scaledice 1d4|1-3|1d4||") + Deep + TEXT("}"))); const FDnd5eSpell DefaultSpell;
	TestTrue(TEXT("Recursive display limit fails without partial spell"), Read.Status == EDnd5eJsonReadStatus::Failure && Equal(FDnd5eSpell::StaticStruct(), &Read.Spell, &DefaultSpell));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eExternalScalingTest, "Dnd5e.Json.ExternalScaling", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eExternalScalingTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Directory; if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory)) { Names.Add(TEXT("SuppliedSpellDirectory")); Commands.Add(Directory); }
}
bool FDnd5eExternalScalingTest::RunTest(const FString& Directory)
{
	TArray<FString> Files; IFileManager::Get().FindFiles(Files, *(Directory / TEXT("spells-*.json")), true, false); Files.Sort();
	if (!TestTrue(TEXT("Selected directory has files"), !Files.IsEmpty())) { return false; }
	int32 ExpectedDice = 0, ExpectedDamage = 0, ActualDice = 0, ActualDamage = 0, Records = 0;
	for (const auto& File : Files)
	{
		FDnd5eJsonSpellReadRequest Request; Request.Context.DatasetNamespace = TEXT("external-scaling-audit"); Request.Context.RelativeFilePath = File;
		if (!TestTrue(TEXT("Load explicit file"), FFileHelper::LoadFileToString(Request.JsonText, *(Directory / File)))) { return false; }
		TSharedPtr<FJsonObject> Root; if (!TestTrue(TEXT("Object document"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Request.JsonText), Root))) { return false; }
		const auto& Values = Root->GetArrayField(TEXT("spell"));
		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			CountSourceScaling(Values[Index], ExpectedDice, ExpectedDamage); Request.SpellIndex = Index;
			const auto Read = FDnd5eJsonSpellReader::ReadSpell(Request);
			if (!TestEqual(TEXT("Spell conversion succeeds"), Read.Status, EDnd5eJsonReadStatus::Success)) { return false; }
			CountTypedScaling(*this, FDnd5eSpell::StaticStruct(), &Read.Spell, ActualDice, ActualDamage); ++Records;
		}
	}
	TestEqual(TEXT("Every raw scaledice token has a typed annotation"), ActualDice, ExpectedDice);
	TestEqual(TEXT("Every raw scaledamage token has a typed annotation"), ActualDamage, ExpectedDamage);
	TestTrue(TEXT("Dataset exercises scaling"), ExpectedDice + ExpectedDamage > 0);
	AddInfo(FString::Printf(TEXT("Verified %d scaledice and %d scaledamage typed annotations across %d spells; no source passages logged."), ActualDice, ActualDamage, Records));
	return !HasAnyErrors();
}

#endif
