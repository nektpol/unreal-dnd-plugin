#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eCoverageTestSupport.h"
#include "UObject/StructOnScope.h"
#include <limits>
using namespace Dnd5eTests;

namespace
{
	void One(FDnd5eSpell& Spell, const FDnd5eInlineContent& Value) { FDnd5eParagraphContent Body; Body.Inlines.Add(Value); Spell.Description = {Dnd5eTests::Block(Dnd5e::Content::Block::Paragraph, Body)}; }
	template<typename T> FDnd5eInlineContent MakeAnnotation(FGameplayTag Kind, const T& Value) { FDnd5eAnnotationContent A; A.Kind = Kind; A.Value = FInstancedStruct::Make<T>(Value); return Dnd5eTests::Inline(Dnd5e::Content::Inline::Annotation, A); }
	struct FValidationCase { FString Name; TFunction<void(FDnd5eSpell&)> Setup; bool Valid = false; };
	TArray<FValidationCase> ValidationCases()
	{
		using namespace Dnd5e::Content;
		TArray<FValidationCase> Cases = {
			{TEXT("Empty"), [](auto&) {}, true},
			{TEXT("Text"), [](auto& S) { One(S, Text(TEXT("Invented"))); }, true},
			{TEXT("EmptyPayload"), [](auto& S) { S.Description.AddDefaulted(); }},
			{TEXT("WrongPayload"), [](auto& S) { S.Description = {Dnd5eTests::Block(Dnd5e::Content::Block::Paragraph, FDnd5eSectionContent{})}; }},
			{TEXT("WrongKind"), [](auto& S) { One(S, Dnd5eTests::Inline(Dnd5e::Category::Spell, FDnd5eTextContent{})); }},
			{TEXT("FormattedEvidence"), [](auto& S) { FDnd5eFormattedContent V; V.SourceFormat = TEXT("5etools"); One(S, Dnd5eTests::Inline(Inline::Formatted, V)); }},
			{TEXT("NestedLabel"), [](auto& S) { FDnd5eReferenceContent V; V.Display.Add(Text(TEXT("label"))); One(S, Dnd5eTests::Inline(Inline::Reference, V)); }, true},
			{TEXT("ReferenceSelector"), [](auto& S) { FDnd5eReferenceContent V; V.bHasTargetSelector = true; One(S, Dnd5eTests::Inline(Inline::Reference, V)); }},
			{TEXT("InactiveReference"), [](auto& S) { FDnd5eReferenceContent V; V.TargetSelector.Name = TEXT("stale"); V.TargetRecordId.SourceKey = TEXT("stale"); One(S, Dnd5eTests::Inline(Inline::Reference, V)); }, true},
			{TEXT("ReferenceId"), [](auto& S) { FDnd5eReferenceContent V; V.bHasTargetRecordId = true; One(S, Dnd5eTests::Inline(Inline::Reference, V)); }},
			{TEXT("ReferenceSourcePair"), [](auto& S) { FDnd5eReferenceContent V; V.SourceReference = TEXT("token"); One(S, Dnd5eTests::Inline(Inline::Reference, V)); }},
			{TEXT("OmittedSelectorSource"), [](auto& S) { FDnd5eReferenceContent V; V.bHasTargetSelector = true; V.TargetSelector.Category = Dnd5e::Category::Item; V.TargetSelector.Name = TEXT("Invented"); One(S, Dnd5eTests::Inline(Inline::Reference, V)); }, true},
			{TEXT("FeatureLevel"), [](auto& S) { FDnd5eReferenceContent V; V.bHasTargetSelector = true; V.TargetSelector.Category = Dnd5e::Category::ClassFeature; V.TargetSelector.Name = TEXT("Invented"); FDnd5eClassFeatureSelector Q; Q.ClassName = TEXT("Class"); Q.Level = 21; V.TargetSelector.Qualifier = FInstancedStruct::Make<FDnd5eClassFeatureSelector>(Q); One(S, Dnd5eTests::Inline(Inline::Reference, V)); }},
			{TEXT("SelectorUnicode"), [](auto& S) { FDnd5eReferenceContent V; V.bHasTargetSelector = true; V.TargetSelector.Category = Dnd5e::Category::Item; V.TargetSelector.Name = FString::ChrN(1, static_cast<TCHAR>(0xD800)); One(S, Dnd5eTests::Inline(Inline::Reference, V)); }},
			{TEXT("NumberUnspecified"), [](auto& S) { One(S, Dnd5eTests::Inline(Inline::Number, FDnd5eNumberContent{})); }},
			{TEXT("NumberInfinity"), [](auto& S) { FDnd5eNumberContent V; V.Value.Kind = EDnd5eNumberKind::Decimal; V.Value.Decimal = std::numeric_limits<double>::infinity(); One(S, Dnd5eTests::Inline(Inline::Number, V)); }},
			{TEXT("NumberLiteral"), [](auto& S) { FDnd5eNumberContent V; V.Value.Kind = EDnd5eNumberKind::Literal; V.Value.SourceLiteral = TEXT("PB + 1"); One(S, Dnd5eTests::Inline(Inline::Number, V)); }, true},
			{TEXT("ExternalScheme"), [](auto& S) { FDnd5eExternalLinkContent V; V.Url = TEXT("ftp://invented.invalid"); One(S, Dnd5eTests::Inline(Inline::ExternalLink, V)); }},
			{TEXT("ExternalEmptyHost"), [](auto& S) { FDnd5eExternalLinkContent V; V.Url = TEXT("https://"); One(S, Dnd5eTests::Inline(Inline::ExternalLink, V)); }},
			{TEXT("ExternalWhitespace"), [](auto& S) { FDnd5eExternalLinkContent V; V.Url = TEXT("https://invented.invalid/a b"); One(S, Dnd5eTests::Inline(Inline::ExternalLink, V)); }},
			{TEXT("ExternalValid"), [](auto& S) { FDnd5eExternalLinkContent V; V.Url = TEXT("https://invented.invalid/a"); One(S, Dnd5eTests::Inline(Inline::ExternalLink, V)); }, true},
			{TEXT("AnchorEmpty"), [](auto& S) { One(S, Dnd5eTests::Inline(Inline::AnchorLink, FDnd5eAnchorLinkContent{})); }},
			{TEXT("ListZeroStart"), [](auto& S) { FDnd5eListContent V; V.StartIndex = 0; S.Description = {Dnd5eTests::Block(Block::List, V)}; }, true},
			{TEXT("ListNegativeStart"), [](auto& S) { FDnd5eListContent V; V.StartIndex = -1; S.Description = {Dnd5eTests::Block(Block::List, V)}; }},
			{TEXT("ListColumnFlag"), [](auto& S) { FDnd5eListContent V; V.bHasColumnCount = true; V.ColumnCount = 0; S.Description = {Dnd5eTests::Block(Block::List, V)}; }},
			{TEXT("TableWeight"), [](auto& S) { FDnd5eTableContent V; V.Columns.AddDefaulted(); V.Columns[0].bHasWidthWeight = true; V.Columns[0].WidthWeight = 0; S.Description = {Dnd5eTests::Block(Block::Table, V)}; }},
			{TEXT("TableSpan"), [](auto& S) { FDnd5eTableContent V; V.Columns.AddDefaulted(); V.Rows.AddDefaulted(); V.Rows[0].Cells.AddDefaulted(); V.Rows[0].Cells[0].ColumnSpan = 2; S.Description = {Dnd5eTests::Block(Block::Table, V)}; }},
			{TEXT("TableRange"), [](auto& S) { FDnd5eTableContent V; V.Columns.AddDefaulted(); V.Rows.AddDefaulted(); V.Rows[0].Cells.AddDefaulted(); auto& Cell = V.Rows[0].Cells[0]; Cell.bHasRollRange = true; Cell.RollRange.Minimum = 5; Cell.RollRange.Maximum = 1; S.Description = {Dnd5eTests::Block(Block::Table, V)}; }},
			{TEXT("RaggedRows"), [](auto& S) { FDnd5eTableContent V; V.Columns.SetNum(2); V.Rows.AddDefaulted(); V.Rows[0].Cells.AddDefaulted(); S.Description = {Dnd5eTests::Block(Block::Table, V)}; }, true},
			{TEXT("ImageEmpty"), [](auto& S) { S.Description = {Dnd5eTests::Block(Block::Image, FDnd5eImageContent{})}; }},
			{TEXT("ImageTraversal"), [](auto& S) { FDnd5eImageContent V; V.Resource.Kind = EDnd5eMediaReferenceKind::DatasetRelative; V.Resource.RelativePath = TEXT("../outside"); S.Description = {Dnd5eTests::Block(Block::Image, V)}; }},
			{TEXT("ImageDimension"), [](auto& S) { FDnd5eImageContent V; V.Resource.Kind = EDnd5eMediaReferenceKind::DatasetRelative; V.Resource.RelativePath = TEXT("art/test.png"); V.bHasWidth = true; S.Description = {Dnd5eTests::Block(Block::Image, V)}; }},
			{TEXT("ImageInactiveFields"), [](auto& S) { FDnd5eImageContent V; V.Resource.Kind = EDnd5eMediaReferenceKind::DatasetRelative; V.Resource.RelativePath = TEXT("art/test.png"); V.Width = -1; S.Description = {Dnd5eTests::Block(Block::Image, V)}; }, true},
			{TEXT("GalleryKind"), [](auto& S) { FDnd5eGalleryContent V; V.Images.Add(Paragraph(TEXT("wrong"))); S.Description = {Dnd5eTests::Block(Block::Gallery, V)}; }},
			{TEXT("DocumentLocator"), [](auto& S) { FDnd5eDocumentLinkContent V; V.DocumentKind = Dnd5e::Document::Kind::Book; V.DocumentId = TEXT("SYN"); V.bHasChapter = true; V.ChapterIndex = -1; One(S, Dnd5eTests::Inline(Inline::DocumentLink, V)); }},
			{TEXT("QuickSource"), [](auto& S) { FDnd5eQuickReferenceContent V; V.Name = TEXT("Invented"); V.bHasSource = true; One(S, Dnd5eTests::Inline(Inline::QuickReference, V)); }},
			{TEXT("QueryEmpty"), [](auto& S) { FDnd5eQueryLinkContent V; V.TargetPage = TEXT("spells"); One(S, Dnd5eTests::Inline(Inline::QueryLink, V)); }},
			{TEXT("OpaqueMissing"), [](auto& S) { S.Description = {Dnd5eTests::Block(Block::Unsupported, FDnd5eUnsupportedContent{})}; }},
			{TEXT("OpaqueRetained"), [](auto& S) { FDnd5eUnsupportedContent V; V.SourceFormat = TEXT("synthetic"); V.FragmentKind = EDnd5eSourceFragmentKind::Text; V.SourceFragment = TEXT("invented evidence"); S.Description = {Dnd5eTests::Block(Block::Unsupported, V)}; }, true},
			{TEXT("HigherRoot"), [](auto& S) { S.HigherLevelDescription.AddDefaulted(); }},
			{TEXT("SupplementRoot"), [](auto& S) { S.bHasSupplementaryContent = true; S.SupplementaryContent.Entries.AddDefaulted(); }},
			{TEXT("SupplementImages"), [](auto& S) { S.bHasSupplementaryContent = true; S.SupplementaryContent.Images.Add(Paragraph(TEXT("wrong"))); }},
			{TEXT("InactiveSupplement"), [](auto& S) { S.SupplementaryContent.Images.AddDefaulted(); }, true},
			{TEXT("DepthLimit"), [](auto& S) { auto Child = Text(TEXT("leaf")); for (int32 I = 0; I < 65; ++I) { FDnd5eFormattedContent V; V.Inlines.Add(Child); Child = Dnd5eTests::Inline(Inline::Formatted, V); } One(S, Child); }},
			{TEXT("SizeLimit"), [](auto& S) { FDnd5eParagraphContent V; V.Inlines.Init(Text(TEXT("leaf")), 5000); S.Description = {Dnd5eTests::Block(Block::Paragraph, V)}; }}
		};
		for (const auto Kind : {Annotation::DiceRoll.GetTag(),Annotation::DamageRoll.GetTag(),Annotation::DifficultyClass.GetTag(),Annotation::AttackRoll.GetTag(),Annotation::D20Roll.GetTag(),Annotation::PercentChance.GetTag(),Annotation::ScaledDiceRoll.GetTag(),Annotation::ScaledDamageRoll.GetTag()})
		{
			Cases.Add({TEXT("WrongAnnotationType.") + Kind.ToString(), [Kind](auto& S) { One(S, MakeAnnotation(Kind, FDnd5eTextContent{})); }});
		}
		Cases.Add({TEXT("ChanceBounds"), [](auto& S) { FDnd5eChanceContentValue V; V.Percent = 101; One(S, MakeAnnotation(Annotation::PercentChance, V)); }});
		Cases.Add({TEXT("ChanceBranches"), [](auto& S) { FDnd5eChanceContentValue V; V.Percent = 50; V.Success.AddDefaulted(); One(S, MakeAnnotation(Annotation::PercentChance, V)); }});
		Cases.Add({TEXT("ChanceValid"), [](auto& S) { FDnd5eChanceContentValue V; V.Percent = 0; V.Failure.Add(Text(TEXT("failure"))); One(S, MakeAnnotation(Annotation::PercentChance, V)); }, true});
		Cases.Add({TEXT("D20Sides"), [](auto& S) { FDnd5eD20Roll V; V.DieSides = 12; V.Modifier.Kind = EDnd5eNumberKind::Integer; One(S, MakeAnnotation(Annotation::AttackRoll, V)); }});
		Cases.Add({TEXT("NegativeDc"), [](auto& S) { FDnd5eNumber V; V.Kind = EDnd5eNumberKind::Integer; V.Integer = -1; One(S, MakeAnnotation(Annotation::DifficultyClass, V)); }});
		Cases.Add({TEXT("SharedRootBudget"), [](auto& S) { FDnd5eParagraphContent V; V.Inlines.Init(Text(TEXT("leaf")), 2000); S.Description = {Dnd5eTests::Block(Block::Paragraph, V)}; S.HigherLevelDescription = S.Description; S.bHasSupplementaryContent = true; S.SupplementaryContent.Entries = S.Description; }});
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eScalingValidationTest, "Dnd5e.Content.Validation.ScalingValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eScalingValidationTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : {TEXT("ValidSlot"),TEXT("ValidPsi"),TEXT("SlotOverNine"),TEXT("Duplicate"),TEXT("Unsorted"),TEXT("Zero"),TEXT("NoInputs"),TEXT("NoBases"),TEXT("EmptyExpression"),TEXT("UnknownAxis")}) { Names.Add(Name); Commands.Add(Name); }
}
bool FDnd5eScalingValidationTest::RunTest(const FString& Parameters)
{
	FDnd5eScaledRoll V; FDnd5eDiceExpression Base; Base.Expression = TEXT("1d6"); Base.Syntax = TEXT("synthetic"); V.BaseExpressions.Add(Base); V.Increment = Base; V.InputValues = {1,2,3}; V.InputKind = Dnd5e::Scaling::Input::SpellSlotLevel;
	if (Parameters == TEXT("ValidPsi")) { V.InputKind = Dnd5e::Scaling::Input::PsiPoints; V.InputValues = {1,10,20}; }
	if (Parameters == TEXT("SlotOverNine")) { V.InputValues = {10}; }
	if (Parameters == TEXT("Duplicate")) { V.InputValues = {1,1}; }
	if (Parameters == TEXT("Unsorted")) { V.InputValues = {2,1}; }
	if (Parameters == TEXT("Zero")) { V.InputValues = {0}; }
	if (Parameters == TEXT("NoInputs")) { V.InputValues.Reset(); }
	if (Parameters == TEXT("NoBases")) { V.BaseExpressions.Reset(); }
	if (Parameters == TEXT("EmptyExpression")) { V.Increment.Expression.Reset(); }
	if (Parameters == TEXT("UnknownAxis")) { V.InputKind = Dnd5e::Category::Spell; }
	FDnd5eSpell Spell; One(Spell, MakeAnnotation(Dnd5e::Content::Annotation::ScaledDamageRoll, V));
	TestEqual(TEXT("Type/axis/active expression validation"), UDnd5eContentValidationLibrary::ValidateSpellContent(Spell).bValid, Parameters.StartsWith(TEXT("Valid")));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eContentValidationCasesTest, "Dnd5e.Content.Validation.Cases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eContentValidationCasesTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const { for (const auto& C : ValidationCases()) { Names.Add(C.Name); Commands.Add(C.Name); } }
bool FDnd5eContentValidationCasesTest::RunTest(const FString& Parameters)
{
	const auto Cases = ValidationCases(); const auto* C = Cases.FindByPredicate([&](const auto& V) { return V.Name == Parameters; }); if (!C) { return false; }
	FDnd5eSpell Spell; Spell.Header.RecordId = SyntheticSpell().Header.RecordId; C->Setup(Spell); const auto Before = Spell;
	const auto Result = UDnd5eContentValidationLibrary::ValidateSpellContent(Spell);
	TestEqual(TEXT("Canonical Content validity"), Result.bValid, C->Valid);
	TestTrue(TEXT("Validation is read-only"), Equal(FDnd5eSpell::StaticStruct(), &Spell, &Before));
	if (!C->Valid && TestTrue(TEXT("Controlled first error"), Result.Issues.Num() == 1))
	{
		const auto& Issue = Result.Issues[0]; TestTrue(TEXT("Model location and owner retained separately"), !Issue.ContentPath.IsEmpty() && Issue.OwnerRecordId == Spell.Header.RecordId && Issue.Diagnostic.JsonPointer.IsEmpty() && Issue.Diagnostic.RelativeFilePath.IsEmpty() && Issue.Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error);
	}
	if (Parameters == TEXT("OpaqueRetained")) { TestTrue(TEXT("Validity does not claim opaque semantics are understood"), Result.Issues.Num() == 1 && Result.Issues[0].Diagnostic.Severity == EDnd5eDiagnosticSeverity::Warning); }
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eValidationReflectionTest, "Dnd5e.Blueprint.ContentValidationInvocation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eValidationReflectionTest::RunTest(const FString& Parameters)
{
	UFunction* Function = UDnd5eContentValidationLibrary::StaticClass()->FindFunctionByName(TEXT("ValidateSpellContent")); if (!TestNotNull(TEXT("Reflected validator"), Function)) { return false; }
	TestTrue(TEXT("Callable static API"), Function->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Static));
	FStructOnScope Params(Function); auto* Input = FindFProperty<FStructProperty>(Function, TEXT("Spell")); auto* Output = FindFProperty<FStructProperty>(Function, TEXT("ReturnValue"));
	if (!TestTrue(TEXT("Exact reflected input and result"), Input && Output && Input->Struct == FDnd5eSpell::StaticStruct() && Output->Struct == FDnd5eContentValidationResult::StaticStruct())) { return false; }
	FDnd5eSpell Spell; Input->CopyCompleteValue(Input->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), &Spell);
	UDnd5eContentValidationLibrary::StaticClass()->GetDefaultObject()->ProcessEvent(Function, Params.GetStructMemory());
	TestTrue(TEXT("Blueprint dispatch executes validation"), Output->ContainerPtrToValuePtr<FDnd5eContentValidationResult>(Params.GetStructMemory())->bValid);
	return !HasAnyErrors();
}
#endif
