// Developed by Neko Creative Technologies

#include "Browser/Dnd5eItemBrowserDetails.h"
#include "Content/Dnd5eContentLibrary.h"
#include "Dnd5eTags.h"
#include "Catalog/Dnd5eItemDefinitionCatalogObject.h"
#include "Items/Dnd5eItemDefinitionIdentity.h"
#include "Browser/Dnd5eBrowserText.h"
#include "Items/Dnd5eItem.h"

#define LOCTEXT_NAMESPACE "Dnd5eItemBrowserDetails"

namespace
{
	using namespace Dnd5eBrowserText;

	FString FormatPrice(const FDnd5ePrice& Price)
	{
		if (Price.CostPresence == EDnd5eValuePresence::ExplicitNull)
		{
			return LOCTEXT("NullPrice", "No listed price").ToString();
		}
		TArray<FString> Parts;
		if (Price.CostPresence == EDnd5eValuePresence::Present)
		{
			for (const FDnd5eMoney& Money : Price.Cost)
			{
				if (Price.Cost.Num() == 1 && Money.Currency == EDnd5eCurrency::Copper &&
					Money.Amount.Kind == EDnd5eNumberKind::Integer && Money.Amount.Integer > 0 &&
					Money.Amount.Integer % 100 == 0)
				{
					Parts.Add(FText::AsNumber(Money.Amount.Integer / 100).ToString() + TEXT(" gp"));
					continue;
				}
				FString Amount = FormatNumber(Money.Amount);
				if (Amount.IsEmpty()) { continue; }
				const FString Unit = FormatCurrency(Money.Currency);
				if (!Unit.IsEmpty()) { Amount += TEXT(" ") + Unit; }
				Parts.Add(MoveTemp(Amount));
			}
		}
		if (Price.bHasMultiplier)
		{
			const FString Factor = FormatNumber(Price.Multiplier);
			if (!Factor.IsEmpty()) { Parts.Add(TEXT("×") + Factor); }
		}
		return FString::Join(Parts, TEXT(", "));
	}

	FString FormatWeight(const FDnd5eMass& Mass)
	{
		const FString Amount = FormatNumber(Mass.Amount);
		if (Amount.IsEmpty()) { return {}; }
		switch (Mass.Unit)
		{
		case EDnd5eMassUnit::Ounces: return Amount + TEXT(" oz");
		case EDnd5eMassUnit::Pounds: return Amount + TEXT(" lb");
		case EDnd5eMassUnit::Grams: return Amount + TEXT(" g");
		case EDnd5eMassUnit::Kilograms: return Amount + TEXT(" kg");
		default: return Amount;
		}
	}

	FString FormatCharges(const FDnd5eItemChargeAmount& Charges)
	{
		switch (Charges.Kind)
		{
		case EDnd5eItemChargeAmountKind::Fixed: return FText::AsNumber(Charges.Fixed).ToString();
		case EDnd5eItemChargeAmountKind::Dice:
			return Charges.Dice.Expression.IsEmpty() ? Charges.SourceLiteral : Charges.Dice.Expression;
		case EDnd5eItemChargeAmountKind::ExplicitNull:
			return LOCTEXT("NullCharges", "No listed charge capacity").ToString();
		default: return {};
		}
	}

	void AddFact(TArray<FString>& Facts, const FText& Pattern, const FString& Value)
	{
		if (!Value.IsEmpty()) { Facts.Add(FText::Format(Pattern, FText::FromString(Value)).ToString()); }
	}

