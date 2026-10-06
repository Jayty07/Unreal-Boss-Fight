#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BossArenaTypes.generated.h"

class UNiagaraSystem;

UENUM(BlueprintType)
enum class EBossArenaTeam : uint8
{
	Neutral,
	Players,
	Enemies
};

/** Every attack in the encounter is one of these projected shapes. */
UENUM(BlueprintType)
enum class EBossAoEShape : uint8
{
	/** Radius around the origin. */
	Circle,
	/** Frontal wedge: Radius + ConeAngle (full angle). */
	Cone,
	/** Ring between InnerRadius (safe) and Radius. */
	Donut,
	/** Column starting at the origin, extending Length forward, Width wide. */
	Line,
	/** Patch centred on the origin, Length (forward) x Width. */
	Rectangle,
	/** A Line that rotates around its origin at SweepDegreesPerSecond while active. */
	RotatingSweep
};

UENUM(BlueprintType)
enum class EBossAoEPlacement : uint8
{
	/** At the caster, facing its target. Count > 1 fans copies evenly around 360 degrees. */
	Self,
	/** On the highest-threat player. */
	TopThreat,
	RandomPlayer,
	/** One copy under every living player. */
	EachPlayer,
	ArenaCenter
};

UENUM(BlueprintType)
enum class EBossArenaCombatText : uint8
{
	Damage,
	Immune,
	Interrupted,
	Taunted
};

USTRUCT(BlueprintType)
struct BOSSARENA_API FAoEShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE")
	EBossAoEShape Shape = EBossAoEShape::Circle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE", meta = (ClampMin = "0"))
	float Radius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE", meta = (ClampMin = "0"))
	float InnerRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE", meta = (ClampMin = "1", ClampMax = "360"))
	float ConeAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE", meta = (ClampMin = "0"))
	float Length = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE", meta = (ClampMin = "0"))
	float Width = 300.0f;

	float GetBoundingRadius() const;

	/** Scales the dangerous extent (outer radius, length, width). Safe inner radius is kept. */
	FAoEShape Scaled(float Scale) const;

	static FAoEShape MakeCircle(float InRadius);
	static FAoEShape MakeCone(float InRadius, float InAngle);
	static FAoEShape MakeLine(float InLength, float InWidth);
	static FAoEShape MakeRectangle(float InLength, float InWidth);
};

/** Data-table row describing one telegraphed AoE (source: Content/BossArena/Data/BossAoE.csv -> DT_BossAoE). */
USTRUCT(BlueprintType)
struct BOSSARENA_API FBossAoEShapeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE")
	FString CastName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Shape")
	EBossAoEShape Shape = EBossAoEShape::Circle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Shape")
	float Radius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Shape")
	float InnerRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Shape")
	float ConeAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Shape")
	float Length = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Shape")
	float Width = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Placement")
	EBossAoEPlacement Placement = EBossAoEPlacement::Self;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Placement", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Telegraph lead time. Damage resolves when it elapses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Timing", meta = (ClampMin = "0"))
	float CastTime = 2.0f;

	/** Raw damage per hit (per tick for ActiveDuration > 0), before boss Damage multiplier and armor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Damage")
	float Damage = 1000.0f;

	/** If > 0 the AoE stays live after the cast and deals Damage every TickInterval (rotating beams). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Timing", meta = (ClampMin = "0"))
	float ActiveDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Timing", meta = (ClampMin = "0.05"))
	float TickInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Timing")
	float SweepDegreesPerSecond = 0.0f;

	/** Warrior interrupts (Shockwave) cancel the cast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Rules")
	bool bInterruptible = false;

	/** Ignores dodge i-frames, block and armor. Only the hard-enrage wipe uses this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Rules")
	bool bUnblockable = false;

	/** Players without line of sight to the caster (behind a pillar) are not hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Rules")
	bool bRequiresLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Presentation")
	FLinearColor Color = FLinearColor(1.0f, 0.25f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Presentation")
	FString RaidWarning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AoE|Presentation")
	TSoftObjectPtr<UNiagaraSystem> ImpactEffect;

	FAoEShape ToShape() const;
};
