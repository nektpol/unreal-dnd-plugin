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
		Creature.CreatureSubtypes.Num() == 1 && Creature.CreatureSubtypes[0].Name == TEXT("lantern folk"));
	TestTrue(TEXT("Both AC forms retain sources in their own entry"), Creature.ArmorClasses.Num() == 2 &&
		Creature.ArmorClasses[0].Value == 12 && Creature.ArmorClasses[0].Sources.IsEmpty() &&
		Creature.ArmorClasses[1].Value == 14 && Creature.ArmorClasses[1].Sources.Num() == 2);
	TestTrue(TEXT("HP, movement and printed scores"), Creature.HitPoints.Average == 9 &&
		Creature.HitPoints.Formula.Expression == TEXT("2d8") && Creature.WalkSpeed.Unit == EDnd5eDistanceUnit::Feet &&
		Creature.WalkSpeed.Amount.Integer == 25 && Creature.AbilityScores.Dexterity == 13 && Creature.AbilityScores.Wisdom == 9);
	TestTrue(TEXT("Positive and negative skill bonuses"), Creature.Skills.Num() == 2 &&
		Creature.Skills.ContainsByPredicate([](const auto& Skill) { return Skill.Skill == TEXT("stealth") && Skill.Bonus == 6; }) &&
		Creature.Skills.ContainsByPredicate([](const auto& Skill) { return Skill.Skill == TEXT("arcana") && Skill.Bonus == -2; }));
	TestTrue(TEXT("Observations"), Creature.PassivePerception == 9 && Creature.ChallengeRating.Base == TEXT("1/4") &&
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureTypeTest, "Dnd5e.Bestiary.Reader.CreatureTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureTypeTest::RunTest(const FString& Parameters)
{
	const TPair<const TCHAR*, FGameplayTag> Types[] = {
		{TEXT("aberration"), Dnd5e::Creature::Type::Aberration},
		{TEXT("beast"), Dnd5e::Creature::Type::Beast},
		{TEXT("celestial"), Dnd5e::Creature::Type::Celestial},
		{TEXT("construct"), Dnd5e::Creature::Type::Construct},
		{TEXT("dragon"), Dnd5e::Creature::Type::Dragon},
		{TEXT("elemental"), Dnd5e::Creature::Type::Elemental},
		{TEXT("fey"), Dnd5e::Creature::Type::Fey},
		{TEXT("fiend"), Dnd5e::Creature::Type::Fiend},
		{TEXT("giant"), Dnd5e::Creature::Type::Giant},
		{TEXT("humanoid"), Dnd5e::Creature::Type::Humanoid},
		{TEXT("monstrosity"), Dnd5e::Creature::Type::Monstrosity},
		{TEXT("ooze"), Dnd5e::Creature::Type::Ooze},
		{TEXT("plant"), Dnd5e::Creature::Type::Plant},
		{TEXT("undead"), Dnd5e::Creature::Type::Undead},
	};

	const auto ReadType = [](const FString& TypeJson)
	{
		const FString Json = FString(StatBlock).Replace(
			TEXT(R"({"type":"fey","tags":["lantern folk"]})"), *TypeJson, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	for (const auto& Type : Types)
	{
		const FString Plain = FString::Printf(TEXT(R"("%s")"), Type.Key);
		const FString Nested = FString::Printf(TEXT(R"({"type":"%s","tags":["invented subtype"]})"), Type.Key);
		const auto PlainResult = ReadType(Plain);
		const auto NestedResult = ReadType(Nested);
		TestTrue(FString::Printf(TEXT("%s maps in both forms"), Type.Key),
			PlainResult.Status == EDnd5eJsonReadStatus::Success && PlainResult.Creature.CreatureType == Type.Value &&
			NestedResult.Status == EDnd5eJsonReadStatus::Success && NestedResult.Creature.CreatureType == Type.Value &&
			NestedResult.Creature.CreatureSubtypes.Num() == 1 &&
			NestedResult.Creature.CreatureSubtypes[0].Name == TEXT("invented subtype"));
	}

	const auto CheckUnsupported = [this, &ReadType](const TCHAR* TypeJson, const TCHAR* Pointer)
	{
		const auto Result = ReadType(TypeJson);
		TestTrue(Pointer, Result.Status == EDnd5eJsonReadStatus::Failure &&
			Result.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer &&
					Diagnostic.Code == FName(TEXT("Dnd5e.Json.UnsupportedRequiredValue"));
			}));
	};
	CheckUnsupported(TEXT(R"("invented type")"), TEXT("/monster/1/type"));
	CheckUnsupported(TEXT(R"({"type":"invented type"})"), TEXT("/monster/1/type/type"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureAlignmentTest, "Dnd5e.Bestiary.Reader.Alignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureAlignmentTest::RunTest(const FString& Parameters)
{
	const auto ReadAlignment = [](const FString& Field)
	{
		const FString Json = FString(StatBlock).Replace(
			TEXT(R"("alignment":["C","N"],)"), *Field, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Missing = ReadAlignment(TEXT(""));
	const auto Empty = ReadAlignment(TEXT(R"("alignment":[],)"));
	TestTrue(TEXT("Omitted and empty arrays have distinct presence"), Missing.Status == EDnd5eJsonReadStatus::Success &&
		!Missing.Creature.bHasAlignment && Missing.Creature.AlignmentOptions.IsEmpty() &&
		Empty.Status == EDnd5eJsonReadStatus::Success && Empty.Creature.bHasAlignment &&
		Empty.Creature.AlignmentOptions.IsEmpty());

	const auto Pair = ReadAlignment(TEXT(R"("alignment":["E","N"],)"));
	TestTrue(TEXT("Simple pair keeps source order and exact tags"), Pair.Status == EDnd5eJsonReadStatus::Success &&
		Pair.Creature.AlignmentOptions.Num() == 1 &&
		Pair.Creature.AlignmentOptions[0].Codes == TArray<FString>{TEXT("E"), TEXT("N")} &&
		Pair.Creature.Alignment.HasTagExact(Dnd5e::Alignment::Evil) &&
		Pair.Creature.Alignment.HasTagExact(Dnd5e::Alignment::Neutral));

	for (const TCHAR* Field : {TEXT(R"("alignment":["A"],)"), TEXT(R"("alignment":["NX","NY","N"],)")})
	{
		const auto Compound = ReadAlignment(Field);
		TestTrue(TEXT("Compound codes preserve a source option without simple tags"),
			Compound.Status == EDnd5eJsonReadStatus::Success && Compound.Creature.AlignmentOptions.Num() == 1 &&
			!Compound.Creature.AlignmentOptions[0].Codes.IsEmpty() && Compound.Creature.Alignment.IsEmpty());
	}

	const auto Alternatives = ReadAlignment(TEXT(R"("alignment":[{"alignment":["N","G"],"chance":0},
		{"special":"as its host","note":"when linked"}],)"));
	const auto& Options = Alternatives.Creature.AlignmentOptions;
	TestTrue(TEXT("Qualified alternatives preserve order, zero chance and special text"),
		Alternatives.Status == EDnd5eJsonReadStatus::Success && Alternatives.Creature.Alignment.IsEmpty() &&
		Options.Num() == 2 && Options[0].Codes == TArray<FString>{TEXT("N"), TEXT("G")} &&
		Options[0].bHasChance && Options[0].ChancePercent == 0 && !Options[1].bHasChance &&
		Options[1].Special == TEXT("as its host") && Options[1].Note == TEXT("when linked"));

	const auto CheckFailure = [this, &ReadAlignment](const TCHAR* Field, const TCHAR* Pointer)
	{
		const auto Result = ReadAlignment(Field);
		TestTrue(Pointer, Result.Status == EDnd5eJsonReadStatus::Failure &&
			Result.Creature.AlignmentOptions.IsEmpty() && Result.Diagnostics.ContainsByPredicate(
				[Pointer](const FDnd5eDiagnostic& Diagnostic) { return Diagnostic.JsonPointer == Pointer; }));
	};
	CheckFailure(TEXT(R"("alignment":["N",{"special":"as its host"}],)"), TEXT("/monster/1/alignment/1"));
	CheckFailure(TEXT(R"("alignment":[{"alignment":["N"],"chance":101}],)"), TEXT("/monster/1/alignment/0/chance"));
	CheckFailure(TEXT(R"("alignment":null,)"), TEXT("/monster/1/alignment"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureChallengeRatingTest, "Dnd5e.Bestiary.Reader.ChallengeRating",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureChallengeRatingTest::RunTest(const FString& Parameters)
{
	const auto Read = [](const TCHAR* Field)
	{
		const FString Json = FString(StatBlock).Replace(TEXT(R"(,"cr":"1/4")"), Field, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Missing = Read(TEXT(""));
	const auto Null = Read(TEXT(R"(,"cr":null)"));
	const auto Plain = Read(TEXT(R"(,"cr":"Unknown")"));
	TestTrue(TEXT("Omitted, null and authored text are distinct"),
		Missing.Status == EDnd5eJsonReadStatus::Success && Missing.Creature.ChallengeRating.Presence == EDnd5eValuePresence::Unspecified &&
		Null.Status == EDnd5eJsonReadStatus::Success && Null.Creature.ChallengeRating.Presence == EDnd5eValuePresence::ExplicitNull &&
		Plain.Status == EDnd5eJsonReadStatus::Success && Plain.Creature.ChallengeRating.Presence == EDnd5eValuePresence::Present &&
		Plain.Creature.ChallengeRating.Base == TEXT("Unknown") && !Plain.Creature.ChallengeRating.bHasXp);

	const auto Variants = Read(TEXT(R"(,"cr":{"cr":"1/4","lair":"1/2","coven":"1"})"));
	const auto& Ratings = Variants.Creature.ChallengeRating;
	TestTrue(TEXT("Base, lair, and coven ratings remain separate"), Variants.Status == EDnd5eJsonReadStatus::Success &&
		Ratings.Base == TEXT("1/4") && Ratings.Lair == TEXT("1/2") && Ratings.Coven == TEXT("1") &&
		!Ratings.bHasXp && !Ratings.bHasLairXp);

	const auto Overrides = Read(TEXT(R"(,"cr":{"cr":"3","xp":0,"xpLair":900})"));
	const auto& Xp = Overrides.Creature.ChallengeRating;
	TestTrue(TEXT("Zero XP override is present and lair XP stays separate"), Overrides.Status == EDnd5eJsonReadStatus::Success &&
		Xp.bHasXp && Xp.Xp == 0 && Xp.bHasLairXp && Xp.LairXp == 900 && Xp.Lair.IsEmpty());

	const auto CheckFailure = [this, &Read](const TCHAR* Field, const TCHAR* Pointer)
	{
		const auto Result = Read(Field);
		TestTrue(Pointer, Result.Status == EDnd5eJsonReadStatus::Failure &&
			Result.Creature.ChallengeRating.Presence == EDnd5eValuePresence::Unspecified &&
			Result.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(TEXT(R"(,"cr":{"lair":"6"})"), TEXT("/monster/1/cr/cr"));
	CheckFailure(TEXT(R"(,"cr":{"cr":"3","xpLair":-1})"), TEXT("/monster/1/cr/xpLair"));
	CheckFailure(TEXT(R"(,"cr":{"cr":"3","xp":1.5})"), TEXT("/monster/1/cr/xp"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureArmorClassTest, "Dnd5e.Bestiary.Reader.ArmorClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureArmorClassTest::RunTest(const FString& Parameters)
{
	const auto Read = [](const TCHAR* Armor)
	{
		const FString Json = FString(StatBlock).Replace(
			TEXT(R"([12,{"ac":14,"from":["Invented shell","{@item Glass Plate|SYN-B}"]}])"), Armor, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Result = Read(TEXT(R"json([0,{"ac":16,"condition":"with {@spell Invented Ward|SYN-B}","braces":true},
		{"ac":14,"from":["invented shell"],"condition":"while braced","braces":false},{"special":"12 + its proficiency bonus"}])json"));
	const auto& Armor = Result.Creature.ArmorClasses;
	if (!TestTrue(TEXT("All four entries convert in order"), Result.Status == EDnd5eJsonReadStatus::Success && Armor.Num() == 4))
	{
		return false;
	}
	TestTrue(TEXT("Authored zero is numeric"), Armor[0].bHasValue && Armor[0].Value == 0);
	TestTrue(TEXT("Condition retains its spell reference and parentheses hint"), Armor[1].bHasValue &&
		Armor[1].Value == 16 && Armor[1].bWrapInParentheses && Armor[1].Condition.ContainsByPredicate(
			[](const FDnd5eInlineContent& Inline) { return Inline.Payload.GetPtr<FDnd5eReferenceContent>() != nullptr; }));
	TestTrue(TEXT("Sources and condition coexist"), Armor[2].bHasValue && Armor[2].Value == 14 &&
		Armor[2].Sources.Num() == 1 && !Armor[2].Condition.IsEmpty() && !Armor[2].bWrapInParentheses);
	TestTrue(TEXT("Special formula has inline content and no numeric value"), !Armor[3].bHasValue && !Armor[3].Special.IsEmpty());

	const auto CheckFailure = [this, &Read](const TCHAR* Input, const TCHAR* Pointer)
	{
		const auto Failed = Read(Input);
		TestTrue(Pointer, Failed.Status == EDnd5eJsonReadStatus::Failure && Failed.Creature.ArmorClasses.IsEmpty() &&
			Failed.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(TEXT(R"([12,{"ac":16,"braces":"yes"}])"), TEXT("/monster/1/ac/1/braces"));
	CheckFailure(TEXT(R"([12,{"special":"varies","ac":16}])"), TEXT("/monster/1/ac/1/ac"));
	CheckFailure(TEXT(R"([12,{"ac":16,"condition":false}])"), TEXT("/monster/1/ac/1/condition"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureSubtypeTest, "Dnd5e.Bestiary.Reader.CreatureSubtypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureSubtypeTest::RunTest(const FString& Parameters)
{
	const auto Read = [](const TCHAR* Subtypes)
	{
		const FString Json = FString(StatBlock).Replace(TEXT(R"(["lantern folk"])"), Subtypes, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Result = Read(TEXT(R"(["plain",{"tag":"visible","prefix":"River"},{"tag":"hidden","prefix":"Night","prefixHidden":true}])"));
	const auto& Subtypes = Result.Creature.CreatureSubtypes;
	if (!TestTrue(TEXT("Mixed subtype forms convert in order"), Result.Status == EDnd5eJsonReadStatus::Success && Subtypes.Num() == 3))
	{
		return false;
	}
	TestTrue(TEXT("String has an empty visible prefix"), Subtypes[0].Name == TEXT("plain") &&
		Subtypes[0].Prefix.IsEmpty() && !Subtypes[0].bPrefixHidden);
	TestTrue(TEXT("Visible prefix is independent of subtype name"), Subtypes[1].Name == TEXT("visible") &&
		Subtypes[1].Prefix == TEXT("River") && !Subtypes[1].bPrefixHidden);
	TestTrue(TEXT("Hidden prefix remains available"), Subtypes[2].Name == TEXT("hidden") &&
		Subtypes[2].Prefix == TEXT("Night") && Subtypes[2].bPrefixHidden);

	const auto CheckFailure = [this, &Read](const TCHAR* Input, const TCHAR* Pointer)
	{
		const auto Failed = Read(Input);
		TestTrue(Pointer, Failed.Status == EDnd5eJsonReadStatus::Failure && Failed.Creature.CreatureSubtypes.IsEmpty() &&
			Failed.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(TEXT(R"(["plain",{"tag":"hidden","prefix":"Night","prefixHidden":"yes"}])"), TEXT("/monster/1/type/tags/1/prefixHidden"));
	CheckFailure(TEXT(R"([{"tag":"incomplete"}])"), TEXT("/monster/1/type/tags/0/prefix"));
	CheckFailure(TEXT(R"([false])"), TEXT("/monster/1/type/tags/0"));
	CheckFailure(TEXT("false"), TEXT("/monster/1/type/tags"));
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
