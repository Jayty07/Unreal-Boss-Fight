#pragma once

#include "CoreMinimal.h"
#include "BossArenaCharacterBase.h"
#include "BossCharacter.generated.h"

class UBossAttributeSet;
class UBossPhaseComponent;
class UGameplayAbility;
class UStaticMeshComponent;
class UThreatComponent;

USTRUCT(BlueprintType)
struct BOSSARENA_API FBossCastInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cast")
	FString CastName;

	UPROPERTY(BlueprintReadOnly, Category = "Cast")
	double StartServerTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Cast")
	float Duration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cast")
	bool bInterruptible = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cast")
	bool bCasting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cast")
	bool bWasInterrupted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cast")
	double EndServerTime = 0.0;
};

/**
 * Raid boss. All attacks are UBossAoEGameplayAbility telegraphs; kit is driven by UBossPhaseComponent,
 * targets by UThreatComponent. Soft enrage scales damage/radius; hard enrage casts an unblockable wipe.
 */
UCLASS()
class BOSSARENA_API ABossCharacter : public ABossArenaCharacterBase
{
	GENERATED_BODY()

public:
	ABossCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleDamageTaken(float Damage, AActor* DamageInstigator, bool bFatal) override;
	virtual void Die() override;

	/** Server: scale health to party size, start enrage clock and phase 0. */
	void StartEncounter(int32 NumPlayers);

	/** Server: called by boss AoE abilities to drive the replicated cast bar. */
	void StartCast(const FString& CastName, float Duration, bool bInterruptible);
	void EndCast(bool bInterrupted);

	/** Abilities the AI may pick right now plus the delay between casts. */
	void GetActiveKit(TArray<TSubclassOf<UGameplayAbility>>& OutKit, float& OutGlobalCooldown) const;

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsEncounterActive() const { return bEncounterActive; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsSoftEnraged() const { return bSoftEnraged; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsHardEnraged() const { return bHardEnraged; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetEncounterElapsed() const;

	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetSoftEnrageTime() const { return SoftEnrageTime; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetHardEnrageTime() const { return HardEnrageTime; }

	/** AoE size multiplier (soft enrage). */
	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetAoERadiusScale() const { return bSoftEnraged ? SoftEnrageRadiusMultiplier : 1.0f; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetEnrageTimerRemaining() const;

	/** Server debug helpers (BossArenaPlayerController exec commands). */
	void DebugAdvanceEncounter(float Seconds);
	void DebugSetHealthPercent(float Percent);

	const FBossCastInfo& GetCastInfo() const { return CastInfo; }
	UThreatComponent* GetThreatComponent() const { return ThreatComponent; }
	UBossPhaseComponent* GetPhaseComponent() const { return PhaseComponent; }

protected:
	virtual void GiveDefaultAbilities() override;
	virtual void OnDeathStateChanged() override;
	void TriggerSoftEnrage();
	void TriggerHardEnrage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UThreatComponent> ThreatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBossPhaseComponent> PhaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	/** Greybox "face" so players can read the boss's facing for frontal cones. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderFacing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UBossAttributeSet> BossAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Health")
	float BaseMaxHealth = 150000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Health")
	float HealthPerExtraPlayer = 120000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float SoftEnrageTime = 480.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float HardEnrageTime = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float SoftEnrageDamageMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	float SoftEnrageRadiusMultiplier = 1.2f;

	/** Unblockable arena-wide wipe, recast until everyone is dead. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage")
	TSubclassOf<UGameplayAbility> HardEnrageAbility;

	UPROPERTY(Replicated)
	FBossCastInfo CastInfo;

	UPROPERTY(Replicated)
	bool bEncounterActive = false;

	UPROPERTY(Replicated)
	double EncounterStartServerTime = 0.0;

	UPROPERTY(Replicated)
	bool bSoftEnraged = false;

	UPROPERTY(Replicated)
	bool bHardEnraged = false;

	float EnrageAttributeAccumulator = 0.0f;
};
