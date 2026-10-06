#include "Warrior/Abilities/GA_TauntingShout.h"

#include "BossArenaCharacterBase.h"
#include "BossArenaGameplayTags.h"
#include "Boss/ThreatComponent.h"
#include "GAS/BossArenaGameplayEffects.h"
#include "Warrior/WarriorCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_TauntingShout)

UGA_TauntingShout::UGA_TauntingShout()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(BossArenaTags::Ability_Warrior_Taunt);
	SetAssetTags(Tags);
	CooldownGameplayEffectClass = UGE_Cooldown_Taunt::StaticClass();

	AimShape = FAoEShape::MakeCircle(1500.0f);
	ImpactDelay = 0.25f;
	CooldownDuration = 10.0f;
	RageOnHit = 10.0f;
	MontageSlot = EWarriorMontage::TauntShout;
	IndicatorColor = FLinearColor(1.0f, 0.65f, 0.15f);
	bSupportsHoldToAim = false;
}

void UGA_TauntingShout::OnTargetsHit(const TArray<ABossArenaCharacterBase*>& Targets)
{
	AWarriorCharacter* Warrior = GetWarrior();
	for (ABossArenaCharacterBase* Target : Targets)
	{
		if (UThreatComponent* Threat = Target ? Target->FindComponentByClass<UThreatComponent>() : nullptr)
		{
			Threat->Taunt(Warrior, BurstThreat, FixateDuration);
			Target->MulticastCombatText(0.0f, EBossArenaCombatText::Taunted);
		}
	}
}
