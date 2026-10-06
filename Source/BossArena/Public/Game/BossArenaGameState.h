#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BossArenaGameState.generated.h"

class ABossCharacter;

UENUM(BlueprintType)
enum class EBossEncounterState : uint8
{
	Waiting,
	InProgress,
	Victory,
	Wipe
};

USTRUCT(BlueprintType)
struct BOSSARENA_API FRaidWarningEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RaidWarning")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "RaidWarning")
	FLinearColor Color = FLinearColor::White;

	/** Local world time. */
	UPROPERTY(BlueprintReadOnly, Category = "RaidWarning")
	double StartTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "RaidWarning")
	float Duration = 3.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidWarning, const FRaidWarningEntry&, Warning);

UCLASS()
class BOSSARENA_API ABossArenaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Encounter")
	ABossCharacter* GetBoss() const { return Boss; }

	void SetBoss(ABossCharacter* InBoss) { Boss = InBoss; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	EBossEncounterState GetEncounterState() const { return EncounterState; }

	void SetEncounterState(EBossEncounterState NewState) { EncounterState = NewState; }

	/** Server: show a raid warning on every client. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Encounter")
	void BroadcastRaidWarning(const FString& Text, FLinearColor Color, float Duration);

	/** Active (non-expired) warnings, newest last. */
	void GetActiveRaidWarnings(TArray<FRaidWarningEntry>& OutWarnings) const;

	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FOnRaidWarning OnRaidWarning;

protected:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRaidWarning(const FString& Text, FLinearColor Color, float Duration);

	UPROPERTY(Replicated)
	TObjectPtr<ABossCharacter> Boss;

	UPROPERTY(Replicated)
	EBossEncounterState EncounterState = EBossEncounterState::Waiting;

	TArray<FRaidWarningEntry> RaidWarnings;
};
