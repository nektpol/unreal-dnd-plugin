// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
#include "GameplayTagsManager.h"
using namespace Dnd5eTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemRarityRolesTest, "Dnd5e.Items.Foundation.RarityAndDefinitionRoles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemRarityRolesTest::RunTest(const FString& Parameters)
{
	const auto Root = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Dnd5e.Item.Rarity"), false);
	const FGameplayTag Rarities[] = {Dnd5e::Item::Rarity::None, Dnd5e::Item::Rarity::Common, Dnd5e::Item::Rarity::Uncommon, Dnd5e::Item::Rarity::Rare, Dnd5e::Item::Rarity::VeryRare, Dnd5e::Item::Rarity::Legendary, Dnd5e::Item::Rarity::Artifact, Dnd5e::Item::Rarity::Unknown, Dnd5e::Item::Rarity::UnknownMagic, Dnd5e::Item::Rarity::Varies};
	TSet<FGameplayTag> Seen;
	for (auto Tag : Rarities) { TestTrue(TEXT("Rarity is registered under its domain"), Tag.IsValid() && Tag.MatchesTag(Root)); Seen.Add(Tag); }
	TestEqual(TEXT("None, unknown, unknown magic and varies remain distinct"), Seen.Num(), 10);
	TestFalse(TEXT("Unknown magical rarity does not imply general unknown through hierarchy"), Dnd5e::Item::Rarity::UnknownMagic.GetTag().MatchesTag(Dnd5e::Item::Rarity::Unknown));
	TestFalse(TEXT("Type definitions are not item instances"), Dnd5e::Category::ItemType.GetTag().MatchesTag(Dnd5e::Category::Item));
	TestFalse(TEXT("Properties and types are distinct selector domains"), Dnd5e::Category::ItemProperty.GetTag() == Dnd5e::Category::ItemType.GetTag());
	FDnd5eItem Item;
	TestFalse(TEXT("Default authoring value invents no item identity"), Item.Header.RecordId.Category.IsValid());
	TestFalse(TEXT("Missing rarity is distinct from explicit None"), Item.Rarity.IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eItemContentOwnershipTest, "Dnd5e.Items.Foundation.NestedContentAndSelectors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eItemContentOwnershipTest::RunTest(const FString& Parameters)
{
	FDnd5eItem Item; Item.Header.DisplayName = TEXT("Invented Compass"); Item.Header.RecordId.Category = Dnd5e::Category::Item;
	Item.Rarity = Dnd5e::Item::Rarity::Common; Item.bHasType = true;
	Item.bHasBaseItemId = true;
	Item.BaseItemId.DatasetNamespace = TEXT("item-foundation");
	Item.BaseItemId.Category = Dnd5e::Category::Item;
	Item.BaseItemId.SourceKey = TEXT("dnd5e:item:v1:invented base|syn");
	Item.Type.Category = Dnd5e::Category::ItemType; Item.Type.Abbreviation = TEXT("SYN-TYPE");
	FDnd5eItemDefinitionSelector Property; Property.Category = Dnd5e::Category::ItemProperty; Property.Abbreviation = TEXT("SYN-PROP"); Property.bHasSource = true; Property.SourceId = TEXT("SYN"); Item.Properties.Add(Property);
	Item.Description.Add(Paragraph(TEXT("Original invented description")));
	Item.bHasSupplementaryContent = true; Item.SupplementaryContent.Entries.Add(Paragraph(TEXT("Original supplementary prose")));
	FDnd5eImageContent Image; Image.Resource.Kind = EDnd5eMediaReferenceKind::DatasetRelative; Image.Resource.RelativePath = TEXT("art/invented.png"); Image.Credit.Add(Text(TEXT("Original credit"))); Item.SupplementaryContent.Images.Add(Block(Dnd5e::Content::Block::Image, Image));
	const auto Before = Item; auto Copy = Item;
	Copy.BaseItemId.SourceKey = TEXT("changed");
	Copy.Type.Abbreviation = TEXT("Changed type"); Copy.Properties[0].SourceId = TEXT("CHANGED");
	Copy.Description[0].Payload.GetMutable<FDnd5eParagraphContent>().Inlines[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed description");
	Copy.SupplementaryContent.Images[0].Payload.GetMutable<FDnd5eImageContent>().Credit[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("Changed credit");
	TestTrue(TEXT("Nested item copies share no mutable storage"), Equal(FDnd5eItem::StaticStruct(), &Item, &Before));
	TestFalse(TEXT("Copy actually changed"), Equal(FDnd5eItem::StaticStruct(), &Item, &Copy));
	TestTrue(TEXT("Omitted type publication stays omitted"), !Item.Type.bHasSource && Item.Type.SourceId.IsEmpty());
	FDnd5eItem Restored; RoundTrip(*this, FDnd5eItem::StaticStruct(), &Item, &Restored);
	TestTrue(TEXT("Serialized Content and selector roles survive together"), Equal(FDnd5eItem::StaticStruct(), &Item, &Restored));
	const auto* BaseId = FindFProperty<FStructProperty>(FDnd5eItem::StaticStruct(), TEXT("BaseItemId"));
	const auto* Type = FindFProperty<FStructProperty>(FDnd5eItem::StaticStruct(), TEXT("Type"));
	const auto* Supplement = FindFProperty<FStructProperty>(FDnd5eItem::StaticStruct(), TEXT("SupplementaryContent"));
	TestTrue(TEXT("Editor exposes the base-item presence condition"), BaseId && BaseId->GetMetaData(TEXT("EditCondition")) == TEXT("bHasBaseItemId"));
	TestTrue(TEXT("Editor exposes the active type condition"), Type && Type->GetMetaData(TEXT("EditCondition")) == TEXT("bHasType"));
	TestTrue(TEXT("Editor exposes the supplementary presence condition"), Supplement && Supplement->GetMetaData(TEXT("EditCondition")) == TEXT("bHasSupplementaryContent"));
	return !HasAnyErrors();
}
#endif
