#include "Warrior/Abilities/GA_GroundRend.h"

#include "AoE/AoEPersistentZone.h"
#include "AoE/AoETelegraphActor.h"
#include "BossArenaGameplayTags.h"
#include "GAS/BossArenaGameplayEffects.h"
#include "Warrior/WarriorCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_GroundRend)

UGA_GroundRend::UGA_GroundRend()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(BossArenaTags::Ability_Warrior_GroundRend);
	SetAssetTags(Tags);
	CooldownGameplayEffectClass = UGE_Cooldown_GroundRend::StaticClass();

	AimShape = FAoEShape::MakeRectangle(700.0f, 360.0f);
	AimOffset = FVector(450.0f, 0.0f, 0.0f);
	ImpactDelay = 0.4f;
	RageCost = 20.0f;
	CooldownDuration = 15.0f;
	MontageSlot = EWarriorMontage::GroundRend;
}

void UGA_GroundRend::PerformImpact()
{
	AWarriorCharacter* Warrior = GetWarrior();
	if (!Warrior || !Warrior->HasAuthority())
	{
		return;
	}
	const FTransform ZoneTransform(GetAimRotation(), GetAimOrigin(AimOffset));
	AAoEPersistentZone::SpawnZone(Warrior, GetAbilitySystemComponentFromActorInfo(), Warrior->GetTeam(), AimShape, ZoneTransform,
		ZoneDuration, ZoneTickInterval, GetScaledDamage(DamagePerTick), IndicatorColor);
}
