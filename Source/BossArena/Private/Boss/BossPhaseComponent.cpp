#include "Boss/BossPhaseComponent.h"

#include "AbilitySystemComponent.h"
#include "Attributes/BossArenaAttributeSetBase.h"
#include "BossArenaGameplayTags.h"
#include "Engine/World.h"
#include "Game/BossArenaGameState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossPhaseComponent)

UBossPhaseComponent::UBossPhaseComponent()
{
	SetIsReplicatedByDefault(true);
}

void UBossPhaseComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBossPhaseComponent, CurrentPhaseIndex);
}

void UBossPhaseComponent::StartPhases(UAbilitySystemComponent* InASC)
{
	if (!InASC || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (ASC.Get() != InASC)
	{
		if (UAbilitySystemComponent* OldASC = ASC.Get())
		{
			OldASC->GetGameplayAttributeValueChangeDelegate(UBossArenaAttributeSetBase::GetHealthAttribute()).Remove(HealthChangedHandle);
		}
		ASC = InASC;
		HealthChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UBossArenaAttributeSetBase::GetHealthAttribute()).AddUObject(this, &UBossPhaseComponent::OnHealthChanged);
	}

	EnterPhase(0, false);
}

const FBossPhaseDefinition* UBossPhaseComponent::GetCurrentPhase() const
{
	return Phases.IsValidIndex(CurrentPhaseIndex) ? &Phases[CurrentPhaseIndex] : nullptr;
}

FString UBossPhaseComponent::GetCurrentPhaseName() const
{
	const FBossPhaseDefinition* Phase = GetCurrentPhase();
	return Phase ? Phase->PhaseName : FString();
}

void UBossPhaseComponent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	UAbilitySystemComponent* OwnerASC = ASC.Get();
	if (!OwnerASC || Data.NewValue <= 0.0f)
	{
		return;
	}

	bool bFound = false;
	const float MaxHealth = OwnerASC->GetGameplayAttributeValue(UBossArenaAttributeSetBase::GetMaxHealthAttribute(), bFound);
	if (!bFound || MaxHealth <= 0.0f)
	{
		return;
	}

	// A big hit can skip a phase entirely; jump to the deepest threshold crossed but only fire one transition.
	const float Fraction = Data.NewValue / MaxHealth;
	int32 Target = CurrentPhaseIndex;
	for (int32 Index = CurrentPhaseIndex + 1; Index < Phases.Num(); ++Index)
	{
		if (Fraction <= Phases[Index].HealthThreshold)
		{
			Target = Index;
		}
	}
	if (Target != CurrentPhaseIndex)
	{
		EnterPhase(Target, true);
	}
}

void UBossPhaseComponent::EnterPhase(int32 NewPhaseIndex, bool bPlayTransition)
{
	if (!Phases.IsValidIndex(NewPhaseIndex))
	{
		return;
	}

	CurrentPhaseIndex = NewPhaseIndex;
	const FBossPhaseDefinition& Phase = Phases[NewPhaseIndex];

	if (bPlayTransition)
	{
		if (UAbilitySystemComponent* OwnerASC = ASC.Get())
		{
			const FGameplayTagContainer BossAbilityTags(BossArenaTags::Ability_Boss);
			OwnerASC->CancelAbilities(&BossAbilityTags);
			if (Phase.TransitionAbility)
			{
				OwnerASC->TryActivateAbilityByClass(Phase.TransitionAbility);
			}
		}

		if (ABossArenaGameState* GameState = GetWorld()->GetGameState<ABossArenaGameState>())
		{
			GameState->BroadcastRaidWarning(Phase.PhaseName, FLinearColor(1.0f, 0.55f, 0.1f), 4.0f);
		}
	}

	OnPhaseChanged.Broadcast(CurrentPhaseIndex);
}

void UBossPhaseComponent::OnRep_CurrentPhaseIndex()
{
	OnPhaseChanged.Broadcast(CurrentPhaseIndex);
}
