// Developed by Neko Creative Technologies

#include "Browser/Bestiary/Dnd5eBestiaryStatBlockText.h"
#include "Bestiary/Dnd5eCreature.h"
#include "Browser/Dnd5eBrowserText.h"

namespace
{
    using namespace Dnd5eBrowserText;

    void AddText(TArray<FString>& Parts, FString Text)
    {
        if (!Text.IsEmpty()) { Parts.Add(MoveTemp(Text)); }
    }

    void AppendStat(FString& Out, const TCHAR* Label, const FString& Value)
    {
        if (!Value.IsEmpty())
        {
            AppendPreview(Out, FString::Printf(TEXT("%s  %s\n"), Label, *Value));
        }
    }

    void AppendDescription(FString& Out, const TArray<FDnd5eContentBlock>& Blocks)
    {
        if (!Blocks.IsEmpty()) { AppendPreview(Out, FormatDescription(Blocks).ToString() + TEXT("\n")); }
    }

    FString TagLabel(const FGameplayTag& Tag)
    {
        if (!Tag.IsValid()) { return {}; }
        const FString Name = Tag.ToString();
        int32 Dot = INDEX_NONE;
        Name.FindLastChar(TEXT('.'), Dot);
        return FName::NameToDisplayString(Name.Mid(Dot + 1), false);
    }

    FString AlignmentCodeLabel(const FString& Code)
    {
        if (Code == TEXT("L")) { return TEXT("lawful"); }
        if (Code == TEXT("C")) { return TEXT("chaotic"); }
        if (Code == TEXT("N")) { return TEXT("neutral"); }
        if (Code == TEXT("G")) { return TEXT("good"); }
        if (Code == TEXT("E")) { return TEXT("evil"); }
        if (Code == TEXT("U")) { return TEXT("unaligned"); }
        if (Code == TEXT("A")) { return TEXT("any alignment"); }
        return Code;
    }

    FString FormatAlignmentCodes(const TArray<FString>& Codes)
    {
        if (Codes.Num() > 2)
        {
            TArray<FString> SortedCodes = Codes;
            SortedCodes.Sort();
            const FString Key = FString::Join(SortedCodes, TEXT(","));
            if (Key == TEXT("N,NX,NY")) { return TEXT("any neutral alignment"); }
            if (Key == TEXT("C,E,L,NX,NY")) { return TEXT("any non-good alignment"); }
            if (Key == TEXT("C,G,L,NX,NY")) { return TEXT("any non-evil alignment"); }
            if (Key == TEXT("C,E,G,NX,NY")) { return TEXT("any non-lawful alignment"); }
            if (Key == TEXT("E,G,L,NX,NY")) { return TEXT("any non-chaotic alignment"); }
            if (Key == TEXT("C,E,G,NY")) { return TEXT("any chaotic alignment"); }
            if (Key == TEXT("C,E,L,NX")) { return TEXT("any evil alignment"); }
            if (Key == TEXT("E,G,L,NY")) { return TEXT("any lawful alignment"); }
            if (Key == TEXT("C,G,L,NX")) { return TEXT("any good alignment"); }
        }

        TArray<FString> Labels;
        for (const FString& Code : Codes) { AddText(Labels, AlignmentCodeLabel(Code)); }
        return FString::Join(Labels, TEXT(" "));
    }

    FString FormatAlignment(const FDnd5eCreature& Creature)
    {
        if (!Creature.AlignmentOptions.IsEmpty())
        {
            TArray<FString> Options;
            for (const FDnd5eCreatureAlignmentOption& Option : Creature.AlignmentOptions)
            {
                TArray<FString> Labels;
                AddText(Labels, FormatAlignmentCodes(Option.Codes));
                AddText(Labels, Option.Special);
                AddText(Labels, Option.Note);
                if (Option.bHasChance) { Labels.Add(FString::Printf(TEXT("(%d%%)"), Option.ChancePercent)); }
                AddText(Options, FString::Join(Labels, TEXT(" ")));
            }
            return FString::Join(Options, TEXT(" or "));
        }

        TArray<FGameplayTag> Tags;
        Creature.Alignment.GetGameplayTagArray(Tags);
        TArray<FString> Labels;
        for (const FGameplayTag& Tag : Tags) { AddText(Labels, TagLabel(Tag).ToLower()); }
        return FString::Join(Labels, TEXT(" "));
    }

