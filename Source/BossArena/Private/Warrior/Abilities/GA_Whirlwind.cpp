#include "Warrior/Abilities/GA_Whirlwind.h"

#include "BossArenaGameplayTags.h"
#include "GAS/BossArenaGameplayEffects.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_Whirlwind)

UGA_Whirlwind::UGA_Whirlwind()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(BossArenaTags::Ability_Warrior_Whirlwind);
	SetAssetTags(Tags);
	CooldownGameplayEffectClass = UGE_Cooldown_Whirlwind::StaticClass();

	AimShape = FAoEShape::MakeCircle(450.0f);
	BaseDamage = 260.0f;
	ImpactDelay = 0.2f;
	RageCost = 25.0f;
	CooldownDuration = 8.0f;
	MontageSlot = EWarriorMontage::Whirlwind;
	bSupportsHoldToAim = false;
}

void UGA_Whirlwind::ExecuteAbility()
{
	if (!CommitOrCancel())
	{
		return;
	}
	PulsesRemaining = PulseCount;
	PlayWarriorMontage(MontageSlot);
	ShowAimPreview(AimShape, FVector::ZeroVector);
	WaitThen(ImpactDelay, GET_FUNCTION_NAME_CHECKED(UGA_Whirlwind, OnPulse));
}

void UGA_Whirlwind::OnPulse()
{
	ResolveAoE(AimShape, GetFeetLocation(), GetAimRotation(), BaseDamage, false);
	if (--PulsesRemaining > 0)
	{
		WaitThen(PulseInterval, GET_FUNCTION_NAME_CHECKED(UGA_Whirlwind, OnPulse));
		return;
	}
	FinishAbility();
}
