#include "AoE/AoEImpactVFX.h"

#include "AoE/AoEShapeMeshComponent.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AoEImpactVFX)

static TAutoConsoleVariable<int32> CVarBossArenaImpactVFX(
	TEXT("BossArena.ImpactVFX"), 1, TEXT("0 = disable the procedural AoE impact effects."));

static TAutoConsoleVariable<float> CVarBossArenaImpactShake(
	TEXT("BossArena.ImpactShake"), 1.0f, TEXT("Camera shake scale for AoE impacts (0 = off)."));

namespace AoEImpactVFXImpl
{
	const TCHAR* BasicShapeMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	float ShapeArea(const FAoEShape& S)
	{
		switch (S.Shape)
		{
		case EBossAoEShape::Circle: return PI * FMath::Square(S.Radius);
		case EBossAoEShape::Donut:  return PI * FMath::Max(0.0f, FMath::Square(S.Radius) - FMath::Square(S.InnerRadius));
		case EBossAoEShape::Cone:   return PI * FMath::Square(S.Radius) * S.ConeAngle / 360.0f;
		default:                    return S.Length * S.Width;
		}
	}

	/** Uniform random point inside the shape, in the shape's local space (X = forward). */
	FVector SamplePoint(const FAoEShape& S, FRandomStream& Random)
	{
		const float HalfW = S.Width * 0.5f;
		switch (S.Shape)
		{
		case EBossAoEShape::Circle:
		case EBossAoEShape::Donut:
		case EBossAoEShape::Cone:
		{
			const float Inner = S.Shape == EBossAoEShape::Donut ? S.InnerRadius : 0.0f;
			const float R = FMath::Sqrt(FMath::Lerp(FMath::Square(Inner), FMath::Square(S.Radius), Random.FRand()));
			const float HalfAngle = S.Shape == EBossAoEShape::Cone ? S.ConeAngle * 0.5f : 180.0f;
			const float A = FMath::DegreesToRadians(Random.FRandRange(-HalfAngle, HalfAngle));
			return FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.0f);
		}
		case EBossAoEShape::Line:
		case EBossAoEShape::RotatingSweep:
			return FVector(Random.FRandRange(0.0f, S.Length), Random.FRandRange(-HalfW, HalfW), 0.0f);
		case EBossAoEShape::Rectangle:
			return FVector(Random.FRandRange(-S.Length * 0.5f, S.Length * 0.5f), Random.FRandRange(-HalfW, HalfW), 0.0f);
		}
		return FVector::ZeroVector;
	}

	/** 0 at the origin, 1 at the far edge: drives the outward propagation delay. */
	float NormalizedDistance(const FAoEShape& S, const FVector& P)
	{
		switch (S.Shape)
		{
		case EBossAoEShape::Line:
		case EBossAoEShape::RotatingSweep:
			return FMath::Clamp(P.X / FMath::Max(1.0f, S.Length), 0.0f, 1.0f);
		case EBossAoEShape::Rectangle:
			return FMath::Clamp(FMath::Abs(P.X) / FMath::Max(1.0f, S.Length * 0.5f), 0.0f, 1.0f);
		default:
			return FMath::Clamp(float(P.Size2D()) / FMath::Max(1.0f, S.Radius), 0.0f, 1.0f);
		}
	}

	FVector OutwardDirection(const FAoEShape& S, const FVector& P, FRandomStream& Random)
	{
		if (S.Shape == EBossAoEShape::Line || S.Shape == EBossAoEShape::RotatingSweep || S.Shape == EBossAoEShape::Rectangle)
		{
			const float Side = P.Y >= 0.0f ? 1.0f : -1.0f;
			return FVector(Random.FRandRange(-0.4f, 0.6f), Side, 0.0f).GetSafeNormal();
		}
		const FVector Out = P.GetSafeNormal2D();
		if (!Out.IsNearlyZero())
		{
			return Out;
		}
		const float A = Random.FRandRange(0.0f, 2.0f * PI);
		return FVector(FMath::Cos(A), FMath::Sin(A), 0.0f);
	}
}