    FString FormatClassification(const FDnd5eCreature& Creature)
    {
        TArray<FString> Sizes;
        for (EDnd5eSize Size : Creature.Sizes)
        {
            if (Size == EDnd5eSize::Unspecified) { continue; }
            AddText(Sizes, StaticEnum<EDnd5eSize>()->GetDisplayNameTextByValue(static_cast<int64>(Size)).ToString());
        }

        TArray<FString> Types;
        AddText(Types, TagLabel(Creature.CreatureType).ToLower());
        for (const FGameplayTag& Choice : Creature.CreatureTypeChoices)
        {
            const FString Label = TagLabel(Choice).ToLower();
            if (!Label.IsEmpty()) { Types.AddUnique(Label); }
        }
        TArray<FString> Classification;
        AddText(Classification, FString::Join(Sizes, TEXT(" or ")));
        AddText(Classification, FString::Join(Types, TEXT(" or ")));
        FString Value = FString::Join(Classification, TEXT(" "));

        TArray<FString> Subtypes;
        for (const FDnd5eCreatureSubtype& Subtype : Creature.CreatureSubtypes)
        {
            const FString Prefix = Subtype.bPrefixHidden || Subtype.Prefix.IsEmpty() ? FString() : Subtype.Prefix + TEXT(" ");
            AddText(Subtypes, Prefix + Subtype.Name);
        }
        if (!Subtypes.IsEmpty())
        {
            if (!Value.IsEmpty()) { Value += TEXT(" "); }
            Value += TEXT("(") + FString::Join(Subtypes, TEXT(", ")) + TEXT(")");
        }

        const FString Alignment = FormatAlignment(Creature);
        if (Value.IsEmpty()) { return Alignment; }
        if (!Alignment.IsEmpty()) { Value += TEXT(", ") + Alignment; }
        return Value;
    }

    FString FormatArmorClass(const TArray<FDnd5eCreatureArmorClass>& ArmorClasses)
    {
        TArray<FString> Values;
        for (const FDnd5eCreatureArmorClass& Armor : ArmorClasses)
        {
            TArray<FString> Parts;
            if (Armor.bHasValue) { Parts.Add(FString::FromInt(Armor.Value)); }
            if (!Armor.Sources.IsEmpty())
            {
                Parts.Add(TEXT("(") + FormatDescription(Armor.Sources).ToString() + TEXT(")"));
            }
            AddText(Parts, FormatInlines(Armor.Special));
            AddText(Parts, FormatInlines(Armor.Condition));
            FString Value = FString::Join(Parts, TEXT(" "));
            if (Armor.bWrapInParentheses && !Value.IsEmpty()) { Value = TEXT("(") + Value + TEXT(")"); }
            AddText(Values, MoveTemp(Value));
        }
        return FString::Join(Values, TEXT(", "));
    }

    FString FormatHitPoints(const FDnd5eCreatureHitPoints& HitPoints)
    {
        TArray<FString> Parts;
        if (HitPoints.bHasAverage) { Parts.Add(FString::FromInt(HitPoints.Average)); }
        if (!HitPoints.Formula.Expression.IsEmpty())
        {
            Parts.Add(Parts.IsEmpty() ? HitPoints.Formula.Expression : TEXT("(") + HitPoints.Formula.Expression + TEXT(")"));
        }
        AddText(Parts, FormatInlines(HitPoints.Special));
        return FString::Join(Parts, TEXT(" "));
    }

