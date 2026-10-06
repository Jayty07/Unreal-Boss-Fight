#pragma once

#include "CoreMinimal.h"
#include "BossArenaTypes.h"
#include "Camera/CameraModifier.h"
#include "GameFramework/Actor.h"
#include "AoEImpactVFX.generated.h"

class UAoEShapeMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EAoEImpactVFXStyle : uint8
{
	/** Boss/add AoE resolving: white-hot flash, shockwave, erupting pillars, debris, light, camera shake. */
	BossImpact,
	/** Warrior AoE resolving: lighter flash, sparks and a small shake. */
	PlayerImpact,
	/** Small ambient burst for ticking areas (live rotating beam, lingering zones). */
	Pulse
};

enum class EAoEImpactPieceKind : uint8
{
	Pillar,
	Debris,
	Spark
};

struct FAoEImpactPiece
{
	EAoEImpactPieceKind Kind = EAoEImpactPieceKind::Spark;
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FRotator Spin = FRotator::ZeroRotator;
	FVector BaseScale = FVector::OneVector;
	float Delay = 0.0f;
	float Life = 1.0f;
};

/**
 * Asset-free, client-local impact effect shaped like the AoE that triggered it. Built from the
 * projected shape mesh, engine basic shapes and a point light, so it works before any Niagara exists.
 * Never replicated: every client spawns its own copy from an already replicated trigger.
 * Disable with "BossArena.ImpactVFX 0"; scale shake with "BossArena.ImpactShake 0..2".
 */
UCLASS(NotBlueprintable)
class BOSSARENA_API AAoEImpactVFX : public AActor
{
	GENERATED_BODY()

public:
	AAoEImpactVFX();

	virtual void Tick(float DeltaSeconds) override;

	/** Client only (no-op on dedicated servers). Transform is the AoE origin and facing. */
	static AAoEImpactVFX* Spawn(const UObject* WorldContextObject, const FAoEShape& Shape, const FTransform& Transform, const FLinearColor& Color,
		EAoEImpactVFXStyle Style);

protected:
	void Initialize(const FAoEShape& InShape, const FLinearColor& InColor, EAoEImpactVFXStyle InStyle);
	void AddPiece(EAoEImpactPieceKind Kind, const FVector& LocalPoint, float Delay, FRandomStream& Random);
	void UpdateLayers();
	void UpdatePieces(float DeltaSeconds);
	void ApplyCameraShake() const;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UAoEShapeMeshComponent> FlashMesh;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UAoEShapeMeshComponent> WaveMesh;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UAoEShapeMeshComponent> RingMesh;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DebrisMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PieceComponents;

	TArray<FAoEImpactPiece> Pieces;

	FAoEShape Shape;
	FLinearColor Color = FLinearColor::White;
	EAoEImpactVFXStyle Style = EAoEImpactVFXStyle::BossImpact;
	float Age = 0.0f;
	float Duration = 1.6f;
	float Scale = 1.0f;
	float PeakLight = 0.0f;
	bool bGlowFallback = false;
};

/** Trauma-style camera shake applied by impacts near the local player. No shake asset required. */
UCLASS()
class BOSSARENA_API UBossArenaImpactShakeModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	void AddShake(float Strength, float Duration);

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	float Trauma = 0.0f;
	float DecayPerSecond = 1.5f;
	float NoiseTime = 0.0f;
};
