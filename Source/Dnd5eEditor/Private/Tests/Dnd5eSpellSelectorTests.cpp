// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"

using namespace Dnd5eTests;

namespace
{
	FDnd5eSpell SelectorRecord(const TCHAR* Key, const TCHAR* Name, const TCHAR* Source, const TCHAR* Namespace = TEXT("selector-synthetic"))
	{
		FDnd5eSpell Spell;
		Spell.Header.RecordId.DatasetNamespace = Namespace;
		Spell.Header.RecordId.Category = Dnd5e::Category::Spell;
		Spell.Header.RecordId.SourceKey = Key;
		Spell.Header.DisplayName = Name;
		Spell.Header.Aliases = {TEXT("Guiding Light")};
		if (Source && *Source) { FDnd5eRecordProvenance Provenance; Provenance.SourceId = Source; Spell.Header.Provenance.Add(Provenance); }
		Spell.Level = 0;
		return Spell;
	}

	TArray<FDnd5eSpell> SelectorRecords()
	{
		auto Contributed = SelectorRecord(TEXT("contributed"), TEXT("Invented Star"), TEXT("BASE"));
		FDnd5eRecordProvenance Extra; Extra.SourceId = TEXT("SYN-B"); Contributed.Header.Provenance.Add(Extra);
		return {
			SelectorRecord(TEXT("opaque/A"), TEXT("Invented Beacon"), TEXT("SYN-B")),
			SelectorRecord(TEXT("opaque/B"), TEXT("Invented Beacon"), TEXT("SYN-C")),
			SelectorRecord(TEXT("foreign"), TEXT("Invented Beacon"), TEXT("SYN-B"), TEXT("other-namespace")),
			SelectorRecord(TEXT("unsourced"), TEXT("Source-less Orb"), TEXT("")),
			Contributed
		};
	}

	struct FSelectorCase
	{
		FString Name;
		FDnd5eSpellSelector Selector;
		EDnd5eSpellSelectorMatchStatus Status;
		TArray<FString> ExpectedKeys;
		bool bEmptyCatalog = false;
	};