    FString MovementLabel(EDnd5eCreatureMovementMode Mode)
    {
        return StaticEnum<EDnd5eCreatureMovementMode>()->GetDisplayNameTextByValue(static_cast<int64>(Mode)).ToString().ToLower();
    }

    FString FormatSpeed(const FDnd5eCreatureSpeed& Speed)
    {
        TArray<FString> Parts;
        if (Speed.Mode != EDnd5eCreatureMovementMode::Walk) { AddText(Parts, MovementLabel(Speed.Mode)); }
        AddText(Parts, FormatDistance(Speed.Distance));
        AddText(Parts, FormatInlines(Speed.Condition));
        return FString::Join(Parts, TEXT(" "));
    }

    FString FormatMovement(const FDnd5eCreatureMovement& Movement)
    {
        TArray<FString> Values;
        for (const FDnd5eCreatureSpeed& Speed : Movement.Speeds) { AddText(Values, FormatSpeed(Speed)); }
        for (const FDnd5eCreatureSpeed& Speed : Movement.AlternateSpeeds)
        {
            const FString Alternate = FormatSpeed(Speed);
            if (!Alternate.IsEmpty()) { Values.Add(TEXT("alternate ") + Alternate); }
        }
        if (!Movement.ChoiceModes.IsEmpty())
        {
            TArray<FString> Modes;
            for (EDnd5eCreatureMovementMode Mode : Movement.ChoiceModes) { AddText(Modes, MovementLabel(Mode)); }
            TArray<FString> Parts{FString::Join(Modes, TEXT(" or "))};
            AddText(Parts, FormatDistance(Movement.ChoiceDistance));
            AddText(Parts, FormatInlines(Movement.ChoiceNote));
            Values.Add(FString::Join(Parts, TEXT(" ")));
        }
        if (Movement.bCanHover) { Values.Add(TEXT("hover")); }
        return FString::Join(Values, TEXT(", "));
    }

    FString SignedBonus(int32 Bonus)
    {
        return FString::Printf(TEXT("%+d"), Bonus);
    }

    FString FormatAbilityScore(int32 Score)
    {
        const int64 Difference = static_cast<int64>(Score) - 10;
        const int32 Modifier = static_cast<int32>((Difference - (Difference < 0 ? 1 : 0)) / 2);
        return FString::Printf(TEXT("%d (%s)"), Score, *SignedBonus(Modifier));
    }

    FString FormatBonus(bool bHasNumericBonus, int32 Bonus, const FString& Expression)
    {
        return bHasNumericBonus ? SignedBonus(Bonus) : Expression;
    }

    FString FormatSavingThrows(const TArray<FDnd5eCreatureSavingThrowBonus>& SavingThrows)
    {
        TArray<FString> Values;
        for (const FDnd5eCreatureSavingThrowBonus& Save : SavingThrows)
        {
            Values.Add(TagLabel(Save.Ability) + TEXT(" ") + FormatBonus(Save.bHasNumericBonus, Save.Bonus, Save.BonusExpression));
        }
        return FString::Join(Values, TEXT(", "));
    }

    FString FormatSkill(const FDnd5eCreatureSkillBonus& Skill)
    {
        return FName::NameToDisplayString(Skill.Skill, false) + TEXT(" ") +
            FormatBonus(Skill.bHasNumericBonus, Skill.Bonus, Skill.BonusExpression);
    }

    FString FormatSkills(const FDnd5eCreature& Creature)
    {
        TArray<FString> Values;
        for (const FDnd5eCreatureSkillBonus& Skill : Creature.Skills) { Values.Add(FormatSkill(Skill)); }
        for (const FDnd5eCreatureSkillChoice& Choice : Creature.SkillChoices)
        {
            TArray<FString> Options;
            for (const FDnd5eCreatureSkillBonus& Option : Choice.Options) { Options.Add(FormatSkill(Option)); }
            if (!Options.IsEmpty()) { Values.Add(TEXT("one of: ") + FString::Join(Options, TEXT(" or "))); }
        }
        return FString::Join(Values, TEXT(", "));
    }

