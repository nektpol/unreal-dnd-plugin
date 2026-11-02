// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
using namespace Dnd5eTests;

namespace
{
	FDnd5eAlignmentRequirement Option(const TArray<FGameplayTag>& Traits)
	{
		FDnd5eAlignmentRequirement Out; for (const auto& Trait : Traits) { Out.RequiredTraits.AddTag(Trait); } return Out;
	}
	FDnd5eUnsupportedContent Fragment(const TCHAR* Json)
	{
		FDnd5eUnsupportedContent Out; Out.SourceFormat = TEXT("synthetic"); Out.FragmentKind = EDnd5eSourceFragmentKind::Json; Out.SourceFragment = Json; return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAlignmentStorageTest, "Dnd5e.Items.Attunement.AlignmentCombinations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAlignmentStorageTest::RunTest(const FString& Parameters)
{
	// Freeze storage encodings, not a character matcher: singleton Neutral means True Neutral.
	using namespace Dnd5e::Alignment;
	const TArray<FDnd5eAlignmentRequirement> Options = {
		Option({Lawful, Good}), Option({Neutral, Good}), Option({Chaotic, Good}),
		Option({Lawful, Neutral}), Option({Neutral}), Option({Chaotic, Neutral}),
		Option({Lawful, Evil}), Option({Neutral, Evil}), Option({Chaotic, Evil}), Option({Unaligned})
	};
	for (int32 I = 0; I < Options.Num(); ++I)
	{
		FDnd5eAlignmentRequirement Restored;
		if (!RoundTrip(*this, FDnd5eAlignmentRequirement::StaticStruct(), &Options[I], &Restored)) { return false; }
		TestTrue(TEXT("Exact trait combinations survive serialization"), Restored.RequiredTraits == Options[I].RequiredTraits);
		for (int32 J = I + 1; J < Options.Num(); ++J) { TestFalse(TEXT("Concrete alignment encodings remain distinct"), Restored.RequiredTraits == Options[J].RequiredTraits); }
	}
	TestEqual(TEXT("True Neutral has one explicit Neutral tag"), Options[4].RequiredTraits.Num(), 1);
	TestTrue(TEXT("Neutral Good retains both traits"), Options[1].RequiredTraits.HasTagExact(Neutral) && Options[1].RequiredTraits.HasTagExact(Good));
	TestTrue(TEXT("Lawful Neutral retains both traits"), Options[3].RequiredTraits.HasTagExact(Lawful) && Options[3].RequiredTraits.HasTagExact(Neutral));
	TestTrue(TEXT("Checking only for Neutral would also find paired forms"), Options[1].RequiredTraits.HasAllExact(Options[4].RequiredTraits));
	FDnd5eAttunementIndexGroup Group; Group.bHasAlignmentFilter = true; Group.AlignmentOptions = {Options[0], Options[8]};
	FDnd5eAttunementIndexGroup Out;
	if (!RoundTrip(*this, FDnd5eAttunementIndexGroup::StaticStruct(), &Group, &Out)) { return false; }
	if (!TestEqual(TEXT("Lawful Good OR Chaotic Evil remains two options"), Out.AlignmentOptions.Num(), 2)) { return false; }
	TestTrue(TEXT("Option pairings are preserved"), Out.AlignmentOptions[0].RequiredTraits == Options[0].RequiredTraits && Out.AlignmentOptions[1].RequiredTraits == Options[8].RequiredTraits);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAttunementOwnershipTest, "Dnd5e.Items.Attunement.GroupedOwnershipAndRetention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAttunementOwnershipTest::RunTest(const FString& Parameters)
{
	FDnd5eItem Item; Item.Attunement.Mode = EDnd5eAttunementMode::Required; Item.Attunement.Conditions.Add(Text(TEXT("Original invented wording"))); Item.Attunement.bHasIndex = true;
	FDnd5eAttunementIndexGroup First; FDnd5eRecordSelector Selector; Selector.Category = Dnd5e::Category::Class; Selector.Name = TEXT("Invented Navigator"); Selector.bHasSource = true; Selector.SourceId = TEXT("SYN");
	First.RecordRequirements.Add(Selector); Selector.Category = Dnd5e::Category::Skill; Selector.Name = TEXT("Invented Skill"); Selector.bHasSource = false; Selector.SourceId.Reset(); First.RecordRequirements.Add(Selector);
	First.LanguageProficiencies.Add(TEXT("invented-language")); First.bHasAlignmentFilter = true; First.AlignmentOptions.Add(Option({Dnd5e::Alignment::Good}));
	FDnd5eAbilityScoreMinimum Minimum; Minimum.Ability = Dnd5e::Ability::Intelligence; Minimum.Minimum = 12; First.MinimumAbilityScores.Add(Minimum);
	First.UnsupportedContent.Add(Fragment(TEXT("{\"inventedRequirement\":true}")));
	FDnd5eAttunementIndexGroup Second; Second.bHasSpellcasting = true; Second.bSpellcasting = true; Second.bHasPsionics = true; Second.bPsionics = false;
	Item.Attunement.Index.Groups = {First, Second}; Item.Attunement.Index.UnsupportedContent.Add(Fragment(TEXT("17"))); // Independent outer retention.
	Item.bHasAlternativeAttunement = true; Item.AlternativeAttunement.Mode = EDnd5eAttunementMode::Optional; Item.AlternativeAttunement.bHasIndex = true;
	FDnd5eAttunementIndexGroup Alternative; Alternative.Size = EDnd5eSize::Small; Alternative.CreatureType = Dnd5e::Creature::Type::Humanoid; Alternative.bHasAlignmentFilter = true; Alternative.AlignmentOptions.Add(Option({Dnd5e::Alignment::Neutral}));
	Item.AlternativeAttunement.Index.Groups.Add(Alternative); Item.AlternativeAttunement.Index.bIsFullyRepresented = true;
	const auto Before = Item; FDnd5eItem Out;
	if (!RoundTrip(*this, FDnd5eItem::StaticStruct(), &Item, &Out)) { return false; }
	if (!TestTrue(TEXT("Nested primary/alternative indexes and fragments round trip"), Equal(FDnd5eItem::StaticStruct(), &Item, &Out))) { return false; }
	TestTrue(TEXT("Completeness belongs to each supplied index"), !Out.Attunement.Index.bIsFullyRepresented && Out.AlternativeAttunement.Index.bIsFullyRepresented);
	TestEqual(TEXT("Known and omitted selector sources remain separate"), Out.Attunement.Index.Groups[0].RecordRequirements[1].SourceId, FString());
	Out.Attunement.Index.Groups[0].RecordRequirements[0].Name = TEXT("Changed selector"); Out.Attunement.Index.Groups[0].LanguageProficiencies[0] = TEXT("changed");
	Out.Attunement.Index.Groups[0].AlignmentOptions[0].RequiredTraits.AddTag(Dnd5e::Alignment::Lawful); Out.Attunement.Index.Groups[0].MinimumAbilityScores[0].Minimum = 99;
	Out.Attunement.Index.Groups[0].UnsupportedContent[0].SourceFragment = TEXT("changed group"); Out.Attunement.Index.UnsupportedContent[0].SourceFragment = TEXT("changed outer");
	Out.AlternativeAttunement.Index.Groups[0].Size = EDnd5eSize::Huge; Out.AlternativeAttunement.Index.Groups[0].AlignmentOptions[0].RequiredTraits.Reset();
	TestTrue(TEXT("Nested mutations leave the original item intact"), Equal(FDnd5eItem::StaticStruct(), &Item, &Before));
	TestEqual(TEXT("Index changes leave primary readable wording intact"), Out.Attunement.Conditions[0].Payload.Get<FDnd5eTextContent>().Text, FString(TEXT("Original invented wording")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eAttunementPresenceTest, "Dnd5e.Items.Attunement.PresenceAndEditorConditions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eAttunementPresenceTest::RunTest(const FString& Parameters)
{
	FDnd5eAttunementRequirement Missing, Empty; Empty.bHasIndex = true;
	TestFalse(TEXT("Missing and supplied empty indexes differ"), Equal(FDnd5eAttunementRequirement::StaticStruct(), &Missing, &Empty));
	TestTrue(TEXT("Index presence does not infer a mode"), Empty.Mode == EDnd5eAttunementMode::Unspecified);
	FDnd5eAttunementIndexGroup Absent, ExplicitFalse; ExplicitFalse.bHasSpellcasting = true; ExplicitFalse.bHasPsionics = true;
	TestFalse(TEXT("Explicit false differs from absent booleans"), Equal(FDnd5eAttunementIndexGroup::StaticStruct(), &Absent, &ExplicitFalse));
	FDnd5eAttunementIndexGroup Out;
	if (!RoundTrip(*this, FDnd5eAttunementIndexGroup::StaticStruct(), &ExplicitFalse, &Out)) { return false; }
	TestTrue(TEXT("False assertions survive serialization"), Out.bHasSpellcasting && !Out.bSpellcasting && Out.bHasPsionics && !Out.bPsionics);
	Missing.Index.Groups.Add(ExplicitFalse); FDnd5eAttunementRequirement Inactive;
	if (!RoundTrip(*this, FDnd5eAttunementRequirement::StaticStruct(), &Missing, &Inactive)) { return false; }
	TestTrue(TEXT("Inactive index data is retained without asserting presence"), !Inactive.bHasIndex && Inactive.Index.Groups.Num() == 1);
	struct FCondition { UScriptStruct* Type; const TCHAR* Field; const TCHAR* Expected; };
	const FCondition Cases[] = {
		{FDnd5eAttunementRequirement::StaticStruct(), TEXT("Index"), TEXT("bHasIndex")},
		{FDnd5eAttunementIndexGroup::StaticStruct(), TEXT("AlignmentOptions"), TEXT("bHasAlignmentFilter")},
		{FDnd5eAttunementIndexGroup::StaticStruct(), TEXT("bSpellcasting"), TEXT("bHasSpellcasting")},
		{FDnd5eAttunementIndexGroup::StaticStruct(), TEXT("bPsionics"), TEXT("bHasPsionics")}
	};
	for (const auto& Case : Cases) { const auto* Field = Case.Type->FindPropertyByName(Case.Field); if (TestNotNull(Case.Field, Field)) { TestEqual(Case.Field, Field->GetMetaData(TEXT("EditCondition")), FString(Case.Expected)); } }
	return !HasAnyErrors();
}
#endif
