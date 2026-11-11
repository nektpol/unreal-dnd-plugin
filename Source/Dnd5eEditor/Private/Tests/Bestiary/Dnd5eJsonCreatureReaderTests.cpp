// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Dnd5eTags.h"
#include "Readers/Bestiary/Dnd5eJsonCreatureReader.h"

namespace
{
	const TCHAR* StatBlock = TEXT(R"json({
		"monster":[{"name":"Lantern Scout","source":"SYN-A"},
		{"name":"Lantern Scout","source":"SYN-B","page":4,"size":["S"],"alignment":["C","N"],
		"type":{"type":"fey","tags":["lantern folk"]},"ac":[12,{"ac":14,"from":["Invented shell","{@item Glass Plate|SYN-B}"]}],
		"hp":{"average":9,"formula":"2d8"},"speed":{"walk":25},"str":7,"dex":13,"con":11,"int":12,"wis":9,"cha":8,
		"skill":{"stealth":"+6","arcana":"-2"},"senses":["Invented sense"],"passive":9,"languages":["Invented speech"],"cr":"1/4",
		"trait":[{"name":"Glow","entries":["A soft glow."]}],
		"action":[{"name":"Flicker","entries":["A quick flicker."]}],
		"bonus":[{"name":"Dim","entries":["A dim glow."]}]}]
	})json");

	FDnd5eJsonDocument MakeDocument(const FString& Json)
	{
		FDnd5eJsonDocument Document;
		Document.JsonText = Json;
		Document.Context.DatasetNamespace = TEXT("bestiary-test");
		Document.Context.RelativeFilePath = TEXT("fixtures/creatures.json");
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureConversionTest, "Dnd5e.Bestiary.Reader.Conversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureConversionTest::RunTest(const FString& Parameters)
{
	const auto Result = FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(StatBlock), TEXT("Lantern Scout"), TEXT("SYN-B"));
	if (!TestTrue(TEXT("Exact publication selects the complete row"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto& Creature = Result.Creature;
	TestTrue(TEXT("Identity and original location"), Creature.Header.RecordId.Category == Dnd5e::Category::Creature &&
		Creature.Header.RecordId.SourceKey == TEXT("5etools:creature:v1:Lantern%20Scout|SYN-B") &&
		Creature.Header.RecordId.DatasetNamespace == TEXT("bestiary-test") && Creature.Header.Provenance.Num() == 1 &&
		Creature.Header.Provenance[0].SourceId == TEXT("SYN-B") && Creature.Header.Provenance[0].JsonPointer == TEXT("/monster/1"));
	TestTrue(TEXT("Classification"), Creature.CreatureType == Dnd5e::Creature::Type::Fey &&
		Creature.Alignment.HasTagExact(Dnd5e::Alignment::Chaotic) && Creature.Alignment.HasTagExact(Dnd5e::Alignment::Neutral) &&
		Creature.Sizes.Num() == 1 && Creature.Sizes[0] == EDnd5eSize::Small &&
		Creature.CreatureSubtypes == TArray<FString>{TEXT("lantern folk")});
	TestTrue(TEXT("Both AC forms retain sources in their own entry"), Creature.ArmorClasses.Num() == 2 &&
		Creature.ArmorClasses[0].Value == 12 && Creature.ArmorClasses[0].Sources.IsEmpty() &&
		Creature.ArmorClasses[1].Value == 14 && Creature.ArmorClasses[1].Sources.Num() == 2);
	TestTrue(TEXT("HP, movement and printed scores"), Creature.HitPoints.Average == 9 &&
		Creature.HitPoints.Formula.Expression == TEXT("2d8") && Creature.WalkSpeed.Unit == EDnd5eDistanceUnit::Feet &&
		Creature.WalkSpeed.Amount.Integer == 25 && Creature.AbilityScores.Dexterity == 13 && Creature.AbilityScores.Wisdom == 9);
	TestTrue(TEXT("Positive and negative skill bonuses"), Creature.Skills.Num() == 2 &&
		Creature.Skills.ContainsByPredicate([](const auto& Skill) { return Skill.Skill == TEXT("stealth") && Skill.Bonus == 6; }) &&
		Creature.Skills.ContainsByPredicate([](const auto& Skill) { return Skill.Skill == TEXT("arcana") && Skill.Bonus == -2; }));
	TestTrue(TEXT("Observations"), Creature.PassivePerception == 9 && Creature.ChallengeRating == TEXT("1/4") &&
		Creature.Senses.Num() == 1 && Creature.Languages.Num() == 1);
	TestTrue(TEXT("Independent ordered rule groups"), Creature.Traits.Num() == 1 && Creature.Traits[0].Name == TEXT("Glow") &&
		Creature.Actions.Num() == 1 && Creature.Actions[0].Name == TEXT("Flicker") && Creature.Actions[0].Entries.Num() == 1 &&
		Creature.BonusActions.Num() == 1 && Creature.BonusActions[0].Name == TEXT("Dim"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureFailuresTest, "Dnd5e.Bestiary.Reader.Failures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureFailuresTest::RunTest(const FString& Parameters)
{
	const auto CheckFailure = [this](const TCHAR* Label, const FString& Json, const TCHAR* Source, const TCHAR* Pointer)
	{
		const auto Result = FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), Source);
		TestTrue(Label, Result.Status == EDnd5eJsonReadStatus::Failure && Result.Creature.Header.DisplayName.IsEmpty() &&
			Result.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error && Diagnostic.JsonPointer == Pointer;
			}));
	};

	CheckFailure(TEXT("Missing exact publication"), StatBlock, TEXT("syn-b"), TEXT("/monster"));
	CheckFailure(TEXT("Malformed HP fails atomically"), FString(StatBlock).Replace(TEXT("9,\"formula\""), TEXT("\"nine\",\"formula\"")),
		TEXT("SYN-B"), TEXT("/monster/1/hp/average"));
	CheckFailure(TEXT("Duplicate row"), TEXT(R"({"monster":[{"name":"Lantern Scout","source":"SYN-B"},{"name":"Lantern Scout","source":"SYN-B"}]})"),
		TEXT("SYN-B"), TEXT("/monster/1"));
	CheckFailure(TEXT("Copy row requires assembly"), TEXT(R"({"monster":[{"name":"Lantern Scout","source":"SYN-B","_copy":{"name":"Parent","source":"SYN-A"}}]})"),
		TEXT("SYN-B"), TEXT("/monster/0/_copy"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureAuditTest, "Dnd5e.Bestiary.Reader.DocumentAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureAuditTest::RunTest(const FString& Parameters)
{
	FString Json(StatBlock);
	Json.ReplaceInline(TEXT(R"({"name":"Lantern Scout","source":"SYN-A"})"),
		TEXT(R"(null,{"name":"Bad Scout","source":"SYN-A","hp":{"average":"bad"},"ac":[12,{"ac":14}]},{"name":" ","source":"SYN-A"},{"name":"Copy Scout","source":"SYN-A","_copy":{"name":"Lantern Scout","source":"SYN-B"}})"));
	const auto Audit = FDnd5eJsonCreatureReader::AuditDocument(MakeDocument(Json));
	if (!TestTrue(TEXT("All five rows visited"), Audit.Status == EDnd5eJsonReadStatus::Success && Audit.Rows.Num() == 5)) { return false; }

	const auto CheckFailure = [this, &Audit](int32 Index, const TCHAR* Pointer)
	{
		const auto& Row = Audit.Rows[Index];
		TestTrue(Pointer, Row.Status == EDnd5eJsonReadStatus::Failure && Row.Diagnostics.ContainsByPredicate(
			[Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error && Diagnostic.JsonPointer == Pointer;
			}));
	};
	CheckFailure(0, TEXT("/monster/0"));
	CheckFailure(1, TEXT("/monster/1/size"));
	CheckFailure(2, TEXT("/monster/2/name"));
	CheckFailure(3, TEXT("/monster/3/_copy"));
	TestTrue(TEXT("Raw shapes survive an earlier conversion failure"), Audit.Rows[1].FieldShapes.ContainsByPredicate(
		[](const FDnd5eCreatureFieldShape& Shape)
		{
			return Shape.Path == TEXT("ac[].ac") && Shape.JsonType == TEXT("Number") &&
				Shape.JsonPointer == TEXT("/monster/1/ac/1/ac");
		}));
	TestTrue(TEXT("Copy marked and later direct row still converts"), Audit.Rows[3].bIsCopy &&
		!Audit.Rows[4].bIsCopy && Audit.Rows[4].Status == EDnd5eJsonReadStatus::Success &&
		Audit.Rows[4].SourceId == TEXT("SYN-B") && Audit.Diagnostics.IsEmpty());

	const auto BadEnvelope = FDnd5eJsonCreatureReader::AuditDocument(MakeDocument(TEXT(R"({"monster":false})")));
	TestTrue(TEXT("Invalid envelope has document diagnostics and no rows"), BadEnvelope.Status == EDnd5eJsonReadStatus::Failure &&
		BadEnvelope.Rows.IsEmpty() && !BadEnvelope.Diagnostics.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureLocalSmokeTest, "Dnd5e.Bestiary.Local.GoblinSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureLocalSmokeTest::RunTest(const FString& Parameters)
{
	FString Root;
	if (!FParse::Value(FCommandLine::Get(), TEXT("Dnd5eBestiaryRoot="), Root)) { return true; }
	for (const bool bNewer : {false, true})
	{
		const TCHAR* File = bNewer ? TEXT("bestiary-xmm.json") : TEXT("bestiary-mm.json");
		auto Document = MakeDocument(FString());
		Document.Context.RelativeFilePath = File;
		if (!TestTrue(File, FFileHelper::LoadFileToString(Document.JsonText, *FPaths::Combine(Root, File)))) { return false; }

		const auto Result = FDnd5eJsonCreatureReader::ReadCreature(Document,
			bNewer ? TEXT("Goblin Warrior") : TEXT("Goblin"), bNewer ? TEXT("XMM") : TEXT("MM"));
		if (!TestTrue(TEXT("Goblin imports"), Result.Status == EDnd5eJsonReadStatus::Success))
		{
			for (const auto& Diagnostic : Result.Diagnostics) { AddInfo(Diagnostic.JsonPointer + TEXT(" ") + Diagnostic.Message); }
			return false;
		}
		const auto& Creature = Result.Creature;
		TestTrue(TEXT("Published AC and contributors"), Creature.ArmorClasses.Num() == 1 &&
			Creature.ArmorClasses[0].Value == 15 && Creature.ArmorClasses[0].Sources.Num() == (bNewer ? 0 : 2));
		TestEqual(TEXT("Published average HP"), Creature.HitPoints.Average, bNewer ? 10 : 7);
		TestEqual(TEXT("Published Dexterity"), Creature.AbilityScores.Dexterity, bNewer ? 15 : 14);
		const auto& NimbleEscapeGroup = bNewer ? Creature.BonusActions : Creature.Traits;
		TestTrue(TEXT("Nimble Escape grouping"), NimbleEscapeGroup.ContainsByPredicate([](const auto& Entry)
		{
			return Entry.Name == TEXT("Nimble Escape");
		}));
	}
	return !HasAnyErrors();
}
#endif