    FString FormatDefenseList(const FDnd5eCreatureDefenseList& List)
    {
        // The reader emits parents before their children. Walk backward to
        // render each subtree once and prepend its notes in authored order.
        TArray<TArray<FString>> Children;
        Children.SetNum(List.Entries.Num());
        TArray<FString> Roots;
        for (int32 Index = List.Entries.Num() - 1; Index >= 0; --Index)
        {
            const auto& Entry = List.Entries[Index];
            TArray<FString> Parts;
            AddText(Parts, FormatInlines(Entry.PreNote));
            if (Entry.Kind == EDnd5eCreatureDefenseEntryKind::Type)
            {
                AddText(Parts, Entry.SourceToken.IsEmpty() ? TagLabel(Entry.TypeTag).ToLower() : Entry.SourceToken);
            }
            AddText(Parts, FormatInlines(Entry.Special));
            FString ChildText;
            for (int32 Child = Children[Index].Num() - 1; Child >= 0; --Child)
            {
                if (!ChildText.IsEmpty()) { AppendPreview(ChildText, TEXT(", ")); }
                AppendPreview(ChildText, Children[Index][Child]);
            }
            AddText(Parts, MoveTemp(ChildText));
            AddText(Parts, FormatInlines(Entry.Note));
            FString Value = FString::Join(Parts, TEXT(" ")).Left(MaxPreviewChars);
            if (Value.IsEmpty()) { continue; }
            if (Entry.ParentIndex >= 0 && Entry.ParentIndex < Index)
            {
                Children[Entry.ParentIndex].Add(MoveTemp(Value));
            }
            else
            {
                Roots.Add(MoveTemp(Value));
            }
        }
        FString Result;
        for (int32 Index = Roots.Num() - 1; Index >= 0; --Index)
        {
            if (!Result.IsEmpty()) { AppendPreview(Result, TEXT("; ")); }
            AppendPreview(Result, Roots[Index]);
        }
        return Result;
    }

    int32 StandardChallengeInteger(const FString& Cr)
    {
        if (Cr.IsEmpty() || Cr.Len() > 2) { return INDEX_NONE; }
        for (TCHAR Character : Cr)
        {
            if (Character < TEXT('0') || Character > TEXT('9')) { return INDEX_NONE; }
        }
        const int32 Value = FCString::Atoi(*Cr);
        return Value <= 30 && Cr == FString::FromInt(Value) ? Value : INDEX_NONE;
    }

    bool IsFractionalChallenge(const FString& Cr)
    {
        return Cr == TEXT("1/8") || Cr == TEXT("1/4") || Cr == TEXT("1/2");
    }

    int32 DefaultChallengeXp(const FString& Cr)
    {
        if (Cr == TEXT("1/8")) { return 25; }
        if (Cr == TEXT("1/4")) { return 50; }
        if (Cr == TEXT("1/2")) { return 100; }

        static constexpr int32 XpByCr[] = {
            10, 200, 450, 700, 1100, 1800, 2300, 2900, 3900, 5000,
            5900, 7200, 8400, 10000, 11500, 13000, 15000, 18000, 20000,
            22000, 25000, 33000, 41000, 50000, 62000, 75000, 90000,
            105000, 120000, 135000, 155000
        };
        const int32 Value = StandardChallengeInteger(Cr);
        return Value == INDEX_NONE ? INDEX_NONE : XpByCr[Value];
    }

