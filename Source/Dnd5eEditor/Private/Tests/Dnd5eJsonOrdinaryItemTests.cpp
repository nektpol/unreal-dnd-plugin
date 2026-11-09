// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "Readers/Items/Dnd5eJsonItemLibrary.h"

namespace
{
	FDnd5eJsonDocument StandaloneItemDocument(const TCHAR* Text)
	{
		FDnd5eJsonDocument Document;
		Document.Context.DatasetNamespace = TEXT("ordinary-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/items.json");
		Document.JsonText = Text;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eOrdinaryItemConversionTest, "Dnd5e.Items.OrdinaryJson.Conversion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eOrdinaryItemConversionTest::RunTest(const FString& Parameters)
{
	const auto Document = StandaloneItemDocument(TEXT(R"json({"item":[
		{"name":"Charm","source":"SYN","rarity":"rare","reqAttune":"Invented condition",
		 "reqAttuneTags":[{"background":"SYN"}]},
		{"name":"Thrower","source":"SYN","rarity":"uncommon","type":"T1","range":"20",
		 "dmg1":"1d6","dmgType":"O","reqAttune":false,"reqAttuneAlt":"optional"},
		{"name":"Plain","source":"SYN","rarity":"unknown (magic)"}
	]})json"));
	const auto Charm = UDnd5eJsonItemLibrary::ReadItemAtIndex(Document, 0);
	if (!TestTrue(TEXT("Read selected row without a type"), Charm.Status == EDnd5eJsonReadStatus::Success &&
		!Charm.Item.bHasType && Charm.Item.Rarity == Dnd5e::Item::Rarity::Rare &&
		Charm.Item.Header.Provenance.Num() == 1)) { return false; }
	const auto& Attunement = Charm.Item.Attunement;
	TestTrue(TEXT("Readable condition and typed index coexist"), Attunement.Mode == EDnd5eAttunementMode::Required &&
		Attunement.Conditions.Num() == 1 && Attunement.bHasIndex && Attunement.Index.bIsFullyRepresented &&
		Attunement.Index.Groups.Num() == 1 && Attunement.Index.Groups[0].RecordRequirements.Num() == 1 &&
		Attunement.Index.Groups[0].RecordRequirements[0].Category == Dnd5e::Category::Background &&
		!Charm.Item.Header.Provenance[0].AdditionalMetadataJson.Contains(TEXT("reqAttuneTags")));
	TestEqual(TEXT("Selected row pointer"), Charm.Item.Header.Provenance[0].JsonPointer, FString(TEXT("/item/0")));

	const auto Weapon = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	TestTrue(TEXT("Single range and additional damage code are typed"), Weapon.Status == EDnd5eJsonReadStatus::Success &&
		Weapon.Item.WeaponData.bHasRange && !Weapon.Item.WeaponData.Range.bHasLong &&
		Weapon.Item.WeaponData.Range.Normal.Amount.Integer == 20 &&
		Weapon.Item.WeaponData.PrimaryDamage.Type == Dnd5e::Damage::Type::Force);
	TestTrue(TEXT("False primary and optional alternative keep independent presence"),
		Weapon.Item.Attunement.Mode == EDnd5eAttunementMode::NotRequired && Weapon.Item.bHasAlternativeAttunement &&
		Weapon.Item.AlternativeAttunement.Mode == EDnd5eAttunementMode::Optional);
	const auto Plain = FDnd5eJsonItemReader::ReadAtIndex(Document, 2);
	TestTrue(TEXT("Omitted attunement and explicit rarity stay distinct"), Plain.Status == EDnd5eJsonReadStatus::Success &&
		Plain.Item.Attunement.Mode == EDnd5eAttunementMode::Unspecified && !Plain.Item.bHasAlternativeAttunement &&
		Plain.Item.Rarity == Dnd5e::Item::Rarity::UnknownMagic);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eOrdinaryItemFailureTest, "Dnd5e.Items.OrdinaryJson.SelectionAndFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eOrdinaryItemFailureTest::RunTest(const FString& Parameters)
{
	const auto Document = StandaloneItemDocument(TEXT(R"json({"item":[
		{"name":"Dependent","source":"SYN","rarity":"none","baseItem":"Base|SYN"},
		{"name":"Copy","source":"SYN","rarity":"none","_copy":{}},
		{"name":"Bad condition","source":"SYN","rarity":"none","reqAttune":7},
		{"name":"Valid","source":"SYN","rarity":"none"}
	]})json"));
	struct FCase { int32 Index; const TCHAR* Pointer; };
	for (const FCase& Case : {FCase{-1, TEXT("/item")}, {4, TEXT("/item")},
		{0, TEXT("/item/0/baseItem")}, {1, TEXT("/item/1/_copy")}, {2, TEXT("/item/2/reqAttune")}})
	{
		const auto Result = FDnd5eJsonItemReader::ReadAtIndex(Document, Case.Index);
		TestTrue(FString::Printf(TEXT("Failure clears output and identifies %s"), Case.Pointer),
			Result.Status == EDnd5eJsonReadStatus::Failure && Result.Item.Header.RecordId == FDnd5eRecordId{} &&
			Result.Diagnostics.Num() == 1 && Result.Diagnostics[0].JsonPointer == Case.Pointer);
	}
	TestTrue(TEXT("A valid row can be selected after unsupported rows"),
		FDnd5eJsonItemReader::ReadAtIndex(Document, 3).Status == EDnd5eJsonReadStatus::Success);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eOrdinaryItemAttunementIndexTest, "Dnd5e.Items.OrdinaryJson.AttunementIndex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eOrdinaryItemAttunementIndexTest::RunTest(const FString& Parameters)
{
	const auto Document = StandaloneItemDocument(TEXT(R"json({"item":[{
		"name":"Invented Lens","source":"SYN","rarity":"rare","reqAttune":false,
		"reqAttuneTags":[
			{"class":"Invented Class|SYN","race":"Invented Species","spellcasting":false,"psionics":true,"int":13,
			 "alignment":["G"],"size":"S","languageProficiency":"invented language","creatureType":"fey"},
			{"skillProficiency":"Invented Skill","alignment":["E"],"creatureType":"humanoid"}],
		"reqAttuneAltTags":[{"background":"Invented Background|ALT"}]
	}]})json"));
	const auto Result = FDnd5eJsonItemReader::ReadAtIndex(Document, 0);
	const auto& Primary = Result.Item.Attunement;
	const auto& Alternative = Result.Item.AlternativeAttunement;
	if (!TestTrue(TEXT("Both indexes are typed with their own group boundaries"),
		Result.Status == EDnd5eJsonReadStatus::Success && Result.Diagnostics.IsEmpty() &&
		Primary.bHasIndex && Primary.Index.bIsFullyRepresented && Primary.Index.Groups.Num() == 2 &&
		Result.Item.bHasAlternativeAttunement && Alternative.bHasIndex && Alternative.Index.bIsFullyRepresented &&
		Alternative.Index.Groups.Num() == 1)) { return false; }

