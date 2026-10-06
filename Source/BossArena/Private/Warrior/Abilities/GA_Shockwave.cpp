#include "Warrior/Abilities/GA_Shockwave.h"

#include "BossArenaGameplayTags.h"
#include "GAS/BossArenaGameplayEffects.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_Shockwave)

UGA_Shockwave::UGA_Shockwave()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(BossArenaTags::Ability_Warrior_Shockwave);
	SetAssetTags(Tags);
	CooldownGameplayEffectClass = UGE_Cooldown_Shockwave::StaticClass();

	AimShape = FAoEShape::MakeLine(1400.0f, 260.0f);
	AimOffset = FVector(40.0f, 0.0f, 0.0f);
	BaseDamage = 600.0f;
	ImpactDelay = 0.35f;
	RageCost = 15.0f;
	CooldownDuration = 10.0f;
	bInterruptsTargets = true;
	MontageSlot = EWarriorMontage::Shockwave;
}