    FString FormatProficiency(const FDnd5eCreature& Creature)
    {
        if (!Creature.ProficiencyNote.IsEmpty()) { return Creature.ProficiencyNote; }

        const FString& Cr = Creature.ChallengeRating.Base;
        if (IsFractionalChallenge(Cr)) { return TEXT("+2"); }
        const int32 Value = StandardChallengeInteger(Cr);
        return Value == INDEX_NONE ? FString() : FString::Printf(TEXT("+%d"), Value < 5 ? 2 : (Value + 3) / 4 + 1);
    }

    FString FormatChallenge(const FDnd5eCreatureChallengeRating& Rating)
    {
        TArray<FString> Parts;
        FString Base = Rating.Base;
        const int32 BaseXp = Rating.bHasXp ? Rating.Xp : DefaultChallengeXp(Rating.Base);
        if (BaseXp != INDEX_NONE)
        {
            if (!Base.IsEmpty()) { Base += TEXT(" "); }
            Base += FString::Printf(TEXT("(%d XP)"), BaseXp);
        }
        AddText(Parts, MoveTemp(Base));
        if (!Rating.Lair.IsEmpty() || Rating.bHasLairXp)
        {
            FString Lair = TEXT("lair");
            if (!Rating.Lair.IsEmpty()) { Lair += TEXT(" ") + Rating.Lair; }
            const int32 LairXp = Rating.bHasLairXp ? Rating.LairXp : DefaultChallengeXp(Rating.Lair);
            if (LairXp != INDEX_NONE) { Lair += FString::Printf(TEXT(" (%d XP)"), LairXp); }
            Parts.Add(MoveTemp(Lair));
        }
        if (!Rating.Coven.IsEmpty()) { Parts.Add(TEXT("coven ") + Rating.Coven); }
        return FString::Join(Parts, TEXT(", "));
    }

    FString SpellListLabel(const FDnd5eCreatureSpellcastingList& List)
    {
        if (List.Level == 0) { return TEXT("Cantrips"); }
        if (List.Level > 0)
        {
            FString Label = List.LowerLevel == INDEX_NONE
                ? FString::Printf(TEXT("Level %d"), List.Level)
                : FString::Printf(TEXT("Levels %d-%d"), List.LowerLevel, List.Level);
            if (List.Slots != INDEX_NONE) { Label += FString::Printf(TEXT(" (%d slots)"), List.Slots); }
            return Label;
        }
        if (List.Group == TEXT("will")) { return TEXT("At will"); }

        FString Group = FName::NameToDisplayString(List.Group, false);
        if (List.Group == TEXT("restLong")) { Group = TEXT("Long rest"); }
        if (List.UseCount == INDEX_NONE) { return Group; }
        return FString::Printf(TEXT("%s (%d%s)"), *Group, List.UseCount, List.bEach ? TEXT(" each") : TEXT(""));
    }

    void AppendSpellcasting(FString& Out, const TArray<FDnd5eCreatureSpellcasting>& Blocks)
    {
        for (const FDnd5eCreatureSpellcasting& Block : Blocks)
        {
            AppendPreview(Out, TEXT("\n"));
            AppendPreview(Out, (Block.Name.IsEmpty() ? FString(TEXT("Spellcasting")) : Block.Name) + TEXT("\n"));
            AppendDescription(Out, Block.Header);
            AppendStat(Out, TEXT("Spellcasting ability"), Block.Ability);
            AppendStat(Out, TEXT("Charges item"), FormatInlines(Block.ChargesItem));
            for (const FDnd5eCreatureSpellcastingList& List : Block.Lists)
            {
                if (Block.HiddenGroups.Contains(List.Group)) { continue; }
                TArray<FString> Spells;
                for (const FDnd5eCreatureSpellcastingEntry& Spell : List.Spells)
                {
                    if (!Spell.bHidden) { AddText(Spells, FormatInlines(Spell.Content)); }
                }
                AppendStat(Out, *SpellListLabel(List), FString::Join(Spells, TEXT(", ")));
            }
            AppendDescription(Out, Block.Footer);
        }
    }