	FText FormatClassification(const FDnd5eItem& Item)
	{
		if (Item.Header.RecordId.Category == Dnd5e::Category::ItemGroup.GetTag())
		{
			return LOCTEXT("GroupClassification", "Item group");
		}
		const FString Type = Item.bHasType ? Item.Type.Abbreviation : FString();
		if (Item.bHasWeaponData && (Type == TEXT("M") || Type == TEXT("R")))
		{
			const bool bMelee = Type == TEXT("M");
			switch (Item.WeaponData.Category)
			{
			case EDnd5eWeaponCategory::Martial:
				return bMelee ? LOCTEXT("MartialMelee", "Martial melee weapon") : LOCTEXT("MartialRanged", "Martial ranged weapon");
			case EDnd5eWeaponCategory::Simple:
				return bMelee ? LOCTEXT("SimpleMelee", "Simple melee weapon") : LOCTEXT("SimpleRanged", "Simple ranged weapon");
			default:
				return bMelee ? LOCTEXT("Melee", "Melee weapon") : LOCTEXT("Ranged", "Ranged weapon");
			}
		}
		if (Type == TEXT("LA")) { return LOCTEXT("LightArmor", "Light armor"); }
		if (Type == TEXT("MA")) { return LOCTEXT("MediumArmor", "Medium armor"); }
		if (Type == TEXT("HA")) { return LOCTEXT("HeavyArmor", "Heavy armor"); }
		if (Type == TEXT("S")) { return LOCTEXT("Shield", "Shield"); }
		if (Item.bHasWeaponData) { return LOCTEXT("Weapon", "Weapon"); }
		if (Item.bHasArmorData) { return LOCTEXT("Armor", "Armor"); }
		return FText::GetEmpty();
	}

	FString FormatDamage(const FDnd5eDamageExpression& Damage)
	{
		if (Damage.Amount.Expression.IsEmpty()) { return {}; }
		if (!Damage.Type.IsValid()) { return Damage.Amount.Expression; }

		FString TypeName = Damage.Type.ToString();
		int32 Separator;
		if (TypeName.FindLastChar(TEXT('.'), Separator)) { TypeName = TypeName.Mid(Separator + 1); }
		return Damage.Amount.Expression + TEXT(" ") + TypeName.ToLower();
	}

	bool HasVersatileProperty(const FDnd5eItem& Item)
	{
		return Item.Properties.ContainsByPredicate([](const FDnd5eItemDefinitionSelector& Property)
		{
			return Property.Abbreviation.Equals(TEXT("V"), ESearchCase::IgnoreCase) &&
				(!Property.bHasSource || Property.SourceId.Equals(TEXT("PHB"), ESearchCase::IgnoreCase) ||
					Property.SourceId.Equals(TEXT("XPHB"), ESearchCase::IgnoreCase));
		});
	}

	void AddWeaponFacts(TArray<FString>& Facts, const FDnd5eItem& Item)
	{
		if (!Item.bHasWeaponData) { return; }
		const auto& Weapon = Item.WeaponData;
		if (Weapon.bHasPrimaryDamage)
		{
			AddFact(Facts, LOCTEXT("PrimaryDamage", "{0}"), FormatDamage(Weapon.PrimaryDamage));
		}
		if (Weapon.bHasAlternateDamage)
		{
			const FString Alternate = FormatDamage(Weapon.AlternateDamage);
			if (!Alternate.IsEmpty())
			{
				const bool bVersatile = HasVersatileProperty(Item);
				const bool bSameType = Weapon.bHasPrimaryDamage && Weapon.PrimaryDamage.Type == Weapon.AlternateDamage.Type;
				AddFact(Facts, bVersatile ? LOCTEXT("Versatile", "Versatile ({0})") : LOCTEXT("AlternateDamage", "Alternate damage: {0}"),
					bVersatile && bSameType ? Weapon.AlternateDamage.Amount.Expression : Alternate);
			}
		}
		if (Weapon.bHasRange)
		{
			FString Range = FormatDistance(Weapon.Range.Normal);
			if (Weapon.Range.bHasLong)
			{
				const FString Long = FormatDistance(Weapon.Range.Long);
				if (!Long.IsEmpty()) { Range = Range.IsEmpty() ? Long : Range + TEXT(" / ") + Long; }
			}
			AddFact(Facts, LOCTEXT("WeaponRange", "Range: {0}"), Range);
		}
	}

