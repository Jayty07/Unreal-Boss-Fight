#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BossArenaTypes.h"
#include "BossAoEGameplayAbility.generated.h"

class AAoETelegraphActor;
class ABossArenaCharacterBase;
class UDataTable;

/**
 * Server-only telegraphed AoE: cast time -> replicated telegraph (decal-style projected mesh, optional
 * Niagara) -> overlap damage at cast end (or every TickInterval for ActiveDuration, e.g. rotating beams).
 * Parameters come from a FBossAoEShapeRow (DT_BossAoE row RowName), falling back to DefaultParams.
 */
UCLASS(Abstract)
class BOSSARENA_API UBossAoEGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBossAoEGameplayAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Row (or DefaultParams) without enrage scaling. */
	FBossAoEShapeRow ResolveParams() const;

	static const TCHAR* DefaultShapeTablePath;

protected:
	/** Where telegraphs go. Default implementation follows ActiveParams.Placement. */
	virtual void ComputePlacements(TArray<FTransform>& OutPlacements) const;

	/** Server: damage everything hostile in Shape at Where. */
	virtual void ResolveAt(const FTransform& Where, const FAoEShape& Shape);

	/** Server: after impact damage (summons, puddles, charges). */
	virtual void OnImpactResolved() {}

	ABossArenaCharacterBase* GetCaster() const;
	AActor* GetPrimaryTarget() const;
	FAoEShape GetScaledShape() const;
	float GetDamageMultiplier() const;

	UFUNCTION()
	void OnCastFinished();

	UFUNCTION()
	void OnInterruptEvent(FGameplayEventData Payload);

	void OnActiveTick();
	void ClearTelegraphs(float Linger);

	UPROPERTY(EditDefaultsOnly, Category = "AoE")
	TSoftObjectPtr<UDataTable> ShapeTable;

	UPROPERTY(EditDefaultsOnly, Category = "AoE")
	FName RowName;

	UPROPERTY(EditDefaultsOnly, Category = "AoE")
	FBossAoEShapeRow DefaultParams;

	/** AI will not reuse this ability until this long after its last activation. */
	UPROPERTY(EditDefaultsOnly, Category = "AoE")
	float MinRecastInterval = 0.0f;

	UPROPERTY(Transient)
	FBossAoEShapeRow ActiveParams;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AAoETelegraphActor>> Telegraphs;

	FAoEShape ActiveShape;
	FTimerHandle ActiveTickTimer;
	double ActiveEndTime = 0.0;
	double LastActivationTime = -1.0e9;
};
