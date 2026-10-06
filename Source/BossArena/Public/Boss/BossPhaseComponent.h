#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossPhaseComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
struct FOnAttributeChangeData;

USTRUCT(BlueprintType)
struct BOSSARENA_API FBossPhaseDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FString PhaseName;

	/** Entered when boss health fraction drops to or below this (first phase uses 1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	TArray<TSubclassOf<UGameplayAbility>> AbilityKit;

	/** Fired when the phase begins (room-wide AoE burst). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	TSubclassOf<UGameplayAbility> TransitionAbility;

	/** Delay between the end of one cast and the start of the next. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	float GlobalCooldown = 2.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossPhaseChanged, int32, NewPhaseIndex);

/** Switches the boss kit at health thresholds (default 70% / 40%) and fires transition AoEs. */
UCLASS(ClassGroup = (BossArena), meta = (BlueprintSpawnableComponent))
class BOSSARENA_API UBossPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossPhaseComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: start listening to health and enter phase 0. */
	void StartPhases(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintPure, Category = "Phase")
	int32 GetCurrentPhaseIndex() const { return CurrentPhaseIndex; }

	const FBossPhaseDefinition* GetCurrentPhase() const;

	UFUNCTION(BlueprintPure, Category = "Phase")
	FString GetCurrentPhaseName() const;

	const TArray<FBossPhaseDefinition>& GetPhases() const { return Phases; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	TArray<FBossPhaseDefinition> Phases;

	UPROPERTY(BlueprintAssignable, Category = "Phase")
	FOnBossPhaseChanged OnPhaseChanged;

protected:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void EnterPhase(int32 NewPhaseIndex, bool bPlayTransition);

	UFUNCTION()
	void OnRep_CurrentPhaseIndex();

	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhaseIndex)
	int32 CurrentPhaseIndex = 0;

	TWeakObjectPtr<UAbilitySystemComponent> ASC;
	FDelegateHandle HealthChangedHandle;
};
