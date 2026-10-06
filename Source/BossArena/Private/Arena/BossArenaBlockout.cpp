#include "Arena/BossArenaBlockout.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaBlockout)

ABossArenaBlockout::ABossArenaBlockout()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Static);

	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(Root);
	Floor->SetStaticMesh(CylinderFinder.Object);
	Floor->SetMobility(EComponentMobility::Static);
	Floor->SetCollisionProfileName(TEXT("BlockAll"));

	Walls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Walls"));
	Walls->SetupAttachment(Root);
	Walls->SetStaticMesh(CubeFinder.Object);
	Walls->SetMobility(EComponentMobility::Static);
	Walls->SetCollisionProfileName(TEXT("BlockAll"));

	Pillars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Pillars"));
	Pillars->SetupAttachment(Root);
	Pillars->SetStaticMesh(CylinderFinder.Object);
	Pillars->SetMobility(EComponentMobility::Static);
	Pillars->SetCollisionProfileName(TEXT("BlockAll"));

	SpawnMarkers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SpawnMarkers"));
	SpawnMarkers->SetupAttachment(Root);
	SpawnMarkers->SetStaticMesh(CylinderFinder.Object);
	SpawnMarkers->SetMobility(EComponentMobility::Static);
	SpawnMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABossArenaBlockout::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ABossArenaBlockout::Rebuild()
{
	// Engine basic shapes are 100uu, pivot at centre.
	const float FloorThickness = 40.0f;
	Floor->SetRelativeLocation(FVector(0.0f, 0.0f, -FloorThickness * 0.5f));
	Floor->SetRelativeScale3D(FVector((ArenaRadius + WallThickness) * 2.0f / 100.0f, (ArenaRadius + WallThickness) * 2.0f / 100.0f, FloorThickness / 100.0f));

	Walls->ClearInstances();
	const int32 Segments = FMath::Max(8, WallSegments);
	const float WallRadius = ArenaRadius + WallThickness * 0.5f;
	const float SegmentLength = 2.0f * PI * WallRadius / Segments * 1.06f;
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float Angle = 360.0f * Index / Segments;
		const FVector Location = FRotator(0.0f, Angle, 0.0f).Vector() * WallRadius + FVector(0.0f, 0.0f, WallHeight * 0.5f);
		Walls->AddInstance(FTransform(FRotator(0.0f, Angle, 0.0f), Location, FVector(WallThickness / 100.0f, SegmentLength / 100.0f, WallHeight / 100.0f)));
	}

	Pillars->ClearInstances();
	for (int32 Index = 0; Index < PillarCount; ++Index)
	{
		const float Angle = 45.0f + 360.0f * Index / FMath::Max(1, PillarCount);
		const FVector Location = FRotator(0.0f, Angle, 0.0f).Vector() * PillarRingRadius + FVector(0.0f, 0.0f, PillarHeight * 0.5f);
		Pillars->AddInstance(FTransform(FRotator::ZeroRotator, Location, FVector(PillarDiameter / 100.0f, PillarDiameter / 100.0f, PillarHeight / 100.0f)));
	}

	SpawnMarkers->ClearInstances();
	for (int32 Index = 0; Index < NumPlayerSpawns; ++Index)
	{
		FTransform Spawn = GetPlayerSpawnTransform(Index).GetRelativeTransform(GetActorTransform());
		Spawn.SetScale3D(FVector(1.2f, 1.2f, 0.02f));
		Spawn.AddToTranslation(FVector(0.0f, 0.0f, 1.0f));
		SpawnMarkers->AddInstance(Spawn);
	}
}

FTransform ABossArenaBlockout::GetPlayerSpawnTransform(int32 Index) const
{
	const int32 Count = FMath::Max(1, NumPlayerSpawns);
	const float Alpha = Count > 1 ? float(Index % Count) / (Count - 1) : 0.5f;
	const float Angle = 180.0f + FMath::Lerp(-PlayerSpawnArc * 0.5f, PlayerSpawnArc * 0.5f, Alpha);
	const FVector Local = FRotator(0.0f, Angle, 0.0f).Vector() * PlayerSpawnRadius;
	const FRotator FacingCentre(0.0f, Angle + 180.0f, 0.0f);
	return FTransform(FacingCentre, Local) * GetActorTransform();
}

FTransform ABossArenaBlockout::GetBossSpawnTransform() const
{
	// Boss faces the player spawn side.
	return FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector::ZeroVector) * GetActorTransform();
}

void ABossArenaBlockout::GetAddSpawnTransforms(int32 Count, TArray<FTransform>& OutTransforms) const
{
	const float Offset = FMath::FRandRange(0.0f, 360.0f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = Offset + 360.0f * Index / FMath::Max(1, Count);
		OutTransforms.Add(FTransform(FRotator(0.0f, Angle + 180.0f, 0.0f), FRotator(0.0f, Angle, 0.0f).Vector() * AddSpawnRadius) * GetActorTransform());
	}
}
