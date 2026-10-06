#include "Attributes/BossAttributeSet.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossAttributeSet)

UBossAttributeSet::UBossAttributeSet()
{
	InitMaxHealth(150000.0f);
	InitHealth(150000.0f);
	InitDamage(1.0f);
	InitEnrageTimer(600.0f);
}

void UBossAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UBossAttributeSet, Damage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBossAttributeSet, EnrageTimer, COND_None, REPNOTIFY_Always);
}

void UBossAttributeSet::OnRep_Damage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBossAttributeSet, Damage, OldValue);
}

void UBossAttributeSet::OnRep_EnrageTimer(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBossAttributeSet, EnrageTimer, OldValue);
}
