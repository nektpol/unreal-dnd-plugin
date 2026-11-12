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
#endif
