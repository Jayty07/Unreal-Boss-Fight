#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_Dodge.generated.h"

/**
 * Dodge: quick dash/roll in the movement-input direction (backstep with no input) with i-frames.
 *
 * Networking: the owning client sends its chosen direction as target data. The server validates it
 * (normalized, horizontal), applies UGE_DodgeInvulnerability on its own ASC (authoritative i-frames)
 * and runs the same root-motion force. Cooldown is enforced by GAS on both sides.
 */
UCLASS()
class BOSSARENA_API UGA_Dodge : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	float GetInvulnerabilityDuration() const { return InvulnerabilityDuration; }

protected:
	FVector ComputeLocalDirection() const;
	FVector SanitizeDirection(const FVector& Requested) const;
	void StartDodge(const FVector& Direction);
	void OnServerTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag);

	UFUNCTION()
	void OnDodgeFinished();

	UFUNCTION()
	void OnTargetDataTimeout();

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeDistance = 650.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeDuration = 0.35f;

	/** Authoritative i-frame window, starting when the dodge starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float InvulnerabilityDuration = 0.45f;

	bool bDodgeStarted = false;
};
