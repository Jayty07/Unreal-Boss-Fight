#pragma once

#include "CoreMinimal.h"
#include "BossArenaTypes.h"
#include "ProceduralMeshComponent.h"
#include "AoEShapeMeshComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * Flat projected AoE indicator built at runtime: section 0 = area, 1 = outline, 2 = cast-progress fill.
 * Uses /Game/BossArena/Materials/M_Telegraph (translucent, "Color"/"Opacity" params) when present,
 * otherwise falls back to the engine BasicShapeMaterial.
 */
UCLASS(ClassGroup = (BossArena), meta = (BlueprintSpawnableComponent))
class BOSSARENA_API UAoEShapeMeshComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UAoEShapeMeshComponent(const FObjectInitializer& ObjectInitializer);

	/** Rebuilds geometry. FillFraction 0..1 controls the progress fill. */
	UFUNCTION(BlueprintCallable, Category = "BossArena|Telegraph")
	void SetShape(const FAoEShape& InShape, float FillFraction);

	UFUNCTION(BlueprintCallable, Category = "BossArena|Telegraph")
	void SetStyle(const FLinearColor& Color, float AreaOpacity, float OutlineOpacity, float FillOpacity);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossArena|Telegraph")
	TObjectPtr<UMaterialInterface> TelegraphMaterial;

	static const TCHAR* TelegraphMaterialPath;

private:
	void EnsureMaterials();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SectionMaterials;

	FAoEShape CachedShape;
	float CachedFill = -1.0f;
	bool bHasGeometry = false;
	bool bUsesFallbackMaterial = false;
};
