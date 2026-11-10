// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eNativeTagsTest, "Dnd5e.Tags.NativeVocabulary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eNativeTagsTest::RunTest(const FString& Parameters)
{
	struct FTagContract { const FNativeGameplayTag* Native; const TCHAR* Name; };
	const FTagContract Cases[] =
	{
		#include "Dnd5eNativeTagContracts.inl"
	};
	TSet<FName> Seen;
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	for (const FTagContract& Case : Cases)
	{
		const FGameplayTag Tag = Case.Native->GetTag();
		TestTrue(Case.Name, Tag.IsValid());
		TestEqual(TEXT("The native symbol has its fixed public string"), Tag.GetTagName().ToString(), FString(Case.Name));
		TestFalse(TEXT("No two native symbols accidentally share an identity"), Seen.Contains(Tag.GetTagName()));
		Seen.Add(Tag.GetTagName());
		TestTrue(TEXT("The identity is registered in Unreal's dictionary"),
			Manager.RequestGameplayTag(FName(Case.Name), false) == Tag);
		TestTrue(TEXT("The tag uses the correct root prefix"), Tag.GetTagName().ToString().StartsWith(TEXT("Dnd5e.")));
	}
	TestEqual(TEXT("Every documented exported native identity was checked"), Seen.Num(), 178);
	for (const TCHAR* Legacy : { TEXT("Dnd5.e.Damage.Type.Fire"), TEXT("Dnd5.e.Damage.Type.Necrotic"),
		TEXT("Dnd5e.Condition.Type.Restraint"), TEXT("Dnd5e.Spell.Duration.Requirement.Concentration"),
		TEXT("Dnd5e.Spell.CastingTime.Unit.Action"), TEXT("Dnd5e.Spell.Attack.Type.Melee") })
	{
		// FindTagNode may follow a configured redirect; compare the returned
		// canonical name rather than mistaking successful migration for an alias.
		const TSharedPtr<FGameplayTagNode> Node = Manager.FindTagNode(FName(Legacy));
		TestTrue(TEXT("Legacy names are not duplicate registered native identities"),
			!Node.IsValid() || Node->GetCompleteTagName() != FName(Legacy));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eTagHierarchyTest, "Dnd5e.Tags.HierarchyAndRoles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eTagHierarchyTest::RunTest(const FString& Parameters)
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	const FGameplayTag Map = Dnd5e::Content::ImageRole::Map;
	const FGameplayTag PlayerMap = Dnd5e::Content::ImageRole::PlayerMap;
	TestTrue(TEXT("Player maps also match maps"), PlayerMap.MatchesTag(Map));
	TestFalse(TEXT("A map does not imply a player map"), Map.MatchesTag(PlayerMap));
	TestFalse(TEXT("Hierarchical relatives are not exact identities"), Map.MatchesTagExact(PlayerMap));
	const FGameplayTag Action = Dnd5e::Activation::Time::Action;
	const FGameplayTag Bonus = Dnd5e::Activation::Time::BonusAction;
	const FGameplayTag Timing = Manager.RequestGameplayTag(TEXT("Dnd5e.Activation.Time"), false);
	TestTrue(TEXT("Action and bonus action share the timing branch"), Action.MatchesTag(Timing) && Bonus.MatchesTag(Timing));
	TestFalse(TEXT("A bonus action is not a subtype of an action"), Bonus.MatchesTag(Action));
	TestFalse(TEXT("An action is not a subtype of a bonus action"), Action.MatchesTag(Bonus));
	const FGameplayTag Spell = Manager.RequestGameplayTag(TEXT("Dnd5e.Spell"), false);
	TestFalse(TEXT("Shared activation timing is not spell-owned"), Action.MatchesTag(Spell));
	const FGameplayTag Melee = Dnd5e::Attack::Mode::Melee;
	TestTrue(TEXT("Melee is a shared attack mode"), Melee.MatchesTag(Manager.RequestGameplayTag(TEXT("Dnd5e.Attack.Mode"), false)));
	TestFalse(TEXT("Melee does not imply a spell attack"), Melee.MatchesTag(Spell));
	TestFalse(TEXT("Concentration is not a spell-owned identity"), Dnd5e::Trait::Concentration.GetTag().MatchesTag(Spell));
	TestTrue(TEXT("Ritual keeps its spell-capability scope"), Dnd5e::Spell::Trait::Ritual.GetTag().MatchesTag(Spell));
	TestFalse(TEXT("Ritual is not an activation-time unit"), Dnd5e::Spell::Trait::Ritual.GetTag().MatchesTag(Timing));
	TestTrue(TEXT("Schools live under the shared magic branch"), Dnd5e::Magic::School::Evocation.GetTag().MatchesTag(
		Manager.RequestGameplayTag(TEXT("Dnd5e.Magic.School"), false)));
	TestFalse(TEXT("Range Self and area-origin Self are separate identities"), Dnd5e::Range::Kind::Self.GetTag() == Dnd5e::Area::Origin::Self.GetTag());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eTagRedirectTest, "Dnd5e.Tags.ConfiguredRedirects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDnd5eTagRedirectTest::RunTest(const FString& Parameters)
{
	int32 Checked = 0;
	for (const FGameplayTagRedirect& Redirect : GetDefault<UGameplayTagsSettings>()->GameplayTagRedirects)
	{
		const FString Old = Redirect.OldTagName.ToString();
		if (!Old.StartsWith(TEXT("Dnd5e.")) && !Old.StartsWith(TEXT("Dnd5.e."))) { continue; }
		++Checked;
		const FGameplayTag Result = UGameplayTagsManager::Get().RequestGameplayTag(Redirect.NewTagName, false);
		TestTrue(TEXT("The target is in the current dictionary"),
			UGameplayTagsManager::Get().RequestGameplayTag(Redirect.NewTagName, false).IsValid());
		// Exercise the same text-import route used by reflected tag properties.
		const FString LegacyText = FString::Printf(TEXT("(TagName=\"%s\")"), *Old);
		const TCHAR* Buffer = *LegacyText;
		FGameplayTag Imported;
		TestTrue(TEXT("A serialized old tag can be imported"), Imported.ImportTextItem(Buffer, PPF_None, nullptr, nullptr));
		TestTrue(TEXT("Import resolves the legacy identity"), Imported == Result);
	}
	// The plugin must also work in a fresh host with no legacy configuration.
	AddInfo(FString::Printf(TEXT("Checked %d configured Dnd5e leaf redirects; fresh hosts may have none."), Checked));
	return !HasAnyErrors();
}

#endif
