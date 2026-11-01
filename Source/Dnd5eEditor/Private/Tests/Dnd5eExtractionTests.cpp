// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "GameplayTagsManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

using namespace Dnd5eTests;

namespace
{
// Editor modules cannot declare native gameplay tags. Load a disposable INI
// source instead: no production declarations or persistent project config edits.
// The in-memory fixture nodes may live until this editor worker exits.
FGameplayTag ChildTag(FGameplayTag Parent)
{
    UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
    const FString Name = Parent.GetTagName().ToString() + TEXT(".AutomationChild");
    if (const FGameplayTag Existing = Manager.RequestGameplayTag(FName(*Name), false); Existing.IsValid()) { return Existing; }
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/TagFixtures"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const FString Filename = FPaths::Combine(Directory, TEXT("Dnd5eFixture_") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".ini"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Contents = FString::Printf(TEXT("[/Script/GameplayTags.GameplayTagsList]\nGameplayTagList=(Tag=\"%s\",DevComment=\"Automation fixture\")\n"), *Name);
    if (!FFileHelper::SaveStringToFile(Contents, *Filename)) { return FGameplayTag{}; }
    Manager.AddTagIniSearchPath(Directory);
    const FGameplayTag Result = Manager.RequestGameplayTag(FName(*Name), false);
    IFileManager::Get().Delete(*Filename);
    IFileManager::Get().DeleteDirectory(*Directory, false, false);
    return Result;
}

template<typename TWrapper, typename TPayload>
bool CheckExtractor(FAutomationTestBase& Test, FGameplayTag Kind, FGameplayTag Child,
    bool (*Extract)(const TWrapper&, TPayload&))
{
    UScriptStruct* PayloadType = TPayload::StaticStruct();
    const FInstancedStruct Sample = MakeStorageProbe(PayloadType);
    TWrapper Input = MakeStorageProbe(TWrapper::StaticStruct()).template Get<TWrapper>();
    Input.Kind = Kind;
    Input.Payload = Sample;
    const TWrapper Snapshot = Input;
    TPayload Output{};
    Test.TestTrue(TEXT("Correct kind and payload are accepted"), Extract(Input, Output));
    Test.TestTrue(TEXT("Extraction preserves every payload field"), Equal(PayloadType, &Output, Sample.GetMemory()));
    Test.TestTrue(TEXT("The returned copy can be edited"), MutateOneLeaf(PayloadType, &Output));
    Test.TestTrue(TEXT("Editing the returned payload preserves the actual source tree"),
        Equal(TWrapper::StaticStruct(), &Input, &Snapshot));

    // Seed the SAME output on every failure. Otherwise an already-empty output
    // would hide a helper that forgot to clear a previous successful value.
    auto Reject = [&](const TCHAR* Why, const TWrapper& Invalid)
    {
        Output = Sample.template Get<TPayload>();
        const TWrapper Before = Invalid;
        const TPayload Defaults{};
        Test.TestFalse(Why, Extract(Invalid, Output));
        Test.TestTrue(TEXT("Failure restores the complete default output, including nonzero defaults"),
            Equal(PayloadType, &Output, &Defaults));
        Test.TestTrue(TEXT("Failure leaves its input untouched"), Equal(TWrapper::StaticStruct(), &Invalid, &Before));
    };

    TWrapper Invalid = Input;
    Invalid.Payload.Reset();
    Reject(TEXT("Empty payload storage is rejected"), Invalid);
    Invalid = Input;
    Invalid.Kind = FGameplayTag{};
    Reject(TEXT("An unspecified kind is rejected"), Invalid);
    Invalid.Kind = Dnd5e::Category::Spell;
    Reject(TEXT("A different valid native kind is rejected"), Invalid);
    Invalid.Kind = UGameplayTagsManager::Get().RequestGameplayTagDirectParent(Kind);
    Test.TestTrue(TEXT("The parent-kind fixture is registered"), Invalid.Kind.IsValid());
    Reject(TEXT("A parent kind is not the exact discriminator"), Invalid);
    Invalid.Kind = Child;
    Test.TestTrue(TEXT("The child fixture really matches the parent hierarchically"), Child.MatchesTag(Kind));
    Reject(TEXT("A child kind does not inherit the payload contract"), Invalid);

    for (const FModelContract& Other : ModelContracts())
    {
        if (Other.Type() == PayloadType) { continue; }
        Invalid = Input;
        Invalid.Payload = MakeStorageProbe(Other.Type());
        const FString Why = FString::Printf(TEXT("Wrong payload %s is rejected"), Other.Name);
        Reject(*Why, Invalid);
    }

    // Extraction is a storage/type operation, not a semantic D&D validator.
    TWrapper EmptyValue = Input;
    EmptyValue.Payload = FInstancedStruct::Make<TPayload>();
    Output = Sample.template Get<TPayload>();
    const TPayload Defaults{};
    Test.TestTrue(TEXT("A correctly typed default payload is still extractable"), Extract(EmptyValue, Output));
    Test.TestTrue(TEXT("A default payload is returned without invented values"), Equal(PayloadType, &Output, &Defaults));
    Test.TestTrue(TEXT("The original complete wrapper remains unchanged"), Equal(TWrapper::StaticStruct(), &Input, &Snapshot));
    return !Test.HasAnyErrors();
}

