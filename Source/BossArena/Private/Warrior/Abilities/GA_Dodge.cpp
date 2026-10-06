#include "Warrior/Abilities/GA_Dodge.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "BossArenaGameplayTags.h"
#include "GAS/BossArenaGameplayEffects.h"
#include "Warrior/WarriorCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_Dodge)

UGA_Dodge::UGA_Dodge()
{
	SetAssetTags(FGameplayTagContainer(BossArenaTags::Ability_Warrior_Dodge));
	CooldownGameplayEffectClass = UGE_Cooldown_Dodge::StaticClass();
	CooldownDuration = 3.0f;

	ActivationBlockedTags.Reset();
	ActivationBlockedTags.AddTag(BossArenaTags::State_Dead);
	ActivationBlockedTags.AddTag(BossArenaTags::State_Leaping);
	BlockAbilitiesWithTag.Reset();

	// Dodge is the panic button: it cancels attack wind-ups and block.
	CancelAbilitiesWithTag.AddTag(BossArenaTags::Ability_Warrior_Attack);
	CancelAbilitiesWithTag.AddTag(BossArenaTags::Ability_Warrior_Block);

	bSupportsHoldToAim = false;
	bShowAimPreview = false;
}

void UGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bDodgeStarted = false;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const bool bIsServer = ActorInfo->IsNetAuthority();

	if (IsLocallyControlled())
	{
		const FVector Direction = ComputeLocalDirection();

		if (!bIsServer && ASC)
		{
			FScopedPredictionWindow ScopedPrediction(ASC, true);

			FGameplayAbilityTargetData_LocationInfo* Data = new FGameplayAbilityTargetData_LocationInfo();
			Data->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
			Data->SourceLocation.LiteralTransform = FTransform(Direction);
			Data->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
			Data->TargetLocation.LiteralTransform = FTransform(Direction);

			FGameplayAbilityTargetDataHandle DataHandle(Data);
			ASC->ServerSetReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey(), DataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
		}

		StartDodge(Direction);
		return;
	}

	if (bIsServer && ASC)
	{
		// Remote client's dodge: wait for its direction (usually already buffered).
		ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).AddUObject(this, &UGA_Dodge::OnServerTargetDataReceived);
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
		if (!bDodgeStarted)
		{
			WaitThen(0.3f, GET_FUNCTION_NAME_CHECKED(UGA_Dodge, OnTargetDataTimeout));
		}
	}
}

void UGA_Dodge::OnServerTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	if (bDodgeStarted)
	{
		return;
	}

	const FGameplayAbilityTargetData* Entry = Data.Get(0);
	const FVector Requested = (Entry && Entry->HasEndPoint()) ? Entry->GetEndPoint() : FVector::ZeroVector;
	StartDodge(SanitizeDirection(Requested));
}

void UGA_Dodge::OnTargetDataTimeout()
{
	if (!bDodgeStarted)
	{
		StartDodge(SanitizeDirection(FVector::ZeroVector));
	}
}

FVector UGA_Dodge::ComputeLocalDirection() const
{
	const AWarriorCharacter* Warrior = GetWarrior();
	return SanitizeDirection(Warrior ? Warrior->GetLastMovementInputVector() : FVector::ZeroVector);
}

FVector UGA_Dodge::SanitizeDirection(const FVector& Requested) const
{
	FVector Direction = FVector(Requested.X, Requested.Y, 0.0f).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		const AWarriorCharacter* Warrior = GetWarrior();
		Direction = Warrior ? -Warrior->GetActorForwardVector().GetSafeNormal2D() : FVector::BackwardVector;
	}
	return Direction;
}

void UGA_Dodge::StartDodge(const FVector& Direction)
{
	AWarriorCharacter* Warrior = GetWarrior();
	if (!Warrior || bDodgeStarted)
	{
		return;
	}
	bDodgeStarted = true;

	// I-frames: predicted on the owning client for feedback, authoritative on the server.
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UGE_DodgeInvulnerability::StaticClass(), 1.0f);
	if (Spec.IsValid())
	{
		Spec.Data->SetDuration(InvulnerabilityDuration, true);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Spec);
	}

	const FVector Local = Warrior->GetActorRotation().UnrotateVector(Direction);
	EWarriorMontage Slot = EWarriorMontage::DodgeForward;
	if (FMath::Abs(Local.X) >= FMath::Abs(Local.Y))
	{
		Slot = Local.X >= 0.0f ? EWarriorMontage::DodgeForward : EWarriorMontage::DodgeBackward;
	}
	else
	{
		Slot = Local.Y >= 0.0f ? EWarriorMontage::DodgeRight : EWarriorMontage::DodgeLeft;
	}
	PlayWarriorMontage(Slot);

	UAbilityTask_ApplyRootMotionConstantForce* Force = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(this, NAME_None, Direction,
		DodgeDistance / FMath::Max(DodgeDuration, 0.05f), DodgeDuration, false, nullptr, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 200.0f, true);
	Force->OnFinish.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	Force->ReadyForActivation();
}

void UGA_Dodge::OnDodgeFinished()
{
	FinishAbility();
}

void UGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).RemoveAll(this);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
