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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureHitPointsTest, "Dnd5e.Bestiary.Reader.HitPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureHitPointsTest::RunTest(const FString& Parameters)
{
	const auto Read = [](const TCHAR* Field)
	{
		const FString Json = FString(StatBlock).Replace(TEXT(R"("hp":{"average":9,"formula":"2d8"},)"),
			Field, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Missing = Read(TEXT(""));
	const auto Null = Read(TEXT(R"("hp":null,)"));
	TestTrue(TEXT("Omitted and null HP have distinct presence"),
		Missing.Status == EDnd5eJsonReadStatus::Success && Missing.Creature.HitPoints.Presence == EDnd5eValuePresence::Unspecified &&
		Null.Status == EDnd5eJsonReadStatus::Success && Null.Creature.HitPoints.Presence == EDnd5eValuePresence::ExplicitNull &&
		!Missing.Creature.HitPoints.bHasAverage && !Null.Creature.HitPoints.bHasAverage);

	const auto Printed = Read(TEXT(R"("hp":{"average":0,"formula":"1d4 - 4"},)"));
	const auto& Hp = Printed.Creature.HitPoints;
	TestTrue(TEXT("Authored zero and formula are retained"), Printed.Status == EDnd5eJsonReadStatus::Success &&
		Hp.Presence == EDnd5eValuePresence::Present && Hp.bHasAverage && Hp.Average == 0 &&
		Hp.Formula.Expression == TEXT("1d4 - 4") && Hp.Formula.Syntax == TEXT("5etools") && Hp.Special.IsEmpty());

	const auto Special = Read(TEXT(R"("hp":{"special":"Varies with {@spell Invented Ward|SYN-B}"},)"));
	TestTrue(TEXT("Special HP retains inline references"), Special.Status == EDnd5eJsonReadStatus::Success &&
		Special.Creature.HitPoints.Presence == EDnd5eValuePresence::Present && !Special.Creature.HitPoints.bHasAverage &&
		Special.Creature.HitPoints.Special.ContainsByPredicate(
			[](const FDnd5eInlineContent& Inline) { return Inline.Payload.GetPtr<FDnd5eReferenceContent>() != nullptr; }));

	const auto CheckFailure = [this, &Read](const TCHAR* Input, const TCHAR* Pointer)
	{
		const auto Failed = Read(Input);
		TestTrue(Pointer, Failed.Status == EDnd5eJsonReadStatus::Failure &&
			Failed.Creature.HitPoints.Presence == EDnd5eValuePresence::Unspecified &&
			Failed.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(TEXT(R"("hp":{"average":7},)"), TEXT("/monster/1/hp/formula"));
	CheckFailure(TEXT(R"("hp":{"special":12},)"), TEXT("/monster/1/hp/special"));
	CheckFailure(TEXT(R"("hp":{"average":7,"special":"varies"},)"), TEXT("/monster/1/hp/average"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureMovementTest, "Dnd5e.Bestiary.Reader.Movement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureMovementTest::RunTest(const FString& Parameters)
{
	using FMode = EDnd5eCreatureMovementMode;
	const auto Read = [](const TCHAR* Field)
	{
		const FString Json = FString(StatBlock).Replace(TEXT(R"("speed":{"walk":25},)"), Field, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Missing = Read(TEXT(""));
	const auto Null = Read(TEXT(R"("speed":null,)"));
	TestTrue(TEXT("Omitted and null movement are distinct"), Missing.Status == EDnd5eJsonReadStatus::Success &&
		Missing.Creature.Movement.Presence == EDnd5eValuePresence::Unspecified &&
		Null.Status == EDnd5eJsonReadStatus::Success && Null.Creature.Movement.Presence == EDnd5eValuePresence::ExplicitNull);
	const auto Fly = Read(TEXT(R"("speed":{"fly":40,"canHover":true},)"));
	TestTrue(TEXT("Fly and hover need no walking speed"), Fly.Status == EDnd5eJsonReadStatus::Success &&
		Fly.Creature.Movement.bCanHover && Fly.Creature.Movement.Speeds.Num() == 1 &&
		Fly.Creature.Movement.Speeds[0].Mode == FMode::Fly && Fly.Creature.Movement.Speeds[0].Distance.Amount.Integer == 40 &&
		Fly.Creature.WalkSpeed.Unit == EDnd5eDistanceUnit::Unspecified);

	const auto Result = Read(TEXT(R"json("speed":{"walk":{"number":0,"condition":"while rooted"},"swim":30,
		"alternate":{"walk":[{"number":50,"condition":"in another form"},60]},
		"choose":{"from":["fly","climb"],"amount":20,"note":"choose one"}},)json"));
	const auto& Movement = Result.Creature.Movement;
	if (!TestTrue(TEXT("Combined movement converts"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TestTrue(TEXT("Conditional zero is preserved in movement and projected into WalkSpeed"),
		Movement.Presence == EDnd5eValuePresence::Present && Movement.Speeds.Num() == 2 &&
		Movement.Speeds[0].Mode == FMode::Walk && !Movement.Speeds[0].Condition.IsEmpty() &&
		Movement.Speeds[1].Mode == FMode::Swim && Result.Creature.WalkSpeed.Unit == EDnd5eDistanceUnit::Feet &&
		Result.Creature.WalkSpeed.Amount.Integer == 0);
	TestTrue(TEXT("Alternates and choices keep their ordering"), Movement.AlternateSpeeds.Num() == 2 &&
		Movement.AlternateSpeeds[0].Distance.Amount.Integer == 50 && !Movement.AlternateSpeeds[0].Condition.IsEmpty() &&
		Movement.AlternateSpeeds[1].Distance.Amount.Integer == 60 &&
		Movement.ChoiceModes == TArray<FMode>{FMode::Fly, FMode::Climb} &&
		Movement.ChoiceDistance.Amount.Integer == 20 && !Movement.ChoiceNote.IsEmpty());

	const auto CheckFailure = [this, &Read](const TCHAR* Field, const TCHAR* Pointer)
	{
		const auto Failed = Read(Field);
		TestTrue(Pointer, Failed.Status == EDnd5eJsonReadStatus::Failure &&
			Failed.Creature.Movement.Presence == EDnd5eValuePresence::Unspecified &&
			Failed.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(TEXT(R"("speed":{"choose":{"from":["teleport"],"amount":20}},)"), TEXT("/monster/1/speed/choose/from/0"));
	CheckFailure(TEXT(R"("speed":{"walk":{"number":-5}},)"), TEXT("/monster/1/speed/walk/number"));
	CheckFailure(TEXT(R"("speed":{"alternate":{"walk":[{"number":20,"condition":false}]}},)"),
		TEXT("/monster/1/speed/alternate/walk/0/condition"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureDirectRecordTest, "Dnd5e.Bestiary.Reader.DirectRecordForms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureDirectRecordTest::RunTest(const FString& Parameters)
{
	using FPresence = EDnd5eValuePresence;
	const auto Read = [](const TCHAR* Original, const TCHAR* Replacement)
	{
		return FDnd5eJsonCreatureReader::ReadCreature(
			MakeDocument(FString(StatBlock).Replace(Original, Replacement, ESearchCase::CaseSensitive)),
			TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const TCHAR* PassiveField = TEXT(R"("passive":9,)");
	const auto Omitted = Read(PassiveField, TEXT(""));
	const auto Null = Read(PassiveField, TEXT(R"("passive":null,)"));
	const auto Zero = Read(PassiveField, TEXT(R"("passive":0,)"));
	const auto Formula = Read(PassiveField, TEXT(R"("passive":"10 + PB",)"));
	TestTrue(TEXT("Passive Perception retains omitted, null, zero and formula forms"),
		Omitted.Status == EDnd5eJsonReadStatus::Success && Omitted.Creature.PassivePerceptionPresence == FPresence::Unspecified &&
		Null.Status == EDnd5eJsonReadStatus::Success && Null.Creature.PassivePerceptionPresence == FPresence::ExplicitNull &&
		Zero.Status == EDnd5eJsonReadStatus::Success && Zero.Creature.PassivePerceptionPresence == FPresence::Present &&
		Zero.Creature.PassivePerception == 0 && Zero.Creature.PassivePerceptionFormula.IsEmpty() &&
		Formula.Status == EDnd5eJsonReadStatus::Success && Formula.Creature.PassivePerceptionPresence == FPresence::Present &&
		Formula.Creature.PassivePerceptionFormula == TEXT("10 + PB"));

	const TCHAR* ArmorField = TEXT(R"("ac":[12,{"ac":14,"from":["Invented shell","{@item Glass Plate|SYN-B}"]}],)");
	const auto OmittedArmor = Read(ArmorField, TEXT(""));
	const auto NullArmor = Read(ArmorField, TEXT(R"("ac":null,)"));
	const auto EmptyArmor = Read(ArmorField, TEXT(R"("ac":[],)"));
	TestTrue(TEXT("AC retains omitted, null and present-empty states"),
		OmittedArmor.Status == EDnd5eJsonReadStatus::Success && OmittedArmor.Creature.ArmorClassPresence == FPresence::Unspecified &&
		NullArmor.Status == EDnd5eJsonReadStatus::Success && NullArmor.Creature.ArmorClassPresence == FPresence::ExplicitNull &&
		EmptyArmor.Status == EDnd5eJsonReadStatus::Success && EmptyArmor.Creature.ArmorClassPresence == FPresence::Present &&
		EmptyArmor.Creature.ArmorClasses.IsEmpty());

	const TCHAR* TypeField = TEXT(R"({"type":"fey","tags":["lantern folk"]})");
	const auto Choice = Read(TypeField, TEXT(R"({"type":{"choose":["fiend","fey"]},"tags":["invented"]})"));
	TestTrue(TEXT("Type choices keep order and outer subtypes without selecting a type"),
		Choice.Status == EDnd5eJsonReadStatus::Success && !Choice.Creature.CreatureType.IsValid() &&
		Choice.Creature.CreatureTypeChoices == TArray<FGameplayTag>{Dnd5e::Creature::Type::Fiend, Dnd5e::Creature::Type::Fey} &&
		Choice.Creature.CreatureSubtypes.Num() == 1 && Choice.Creature.CreatureSubtypes[0].Name == TEXT("invented"));

	const auto CheckFailure = [this, &Read](const TCHAR* Original, const TCHAR* Replacement, const TCHAR* Pointer)
	{
		const auto Result = Read(Original, Replacement);
		TestTrue(Pointer, Result.Status == EDnd5eJsonReadStatus::Failure &&
			Result.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(PassiveField, TEXT(R"("passive":false,)"), TEXT("/monster/1/passive"));
	CheckFailure(PassiveField, TEXT(R"("passive":"   ",)"), TEXT("/monster/1/passive"));
	CheckFailure(ArmorField, TEXT(R"("ac":false,)"), TEXT("/monster/1/ac"));
	CheckFailure(TypeField, TEXT(R"({"type":{"choose":[]}})"), TEXT("/monster/1/type/type/choose"));
	CheckFailure(TypeField, TEXT(R"({"type":{"choose":["fey","unknown"]}})"), TEXT("/monster/1/type/type/choose/1"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureSkillsAndPageTest, "Dnd5e.Bestiary.Reader.SkillsAndPage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureSkillsAndPageTest::RunTest(const FString& Parameters)
{
	const auto Read = [](const TCHAR* Skills, const TCHAR* Page)
	{
		const FString Json = FString(StatBlock)
			.Replace(TEXT(R"({"stealth":"+6","arcana":"-2"})"), Skills, ESearchCase::CaseSensitive)
			.Replace(TEXT(R"("page":4)"), Page, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Result = Read(TEXT(R"json({"stealth":"+0","arcana":"-2","athletics":"+2 plus PB",
		"other":[{"oneOf":{"history":"+7","nature":"+6"}},{"oneOf":{"religion":"+3"}}]})json"), TEXT(R"("page":0)"));
	if (!TestTrue(TEXT("Skill forms and page zero convert"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto& Creature = Result.Creature;
	TestTrue(TEXT("Signed values and formula remain distinct"), Creature.Skills.Num() == 3 &&
		Creature.Skills.ContainsByPredicate([](const auto& Skill) { return Skill.Skill == TEXT("stealth") && Skill.bHasNumericBonus && Skill.Bonus == 0; }) &&
		Creature.Skills.ContainsByPredicate([](const auto& Skill) { return Skill.Skill == TEXT("arcana") && Skill.bHasNumericBonus && Skill.Bonus == -2; }) &&
		Creature.Skills.ContainsByPredicate([](const auto& Skill) { return Skill.Skill == TEXT("athletics") && !Skill.bHasNumericBonus && Skill.BonusExpression == TEXT("+2 plus PB"); }));
	TestTrue(TEXT("Choice groups stay ordered and separate from fixed skills"), Creature.SkillChoices.Num() == 2 &&
		Creature.SkillChoices[0].Options.Num() == 2 &&
		Creature.SkillChoices[0].Options.ContainsByPredicate([](const auto& Option)
		{
			return Option.Skill == TEXT("history") && Option.bHasNumericBonus && Option.Bonus == 7;
		}) &&
		Creature.SkillChoices[1].Options.Num() == 1 && Creature.SkillChoices[1].Options[0].Skill == TEXT("religion"));
	TestTrue(TEXT("Numeric page zero is unspecified"), Creature.Header.Provenance.Num() == 1 &&
		Creature.Header.Provenance[0].Page.Kind == EDnd5ePageReferenceKind::Unspecified);

	const auto TextPage = Read(TEXT("{}"), TEXT(R"("page":"0")"));
	TestTrue(TEXT("Text page zero is preserved"), TextPage.Status == EDnd5eJsonReadStatus::Success &&
		TextPage.Creature.Header.Provenance.Num() == 1 &&
		TextPage.Creature.Header.Provenance[0].Page.Kind == EDnd5ePageReferenceKind::Text &&
		TextPage.Creature.Header.Provenance[0].Page.Text == TEXT("0"));

	const auto CheckFailure = [this, &Read](const TCHAR* Skills, const TCHAR* Page, const TCHAR* Pointer)
	{
		const auto Failed = Read(Skills, Page);
		TestTrue(Pointer, Failed.Status == EDnd5eJsonReadStatus::Failure && Failed.Creature.SkillChoices.IsEmpty() &&
			Failed.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(TEXT(R"({"other":[{"oneOf":{"arcana":false}}]})"), TEXT(R"("page":4)"), TEXT("/monster/1/skill/other/0/oneOf/arcana"));
	CheckFailure(TEXT(R"({"stealth":"+2147483648"})"), TEXT(R"("page":4)"), TEXT("/monster/1/skill/stealth"));
	CheckFailure(TEXT("{}"), TEXT(R"("page":-1)"), TEXT("/monster/1/page"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureScorePresenceTest, "Dnd5e.Bestiary.Reader.AbilityScorePresence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureScorePresenceTest::RunTest(const FString& Parameters)
{
	const auto Read = [](const TCHAR* Scores)
	{
		const FString Json = FString(StatBlock).Replace(
			TEXT(R"("str":7,"dex":13,"con":11,"int":12,"wis":9,"cha":8,)"), Scores, ESearchCase::CaseSensitive);
		return FDnd5eJsonCreatureReader::ReadCreature(MakeDocument(Json), TEXT("Lantern Scout"), TEXT("SYN-B"));
	};

	const auto Missing = Read(TEXT(""));
	TestTrue(TEXT("All scores may be omitted without dropping movement"),
		Missing.Status == EDnd5eJsonReadStatus::Success && !Missing.Creature.AbilityScores.bHasScores &&
		Missing.Creature.WalkSpeed.Amount.Integer == 25);

	const auto Zero = Read(TEXT(R"("str":0,"dex":13,"con":11,"int":12,"wis":9,"cha":8,)"));
	TestTrue(TEXT("A complete set with authored zero is present"),
		Zero.Status == EDnd5eJsonReadStatus::Success && Zero.Creature.AbilityScores.bHasScores &&
		Zero.Creature.AbilityScores.Strength == 0 && Zero.Creature.AbilityScores.Dexterity == 13);

	const auto CheckFailure = [this, &Read](const TCHAR* Scores, const TCHAR* Pointer)
	{
		const auto Result = Read(Scores);
		TestTrue(Pointer, Result.Status == EDnd5eJsonReadStatus::Failure && !Result.Creature.AbilityScores.bHasScores &&
			Result.Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(TEXT(R"("dex":13,"con":11,"int":12,"wis":9,"cha":8,)"), TEXT("/monster/1/str"));
	CheckFailure(TEXT(R"("str":7,"dex":13,"con":11,"int":12,"wis":null,"cha":8,)"), TEXT("/monster/1/wis"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyAuditTest, "Dnd5e.Bestiary.Reader.CopyAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureCopyAuditTest::RunTest(const FString& Parameters)
{
	auto Children = MakeDocument(TEXT(R"json({"monster":[
		{"name":"Child","source":"C","_copy":{"name":"middle","source":"m"}},
		{"name":"Missing","source":"C","_copy":{"name":"Absent","source":"M"}},
		{"name":"Cycle","source":"C","_copy":{"name":"cycle","source":"c"}},
		{"name":"Deferred child","source":"C","_copy":{"name":"Deferred","source":"M"}},
		{"name":"Preserves page","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_preserve":{"page":true}}},
		{"name":"Clears page","source":"C","page":null,"_copy":{"name":"Lantern Scout","source":"SYN-B","_preserve":{"*":true}}},
		{"name":"Ambiguous parent","source":"C","_copy":{"name":"Duplicate","source":"M"}}
	]})json"));
	Children.Context.RelativeFilePath = TEXT("children.json");
	auto Middle = MakeDocument(TEXT(R"json({"monster":[
		{"name":"Middle","source":"M","_copy":{"name":"Lantern Scout","source":"SYN-B"}},
		{"name":"Deferred","source":"M","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":{"mode":"replaceTxt","replace":"old","with":"new"}}}},
		{"name":"Duplicate","source":"M"},
		{"name":"duplicate","source":"m"}
	]})json"));
	Middle.Context.RelativeFilePath = TEXT("middle.json");
	const auto Root = MakeDocument(FString(StatBlock).Replace(TEXT("\"page\":4"), TEXT("\"page\":false")));
	const TArray<FDnd5eJsonDocument> Documents{Children, Middle, Root};
	const auto Reports = FDnd5eJsonCreatureReader::AuditDocuments(Documents);
	if (!TestTrue(TEXT("Reports and rows preserve input order"), Reports.Num() == 3 && Reports[0].Rows.Num() == 7 &&
		Reports[1].Rows.Num() == 4 && Reports[2].Rows.Num() == 2)) { return false; }

	const auto& Rows = Reports[0].Rows;
	TestTrue(TEXT("Reversed chain converts and the child identity survives"),
		Rows[0].Status == EDnd5eJsonReadStatus::Success && Rows[0].bIsCopy &&
		Rows[0].Name == TEXT("Child") && Rows[0].SourceId == TEXT("C") &&
		Reports[1].Rows[0].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("Explicit null clears a preserved parent field"), Rows[5].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("The original parent retains its invalid page"), Reports[2].Rows[1].Status == EDnd5eJsonReadStatus::Failure);

	const auto CheckFailure = [this, &Rows](int32 Index, const TCHAR* File, const TCHAR* Pointer, const TCHAR* Message)
	{
		TestTrue(Message, Rows[Index].Status == EDnd5eJsonReadStatus::Failure &&
			Rows[Index].Diagnostics.ContainsByPredicate([File, Pointer, Message](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.RelativeFilePath == File && Diagnostic.JsonPointer == Pointer &&
					Diagnostic.Message.Contains(Message, ESearchCase::IgnoreCase);
			}));
	};
	CheckFailure(1, TEXT("children.json"), TEXT("/monster/1/_copy"), TEXT("was not found"));
	CheckFailure(2, TEXT("children.json"), TEXT("/monster/2/_copy"), TEXT("cycle"));
	CheckFailure(3, TEXT("middle.json"), TEXT("/monster/1/_copy/_mod"), TEXT("deferred"));
	CheckFailure(4, TEXT("children.json"), TEXT("/monster/4/page"), TEXT("page"));
	CheckFailure(6, TEXT("children.json"), TEXT("/monster/6/_copy"), TEXT("ambiguous"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyArrayTest, "Dnd5e.Bestiary.Reader.CopyArrays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureCopyArrayTest::RunTest(const FString& Parameters)
{
	const auto Children = MakeDocument(TEXT(R"json({"monster":[
		{"name":"Middle","source":"M","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":[
			{"mode":"prependArr","items":{"name":"First","entries":["First."]}},
			{"mode":"appendArr","items":[{"name":"Last","entries":["Last."]}]}]}}},
		{"name":"Grandchild","source":"C","_copy":{"name":"Middle","source":"M","_mod":{"trait":{
			"mode":"appendArr","items":{"name":"Invalid","entries":false}}}}},
		{"name":"Sibling","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":{
			"mode":"appendArr","items":{"name":"Invalid","entries":false}}}}},
		{"name":"New bonus","source":"C","bonus":null,"_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{
			"bonus":{"mode":"appendArr","items":{"name":"New","entries":["New."]}}}}},
		{"name":"Invalid modifier","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{
			"trait":{"mode":"appendArr","items":false}}}},
		{"name":"Prepended invalid","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":{
			"mode":"prependArr","items":{"name":"Invalid","entries":false}}}}}
	]})json"));
	const TArray<FDnd5eJsonDocument> Documents{Children, MakeDocument(StatBlock)};
	const auto Reports = FDnd5eJsonCreatureReader::AuditDocuments(Documents);
	if (!TestTrue(TEXT("Both documents were audited"), Reports.Num() == 2 && Reports[0].Rows.Num() == 6 &&
		Reports[1].Rows.Num() == 2)) { return false; }

	const auto& Rows = Reports[0].Rows;
	TestTrue(TEXT("Prepend and append support arrays and singleton items"), Rows[0].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("Append creates a target removed by the child"), Rows[3].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("Parent still converts"), Reports[1].Rows[1].Status == EDnd5eJsonReadStatus::Success);

	const auto CheckFailure = [this, &Rows](int32 Index, const TCHAR* Pointer)
	{
		TestTrue(Pointer, Rows[Index].Status == EDnd5eJsonReadStatus::Failure &&
			Rows[Index].Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(1, TEXT("/monster/1/trait/3/entries"));
	CheckFailure(2, TEXT("/monster/2/trait/1/entries"));
	CheckFailure(4, TEXT("/monster/4/_copy/_mod/trait/items"));
	CheckFailure(5, TEXT("/monster/5/trait/0/entries"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyReplaceRemoveTest, "Dnd5e.Bestiary.Reader.CopyReplaceRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureCopyReplaceRemoveTest::RunTest(const FString& Parameters)
{
	const auto Children = MakeDocument(TEXT(R"json({"monster":[
		{"name":"Replaced","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":[
			{"mode":"replaceArr","replace":"Glow","items":[{"name":"First","entries":["First."]},{"name":"Second","entries":["Second."]}]},
			{"mode":"removeArr","names":["First","Second"]}]}}},
		{"name":"Sibling","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":{"mode":"removeArr","names":"Glow"}}}},
		{"name":"Delete and append","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":[
			"remove",{"mode":"appendArr","items":{"name":"New","entries":false}}]}}},
		{"name":"Missing replacement","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":{
			"mode":"replaceArr","replace":"Missing","items":[]}}}},
		{"name":"Forced removal","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":{
			"mode":"removeArr","names":["Missing","Glow"],"force":true}}}},
		{"name":"Missing removal","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"trait":{
			"mode":"removeArr","names":["Glow","Missing"]}}}}
	]})json"));
	const TArray<FDnd5eJsonDocument> Documents{Children, MakeDocument(StatBlock)};
	const auto Reports = FDnd5eJsonCreatureReader::AuditDocuments(Documents);
	if (!TestTrue(TEXT("All fixtures were audited"), Reports.Num() == 2 && Reports[0].Rows.Num() == 6)) { return false; }

	const auto& Rows = Reports[0].Rows;
	TestTrue(TEXT("Replacement makes both new names available for removal"), Rows[0].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("Sibling can still remove the original name"), Rows[1].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("Forced removal skips a missing name"), Rows[4].Status == EDnd5eJsonReadStatus::Success);

	const auto CheckFailure = [this, &Rows](int32 Index, const TCHAR* Pointer)
	{
		TestTrue(Pointer, Rows[Index].Status == EDnd5eJsonReadStatus::Failure &&
			Rows[Index].Diagnostics.ContainsByPredicate([Pointer](const FDnd5eDiagnostic& Diagnostic)
			{
				return Diagnostic.JsonPointer == Pointer && Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
			}));
	};
	CheckFailure(2, TEXT("/monster/2/trait/0/entries"));
	CheckFailure(3, TEXT("/monster/3/_copy/_mod/trait/replace"));
	CheckFailure(5, TEXT("/monster/5/_copy/_mod/trait/names/1"));
	return !HasAnyErrors();
}

#if UE_ENABLE_ICU
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyTextTest, "Dnd5e.Bestiary.Reader.CopyText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureCopyTextTest::RunTest(const FString& Parameters)
{
	const auto Children = MakeDocument(TEXT(R"json({"monster":[
		{"name":"Capture","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"*":{
			"mode":"replaceTxt","replace":"\\b(soft)\\b","with":"{@syntheticUnknown $1}","flags":"i"}}}},
		{"name":"Protected","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"*":{
			"mode":"replaceTxt","replace":"dice","with":"syntheticUnknown"}}}},
		{"name":"Unprotected","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"*":{
			"mode":"replaceTxt","replace":"dice","with":"syntheticUnknown","tagInsensitive":true}}}},
		{"name":"Renamed","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{
			"*":{"mode":"replaceTxt","replace":"(Glow)","with":"New $1","props":["name"]},
			"trait":{"mode":"replaceArr","replace":"Glow","items":{"name":"Glow","entries":["New trait."]}}}}},
		{"name":"Uses renamed trait","source":"C","_copy":{"name":"Renamed","source":"C","_mod":{
			"trait":{"mode":"removeArr","names":"New Glow"}}}},
		{"name":"Invalid pattern","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B","_mod":{"*":{
			"mode":"replaceTxt","replace":"[","with":"x"}}}}
	]})json"));
	const FString Parent = FString(StatBlock).Replace(TEXT("A soft glow."), TEXT("SOFT soft softwood {@dice 1d6}."));
	const TArray<FDnd5eJsonDocument> Documents{Children, MakeDocument(Parent)};
	const auto Reports = FDnd5eJsonCreatureReader::AuditDocuments(Documents);
	if (!TestTrue(TEXT("Text fixtures were audited"), Reports.Num() == 2 && Reports[0].Rows.Num() == 6 &&
		Reports[1].Rows.Num() == 2)) { return false; }

	const auto& Rows = Reports[0].Rows;
	const auto CountInlineWarnings = [](const FDnd5eCreatureAuditRow& Row)
	{
		int32 Count = 0;
		for (const auto& Diagnostic : Row.Diagnostics)
		{
			if (Diagnostic.JsonPointer.EndsWith(TEXT("/trait/0/entries/0")) &&
				Diagnostic.Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedContent")))
			{
				++Count;
			}
		}
		return Count;
	};
	TestTrue(TEXT("Global case-insensitive boundary matches expand the capture replacement twice"),
		Rows[0].Status == EDnd5eJsonReadStatus::Success && CountInlineWarnings(Rows[0]) == 2);
	TestTrue(TEXT("Default replacement protects inline tags, with an opt-in to rewrite them"),
		Rows[1].Status == EDnd5eJsonReadStatus::Success && CountInlineWarnings(Rows[1]) == 0 &&
		Rows[2].Status == EDnd5eJsonReadStatus::Success && CountInlineWarnings(Rows[2]) == 1 &&
		CountInlineWarnings(Reports[1].Rows[1]) == 0);
	TestTrue(TEXT("Named array operations run first and descendants see the renamed trait"), Rows[3].Status == EDnd5eJsonReadStatus::Success && Rows[4].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("Invalid regex points to the authored pattern"), Rows[5].Status == EDnd5eJsonReadStatus::Failure &&
		Rows[5].Diagnostics.ContainsByPredicate([](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/5/_copy/_mod/*/replace") &&
				Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error;
		}));
	return !HasAnyErrors();
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCreatureCopyTemplateTest, "Dnd5e.Bestiary.Reader.CopyTemplates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCreatureCopyTemplateTest::RunTest(const FString& Parameters)
{
	const auto Children = MakeDocument(TEXT(R"json({"monster":[
		{"name":"Templated","source":"C","action":[{"name":"Authored","entries":["Authored action."]}],
			"_copy":{"name":"Lantern Scout","source":"SYN-B",
				"_templates":[{"name":"first","source":"t"},{"name":"Second","source":"T"}],
				"_mod":{"trait":{"mode":"appendArr","items":{"name":"Child","entries":["Child trait."]}}}}},
		{"name":"Descendant","source":"C","_copy":{"name":"Templated","source":"C",
			"_mod":{"trait":{"mode":"removeArr","names":["Second Root","Replacement"]},
				"action":{"mode":"removeArr","names":"Authored"}}}},
		{"name":"Missing","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B",
			"_templates":[{"name":"Absent","source":"T"}]}},
		{"name":"Invalid","source":"C","_copy":{"name":"Lantern Scout","source":"SYN-B",
			"_templates":[{"name":"Invalid","source":"T"}]}}
	]})json"));
	auto Templates = MakeDocument(TEXT(R"json({"monsterTemplate":[
		{"name":"First","source":"T","apply":{"_root":{"trait":[{"name":"First Root","entries":["First root."]}],"action":[]}}},
		{"name":"Second","source":"T","apply":{"_root":{"trait":[{"name":"Second Root","entries":["Second root."]}]},
			"_mod":{"trait":{"mode":"replaceArr","replace":"Child","items":{"name":"Replacement","entries":["Replacement trait."]}}}}},
		{"name":"Invalid","source":"T","apply":{"_mod":[]}}
	]})json"));
	Templates.Context.RelativeFilePath = TEXT("fixtures/template.json");
	const TArray<FDnd5eJsonDocument> Documents{Children, MakeDocument(StatBlock)};
	const auto Reports = FDnd5eJsonCreatureReader::AuditDocuments(Documents, Templates);
	if (!TestTrue(TEXT("Template fixtures were audited"), Reports.Num() == 2 && Reports[0].Rows.Num() == 4))
	{
		return false;
	}

	const auto& Rows = Reports[0].Rows;
	TestTrue(TEXT("Later roots win, authored fields survive, and child modifiers run before template modifiers"),
		Rows[0].Status == EDnd5eJsonReadStatus::Success && Rows[1].Status == EDnd5eJsonReadStatus::Success);
	TestTrue(TEXT("A missing template points to the creature reference"),
		Rows[2].Status == EDnd5eJsonReadStatus::Failure &&
		Rows[2].Diagnostics.ContainsByPredicate([](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.JsonPointer == TEXT("/monster/2/_copy/_templates/0");
		}));
	TestTrue(TEXT("Malformed modifiers point to the template document"),
		Rows[3].Status == EDnd5eJsonReadStatus::Failure &&
		Rows[3].Diagnostics.ContainsByPredicate([](const FDnd5eDiagnostic& Diagnostic)
		{
			return Diagnostic.RelativeFilePath == TEXT("fixtures/template.json") &&
				Diagnostic.JsonPointer == TEXT("/monsterTemplate/2/apply/_mod");
		}));
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