AAoEImpactVFX::AAoEImpactVFX()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	FlashMesh = CreateDefaultSubobject<UAoEShapeMeshComponent>(TEXT("FlashMesh"));
	FlashMesh->SetupAttachment(RootComponent);
	FlashMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));

	WaveMesh = CreateDefaultSubobject<UAoEShapeMeshComponent>(TEXT("WaveMesh"));
	WaveMesh->SetupAttachment(RootComponent);
	WaveMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));

	RingMesh = CreateDefaultSubobject<UAoEShapeMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(RootComponent);
	RingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	Light->IntensityUnits = ELightUnits::Candelas;
	Light->SetIntensity(0.0f);
	Light->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	CubeMesh = CubeFinder.Object;
	CylinderMesh = CylinderFinder.Object;
	SphereMesh = SphereFinder.Object;
}

AAoEImpactVFX* AAoEImpactVFX::Spawn(const UObject* WorldContextObject, const FAoEShape& InShape, const FTransform& Transform, const FLinearColor& InColor,
	EAoEImpactVFXStyle InStyle)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer || CVarBossArenaImpactVFX.GetValueOnGameThread() == 0)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	const FTransform Flat(FRotator(0.0f, Transform.Rotator().Yaw, 0.0f), Transform.GetLocation());
	AAoEImpactVFX* VFX = World->SpawnActor<AAoEImpactVFX>(AAoEImpactVFX::StaticClass(), Flat, SpawnParams);
	if (VFX)
	{
		VFX->Initialize(InShape, InColor, InStyle);
	}
	return VFX;
}

void AAoEImpactVFX::Initialize(const FAoEShape& InShape, const FLinearColor& InColor, EAoEImpactVFXStyle InStyle)
{
	using namespace AoEImpactVFXImpl;

	Shape = InShape;
	Color = InColor;
	Style = InStyle;

	const float Bounding = FMath::Max(50.0f, Shape.GetBoundingRadius());
	Scale = FMath::Clamp(Bounding / 600.0f, 0.5f, 4.0f);
	FRandomStream Random(FMath::Rand());

	UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, UAoEShapeMeshComponent::TelegraphMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!GlowBase)
	{
		bGlowFallback = true;
		GlowBase = LoadObject<UMaterialInterface>(nullptr, BasicShapeMaterialPath);
	}
	GlowMaterial = GlowBase ? UMaterialInstanceDynamic::Create(GlowBase, this) : nullptr;

	if (UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(nullptr, BasicShapeMaterialPath))
	{
		DebrisMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
		DebrisMaterial->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(FLinearColor(0.05f, 0.045f, 0.04f), Color, 0.2f));
	}

	const float Area = ShapeArea(Shape);
	int32 PillarCount = 0;
	int32 DebrisCount = 0;
	int32 SparkCount = 0;
	float Spread = 0.0f;

	switch (Style)
	{
	case EAoEImpactVFXStyle::BossImpact:
		Duration = 1.8f;
		Spread = 0.3f;
		PillarCount = FMath::Clamp(FMath::RoundToInt(Area / 70000.0f), 6, 32);
		DebrisCount = FMath::Clamp(FMath::RoundToInt(Area / 45000.0f), 8, 36);
		SparkCount = FMath::Clamp(FMath::RoundToInt(Area / 35000.0f), 10, 40);
		PeakLight = 2500.0f * Scale;
		break;
	case EAoEImpactVFXStyle::PlayerImpact:
		Duration = 1.1f;
		Spread = 0.15f;
		PillarCount = FMath::Clamp(FMath::RoundToInt(Area / 150000.0f), 2, 6);
		DebrisCount = FMath::Clamp(FMath::RoundToInt(Area / 90000.0f), 3, 12);
		SparkCount = FMath::Clamp(FMath::RoundToInt(Area / 40000.0f), 8, 24);
		PeakLight = 900.0f * Scale;
		break;
	case EAoEImpactVFXStyle::Pulse:
		Duration = 0.9f;
		PillarCount = Random.RandRange(0, 1);
		SparkCount = Random.RandRange(3, 6);
		break;
	}

	const bool bLayers = Style != EAoEImpactVFXStyle::Pulse;
	const bool bRing = bLayers && (Shape.Shape == EBossAoEShape::Circle || Shape.Shape == EBossAoEShape::Donut);
	FlashMesh->SetVisibility(bLayers);
	WaveMesh->SetVisibility(bLayers);
	RingMesh->SetVisibility(bRing);
	if (bLayers)
	{
		FlashMesh->SetShape(Shape, 0.0f);
	}

	Light->SetVisibility(PeakLight > 0.0f);
	Light->SetAttenuationRadius(Bounding * 1.4f + 400.0f);
	Light->SetLightColor(FMath::Lerp(Color, FLinearColor::White, 0.3f));

	auto AddMany = [&](EAoEImpactPieceKind Kind, int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Point = SamplePoint(Shape, Random);
			AddPiece(Kind, Point, NormalizedDistance(Shape, Point) * Spread + Random.FRandRange(0.0f, 0.06f), Random);
		}
	};
	AddMany(EAoEImpactPieceKind::Pillar, PillarCount);
	AddMany(EAoEImpactPieceKind::Debris, DebrisCount);
	AddMany(EAoEImpactPieceKind::Spark, SparkCount);

	ApplyCameraShake();
	SetLifeSpan(Duration + Spread + 0.1f);
	UpdateLayers();
	UpdatePieces(0.0f);
}

