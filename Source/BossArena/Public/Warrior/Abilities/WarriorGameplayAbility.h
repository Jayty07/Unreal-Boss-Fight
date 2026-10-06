#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BossArenaTypes.h"
#include "Warrior/WarriorAnimSet.h"
#include "WarriorGameplayAbility.generated.h"

class ABossArenaCharacterBase;
class AWarriorCharacter;
class UAnimMontage;

/**
 * Base for the warrior kit. Local-predicted: the owning client plays montages/aim previews
 * immediately, while AoE overlap resolution, damage, rage gain and threat happen only on the server.
 *
 * Default flow: [optional hold-to-aim] -> Commit (rage + cooldown) -> face aim -> montage + faint
 * aim indicator -> after ImpactDelay resolve AimShape at the warrior -> end.
 */
UCLASS(Abstract)
class BOSSARENA_API UWarriorGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWarriorGameplayAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintPure, Category = "Warrior")
	float GetRageCost() const { return RageCost; }

	UFUNCTION(BlueprintPure, Category = "Warrior")
	float GetCooldownDuration() const { return CooldownDuration; }

	/** HUD helper (safe on the CDO): remaining cooldown on ASC; OutDuration = full duration. */
	float GetCooldownRemaining(const UAbilitySystemComponent* ASC, float& OutDuration) const;

	/** Extra situational requirement (Execute: boss below threshold). Safe on the CDO. */
	virtual bool IsUsableNow(const AActor* Avatar) const { return true; }

protected:
	/** Runs after the optional hold-to-aim phase. Must commit and eventually end the ability. */
	virtual void ExecuteAbility();

	/** Resolve the default AimShape at impact time. */
	virtual void PerformImpact();

	/** Server: called with everything a resolved AoE hit. */
	virtual void OnTargetsHit(const TArray<ABossArenaCharacterBase*>& Targets) {}

	/** Commit cost + cooldown; ends (cancelled) on failure. */
	bool CommitOrCancel();
	void FinishAbility(bool bCancelled = false);

	AWarriorCharacter* GetWarrior() const;
	FRotator GetAimRotation() const;
	void FaceAim() const;
	FVector GetFeetLocation() const;
	FVector GetAimOrigin(const FVector& LocalOffset) const;
	float GetScaledDamage(float Base) const;

	/** Server: resolve Shape at Origin/Rotation; applies damage, rage gain, interrupts. Clients: no-op. Returns hit count. */
	int32 ResolveAoE(const FAoEShape& Shape, const FVector& Origin, const FRotator& Rotation, float BaseDamage, bool bInterrupts);

	void ShowAimPreview(const FAoEShape& Shape, const FVector& LocalOffset) const;
	void HideAimPreview() const;
	void PlayWarriorMontage(EWarriorMontage Slot, float PlayRate = 1.0f);
	void WaitThen(float Delay, FName FunctionName);

	UFUNCTION()
	void OnAimReleased(float TimeHeld);

	UFUNCTION()
	void OnImpactTimer();

	/** AoE projected by PerformImpact and shown as the aim indicator. */
	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	FAoEShape AimShape;

	/** Offset of the AoE origin from the warrior's feet, in aim space (X forward). */
	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	FVector AimOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	float BaseDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	float ImpactDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	bool bInterruptsTargets = false;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	FLinearColor IndicatorColor = FLinearColor(0.35f, 0.75f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	bool bShowAimPreview = true;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|AoE")
	bool bSupportsHoldToAim = true;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|Animation")
	EWarriorMontage MontageSlot = EWarriorMontage::None;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|Cost")
	float RageCost = 0.0f;

	/** Rage gained (server) when the AoE hits at least one enemy. */
	UPROPERTY(EditDefaultsOnly, Category = "Warrior|Cost")
	float RageOnHit = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Warrior|Cost")
	float CooldownDuration = 0.0f;

	bool bAiming = false;
};