    void AddSection(Dnd5eBestiaryStatBlockText::FStatBlock& Card, const TCHAR* Heading, FString Body)
    {
        if (Body.IsEmpty()) { return; }
        Card.Sections.Add({FText::FromString(Heading), FinishPreview(MoveTemp(Body))});
    }

    void AddEntrySection(Dnd5eBestiaryStatBlockText::FStatBlock& Card, const TCHAR* Heading,
        const TArray<FDnd5eCreatureEntry>& Entries, const TArray<FDnd5eContentBlock>* Header = nullptr,
        const FString& Tail = FString())
    {
        FString Body;
        if (Header) { AppendDescription(Body, *Header); }
        for (const FDnd5eCreatureEntry& Entry : Entries)
        {
            if (!Entry.Name.IsEmpty()) { AppendPreview(Body, Entry.Name + TEXT("\n")); }
            AppendDescription(Body, Entry.Entries);
        }
        AppendPreview(Body, Tail);
        AddSection(Card, Heading, MoveTemp(Body));
    }
}

Dnd5eBestiaryStatBlockText::FStatBlock Dnd5eBestiaryStatBlockText::Build(const FDnd5eCreature& Creature)
{
    using namespace Dnd5eBrowserText;

    FStatBlock Card;
    Card.Classification = FText::FromString(FormatClassification(Creature));

    FString Vitals;
    AppendStat(Vitals, TEXT("Armor Class"), FormatArmorClass(Creature.ArmorClasses));
    AppendStat(Vitals, TEXT("Hit Points"), FormatHitPoints(Creature.HitPoints));
    AppendStat(Vitals, TEXT("Speed"), FormatMovement(Creature.Movement));
    if (!Vitals.IsEmpty()) { Card.Vitals = FinishPreview(MoveTemp(Vitals)); }

    const auto& Scores = Creature.AbilityScores;
    if (Scores.bHasScores)
    {
        Card.Abilities = {
            {FText::FromString(TEXT("STR")), FText::FromString(FormatAbilityScore(Scores.Strength))},
            {FText::FromString(TEXT("DEX")), FText::FromString(FormatAbilityScore(Scores.Dexterity))},
            {FText::FromString(TEXT("CON")), FText::FromString(FormatAbilityScore(Scores.Constitution))},
            {FText::FromString(TEXT("INT")), FText::FromString(FormatAbilityScore(Scores.Intelligence))},
            {FText::FromString(TEXT("WIS")), FText::FromString(FormatAbilityScore(Scores.Wisdom))},
            {FText::FromString(TEXT("CHA")), FText::FromString(FormatAbilityScore(Scores.Charisma))}
        };
    }

    FString Facts;
    AppendStat(Facts, TEXT("Saving Throws"), FormatSavingThrows(Creature.SavingThrows));
    AppendStat(Facts, TEXT("Skills"), FormatSkills(Creature));
    AppendStat(Facts, TEXT("Damage Vulnerabilities"), FormatDefenseList(Creature.DamageVulnerabilities));
    AppendStat(Facts, TEXT("Damage Resistances"), FormatDefenseList(Creature.DamageResistances));
    AppendStat(Facts, TEXT("Damage Immunities"), FormatDefenseList(Creature.DamageImmunities));
    AppendStat(Facts, TEXT("Condition Immunities"), FormatDefenseList(Creature.ConditionImmunities));

    TArray<FString> Senses = Creature.Senses;
    if (Creature.PassivePerceptionPresence == EDnd5eValuePresence::Present)
    {
        const FString Passive = Creature.PassivePerceptionFormula.IsEmpty()
            ? FString::FromInt(Creature.PassivePerception) : Creature.PassivePerceptionFormula;
        Senses.Add(TEXT("passive Perception ") + Passive);
    }
    AppendStat(Facts, TEXT("Senses"), FString::Join(Senses, TEXT(", ")));
    AppendStat(Facts, TEXT("Languages"), FString::Join(Creature.Languages, TEXT(", ")));
    AppendStat(Facts, TEXT("Challenge"), FormatChallenge(Creature.ChallengeRating));
    AppendStat(Facts, TEXT("Proficiency Bonus"), FormatProficiency(Creature));
    if (!Facts.IsEmpty()) { Card.Facts = FinishPreview(MoveTemp(Facts)); }

    AddEntrySection(Card, TEXT("Traits"), Creature.Traits);
    FString Spellcasting;
    AppendSpellcasting(Spellcasting, Creature.Spellcasting);
    AddSection(Card, TEXT("Spellcasting"), MoveTemp(Spellcasting));
    AddEntrySection(Card, TEXT("Actions"), Creature.Actions);
    AddEntrySection(Card, TEXT("Bonus Actions"), Creature.BonusActions);
    AddEntrySection(Card, TEXT("Reactions"), Creature.Reactions.Entries, &Creature.Reactions.Header);

    FString LegendaryCounts;
    if (Creature.bHasLegendaryActionCount)
    {
        AppendStat(LegendaryCounts, TEXT("Legendary actions"), FString::FromInt(Creature.LegendaryActionCount));
    }
    if (Creature.bHasLegendaryActionCountInLair)
    {
        AppendStat(LegendaryCounts, TEXT("Legendary actions in lair"), FString::FromInt(Creature.LegendaryActionCountInLair));
    }
    AddEntrySection(Card, TEXT("Legendary Actions"), Creature.LegendaryActions.Entries,
        &Creature.LegendaryActions.Header, LegendaryCounts);
    AddEntrySection(Card, TEXT("Mythic Actions"), Creature.MythicActions.Entries, &Creature.MythicActions.Header);
    FString Environment;
    AppendStat(Environment, TEXT("Environment"), FString::Join(Creature.Environments, TEXT(", ")));
    if (!Environment.IsEmpty()) { Card.Environment = FinishPreview(MoveTemp(Environment)); }

    FString References;
    AppendStat(References, TEXT("Referenced in"), FString::Join(Creature.ReferenceSources, TEXT(", ")));
    if (!References.IsEmpty()) { Card.References = FinishPreview(MoveTemp(References)); }
    return Card;
}

