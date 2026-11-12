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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyAddSpellsTest, "Dnd5e.Bestiary.Reader.CopyAddSpells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopyAddSpellsTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;
	const FString Json = TEXT(R"json({"monster":[
		{"name":"Parent","source":"SYN","spellcasting":[{"will":["first"],"spells":{"2":{"slots":2,"spells":["Moss"]}}}]},
		{"name":"Child","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{"_":[
			{"mode":"addSpells","will":["first","second"],"daily":{"1e":["old","keep"]},
				"spells":{"2":{"spells":["zebra","Apple"]},"3":{"spells":["zebra","Apple"]}}},
			{"mode":"replaceSpells","daily":{"1e":[{"replace":"old","with":"new"}]}}
		]}}},
		{"name":"Invalid","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"_":{"mode":"addSpells","will":["temporary"],"daily":{"1e":[4]}}
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
	if (!TestTrue(TEXT("Spell additions fixtures parse"), ParseJsonDocument(Root, State) &&
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
	if (!TestTrue(TEXT("Spell additions resolve"), ResolveCreatureCopy(Index,
		{(*Rows)[1]->AsObject(), Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto Block = Merged->GetArrayField(TEXT("spellcasting"))[0]->AsObject();
	const auto Levels = Block->GetObjectField(TEXT("spells"));
	const auto Level = Levels->GetObjectField(TEXT("2"));
	const auto& Existing = Level->GetArrayField(TEXT("spells"));
	const auto& Added = Levels->GetObjectField(TEXT("3"))->GetArrayField(TEXT("spells"));
	const auto& Will = Block->GetArrayField(TEXT("will"));
	const auto& Daily = Block->GetObjectField(TEXT("daily"))->GetArrayField(TEXT("1e"));
	TestTrue(TEXT("Existing levels sort the merged list and retain slots"), Existing.Num() == 3 &&
		Existing[0]->AsString() == TEXT("Apple") && Existing[1]->AsString() == TEXT("Moss") &&
		Existing[2]->AsString() == TEXT("zebra") && Level->GetNumberField(TEXT("slots")) == 2);
	TestTrue(TEXT("New levels retain authored order"), Added.Num() == 2 &&
		Added[0]->AsString() == TEXT("zebra") && Added[1]->AsString() == TEXT("Apple"));
	TestTrue(TEXT("At-will spells preserve order and duplicates"), Will.Num() == 3 &&
		Will[0]->AsString() == TEXT("first") && Will[1]->AsString() == TEXT("first") &&
		Will[2]->AsString() == TEXT("second"));
	TestTrue(TEXT("Replacement sees the newly added daily list"), Daily.Num() == 2 &&
		Daily[0]->AsString() == TEXT("keep") && Daily[1]->AsString() == TEXT("new"));

	Merged.Reset();
	TestFalse(TEXT("Malformed spell addition fails"), ResolveCreatureCopy(Index,
		{(*Rows)[2]->AsObject(), Context.RelativeFilePath, TEXT("/monster/2")}, Merged, Ancestors, State));
	TestTrue(TEXT("Failed addition leaves the parent intact and publishes no copy"), !Merged.IsValid() &&
		Parent->GetArrayField(TEXT("spellcasting"))[0]->AsObject()->GetArrayField(TEXT("will")).Num() == 1);
	TestTrue(TEXT("Failure identifies the authored spell"), Diagnostics.ContainsByPredicate(
		[](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_mod/_/daily/1e/0");
		}));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyRemoveSpellsTest, "Dnd5e.Bestiary.Reader.CopyRemoveSpells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopyRemoveSpellsTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;
	const FString Json = TEXT(R"json({"monster":[
		{"name":"Parent","source":"SYN","spellcasting":[
			{"daily":{"1e":["A","b","A","a"],"2":["keep"]},
				"spells":{"2":{"slots":2,"spells":["first","second"]},"3":{"slots":1}}},
			{"daily":{"1e":["A"]}}]},
		{"name":"Child","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{"_":[
			{"mode":"addSpells","daily":{"1e":["temporary"]}},
			{"mode":"removeSpells","daily":{"1e":["A","temporary","absent"]},
				"spells":{"2":["first"],"3":["absent"],"4":["absent"]}}
		]}}},
		{"name":"Invalid","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"_":{"mode":"removeSpells","daily":{"1e":[4]}}
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
	if (!TestTrue(TEXT("Spell removal fixtures parse"), ParseJsonDocument(Root, State) &&
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
	if (!TestTrue(TEXT("Spell removals resolve"), ResolveCreatureCopy(Index,
		{(*Rows)[1]->AsObject(), Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto& Casting = Merged->GetArrayField(TEXT("spellcasting"));
	const auto Daily = Casting[0]->AsObject()->GetObjectField(TEXT("daily"));
	const auto& Once = Daily->GetArrayField(TEXT("1e"));
	TestTrue(TEXT("Removal sees earlier additions and removes exact duplicates in order"),
		Once.Num() == 2 && Once[0]->AsString() == TEXT("b") && Once[1]->AsString() == TEXT("a"));
	TestEqual(TEXT("Other daily lists survive"), Daily->GetArrayField(TEXT("2")).Num(), 1);
	TestEqual(TEXT("Other casting blocks survive"),
		Casting[1]->AsObject()->GetObjectField(TEXT("daily"))->GetArrayField(TEXT("1e")).Num(), 1);

	const auto Levels = Casting[0]->AsObject()->GetObjectField(TEXT("spells"));
	const auto Level = Levels->GetObjectField(TEXT("2"));
	const auto& Spells = Level->GetArrayField(TEXT("spells"));
	TestTrue(TEXT("Level removal preserves slots and skips absent lists"),
		Spells.Num() == 1 && Spells[0]->AsString() == TEXT("second") && Level->GetNumberField(TEXT("slots")) == 2 &&
		!Levels->GetObjectField(TEXT("3"))->HasField(TEXT("spells")) && !Levels->HasField(TEXT("4")));
	TestEqual(TEXT("Parent daily list survives"),
		Parent->GetArrayField(TEXT("spellcasting"))[0]->AsObject()->
			GetObjectField(TEXT("daily"))->GetArrayField(TEXT("1e")).Num(), 4);

	Merged.Reset();
	TestFalse(TEXT("Malformed removal fails"), ResolveCreatureCopy(Index,
		{(*Rows)[2]->AsObject(), Context.RelativeFilePath, TEXT("/monster/2")}, Merged, Ancestors, State));
	TestFalse(TEXT("Failed removal publishes no result"), Merged.IsValid());
	TestTrue(TEXT("Failure identifies the authored selector"), Diagnostics.ContainsByPredicate(
		[](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_mod/_/daily/1e/0");
		}));
	return !HasAnyErrors();
}
#endif
