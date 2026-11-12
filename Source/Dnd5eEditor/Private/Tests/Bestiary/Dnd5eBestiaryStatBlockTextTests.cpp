// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS

#include "Browser/Bestiary/Dnd5eBestiaryStatBlockText.h"
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
	Check(TEXT("Damage Resistances  only cold, fire while shielded"));
	Check(TEXT("Daily (1 each)  Visible daily entry"));
	Check(TEXT("Levels 1-3 (0 slots)  Visible level entry"));
	TestFalse(TEXT("Hidden spell groups and entries are omitted"), Text.Contains(TEXT("Hidden group entry")) || Text.Contains(TEXT("Hidden individual entry")));
	const FString Empty = Dnd5eBestiaryStatBlockText::Format(FDnd5eCreature{}).ToString();
	TestFalse(TEXT("Missing fields do not invent numeric values"), Empty.Contains(TEXT("Armor Class")) || Empty.Contains(TEXT("Hit Points")) || Empty.Contains(TEXT("STR")));
	return !HasAnyErrors();
}

#endif
