// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
using namespace Dnd5eTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eDefinitionOwnershipTest, "Dnd5e.Items.Definitions.IdentityAndContentOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eDefinitionOwnershipTest::RunTest(const FString& Parameters)
{
	FDnd5eItemTypeDefinition Type; Type.Abbreviation = TEXT("Z9"); Type.Header.DisplayName = TEXT("Invented Equipment Family"); Type.Header.RecordId.Category = Dnd5e::Category::ItemType; Type.Description.Add(Paragraph(TEXT("Original type paragraph")));
	FDnd5eRecordProvenance Origin; Origin.SourceId = TEXT("SYN-A"); Type.Header.Provenance.Add(Origin);
	FDnd5eRecordRelationship Reprint; Reprint.Kind = Dnd5e::Record::Relationship::ReprintedAs; Reprint.SourceFormat = TEXT("synthetic"); Reprint.SourceReference = TEXT("Z9|SYN-B"); Type.Relationships.Add(Reprint);
	FDnd5eItemPropertyDefinition Property; Property.Abbreviation = Type.Abbreviation; Property.Header.RecordId.Category = Dnd5e::Category::ItemProperty; Origin.SourceId = TEXT("SYN-B"); Property.Header.Provenance.Add(Origin); Property.Description.Add(Paragraph(TEXT("Original property paragraph"))); Property.Relationships.Add(Reprint);
	Property.bHasTemplate = true; Property.TemplateSyntax = TEXT("synthetic-item-property"); Property.TemplateText = TEXT("Invented label {{item.reload}}");
	FDnd5eItemMasteryDefinition Mastery; Mastery.Header.DisplayName = TEXT("Invented Deflection"); Mastery.Header.RecordId.Category = Dnd5e::Category::ItemMastery; Mastery.Header.Provenance.Add(Origin); Mastery.Description.Add(Paragraph(TEXT("Original mastery paragraph")));
	FDnd5eItemTypeDefinition TypeOut; FDnd5eItemPropertyDefinition PropertyOut; FDnd5eItemMasteryDefinition MasteryOut;
	if (!RoundTrip(*this, FDnd5eItemTypeDefinition::StaticStruct(), &Type, &TypeOut) || !RoundTrip(*this, FDnd5eItemPropertyDefinition::StaticStruct(), &Property, &PropertyOut) || !RoundTrip(*this, FDnd5eItemMasteryDefinition::StaticStruct(), &Mastery, &MasteryOut)) { return false; }
	if (!TestTrue(TEXT("Definition headers, Content and raw relationships round trip"), Equal(FDnd5eItemTypeDefinition::StaticStruct(), &Type, &TypeOut) && Equal(FDnd5eItemPropertyDefinition::StaticStruct(), &Property, &PropertyOut) && Equal(FDnd5eItemMasteryDefinition::StaticStruct(), &Mastery, &MasteryOut))) { return false; }
	TestTrue(TEXT("Same abbreviation does not merge definition domains or publications"), TypeOut.Abbreviation == PropertyOut.Abbreviation && TypeOut.Header.RecordId.Category != PropertyOut.Header.RecordId.Category && TypeOut.Header.Provenance[0].SourceId != PropertyOut.Header.Provenance[0].SourceId);
	TestTrue(TEXT("Missing property display name and unconstructed canonical keys remain absent"), PropertyOut.Header.DisplayName.IsEmpty() && TypeOut.Header.RecordId.SourceKey.IsEmpty() && PropertyOut.Header.RecordId.SourceKey.IsEmpty() && MasteryOut.Header.RecordId.SourceKey.IsEmpty());
	TestTrue(TEXT("Raw reprint stays unresolved rather than receiving a fabricated ID"), !TypeOut.Relationships[0].bHasTargetRecord && TypeOut.Relationships[0].SourceReference == TEXT("Z9|SYN-B") && TypeOut.Relationships[0].TargetRecordId.SourceKey.IsEmpty());
	const auto BeforeType = Type; const auto BeforeProperty = Property; const auto BeforeMastery = Mastery;
	TypeOut.Description[0].Payload.GetMutable<FDnd5eParagraphContent>().Inlines[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed type"); TypeOut.Relationships[0].SourceReference = TEXT("changed target");
	PropertyOut.Header.Provenance[0].SourceId = TEXT("CHANGED"); PropertyOut.TemplateText = TEXT("changed template"); PropertyOut.Description[0].Payload.GetMutable<FDnd5eParagraphContent>().Inlines[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed property");
	MasteryOut.Description[0].Payload.GetMutable<FDnd5eParagraphContent>().Inlines[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed mastery");
	TestTrue(TEXT("Copies own all nested definition storage"), Equal(FDnd5eItemTypeDefinition::StaticStruct(), &Type, &BeforeType) && Equal(FDnd5eItemPropertyDefinition::StaticStruct(), &Property, &BeforeProperty) && Equal(FDnd5eItemMasteryDefinition::StaticStruct(), &Mastery, &BeforeMastery));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5ePropertyTemplateTest, "Dnd5e.Items.Definitions.TemplatePresenceAndRetention", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5ePropertyTemplateTest::RunTest(const FString& Parameters)
{
	FDnd5eItemPropertyDefinition Missing, Empty, Literal, Inactive; Empty.bHasTemplate = true; Literal.bHasTemplate = true; Literal.TemplateSyntax = TEXT("synthetic-item-property"); Literal.TemplateText = TEXT("{{prop_name}} {{item.dmg2}} {@invented token}"); Inactive.TemplateSyntax = Literal.TemplateSyntax; Inactive.TemplateText = Literal.TemplateText;
	TestFalse(TEXT("Supplied empty template differs from absence"), Equal(FDnd5eItemPropertyDefinition::StaticStruct(), &Missing, &Empty));
	const FDnd5eItemPropertyDefinition Inputs[] = {Missing, Empty, Literal, Inactive};
	for (const auto& Input : Inputs)
	{
		FDnd5eItemPropertyDefinition Out; if (!RoundTrip(*this, FDnd5eItemPropertyDefinition::StaticStruct(), &Input, &Out)) { return false; }
		TestTrue(TEXT("Presence, grammar and exact unexpanded text are retained"), Equal(FDnd5eItemPropertyDefinition::StaticStruct(), &Input, &Out));
	}
	for (const auto* Name : {TEXT("TemplateSyntax"), TEXT("TemplateText")}) { const auto* Field = FDnd5eItemPropertyDefinition::StaticStruct()->FindPropertyByName(Name); if (TestNotNull(Name, Field)) { TestEqual(Name, Field->GetMetaData(TEXT("EditCondition")), FString(TEXT("bHasTemplate"))); } }
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eMasteryReferenceTest, "Dnd5e.Items.Definitions.MasteryReferenceOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eMasteryReferenceTest::RunTest(const FString& Parameters)
{
	FDnd5eItem Item; FDnd5eRecordProvenance Owner; Owner.SourceId = TEXT("OWNER"); Item.Header.Provenance.Add(Owner);
	FDnd5eItemMasteryReference Explicit; Explicit.Selector.Category = Dnd5e::Category::ItemMastery; Explicit.Selector.Name = TEXT("Invented Deflection"); Explicit.Selector.bHasSource = true; Explicit.Selector.SourceId = TEXT("SYN");
	FDnd5eFormattedContent Note; Note.Formatting.AddTag(Dnd5e::Content::Formatting::Italic); Note.Inlines.Add(Text(TEXT("Original independent note"))); Explicit.Note.Add(Inline(Dnd5e::Content::Inline::Formatted, Note));
	FDnd5eItemMasteryReference Omitted; Omitted.Selector.Category = Dnd5e::Category::ItemMastery; Omitted.Selector.Name = Explicit.Selector.Name; Item.Masteries = {Explicit, Omitted};
	const auto Before = Item; FDnd5eItem Out; if (!RoundTrip(*this, FDnd5eItem::StaticStruct(), &Item, &Out)) { return false; }
	if (!TestTrue(TEXT("Named selectors and independent notes round trip in order"), Equal(FDnd5eItem::StaticStruct(), &Item, &Out))) { return false; }
	TestTrue(TEXT("Omitted source does not borrow owner or neighboring reference publication"), !Out.Masteries[1].Selector.bHasSource && Out.Masteries[1].Selector.SourceId.IsEmpty() && Out.Masteries[1].Note.IsEmpty());
	Out.Masteries[0].Selector.SourceId = TEXT("CHANGED"); Out.Masteries[0].Note[0].Payload.GetMutable<FDnd5eFormattedContent>().Inlines[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed note"); Out.Masteries[1].Selector.Name = TEXT("Changed name");
	TestTrue(TEXT("Returned mastery references own their nested notes and selectors"), Equal(FDnd5eItem::StaticStruct(), &Item, &Before));
	TestFalse(TEXT("Mastery category is distinct from abbreviated property selection"), Dnd5e::Category::ItemMastery.GetTag() == Dnd5e::Category::ItemProperty.GetTag());
	return !HasAnyErrors();
}
#endif