	TArray<FSelectorCase> SelectorCases()
	{
		FDnd5eSpellSelector Base;
		Base.DatasetNamespace = TEXT("selector-synthetic"); Base.Name = TEXT("Invented Beacon"); Base.SourceId = TEXT("SYN-B");
		TArray<FSelectorCase> Cases;
		auto Add = [&](const TCHAR* Name, const FDnd5eSpellSelector& Selector, EDnd5eSpellSelectorMatchStatus Status, TArray<FString> Keys = {}, bool bEmpty = false)
		{
			Cases.Add({Name, Selector, Status, MoveTemp(Keys), bEmpty});
		};
		Add(TEXT("Exact"), Base, EDnd5eSpellSelectorMatchStatus::Matched, {TEXT("opaque/A")});
		auto Value = Base; Value.Name = TEXT("INVENTED BEACON"); Value.SourceId = TEXT("syn-b");
		Add(TEXT("CaseFoldedNameAndSource"), Value, EDnd5eSpellSelectorMatchStatus::Matched, {TEXT("opaque/A")});
		Value = Base; Value.SourceId.Reset();
		Add(TEXT("MissingSourceMany"), Value, EDnd5eSpellSelectorMatchStatus::MissingSource, {TEXT("opaque/A"), TEXT("opaque/B")});
		Value.Name = TEXT("Source-less Orb");
		Add(TEXT("MissingSourceOne"), Value, EDnd5eSpellSelectorMatchStatus::MissingSource, {TEXT("unsourced")});
		Value.Name = TEXT("Not Loaded");
		Add(TEXT("MissingSourceZero"), Value, EDnd5eSpellSelectorMatchStatus::MissingSource);
		Value = Base; Value.Name = TEXT("Not Loaded");
		Add(TEXT("NoMatchName"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.SourceId = TEXT("not-loaded");
		Add(TEXT("NoMatchSource"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.DatasetNamespace = Value.DatasetNamespace.ToUpper();
		Add(TEXT("NamespaceCase"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.Name = TEXT("Beacon");
		Add(TEXT("NoSubstring"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.Name = TEXT("Guiding Light");
		Add(TEXT("NoAlias"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.Name += TEXT(" ");
		Add(TEXT("NoNameTrimming"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.SourceId = TEXT(" SYN-B");
		Add(TEXT("NoSourceTrimming"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.Name = TEXT("Source-less Orb");
		Add(TEXT("MissingProvenance"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		Value = Base; Value.Name = TEXT("Invented Star");
		Add(TEXT("ContributingSourceExcluded"), Value, EDnd5eSpellSelectorMatchStatus::NoMatch);
		for (const auto& Field : {FString(TEXT("Namespace")), FString(TEXT("Name")), FString(TEXT("Source"))})
		{
			for (const auto& TextCase : TArray<TPair<FString, FString>>{{TEXT("Empty"), TEXT("")}, {TEXT("Blank"), TEXT("   ")}, {TEXT("Control"), TEXT("text\ntext")}})
			{
				if (Field == TEXT("Source") && TextCase.Key == TEXT("Empty")) { continue; }
				Value = Base;
				if (Field == TEXT("Namespace")) { Value.DatasetNamespace = TextCase.Value; }
				else if (Field == TEXT("Name")) { Value.Name = TextCase.Value; }
				else { Value.SourceId = TextCase.Value; }
				Add(*(TEXT("Invalid") + Field + TextCase.Key), Value, EDnd5eSpellSelectorMatchStatus::InvalidSelector);
			}
		}
		Add(TEXT("EmptyCatalogExplicit"), Base, EDnd5eSpellSelectorMatchStatus::NoMatch, {}, true);
		Value = Base; Value.SourceId.Reset();
		Add(TEXT("EmptyCatalogMissingSource"), Value, EDnd5eSpellSelectorMatchStatus::MissingSource, {}, true);
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eSelectorMatchTest, "Dnd5e.Catalog.Selector.Matching", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eSelectorMatchTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : SelectorCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}

bool FDnd5eSelectorMatchTest::RunTest(const FString& Parameters)
{
	const auto Cases = SelectorCases();
	const auto* Case = Cases.FindByPredicate([&](const auto& Value) { return Value.Name == Parameters; });
	if (!TestNotNull(TEXT("Selector fixture exists"), Case)) { return false; }
	FDnd5eSpellCatalog Catalog;
	const auto Records = SelectorRecords();
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!Case->bEmptyCatalog && !TestTrue(TEXT("Install synthetic records"), Catalog.ReplaceSpells(Records, Diagnostics))) { return false; }
	const auto BeforeIds = Catalog.GetSpellIds();
	const auto BeforeSelector = Case->Selector;
	TArray<FDnd5eRecordId> Ids = {Records[2].Header.RecordId};
	Diagnostics.AddDefaulted(2);
	const auto Status = Catalog.MatchSpellSelector(Case->Selector, Ids, Diagnostics);
	TestEqual(TEXT("Explicit match status"), Status, Case->Status);
	TArray<FString> Keys;
	for (const auto& Id : Ids)
	{
		Keys.Add(Id.SourceKey);
		TestEqual(TEXT("Candidates remain in the requested namespace"), Id.DatasetNamespace, Case->Selector.DatasetNamespace);
		FDnd5eSpell Loaded;
		TestTrue(TEXT("Returned IDs are canonical exact lookup keys"), Catalog.TryGetSpell(Id, Loaded));
	}
	TestTrue(TEXT("Candidate IDs and order match the intended answer"), Keys == Case->ExpectedKeys);
	if (Status == EDnd5eSpellSelectorMatchStatus::Matched) { TestTrue(TEXT("Unique matches clear old diagnostics"), Diagnostics.IsEmpty() && Ids.Num() == 1); }
	else if (TestEqual(TEXT("Non-matches return one fresh diagnostic"), Diagnostics.Num(), 1))
	{
		const bool bInvalid = Case->Status == EDnd5eSpellSelectorMatchStatus::InvalidSelector;
		const FName Code = bInvalid ? TEXT("Dnd5e.Catalog.Selector.Invalid") : (Case->Status == EDnd5eSpellSelectorMatchStatus::MissingSource ? TEXT("Dnd5e.Catalog.Selector.MissingSource") : TEXT("Dnd5e.Catalog.Selector.NoMatch"));
		TestTrue(TEXT("Controlled code/severity and no invented source location"), Diagnostics[0].Code == Code && Diagnostics[0].Severity == (bInvalid ? EDnd5eDiagnosticSeverity::Error : EDnd5eDiagnosticSeverity::Warning) && !Diagnostics[0].Message.IsEmpty() && Diagnostics[0].RelativeFilePath.IsEmpty() && Diagnostics[0].JsonPointer.IsEmpty());
	}
	TestTrue(TEXT("Matching preserves the selector"), Equal(FDnd5eSpellSelector::StaticStruct(), &Case->Selector, &BeforeSelector));
	TestTrue(TEXT("Matching preserves catalog identities/order"), Catalog.GetSpellIds() == BeforeIds);
	if (!Case->bEmptyCatalog)
	{
		FDnd5eSpell Again;
		TestTrue(TEXT("Matching does not rewrite the stored record"), Catalog.TryGetSpell(Records[0].Header.RecordId, Again) && Equal(FDnd5eSpell::StaticStruct(), &Again, &Records[0]));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSelectorAmbiguityTest, "Dnd5e.Catalog.Selector.AmbiguityAndExistingContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eSelectorAmbiguityTest::RunTest(const FString& Parameters)
{
	const auto First = SelectorRecords()[0];
	auto Other = First;
	Other.Header.RecordId.SourceKey = TEXT("case-duplicate"); Other.Header.DisplayName = TEXT("INVENTED BEACON"); Other.Header.Provenance[0].SourceId = TEXT("syn-b");
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Distinct exact IDs can share a selector equivalence class"), Catalog.ReplaceSpells({Other, First}, Diagnostics))) { return false; }
	FDnd5eSpellSelector Selector;
	Selector.DatasetNamespace = First.Header.RecordId.DatasetNamespace; Selector.Name = TEXT("invented beacon"); Selector.SourceId = TEXT("syn-b");
	TArray<FDnd5eRecordId> Ids;
	TestEqual(TEXT("Explicit equivalent selector is ambiguous"), Catalog.MatchSpellSelector(Selector, Ids, Diagnostics), EDnd5eSpellSelectorMatchStatus::Ambiguous);
	TestTrue(TEXT("All candidates retain installation order and exact identities"), Ids == TArray<FDnd5eRecordId>{Other.Header.RecordId, First.Header.RecordId});
	TestTrue(TEXT("Ambiguity has a fixed warning code"), Diagnostics.Num() == 1 && Diagnostics[0].Code == FName(TEXT("Dnd5e.Catalog.Selector.Ambiguous")) && Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Warning);
	FDnd5eSpellQuery Query; Query.DatasetNamespace = Selector.DatasetNamespace; Query.Name = Selector.Name; Query.SourceId = Selector.SourceId;
	Catalog.QuerySpellIds(Query, Ids, Diagnostics);
	TestTrue(TEXT("The existing query source filter stays case-sensitive"), Ids == TArray<FDnd5eRecordId>{Other.Header.RecordId});
	FDnd5eSpell Copy;
	auto WrongCase = First.Header.RecordId; WrongCase.SourceKey = WrongCase.SourceKey.ToUpper();
	TestFalse(TEXT("Exact ID lookup remains case-sensitive"), Catalog.TryGetSpell(WrongCase, Copy));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSelectorReplacementTest, "Dnd5e.Catalog.Selector.OutputOwnershipAndReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eSelectorReplacementTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog;
	const auto First = SelectorRecords()[0];
	TArray<FDnd5eDiagnostic> Diagnostics;
	Catalog.ReplaceSpells({First}, Diagnostics);
	FDnd5eSpellSelector Selector; Selector.DatasetNamespace = First.Header.RecordId.DatasetNamespace; Selector.Name = First.Header.DisplayName; Selector.SourceId = TEXT("SYN-B");
	TArray<FDnd5eRecordId> Ids;
	if (!TestEqual(TEXT("Initial match"), Catalog.MatchSpellSelector(Selector, Ids, Diagnostics), EDnd5eSpellSelectorMatchStatus::Matched)) { return false; }
	Ids[0].DatasetNamespace = TEXT("edited-copy"); Diagnostics.AddDefaulted();
	Catalog.MatchSpellSelector(Selector, Ids, Diagnostics);
	TestTrue(TEXT("Returned arrays/ID strings are independent and old diagnostics clear"), Ids == TArray<FDnd5eRecordId>{First.Header.RecordId} && Diagnostics.IsEmpty());
	auto Replacement = First; Replacement.Header.RecordId.SourceKey = TEXT("replacement-key");
	Catalog.ReplaceSpells({Replacement}, Diagnostics);
	Catalog.MatchSpellSelector(Selector, Ids, Diagnostics);
	TestTrue(TEXT("A later call returns the replacement's canonical ID"), Ids == TArray<FDnd5eRecordId>{Replacement.Header.RecordId});
	Catalog.ReplaceSpells({}, Diagnostics);
	TestTrue(TEXT("Clearing removes previous candidates and produces NoMatch"), Catalog.MatchSpellSelector(Selector, Ids, Diagnostics) == EDnd5eSpellSelectorMatchStatus::NoMatch && Ids.IsEmpty());
	Selector.SourceId.Reset();
	TestTrue(TEXT("MissingSource remains distinct from an explicit no-match"), Catalog.MatchSpellSelector(Selector, Ids, Diagnostics) == EDnd5eSpellSelectorMatchStatus::MissingSource && Ids.IsEmpty());
	return !HasAnyErrors();
}

#endif
