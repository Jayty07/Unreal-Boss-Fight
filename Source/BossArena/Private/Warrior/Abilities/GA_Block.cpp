#include "Warrior/Abilities/GA_Block.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "BossArenaGameplayTags.h"
#include "GAS/BossArenaGameplayEffects.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_Block)

UGA_Block::UGA_Block()
{
	SetAssetTags(FGameplayTagContainer(BossArenaTags::Ability_Warrior_Block));
	BlockAbilitiesWithTag.Reset();
	BlockAbilitiesWithTag.AddTag(BossArenaTags::Ability_Warrior_Attack);
	CancelAbilitiesWithTag.AddTag(BossArenaTags::Ability_Warrior_Attack);
	MontageSlot = EWarriorMontage::Block;
	bSupportsHoldToAim = false;
	bShowAimPreview = false;
}

void UGA_Block::ExecuteAbility()
{
	if (!CommitOrCancel())
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UGE_WarriorBlocking::StaticClass(), 1.0f);
	if (Spec.IsValid())
	{
		BlockEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Spec);
	}
	PlayWarriorMontage(MontageSlot);

	UAbilityTask_WaitInputRelease* WaitRelease = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	WaitRelease->OnRelease.AddDynamic(this, &UGA_Block::OnBlockReleased);
	WaitRelease->ReadyForActivation();
}

void UGA_Block::OnBlockReleased(float TimeHeld)
{
	FinishAbility();
}

void UGA_Block::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (BlockEffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->RemoveActiveGameplayEffect(BlockEffectHandle);
		}
		BlockEffectHandle.Invalidate();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
