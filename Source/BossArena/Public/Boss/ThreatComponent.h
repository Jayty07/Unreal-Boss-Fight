#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ThreatComponent.generated.h"

USTRUCT(BlueprintType)
struct BOSSARENA_API FThreatEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	float Threat = 0.0f;
};

/**
 * Server-authoritative threat table (replicated, sorted descending, for the HUD meter).
 * Damage adds threat; the warrior's Taunting Shout jumps the taunter to the top and fixates.
 */
UCLASS(ClassGroup = (BossArena), meta = (BlueprintSpawnableComponent))
class BOSSARENA_API UThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UThreatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void AddThreat(AActor* Source, float Amount);

	/** Sets Source's threat above the current top + BurstThreat and fixates on it for FixateDuration. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void Taunt(AActor* Source, float BurstThreat, float FixateDuration);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Threat")
	void ResetThreat();

	/** Highest-threat living target (honours taunt fixate on the server). */
	UFUNCTION(BlueprintPure, Category = "Threat")
	AActor* GetTopThreatActor() const;

	UFUNCTION(BlueprintPure, Category = "Threat")
	float GetThreat(const AActor* Source) const;

	const TArray<FThreatEntry>& GetThreatTable() const { return ThreatTable; }

	/** Multiplier applied to damage when converted to threat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat")
	float ThreatPerDamage = 1.0f;

protected:
	static bool IsValidThreatTarget(const AActor* Actor);
	FThreatEntry& FindOrAdd(AActor* Source);
	void SortAndPrune();

	UPROPERTY(Replicated)
	TArray<FThreatEntry> ThreatTable;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> FixateTarget = nullptr;

	double FixateEndTime = 0.0;
};