	TestTrue(TEXT("Index presence leaves each readable mode intact"), Primary.Mode == EDnd5eAttunementMode::NotRequired &&
		Alternative.Mode == EDnd5eAttunementMode::Unspecified);
	const auto& First = Primary.Index.Groups[0];
	const auto& Second = Primary.Index.Groups[1];
	const auto& AlternateGroup = Alternative.Index.Groups[0];
	TestTrue(TEXT("Class and species selectors keep explicit source presence"), First.RecordRequirements.Num() == 2 &&
		First.RecordRequirements[0].Category == Dnd5e::Category::Class && First.RecordRequirements[0].bHasSource &&
		First.RecordRequirements[0].Name == TEXT("Invented Class") && First.RecordRequirements[0].SourceId == TEXT("SYN") &&
		First.RecordRequirements[1].Category == Dnd5e::Category::Species && !First.RecordRequirements[1].bHasSource &&
		First.RecordRequirements[1].Name == TEXT("Invented Species"));
	TestTrue(TEXT("Boolean presence, intelligence, language, creature type and size are typed"),
		First.bHasSpellcasting && !First.bSpellcasting && First.bHasPsionics && First.bPsionics &&
		First.MinimumAbilityScores.Num() == 1 && First.MinimumAbilityScores[0].Ability == Dnd5e::Ability::Intelligence &&
		First.MinimumAbilityScores[0].Minimum == 13 && First.LanguageProficiencies == TArray<FString>{TEXT("invented language")} &&
		First.CreatureType == Dnd5e::Creature::Type::Fey && First.Size == EDnd5eSize::Small);
	TestTrue(TEXT("Groups retain their own selectors and alignment traits"), First.bHasAlignmentFilter &&
		First.AlignmentOptions.Num() == 1 && First.AlignmentOptions[0].RequiredTraits.HasTagExact(Dnd5e::Alignment::Good) &&
		Second.bHasAlignmentFilter && Second.AlignmentOptions.Num() == 1 &&
		Second.AlignmentOptions[0].RequiredTraits.HasTagExact(Dnd5e::Alignment::Evil) &&
		Second.RecordRequirements.Num() == 1 && Second.RecordRequirements[0].Category == Dnd5e::Category::Skill &&
		Second.CreatureType == Dnd5e::Creature::Type::Humanoid && !Second.bHasSpellcasting);
	TestTrue(TEXT("Alternative index preserves its background selector"), AlternateGroup.RecordRequirements.Num() == 1 &&
		AlternateGroup.RecordRequirements[0].Category == Dnd5e::Category::Background &&
		AlternateGroup.RecordRequirements[0].bHasSource && AlternateGroup.RecordRequirements[0].SourceId == TEXT("ALT"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eOrdinaryItemAttunementFallbackTest, "Dnd5e.Items.OrdinaryJson.AttunementFallback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eOrdinaryItemAttunementFallbackTest::RunTest(const FString& Parameters)
{
	const auto Document = StandaloneItemDocument(TEXT(R"json({"item":[
		{"name":"Partial","source":"SYN","rarity":"none","reqAttune":"Invented condition",
		 "reqAttuneTags":[{"class":"Broken|SYN|EXTRA","spellcasting":true,"x/y~z":null},{},5],
		 "reqAttuneAltTags":null},
		{"name":"Empty","source":"SYN","rarity":"none","reqAttuneTags":[]},
		{"name":"Absent","source":"SYN","rarity":"none"}
	]})json"));
	const auto Partial = FDnd5eJsonItemReader::ReadAtIndex(Document, 0);
	const auto& Primary = Partial.Item.Attunement;
	const auto& Alternative = Partial.Item.AlternativeAttunement;
	if (!TestTrue(TEXT("Partial indexes retain evidence at the correct level"),
		Partial.Status == EDnd5eJsonReadStatus::Success && Primary.bHasIndex && !Primary.Index.bIsFullyRepresented &&
		Primary.Index.Groups.Num() == 1 && Primary.Index.UnsupportedContent.Num() == 2 &&
		Partial.Item.bHasAlternativeAttunement && Alternative.bHasIndex && !Alternative.Index.bIsFullyRepresented &&
		Alternative.Index.Groups.IsEmpty() && Alternative.Index.UnsupportedContent.Num() == 1 &&
		Partial.Item.Header.Provenance.Num() == 1))
	{
		for (const auto& Diagnostic : Partial.Diagnostics)
		{
			AddInfo(FString::Printf(TEXT("%s %s"), *Diagnostic.JsonPointer, *Diagnostic.Message));
		}
		return false;
	}
	const auto& Group = Primary.Index.Groups[0];
	TestTrue(TEXT("Mixed group keeps supported fields and separate JSON fragments"), Group.bHasSpellcasting && Group.bSpellcasting &&
		Group.RecordRequirements.IsEmpty() && Group.UnsupportedContent.Num() == 2 &&
		Group.UnsupportedContent[0].FragmentKind == EDnd5eSourceFragmentKind::Json &&
		Group.UnsupportedContent[0].SourceFragment.Contains(TEXT("Broken|SYN|EXTRA")) &&
		Group.UnsupportedContent[1].SourceFragment.Contains(TEXT("x/y~z")) &&
		Primary.Index.UnsupportedContent[0].SourceFragment.TrimStartAndEnd().StartsWith(TEXT("{")) &&
		Primary.Index.UnsupportedContent[0].SourceFragment.TrimStartAndEnd().EndsWith(TEXT("}")) &&
		Primary.Index.UnsupportedContent[1].SourceFragment == TEXT("5") &&
		Alternative.Index.UnsupportedContent[0].SourceFragment == TEXT("null"));
	TestTrue(TEXT("Fallback leaves readable conditions and consumes original index keys"),
		Primary.Mode == EDnd5eAttunementMode::Required && Primary.Conditions.Num() == 1 &&
		!Partial.Item.Header.Provenance[0].AdditionalMetadataJson.Contains(TEXT("reqAttuneTags")) &&
		!Partial.Item.Header.Provenance[0].AdditionalMetadataJson.Contains(TEXT("reqAttuneAltTags")));
	const TArray<FString> ExpectedPointers{TEXT("/item/0/reqAttuneTags/0/class"), TEXT("/item/0/reqAttuneTags/0/x~1y~0z"),
		TEXT("/item/0/reqAttuneTags/1"), TEXT("/item/0/reqAttuneTags/2"), TEXT("/item/0/reqAttuneAltTags")};
	TArray<FString> ActualPointers;
	for (const auto& Diagnostic : Partial.Diagnostics)
	{
		TestTrue(TEXT("Retained fragment produces a warning"), Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning);
		ActualPointers.Add(Diagnostic.JsonPointer);
	}
	TestTrue(TEXT("Warnings identify each fragment in deterministic order"), ActualPointers == ExpectedPointers);

	const auto Empty = FDnd5eJsonItemReader::ReadAtIndex(Document, 1);
	const auto Absent = FDnd5eJsonItemReader::ReadAtIndex(Document, 2);
	TestTrue(TEXT("Explicit empty index and omitted index stay distinct"), Empty.Status == EDnd5eJsonReadStatus::Success &&
		Empty.Item.Attunement.bHasIndex && Empty.Item.Attunement.Index.bIsFullyRepresented &&
		Empty.Item.Attunement.Index.Groups.IsEmpty() && Empty.Diagnostics.IsEmpty() &&
		Absent.Status == EDnd5eJsonReadStatus::Success && !Absent.Item.Attunement.bHasIndex &&
		!Absent.Item.bHasAlternativeAttunement && Absent.Diagnostics.IsEmpty());
	return !HasAnyErrors();
}
#endif