struct FExtractorCase
{
    const TCHAR* Name;
    const TCHAR* Function;
    UScriptStruct* (*InputType)();
    UScriptStruct* (*OutputType)();
    bool (*Run)(FAutomationTestBase&);
};

const TArray<FExtractorCase>& Extractors()
{
    static const TArray<FExtractorCase> Cases =
    {
        { TEXT("DocumentLink"), TEXT("TryGetDocumentLink"), &FDnd5eInlineContent::StaticStruct, &FDnd5eDocumentLinkContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eDocumentLinkContent>(Test, Dnd5e::Content::Inline::DocumentLink, ChildTag(Dnd5e::Content::Inline::DocumentLink), &UDnd5eContentLibrary::TryGetDocumentLink); } },
        { TEXT("QuickReference"), TEXT("TryGetQuickReference"), &FDnd5eInlineContent::StaticStruct, &FDnd5eQuickReferenceContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eQuickReferenceContent>(Test, Dnd5e::Content::Inline::QuickReference, ChildTag(Dnd5e::Content::Inline::QuickReference), &UDnd5eContentLibrary::TryGetQuickReference); } },
        { TEXT("QueryLink"), TEXT("TryGetQueryLink"), &FDnd5eInlineContent::StaticStruct, &FDnd5eQueryLinkContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eQueryLinkContent>(Test, Dnd5e::Content::Inline::QueryLink, ChildTag(Dnd5e::Content::Inline::QueryLink), &UDnd5eContentLibrary::TryGetQueryLink); } },
        { TEXT("Section"), TEXT("TryGetSection"), &FDnd5eContentBlock::StaticStruct, &FDnd5eSectionContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eSectionContent>(Test, Dnd5e::Content::Block::Section, ChildTag(Dnd5e::Content::Block::Section), &UDnd5eContentLibrary::TryGetSection); } },
        { TEXT("Paragraph"), TEXT("TryGetParagraph"), &FDnd5eContentBlock::StaticStruct, &FDnd5eParagraphContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eParagraphContent>(Test, Dnd5e::Content::Block::Paragraph, ChildTag(Dnd5e::Content::Block::Paragraph), &UDnd5eContentLibrary::TryGetParagraph); } },
        { TEXT("Text"), TEXT("TryGetInlineText"), &FDnd5eInlineContent::StaticStruct, &FDnd5eTextContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eTextContent>(Test, Dnd5e::Content::Inline::Text, ChildTag(Dnd5e::Content::Inline::Text), &UDnd5eContentLibrary::TryGetInlineText); } },
        { TEXT("Formatted"), TEXT("TryGetFormatted"), &FDnd5eInlineContent::StaticStruct, &FDnd5eFormattedContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eFormattedContent>(Test, Dnd5e::Content::Inline::Formatted, ChildTag(Dnd5e::Content::Inline::Formatted), &UDnd5eContentLibrary::TryGetFormatted); } },
        { TEXT("List"), TEXT("TryGetList"), &FDnd5eContentBlock::StaticStruct, &FDnd5eListContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eListContent>(Test, Dnd5e::Content::Block::List, ChildTag(Dnd5e::Content::Block::List), &UDnd5eContentLibrary::TryGetList); } },
        { TEXT("Table"), TEXT("TryGetTable"), &FDnd5eContentBlock::StaticStruct, &FDnd5eTableContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eTableContent>(Test, Dnd5e::Content::Block::Table, ChildTag(Dnd5e::Content::Block::Table), &UDnd5eContentLibrary::TryGetTable); } },
        { TEXT("Quote"), TEXT("TryGetQuote"), &FDnd5eContentBlock::StaticStruct, &FDnd5eQuoteContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eQuoteContent>(Test, Dnd5e::Content::Block::Quote, ChildTag(Dnd5e::Content::Block::Quote), &UDnd5eContentLibrary::TryGetQuote); } },
        { TEXT("Inset"), TEXT("TryGetInset"), &FDnd5eContentBlock::StaticStruct, &FDnd5eInsetContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eInsetContent>(Test, Dnd5e::Content::Block::Inset, ChildTag(Dnd5e::Content::Block::Inset), &UDnd5eContentLibrary::TryGetInset); } },
        { TEXT("Reference"), TEXT("TryGetReference"), &FDnd5eInlineContent::StaticStruct, &FDnd5eReferenceContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eReferenceContent>(Test, Dnd5e::Content::Inline::Reference, ChildTag(Dnd5e::Content::Inline::Reference), &UDnd5eContentLibrary::TryGetReference); } },
        { TEXT("Annotation"), TEXT("TryGetAnnotation"), &FDnd5eInlineContent::StaticStruct, &FDnd5eAnnotationContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eAnnotationContent>(Test, Dnd5e::Content::Inline::Annotation, ChildTag(Dnd5e::Content::Inline::Annotation), &UDnd5eContentLibrary::TryGetAnnotation); } },
        { TEXT("Number"), TEXT("TryGetNumber"), &FDnd5eInlineContent::StaticStruct, &FDnd5eNumberContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eNumberContent>(Test, Dnd5e::Content::Inline::Number, ChildTag(Dnd5e::Content::Inline::Number), &UDnd5eContentLibrary::TryGetNumber); } },
        { TEXT("ExternalLink"), TEXT("TryGetExternalLink"), &FDnd5eInlineContent::StaticStruct, &FDnd5eExternalLinkContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eExternalLinkContent>(Test, Dnd5e::Content::Inline::ExternalLink, ChildTag(Dnd5e::Content::Inline::ExternalLink), &UDnd5eContentLibrary::TryGetExternalLink); } },
        { TEXT("AnchorLink"), TEXT("TryGetAnchorLink"), &FDnd5eInlineContent::StaticStruct, &FDnd5eAnchorLinkContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eAnchorLinkContent>(Test, Dnd5e::Content::Inline::AnchorLink, ChildTag(Dnd5e::Content::Inline::AnchorLink), &UDnd5eContentLibrary::TryGetAnchorLink); } },
        { TEXT("Image"), TEXT("TryGetImage"), &FDnd5eContentBlock::StaticStruct, &FDnd5eImageContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eImageContent>(Test, Dnd5e::Content::Block::Image, ChildTag(Dnd5e::Content::Block::Image), &UDnd5eContentLibrary::TryGetImage); } },
        { TEXT("Gallery"), TEXT("TryGetGallery"), &FDnd5eContentBlock::StaticStruct, &FDnd5eGalleryContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eGalleryContent>(Test, Dnd5e::Content::Block::Gallery, ChildTag(Dnd5e::Content::Block::Gallery), &UDnd5eContentLibrary::TryGetGallery); } },
        { TEXT("UnsupportedBlock"), TEXT("TryGetUnsupportedBlock"), &FDnd5eContentBlock::StaticStruct, &FDnd5eUnsupportedContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eContentBlock, FDnd5eUnsupportedContent>(Test, Dnd5e::Content::Block::Unsupported, ChildTag(Dnd5e::Content::Block::Unsupported), &UDnd5eContentLibrary::TryGetUnsupportedBlock); } },
        { TEXT("UnsupportedInline"), TEXT("TryGetUnsupportedInline"), &FDnd5eInlineContent::StaticStruct, &FDnd5eUnsupportedContent::StaticStruct,
          +[](FAutomationTestBase& Test) { return CheckExtractor<FDnd5eInlineContent, FDnd5eUnsupportedContent>(Test, Dnd5e::Content::Inline::Unsupported, ChildTag(Dnd5e::Content::Inline::Unsupported), &UDnd5eContentLibrary::TryGetUnsupportedInline); } },
    };
    return Cases;
}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eExtractionMatrixTest, "Dnd5e.Content.Extraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FDnd5eExtractionMatrixTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
    for (const FExtractorCase& Case : Extractors()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}

