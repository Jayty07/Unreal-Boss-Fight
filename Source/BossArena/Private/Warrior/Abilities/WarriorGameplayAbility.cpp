#include "Warrior/Abilities/WarriorGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AoE/AoETelegraphActor.h"
#include "AoE/BossArenaAoELibrary.h"
#include "Attributes/WarriorAttributeSet.h"
#include "BossArenaCharacterBase.h"
#include "BossArenaGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "GAS/BossArenaGameplayEffects.h"
#include "Warrior/WarriorCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WarriorGameplayAbility)

UWarriorGameplayAbility::UWarriorGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	FGameplayTagContainer Tags;
	Tags.AddTag(BossArenaTags::Ability_Warrior_Attack);
	SetAssetTags(Tags);

	ActivationBlockedTags.AddTag(BossArenaTags::State_Dead);
	ActivationBlockedTags.AddTag(BossArenaTags::State_Dodging);
	ActivationBlockedTags.AddTag(BossArenaTags::State_Leaping);

	// One attack at a time.
	BlockAbilitiesWithTag.AddTag(BossArenaTags::Ability_Warrior_Attack);
}

bool UWarriorGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	return IsUsableNow(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
}

bool UWarriorGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (RageCost > 0.0f)
	{
		const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		bool bFound = false;
		const float Rage = ASC ? ASC->GetGameplayAttributeValue(UWarriorAttributeSet::GetRageAttribute(), bFound) : 0.0f;
		if (!bFound || Rage < RageCost)
		{
			return false;
		}
	}
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UWarriorGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	if (RageCost > 0.0f)
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UGE_WarriorRageDelta::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(BossArenaTags::Data_Rage, -RageCost);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
		}
	}
}

void UWarriorGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	if (!CooldownEffect || CooldownDuration <= 0.0f)
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownEffect->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BossArenaTags::Data_Cooldown, CooldownDuration);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

float UWarriorGameplayAbility::GetCooldownRemaining(const UAbilitySystemComponent* ASC, float& OutDuration) const
{
	OutDuration = CooldownDuration;
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (!ASC || !CooldownTags || CooldownTags->IsEmpty())
	{
		return 0.0f;
	}

	float Best = 0.0f;
	const TArray<TPair<float, float>> Remaining = ASC->GetActiveEffectsTimeRemainingAndDuration(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags));
	for (const TPair<float, float>& Entry : Remaining)
	{
		if (Entry.Key > Best)
		{
			Best = Entry.Key;
			OutDuration = Entry.Value;
		}
	}
	return Best;
}

void UWarriorGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (bHasBlueprintActivate)
	{
		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		return;
	}

	const AWarriorCharacter* Warrior = GetWarrior();
	if (bSupportsHoldToAim && Warrior && Warrior->IsHoldToAimEnabled())
	{
		// Press shows the projected indicator, release casts. Both client and server wait for the release.
		bAiming = true;
		ShowAimPreview(AimShape, AimOffset);
		UAbilityTask_WaitInputRelease* WaitRelease = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
		WaitRelease->OnRelease.AddDynamic(this, &UWarriorGameplayAbility::OnAimReleased);
		WaitRelease->ReadyForActivation();
		return;
	}

	ExecuteAbility();
}

void UWarriorGameplayAbility::OnAimReleased(float TimeHeld)
{
	bAiming = false;
	ExecuteAbility();
}

void UWarriorGameplayAbility::ExecuteAbility()
{
	if (!CommitOrCancel())
	{
		return;
	}

	FaceAim();
	PlayWarriorMontage(MontageSlot);
	ShowAimPreview(AimShape, AimOffset);
	WaitThen(ImpactDelay, GET_FUNCTION_NAME_CHECKED(UWarriorGameplayAbility, OnImpactTimer));
}

void UWarriorGameplayAbility::OnImpactTimer()
{
	HideAimPreview();
	PerformImpact();
	FinishAbility();
}

void UWarriorGameplayAbility::PerformImpact()
{
	ResolveAoE(AimShape, GetAimOrigin(AimOffset), GetAimRotation(), BaseDamage, bInterruptsTargets);
}

void UWarriorGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	bAiming = false;
	HideAimPreview();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UWarriorGameplayAbility::CommitOrCancel()
{
	if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		return true;
	}
	FinishAbility(true);
	return false;
}

