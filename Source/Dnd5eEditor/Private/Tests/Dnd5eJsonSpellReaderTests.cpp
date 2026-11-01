// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Spells/Dnd5eJsonSpellReader.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

using namespace Dnd5eTests;

namespace
{
	TSharedPtr<FJsonValue> ParseFixture(const FString& Text)
	{
		TSharedPtr<FJsonValue> Value;
		const bool bParsed = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Value, FJsonSerializer::EFlags::StoreNumbersAsStrings);
		check(bParsed && Value.IsValid());
		return Value;
	}

	TSharedRef<FJsonObject> MakeSpell()
	{
		return ParseFixture(TEXT(R"json({
			"name":"Clockwork Spark","source":"SYNTH","page":7,"level":0,"school":"V",
			"time":[{"number":1,"unit":"action"}],
			"range":{"type":"point","distance":{"type":"feet","amount":25}},
			"components":{"v":true,"s":true},
			"duration":[{"type":"timed","duration":{"type":"minute","amount":2}}],
			"entries":["A tiny clockwork light flickers above an empty surface."]
		})json"))->AsObject().ToSharedRef();
	}

	FDnd5eJsonSpellReadRequest MakeRequest(const TSharedRef<FJsonObject>& Spell)
	{
		FDnd5eJsonSpellReadRequest Request;
		Request.Context.DatasetNamespace = TEXT("synthetic");
		Request.Context.RelativeFilePath = TEXT("spells/demo.json");
		Request.SpellIndex = 0;
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetArrayField(TEXT("spell"), {MakeShared<FJsonValueObject>(Spell)});
		FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Request.JsonText));
		return Request;
	}

	bool ExpectFailure(FAutomationTestBase& Test, const FDnd5eJsonSpellReadRequest& Request, const TCHAR* Code, const FString& Pointer, int32 ExpectedDiagnostics = 1)
	{
		const FDnd5eJsonSpellReadResult Result = FDnd5eJsonSpellReader::ReadSpell(Request);
		const FDnd5eSpell Default;
		Test.TestTrue(TEXT("A rejected read returns Failure"), Result.Status == EDnd5eJsonReadStatus::Failure);
		Test.TestTrue(TEXT("Failure never exposes a partially normalized spell"), Equal(FDnd5eSpell::StaticStruct(), &Result.Spell, &Default));
		if (!Test.TestEqual(TEXT("Diagnostic count"), Result.Diagnostics.Num(), ExpectedDiagnostics)) { return false; }
		const FDnd5eDiagnostic& Error = Result.Diagnostics.Last();
		Test.TestTrue(TEXT("The final diagnostic is an Error"), Error.Severity == EDnd5eDiagnosticSeverity::Error);
		Test.TestTrue(TEXT("Controlled diagnostic code"), Error.Code.ToString().Equals(FString(TEXT("Dnd5e.Json.")) + Code, ESearchCase::CaseSensitive));
		Test.TestTrue(TEXT("Exact source pointer"), Error.JsonPointer.Equals(Pointer, ESearchCase::CaseSensitive));
		Test.TestEqual(TEXT("The supplied source label follows the error"), Error.RelativeFilePath, Request.Context.RelativeFilePath);
		Test.TestFalse(TEXT("An emitted diagnostic has a message"), Error.Message.IsEmpty());
		return !Test.HasAnyErrors();
	}

	struct FRejectedField
	{
		const TCHAR* Name;
		const TCHAR* Field;
		const TCHAR* Json;
		const TCHAR* Code;
		const TCHAR* Pointer;
	};

	const TArray<FRejectedField>& RejectedFields()
	{
		static const TArray<FRejectedField> Cases = {
			{TEXT("MissingName"), TEXT("name"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/name")},
			{TEXT("MissingSource"), TEXT("source"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/source")},
			{TEXT("MissingLevel"), TEXT("level"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/level")},
			{TEXT("MissingSchool"), TEXT("school"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/school")},
			{TEXT("MissingTime"), TEXT("time"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/time")},
			{TEXT("MissingRange"), TEXT("range"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/range")},
			{TEXT("MissingComponents"), TEXT("components"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/components")},
			{TEXT("MissingDuration"), TEXT("duration"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/duration")},
			{TEXT("MissingEntries"), TEXT("entries"), nullptr, TEXT("MissingRequiredField"), TEXT("/spell/0/entries")},
			{TEXT("NullName"), TEXT("name"), TEXT("null"), TEXT("InvalidFieldValue"), TEXT("/spell/0/name")},
			{TEXT("EmptyName"), TEXT("name"), TEXT("\"\""), TEXT("InvalidFieldValue"), TEXT("/spell/0/name")},
			{TEXT("NameWhitespace"), TEXT("name"), TEXT("\" Name\""), TEXT("InvalidFieldValue"), TEXT("/spell/0/name")},
			{TEXT("NameControl"), TEXT("name"), TEXT("\"Name\\u0000Suffix\""), TEXT("InvalidFieldValue"), TEXT("/spell/0/name")},
			{TEXT("NameLoneSurrogate"), TEXT("name"), TEXT("\"\\ud800\""), TEXT("InvalidFieldValue"), TEXT("/spell/0/name")},
			{TEXT("SourceWrongType"), TEXT("source"), TEXT("7"), TEXT("InvalidFieldValue"), TEXT("/spell/0/source")},
			{TEXT("SourceWhitespace"), TEXT("source"), TEXT("\"SYNTH \""), TEXT("InvalidFieldValue"), TEXT("/spell/0/source")},
			{TEXT("QuotedLevel"), TEXT("level"), TEXT("\"0\""), TEXT("InvalidFieldValue"), TEXT("/spell/0/level")},
			{TEXT("FractionLevel"), TEXT("level"), TEXT("1.0"), TEXT("InvalidFieldValue"), TEXT("/spell/0/level")},
			{TEXT("ExponentLevel"), TEXT("level"), TEXT("1e0"), TEXT("InvalidFieldValue"), TEXT("/spell/0/level")},
			{TEXT("NegativeLevel"), TEXT("level"), TEXT("-1"), TEXT("InvalidFieldValue"), TEXT("/spell/0/level")},
			{TEXT("LargeLevel"), TEXT("level"), TEXT("10"), TEXT("InvalidFieldValue"), TEXT("/spell/0/level")},
			{TEXT("UnknownSchool"), TEXT("school"), TEXT("\"Z\""), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/school")},
			{TEXT("WrongSchoolCase"), TEXT("school"), TEXT("\"v\""), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/school")},
			{TEXT("EmptyTimes"), TEXT("time"), TEXT("[]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/time")},
			{TEXT("NullTime"), TEXT("time"), TEXT("[null]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/time/0")},
			{TEXT("ZeroTime"), TEXT("time"), TEXT("[{\"number\":0,\"unit\":\"action\"}]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/time/0/number")},
			{TEXT("OverflowTime"), TEXT("time"), TEXT("[{\"number\":2147483648,\"unit\":\"action\"}]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/time/0/number")},
			{TEXT("UnknownTime"), TEXT("time"), TEXT("[{\"number\":1,\"unit\":\"unknown\"}]"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/time/0/unit")},
			{TEXT("WrongTimeCase"), TEXT("time"), TEXT("[{\"number\":1,\"unit\":\"Action\"}]"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/time/0/unit")},
			{TEXT("ConditionWrongType"), TEXT("time"), TEXT("[{\"number\":1,\"unit\":\"reaction\",\"condition\":null}]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/time/0/condition")},
			{TEXT("MissingAreaDistance"), TEXT("range"), TEXT("{\"type\":\"cone\"}"), TEXT("MissingRequiredField"), TEXT("/spell/0/range/distance")},
			{TEXT("MissingDistance"), TEXT("range"), TEXT("{\"type\":\"point\"}"), TEXT("MissingRequiredField"), TEXT("/spell/0/range/distance")},
			{TEXT("UnknownDistance"), TEXT("range"), TEXT("{\"type\":\"point\",\"distance\":{\"type\":\"furlongs\",\"amount\":1}}"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/range/distance/type")},
			{TEXT("SymbolicAmount"), TEXT("range"), TEXT("{\"type\":\"point\",\"distance\":{\"type\":\"touch\",\"amount\":5}}"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/range/distance/amount")},
			{TEXT("NegativeDistance"), TEXT("range"), TEXT("{\"type\":\"point\",\"distance\":{\"type\":\"feet\",\"amount\":-1}}"), TEXT("InvalidFieldValue"), TEXT("/spell/0/range/distance/amount")},
			{TEXT("QuotedDistance"), TEXT("range"), TEXT("{\"type\":\"point\",\"distance\":{\"type\":\"feet\",\"amount\":\"0.5\"}}"), TEXT("InvalidFieldValue"), TEXT("/spell/0/range/distance/amount")},
			{TEXT("NullComponents"), TEXT("components"), TEXT("null"), TEXT("InvalidFieldValue"), TEXT("/spell/0/components")},
			{TEXT("QuotedComponent"), TEXT("components"), TEXT("{\"v\":\"false\"}"), TEXT("InvalidFieldValue"), TEXT("/spell/0/components/v")},
			{TEXT("NullMaterial"), TEXT("components"), TEXT("{\"m\":null}"), TEXT("InvalidFieldValue"), TEXT("/spell/0/components/m")},
			{TEXT("QuotedRoyalty"), TEXT("components"), TEXT("{\"r\":\"true\"}"), TEXT("InvalidFieldValue"), TEXT("/spell/0/components/r")},
			{TEXT("EmptyDurations"), TEXT("duration"), TEXT("[]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/duration")},
			{TEXT("UnknownDuration"), TEXT("duration"), TEXT("[{\"type\":\"forever\"}]"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/duration/0/type")},
			{TEXT("InstantTime"), TEXT("duration"), TEXT("[{\"type\":\"instant\",\"duration\":{}}]"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/duration/0/duration")},
			{TEXT("ZeroDuration"), TEXT("duration"), TEXT("[{\"type\":\"timed\",\"duration\":{\"type\":\"minute\",\"amount\":0}}]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/duration/0/duration/amount")},
			{TEXT("UnknownDurationUnit"), TEXT("duration"), TEXT("[{\"type\":\"timed\",\"duration\":{\"type\":\"century\",\"amount\":1}}]"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/duration/0/duration/type")},
			{TEXT("WrongMaximumFlag"), TEXT("duration"), TEXT("[{\"type\":\"timed\",\"duration\":{\"type\":\"minute\",\"amount\":1,\"upTo\":1}}]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/duration/0/duration/upTo")},
			{TEXT("WrongConcentrationFlag"), TEXT("duration"), TEXT("[{\"type\":\"timed\",\"duration\":{\"type\":\"minute\",\"amount\":1},\"concentration\":\"true\"}]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/duration/0/concentration")},
			{TEXT("EmptyEntries"), TEXT("entries"), TEXT("[]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/entries")},
			{TEXT("MissingSectionEntries"), TEXT("entries"), TEXT("[{\"type\":\"entries\",\"name\":\"Invented section\"}]"), TEXT("MissingRequiredField"), TEXT("/spell/0/entries/0/entries")},
			{TEXT("NullEntry"), TEXT("entries"), TEXT("[null]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/entries/0")},
			{TEXT("BooleanEntry"), TEXT("entries"), TEXT("[false]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/entries/0")},
			{TEXT("NullHigherEntries"), TEXT("entriesHigherLevel"), TEXT("null"), TEXT("InvalidFieldValue"), TEXT("/spell/0/entriesHigherLevel")},
			{TEXT("ZeroPage"), TEXT("page"), TEXT("0"), TEXT("InvalidFieldValue"), TEXT("/spell/0/page")},
			{TEXT("EmptyPage"), TEXT("page"), TEXT("\"\""), TEXT("InvalidFieldValue"), TEXT("/spell/0/page")},
			{TEXT("OverflowPage"), TEXT("page"), TEXT("2147483648"), TEXT("InvalidFieldValue"), TEXT("/spell/0/page")},
			{TEXT("NullAlias"), TEXT("alias"), TEXT("null"), TEXT("InvalidFieldValue"), TEXT("/spell/0/alias")},
			{TEXT("NumericAlias"), TEXT("alias"), TEXT("[1]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/alias/0")},
			{TEXT("UnknownMeta"), TEXT("meta"), TEXT("{\"invented\":true}"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/meta/invented")},
			{TEXT("WrongRitualFlag"), TEXT("meta"), TEXT("{\"ritual\":1}"), TEXT("InvalidFieldValue"), TEXT("/spell/0/meta/ritual")},
			{TEXT("WrongSrd"), TEXT("srd"), TEXT("1"), TEXT("InvalidFieldValue"), TEXT("/spell/0/srd")},
			{TEXT("WrongBasicRules"), TEXT("basicRules"), TEXT("\"true\""), TEXT("InvalidFieldValue"), TEXT("/spell/0/basicRules")},
			{TEXT("WrongClassification"), TEXT("areaTags"), TEXT("[1]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/areaTags/0")},
			{TEXT("WrongSourceReference"), TEXT("otherSources"), TEXT("[\"SYNTH\"]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/otherSources/0")},
			{TEXT("WrongReferenceSourceObject"), TEXT("referenceSources"), TEXT("[{\"page\":1}]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/referenceSources/0")},
			{TEXT("EmptyReferenceSource"), TEXT("referenceSources"), TEXT("[\"\"]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/referenceSources/0")},
			{TEXT("UnknownReferenceMetadata"), TEXT("otherSources"), TEXT("[{\"source\":\"SYNTH\",\"unknown\":true}]"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/otherSources/0/unknown")},
			{TEXT("EmptyReprint"), TEXT("reprintedAs"), TEXT("[\"\"]"), TEXT("InvalidFieldValue"), TEXT("/spell/0/reprintedAs/0")},
			{TEXT("MissingScalingLabel"), TEXT("scalingLevelDice"), TEXT("{}"), TEXT("MissingRequiredField"), TEXT("/spell/0/scalingLevelDice/label")},
			{TEXT("CopyInstruction"), TEXT("_copy"), TEXT("{}"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/_copy")},
			{TEXT("UnknownEscapedKey"), TEXT("a/b~c"), TEXT("true"), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/a~1b~0c")}
		};
		return Cases;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonSpellBasicTest, "Dnd5e.Json.Reader.BasicMapping", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonSpellBasicTest::RunTest(const FString& Parameters)
{
	const auto Request = MakeRequest(MakeSpell());
	const auto Before = Request;
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestTrue(TEXT("Complete synthetic input succeeds"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TestEqual(TEXT("No warnings for the basic supported form"), Result.Diagnostics.Num(), 0);
	const auto& Spell = Result.Spell;
	TestEqual(TEXT("Display name"), Spell.Header.DisplayName, FString(TEXT("Clockwork Spark")));
	TestTrue(TEXT("Versioned exact key"), Spell.Header.RecordId.SourceKey.Equals(TEXT("5etools:spell:v1:Clockwork%20Spark|SYNTH"), ESearchCase::CaseSensitive));
	TestEqual(TEXT("Dataset namespace"), Spell.Header.RecordId.DatasetNamespace, Request.Context.DatasetNamespace);
	TestTrue(TEXT("Spell category"), Spell.Header.RecordId.Category == Dnd5e::Category::Spell);
	TestEqual(TEXT("Cantrip zero is retained"), Spell.Level, 0);
	TestTrue(TEXT("School mapping"), Spell.Schools.HasTagExact(Dnd5e::Magic::School::Evocation));
	if (!TestEqual(TEXT("One casting option"), Spell.CastingTimes.Num(), 1)) { return false; }
	TestEqual(TEXT("Casting quantity"), Spell.CastingTimes[0].Quantity, 1);
	TestTrue(TEXT("Shared action unit"), Spell.CastingTimes[0].Unit == Dnd5e::Activation::Time::Action);
	TestTrue(TEXT("Measured range presence and kind"), Spell.Range.bHasDistance && Spell.Range.Kind == Dnd5e::Range::Kind::Distance);
	TestTrue(TEXT("Feet and integer amount"), Spell.Range.Distance.Unit == EDnd5eDistanceUnit::Feet && Spell.Range.Distance.Amount.Kind == EDnd5eNumberKind::Integer);
	TestEqual(TEXT("Range amount"), Spell.Range.Distance.Amount.Integer, 25);
	TestEqual(TEXT("Original range number token"), Spell.Range.Distance.Amount.SourceLiteral, FString(TEXT("25")));
	TestTrue(TEXT("Verbal and somatic"), Spell.Components.Types.HasTagExact(Dnd5e::Component::Type::Verbal) && Spell.Components.Types.HasTagExact(Dnd5e::Component::Type::Somatic));
	TestFalse(TEXT("No invented material"), Spell.Components.bHasMaterial);
	if (!TestEqual(TEXT("One duration"), Spell.Durations.Num(), 1)) { return false; }
	TestTrue(TEXT("Timed duration"), Spell.Durations[0].Duration.Kind == EDnd5eDurationKind::Timed && Spell.Durations[0].Duration.bHasTime);
	TestEqual(TEXT("Duration amount"), Spell.Durations[0].Duration.Time.Amount.Integer, 2);
	TestEqual(TEXT("Original duration number token"), Spell.Durations[0].Duration.Time.Amount.SourceLiteral, FString(TEXT("2")));
	TestFalse(TEXT("No invented concentration"), Spell.Traits.HasTagExact(Dnd5e::Trait::Concentration));
	if (!TestEqual(TEXT("One description block"), Spell.Description.Num(), 1)) { return false; }
	FDnd5eParagraphContent Paragraph;
	FDnd5eTextContent Text;
	if (!TestTrue(TEXT("Typed paragraph"), UDnd5eContentLibrary::TryGetParagraph(Spell.Description[0], Paragraph)) || !TestEqual(TEXT("One inline"), Paragraph.Inlines.Num(), 1)) { return false; }
	TestTrue(TEXT("Typed text"), UDnd5eContentLibrary::TryGetInlineText(Paragraph.Inlines[0], Text));
	TestEqual(TEXT("Exact synthetic text"), Text.Text, FString(TEXT("A tiny clockwork light flickers above an empty surface.")));
	TestEqual(TEXT("Block pointer"), Spell.Description[0].Metadata.JsonPointer, FString(TEXT("/spell/0/entries/0")));
	TestEqual(TEXT("Inline source ID"), Paragraph.Inlines[0].Metadata.SourceId, FString(TEXT("SYNTH")));
	if (!TestEqual(TEXT("One primary provenance"), Spell.Header.Provenance.Num(), 1)) { return false; }
	TestEqual(TEXT("Source pointer"), Spell.Header.Provenance[0].JsonPointer, FString(TEXT("/spell/0")));
	TestEqual(TEXT("Source page"), Spell.Header.Provenance[0].Page.Number, 7);
	TestTrue(TEXT("Request is immutable"), Equal(FDnd5eJsonSpellReadRequest::StaticStruct(), &Request, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eJsonSpellRejectedFieldsTest, "Dnd5e.Json.Reader.RejectedField", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eJsonSpellRejectedFieldsTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (int32 Index = 0; Index < RejectedFields().Num(); ++Index) { Names.Add(RejectedFields()[Index].Name); Commands.Add(FString::FromInt(Index)); }
}
bool FDnd5eJsonSpellRejectedFieldsTest::RunTest(const FString& Parameters)
{
	const auto& Case = RejectedFields()[FCString::Atoi(*Parameters)];
	auto Spell = MakeSpell();
	if (Case.Json) { Spell->SetField(Case.Field, ParseFixture(FString(TEXT("{\"v\":")) + Case.Json + TEXT("}"))->AsObject()->TryGetField(TEXT("v"))); }
	else { Spell->RemoveField(Case.Field); }
	return ExpectFailure(*this, MakeRequest(Spell), Case.Code, Case.Pointer);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonSpellSelectionTest, "Dnd5e.Json.Reader.ContextSyntaxAndSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonSpellSelectionTest::RunTest(const FString& Parameters)
{
	auto Request = MakeRequest(MakeSpell());
	for (const TCHAR* Namespace : {TEXT(""), TEXT(" synthetic"), TEXT("synthetic "), TEXT("a\nb")})
	{
		auto Invalid = Request; Invalid.Context.DatasetNamespace = Namespace;
		ExpectFailure(*this, Invalid, TEXT("InvalidContext"), TEXT(""));
	}
	for (const TCHAR* Path : {TEXT("/root.json"), TEXT("C:/root.json"), TEXT("a\\b.json"), TEXT("a//b"), TEXT("a/../b"), TEXT("./a"), TEXT("a/")})
	{
		auto Invalid = Request; Invalid.Context.RelativeFilePath = Path;
		ExpectFailure(*this, Invalid, TEXT("InvalidContext"), TEXT(""));
	}
	Request.SpellIndex = INDEX_NONE;
	ExpectFailure(*this, Request, TEXT("NoSpellSelection"), TEXT(""));
	Request.SpellIndex = 0;
	struct FInput { const TCHAR* Text; const TCHAR* Code; const TCHAR* Pointer; };
	const FInput Inputs[] = {
		{TEXT(" \r\n\t"), TEXT("EmptyInput"), TEXT("")}, {TEXT("{"), TEXT("InvalidSyntax"), TEXT("")},
		{TEXT("{}{}"), TEXT("InvalidSyntax"), TEXT("")}, {TEXT("null"), TEXT("InvalidSyntax"), TEXT("")},
		{TEXT("[]"), TEXT("InvalidRoot"), TEXT("")}, {TEXT("{}"), TEXT("MissingSpellArray"), TEXT("/spell")},
		{TEXT("{\"Spell\":[]}"), TEXT("MissingSpellArray"), TEXT("/spell")}, {TEXT("{\"spell\":null}"), TEXT("InvalidSpellArray"), TEXT("/spell")},
		{TEXT("{\"spell\":[]}"), TEXT("SpellIndexOutOfBounds"), TEXT("/spell/0")}, {TEXT("{\"spell\":[null]}"), TEXT("InvalidSpellObject"), TEXT("/spell/0")}
	};
	for (const auto& Input : Inputs) { Request.JsonText = Input.Text; ExpectFailure(*this, Request, Input.Code, Input.Pointer); }
	auto Spell = MakeSpell();
	Request = MakeRequest(Spell);
	Request.JsonText = TEXT("{\"spell\":[null,") + Request.JsonText.Mid(Request.JsonText.Find(TEXT("[")) + 1);
	Request.SpellIndex = 1;
	Request.Context.RelativeFilePath.Empty();
	const auto Selected = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (TestTrue(TEXT("Explicit selection skips an unselected invalid record"), Selected.Status == EDnd5eJsonReadStatus::Success))
	{
		TestEqual(TEXT("Selected provenance index"), Selected.Spell.Header.Provenance[0].JsonPointer, FString(TEXT("/spell/1")));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonSpellIdentityTest, "Dnd5e.Json.Reader.IdentityAndOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonSpellIdentityTest::RunTest(const FString& Parameters)
{
	auto Spell = MakeSpell();
	Spell->SetStringField(TEXT("name"), TEXT("Ω | % 😀"));
	Spell->SetStringField(TEXT("source"), TEXT("S|%"));
	auto Request = MakeRequest(Spell);
	auto Result = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestTrue(TEXT("Unicode identity succeeds"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TestTrue(TEXT("UTF-8 components use uppercase escaping"), Result.Spell.Header.RecordId.SourceKey.Equals(TEXT("5etools:spell:v1:%CE%A9%20%7C%20%25%20%F0%9F%98%80|S%7C%25"), ESearchCase::CaseSensitive));
	const auto Before = Result;
	Request.Context.RelativeFilePath = TEXT("different/location.json");
	const auto Moved = FDnd5eJsonSpellReader::ReadSpell(Request);
	TestTrue(TEXT("File labels do not change identity"), Moved.Spell.Header.RecordId.SourceKey.Equals(Result.Spell.Header.RecordId.SourceKey, ESearchCase::CaseSensitive));
	Spell->SetStringField(TEXT("source"), TEXT("s|%"));
	const auto ChangedCase = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	TestFalse(TEXT("Source case is preserved in v1 identity"), ChangedCase.Spell.Header.RecordId.SourceKey.Equals(Result.Spell.Header.RecordId.SourceKey, ESearchCase::CaseSensitive));
	Result.Spell.Header.DisplayName = TEXT("Changed output");
	TestTrue(TEXT("A retained earlier result owns its values"), Before.Spell.Header.DisplayName.Equals(TEXT("Ω | % 😀"), ESearchCase::CaseSensitive));
	Request.JsonText = TEXT("{}");
	Result = FDnd5eJsonSpellReader::ReadSpell(Request);
	const FDnd5eSpell Default;
	TestTrue(TEXT("Replacing success clears the old spell"), Result.Status == EDnd5eJsonReadStatus::Failure && Equal(FDnd5eSpell::StaticStruct(), &Result.Spell, &Default));
	TestEqual(TEXT("Replacing success replaces diagnostics"), Result.Diagnostics.Num(), 1);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonSpellVocabularyTest, "Dnd5e.Json.Reader.SupportedVocabulary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonSpellVocabularyTest::RunTest(const FString& Parameters)
{
	struct FTagCase { const TCHAR* Source; FGameplayTag Expected; };
	const FTagCase Schools[] = {
		{TEXT("A"), Dnd5e::Magic::School::Abjuration}, {TEXT("C"), Dnd5e::Magic::School::Conjuration}, {TEXT("D"), Dnd5e::Magic::School::Divination},
		{TEXT("E"), Dnd5e::Magic::School::Enchantment}, {TEXT("V"), Dnd5e::Magic::School::Evocation}, {TEXT("I"), Dnd5e::Magic::School::Illusion},
		{TEXT("N"), Dnd5e::Magic::School::Necromancy}, {TEXT("T"), Dnd5e::Magic::School::Transmutation}, {TEXT("P"), Dnd5e::Magic::School::Psionic}
	};
	for (const auto& School : Schools)
	{
		auto Spell = MakeSpell(); Spell->SetStringField(TEXT("school"), School.Source);
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		TestTrue(FString(TEXT("School ")) + School.Source, Result.Status == EDnd5eJsonReadStatus::Success && Result.Spell.Schools.Num() == 1 && Result.Spell.Schools.HasTagExact(School.Expected));
	}
	const FTagCase Times[] = {
		{TEXT("action"), Dnd5e::Activation::Time::Action}, {TEXT("bonus"), Dnd5e::Activation::Time::BonusAction}, {TEXT("reaction"), Dnd5e::Activation::Time::Reaction},
		{TEXT("round"), Dnd5e::Activation::Time::Round}, {TEXT("minute"), Dnd5e::Activation::Time::Minute}, {TEXT("hour"), Dnd5e::Activation::Time::Hour}
	};
	for (const auto& Time : Times)
	{
		auto Spell = MakeSpell(); Spell->GetArrayField(TEXT("time"))[0]->AsObject()->SetStringField(TEXT("unit"), Time.Source);
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		TestTrue(FString(TEXT("Casting unit ")) + Time.Source, Result.Status == EDnd5eJsonReadStatus::Success && Result.Spell.CastingTimes[0].Unit == Time.Expected);
	}
	const FTagCase Ranges[] = {
		{TEXT("self"), Dnd5e::Range::Kind::Self}, {TEXT("touch"), Dnd5e::Range::Kind::Touch}, {TEXT("sight"), Dnd5e::Range::Kind::Sight},
		{TEXT("unlimited"), Dnd5e::Range::Kind::Unlimited}, {TEXT("plane"), Dnd5e::Range::Kind::UnlimitedSamePlane}
	};
	for (const auto& Range : Ranges)
	{
		auto Spell = MakeSpell();
		auto Distance = Spell->GetObjectField(TEXT("range"))->GetObjectField(TEXT("distance"));
		Distance->RemoveField(TEXT("amount")); Distance->SetStringField(TEXT("type"), Range.Source);
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		TestTrue(FString(TEXT("Symbolic range ")) + Range.Source, Result.Status == EDnd5eJsonReadStatus::Success && Result.Spell.Range.Kind == Range.Expected && !Result.Spell.Range.bHasDistance && Result.Spell.Range.Distance.Amount.Kind == EDnd5eNumberKind::Unspecified);
	}
	struct FUnitCase { const TCHAR* Source; EDnd5eTimeUnit Expected; };
	const FUnitCase Units[] = {
		{TEXT("turn"), EDnd5eTimeUnit::Turn}, {TEXT("round"), EDnd5eTimeUnit::Round}, {TEXT("minute"), EDnd5eTimeUnit::Minute}, {TEXT("hour"), EDnd5eTimeUnit::Hour},
		{TEXT("day"), EDnd5eTimeUnit::Day}, {TEXT("week"), EDnd5eTimeUnit::Week}, {TEXT("month"), EDnd5eTimeUnit::Month}, {TEXT("year"), EDnd5eTimeUnit::Year}
	};
	for (const auto& Unit : Units)
	{
		auto Spell = MakeSpell(); Spell->GetArrayField(TEXT("duration"))[0]->AsObject()->GetObjectField(TEXT("duration"))->SetStringField(TEXT("type"), Unit.Source);
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		TestTrue(FString(TEXT("Duration unit ")) + Unit.Source, Result.Status == EDnd5eJsonReadStatus::Success && Result.Spell.Durations[0].Duration.Time.Unit == Unit.Expected);
	}
	auto Spell = MakeSpell();
	Spell->SetNumberField(TEXT("level"), 9);
	Spell->SetObjectField(TEXT("components"), MakeShared<FJsonObject>());
	Spell->SetField(TEXT("duration"), ParseFixture(TEXT("{\"v\":[{\"type\":\"instant\"}]}"))->AsObject()->TryGetField(TEXT("v")));
	Spell->GetObjectField(TEXT("range"))->GetObjectField(TEXT("distance"))->SetStringField(TEXT("type"), TEXT("miles"));
	Spell->GetObjectField(TEXT("range"))->GetObjectField(TEXT("distance"))->SetField(TEXT("amount"), MakeShared<FJsonValueNumberString>(FString(TEXT("2147483647"))));
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	if (TestTrue(TEXT("Upper integer boundary, empty components, and instantaneous form succeed"), Result.Status == EDnd5eJsonReadStatus::Success))
	{
		TestEqual(TEXT("Maximum int32 without overflow"), Result.Spell.Range.Distance.Amount.Integer, MAX_int32);
		TestTrue(TEXT("Miles mapping"), Result.Spell.Range.Distance.Unit == EDnd5eDistanceUnit::Miles);
		TestTrue(TEXT("No invented time on instant duration"), Result.Spell.Durations[0].Duration.Kind == EDnd5eDurationKind::Instantaneous && !Result.Spell.Durations[0].Duration.bHasTime);
		TestEqual(TEXT("Explicit empty components"), Result.Spell.Components.Types.Num(), 0);
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonSpellRetentionTest, "Dnd5e.Json.Reader.MetadataFallbackAndAssociations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonSpellRetentionTest::RunTest(const FString& Parameters)
{
	auto Spell = MakeSpell();
	auto Extra = ParseFixture(TEXT(R"json({
		"alias":["Invented Alias",""],"meta":{"ritual":true},"miscTags":["SYNTHETIC"],"areaTags":["ST"],
		"srd":"Invented label","srd52":false,"basicRules":true,"basicRules2024":false,"hasFluff":true,"hasFluffImages":false,
		"otherSources":[{"source":"EXTRA","page":2147483647}],"additionalSources":[{"source":"EXTRA2","page":"Appendix A"}],"referenceSources":["SYNREF"],
		"reprintedAs":["New Invented Spark|SYNTH2","Later Spark|SYNTH3"],
		"entries":["", "An {@future invented} light.", "Text with \"quotes\", a \\ slash, and Ω."],
		"entriesHigherLevel":["An {@future invented} change."],
		"time":[{"number":1,"unit":"reaction","condition":"An invented trigger.","note":"An invented note."},{"number":2,"unit":"minute"}],
		"duration":[{"type":"instant"},{"type":"timed","concentration":true,"duration":{"type":"hour","amount":3,"upTo":true}}]
	})json"))->AsObject();
	for (const auto& Pair : Extra->Values) { Spell->SetField(Pair.Key, Pair.Value); }
	Spell->SetStringField(TEXT("page"), TEXT("Appendix Z"));
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	if (!TestTrue(TEXT("Retained limitations are accepted"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto& Output = Result.Spell;
	if (!TestEqual(TEXT("Only unsupported inline fallbacks need warnings"), Result.Diagnostics.Num(), 2)) { return false; }
	TestEqual(TEXT("First retained fragment location"), Result.Diagnostics[0].JsonPointer, FString(TEXT("/spell/0/entries/1")));
	TestEqual(TEXT("Higher-level retained fragment location"), Result.Diagnostics[1].JsonPointer, FString(TEXT("/spell/0/entriesHigherLevel/0")));
	TestEqual(TEXT("Fallback warning code"), Result.Diagnostics[0].Code, FName(TEXT("Dnd5e.Json.PreservedUnsupportedContent")));
	for (const auto& Diagnostic : Result.Diagnostics) { TestTrue(TEXT("Accepted diagnostics are warnings"), Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning); }
	FDnd5eUnsupportedContent Unsupported;
	FDnd5eParagraphContent Paragraph;
	if (!TestTrue(TEXT("Paragraph retains typed neighbors"), UDnd5eContentLibrary::TryGetParagraph(Output.Description[1], Paragraph)) || !TestEqual(TEXT("Three inline fragments"), Paragraph.Inlines.Num(), 3)) { return false; }
	TestTrue(TEXT("Fallback payload can be extracted"), UDnd5eContentLibrary::TryGetUnsupportedInline(Paragraph.Inlines[1], Unsupported));
	TestEqual(TEXT("The complete unknown token survives"), Unsupported.SourceFragment, FString(TEXT("{@future invented}")));
	TestTrue(TEXT("Fallback retains text format"), Unsupported.SourceFormat.Equals(TEXT("5etools")) && Unsupported.FragmentKind == EDnd5eSourceFragmentKind::Text && Unsupported.DisplayText.IsEmpty());
	TestTrue(TEXT("Concentration stays on its duration option"), Output.Durations[1].Requirements.HasTagExact(Dnd5e::Trait::Concentration) && !Output.Durations[0].Requirements.HasTagExact(Dnd5e::Trait::Concentration));
	TestTrue(TEXT("Concentration summary and ritual"), Output.Traits.HasTagExact(Dnd5e::Trait::Concentration) && Output.Traits.HasTagExact(Dnd5e::Spell::Trait::Ritual));
	TestTrue(TEXT("Maximum flag"), Output.Durations[1].Duration.bIsMaximum);
	TestEqual(TEXT("Casting options preserve order"), Output.CastingTimes[1].Quantity, 2);
	TestEqual(TEXT("Condition location"), Output.CastingTimes[0].Condition[0].Metadata.JsonPointer, FString(TEXT("/spell/0/time/0/condition")));
	TestEqual(TEXT("Aliases retain all entries"), Output.Header.Aliases.Num(), 2);
	TestTrue(TEXT("Textual page form"), Output.Header.Provenance[0].Page.Kind == EDnd5ePageReferenceKind::Text);
	TestTrue(TEXT("No invented classification or area from evidence"), Output.Header.Classification.IsEmpty() && Output.Areas.IsEmpty());
	const auto Metadata = ParseFixture(Output.Header.Provenance[0].AdditionalMetadataJson)->AsObject();
	TestTrue(TEXT("Retained Boolean is not a string"), Metadata->TryGetField(TEXT("srd52"))->Type == EJson::Boolean);
	TestTrue(TEXT("Retained string remains string"), Metadata->TryGetField(TEXT("srd"))->Type == EJson::String);
	TestTrue(TEXT("Retained page is an exact numeric token"), Metadata->GetArrayField(TEXT("otherSources"))[0]->AsObject()->TryGetField(TEXT("page"))->AsString().Equals(TEXT("2147483647")));
	TestEqual(TEXT("Reference sources remain source-ID strings"), Metadata->GetArrayField(TEXT("referenceSources"))[0]->AsString(), FString(TEXT("SYNREF")));
	TestTrue(TEXT("Classification evidence retained"), ParseFixture(Output.Header.Provenance[0].ClassificationEvidenceJson)->AsObject()->HasField(TEXT("areaTags")));
	const auto Identity = ParseFixture(Output.Header.Provenance[0].IdentityFieldsJson)->AsObject();
	TestEqual(TEXT("Original identity evidence"), Identity->GetStringField(TEXT("name")), FString(TEXT("Clockwork Spark")));
	TestEqual(TEXT("Both directed relationships retained"), Output.Relationships.Num(), 2);
	TestTrue(TEXT("Source-explicit reprints normalize without requiring a loaded target"), Output.Relationships[0].bHasTargetRecord && Output.Relationships[0].TargetRecordId.DatasetNamespace == TEXT("synthetic") && Output.Relationships[0].TargetRecordId.SourceKey == TEXT("5etools:spell:v1:New%20Invented%20Spark|SYNTH2"));
	TestEqual(TEXT("Original reprint reference"), Output.Relationships[0].SourceReference, FString(TEXT("New Invented Spark|SYNTH2")));
	Spell->SetField(TEXT("basicRules"), MakeShared<FJsonValueNull>());
	ExpectFailure(*this, MakeRequest(Spell), TEXT("InvalidFieldValue"), TEXT("/spell/0/basicRules"), 3);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonSpellPreflightTest, "Dnd5e.Json.Reader.DuplicatesAndLimits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonSpellPreflightTest::RunTest(const FString& Parameters)
{
	auto Request = MakeRequest(MakeSpell());
	for (const TCHAR* Text : {TEXT("{\"spell\":[],\"spell\":[]}"), TEXT("{\"spell\":[],\"Spell\":[]}"), TEXT("{\"\":1,\"\":2}"), TEXT("{\"a\":{\"x\":1,\"X\":2},\"spell\":[]}")})
	{
		Request.JsonText = Text; ExpectFailure(*this, Request, TEXT("DuplicateProperty"), TEXT(""));
	}
	const auto Base = MakeRequest(MakeSpell());
	const FString SpellArray = Base.JsonText.Mid(Base.JsonText.Find(TEXT("[")));
	for (int32 Depth : {63, 64})
	{
		Request = Base;
		Request.JsonText = TEXT("{\"extra\":") + FString::ChrN(Depth, TEXT('[')) + FString::ChrN(Depth, TEXT(']')) + TEXT(",\"spell\":") + SpellArray;
		if (Depth == 63) { TestTrue(TEXT("64-container boundary succeeds"), FDnd5eJsonSpellReader::ReadSpell(Request).Status == EDnd5eJsonReadStatus::Success); }
		else { ExpectFailure(*this, Request, TEXT("ResourceLimitExceeded"), TEXT("")); }
	}
	Request = Base;
	Request.JsonText += FString::ChrN(16777216 - Request.JsonText.Len(), TEXT(' '));
	TestTrue(TEXT("Input length boundary succeeds"), FDnd5eJsonSpellReader::ReadSpell(Request).Status == EDnd5eJsonReadStatus::Success);
	Request.JsonText.AppendChar(TEXT(' '));
	ExpectFailure(*this, Request, TEXT("ResourceLimitExceeded"), TEXT(""));
	Request = Base;
	FString ExcessValues;
	ExcessValues.Reserve(5000020);
	for (int32 Index = 0; Index < 1000000; ++Index) { ExcessValues += TEXT("null,"); }
	ExcessValues += TEXT("null");
	Request.JsonText = TEXT("{\"extra\":[") + ExcessValues + TEXT("],\"spell\":") + SpellArray;
	ExpectFailure(*this, Request, TEXT("ResourceLimitExceeded"), TEXT(""));
	return !HasAnyErrors();
}

// Discovered only when a caller explicitly supplies a local file on the command line.
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eJsonExternalSpellTest, "Dnd5e.Json.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eJsonExternalSpellTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeFile="), Path))
	{
		Names.Add(TEXT("SelectedSpell"));
		Commands.Add(TEXT("SelectedSpell"));
	}
}
bool FDnd5eJsonExternalSpellTest::RunTest(const FString& Parameters)
{
	FString Path;
	int32 Index = INDEX_NONE;
	if (!TestTrue(TEXT("Explicit local file argument"), FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeFile="), Path))) { return false; }
	if (!TestTrue(TEXT("Explicit nonnegative spell index"), FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeIndex="), Index) && Index >= 0)) { return false; }
	FDnd5eJsonSpellReadRequest Request;
	Request.Context.DatasetNamespace = TEXT("external-smoke");
	Request.Context.RelativeFilePath = TEXT("spells/external-smoke.json");
	Request.SpellIndex = Index;
	if (!TestTrue(TEXT("Caller-selected local file loads into supplied text"), FFileHelper::LoadFileToString(Request.JsonText, *Path))) { return false; }
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(Request);
	for (const auto& Diagnostic : Result.Diagnostics)
	{
		// Report only controlled codes and locations, never retained source text.
		if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error) { AddError(Diagnostic.Code.ToString() + TEXT(" at ") + Diagnostic.JsonPointer); }
	}
	if (!TestTrue(TEXT("Selected external spell is accepted under the documented subset"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	TestFalse(TEXT("A normalized identity is available"), Result.Spell.Header.RecordId.SourceKey.IsEmpty());
	TestEqual(TEXT("Selected source pointer"), Result.Spell.Header.Provenance[0].JsonPointer, FString::Printf(TEXT("/spell/%d"), Index));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonMaterialsTest, "Dnd5e.Json.Reader.MaterialsAndExactNumbers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonMaterialsTest::RunTest(const FString& Parameters)
{
	for (bool bRequired : {false, true})
	{
		auto Spell = MakeSpell();
		Spell->GetObjectField(TEXT("components"))->SetBoolField(TEXT("m"), bRequired);
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		TestTrue(TEXT("Boolean material requirement is accepted"), Result.Status == EDnd5eJsonReadStatus::Success);
		TestEqual(TEXT("Requirement follows the Boolean"), Result.Spell.Components.Types.HasTagExact(Dnd5e::Component::Type::Material), bRequired);
		TestFalse(TEXT("A bare requirement invents no detail"), Result.Spell.Components.bHasMaterial);
	}
	struct FCost { const TCHAR* Token; EDnd5eNumberKind Kind; };
	const FCost Costs[] = {{TEXT("0"), EDnd5eNumberKind::Integer}, {TEXT("1.25"), EDnd5eNumberKind::Decimal}, {TEXT("2147483648"), EDnd5eNumberKind::Literal}, {TEXT("1e-999"), EDnd5eNumberKind::Literal}};
	for (const auto& Cost : Costs)
	{
		auto Spell = MakeSpell();
		Spell->GetObjectField(TEXT("components"))->SetField(TEXT("m"), ParseFixture(FString(TEXT("{\"text\":\"An invented token\",\"cost\":")) + Cost.Token + TEXT(",\"consume\":\"optional\"}")));
		Spell->GetObjectField(TEXT("components"))->SetBoolField(TEXT("r"), true);
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		if (!TestTrue(TEXT("Exact material cost accepted"), Result.Status == EDnd5eJsonReadStatus::Success)) { continue; }
		const auto& Material = Result.Spell.Components.Material;
		if (!TestTrue(TEXT("Explicit material cost presence"), Result.Spell.Components.bHasMaterial && Material.bHasCost && Material.Cost.Num() == 1)) { continue; }
		TestTrue(TEXT("Source costs are copper entries"), Material.Cost[0].Currency == EDnd5eCurrency::Copper);
		TestTrue(TEXT("The suitable numeric representation is selected"), Material.Cost[0].Amount.Kind == Cost.Kind);
		TestEqual(TEXT("Original cost spelling is retained"), Material.Cost[0].Amount.SourceLiteral, FString(Cost.Token));
		TestTrue(TEXT("Optional consumption and independent royalty"), Material.Consumption == EDnd5eMaterialConsumption::Optional && Result.Spell.Components.Types.HasTagExact(Dnd5e::Component::Type::Royalty));
	}
	for (const TCHAR* Consume : {TEXT("true"), TEXT("false")})
	{
		auto Spell = MakeSpell();
		Spell->GetObjectField(TEXT("components"))->SetField(TEXT("m"), ParseFixture(FString(TEXT("{\"text\":\"Invented material\",\"consume\":")) + Consume + TEXT("}")));
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		TestTrue(TEXT("Boolean consumption accepted"), Result.Status == EDnd5eJsonReadStatus::Success);
		TestTrue(TEXT("Consumption distinguishes explicit false from omission"), Result.Spell.Components.Material.Consumption == (FString(Consume).Equals(TEXT("true")) ? EDnd5eMaterialConsumption::Consumed : EDnd5eMaterialConsumption::NotConsumed));
	}
	auto Invalid = MakeSpell();
	Invalid->GetObjectField(TEXT("components"))->SetField(TEXT("m"), ParseFixture(TEXT("{\"text\":\"Invented\",\"cost\":-1}")));
	ExpectFailure(*this, MakeRequest(Invalid), TEXT("InvalidFieldValue"), TEXT("/spell/0/components/m/cost"));
	Invalid = MakeSpell();
	Invalid->SetField(TEXT("duration"), ParseFixture(TEXT("[{\"type\":\"timed\",\"duration\":{\"type\":\"minute\",\"amount\":0e999}}]")));
	ExpectFailure(*this, MakeRequest(Invalid), TEXT("InvalidFieldValue"), TEXT("/spell/0/duration/0/duration/amount"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonAreasTest, "Dnd5e.Json.Reader.AreasAndRemainingDurations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonAreasTest::RunTest(const FString& Parameters)
{
	struct FShape { const TCHAR* Name; FGameplayTag Tag; EDnd5eAreaDimensionKind Dimension; };
	const FShape Shapes[] = {
		{TEXT("cone"), Dnd5e::Area::Shape::Cone, EDnd5eAreaDimensionKind::Length}, {TEXT("line"), Dnd5e::Area::Shape::Line, EDnd5eAreaDimensionKind::Length},
		{TEXT("cube"), Dnd5e::Area::Shape::Cube, EDnd5eAreaDimensionKind::EdgeLength}, {TEXT("sphere"), Dnd5e::Area::Shape::Sphere, EDnd5eAreaDimensionKind::Radius},
		{TEXT("radius"), Dnd5e::Area::Shape::Radius, EDnd5eAreaDimensionKind::Radius}, {TEXT("hemisphere"), Dnd5e::Area::Shape::Hemisphere, EDnd5eAreaDimensionKind::Radius},
		{TEXT("emanation"), Dnd5e::Area::Shape::Emanation, EDnd5eAreaDimensionKind::Extent}, {TEXT("cylinder"), Dnd5e::Area::Shape::Cylinder, EDnd5eAreaDimensionKind::Radius}
	};
	for (const auto& Shape : Shapes)
	{
		auto Spell = MakeSpell();
		Spell->GetObjectField(TEXT("range"))->SetStringField(TEXT("type"), Shape.Name);
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
		if (!TestTrue(TEXT("Area source form accepted"), Result.Status == EDnd5eJsonReadStatus::Success) || !TestEqual(TEXT("One explicit area"), Result.Spell.Areas.Num(), 1)) { continue; }
		TestTrue(TEXT("Source area range is self-origin"), Result.Spell.Range.Kind == Dnd5e::Range::Kind::Self && Result.Spell.Areas[0].Origin == Dnd5e::Area::Origin::Self);
		TestTrue(TEXT("Shape and dimension roles survive"), Result.Spell.Areas[0].Geometry.Shape == Shape.Tag && Result.Spell.Areas[0].Geometry.Dimensions[0].Kind == Shape.Dimension);
		TestEqual(TEXT("No invented width/height"), Result.Spell.Areas[0].Geometry.Dimensions.Num(), 1);
	}
	auto Spell = MakeSpell();
	Spell->SetField(TEXT("range"), ParseFixture(TEXT("{\"type\":\"cylinder\",\"distance\":{\"type\":\"feet\",\"amount\":10,\"typeSecondary\":\"yards\",\"amountSecondary\":2.5}}")));
	Spell->SetField(TEXT("duration"), ParseFixture(TEXT("[{\"type\":\"permanent\",\"ends\":[\"dispel\",\"trigger\",\"discharge\"]},{\"type\":\"special\"}]")));
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	if (TestTrue(TEXT("Cylinder height and remaining durations accepted"), Result.Status == EDnd5eJsonReadStatus::Success))
	{
		TestEqual(TEXT("Independent cylinder height"), Result.Spell.Areas[0].Geometry.Dimensions.Num(), 2);
		TestTrue(TEXT("Height is not an invented width"), Result.Spell.Areas[0].Geometry.Dimensions[1].Kind == EDnd5eAreaDimensionKind::Height);
		TestEqual(TEXT("Decimal measurement spelling"), Result.Spell.Areas[0].Geometry.Dimensions[1].Measurement.Amount.SourceLiteral, FString(TEXT("2.5")));
		TestTrue(TEXT("Permanent and special are distinct"), Result.Spell.Durations[0].Duration.Kind == EDnd5eDurationKind::Permanent && Result.Spell.Durations[1].Duration.Kind == EDnd5eDurationKind::Special);
		TestTrue(TEXT("Ending conditions"), Result.Spell.Durations[0].Duration.EndingConditions.HasTagExact(Dnd5e::Duration::Ending::Discharged));
	}
	Spell->SetField(TEXT("range"), ParseFixture(TEXT("{\"type\":\"special\"}")));
	Spell->SetField(TEXT("time"), ParseFixture(TEXT("[{\"unit\":\"special\"}]")));
	const auto Special = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	TestTrue(TEXT("Special timing/range invents no quantity/distance"), Special.Status == EDnd5eJsonReadStatus::Success && Special.Spell.CastingTimes[0].Quantity == 0 && Special.Spell.Range.Kind == Dnd5e::Range::Kind::Special);
	Spell->SetField(TEXT("range"), ParseFixture(TEXT("{\"type\":\"cylinder\",\"distance\":{\"type\":\"feet\",\"amount\":10,\"typeSecondary\":\"feet\"}}")));
	ExpectFailure(*this, MakeRequest(Spell), TEXT("MissingRequiredField"), TEXT("/spell/0/range/distance/amountSecondary"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonScalingTest, "Dnd5e.Json.Reader.ClassificationsAndScaling", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonScalingTest::RunTest(const FString& Parameters)
{
	auto Spell = MakeSpell();
	const auto Extra = ParseFixture(TEXT(R"json({
		"damageInflict":["fire"],"damageResist":["cold"],"damageImmune":["poison"],"damageVulnerable":["thunder"],
		"conditionInflict":["prone"],"conditionImmune":["charmed"],"savingThrow":["dexterity"],"abilityCheck":["strength"],
		"spellAttack":["M","R"],"affectsCreatureType":["beast","fiend"],
		"scalingLevelDice":[{"label":"Invented {@b fire} scale","scaling":{"17":"4d7","5":"2d7","11":"3d7"}},{"label":"Other invented scale","scaling":{"1":"1d3"}}]
	})json"))->AsObject();
	for (const auto& Pair : Extra->Values) { Spell->SetField(Pair.Key, Pair.Value); }
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	if (!TestTrue(TEXT("Classification and scaling source fields accepted"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	const auto& Output = Result.Spell;
	TestTrue(TEXT("Damage roles are independent"), Output.DamageInflicted.HasTagExact(Dnd5e::Damage::Type::Fire) && Output.DamageResisted.HasTagExact(Dnd5e::Damage::Type::Cold) && !Output.DamageResisted.HasTagExact(Dnd5e::Damage::Type::Fire));
	TestTrue(TEXT("Condition roles remain independent"), Output.ConditionInflicted.HasTagExact(Dnd5e::Condition::Type::Prone) && Output.ConditionImmunities.HasTagExact(Dnd5e::Condition::Type::Charmed));
	TestTrue(TEXT("Ability roles"), Output.SavingThrowAbilities.HasTagExact(Dnd5e::Ability::Dexterity) && Output.AbilityCheckAbilities.HasTagExact(Dnd5e::Ability::Strength));
	TestTrue(TEXT("Attack modes and creature types"), Output.AttackTypes.Num() == 2 && Output.AffectedCreatureTypes.HasTagExact(Dnd5e::Creature::Type::Fiend));
	if (!TestEqual(TEXT("Scaling groups preserve source order"), Output.LevelScaling.Num(), 2)) { return false; }
	TestEqual(TEXT("Level keys sorted numerically"), Output.LevelScaling[0].Entries[0].Level, 5);
	TestEqual(TEXT("No invented level-one entry"), Output.LevelScaling[0].Entries.Num(), 3);
	TestEqual(TEXT("Expression remains unevaluated"), Output.LevelScaling[0].Entries[2].Dice.Expression, FString(TEXT("4d7")));
	FDnd5eSpell Restored;
	RoundTrip(*this, FDnd5eSpell::StaticStruct(), &Output, &Restored);
	Spell->SetField(TEXT("damageInflict"), ParseFixture(TEXT("[\"invented-type\"]")));
	ExpectFailure(*this, MakeRequest(Spell), TEXT("UnsupportedRequiredValue"), TEXT("/spell/0/damageInflict/0"));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonContentTreeTest, "Dnd5e.Json.Reader.StructuredContentAndInlineTokens", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonContentTreeTest::RunTest(const FString& Parameters)
{
	auto Spell = MakeSpell();
	Spell->SetField(TEXT("entries"), ParseFixture(TEXT(R"json([
		{"type":"entries","name":"An {@b invented} section","entries":[
			"A {@i small {@b nested}} effect rolls {@damage 2d7|an invented roll} and cites {@spell Other Spark|SYNTH|another spell}.",
			{"type":"list","items":["First invented entry",{"type":"item","name":"Named item","entries":["Named body"]}]},
			{"type":"table","caption":"Invented outcomes","colLabels":["{@dice 1d2}","Outcome"],"colStyles":["col-2 text-center","col-10"],"rows":[[{"type":"cell","roll":{"exact":1,"pad":true}},"A"],[2,{"type":"cell","width":1,"entry":"B"}]]}
		]},
		{"type":"quote","entries":["Invented quotation"],"by":"Invented author","skipMarks":true},
		{"type":"insetReadaloud","name":"Invented inset","source":"EXTRA","page":3,"entries":["Inset body"]},
		"Link {@link an invented label|https://example.invalid/} and unknown {@unmodeledAnnotation 15}."
	])json")));
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	if (!TestTrue(TEXT("Recursive Content accepted"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eSectionContent Section;
	if (!TestTrue(TEXT("Typed section"), UDnd5eContentLibrary::TryGetSection(Result.Spell.Description[0], Section))) { return false; }
	TestEqual(TEXT("Ordered section children"), Section.Blocks.Num(), 3);
	FDnd5eParagraphContent Paragraph;
	if (!UDnd5eContentLibrary::TryGetParagraph(Section.Blocks[0], Paragraph)) { AddError(TEXT("Missing paragraph")); return false; }
	bool bDice = false, bReference = false, bFormatted = false;
	for (const auto& Inline : Paragraph.Inlines)
	{
		if (Inline.Kind == Dnd5e::Content::Inline::Annotation) { FDnd5eAnnotationContent Annotation; UDnd5eContentLibrary::TryGetAnnotation(Inline, Annotation); bDice = Annotation.SourceAnnotation.Equals(TEXT("{@damage 2d7|an invented roll}")); }
		if (Inline.Kind == Dnd5e::Content::Inline::Reference) { FDnd5eReferenceContent Reference; UDnd5eContentLibrary::TryGetReference(Inline, Reference); bReference = Reference.bHasTargetRecordId && Reference.TargetRecordId.SourceKey.Equals(TEXT("5etools:spell:v1:Other%20Spark|SYNTH"), ESearchCase::CaseSensitive); }
		bFormatted |= Inline.Kind == Dnd5e::Content::Inline::Formatted;
	}
	TestTrue(TEXT("Typed formatting, dice annotation, and explicit-source reference"), bDice && bReference && bFormatted);
	FDnd5eListContent List;
	if (TestTrue(TEXT("Typed list"), UDnd5eContentLibrary::TryGetList(Section.Blocks[1], List))) { TestEqual(TEXT("List item count"), List.Items.Num(), 2); TestFalse(TEXT("Item label retained"), List.Items[1].Label.IsEmpty()); }
	FDnd5eTableContent Table;
	if (TestTrue(TEXT("Typed table"), UDnd5eContentLibrary::TryGetTable(Section.Blocks[2], Table)))
	{
		TestEqual(TEXT("Column count"), Table.Columns.Num(), 2);
		TestTrue(TEXT("Weight and alignment"), Table.Columns[0].bHasWidthWeight && Table.Columns[0].WidthWeight == 2.0f && Table.Columns[0].Alignment == EDnd5eContentAlignment::Center);
		TestTrue(TEXT("Roll metadata"), Table.Rows[0].Cells[0].bHasRollRange && Table.Rows[0].Cells[0].RollRange.Minimum == 1 && Table.Rows[0].Cells[0].RollRange.PadToDigits == 2);
		FDnd5eParagraphContent Cell;
		UDnd5eContentLibrary::TryGetParagraph(Table.Rows[1].Cells[0].Blocks[0], Cell);
		TestTrue(TEXT("Raw numeric cells stay numeric"), Cell.Inlines[0].Kind == Dnd5e::Content::Inline::Number);
	}
	FDnd5eQuoteContent Quote;
	TestTrue(TEXT("Quote flags"), UDnd5eContentLibrary::TryGetQuote(Result.Spell.Description[1], Quote) && !Quote.bShowQuotationMarks && Quote.bItalicize);
	FDnd5eInsetContent Inset;
	TestTrue(TEXT("Read-aloud classification"), UDnd5eContentLibrary::TryGetInset(Result.Spell.Description[2], Inset) && Inset.Presentation.HasTagExact(Dnd5e::Content::Presentation::ReadAloud));
	TestEqual(TEXT("Explicit source override propagates to child metadata"), Inset.Blocks[0].Metadata.SourceId, FString(TEXT("EXTRA")));
	TestTrue(TEXT("A semantic gap remains visible as a warning"), Result.Diagnostics.ContainsByPredicate([](const auto& Diagnostic) { return Diagnostic.Code == FName(TEXT("Dnd5e.Json.PreservedUnsupportedContent")); }));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonRollNormalizationTest, "Dnd5e.Json.Reader.RollCellNormalization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonRollNormalizationTest::RunTest(const FString& Parameters)
{
	auto Spell = MakeSpell();
	Spell->SetField(TEXT("entries"), ParseFixture(TEXT(R"json([{"type":"table","rows":[
		[{"type":"cell","roll":{"min":73,"max":0,"pad":true}},0],
		[{"type":"cell","roll":{"min":-4,"max":5}},0],
		[{"type":"cell","roll":{"min":-100000000000000000000,"max":20,"displayMin":-9}},0],
		[{"type":"cell","roll":{"exact":0,"pad":true}},0]
	]}])json")));
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	if (!TestTrue(TEXT("Source roll conventions normalize before validation"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eTableContent Table;
	if (!UDnd5eContentLibrary::TryGetTable(Result.Spell.Description[0], Table)) { AddError(TEXT("Expected a table")); return false; }
	const auto& Percentile = Table.Rows[0].Cells[0];
	TestTrue(TEXT("Matching bound is 100, display remains 00"), Percentile.RollRange.Maximum == 100 && Percentile.RollRange.bHasDisplayMaximum && Percentile.RollRange.DisplayMaximum == 0);
	FDnd5eParagraphContent Paragraph;
	FDnd5eTextContent Text;
	UDnd5eContentLibrary::TryGetParagraph(Percentile.Blocks[0], Paragraph);
	UDnd5eContentLibrary::TryGetInlineText(Paragraph.Inlines[0], Text);
	TestEqual(TEXT("Original padded label retained in Blocks"), Text.Text, FString(TEXT("73-00")));
	TestEqual(TEXT("A negative finite bound stays finite"), Table.Rows[1].Cells[0].RollRange.Minimum, -4);
	TestFalse(TEXT("Negative values are not automatically infinity"), Table.Rows[1].Cells[0].RollRange.bMinimumUnbounded);
	TestTrue(TEXT("Verified sentinel becomes an unbounded flag"), Table.Rows[2].Cells[0].RollRange.bMinimumUnbounded);
	TestEqual(TEXT("Explicit negative display override"), Table.Rows[2].Cells[0].RollRange.DisplayMinimum, -9);
	TestTrue(TEXT("Exact zero follows the roll-cell convention"), Table.Rows[3].Cells[0].RollRange.Minimum == 100 && Table.Rows[3].Cells[0].RollRange.Maximum == 100);
	UDnd5eContentLibrary::TryGetParagraph(Table.Rows[0].Cells[1].Blocks[0], Paragraph);
	FDnd5eNumberContent Number;
	UDnd5eContentLibrary::TryGetNumber(Paragraph.Inlines[0], Number);
	TestEqual(TEXT("An ordinary numeric zero is never changed to 100"), Number.Value.Integer, 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonFallbackLimitsTest, "Dnd5e.Json.Reader.JsonFallbackAndConversionLimits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eJsonFallbackLimitsTest::RunTest(const FString& Parameters)
{
	auto Spell = MakeSpell();
	Spell->SetField(TEXT("entries"), ParseFixture(TEXT("[{\"type\":\"future-rule\",\"source\":\"EXTRA\",\"page\":8,\"quantity\":123456789012345678901,\"nested\":{\"enabled\":true}}]")));
	const auto Result = FDnd5eJsonSpellReader::ReadSpell(MakeRequest(Spell));
	if (!TestTrue(TEXT("Unknown object is preserved"), Result.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eUnsupportedContent Unsupported;
	TestTrue(TEXT("JSON fallback is explicit"), UDnd5eContentLibrary::TryGetUnsupportedBlock(Result.Spell.Description[0], Unsupported) && Unsupported.FragmentKind == EDnd5eSourceFragmentKind::Json);
	TestEqual(TEXT("Fallback wrapper retains explicit source metadata"), Result.Spell.Description[0].Metadata.SourceId, FString(TEXT("EXTRA")));
	TestEqual(TEXT("Fallback wrapper retains explicit page metadata"), Result.Spell.Description[0].Metadata.Page.Number, 8);
	const auto Retained = ParseFixture(Unsupported.SourceFragment)->AsObject();
	TestTrue(TEXT("Large numeric token survives exactly as Number"), Retained->TryGetField(TEXT("quantity"))->Type == EJson::Number && Retained->TryGetField(TEXT("quantity"))->AsString().Equals(TEXT("123456789012345678901")));
	Spell->SetField(TEXT("entries"), ParseFixture(TEXT("[{\"type\":\"table\",\"rows\":[[{\"type\":\"cell\",\"entry\":\"Invented\",\"width\":4097}]]}]")));
	ExpectFailure(*this, MakeRequest(Spell), TEXT("ResourceLimitExceeded"), TEXT("/spell/0/entries/0"));
	FString Nested;
	for (int32 Index = 0; Index < 65; ++Index) { Nested += TEXT("{@b "); }
	Nested += TEXT("Invented");
	Nested += FString::ChrN(65, TEXT('}'));
	Spell->SetArrayField(TEXT("entries"), {MakeShared<FJsonValueString>(Nested)});
	ExpectFailure(*this, MakeRequest(Spell), TEXT("ResourceLimitExceeded"), TEXT("/spell/0/entries/0"));
	FString Many;
	for (int32 Index = 0; Index < 50001; ++Index) { Many += TEXT("{@b }"); }
	Spell->SetArrayField(TEXT("entries"), {MakeShared<FJsonValueString>(Many)});
	ExpectFailure(*this, MakeRequest(Spell), TEXT("ResourceLimitExceeded"), TEXT("/spell/0/entries/0"));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eJsonExternalDatasetTest, "Dnd5e.Json.ExternalDataset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eJsonExternalDatasetTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Directory;
	if (!FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory)) { return; }
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Directory, TEXT("spells-*.json")), true, false);
	Files.Sort();
	if (Files.IsEmpty()) { Names.Add(TEXT("NoFiles")); Commands.Add(TEXT("")); }
	for (const FString& File : Files) { Names.Add(FPaths::GetBaseFilename(File)); Commands.Add(FPaths::Combine(Directory, File)); }
}
bool FDnd5eJsonExternalDatasetTest::RunTest(const FString& Parameters)
{
	if (Parameters.IsEmpty()) { AddError(TEXT("The explicitly selected directory contains no spells-*.json files.")); return false; }
	FDnd5eJsonSpellReadRequest Request;
	Request.Context.DatasetNamespace = TEXT("external-audit");
	Request.Context.RelativeFilePath = FPaths::GetCleanFilename(Parameters);
	if (!TestTrue(TEXT("Load explicitly selected external file"), FFileHelper::LoadFileToString(Request.JsonText, *Parameters))) { return false; }
	TSharedPtr<FJsonObject> Root;
	if (!TestTrue(TEXT("External file has an object JSON root"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Request.JsonText), Root))) { return false; }
	const TArray<TSharedPtr<FJsonValue>>* SpellValues = nullptr;
	if (!TestTrue(TEXT("External document has a spell array"), Root->TryGetArrayField(TEXT("spell"), SpellValues) && SpellValues)) { return false; }
	const auto& Spells = *SpellValues;
	int32 Accepted = 0, Warnings = 0;
	for (int32 Index = 0; Index < Spells.Num(); ++Index)
	{
		Request.SpellIndex = Index;
		const auto Result = FDnd5eJsonSpellReader::ReadSpell(Request);
		if (Result.Status != EDnd5eJsonReadStatus::Success)
		{
			for (const auto& Diagnostic : Result.Diagnostics)
			{
				if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error) { AddError(Diagnostic.Code.ToString() + TEXT(" at ") + Diagnostic.JsonPointer); }
			}
		}
		else { ++Accepted; }
		for (const auto& Diagnostic : Result.Diagnostics) { if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning) { ++Warnings; } }
	}
	TestEqual(TEXT("Every explicitly supplied spell converts under the documented preservation policy"), Accepted, Spells.Num());
	AddInfo(FString::Printf(TEXT("Accepted %d records; %d returned warnings mark preservation/reference gaps. No source passages are logged."), Accepted, Warnings));
	return !HasAnyErrors();
}

#endif
