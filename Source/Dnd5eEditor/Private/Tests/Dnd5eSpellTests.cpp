// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"

namespace Dnd5eTests
{
	FDnd5eSpell SyntheticSpell()
	{
		FDnd5eSpell Spell;
		Spell.Header.RecordId.DatasetNamespace = TEXT("synthetic-tests");
		Spell.Header.RecordId.Category = Dnd5e::Category::Spell;
		Spell.Header.RecordId.SourceKey = TEXT("opaque:prism-a");
		Spell.Header.DisplayName = TEXT("Synthetic Prism Ω");
		Spell.Header.Aliases = { TEXT("Invented alias one"), TEXT("Invented alias two") };
		FDnd5eRecordProvenance Primary;
		Primary.SourceId = TEXT("SYN-A");
		Primary.Page.Kind = EDnd5ePageReferenceKind::Number;
		Primary.Page.Number = 0;
		Primary.RelativeFilePath = TEXT("synthetic/spells.json");
		Primary.JsonPointer = TEXT("/spell/0");
		Primary.IdentityFieldsJson = TEXT("{\"syntheticKey\":\"prism-a\"}");
		Primary.ClassificationEvidenceJson = TEXT("{\"synthetic\":true}");
		Primary.AdditionalMetadataJson = TEXT("{\"note\":\"original fixture\"}");
		Spell.Header.Provenance.Add(Primary);
		FDnd5eRecordProvenance Contributor = Primary;
		Contributor.SourceId = TEXT("SYN-B");
		Contributor.Page.Kind = EDnd5ePageReferenceKind::Text;
		Contributor.Page.Text = TEXT("A-4");
		Contributor.Page.Number = 0;
		Spell.Header.Provenance.Add(Contributor);
		Spell.Level = 1;
		Spell.Schools.AddTag(Dnd5e::Magic::School::Evocation);
		Spell.Schools.AddTag(Dnd5e::Magic::School::Illusion);
		FDnd5eCastingTime Action;
		Action.Quantity = 1;
		Action.Unit = Dnd5e::Activation::Time::Action;
		Action.Note.Add(Paragraph(TEXT("Synthetic primary timing note")));
		Spell.CastingTimes.Add(Action);
		FDnd5eCastingTime Reaction;
		Reaction.Quantity = 1;
		Reaction.Unit = Dnd5e::Activation::Time::Reaction;
		Reaction.Condition.Add(Paragraph(TEXT("When the synthetic trigger occurs")));
		Spell.CastingTimes.Add(Reaction);
		Spell.Range.Kind = Dnd5e::Range::Kind::Self;
		FDnd5eSpellArea Area;
		Area.Origin = Dnd5e::Area::Origin::Self;
		Area.Geometry.Shape = Dnd5e::Area::Shape::Cone;
		FDnd5eAreaDimension Length;
		Length.Kind = EDnd5eAreaDimensionKind::Length;
		Length.Measurement.Amount.Kind = EDnd5eNumberKind::Integer;
		Length.Measurement.Amount.Integer = 30;
		Length.Measurement.Unit = EDnd5eDistanceUnit::Feet;
		Area.Geometry.Dimensions.Add(Length);
		Area.OriginDescription.Add(Paragraph(TEXT("Synthetic placement explanation")));
		Spell.Areas.Add(Area);
		Spell.Components.Types.AddTag(Dnd5e::Component::Type::Verbal);
		Spell.Components.Types.AddTag(Dnd5e::Component::Type::Somatic);
		Spell.Components.Types.AddTag(Dnd5e::Component::Type::Material);
		Spell.Components.bHasMaterial = true;
		Spell.Components.Material.Description.Add(Paragraph(TEXT("An invented multicolored token")));
		Spell.Components.Material.bHasCost = true;
		Spell.Components.Material.Consumption = EDnd5eMaterialConsumption::Optional;
		FDnd5eMoney Gold;
		Gold.Amount.Kind = EDnd5eNumberKind::Integer;
		Gold.Amount.Integer = 5;
		Gold.Currency = EDnd5eCurrency::Gold;
		Spell.Components.Material.Cost.Add(Gold);
		FDnd5eMoney Copper;
		Copper.Amount.Kind = EDnd5eNumberKind::Integer;
		Copper.Amount.Integer = 1;
		Copper.Currency = EDnd5eCurrency::Copper;
		Spell.Components.Material.Cost.Add(Copper);
		FDnd5eSpellDuration Timed;
		Timed.Duration.Kind = EDnd5eDurationKind::Timed;
		Timed.Duration.bHasTime = true;
		Timed.Duration.Time.Amount.Kind = EDnd5eNumberKind::Integer;
		Timed.Duration.Time.Amount.Integer = 1;
		Timed.Duration.Time.Unit = EDnd5eTimeUnit::Minute;
		Timed.Duration.bIsMaximum = true;
		Timed.Requirements.AddTag(Dnd5e::Trait::Concentration);
		Timed.Duration.EndingConditions.AddTag(Dnd5e::Duration::Ending::Dispelled);
		Spell.Durations.Add(Timed);
		FDnd5eSpellDuration Instant;
		Instant.Duration.Kind = EDnd5eDurationKind::Instantaneous;
		Spell.Durations.Add(Instant);
		Spell.Traits.AddTag(Dnd5e::Spell::Trait::Ritual);
		Spell.Traits.AddTag(Dnd5e::Trait::Concentration); // Explicitly authored summary for this fixture.
		Spell.DamageInflicted.AddTag(Dnd5e::Damage::Type::Fire);
		Spell.DamageResisted.AddTag(Dnd5e::Damage::Type::Fire);
		Spell.DamageImmunities.AddTag(Dnd5e::Damage::Type::Cold);
		Spell.DamageVulnerabilities.AddTag(Dnd5e::Damage::Type::Thunder);
		Spell.ConditionInflicted.AddTag(Dnd5e::Condition::Type::Charmed);
		Spell.ConditionImmunities.AddTag(Dnd5e::Condition::Type::Frightened);
		Spell.SavingThrowAbilities.AddTag(Dnd5e::Ability::Dexterity);
		Spell.AbilityCheckAbilities.AddTag(Dnd5e::Ability::Intelligence);
		Spell.AttackTypes.AddTag(Dnd5e::Attack::Mode::Melee);
		Spell.AffectedCreatureTypes.AddTag(Dnd5e::Creature::Type::Fiend);
		FDnd5eRecordRelationship Reprint;
		Reprint.Kind = Dnd5e::Record::Relationship::ReprintedAs;
		Reprint.bHasTargetRecord = true;
		Reprint.TargetRecordId.DatasetNamespace = TEXT("synthetic-tests");
		Reprint.TargetRecordId.Category = Dnd5e::Category::Spell;
		Reprint.TargetRecordId.SourceKey = TEXT("opaque:prism-b");
		Reprint.SourceFormat = TEXT("5etools");
		Reprint.SourceReference = TEXT("Invented Reprint|SYN-B");
		Spell.Relationships.Add(Reprint);
		FDnd5eRecordRelationship Unresolved;
		Unresolved.Kind = Dnd5e::Record::Relationship::ReprintedAs;
		Unresolved.SourceFormat = TEXT("5etools");
		Unresolved.SourceReference = TEXT("Invented Later Record|SYN-C");
		Spell.Relationships.Add(Unresolved);

		// A synthetic mixed document exercises ownership across the existing payload families.
		FDnd5eSectionContent Section;
		Section.Heading.Add(Text(TEXT("Synthetic section heading")));
		FDnd5eParagraphContent Body;
		Body.Inlines.Add(Text(TEXT("Original nested text")));
		FDnd5eFormattedContent Bold;
		Bold.Formatting.AddTag(Dnd5e::Content::Formatting::Bold);
		Bold.Inlines.Add(Text(TEXT("Emphasized synthetic text")));
		Body.Inlines.Add(Inline(Dnd5e::Content::Inline::Formatted, Bold));
		FDnd5eReferenceContent Reference;
		Reference.Display.Add(Text(TEXT("Synthetic reference label")));
		Reference.SourceFormat = TEXT("5etools");
		Reference.SourceReference = TEXT("synthetic-reference-token");
		Reference.bHasTargetRecordId = true;
		Reference.TargetRecordId = Spell.Header.RecordId;
		Body.Inlines.Add(Inline(Dnd5e::Content::Inline::Reference, Reference));
		FDnd5eDiceExpression Dice;
		Dice.Expression = TEXT("2d6 + 3"); Dice.Syntax = TEXT("5etools");
		FDnd5eAnnotationContent Annotation;
		Annotation.Kind = Dnd5e::Content::Annotation::DamageRoll;
		Annotation.Value = FInstancedStruct::Make<FDnd5eDiceExpression>(Dice);
		Annotation.Display.Add(Text(TEXT("Synthetic roll label")));
		Body.Inlines.Add(Inline(Dnd5e::Content::Inline::Annotation, Annotation));
		FDnd5eNumberContent Number;
		Number.Value.Kind = EDnd5eNumberKind::Decimal;
		Number.Value.Decimal = 0.123456789012345;
		Number.Value.SourceLiteral = TEXT("0.123456789012345");
		Body.Inlines.Add(Inline(Dnd5e::Content::Inline::Number, Number));
		FDnd5eExternalLinkContent External;
		External.Url = TEXT("https://example.invalid/synthetic");
		External.Display.Add(Text(TEXT("Unopened external reference")));
		Body.Inlines.Add(Inline(Dnd5e::Content::Inline::ExternalLink, External));
		FDnd5eAnchorLinkContent Anchor;
		Anchor.TargetAnchorId = TEXT("synthetic-anchor");
		Anchor.Display.Add(Text(TEXT("Synthetic anchor link")));
		Body.Inlines.Add(Inline(Dnd5e::Content::Inline::AnchorLink, Anchor));
		FDnd5eUnsupportedContent Unsupported;
		Unsupported.SourceFormat = TEXT("synthetic");
		Unsupported.FragmentKind = EDnd5eSourceFragmentKind::Text;
		Unsupported.SourceFragment = TEXT("synthetic unrecognized fragment Ω");
		Unsupported.DisplayText = TEXT("Preserved, not interpreted");
		Body.Inlines.Add(Inline(Dnd5e::Content::Inline::Unsupported, Unsupported));
		Section.Blocks.Add(Block(Dnd5e::Content::Block::Paragraph, Body));
		FDnd5eListContent List;
		List.MarkerStyle = EDnd5eListMarkerStyle::Decimal;
		List.StartIndex = 3;
		List.bHasColumnCount = true;
		List.ColumnCount = 2;
		FDnd5eListItemContent Item;
		Item.Label.Add(Text(TEXT("Synthetic item")));
		Item.Metadata.AnchorId = TEXT("synthetic-item");
		Item.Blocks.Add(Paragraph(TEXT("Synthetic list body")));
		List.Items.Add(Item);
		Section.Blocks.Add(Block(Dnd5e::Content::Block::List, List));
		FDnd5eTableContent Table;
		Table.Caption.Add(Text(TEXT("Synthetic table")));
		Table.bStripedRows = false;
		FDnd5eTableColumnContent Column;
		Column.Alignment = EDnd5eContentAlignment::Right;
		Column.bHasWidthWeight = true;
		Column.WidthWeight = 2.3f;
		Column.RollExpression = TEXT("1d10");
		Table.Columns.Add(Column);
		FDnd5eTableCellContent Cell;
		Cell.Blocks.Add(Paragraph(TEXT("Synthetic cell")));
		Cell.bHasRollRange = true;
		Cell.RollRange.Minimum = 1;
		Cell.RollRange.Maximum = 10;
		Cell.RollRange.bHasDisplayMinimum = true;
		Cell.RollRange.DisplayMinimum = 1;
		Cell.RollRange.PadToDigits = 2;
		FDnd5eTableRowContent Row;
		Row.Cells.Add(Cell);
		Table.HeaderRows.Add(Row);
		Table.Rows.Add(Row);
		Table.Footnotes.Add(Paragraph(TEXT("Synthetic footnote")));
		Section.Blocks.Add(Block(Dnd5e::Content::Block::Table, Table));
		FDnd5eQuoteContent Quote;
		Quote.Blocks.Add(Paragraph(TEXT("An entirely invented quotation")));
		Quote.Attribution.Add(Paragraph(TEXT("Synthetic author")));
		Quote.Source.Add(Text(TEXT("Synthetic work")));
		Quote.bItalicize = false;
		Section.Blocks.Add(Block(Dnd5e::Content::Block::Quote, Quote));
		FDnd5eInsetContent Inset;
		Inset.Heading.Add(Text(TEXT("Synthetic inset")));
		Inset.Blocks.Add(Paragraph(TEXT("Synthetic inset body")));
		Inset.Presentation.AddTag(Dnd5e::Content::Presentation::ReadAloud);
		Section.Blocks.Add(Block(Dnd5e::Content::Block::Inset, Inset));
		FDnd5eImageContent Image;
		Image.Resource.Kind = EDnd5eMediaReferenceKind::DatasetRelative;
		Image.Resource.RelativePath = TEXT("synthetic/unloaded-map.png");
		Image.bHasThumbnail = true;
		Image.Thumbnail.Kind = EDnd5eMediaReferenceKind::ExternalUrl;
		Image.Thumbnail.Url = TEXT("https://example.invalid/unloaded-thumbnail.png");
		Image.Title.Add(Text(TEXT("Synthetic map title")));
		Image.Credit.Add(Text(TEXT("Synthetic map credit")));
		Image.AltText = TEXT("Synthetic map description");
		Image.bHasWidth = true; Image.Width = 640;
		Image.bHasHeight = true; Image.Height = 480;
		Image.Classification.AddTag(Dnd5e::Content::ImageRole::PlayerMap);
		FDnd5eGalleryContent Gallery;
		Gallery.Heading.Add(Text(TEXT("Synthetic gallery")));
		FDnd5eContentBlock ImageBlock = Block(Dnd5e::Content::Block::Image, Image);
		ImageBlock.Metadata.AnchorId = TEXT("synthetic-image");
		Gallery.Images.Add(ImageBlock);
		Section.Blocks.Add(Block(Dnd5e::Content::Block::Gallery, Gallery));
		Unsupported.FragmentKind = EDnd5eSourceFragmentKind::Json;
		Unsupported.SourceFragment = TEXT("{\"type\":\"synthetic-extension\",\"value\":3}");
		Section.Blocks.Add(Block(Dnd5e::Content::Block::Unsupported, Unsupported));
		FDnd5eContentBlock Root = Block(Dnd5e::Content::Block::Section, Section);
		Root.Metadata.SourceType = TEXT("synthetic-section");
		Root.Metadata.AnchorId = TEXT("synthetic-anchor");
		Root.Metadata.RelativeFilePath = TEXT("synthetic/spells.json");
		Root.Metadata.JsonPointer = TEXT("/spell/0/entries/0");
		Root.Metadata.Aliases.Add(TEXT("synthetic-alias"));
		Spell.Description.Add(Root);
		Spell.HigherLevelDescription.Add(Paragraph(TEXT("Separate synthetic higher-level text")));
		return Spell;
	}
}

