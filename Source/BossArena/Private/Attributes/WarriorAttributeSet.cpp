#include "Attributes/WarriorAttributeSet.h"

#include "BossArenaGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WarriorAttributeSet)

UWarriorAttributeSet::UWarriorAttributeSet()
{
	InitMaxHealth(10000.0f);
	InitHealth(10000.0f);
	InitRage(0.0f);
	InitMaxRage(100.0f);
	InitArmor(50.0f);
	InitStrength(20.0f);
}

void UWarriorAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UWarriorAttributeSet, Rage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWarriorAttributeSet, MaxRage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWarriorAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWarriorAttributeSet, Strength, COND_None, REPNOTIFY_Always);
}

void UWarriorAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::ClampAttribute(Attribute, NewValue);
	if (Attribute == GetRageAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxRage());
	}
	else if (Attribute == GetArmorAttribute() || Attribute == GetStrengthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UWarriorAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetRageAttribute())
	{
		SetRage(FMath::Clamp(GetRage(), 0.0f, GetMaxRage()));
	}
}

float UWarriorAttributeSet::MitigateDamage(float RawDamage, const FGameplayEffectModCallbackData& Data) const
{
	float Damage = RawDamage * 100.0f / (100.0f + FMath::Max(0.0f, GetArmor()));
	if (Data.Target.HasMatchingGameplayTag(BossArenaTags::State_Blocking))
	{
		Damage *= (1.0f - FMath::Clamp(BlockMitigation, 0.0f, 1.0f));
	}
	return Damage;
}

void UWarriorAttributeSet::OnDamageApplied(float FinalDamage, const FGameplayEffectModCallbackData& Data)
{
	SetRage(FMath::Clamp(GetRage() + FinalDamage * RagePerDamageTaken, 0.0f, GetMaxRage()));
}

void UWarriorAttributeSet::OnRep_Rage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWarriorAttributeSet, Rage, OldValue);
}

void UWarriorAttributeSet::OnRep_MaxRage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWarriorAttributeSet, MaxRage, OldValue);
}

void UWarriorAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWarriorAttributeSet, Armor, OldValue);
}

void UWarriorAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWarriorAttributeSet, Strength, OldValue);
}