void AAoEImpactVFX::AddPiece(EAoEImpactPieceKind Kind, const FVector& LocalPoint, float Delay, FRandomStream& Random)
{
	UStaticMesh* Mesh = Kind == EAoEImpactPieceKind::Pillar ? CylinderMesh.Get() : (Kind == EAoEImpactPieceKind::Debris ? CubeMesh.Get() : SphereMesh.Get());
	if (!Mesh)
	{
		return;
	}

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetGenerateOverlapEvents(false);
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetCastShadow(Kind == EAoEImpactPieceKind::Debris);
	Comp->SetMaterial(0, Kind == EAoEImpactPieceKind::Debris ? DebrisMaterial.Get() : GlowMaterial.Get());
	Comp->SetupAttachment(RootComponent);
	Comp->SetVisibility(false);
	Comp->RegisterComponent();
	PieceComponents.Add(Comp);

	const float SizeScale = FMath::Sqrt(Scale) * (Style == EAoEImpactVFXStyle::BossImpact ? 1.0f : 0.6f);
	const FVector Out = AoEImpactVFXImpl::OutwardDirection(Shape, LocalPoint, Random);

	FAoEImpactPiece Piece;
	Piece.Kind = Kind;
	Piece.Delay = Delay;
	Piece.Location = LocalPoint;

	switch (Kind)
	{
	case EAoEImpactPieceKind::Pillar:
	{
		const float Diameter = Random.FRandRange(25.0f, 60.0f) * SizeScale;
		const float Height = Random.FRandRange(180.0f, 520.0f) * SizeScale;
		Piece.BaseScale = FVector(Diameter / 100.0f, Diameter / 100.0f, Height / 100.0f);
		Piece.Rotation = FRotator(Random.FRandRange(-8.0f, 8.0f), Random.FRandRange(0.0f, 360.0f), Random.FRandRange(-8.0f, 8.0f));
		Piece.Life = Random.FRandRange(0.45f, 0.7f);
		break;
	}
	case EAoEImpactPieceKind::Debris:
	{
		const float Size = Random.FRandRange(10.0f, 32.0f) * SizeScale;
		Piece.BaseScale = FVector(Size / 100.0f);
		Piece.Location.Z = Size * 0.5f;
		Piece.Velocity = Out * Random.FRandRange(150.0f, 450.0f) + FVector(0.0f, 0.0f, Random.FRandRange(450.0f, 950.0f));
		Piece.Rotation = FRotator(Random.FRandRange(0.0f, 360.0f), Random.FRandRange(0.0f, 360.0f), Random.FRandRange(0.0f, 360.0f));
		Piece.Spin = FRotator(Random.FRandRange(-540.0f, 540.0f), Random.FRandRange(-540.0f, 540.0f), Random.FRandRange(-540.0f, 540.0f));
		Piece.Life = Random.FRandRange(0.9f, 1.3f);
		break;
	}
	case EAoEImpactPieceKind::Spark:
	{
		const float Size = Random.FRandRange(5.0f, 12.0f);
		const float MaxUp = Style == EAoEImpactVFXStyle::Pulse ? 600.0f : 1300.0f;
		Piece.BaseScale = FVector(Size / 100.0f);
		Piece.Location.Z = 10.0f;
		Piece.Velocity = Out * Random.FRandRange(100.0f, 500.0f) + FVector(0.0f, 0.0f, Random.FRandRange(250.0f, MaxUp));
		Piece.Life = Random.FRandRange(0.5f, 0.9f);
		break;
	}
	}

	Pieces.Add(Piece);
}

void AAoEImpactVFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	UpdateLayers();
	UpdatePieces(DeltaSeconds);
}