bool FDnd5eExtractionMatrixTest::RunTest(const FString& Parameters)
{
    const FExtractorCase* Case = Extractors().FindByPredicate([&](const FExtractorCase& Item) { return Parameters == Item.Name; });
    return TestNotNull(TEXT("The extraction scenario exists"), Case) && Case->Run(*this);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDiceExtractionTest, "Dnd5e.Content.Annotation.DiceExpression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eDiceExtractionTest::RunTest(const FString& Parameters)
{
    FDnd5eDiceExpression Expression;
    Expression.Expression = TEXT("2d6 + 3");
    Expression.Syntax = TEXT("5etools");
    FDnd5eAnnotationContent Input;
    Input.Value = FInstancedStruct::Make<FDnd5eDiceExpression>(Expression);
    Input.Display.Add(Text(TEXT("Synthetic damage display")));
    Input.SourceFormat = TEXT("5etools");
    Input.SourceAnnotation = TEXT("synthetic-dice-token");
    FDnd5eDiceExpression Output;
    for (FGameplayTag Kind : { Dnd5e::Content::Annotation::DiceRoll.GetTag(), Dnd5e::Content::Annotation::DamageRoll.GetTag() })
    {
        Input.Kind = Kind;
        const FDnd5eAnnotationContent Before = Input;
        TestTrue(TEXT("Both supported annotation kinds extract a dice expression"), UDnd5eContentLibrary::TryGetAnnotationDiceExpression(Input, Output));
        TestEqual(TEXT("The expression is preserved, not evaluated"), Output.Expression, Expression.Expression);
        TestEqual(TEXT("Expression syntax survives extraction"), Output.Syntax, Expression.Syntax);
        Output.Expression = TEXT("changed");
        TestTrue(TEXT("The stored expression and display remain unchanged"), Equal(FDnd5eAnnotationContent::StaticStruct(), &Input, &Before));
    }
    auto Reject = [&](const FDnd5eAnnotationContent& Invalid)
    {
        Output = Expression;
        const FDnd5eAnnotationContent Before = Invalid;
        TestFalse(TEXT("Invalid inner annotation extraction fails"), UDnd5eContentLibrary::TryGetAnnotationDiceExpression(Invalid, Output));
        TestTrue(TEXT("Failure clears both expression and syntax"), Output.Expression.IsEmpty() && Output.Syntax.IsEmpty());
        TestTrue(TEXT("Failure is read-only"), Equal(FDnd5eAnnotationContent::StaticStruct(), &Invalid, &Before));
    };
    FDnd5eAnnotationContent Invalid = Input;
    Invalid.Value.Reset(); Reject(Invalid);
    for (FGameplayTag Kind : { FGameplayTag{}, Dnd5e::Content::Inline::Annotation.GetTag(), ChildTag(Dnd5e::Content::Annotation::DiceRoll), ChildTag(Dnd5e::Content::Annotation::DamageRoll) })
    {
        Invalid = Input; Invalid.Kind = Kind; Reject(Invalid);
    }
    for (const FModelContract& Other : ModelContracts())
    {
        if (Other.Type() == FDnd5eDiceExpression::StaticStruct()) { continue; }
        Invalid = Input; Invalid.Value = MakeStorageProbe(Other.Type()); Reject(Invalid);
    }
    // An outer Annotation payload is not the inner DiceExpression value.
    const FDnd5eInlineContent Wrapper = Inline(Dnd5e::Content::Inline::Annotation, Input);
    FDnd5eAnnotationContent Extracted;
    TestTrue(TEXT("Outer annotation extraction remains a separate step"), UDnd5eContentLibrary::TryGetAnnotation(Wrapper, Extracted));
    TestTrue(TEXT("The extracted annotation then exposes its inner value"), UDnd5eContentLibrary::TryGetAnnotationDiceExpression(Extracted, Output));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eBlueprintApiTest, "Dnd5e.Blueprint.ExtractionAPI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eBlueprintApiTest::RunTest(const FString& Parameters)
{
    TArray<FExtractorCase> Cases = Extractors();
    Cases.Add({ TEXT("DiceExpression"), TEXT("TryGetAnnotationDiceExpression"), &FDnd5eAnnotationContent::StaticStruct, &FDnd5eDiceExpression::StaticStruct, nullptr });
    Cases.Add({ TEXT("DifficultyClass"), TEXT("TryGetAnnotationDifficultyClass"), &FDnd5eAnnotationContent::StaticStruct, &FDnd5eNumber::StaticStruct, nullptr });
    Cases.Add({ TEXT("D20Roll"), TEXT("TryGetAnnotationD20Roll"), &FDnd5eAnnotationContent::StaticStruct, &FDnd5eD20Roll::StaticStruct, nullptr });
    Cases.Add({ TEXT("PercentChance"), TEXT("TryGetAnnotationPercentChance"), &FDnd5eAnnotationContent::StaticStruct, &FDnd5eChanceContentValue::StaticStruct, nullptr });
    Cases.Add({ TEXT("ScaledRoll"), TEXT("TryGetAnnotationScaledRoll"), &FDnd5eAnnotationContent::StaticStruct, &FDnd5eScaledRoll::StaticStruct, nullptr });
    for (const FExtractorCase& Case : Cases)
    {
        UFunction* Function = UDnd5eContentLibrary::StaticClass()->FindFunctionByName(Case.Function);
        if (!TestNotNull(Case.Function, Function)) { continue; }
        const EFunctionFlags Required = FUNC_BlueprintCallable | FUNC_BlueprintPure | FUNC_Static;
        TestTrue(TEXT("The helper is static, callable, and pure in Blueprint"), (Function->FunctionFlags & Required) == Required);
        int32 Inputs = 0, Outputs = 0, Returns = 0;
        for (TFieldIterator<FProperty> It(Function); It; ++It)
        {
            if (!It->HasAnyPropertyFlags(CPF_Parm)) { continue; }
            if (It->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                ++Returns;
                TestNotNull(TEXT("Return value is a bool"), CastField<FBoolProperty>(*It));
                continue;
            }
            const FStructProperty* Struct = CastField<FStructProperty>(*It);
            if (!TestNotNull(TEXT("Payload parameters are reflected structs"), Struct)) { continue; }
            if (It->HasAnyPropertyFlags(CPF_OutParm) && !It->HasAnyPropertyFlags(CPF_ConstParm))
            {
                ++Outputs; TestTrue(TEXT("Output has the documented concrete type"), Struct->Struct == Case.OutputType());
            }
            else
            {
                ++Inputs; TestTrue(TEXT("Input has the documented type"), Struct->Struct == Case.InputType());
                TestTrue(TEXT("Input is const"), It->HasAnyPropertyFlags(CPF_ConstParm));
            }
        }
        TestEqual(TEXT("Exactly one input"), Inputs, 1);
        TestEqual(TEXT("Exactly one output"), Outputs, 1);
        TestEqual(TEXT("Exactly one success return"), Returns, 1);
    }
    return !HasAnyErrors();
}

#endif
