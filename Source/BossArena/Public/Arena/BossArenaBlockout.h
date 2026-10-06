#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossArenaBlockout.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Circular greybox arena built from engine basic shapes: floor disc, ring wall, LOS-blocking pillars,
 * plus computed spawn points for players (south edge), the boss (centre) and adds (inner ring).
 * Everything is rebuilt in OnConstruction so the parameters can be tweaked live in the editor.
 */
UCLASS()
class BOSSARENA_API ABossArenaBlockout : public AActor
{
	GENERATED_BODY()

public:
	ABossArenaBlockout();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Arena")
	FTransform GetPlayerSpawnTransform(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Arena")
	FTransform GetBossSpawnTransform() const;

	/** Count points spread around the add ring, randomly rotated each call. Z is floor height. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void GetAddSpawnTransforms(int32 Count, TArray<FTransform>& OutTransforms) const;

	UFUNCTION(BlueprintPure, Category = "Arena")
	float GetArenaRadius() const { return ArenaRadius; }

	UFUNCTION(BlueprintPure, Category = "Arena")
	int32 GetNumPlayerSpawns() const { return NumPlayerSpawns; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena", meta = (ClampMin = "1000"))
	float ArenaRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	float WallHeight = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	float WallThickness = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena", meta = (ClampMin = "8"))
	int32 WallSegments = 48;

	/** Pillars break line of sight for LOS-checked AoEs (phase transition bursts). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|LOS")
	int32 PillarCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|LOS")
	float PillarRingRadius = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|LOS")
	float PillarDiameter = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|LOS")
	float PillarHeight = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawns")
	int32 NumPlayerSpawns = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawns")
	float PlayerSpawnRadius = 2500.0f;

	/** Arc (degrees) the player spawns are spread across, centred on the arena's -X side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawns")
	float PlayerSpawnArc = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Spawns")
	float AddSpawnRadius = 1300.0f;

protected:
	void Rebuild();

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UStaticMeshComponent> Floor;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UInstancedStaticMeshComponent> Walls;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UInstancedStaticMeshComponent> Pillars;

	/** Small floor markers at the player spawn points (visual only). */
	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UInstancedStaticMeshComponent> SpawnMarkers;
};
