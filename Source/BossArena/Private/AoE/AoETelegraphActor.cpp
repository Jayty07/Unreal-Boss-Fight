#include "AoE/AoETelegraphActor.h"

#include "AoE/AoEImpactVFX.h"
#include "AoE/AoEShapeMeshComponent.h"
#include "AoE/BossArenaAoELibrary.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AoETelegraphActor)

AAoETelegraphActor::AAoETelegraphActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetCanBeDamaged(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	ShapeMesh = CreateDefaultSubobject<UAoEShapeMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(RootComponent);
}

void AAoETelegraphActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAoETelegraphActor, Params);
	DOREPLIFETIME(AAoETelegraphActor, BaseYaw);
}

AAoETelegraphActor* AAoETelegraphActor::SpawnTelegraph(UWorld* World, AActor* InOwner, const FAoETelegraphParams& InParams, const FTransform& Transform)
{
	if (!World)
	{
		return nullptr;
	}

	AAoETelegraphActor* Telegraph = World->SpawnActorDeferred<AAoETelegraphActor>(AAoETelegraphActor::StaticClass(), Transform, InOwner, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Telegraph)
	{
		return nullptr;
	}

	Telegraph->Params = InParams;
	Telegraph->BaseYaw = Transform.Rotator().Yaw;
	Telegraph->FinishSpawning(Transform);
	return Telegraph;
}

AAoETelegraphActor* AAoETelegraphActor::SpawnFlash(AActor* InOwner, const FAoEShape& Shape, const FTransform& Transform, const FLinearColor& Color)
{
	if (!InOwner || !InOwner->HasAuthority())
	{
		return nullptr;
	}

	FAoETelegraphParams FlashParams;
	FlashParams.Shape = Shape;
	FlashParams.StartServerTime = UBossArenaAoELibrary::GetServerTime(InOwner);
	FlashParams.Color = Color;
	FlashParams.Style = EAoETelegraphStyle::PlayerFlash;

	AAoETelegraphActor* Flash = SpawnTelegraph(InOwner->GetWorld(), InOwner, FlashParams, Transform);
	if (Flash)
	{
		Flash->SetLifeSpan(0.6f);
	}
	return Flash;
}

void AAoETelegraphActor::BeginPlay()
{
	Super::BeginPlay();
	LocalSpawnTime = GetWorld()->GetTimeSeconds();

	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		ShapeMesh->SetVisibility(false);
		return;
	}
	RefreshVisual();

	if (Params.Style == EAoETelegraphStyle::PlayerFlash)
	{
		AAoEImpactVFX::Spawn(this, Params.Shape, GetActorTransform(), Params.Color, EAoEImpactVFXStyle::PlayerImpact);
	}
}

void AAoETelegraphActor::OnRep_Params()
{
	RefreshVisual();
}

FRotator AAoETelegraphActor::GetRotationAtServerTime(double ServerTime) const
{
	float Yaw = BaseYaw;
	if (Params.Shape.Shape == EBossAoEShape::RotatingSweep && !FMath::IsNearlyZero(Params.SweepDegreesPerSecond))
	{
		const double ActiveStart = Params.StartServerTime + Params.CastTime;
		const double Elapsed = FMath::Clamp(ServerTime - ActiveStart, 0.0, double(Params.ActiveDuration));
		Yaw += float(Elapsed * Params.SweepDegreesPerSecond);
	}
	return FRotator(0.0f, Yaw, 0.0f);
}

void AAoETelegraphActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshVisual();
	UpdateActiveVFX();
}

void AAoETelegraphActor::UpdateActiveVFX()
{
	if (Params.Style != EAoETelegraphStyle::Enemy || Params.ActiveDuration <= 0.0f)
	{
		return;
	}

	const double Now = UBossArenaAoELibrary::GetServerTime(this);
	const double ActiveStart = Params.StartServerTime + Params.CastTime;
	if (Now < ActiveStart || Now > ActiveStart + Params.ActiveDuration)
	{
		return;
	}

	const FTransform LiveTransform(GetRotationAtServerTime(Now), GetActorLocation());
	if (!bPlayedActiveBurst)
	{
		bPlayedActiveBurst = true;
		AAoEImpactVFX::Spawn(this, Params.Shape, LiveTransform, Params.Color, EAoEImpactVFXStyle::BossImpact);
	}

	const double LocalNow = GetWorld()->GetTimeSeconds();
	if (LocalNow >= NextPulseTime)
	{
		NextPulseTime = LocalNow + 0.12;
		AAoEImpactVFX::Spawn(this, Params.Shape, LiveTransform, Params.Color, EAoEImpactVFXStyle::Pulse);
	}
}

void AAoETelegraphActor::RefreshVisual()
{
	if (!ShapeMesh || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const double Now = UBossArenaAoELibrary::GetServerTime(this);
	const double LocalNow = GetWorld()->GetTimeSeconds();

	if (Params.Style == EAoETelegraphStyle::PlayerFlash)
	{
		const float Age = float(LocalNow - LocalSpawnTime);
		const float Fade = FMath::Clamp(1.0f - Age / 0.5f, 0.0f, 1.0f);
		ShapeMesh->SetShape(Params.Shape, 1.0f);
		ShapeMesh->SetStyle(Params.Color, 0.0f, 0.35f * Fade, 0.22f * Fade);
		return;
	}

	const float Progress = Params.CastTime > 0.0f ? FMath::Clamp(float((Now - Params.StartServerTime) / Params.CastTime), 0.0f, 1.0f) : 1.0f;
	const bool bActivePhase = Progress >= 1.0f && Params.ActiveDuration > 0.0f;

	if (Params.Shape.Shape == EBossAoEShape::RotatingSweep)
	{
		SetActorRotation(GetRotationAtServerTime(Now));
	}

	float FillOpacity = 0.45f;
	float AreaOpacity = 0.18f;
	if (bActivePhase)
	{
		// Live beam: pulse so it reads as "this hurts right now".
		FillOpacity = 0.55f + 0.25f * FMath::Sin(float(LocalNow) * 18.0f);
		AreaOpacity = 0.3f;
	}
	if (LocalImpactTime >= 0.0)
	{
		const float Flash = FMath::Clamp(1.0f - float(LocalNow - LocalImpactTime) / 0.3f, 0.0f, 1.0f);
		FillOpacity = FMath::Max(FillOpacity, 0.9f * Flash);
	}

	ShapeMesh->SetShape(Params.Shape, bActivePhase ? 1.0f : Progress);
	ShapeMesh->SetStyle(Params.Color, AreaOpacity, 0.85f, FillOpacity);
}

void AAoETelegraphActor::MulticastImpact_Implementation(UNiagaraSystem* ImpactEffect)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	LocalImpactTime = GetWorld()->GetTimeSeconds();
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation(), GetActorRotation());
	}
	else
	{
		AAoEImpactVFX::Spawn(this, Params.Shape, GetActorTransform(), Params.Color, EAoEImpactVFXStyle::BossImpact);
	}
}