void AAoEImpactVFX::UpdateLayers()
{
	if (GlowMaterial)
	{
		const FLinearColor GlowColor = bGlowFallback ? Color : FMath::Lerp(FLinearColor::White, Color, FMath::Clamp(Age / 0.25f, 0.0f, 1.0f)) * 1.6f;
		GlowMaterial->SetVectorParameterValue(TEXT("Color"), GlowColor);
		GlowMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.85f * FMath::Clamp(1.0f - (Age - 0.4f) / FMath::Max(0.1f, Duration - 0.4f), 0.0f, 1.0f));
	}

	if (PeakLight > 0.0f)
	{
		const float LightAlpha = FMath::Clamp(1.0f - Age / 0.6f, 0.0f, 1.0f);
		Light->SetIntensity(PeakLight * LightAlpha * LightAlpha);
	}

	if (Style == EAoEImpactVFXStyle::Pulse)
	{
		return;
	}

	const float Strength = Style == EAoEImpactVFXStyle::BossImpact ? 1.0f : 0.6f;
	const FLinearColor Hot = FMath::Lerp(FLinearColor::White, Color, FMath::Clamp(Age / 0.18f, 0.0f, 1.0f));

	// White-hot flash of the whole area.
	const float FlashAlpha = FMath::Clamp(1.0f - Age / 0.4f, 0.0f, 1.0f);
	FlashMesh->SetStyle(Hot, 0.75f * FlashAlpha * Strength, 0.9f * FlashAlpha, 0.0f);

	// Wavefront sweeping from the origin to the far edge (cone/line/rect read as a directional blast).
	const float WaveT = FMath::Clamp(Age / 0.28f, 0.0f, 1.0f);
	const float WaveAlpha = 1.0f - FMath::Clamp((Age - 0.2f) / 0.35f, 0.0f, 1.0f);
	WaveMesh->SetShape(Shape, 1.0f - FMath::Pow(1.0f - WaveT, 3.0f));
	WaveMesh->SetStyle(Hot, 0.0f, 0.0f, 0.55f * WaveAlpha * Strength);

	// Expanding shockwave ring; for donuts it starts at the safe edge so the centre stays clean.
	if (RingMesh->IsVisible())
	{
		const float RingT = FMath::Clamp(Age / 0.55f, 0.0f, 1.0f);
		const float Ease = 1.0f - FMath::Square(1.0f - RingT);
		const float Start = Shape.Shape == EBossAoEShape::Donut ? Shape.InnerRadius : Shape.Radius * 0.15f;
		const float Radius = FMath::Lerp(Start, Shape.Radius * 1.12f, Ease);
		const float Thickness = FMath::Max(30.0f, Shape.Radius * 0.06f) * (1.0f - 0.5f * RingT);

		FAoEShape Ring;
		Ring.Shape = EBossAoEShape::Donut;
		Ring.Radius = Radius;
		Ring.InnerRadius = FMath::Max(0.0f, Radius - Thickness);
		RingMesh->SetShape(Ring, 0.0f);
		RingMesh->SetStyle(Hot, 0.8f * (1.0f - RingT) * Strength, 0.0f, 0.0f);
		if (RingT >= 1.0f)
		{
			RingMesh->SetVisibility(false);
		}
	}
}

void AAoEImpactVFX::UpdatePieces(float DeltaSeconds)
{
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		UStaticMeshComponent* Comp = PieceComponents.IsValidIndex(Index) ? PieceComponents[Index].Get() : nullptr;
		if (!Comp)
		{
			continue;
		}

		FAoEImpactPiece& Piece = Pieces[Index];
		const float T = Age - Piece.Delay;
		if (T < 0.0f || T > Piece.Life)
		{
			Comp->SetVisibility(false);
			continue;
		}

		const float N = T / Piece.Life;
		FVector PieceScale = Piece.BaseScale;
		FVector Location = Piece.Location;

		switch (Piece.Kind)
		{
		case EAoEImpactPieceKind::Pillar:
		{
			// Erupt out of the ground, then thin out.
			const float Rise = 1.0f - FMath::Square(1.0f - FMath::Clamp(T / 0.1f, 0.0f, 1.0f));
			const float Thin = 1.0f - FMath::SmoothStep(0.4f, 1.0f, N);
			PieceScale.X *= Thin;
			PieceScale.Y *= Thin;
			PieceScale.Z *= Rise * (1.0f + 0.25f * N);
			Location.Z = PieceScale.Z * 50.0f;
			break;
		}
		case EAoEImpactPieceKind::Debris:
		{
			Piece.Velocity.Z -= 1800.0f * DeltaSeconds;
			Piece.Location += Piece.Velocity * DeltaSeconds;
			const float Half = Piece.BaseScale.X * 50.0f;
			if (Piece.Location.Z < Half)
			{
				Piece.Location.Z = Half;
				if (Piece.Velocity.Z < 0.0f)
				{
					Piece.Velocity.Z *= -0.3f;
					Piece.Velocity.X *= 0.5f;
					Piece.Velocity.Y *= 0.5f;
					Piece.Spin *= 0.5f;
				}
			}
			Piece.Rotation += Piece.Spin * DeltaSeconds;
			PieceScale *= 1.0f - FMath::SmoothStep(0.75f, 1.0f, N);
			Location = Piece.Location;
			break;
		}
		case EAoEImpactPieceKind::Spark:
		{
			Piece.Velocity.Z -= 700.0f * DeltaSeconds;
			Piece.Velocity *= FMath::Max(0.0f, 1.0f - 1.5f * DeltaSeconds);
			Piece.Location += Piece.Velocity * DeltaSeconds;
			PieceScale *= 1.0f - N;
			Location = Piece.Location;
			break;
		}
		}

		Comp->SetRelativeLocationAndRotation(Location, Piece.Rotation);
		Comp->SetRelativeScale3D(PieceScale.ComponentMax(FVector(0.001f)));
		Comp->SetVisibility(true);
	}
}

