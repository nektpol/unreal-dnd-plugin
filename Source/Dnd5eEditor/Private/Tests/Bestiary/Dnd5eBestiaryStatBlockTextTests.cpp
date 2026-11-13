// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS

#include "Browser/Bestiary/Dnd5eBestiaryStatBlockText.h"
#include "Browser/Dnd5eBrowserText.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Dnd5eTags.h"
#include "Misc/AutomationTest.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBestiaryStatBlockTextTest, "Dnd5e.Bestiary.Editor.StatBlockText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eBestiaryStatBlockTextTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonDocument Document;
	Document.Context.DatasetNamespace = TEXT("stat-block-test");
	Document.Context.RelativeFilePath = TEXT("fixture.json");
	Document.JsonText = TEXT(R"json({"monster":[{
		"name":"Invented Scout","source":"SYN","size":["S"],"type":"fey",
		"cr":"1/4","environment":["forest","hill"],"referenceSources":["SYN-REF"],
		"alignment":["NY","C","E","NX","L"],
		"ac":[0,{"special":"variable shell"},{"ac":15,"condition":"while glowing"}],
		"hp":{"average":0,"formula":"1d4"},
		"speed":{"walk":20,"fly":{"number":30,"condition":"while lit"}},
		"str":7,"dex":14,"con":10,"int":10,"wis":10,"cha":10,
		"save":{"str":"-2"},"skill":{"stealth":"+6"},
		"resist":[{"resist":["cold","fire"],"preNote":"only","note":"while shielded"}],
		"spellcasting":[{"name":"Lantern Magic","hidden":["will"],"will":["Hidden group entry"],
			"daily":{"1e":[{"entry":"Hidden individual entry","hidden":true},"Visible daily entry"]},
			"spells":{"3":{"lower":1,"slots":0,"spells":["Visible level entry"]}}}]
	}]})json");
	const auto Result = FDnd5eJsonCreatureReader::ReadCreature(Document, TEXT("Invented Scout"), TEXT("SYN"));
	if (!TestTrue(TEXT("Synthetic fixture reads successfully"), Result.Status == EDnd5eJsonReadStatus::Success))
	{
		for (const auto& Diagnostic : Result.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
		return false;
	}

	const FString Text = Dnd5eBestiaryStatBlockText::Format(Result.Creature).ToString();
	const auto Check = [this, &Text](const TCHAR* Expected)
	{
		TestTrue(FString::Printf(TEXT("Card contains %s"), Expected), Text.Contains(Expected, ESearchCase::CaseSensitive));
	};
	Check(TEXT("Small fey, any non-good alignment"));
	Check(TEXT("Armor Class  0, variable shell, 15 while glowing"));
	Check(TEXT("Hit Points  0 (1d4)"));
	Check(TEXT("Speed  20 ft, fly 30 ft while lit"));
	Check(TEXT("STR 7 (-2)    DEX 14 (+2)"));
	Check(TEXT("Saving Throws  Strength -2"));
	Check(TEXT("Skills  Stealth +6"));
	Check(TEXT("Challenge  1/4 (50 XP)"));
	Check(TEXT("Proficiency Bonus  +2"));
	Check(TEXT("Environment  forest, hill"));
	Check(TEXT("Referenced in  SYN-REF"));

	auto Override = Result.Creature;
	Override.ChallengeRating.bHasXp = true;
	Override.ChallengeRating.Xp = 0;
	Override.ProficiencyNote = TEXT("Varies by form");
	const FString OverrideText = Dnd5eBestiaryStatBlockText::Format(Override).ToString();
	TestTrue(TEXT("Authored zero XP and proficiency wording override defaults"),
		OverrideText.Contains(TEXT("Challenge  1/4 (0 XP)")) && OverrideText.Contains(TEXT("Proficiency Bonus  Varies by form")));

	Check(TEXT("Damage Resistances  only cold, fire while shielded"));
	Check(TEXT("Daily (1 each)  Visible daily entry"));
	Check(TEXT("Levels 1-3 (0 slots)  Visible level entry"));
	TestFalse(TEXT("Hidden spell groups and entries are omitted"), Text.Contains(TEXT("Hidden group entry")) || Text.Contains(TEXT("Hidden individual entry")));
	const FString Empty = Dnd5eBestiaryStatBlockText::Format(FDnd5eCreature{}).ToString();
	TestFalse(TEXT("Missing fields do not invent numeric values"), Empty.Contains(TEXT("Armor Class")) || Empty.Contains(TEXT("Hit Points")) || Empty.Contains(TEXT("STR")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBestiaryCombatLabelTest, "Dnd5e.Bestiary.Editor.CombatLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eBestiaryCombatLabelTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonDocument Document;
	Document.Context.DatasetNamespace = TEXT("combat-label-test");
	Document.Context.RelativeFilePath = TEXT("creatures.json");
	Document.JsonText = TEXT(R"json({"monster":[{
		"name":"Invented Guard","source":"SYN","size":["M"],"type":"humanoid","action":[
			{"name":"Legacy","entries":["{@atk mw} {@hit 2} to hit. {@h}2 ({@damage 1d4}) damage."]},
			{"name":"Revised","entries":["{@atkr m,r} {@hit 3}. {@actSave dex} {@dc 12}. {@actSaveFail 2} Prone. {@actSaveSuccess} Safe."]},
			{"name":"Unknown","entries":["Prefix {@inventedToken payload} suffix."]}
		]
	}]})json");
	const auto Result = FDnd5eJsonCreatureReader::ReadCreature(Document, TEXT("Invented Guard"), TEXT("SYN"));
	if (!TestTrue(TEXT("Combat fixture converts"), Result.Status == EDnd5eJsonReadStatus::Success &&
		Result.Creature.Actions.Num() == 3 && Result.Creature.Actions[0].Entries.Num() == 1))
	{
		for (const auto& Diagnostic : Result.Diagnostics) { AddError(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
		return false;
	}

	const auto& Actions = Result.Creature.Actions;
	TestEqual(TEXT("Legacy attack preserves hit and damage spacing"), Dnd5eBrowserText::FormatDescription(Actions[0].Entries).ToString(),
		FString(TEXT("Melee Weapon Attack: +2 to hit. Hit: 2 (1d4) damage.")));
	TestEqual(TEXT("Revised attack and save labels display with the DC"), Dnd5eBrowserText::FormatDescription(Actions[1].Entries).ToString(),
		FString(TEXT("Melee or Ranged Attack Roll: +3. Dexterity Saving Throw: DC 12. Second Failure: Prone. Success: Safe.")));
	TestEqual(TEXT("Unrecognized text still appears in the shared preview"), Dnd5eBrowserText::FormatDescription(Actions[2].Entries).ToString(),
		FString(TEXT("Prefix {@inventedToken payload} suffix.")));

	FDnd5eParagraphContent Paragraph;
	FDnd5eFormattedContent AttackLabel;
	TestTrue(TEXT("The reader preserves the exact token on typed italic content"),
		UDnd5eContentLibrary::TryGetParagraph(Actions[0].Entries[0], Paragraph) && !Paragraph.Inlines.IsEmpty() &&
		UDnd5eContentLibrary::TryGetFormatted(Paragraph.Inlines[0], AttackLabel) &&
		AttackLabel.SourceFormat == TEXT("5etools") && AttackLabel.SourceReference == TEXT("{@atk mw}") &&
		AttackLabel.Formatting.HasTagExact(Dnd5e::Content::Formatting::Italic));
	return !HasAnyErrors();
}

#endif
