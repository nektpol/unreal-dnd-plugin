// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS
#include "Readers/Bestiary/Dnd5eJsonCreatureCopyResolver.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopySensesAndArraysTest, "Dnd5e.Bestiary.Reader.CopySensesAndArrays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopySensesAndArraysTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;

	const auto Parent = MakeShared<FJsonObject>();
	Parent->SetStringField(TEXT("name"), TEXT("Invented parent"));
	Parent->SetStringField(TEXT("source"), TEXT("SYN"));
	Parent->SetArrayField(TEXT("senses"), {MakeShared<FJsonValueString>(TEXT("Darkvision 30 ft.")),
		MakeShared<FJsonValueString>(TEXT("blindsight 120 ft. (qualified)"))});
	Parent->SetArrayField(TEXT("languages"), {MakeShared<FJsonValueString>(TEXT("Invented speech"))});

	const FString Json = TEXT(R"json({
		"name":"Invented child","source":"SYN",
		"_copy":{"name":"Invented parent","source":"SYN","_mod":{
			"languages":{"mode":"appendIfNotExistsArr","items":["Invented speech","Other speech","Other speech"]},
			"_":{"mode":"addSenses","senses":[{"type":"darkvision","range":60},
				{"type":"blindsight","range":60},{"type":"tremorsense","range":10}]}
		}}
	})json");
	FDnd5eJsonReadContext Context;
	Context.DatasetNamespace = TEXT("bestiary-test");
	Context.RelativeFilePath = TEXT("fixtures/creatures.json");
	TArray<FDnd5eDiagnostic> Diagnostics;
	FReadState State{Context, Json, Diagnostics};
	TSharedPtr<FJsonValue> Root;
	TSharedPtr<FJsonObject> Child;
	if (!TestTrue(TEXT("Copy fixture parses"), ParseJsonDocument(Root, State) &&
		RequireObject(Root, TEXT(""), Child, State)))
	{
		return false;
	}

	FDnd5eCreatureCopyIndex Index;
	IndexCreatureCopyRow(Index, {Parent, Context.RelativeFilePath, TEXT("/monster/0")});
	TSharedPtr<FJsonObject> Merged;
	TArray<FDnd5eCreatureCopyRow> Ancestors;
	if (!TestTrue(TEXT("Copy modifiers resolve"), ResolveCreatureCopy(Index,
		{Child, Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto& Languages = Merged->GetArrayField(TEXT("languages"));
	const auto& Senses = Merged->GetArrayField(TEXT("senses"));
	TestTrue(TEXT("Unique append compares only with the original array and preserves order"),
		Languages.Num() == 3 && Languages[0]->AsString() == TEXT("Invented speech") &&
		Languages[1]->AsString() == TEXT("Other speech") && Languages[2]->AsString() == TEXT("Other speech"));
	TestTrue(TEXT("Sense grants upgrade a weaker range, preserve a stronger sense, and append missing senses"),
		Senses.Num() == 3 && Senses[0]->AsString() == TEXT("darkvision 60 ft.") &&
		Senses[1]->AsString() == TEXT("blindsight 120 ft. (qualified)") &&
		Senses[2]->AsString() == TEXT("tremorsense 10 ft."));
	TestEqual(TEXT("Parent senses are unchanged"), Parent->GetArrayField(TEXT("senses"))[0]->AsString(),
		FString(TEXT("Darkvision 30 ft.")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopySkillGrantsTest, "Dnd5e.Bestiary.Reader.CopySkillGrants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopySkillGrantsTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;

	const FString Json = TEXT(R"json({"monster":[
		{"name":"Invented parent","source":"SYN","cr":{"cr":"5","lair":"6"},"wis":9,"dex":9,
			"skill":{"insight":"+6","medicine":"+2 plus a bonus","stealth":"-3"},"miscTags":["old"]},
		{"name":"Invented child","source":"SYN","_copy":{"name":"Invented parent","source":"SYN","_mod":{
			"_":{"mode":"addSkills","skills":{"perception":2,"insight":1,"medicine":1,"stealth":1}},
			"miscTags":{"mode":"appendIfNotExistsArr","items":["old","new"]}
		}}},
		{"name":"Invalid grant","source":"SYN","_copy":{"name":"Invented parent","source":"SYN",
			"_mod":{"_":{"mode":"addSkills","skills":{"perception":3}}}}}
	]})json");
	FDnd5eJsonReadContext Context;
	Context.DatasetNamespace = TEXT("bestiary-test");
	Context.RelativeFilePath = TEXT("fixtures/creatures.json");
	TArray<FDnd5eDiagnostic> Diagnostics;
	FReadState State{Context, Json, Diagnostics};
	TSharedPtr<FJsonValue> Root;
	TSharedPtr<FJsonObject> Document;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Skill grant fixtures parse"), ParseJsonDocument(Root, State) &&
		RequireObject(Root, TEXT(""), Document, State) &&
		ReadRequiredArray(*Document, TEXT("monster"), TEXT(""), Rows, State) && Rows->Num() == 3))
	{
		return false;
	}

	const auto Parent = (*Rows)[0]->AsObject();
	FDnd5eCreatureCopyIndex Index;
	IndexCreatureCopyRow(Index, {Parent, Context.RelativeFilePath, TEXT("/monster/0")});
	TSharedPtr<FJsonObject> Merged;
	TArray<FDnd5eCreatureCopyRow> Ancestors;
	if (!TestTrue(TEXT("Skill and metadata modifiers resolve"), ResolveCreatureCopy(Index,
		{(*Rows)[1]->AsObject(), Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto Skills = Merged->GetObjectField(TEXT("skill"));
	TestEqual(TEXT("Expertise uses base CR and floors a negative ability modifier"),
		Skills->GetStringField(TEXT("perception")), FString(TEXT("+5")));
	TestEqual(TEXT("Higher bonus survives"), Skills->GetStringField(TEXT("insight")), FString(TEXT("+6")));
	TestEqual(TEXT("Compound bonus survives"), Skills->GetStringField(TEXT("medicine")), FString(TEXT("+2 plus a bonus")));
	TestEqual(TEXT("Lower signed bonus is raised"), Skills->GetStringField(TEXT("stealth")), FString(TEXT("+2")));
	TestEqual(TEXT("Parent retains its original bonus"),
		Parent->GetObjectField(TEXT("skill"))->GetStringField(TEXT("stealth")), FString(TEXT("-3")));
	const auto& Tags = Merged->GetArrayField(TEXT("miscTags"));
	TestTrue(TEXT("Metadata append preserves order and skips original duplicates"),
		Tags.Num() == 2 && Tags[0]->AsString() == TEXT("old") && Tags[1]->AsString() == TEXT("new"));

	TestFalse(TEXT("Rank outside 1 and 2 fails"), ResolveCreatureCopy(Index,
		{(*Rows)[2]->AsObject(), Context.RelativeFilePath, TEXT("/monster/2")}, Merged, Ancestors, State));
	TestTrue(TEXT("Rank diagnostic points to the authored grant"), Diagnostics.ContainsByPredicate(
		[](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_mod/_/skills/perception");
		}));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyReplaceSpellsTest, "Dnd5e.Bestiary.Reader.CopyReplaceSpells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopyReplaceSpellsTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;

	const FString Json = TEXT(R"json({"monster":[
		{"name":"Invented parent","source":"SYN","vulnerable":["cold"],"spellcasting":[
			{"spells":{"7":{"slots":2,"spells":["Moss","old"]}},"daily":{"1e":["daily old"]}},
			{"daily":{"1e":["second block"]}}]},
		{"name":"Invented child","source":"SYN","_copy":{"name":"Invented parent","source":"SYN","_mod":{
			"vulnerable":{"mode":"appendIfNotExistsArr","items":["cold","fire"]},
			"_":{"mode":"replaceSpells","spells":{"7":[{"replace":"old","with":["zebra","Apple"]}],
				"8":[{"replace":"missing","with":"ignored"}]},"daily":{"1e":[{"replace":"daily old","with":"daily new"}]}}
		}}},
		{"name":"Invalid copy","source":"SYN","_copy":{"name":"Invented parent","source":"SYN","_mod":{
			"_":{"mode":"replaceSpells","spells":{"7":[{"replace":"old","with":"temporary"}]},
				"daily":{"1e":[{"replace":"Daily old","with":"invalid"}]}}
		}}}
	]})json");
	FDnd5eJsonReadContext Context;
	Context.DatasetNamespace = TEXT("bestiary-test");
	Context.RelativeFilePath = TEXT("fixtures/creatures.json");
	TArray<FDnd5eDiagnostic> Diagnostics;
	FReadState State{Context, Json, Diagnostics};
	TSharedPtr<FJsonValue> Root;
	TSharedPtr<FJsonObject> Document;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Spell replacement fixtures parse"), ParseJsonDocument(Root, State) &&
		RequireObject(Root, TEXT(""), Document, State) &&
		ReadRequiredArray(*Document, TEXT("monster"), TEXT(""), Rows, State) && Rows->Num() == 3))
	{
		return false;
	}

	const auto Parent = (*Rows)[0]->AsObject();
	FDnd5eCreatureCopyIndex Index;
	IndexCreatureCopyRow(Index, {Parent, Context.RelativeFilePath, TEXT("/monster/0")});
	TSharedPtr<FJsonObject> Merged;
	TArray<FDnd5eCreatureCopyRow> Ancestors;
	if (!TestTrue(TEXT("Level and daily spell replacements resolve"), ResolveCreatureCopy(Index,
		{(*Rows)[1]->AsObject(), Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto& Casting = Merged->GetArrayField(TEXT("spellcasting"));
	const auto Levels = Casting[0]->AsObject()->GetObjectField(TEXT("spells"));
	const auto Level = Levels->GetObjectField(TEXT("7"));
	const auto& Spells = Level->GetArrayField(TEXT("spells"));
	TestTrue(TEXT("Replacements sort case-insensitively and preserve slots"),
		Spells.Num() == 3 && Spells[0]->AsString() == TEXT("Apple") &&
		Spells[1]->AsString() == TEXT("Moss") && Spells[2]->AsString() == TEXT("zebra") &&
		Level->GetNumberField(TEXT("slots")) == 2 && !Levels->HasField(TEXT("8")));
	TestEqual(TEXT("Daily replacement applies to the first block"),
		Casting[0]->AsObject()->GetObjectField(TEXT("daily"))->GetArrayField(TEXT("1e"))[0]->AsString(),
		FString(TEXT("daily new")));
	TestEqual(TEXT("Second spellcasting block is preserved"),
		Casting[1]->AsObject()->GetObjectField(TEXT("daily"))->GetArrayField(TEXT("1e"))[0]->AsString(),
		FString(TEXT("second block")));
	const auto& Vulnerabilities = Merged->GetArrayField(TEXT("vulnerable"));
	TestTrue(TEXT("Vulnerability append skips existing entries"),
		Vulnerabilities.Num() == 2 && Vulnerabilities[0]->AsString() == TEXT("cold") &&
		Vulnerabilities[1]->AsString() == TEXT("fire"));

	Merged.Reset();
	TestFalse(TEXT("Spell selector requires an exact match"), ResolveCreatureCopy(Index,
		{(*Rows)[2]->AsObject(), Context.RelativeFilePath, TEXT("/monster/2")}, Merged, Ancestors, State));
	TestFalse(TEXT("Failed replacement publishes no merged creature"), Merged.IsValid());
	TestTrue(TEXT("Failure points to the authored selector"), Diagnostics.ContainsByPredicate(
		[](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_mod/_/daily/1e/0/replace");
		}));
	const auto ParentLevel = Parent->GetArrayField(TEXT("spellcasting"))[0]->AsObject()->
		GetObjectField(TEXT("spells"))->GetObjectField(TEXT("7"));
	TestEqual(TEXT("Parent spell list survives successful and failed replacements"),
		ParentLevel->GetArrayField(TEXT("spells"))[1]->AsString(), FString(TEXT("old")));
	return !HasAnyErrors();
}
#endif