void UWarriorGameplayAbility::FinishAbility(bool bCancelled)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
}

AWarriorCharacter* UWarriorGameplayAbility::GetWarrior() const
{
	return CurrentActorInfo ? Cast<AWarriorCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
}

FRotator UWarriorGameplayAbility::GetAimRotation() const
{
	const AWarriorCharacter* Warrior = GetWarrior();
	if (!Warrior)
	{
		return FRotator::ZeroRotator;
	}
	const float Yaw = Warrior->GetController() ? Warrior->GetControlRotation().Yaw : Warrior->GetActorRotation().Yaw;
	return FRotator(0.0f, Yaw, 0.0f);
}

void UWarriorGameplayAbility::FaceAim() const
{
	if (AWarriorCharacter* Warrior = GetWarrior())
	{
		Warrior->SetActorRotation(GetAimRotation());
	}
}

FVector UWarriorGameplayAbility::GetFeetLocation() const
{
	return UBossArenaAoELibrary::GetGroundLocation(GetWarrior());
}

FVector UWarriorGameplayAbility::GetAimOrigin(const FVector& LocalOffset) const
{
	return GetFeetLocation() + GetAimRotation().RotateVector(LocalOffset);
}

float UWarriorGameplayAbility::GetScaledDamage(float Base) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	bool bFound = false;
	const float Strength = ASC ? ASC->GetGameplayAttributeValue(UWarriorAttributeSet::GetStrengthAttribute(), bFound) : 0.0f;
	return Base * (1.0f + (bFound ? Strength : 0.0f) / 100.0f);
}

int32 UWarriorGameplayAbility::ResolveAoE(const FAoEShape& Shape, const FVector& Origin, const FRotator& Rotation, float InBaseDamage, bool bInterrupts)
{
	AWarriorCharacter* Warrior = GetWarrior();
	if (!Warrior || !Warrior->HasAuthority())
	{
		return 0;
	}

	AAoETelegraphActor::SpawnFlash(Warrior, Shape, FTransform(Rotation, Origin), IndicatorColor);

	TArray<ABossArenaCharacterBase*> Targets;
	UBossArenaAoELibrary::GatherTargetsInShape(Warrior, Shape, Origin, Rotation, Warrior->GetTeam(), false, Origin, Targets);
	if (Targets.Num() == 0)
	{
		return 0;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (InBaseDamage > 0.0f)
	{
		UBossArenaAoELibrary::ApplyAoEDamage(ASC, Targets, GetScaledDamage(InBaseDamage), false, this);
	}
	if (bInterrupts)
	{
		UBossArenaAoELibrary::SendInterrupt(Warrior, Targets);
	}
	if (RageOnHit > 0.0f && ASC)
	{
		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UGE_WarriorRageDelta::StaticClass(), 1.0f);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(BossArenaTags::Data_Rage, RageOnHit);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	OnTargetsHit(Targets);
	return Targets.Num();
}

void UWarriorGameplayAbility::ShowAimPreview(const FAoEShape& Shape, const FVector& LocalOffset) const
{
	if (!bShowAimPreview)
	{
		return;
	}
	if (AWarriorCharacter* Warrior = GetWarrior())
	{
		Warrior->ShowAimIndicator(Shape, LocalOffset, IndicatorColor);
	}
}

void UWarriorGameplayAbility::HideAimPreview() const
{
	if (AWarriorCharacter* Warrior = GetWarrior())
	{
		Warrior->HideAimIndicator();
	}
}

void UWarriorGameplayAbility::PlayWarriorMontage(EWarriorMontage Slot, float PlayRate)
{
	const AWarriorCharacter* Warrior = GetWarrior();
	const UWarriorAnimSet* AnimSet = Warrior ? Warrior->GetAnimSet() : nullptr;
	UAnimMontage* Montage = AnimSet ? AnimSet->GetMontage(Slot) : nullptr;
	if (!Montage)
	{
		return;
	}
	// Montage is cosmetic; gameplay timing is driven by WaitDelay so missing/short anims never desync damage.
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, PlayRate, NAME_None, false);
	Task->ReadyForActivation();
}

void UWarriorGameplayAbility::WaitThen(float Delay, FName FunctionName)
{
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(Delay, 0.01f));
	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, FunctionName);
	Wait->OnFinish.Add(Delegate);
	Wait->ReadyForActivation();
}