void AAoEImpactVFX::ApplyCameraShake() const
{
	const float ShakeScale = CVarBossArenaImpactShake.GetValueOnGameThread();
	UWorld* World = GetWorld();
	if (!World || ShakeScale <= 0.0f || Style == EAoEImpactVFXStyle::Pulse)
	{
		return;
	}

	const float Base = Style == EAoEImpactVFXStyle::BossImpact ? FMath::Clamp(0.35f + Scale * 0.25f, 0.4f, 1.0f) : 0.2f;
	const float ShakeDuration = Style == EAoEImpactVFXStyle::BossImpact ? 0.7f : 0.3f;
	const float Bounding = Shape.GetBoundingRadius();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController() || !PC->PlayerCameraManager)
		{
			continue;
		}

		const FVector ViewLocation = PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : PC->PlayerCameraManager->GetCameraLocation();
		const float Distance = float(FVector::Dist2D(ViewLocation, GetActorLocation()));
		const float Falloff = FMath::Clamp(1.0f - FMath::Max(0.0f, Distance - Bounding) / 2500.0f, 0.0f, 1.0f);
		if (Falloff <= 0.0f)
		{
			continue;
		}

		UCameraModifier* Modifier = PC->PlayerCameraManager->FindCameraModifierByClass(UBossArenaImpactShakeModifier::StaticClass());
		if (!Modifier)
		{
			Modifier = PC->PlayerCameraManager->AddNewCameraModifier(UBossArenaImpactShakeModifier::StaticClass());
		}
		if (UBossArenaImpactShakeModifier* Shake = Cast<UBossArenaImpactShakeModifier>(Modifier))
		{
			Shake->AddShake(Base * Falloff * ShakeScale, ShakeDuration);
		}
	}
}

void UBossArenaImpactShakeModifier::AddShake(float Strength, float Duration)
{
	Strength = FMath::Clamp(Strength, 0.0f, 1.0f);
	if (Strength > Trauma)
	{
		Trauma = Strength;
		DecayPerSecond = Trauma / FMath::Max(0.05f, Duration);
	}
}

bool UBossArenaImpactShakeModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
	if (Trauma <= 0.0f)
	{
		return false;
	}

	NoiseTime += DeltaTime * 28.0f;
	const float Amount = Trauma * Trauma;
	InOutPOV.Rotation.Pitch += 2.5f * Amount * FMath::PerlinNoise1D(NoiseTime);
	InOutPOV.Rotation.Yaw += 2.5f * Amount * FMath::PerlinNoise1D(NoiseTime + 37.1f);
	InOutPOV.Rotation.Roll += 1.5f * Amount * FMath::PerlinNoise1D(NoiseTime + 71.7f);
	InOutPOV.Location += FVector(FMath::PerlinNoise1D(NoiseTime + 13.3f), FMath::PerlinNoise1D(NoiseTime + 51.9f), FMath::PerlinNoise1D(NoiseTime + 89.4f))
		* 10.0f * Amount;

	Trauma = FMath::Max(0.0f, Trauma - DecayPerSecond * DeltaTime);
	return false;
}
