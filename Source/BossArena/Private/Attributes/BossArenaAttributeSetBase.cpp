#include "Attributes/BossArenaAttributeSetBase.h"

#include "BossArenaCharacterBase.h"
#include "BossArenaGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaAttributeSetBase)

UBossArenaAttributeSetBase::UBossArenaAttributeSetBase()
{
	InitHealth(1000.0f);
	InitMaxHealth(1000.0f);
	InitIncomingDamage(0.0f);
}

void UBossArenaAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UBossArenaAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBossArenaAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UBossArenaAttributeSetBase::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void UBossArenaAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

bool UBossArenaAttributeSetBase::IsUnblockable(const FGameplayEffectSpec& Spec)
{
	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);
	return AssetTags.HasTag(BossArenaTags::Damage_Unblockable);
}

bool UBossArenaAttributeSetBase::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	if (Data.EvaluatedData.Attribute != GetIncomingDamageAttribute())
	{
		return true;
	}

	ABossArenaCharacterBase* Owner = Cast<ABossArenaCharacterBase>(GetOwningActor());
	if (Owner && !Owner->IsAlive())
	{
		return false;
	}

	// Server-authoritative i-frames: the dodge GE grants State.Invulnerable on the server's copy
	// of the ASC; anything that lands while it is active is discarded. The hard-enrage wipe is
	// tagged Damage.Unblockable and goes straight through.
	if (!IsUnblockable(Data.EffectSpec) && Data.Target.HasMatchingGameplayTag(BossArenaTags::State_Invulnerable))
	{
		if (Owner)
		{
			Owner->NotifyDamageNegated();
		}
		return false;
	}

	return true;
}

void UBossArenaAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float RawDamage = GetIncomingDamage();
		SetIncomingDamage(0.0f);
		if (RawDamage <= 0.0f)
		{
			return;
		}

		const float FinalDamage = IsUnblockable(Data.EffectSpec) ? RawDamage : FMath::Max(0.0f, MitigateDamage(RawDamage, Data));
		const float NewHealth = FMath::Clamp(GetHealth() - FinalDamage, 0.0f, GetMaxHealth());
		SetHealth(NewHealth);
		OnDamageApplied(FinalDamage, Data);

		if (ABossArenaCharacterBase* Owner = Cast<ABossArenaCharacterBase>(GetOwningActor()))
		{
			AActor* Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
			Owner->HandleDamageTaken(FinalDamage, Instigator, NewHealth <= 0.0f);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}

void UBossArenaAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBossArenaAttributeSetBase, Health, OldValue);
}

void UBossArenaAttributeSetBase::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBossArenaAttributeSetBase, MaxHealth, OldValue);
}
