// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Spells/Dnd5eJsonSpellReader.h"

using namespace Dnd5eTests;

namespace
{
	FDnd5eReferenceContent ReferenceTo(const FDnd5eSpell& Spell)
	{
		FDnd5eReferenceContent Reference;
		Reference.bHasTargetRecordId = true;
		Reference.TargetRecordId = Spell.Header.RecordId;
		Reference.SourceFormat = TEXT("synthetic-format");
		Reference.SourceReference = TEXT("not-an-identity-parser-input");
		Reference.Display.Add(Text(TEXT("A label that differs from the target name")));
		return Reference;
	}

	struct FResolutionFailure
	{
		FString Name;
		FDnd5eReferenceContent Reference;
		EDnd5eReferenceResolveStatus Status;
		FName Code;
		EDnd5eDiagnosticSeverity Severity;
		bool bEmptyCatalog = false;
	};

	TArray<FResolutionFailure> FailureCases()
	{
		const auto Target = SyntheticSpell();
		const auto Base = ReferenceTo(Target);
		TArray<FResolutionFailure> Cases;
		auto Add = [&](const TCHAR* Name, const FDnd5eReferenceContent& Reference, EDnd5eReferenceResolveStatus Status, const TCHAR* Code, EDnd5eDiagnosticSeverity Severity, bool bEmpty = false)
		{
			Cases.Add({Name, Reference, Status, FName(Code), Severity, bEmpty});
		};
		auto Value = FDnd5eReferenceContent{};
		Add(TEXT("MissingTargetFlag"), Value, EDnd5eReferenceResolveStatus::MissingTargetId, TEXT("Dnd5e.Reference.MissingTargetId"), EDnd5eDiagnosticSeverity::Warning);
		Value = Base; Value.bHasTargetRecordId = false;
		Add(TEXT("InactiveValidId"), Value, EDnd5eReferenceResolveStatus::MissingTargetId, TEXT("Dnd5e.Reference.MissingTargetId"), EDnd5eDiagnosticSeverity::Warning);
		Value.TargetRecordId.Category = FGameplayTag{}; Value.TargetRecordId.SourceKey.Reset();
		Add(TEXT("InactiveMalformedId"), Value, EDnd5eReferenceResolveStatus::MissingTargetId, TEXT("Dnd5e.Reference.MissingTargetId"), EDnd5eDiagnosticSeverity::Warning);
		Value = Base; Value.TargetRecordId.Category = FGameplayTag{};
		Add(TEXT("InvalidCategory"), Value, EDnd5eReferenceResolveStatus::InvalidTargetId, TEXT("Dnd5e.Reference.InvalidTargetId"), EDnd5eDiagnosticSeverity::Error);
		for (const auto& Field : {FString(TEXT("Namespace")), FString(TEXT("SourceKey"))})
		{
			for (const auto& TextCase : TArray<TPair<FString, FString>>{{TEXT("Empty"), TEXT("")}, {TEXT("Blank"), TEXT("   ")}, {TEXT("Control"), TEXT("valid\ninvalid")}})
			{
				Value = Base;
				if (Field == TEXT("Namespace")) { Value.TargetRecordId.DatasetNamespace = TextCase.Value; }
				else { Value.TargetRecordId.SourceKey = TextCase.Value; }
				Add(*(TextCase.Key + Field), Value, EDnd5eReferenceResolveStatus::InvalidTargetId, TEXT("Dnd5e.Reference.InvalidTargetId"), EDnd5eDiagnosticSeverity::Error);
			}
		}
		Value = Base; Value.TargetRecordId.Category = Dnd5e::Category::Class;
		Add(TEXT("UnsupportedClass"), Value, EDnd5eReferenceResolveStatus::UnsupportedCategory, TEXT("Dnd5e.Reference.UnsupportedCategory"), EDnd5eDiagnosticSeverity::Warning);
		Value.TargetRecordId.DatasetNamespace.Reset();
		Add(TEXT("InvalidBeforeUnsupported"), Value, EDnd5eReferenceResolveStatus::InvalidTargetId, TEXT("Dnd5e.Reference.InvalidTargetId"), EDnd5eDiagnosticSeverity::Error);
		Value = Base; Value.TargetRecordId.Category = FGameplayTag::RequestGameplayTag(FName(TEXT("Dnd5e.Category")));
		Add(TEXT("UnsupportedParentCategory"), Value, EDnd5eReferenceResolveStatus::UnsupportedCategory, TEXT("Dnd5e.Reference.UnsupportedCategory"), EDnd5eDiagnosticSeverity::Warning);
		for (const auto& Change : {FString(TEXT("MissingKey")), FString(TEXT("NamespaceCase")), FString(TEXT("KeyCase")), FString(TEXT("NamespaceWhitespace")), FString(TEXT("KeyWhitespace")), FString(TEXT("NameIsNotKey")), FString(TEXT("AliasIsNotKey"))})
		{
			Value = Base;
			if (Change == TEXT("MissingKey")) { Value.TargetRecordId.SourceKey = TEXT("not-loaded"); }
			else if (Change == TEXT("NamespaceCase")) { Value.TargetRecordId.DatasetNamespace = Value.TargetRecordId.DatasetNamespace.ToUpper(); }
			else if (Change == TEXT("KeyCase")) { Value.TargetRecordId.SourceKey = Value.TargetRecordId.SourceKey.ToUpper(); }
			else if (Change == TEXT("NamespaceWhitespace")) { Value.TargetRecordId.DatasetNamespace += TEXT(" "); }
			else if (Change == TEXT("KeyWhitespace")) { Value.TargetRecordId.SourceKey = TEXT(" ") + Value.TargetRecordId.SourceKey; }
			else if (Change == TEXT("NameIsNotKey")) { Value.TargetRecordId.SourceKey = Target.Header.DisplayName; }
			else { Value.TargetRecordId.SourceKey = Target.Header.Aliases[0]; }
			Add(*Change, Value, EDnd5eReferenceResolveStatus::TargetNotFound, TEXT("Dnd5e.Reference.TargetNotFound"), EDnd5eDiagnosticSeverity::Warning);
		}
		Add(TEXT("EmptyCatalog"), Base, EDnd5eReferenceResolveStatus::TargetNotFound, TEXT("Dnd5e.Reference.TargetNotFound"), EDnd5eDiagnosticSeverity::Warning, true);
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eReferenceFailureTest, "Dnd5e.Catalog.Reference.Failures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eReferenceFailureTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : FailureCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}

bool FDnd5eReferenceFailureTest::RunTest(const FString& Parameters)
{
	const auto Cases = FailureCases();
	const auto* Case = Cases.FindByPredicate([&](const auto& Item) { return Item.Name == Parameters; });
	if (!TestNotNull(TEXT("Explicit failure fixture exists"), Case)) { return false; }
	FDnd5eSpellCatalog Catalog;
	const FDnd5eSpell Target = SyntheticSpell();
	FDnd5eSpell Output = Target;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!Case->bEmptyCatalog && !TestTrue(TEXT("Seed the catalog"), Catalog.ReplaceSpells({Target}, Diagnostics))) { return false; }
	const auto BeforeIds = Catalog.GetSpellIds();
	const auto BeforeReference = Case->Reference;
	Diagnostics.AddDefaulted(2);
	const auto Status = Catalog.ResolveSpellReference(Case->Reference, Output, Diagnostics);
	TestEqual(TEXT("The documented failure status is returned"), Status, Case->Status);
	const FDnd5eSpell Default;
	TestTrue(TEXT("Failure resets every spell field, including nested content"), Equal(FDnd5eSpell::StaticStruct(), &Output, &Default));
	if (TestEqual(TEXT("Failure replaces stale diagnostics with one result"), Diagnostics.Num(), 1))
	{
		TestEqual(TEXT("Controlled diagnostic code"), Diagnostics[0].Code, Case->Code);
		TestEqual(TEXT("Diagnostic severity"), Diagnostics[0].Severity, Case->Severity);
		TestTrue(TEXT("A useful message has no invented referring location"), !Diagnostics[0].Message.IsEmpty() && Diagnostics[0].RelativeFilePath.IsEmpty() && Diagnostics[0].JsonPointer.IsEmpty());
	}
	TestTrue(TEXT("The reference's raw/display/identity values remain unchanged"), Equal(FDnd5eReferenceContent::StaticStruct(), &Case->Reference, &BeforeReference));
	FDnd5eRecordRelationship Relationship;
	Relationship.Kind = Dnd5e::Record::Relationship::ReprintedAs;
	Relationship.bHasTargetRecord = Case->Reference.bHasTargetRecordId;
	Relationship.TargetRecordId = Case->Reference.TargetRecordId;
	Relationship.SourceFormat = TEXT("synthetic"); Relationship.SourceReference = TEXT("original relationship evidence");
	const auto BeforeRelationship = Relationship;
	Output = Target; Diagnostics.AddDefaulted(2);
	TestEqual(TEXT("Relationship lookup forwards the same failure status"), Catalog.ResolveSpellRelationship(Relationship, Output, Diagnostics), Case->Status);
	TestTrue(TEXT("Relationship failure also resets output"), Equal(FDnd5eSpell::StaticStruct(), &Output, &Default));
	if (TestEqual(TEXT("Relationship failure has exactly one diagnostic"), Diagnostics.Num(), 1))
	{
		TestTrue(TEXT("Diagnostic code/severity and empty referring location are forwarded"), Diagnostics[0].Code == Case->Code && Diagnostics[0].Severity == Case->Severity && Diagnostics[0].RelativeFilePath.IsEmpty() && Diagnostics[0].JsonPointer.IsEmpty());
	}
	TestTrue(TEXT("Relationship evidence is immutable"), Equal(FDnd5eRecordRelationship::StaticStruct(), &Relationship, &BeforeRelationship));
	TestTrue(TEXT("Failure preserves catalog identity/order"), Catalog.GetSpellIds() == BeforeIds);
	if (!Case->bEmptyCatalog)
	{
		FDnd5eSpell Retained;
		TestTrue(TEXT("Failure preserves the complete catalog record"), Catalog.TryGetSpell(Target.Header.RecordId, Retained) && Equal(FDnd5eSpell::StaticStruct(), &Retained, &Target));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eReferenceCopyTest, "Dnd5e.Catalog.Reference.ExactLookupAndCopy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eReferenceCopyTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog;
	const auto Target = SyntheticSpell();
	auto Other = Target; Other.Header.RecordId.DatasetNamespace = TEXT("other-namespace"); Other.Level = 9;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Equal display names in different namespaces remain separate"), Catalog.ReplaceSpells({Other, Target}, Diagnostics))) { return false; }
	const auto Reference = ReferenceTo(Target);
	const auto Before = Reference;
	FDnd5eSpell Output;
	Diagnostics.AddDefaulted();
	TestTrue(TEXT("Opaque ID resolves without parsing the token or choosing the first matching name"), Catalog.ResolveSpellReference(Reference, Output, Diagnostics) == EDnd5eReferenceResolveStatus::Resolved && Diagnostics.IsEmpty());
	TestTrue(TEXT("Every field is copied from the exact target"), Equal(FDnd5eSpell::StaticStruct(), &Output, &Target));
	auto* Section = Output.Description[0].Payload.GetMutablePtr<FDnd5eSectionContent>();
	if (!TestNotNull(TEXT("Nested instanced payload is present"), Section)) { return false; }
	Section->Heading.Reset();
	Output.Components.Material.Cost[0].Amount.Integer = -456;
	Output.Header.Provenance.Reset();
	FDnd5eSpell Again;
	Catalog.ResolveSpellReference(Reference, Again, Diagnostics);
	TestTrue(TEXT("Modifying deep output storage does not affect the next resolution"), Equal(FDnd5eSpell::StaticStruct(), &Again, &Target));
	TestTrue(TEXT("The referring payload is immutable"), Equal(FDnd5eReferenceContent::StaticStruct(), &Reference, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eReferenceReplacementTest, "Dnd5e.Catalog.Reference.ReplacementLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eReferenceReplacementTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog;
	const auto Target = SyntheticSpell();
	const auto Reference = ReferenceTo(Target);
	FDnd5eSpell Output;
	TArray<FDnd5eDiagnostic> Diagnostics;
	TestEqual(TEXT("A normalized ID can exist before its record is loaded"), Catalog.ResolveSpellReference(Reference, Output, Diagnostics), EDnd5eReferenceResolveStatus::TargetNotFound);
	if (!TestTrue(TEXT("Load the referenced target"), Catalog.ReplaceSpells({Target}, Diagnostics))) { return false; }
	TestTrue(TEXT("The unchanged reference now resolves and clears the old warning"), Catalog.ResolveSpellReference(Reference, Output, Diagnostics) == EDnd5eReferenceResolveStatus::Resolved && Diagnostics.IsEmpty());
	auto Invalid = Target; Invalid.Level = -5;
	TestFalse(TEXT("An invalid replacement preserves the target"), Catalog.ReplaceSpells({Invalid}, Diagnostics));
	TestEqual(TEXT("The retained catalog still resolves the reference"), Catalog.ResolveSpellReference(Reference, Output, Diagnostics), EDnd5eReferenceResolveStatus::Resolved);
	Catalog.ReplaceSpells({}, Diagnostics);
	TestEqual(TEXT("Clearing the catalog makes the target absent again"), Catalog.ResolveSpellReference(Reference, Output, Diagnostics), EDnd5eReferenceResolveStatus::TargetNotFound);
	auto Other = Target; Other.Header.RecordId.DatasetNamespace = TEXT("different-dataset");
	Catalog.ReplaceSpells({Other}, Diagnostics);
	TestEqual(TEXT("An identical name/key in another namespace is not a substitute"), Catalog.ResolveSpellReference(Reference, Output, Diagnostics), EDnd5eReferenceResolveStatus::TargetNotFound);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eReferenceReaderTest, "Dnd5e.Catalog.Reference.ReaderToLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eReferenceReaderTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonSpellReadRequest Request;
	Request.Context.DatasetNamespace = TEXT("reference-synthetic");
	Request.Context.RelativeFilePath = TEXT("spells/original.json");
	Request.JsonText = TEXT(R"json({"spell":[
		{"name":"Invented Gate","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":["See {@spell Invented Beacon|SYN|the invented beacon} and {@spell Invented Beacon}."]},
		{"name":"Invented Beacon","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":["An original synthetic description."]}
	]})json");
	Request.SpellIndex = 0;
	const auto Source = FDnd5eJsonSpellReader::ReadSpell(Request);
	Request.SpellIndex = 1;
	const auto Target = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestTrue(TEXT("Convert both original records"), Source.Status == EDnd5eJsonReadStatus::Success && Target.Status == EDnd5eJsonReadStatus::Success)) { return false; }
	FDnd5eParagraphContent Body;
	if (!TestTrue(TEXT("Extract the referring paragraph"), Source.Spell.Description.Num() == 1 && UDnd5eContentLibrary::TryGetParagraph(Source.Spell.Description[0], Body))) { return false; }
	TArray<FDnd5eReferenceContent> References;
	for (const auto& InlineValue : Body.Inlines)
	{
		FDnd5eReferenceContent Reference;
		if (UDnd5eContentLibrary::TryGetReference(InlineValue, Reference)) { References.Add(MoveTemp(Reference)); }
	}
	if (!TestEqual(TEXT("Both source forms remain reference payloads"), References.Num(), 2)) { return false; }
	FDnd5eSpellCatalog Catalog;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Install the referring and target records"), Catalog.ReplaceSpells({Source.Spell, Target.Spell}, Diagnostics))) { return false; }
	FDnd5eSpell Output;
	TestTrue(TEXT("An explicit-source reader ID resolves to the loaded target"), Catalog.ResolveSpellReference(References[0], Output, Diagnostics) == EDnd5eReferenceResolveStatus::Resolved && Equal(FDnd5eSpell::StaticStruct(), &Output, &Target.Spell));
	TestEqual(TEXT("Even one available name match does not infer an omitted source"), Catalog.ResolveSpellReference(References[1], Output, Diagnostics), EDnd5eReferenceResolveStatus::MissingTargetId);
	TestEqual(TEXT("The original source token remains retained"), References[0].SourceReference, FString(TEXT("{@spell Invented Beacon|SYN|the invented beacon}")));
	return !HasAnyErrors();
}

#endif
