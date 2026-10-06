#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BossArenaGameMode.generated.h"

class ABossAddCharacter;
class ABossArenaBlockout;
class ABossArenaCharacterBase;
class ABossCharacter;
class APlayerStart;

/**
 * Server-only encounter flow: ensures an arena + boss exist, hands out spawn points, starts the pull
 * (boss damaged or a player walks into PullRadius), detects victory/wipe and resets the encounter.
 */
UCLASS()
class BOSSARENA_API ABossArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABossArenaGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void StartEncounter();

	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void ResetEncounter();

	void NotifyCharacterDied(ABossArenaCharacterBase* Character);

	int32 GetLivingAddCount() const;
	void GetAddSpawnTransforms(int32 Count, TArray<FTransform>& OutTransforms) const;
	ABossAddCharacter* SpawnAdd(TSubclassOf<ABossAddCharacter> AddClass, const FTransform& GroundTransform);

	ABossCharacter* GetBoss() const { return Boss.Get(); }
	ABossArenaBlockout* GetArena() const { return Arena.Get(); }

	UPROPERTY(EditDefaultsOnly, Category = "Encounter")
	TSubclassOf<ABossCharacter> BossClass;

	/** Encounter auto-starts when a living player gets this close to the boss. */
	UPROPERTY(EditDefaultsOnly, Category = "Encounter")
	float PullRadius = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Encounter")
	float VictoryResetDelay = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Encounter")
	float WipeResetDelay = 8.0f;

protected:
	void EnsureArena();
	void EnsureBoss();
	class ABossArenaGameState* GetArenaGameState() const;

	TWeakObjectPtr<ABossArenaBlockout> Arena;
	TWeakObjectPtr<ABossCharacter> Boss;
	TArray<TWeakObjectPtr<ABossAddCharacter>> SpawnedAdds;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> PlayerStarts;

	int32 NextPlayerStart = 0;
	FTimerHandle ResetTimer;
};