	FString FormatArmorClass(const FDnd5eArmorClassDefinition& ArmorClass)
	{
		const FString Value = FormatNumber(ArmorClass.Value);
		if (Value.IsEmpty()) { return {}; }
		const FText DisplayValue = FText::FromString(Value);
		if (ArmorClass.Kind == EDnd5eArmorClassValueKind::Bonus)
		{
			return FText::Format(LOCTEXT("ACBonus", "{0} bonus"), DisplayValue).ToString();
		}
		switch (ArmorClass.DexterityMode)
		{
		case EDnd5eArmorClassDexterityMode::Uncapped:
			return FText::Format(LOCTEXT("ACDex", "{0} + Dex modifier"), DisplayValue).ToString();
		case EDnd5eArmorClassDexterityMode::Capped:
			return FText::Format(LOCTEXT("ACDexCapped", "{0} + Dex modifier (max {1})"), DisplayValue,
				FText::AsNumber(ArmorClass.DexterityModifierMaximum)).ToString();
		default:
			return Value;
		}
	}

	void AddArmorFacts(TArray<FString>& Facts, const FDnd5eItem& Item)
	{
		if (!Item.bHasArmorData) { return; }
		const auto& Armor = Item.ArmorData;
		if (Armor.bHasArmorClass)
		{
			AddFact(Facts, LOCTEXT("ArmorClass", "AC: {0}"), FormatArmorClass(Armor.ArmorClass));
		}
		if (Armor.StrengthPresence == EDnd5eValuePresence::Present)
		{
			AddFact(Facts, LOCTEXT("ArmorStrength", "Strength: {0}"), FText::AsNumber(Armor.StrengthMinimum.Minimum).ToString());
		}
		if (Armor.bHasStealthDisadvantage && Armor.bStealthDisadvantage)
		{
			Facts.Add(LOCTEXT("ArmorStealth", "Stealth disadvantage").ToString());
		}
	}

	bool TryGetPreviewDefinitionId(const FDnd5eItem& Item, const FDnd5eItemDefinitionSelector& Selector,
		FDnd5eRecordId& Id)
	{
		FString Source = Selector.SourceId;
		if (!Selector.bHasSource)
		{
			const bool bClassicPhb = !Item.Header.Provenance.IsEmpty() &&
				Item.Header.Provenance[0].SourceId.Equals(TEXT("PHB"), ESearchCase::IgnoreCase);
			const bool bArtisanTools = Selector.Category == Dnd5e::Category::ItemType.GetTag() &&
				Selector.Abbreviation.Equals(TEXT("AT"), ESearchCase::IgnoreCase);
			const bool bVersatile = Selector.Category == Dnd5e::Category::ItemProperty.GetTag() &&
				Selector.Abbreviation.Equals(TEXT("V"), ESearchCase::IgnoreCase);
			if (!bClassicPhb || (!bArtisanTools && !bVersatile)) { return false; }
			Source = TEXT("PHB");
		}
		return Dnd5e::ItemDefinitions::TryMakeId(Item.Header.RecordId.DatasetNamespace, Selector.Category,
			Selector.Abbreviation, Source, Id);
	}

	void AppendDescriptionSection(const TArray<FDnd5eContentBlock>& Blocks, FString& Out)
	{
		if (Blocks.IsEmpty() || Out.Len() >= MaxPreviewChars) { return; }
		if (!Out.IsEmpty()) { AppendPreview(Out, TEXT("\n")); }
		AppendBlocks(Blocks, Out, 0);
	}

	void AppendDefinitionDescriptions(const FDnd5eItem& Item, const UDnd5eItemDefinitionCatalogObject& Definitions,
		FString& Out)
	{
		FDnd5eRecordId Id;
		if (Item.bHasType && TryGetPreviewDefinitionId(Item, Item.Type, Id))
		{
			FDnd5eItemTypeDefinition Type;
			if (Definitions.TryGetType(Id, Type)) { AppendDescriptionSection(Type.Description, Out); }
		}

		TSet<FDnd5eRecordId> SeenProperties;
		for (const auto& Selector : Item.Properties)
		{
			if (Out.Len() >= MaxPreviewChars) { break; }
			if (!TryGetPreviewDefinitionId(Item, Selector, Id) || SeenProperties.Contains(Id)) { continue; }
			FDnd5eItemPropertyDefinition Property;
			if (Definitions.TryGetProperty(Id, Property))
			{
				SeenProperties.Add(Id);
				AppendDescriptionSection(Property.Description, Out);
			}
		}
	}
}

