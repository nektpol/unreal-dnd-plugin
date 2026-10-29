// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"

using namespace Dnd5eTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonRequestContractTest, "Dnd5e.Json.Contract.RequestPreservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eJsonRequestContractTest::RunTest(const FString& Parameters)
{
	// This is supplied text, not a parsed/validated spell and not a file to open.
	FDnd5eJsonSpellReadRequest Original;
	Original.JsonText = TEXT("{\n  \"spell\": [{\"name\": \"Invented Ω\", \"source\": \"SYN\"}]\n}");
	Original.Context.DatasetNamespace = TEXT("Synthetic-Collection");
	Original.Context.RelativeFilePath = TEXT("synthetic/spells.json");
	Original.SpellIndex = 0;
	FDnd5eJsonSpellReadRequest Restored;
	if (!RoundTrip(*this, FDnd5eJsonSpellReadRequest::StaticStruct(), &Original, &Restored)) { return false; }
	TestEqual(TEXT("The text is preserved exactly, including whitespace and Unicode"), Restored.JsonText, Original.JsonText);
	TestEqual(TEXT("Namespace spelling is not normalized by a value declaration"), Restored.Context.DatasetNamespace, Original.Context.DatasetNamespace);
	TestEqual(TEXT("The explicit zero selection is retained"), Restored.SpellIndex, 0);
	const FDnd5eJsonSpellReadRequest Default;
	TestEqual(TEXT("A default request does not select the first spell"), Default.SpellIndex, INDEX_NONE);
	FDnd5eJsonSpellReadRequest Copy = Original;
	Copy.JsonText = TEXT("changed");
	Copy.Context.DatasetNamespace = TEXT("Changed-Collection");
	Copy.Context.RelativeFilePath = TEXT("changed.json");
	Copy.SpellIndex = 7;
	TestTrue(TEXT("Editing a copied request preserves the original text and context"),
		Equal(FDnd5eJsonSpellReadRequest::StaticStruct(), &Original, &Restored));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eJsonResultContractTest, "Dnd5e.Json.Contract.ResultOwnershipAndReplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eJsonResultContractTest::RunTest(const FString& Parameters)
{
	// Manually authored outcomes test the DTO contract, not hypothetical parsing.
	FDnd5eJsonSpellReadResult Success;
	Success.Status = EDnd5eJsonReadStatus::Success;
	Success.Spell = SyntheticSpell();
	FDnd5eDiagnostic Note;
	Note.Severity = EDnd5eDiagnosticSeverity::Info;
	Note.Code = FName(TEXT("Dnd5e.Tests.SyntheticNote"));
	Note.Message = TEXT("An invented informational diagnostic");
	Note.RelativeFilePath = TEXT("synthetic/spells.json");
	Note.JsonPointer = TEXT("/spell/0");
	Success.Diagnostics.Add(Note);
	FDnd5eDiagnostic Warning = Note;
	Warning.Severity = EDnd5eDiagnosticSeverity::Warning;
	Warning.Code = FName(TEXT("Dnd5e.Tests.SyntheticWarning"));
	Warning.Message = TEXT("An invented accepted limitation");
	Warning.JsonPointer = TEXT("/spell/0/entries/0");
	Success.Diagnostics.Add(Warning);
	FDnd5eJsonSpellReadResult Restored;
	if (!RoundTrip(*this, FDnd5eJsonSpellReadResult::StaticStruct(), &Success, &Restored)) { return false; }
	if (!TestEqual(TEXT("Diagnostic order and count survive serialization"), Restored.Diagnostics.Num(), 2)) { return false; }
	TestEqual(TEXT("The controlled FName code survives serialization"), Restored.Diagnostics[0].Code.ToString(), FString(TEXT("Dnd5e.Tests.SyntheticNote")));
	TestTrue(TEXT("Different severities remain attached to their own entries"),
		Restored.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Info
		&& Restored.Diagnostics[1].Severity == EDnd5eDiagnosticSeverity::Warning);
	FDnd5eJsonSpellReadResult Copy = Success;
	Copy.Diagnostics[0].Code = FName(TEXT("Dnd5e.Tests.ChangedCode"));
	Copy.Diagnostics[0].Message = TEXT("changed");
	Copy.Diagnostics[0].JsonPointer = TEXT("/changed");
	Copy.Spell.Header.DisplayName = TEXT("Changed copy");
	TestTrue(TEXT("The output spell and diagnostic array have independent value ownership"),
		Equal(FDnd5eJsonSpellReadResult::StaticStruct(), &Success, &Restored));

	FDnd5eJsonSpellReadResult Failure;
	Failure.Status = EDnd5eJsonReadStatus::Failure;
	FDnd5eDiagnostic Error = Note;
	Error.Severity = EDnd5eDiagnosticSeverity::Error;
	Error.Code = FName(TEXT("Dnd5e.Json.InvalidFieldValue"));
	Error.Message = TEXT("Synthetic invalid field");
	Error.JsonPointer = TEXT("/spell/0/level");
	Failure.Diagnostics.Add(Error);
	// Reflected assignment is the route Blueprint/struct property copies use.
	FDnd5eJsonSpellReadResult Reused = Success;
	FDnd5eJsonSpellReadResult::StaticStruct()->CopyScriptStruct(&Reused, &Failure);
	const FDnd5eSpell DefaultSpell;
	TestTrue(TEXT("Replacing success with a failure value does not retain the old spell"),
		Equal(FDnd5eSpell::StaticStruct(), &Reused.Spell, &DefaultSpell));
	if (!TestEqual(TEXT("Old warnings/info do not survive result replacement"), Reused.Diagnostics.Num(), 1)) { return false; }
	TestTrue(TEXT("The replacement retains the actual failure status and error"),
		Reused.Status == EDnd5eJsonReadStatus::Failure && Reused.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Error);
	FDnd5eJsonSpellReadResult::StaticStruct()->CopyScriptStruct(&Reused, &Success);
	TestTrue(TEXT("Replacing failure with success restores the complete new value"),
		Equal(FDnd5eJsonSpellReadResult::StaticStruct(), &Reused, &Success));
	return !HasAnyErrors();
}

#endif
