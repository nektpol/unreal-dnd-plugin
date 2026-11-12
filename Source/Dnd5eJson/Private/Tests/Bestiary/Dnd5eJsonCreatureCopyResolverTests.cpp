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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyScalarTagsTest, "Dnd5e.Bestiary.Reader.CopyScalarTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopyScalarTagsTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;
	const FString Json = TEXT(R"json({"monster":[
		{"name":"Parent","source":"SYN",
			"action":[{"entries":[["{@hit +5} {@hit -1} {@damage 1d6}"]]}],
			"reaction":[{"entries":["{@dc 15|display} {@dc unresolved}"]}],
			"variant":[{"name":"Variant"}]},
		{"name":"Child","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"action":{"mode":"scalarAddHit","scalar":-2},
			"reaction":{"mode":"scalarAddDc","scalar":1},
			"legendary":{"mode":"scalarAddDc","scalar":1},
			"variant":"remove"
		}}},
		{"name":"Invalid","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"action":{"mode":"scalarAddHit","scalar":1.5}
		}}},
		{"name":"Overflow","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"action":{"mode":"scalarAddHit","scalar":2147483647}
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
	if (!TestTrue(TEXT("Scalar tag fixtures parse"), ParseJsonDocument(Root, State) &&
		RequireObject(Root, TEXT(""), Document, State) &&
		ReadRequiredArray(*Document, TEXT("monster"), TEXT(""), Rows, State) && Rows->Num() == 4))
	{
		return false;
	}

	const auto Parent = (*Rows)[0]->AsObject();
	FDnd5eCreatureCopyIndex Index;
	IndexCreatureCopyRow(Index, {Parent, Context.RelativeFilePath, TEXT("/monster/0")});
	TSharedPtr<FJsonObject> Merged;
	TArray<FDnd5eCreatureCopyRow> Ancestors;
	if (!TestTrue(TEXT("Scalar tags resolve"), ResolveCreatureCopy(Index,
		{(*Rows)[1]->AsObject(), Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto GetActionText = [](const FJsonObject& Object)
	{
		return Object.GetArrayField(TEXT("action"))[0]->AsObject()->
			GetArrayField(TEXT("entries"))[0]->AsArray()[0]->AsString();
	};
	TestEqual(TEXT("Hit changes apply to signed values inside nested arrays"), GetActionText(*Merged),
		FString(TEXT("{@hit 3} {@hit -3} {@damage 1d6}")));
	TestEqual(TEXT("DC changes strip the display suffix and preserve nonnumeric tags"),
		Merged->GetArrayField(TEXT("reaction"))[0]->AsObject()->GetArrayField(TEXT("entries"))[0]->AsString(),
		FString(TEXT("{@dc 16} {@dc unresolved}")));
	TestTrue(TEXT("A variant removal clears only the child field"), !Merged->HasField(TEXT("variant")) && Parent->HasField(TEXT("variant")));
	TestFalse(TEXT("An absent target stays absent"), Merged->HasField(TEXT("legendary")));
	TestEqual(TEXT("Parent text survives"), GetActionText(*Parent),
		FString(TEXT("{@hit +5} {@hit -1} {@damage 1d6}")));

	Merged.Reset();
	TestFalse(TEXT("Fractional scalar fails"), ResolveCreatureCopy(Index,
		{(*Rows)[2]->AsObject(), Context.RelativeFilePath, TEXT("/monster/2")}, Merged, Ancestors, State));
	TestTrue(TEXT("Fractional scalar points to the authored value"), Diagnostics.ContainsByPredicate(
		[](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_mod/action/scalar");
		}));
	TestFalse(TEXT("Overflowing adjustment fails"), ResolveCreatureCopy(Index,
		{(*Rows)[3]->AsObject(), Context.RelativeFilePath, TEXT("/monster/3")}, Merged, Ancestors, State));
	TestFalse(TEXT("Failed adjustments publish no result"), Merged.IsValid());
	return !HasAnyErrors();
}

#if UE_ENABLE_ICU
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyNamedTextTest, "Dnd5e.Bestiary.Reader.CopyNamedText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopyNamedTextTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;
	const FString Json = TEXT(R"json({"monster":[
		{"name":"Parent","source":"SYN","action":[{"name":"Moss","entries":["Moss"]}],
			"trait":[{"name":"Moss","entries":["Moss"]}]},
		{"name":"Child","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"*":{"mode":"replaceTxt","replace":"Bark","with":"Oak","props":["name"]},
			"action":{"mode":"replaceTxt","replace":"Moss","with":"Bark","props":["name"]},
			"actionHeader":{"mode":"replaceTxt","replace":"Moss","with":"Bark"}
		}}},
		{"name":"Invalid","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"action":{"mode":"replaceTxt","replace":"[","with":"Bark"}
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
	if (!TestTrue(TEXT("Named text fixtures parse"), ParseJsonDocument(Root, State) &&
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
	if (!TestTrue(TEXT("Named replacement resolves"), ResolveCreatureCopy(Index,
		{(*Rows)[1]->AsObject(), Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto Action = Merged->GetArrayField(TEXT("action"))[0]->AsObject();
	TestEqual(TEXT("Wildcard sees the named replacement"), Action->GetStringField(TEXT("name")), FString(TEXT("Oak")));
	TestEqual(TEXT("Property selection preserves body text"), Action->GetArrayField(TEXT("entries"))[0]->AsString(),
		FString(TEXT("Moss")));
	TestEqual(TEXT("Named replacement leaves other fields intact"),
		Merged->GetArrayField(TEXT("trait"))[0]->AsObject()->GetStringField(TEXT("name")), FString(TEXT("Moss")));
	TestEqual(TEXT("Parent action remains intact"),
		Parent->GetArrayField(TEXT("action"))[0]->AsObject()->GetStringField(TEXT("name")), FString(TEXT("Moss")));
	TestFalse(TEXT("Absent header stays absent"), Merged->HasField(TEXT("actionHeader")));

	Merged.Reset();
	TestFalse(TEXT("Invalid named regex fails"), ResolveCreatureCopy(Index,
		{(*Rows)[2]->AsObject(), Context.RelativeFilePath, TEXT("/monster/2")}, Merged, Ancestors, State));
	TestTrue(TEXT("Invalid pattern points to the named operation"), Diagnostics.ContainsByPredicate(
		[](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_mod/action/replace");
		}));
	return !HasAnyErrors();
}
#endif


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyNumericPropertiesTest, "Dnd5e.Bestiary.Reader.CopyNumericProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopyNumericPropertiesTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;
	const FString Json = TEXT(R"json({"monster":[
		{"name":"Parent","source":"SYN","hp":{"average":43,"formula":"6d8+16"},
			"save":{"dex":"+2","wis":"-1"},"skill":{"stealth":"+2 plus a bonus"},
			"spellcasting":[{"name":"Original","will":["first"]}]},
		{"name":"Child","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"hp":{"mode":"scalarMultProp","prop":"average","scalar":0.5,"floor":true},
			"save":{"mode":"scalarAddProp","prop":"*","scalar":-2},
			"spellcasting":{"mode":"appendArr","items":{"name":"Added","will":["second"]}},
			"_":{"mode":"addSpells","will":["third"]}
		}}},
		{"name":"Deferred","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{
			"skill":{"mode":"scalarAddProp","prop":"*","scalar":1}
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
	if (!TestTrue(TEXT("Numeric property fixtures parse"), ParseJsonDocument(Root, State) &&
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
	if (!TestTrue(TEXT("Numeric modifiers and spellcasting append resolve"), ResolveCreatureCopy(Index,
		{(*Rows)[1]->AsObject(), Context.RelativeFilePath, TEXT("/monster/1")}, Merged, Ancestors, State)))
	{
		return false;
	}

	const auto Hp = Merged->GetObjectField(TEXT("hp"));
	int32 Average = 0;
	FString Literal;
	TestTrue(TEXT("Floored HP is readable as a numeric integer token"),
		ReadRequiredInteger(*Hp, TEXT("average"), TEXT("/hp"), 0, MAX_int32, Average, Literal, State) &&
		Average == 21 && Literal == TEXT("21"));
	TestEqual(TEXT("HP formula survives"), Hp->GetStringField(TEXT("formula")), FString(TEXT("6d8+16")));
	const auto Save = Merged->GetObjectField(TEXT("save"));
	TestEqual(TEXT("Zero keeps an explicit plus sign"), Save->GetStringField(TEXT("dex")), FString(TEXT("+0")));
	TestEqual(TEXT("Negative save stays a string"), Save->GetStringField(TEXT("wis")), FString(TEXT("-3")));
	const auto& Casting = Merged->GetArrayField(TEXT("spellcasting"));
	TestTrue(TEXT("Append adds a block and the root edit targets the first block"), Casting.Num() == 2 &&
		Casting[0]->AsObject()->GetArrayField(TEXT("will")).Num() == 2 &&
		Casting[1]->AsObject()->GetStringField(TEXT("name")) == TEXT("Added") &&
		Casting[1]->AsObject()->GetArrayField(TEXT("will")).Num() == 1);
	TestEqual(TEXT("Parent HP survives"), Parent->GetObjectField(TEXT("hp"))->GetNumberField(TEXT("average")), 43.0);

	Merged.Reset();
	TestFalse(TEXT("Compound bonus defers the copy"), ResolveCreatureCopy(Index,
		{(*Rows)[2]->AsObject(), Context.RelativeFilePath, TEXT("/monster/2")}, Merged, Ancestors, State));
	TestTrue(TEXT("Compound bonus reports the modifier origin without publishing a result"),
		!Merged.IsValid() && Diagnostics.ContainsByPredicate([](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.Code == Codes::UnsupportedRequiredValue &&
				Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_mod");
		}));
	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyRootPropertiesTest, "Dnd5e.Bestiary.Reader.CopyRootProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eCreatureCopyRootPropertiesTest::RunTest(const FString& Parameters)
{
	using namespace Dnd5eJsonReader;
	const FString Json = TEXT(R"json({"monster":[
		{"name":"Parent","source":"SYN","size":["G","H"],"languages":["Invented"],"vulnerable":["cold"]},
		{"name":"Child","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{"_":[
			{"mode":"setProp","prop":"languages","value":null},
			{"mode":"setProp","prop":"vulnerable","value":null},
			{"mode":"setProp","prop":"size","value":["H","M","S"]},
			{"mode":"maxSize","max":"L"}
		]}}},
		{"name":"Capped","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{"_":{"mode":"maxSize","max":"L"}}}},
		{"name":"Invalid","source":"SYN","_copy":{"name":"Parent","source":"SYN","_mod":{"_":{"mode":"setProp","prop":"languages"}}}}
	]})json");
	FDnd5eJsonReadContext Context;
	Context.DatasetNamespace = TEXT("bestiary-test");
	Context.RelativeFilePath = TEXT("fixtures/creatures.json");
	TArray<FDnd5eDiagnostic> Diagnostics;
	FReadState State{Context, Json, Diagnostics};
	TSharedPtr<FJsonValue> Root;
	TSharedPtr<FJsonObject> Document;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!TestTrue(TEXT("Root property fixtures parse"), ParseJsonDocument(Root, State) &&
		RequireObject(Root, TEXT(""), Document, State) &&
		ReadRequiredArray(*Document, TEXT("monster"), TEXT(""), Rows, State) && Rows->Num() == 4))
	{
		return false;
	}

	const auto Parent = (*Rows)[0]->AsObject();
	FDnd5eCreatureCopyIndex Index;
	IndexCreatureCopyRow(Index, {Parent, Context.RelativeFilePath, TEXT("/monster/0")});
	TSharedPtr<FJsonObject> Merged;
	TArray<FDnd5eCreatureCopyRow> Ancestors;
	const auto Resolve = [&](int32 Row)
	{
		Merged.Reset();
		return ResolveCreatureCopy(Index, {(*Rows)[Row]->AsObject(), Context.RelativeFilePath,
			AppendPointer(TEXT("/monster"), FString::FromInt(Row))}, Merged, Ancestors, State);
	};
	if (!TestTrue(TEXT("Root property edits resolve"), Resolve(1))) { return false; }
	TestTrue(TEXT("Explicit null clears inherited fields"),
		!Merged->HasField(TEXT("languages")) && !Merged->HasField(TEXT("vulnerable")) &&
		Parent->HasField(TEXT("languages")) && Parent->HasField(TEXT("vulnerable")));
	const auto& Sizes = Merged->GetArrayField(TEXT("size"));
	TestTrue(TEXT("Cap reads the preceding assignment and sorts retained sizes"),
		Sizes.Num() == 2 && Sizes[0]->AsString() == TEXT("S") && Sizes[1]->AsString() == TEXT("M"));

	if (!TestTrue(TEXT("Cap with no remaining sizes resolves"), Resolve(2))) { return false; }
	const auto& CappedSizes = Merged->GetArrayField(TEXT("size"));
	TestTrue(TEXT("Cap becomes the fallback size"), CappedSizes.Num() == 1 && CappedSizes[0]->AsString() == TEXT("L"));
	TestFalse(TEXT("Missing value fails"), Resolve(3));
	TestTrue(TEXT("Missing value identifies the modifier without publishing a result"),
		!Merged.IsValid() && Diagnostics.ContainsByPredicate([](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.Code == Codes::MissingRequiredField &&
				Diagnostic.JsonPointer == TEXT("/monster/3/_copy/_mod/_/value");
		}));
	return !HasAnyErrors();
}

#endif
