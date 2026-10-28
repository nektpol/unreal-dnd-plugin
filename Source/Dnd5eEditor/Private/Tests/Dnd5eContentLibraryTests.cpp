// Developed by Neko Creative Technologies

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Content/Dnd5eContentType.h"
#include "Dnd5eTags.h"

namespace
{
	// Test-fixture construction only; no special-purpose runtime API is needed.
	FDnd5eInlineContent MakeTextInline(const TCHAR* Value)
	{
		FDnd5eTextContent Text;
		Text.Text = Value;

		FDnd5eInlineContent Inline;
		Inline.Kind = Dnd5e::Content::Inline::Text;
		Inline.Payload = FInstancedStruct::Make<FDnd5eTextContent>(Text);
		return Inline;
	}
}

// Registers this C++ test under the name shown in Unreal's Automation panel.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDnd5eSectionOutputResetTest,
	"Dnd5e.Content.Section.OutputReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eSectionOutputResetTest::RunTest(const FString& Parameters)
{
	// 1. Arrange: create a valid section with a heading and a child block.
	// The paragraph can be empty: this test checks extraction and output reset,
	// not the rendering of paragraph text.
	FDnd5eContentBlock ParagraphBlock;
	ParagraphBlock.Kind = Dnd5e::Content::Block::Paragraph;
	ParagraphBlock.Payload = FInstancedStruct::Make<FDnd5eParagraphContent>();

	FDnd5eSectionContent SourceSection;
	SourceSection.Heading.Add(MakeTextInline(TEXT("Output reset test")));
	SourceSection.Blocks.Add(ParagraphBlock);

	FDnd5eContentBlock ValidBlock;
	ValidBlock.Kind = Dnd5e::Content::Block::Section;
	ValidBlock.Payload = FInstancedStruct::Make<FDnd5eSectionContent>(SourceSection);

	// 2. Establish the precondition: extraction must first populate OutputSection.
	FDnd5eSectionContent OutputSection;
	const bool bValidSucceeded =
		UDnd5eContentLibrary::TryGetSection(ValidBlock, OutputSection);

	if (!TestTrue(TEXT("A valid section is extracted successfully"), bValidSucceeded))
	{
		return false;
	}

	if (!TestEqual(TEXT("The successful call populates one heading inline"),
		OutputSection.Heading.Num(), 1))
	{
		return false;
	}

	FDnd5eTextContent OutputHeading;
	if (!TestTrue(TEXT("The heading inline contains readable text"),
		UDnd5eContentLibrary::TryGetInlineText(OutputSection.Heading[0], OutputHeading)))
	{
		return false;
	}

	TestEqual(TEXT("The successful call populates the heading text"),
		OutputHeading.Text, FString(TEXT("Output reset test")));
	TestEqual(TEXT("The successful call populates one child block"),
		OutputSection.Blocks.Num(), 1);

	// 3. Act: supply a Section label with a Paragraph payload.
	FDnd5eContentBlock InvalidBlock;
	InvalidBlock.Kind = Dnd5e::Content::Block::Section;
	InvalidBlock.Payload = FInstancedStruct::Make<FDnd5eParagraphContent>();

	// Reuse the SAME output variable. Old data must not survive this failure.
	const bool bInvalidSucceeded =
		UDnd5eContentLibrary::TryGetSection(InvalidBlock, OutputSection);

	// 4. Assert: verify the contract, not just whether the call avoided a crash.
	TestFalse(TEXT("A section with a paragraph payload is rejected"), bInvalidSucceeded);
	TestTrue(TEXT("A failed extraction clears the previous heading"),
		OutputSection.Heading.IsEmpty());
	TestEqual(TEXT("A failed extraction clears the previous child blocks"),
		OutputSection.Blocks.Num(), 0);

	// Assertion failures are recorded by Unreal; report whether any occurred.
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDnd5eContentCopyIndependenceTest,
	"Dnd5e.Content.CopyIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eContentCopyIndependenceTest::RunTest(const FString& Parameters)
{
	// 1. Arrange: build a complete Section -> Paragraph -> Text tree.
	FDnd5eTextContent SourceText;
	SourceText.Text = TEXT("Original text");

	FDnd5eInlineContent TextInline;
	TextInline.Kind = Dnd5e::Content::Inline::Text;
	TextInline.Payload = FInstancedStruct::Make<FDnd5eTextContent>(SourceText);

	FDnd5eParagraphContent SourceParagraph;
	SourceParagraph.Inlines.Add(TextInline);

	FDnd5eContentBlock ParagraphBlock;
	ParagraphBlock.Kind = Dnd5e::Content::Block::Paragraph;
	ParagraphBlock.Payload = FInstancedStruct::Make<FDnd5eParagraphContent>(SourceParagraph);

	FDnd5eSectionContent SourceSection;
	SourceSection.Heading.Add(MakeTextInline(TEXT("Original heading")));
	SourceSection.Blocks.Add(ParagraphBlock);

	FDnd5eContentBlock Original;
	Original.Kind = Dnd5e::Content::Block::Section;
	Original.Payload = FInstancedStruct::Make<FDnd5eSectionContent>(SourceSection);

	// 2. This ordinary C++ assignment is the copy operation being tested.
	FDnd5eContentBlock Copy = Original;

	// Read the original through const pointers. Get mutable pointers only into
	// the actual copy: modifying TryGetSection's copy-out result would not test
	// whether the root copy owns independent nested payloads.
	const FDnd5eSectionContent* OriginalSection =
		Original.Payload.GetPtr<FDnd5eSectionContent>();
	FDnd5eSectionContent* CopiedSection =
		Copy.Payload.GetMutablePtr<FDnd5eSectionContent>();

	if (!TestNotNull(TEXT("The original contains a section"), OriginalSection)
		|| !TestNotNull(TEXT("The copy contains a section"), CopiedSection))
	{
		return false;
	}

	if (!TestEqual(TEXT("The original section has one heading inline"), OriginalSection->Heading.Num(), 1)
		|| !TestEqual(TEXT("The copied section has one heading inline"), CopiedSection->Heading.Num(), 1))
	{
		return false;
	}

	const FDnd5eTextContent* OriginalHeading =
		OriginalSection->Heading[0].Payload.GetPtr<FDnd5eTextContent>();
	FDnd5eTextContent* CopiedHeading =
		CopiedSection->Heading[0].Payload.GetMutablePtr<FDnd5eTextContent>();

	if (!TestNotNull(TEXT("The original heading contains text"), OriginalHeading)
		|| !TestNotNull(TEXT("The copied heading contains text"), CopiedHeading))
	{
		return false;
	}

	// Check counts before indexing so a failure is reported instead of crashing.
	if (!TestEqual(TEXT("The original section has one block"), OriginalSection->Blocks.Num(), 1)
		|| !TestEqual(TEXT("The copied section has one block"), CopiedSection->Blocks.Num(), 1))
	{
		return false;
	}

	const FDnd5eParagraphContent* OriginalParagraph =
		OriginalSection->Blocks[0].Payload.GetPtr<FDnd5eParagraphContent>();
	FDnd5eParagraphContent* CopiedParagraph =
		CopiedSection->Blocks[0].Payload.GetMutablePtr<FDnd5eParagraphContent>();

	if (!TestNotNull(TEXT("The original contains a paragraph"), OriginalParagraph)
		|| !TestNotNull(TEXT("The copy contains a paragraph"), CopiedParagraph))
	{
		return false;
	}

	if (!TestEqual(TEXT("The original paragraph has one inline"), OriginalParagraph->Inlines.Num(), 1)
		|| !TestEqual(TEXT("The copied paragraph has one inline"), CopiedParagraph->Inlines.Num(), 1))
	{
		return false;
	}

	const FDnd5eTextContent* OriginalText =
		OriginalParagraph->Inlines[0].Payload.GetPtr<FDnd5eTextContent>();
	FDnd5eTextContent* CopiedText =
		CopiedParagraph->Inlines[0].Payload.GetMutablePtr<FDnd5eTextContent>();

	if (!TestNotNull(TEXT("The original contains text"), OriginalText)
		|| !TestNotNull(TEXT("The copy contains text"), CopiedText))
	{
		return false;
	}

	TestEqual(TEXT("The copy initially preserves the heading"),
		CopiedHeading->Text, FString(TEXT("Original heading")));
	TestEqual(TEXT("The copy initially preserves the nested text"),
		CopiedText->Text, FString(TEXT("Original text")));

	// 3. Act: edit heading and body text stored IN THE COPY, not copy-out values.
	CopiedHeading->Text = TEXT("Changed heading");
	CopiedText->Text = TEXT("Changed text");

	// 4. Assert: the copy changed, but the original tree did not.
	TestEqual(TEXT("Editing the copy preserves the original heading"),
		OriginalHeading->Text, FString(TEXT("Original heading")));
	TestEqual(TEXT("Editing the copy preserves the original nested text"),
		OriginalText->Text, FString(TEXT("Original text")));
	TestEqual(TEXT("The copied heading contains the edit"),
		CopiedHeading->Text, FString(TEXT("Changed heading")));
	TestEqual(TEXT("The copied nested text contains the edit"),
		CopiedText->Text, FString(TEXT("Changed text")));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
