#include "GAS/BossArenaGameplayEffects.h"

#include "Attributes/BossArenaAttributeSetBase.h"
#include "Attributes/WarriorAttributeSet.h"
#include "BossArenaGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaGameplayEffects)

namespace BossArenaEffectsImpl
{
	FGameplayModifierInfo MakeSetByCallerModifier(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Modifier;
	}
}

void UBossArenaGameplayEffect::AddGrantedTags(const FGameplayTagContainer& Tags)
{
	UTargetTagsGameplayEffectComponent* TagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("GrantedTags"));
	FInheritedTagContainer Container;
	Container.Added = Tags;
	Container.CombinedTags = Tags;
	TagsComponent->SetAndApplyTargetTagChanges(Container);
	GEComponents.Add(TagsComponent);
}

UGE_BossArenaDamage::UGE_BossArenaDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(BossArenaEffectsImpl::MakeSetByCallerModifier(UBossArenaAttributeSetBase::GetIncomingDamageAttribute(), BossArenaTags::Data_Damage));
}

UGE_WarriorRageDelta::UGE_WarriorRageDelta()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(BossArenaEffectsImpl::MakeSetByCallerModifier(UWarriorAttributeSet::GetRageAttribute(), BossArenaTags::Data_Rage));
}

UGE_DodgeInvulnerability::UGE_DodgeInvulnerability()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.45f));

	FGameplayTagContainer Tags;
	Tags.AddTag(BossArenaTags::State_Invulnerable);
	Tags.AddTag(BossArenaTags::State_Dodging);
	AddGrantedTags(Tags);
}

UGE_WarriorBlocking::UGE_WarriorBlocking()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddGrantedTags(FGameplayTagContainer(BossArenaTags::State_Blocking));
}

UGE_BossArenaDead::UGE_BossArenaDead()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddGrantedTags(FGameplayTagContainer(BossArenaTags::State_Dead));
}

UGE_WarriorCooldownBase::UGE_WarriorCooldownBase()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = BossArenaTags::Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

UGE_Cooldown_Whirlwind::UGE_Cooldown_Whirlwind() { AddGrantedTags(FGameplayTagContainer(BossArenaTags::Cooldown_Warrior_Whirlwind)); }
UGE_Cooldown_LeapSlam::UGE_Cooldown_LeapSlam() { AddGrantedTags(FGameplayTagContainer(BossArenaTags::Cooldown_Warrior_LeapSlam)); }
UGE_Cooldown_Shockwave::UGE_Cooldown_Shockwave() { AddGrantedTags(FGameplayTagContainer(BossArenaTags::Cooldown_Warrior_Shockwave)); }
UGE_Cooldown_GroundRend::UGE_Cooldown_GroundRend() { AddGrantedTags(FGameplayTagContainer(BossArenaTags::Cooldown_Warrior_GroundRend)); }
UGE_Cooldown_Taunt::UGE_Cooldown_Taunt() { AddGrantedTags(FGameplayTagContainer(BossArenaTags::Cooldown_Warrior_Taunt)); }
UGE_Cooldown_Execute::UGE_Cooldown_Execute() { AddGrantedTags(FGameplayTagContainer(BossArenaTags::Cooldown_Warrior_Execute)); }
UGE_Cooldown_Dodge::UGE_Cooldown_Dodge() { AddGrantedTags(FGameplayTagContainer(BossArenaTags::Cooldown_Warrior_Dodge)); }