using namespace Dnd5eTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSpellRoundTripTest, "Dnd5e.Spell.ComposedRecordRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eSpellRoundTripTest::RunTest(const FString& Parameters)
{
	const FDnd5eSpell Original = SyntheticSpell();
	FDnd5eSpell Restored;
	if (!RoundTrip(*this, FDnd5eSpell::StaticStruct(), &Original, &Restored)) { return false; }
	TestEqual(TEXT("Both source-ordered casting alternatives survive"), Restored.CastingTimes.Num(), 2);
	TestEqual(TEXT("Both contextual durations survive"), Restored.Durations.Num(), 2);
	TestTrue(TEXT("Only the first duration requires concentration"),
		Restored.Durations[0].Requirements.HasTagExact(Dnd5e::Trait::Concentration)
		&& !Restored.Durations[1].Requirements.HasTagExact(Dnd5e::Trait::Concentration));
	TestTrue(TEXT("Self range does not invent a targeting distance from cone length"),
		Restored.Range.Kind == Dnd5e::Range::Kind::Self && !Restored.Range.bHasDistance);
	TestEqual(TEXT("Mixed denominations are not flattened"), Restored.Components.Material.Cost.Num(), 2);
	TestTrue(TEXT("Gold and copper retain their order and units"),
		Restored.Components.Material.Cost[0].Currency == EDnd5eCurrency::Gold
		&& Restored.Components.Material.Cost[1].Currency == EDnd5eCurrency::Copper);
	TestTrue(TEXT("One relationship has a normalized identity and one retains an unresolved source reference"),
		Restored.Relationships[0].bHasTargetRecord && !Restored.Relationships[1].bHasTargetRecord
		&& !Restored.Relationships[1].SourceReference.IsEmpty());
	TestEqual(TEXT("Supplementary provenance is not overwritten"), Restored.Header.Provenance.Num(), 2);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eSpellCopyTest, "Dnd5e.Spell.NestedCopyAndRoleIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eSpellCopyTest::RunTest(const FString& Parameters)
{
	const FDnd5eSpell Original = SyntheticSpell();
	FDnd5eSpell Copy = Original;
	auto* Section = Copy.Description[0].Payload.GetMutablePtr<FDnd5eSectionContent>();
	if (!TestNotNull(TEXT("The actual copied record owns a section"), Section)) { return false; }
	auto* Body = Section->Blocks[0].Payload.GetMutablePtr<FDnd5eParagraphContent>();
	if (!TestNotNull(TEXT("The actual copy contains a paragraph"), Body)) { return false; }
	auto* Leaf = Body->Inlines[0].Payload.GetMutablePtr<FDnd5eTextContent>();
	if (!TestNotNull(TEXT("The actual copy contains a text leaf"), Leaf)) { return false; }
	Leaf->Text = TEXT("Changed inside the copied spell");
	Copy.Header.Provenance[0].SourceId = TEXT("CHANGED");
	Copy.Components.Material.Cost[0].Amount.Integer = 99;
	Copy.Relationships[0].TargetRecordId.SourceKey = TEXT("changed-target");
	Copy.Durations[0].Requirements.Reset();
	Copy.DamageInflicted.Reset();
	FDnd5eSectionContent SourceSection;
	FDnd5eParagraphContent SourceBody;
	FDnd5eTextContent SourceLeaf;
	if (!UDnd5eContentLibrary::TryGetSection(Original.Description[0], SourceSection)
		|| !UDnd5eContentLibrary::TryGetParagraph(SourceSection.Blocks[0], SourceBody)
		|| !UDnd5eContentLibrary::TryGetInlineText(SourceBody.Inlines[0], SourceLeaf))
	{
		AddError(TEXT("The original nested fixture must remain extractable")); return false;
	}
	TestEqual(TEXT("Nested instanced payloads are independent"), SourceLeaf.Text, FString(TEXT("Original nested text")));
	TestEqual(TEXT("Provenance remains independent"), Original.Header.Provenance[0].SourceId, FString(TEXT("SYN-A")));
	TestEqual(TEXT("Money arrays remain independent"), Original.Components.Material.Cost[0].Amount.Integer, 5);
	TestEqual(TEXT("Relationship targets remain independent"), Original.Relationships[0].TargetRecordId.SourceKey, FString(TEXT("opaque:prism-b")));
	TestTrue(TEXT("Duration requirements are independently owned"), Original.Durations[0].Requirements.HasTagExact(Dnd5e::Trait::Concentration));
	TestTrue(TEXT("Removing inflicted fire does not remove resisted fire"), Copy.DamageResisted.HasTagExact(Dnd5e::Damage::Type::Fire));
	TestTrue(TEXT("The source keeps its inflicted fire classification"), Original.DamageInflicted.HasTagExact(Dnd5e::Damage::Type::Fire));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5ePresenceAndNumbersTest, "Dnd5e.Values.PresenceAndNumericBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5ePresenceAndNumbersTest::RunTest(const FString& Parameters)
{
	TArray<FDnd5eNumber> Numbers;
	Numbers.AddDefaulted();
	for (int32 Value : { MIN_int32, 0, MAX_int32 })
	{
		FDnd5eNumber Number; Number.Kind = EDnd5eNumberKind::Integer; Number.Integer = Value; Numbers.Add(Number);
	}
	FDnd5eNumber Decimal;
	Decimal.Kind = EDnd5eNumberKind::Decimal;
	Decimal.Decimal = 0.123456789012345;
	Decimal.SourceLiteral = TEXT("0.123456789012345"); Numbers.Add(Decimal);
	FDnd5eNumber Literal;
	Literal.Kind = EDnd5eNumberKind::Literal;
	Literal.SourceLiteral = TEXT("9007199254740993"); Numbers.Add(Literal);
	for (const FDnd5eNumber& Number : Numbers)
	{
		FDnd5eNumber Restored;
		RoundTrip(*this, FDnd5eNumber::StaticStruct(), &Number, &Restored);
		TestTrue(TEXT("Numeric kind and exact double/literal representation survive"), Equal(FDnd5eNumber::StaticStruct(), &Number, &Restored));
	}
	const FDnd5ePageReference Unspecified;
	FDnd5ePageReference Zero;
	Zero.Kind = EDnd5ePageReferenceKind::Number;
	FDnd5ePageReference TextZero;
	TextZero.Kind = EDnd5ePageReferenceKind::Text; TextZero.Text = TEXT("0");
	TestFalse(TEXT("An explicitly supplied zero page differs from unspecified"), Equal(FDnd5ePageReference::StaticStruct(), &Unspecified, &Zero));
	TestFalse(TEXT("Numeric and textual zero pages stay distinct"), Equal(FDnd5ePageReference::StaticStruct(), &Zero, &TextZero));
	FDnd5eMaterialComponent MissingCost;
	FDnd5eMaterialComponent ExplicitZero;
	ExplicitZero.bHasCost = true;
	FDnd5eMoney Free;
	Free.Amount.Kind = EDnd5eNumberKind::Integer; Free.Currency = EDnd5eCurrency::Gold;
	ExplicitZero.Cost.Add(Free);
	TestFalse(TEXT("Unspecified cost differs from an explicitly supplied zero"), Equal(FDnd5eMaterialComponent::StaticStruct(), &MissingCost, &ExplicitZero));
	FDnd5eMaterialComponent Restored;
	RoundTrip(*this, FDnd5eMaterialComponent::StaticStruct(), &ExplicitZero, &Restored);
	TestTrue(TEXT("Missing consumption is still unspecified, not NotConsumed"), Restored.Consumption == EDnd5eMaterialConsumption::Unspecified);
	return !HasAnyErrors();
}

#endif