FText Dnd5eItemBrowserDetails::FormatRarity(const FGameplayTag& Rarity)
{
	using namespace Dnd5e::Item::Rarity;
	if (Rarity == None.GetTag()) { return LOCTEXT("None", "No rarity"); }
	if (Rarity == Common.GetTag()) { return LOCTEXT("Common", "Common"); }
	if (Rarity == Uncommon.GetTag()) { return LOCTEXT("Uncommon", "Uncommon"); }
	if (Rarity == Rare.GetTag()) { return LOCTEXT("Rare", "Rare"); }
	if (Rarity == VeryRare.GetTag()) { return LOCTEXT("VeryRare", "Very rare"); }
	if (Rarity == Legendary.GetTag()) { return LOCTEXT("Legendary", "Legendary"); }
	if (Rarity == Artifact.GetTag()) { return LOCTEXT("Artifact", "Artifact"); }
	if (Rarity == UnknownMagic.GetTag()) { return LOCTEXT("UnknownMagic", "Unknown magic rarity"); }
	if (Rarity == Varies.GetTag()) { return LOCTEXT("Varies", "Varies"); }
	if (Rarity == Unknown.GetTag()) { return LOCTEXT("Unknown", "Unknown rarity"); }
	return Rarity.IsValid() ? FText::FromName(Rarity.GetTagName()) : LOCTEXT("Unspecified", "Rarity unspecified");
}

FDnd5eItemBrowserDetails Dnd5eItemBrowserDetails::Build(const FDnd5eItem& Item, const UDnd5eItemDefinitionCatalogObject* Definitions)
{
	FDnd5eItemBrowserDetails Result;
	Result.Title = FText::FromString(Item.Header.DisplayName);
	Result.Source = Dnd5eBrowserText::FormatSource(Item.Header);
	Result.Classification = FormatClassification(Item);

	TArray<FString> Facts;
	if (Item.Rarity.IsValid() && Item.Rarity != Dnd5e::Item::Rarity::None.GetTag())
	{
		Facts.Add(FormatRarity(Item.Rarity).ToString());
	}
	AddFact(Facts, LOCTEXT("Price", "Price: {0}"), FormatPrice(Item.Price));
	if (Item.bHasWeight) { AddFact(Facts, LOCTEXT("Weight", "Weight: {0}"), FormatWeight(Item.Weight)); }
	AddFact(Facts, LOCTEXT("Charges", "Charges: {0}"), FormatCharges(Item.ChargeData.Charges));
	Result.Facts = FText::FromString(FString::Join(Facts, TEXT("\n")));

	TArray<FString> Equipment;
	AddWeaponFacts(Equipment, Item);
	AddArmorFacts(Equipment, Item);
	Result.Equipment = FText::FromString(FString::Join(Equipment, TEXT("\n")));

	FString Description;
	Dnd5eBrowserText::AppendBlocks(Item.Description, Description, 0);
	if (IsValid(Definitions)) { AppendDefinitionDescriptions(Item, *Definitions, Description); }
	if (Item.bHasSupplementaryContent)
	{
		AppendDescriptionSection(Item.SupplementaryContent.Entries, Description);
	}
	Result.Description = Dnd5eBrowserText::FinishPreview(MoveTemp(Description));
	return Result;
}

FText Dnd5eItemBrowserDetails::FormatDescription(const TArray<FDnd5eContentBlock>& Blocks)
{
	return Dnd5eBrowserText::FormatDescription(Blocks);
}

#undef LOCTEXT_NAMESPACE