FText Dnd5eBestiaryStatBlockText::Format(const FDnd5eCreature& Creature)
{
    using namespace Dnd5eBrowserText;

    const FStatBlock Card = Build(Creature);
    FString Preview;
    if (!Card.Classification.IsEmpty()) { AppendPreview(Preview, Card.Classification.ToString() + TEXT("\n\n")); }
    if (!Card.Vitals.IsEmpty()) { AppendPreview(Preview, Card.Vitals.ToString() + TEXT("\n")); }
    for (int32 Index = 0; Index < Card.Abilities.Num(); ++Index)
    {
        const FAbility& Ability = Card.Abilities[Index];
        AppendPreview(Preview, Ability.Label.ToString() + TEXT(" ") + Ability.Value.ToString());
        AppendPreview(Preview, Index % 3 == 2 ? TEXT("\n") : TEXT("    "));
    }
    if (!Card.Facts.IsEmpty()) { AppendPreview(Preview, Card.Facts.ToString() + TEXT("\n")); }
    for (const FSection& Section : Card.Sections)
    {
        AppendPreview(Preview, TEXT("\n") + Section.Heading.ToString() + TEXT("\n"));
        AppendPreview(Preview, Section.Body.ToString() + TEXT("\n"));
    }
    if (!Card.Environment.IsEmpty()) { AppendPreview(Preview, TEXT("\n") + Card.Environment.ToString() + TEXT("\n")); }
    if (!Card.References.IsEmpty()) { AppendPreview(Preview, TEXT("\n") + Card.References.ToString() + TEXT("\n")); }
    return FinishPreview(MoveTemp(Preview));
}
