#include "AoE/AoEPersistentZone.h"

#include "AbilitySystemComponent.h"
#include "AoE/AoEShapeMeshComponent.h"
#include "AoE/BossArenaAoELibrary.h"
#include "BossArenaCharacterBase.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AoEPersistentZone)

AAoEPersistentZone::AAoEPersistentZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	ShapeMesh = CreateDefaultSubobject<UAoEShapeMeshComponent>(TEXT("ShapeMesh"));
	ShapeMesh->SetupAttachment(RootComponent);
}

void AAoEPersistentZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAoEPersistentZone, Shape);
	DOREPLIFETIME(AAoEPersistentZone, Color);
	DOREPLIFETIME(AAoEPersistentZone, Duration);
}

AAoEPersistentZone* AAoEPersistentZone::SpawnZone(AActor* InInstigator, UAbilitySystemComponent* InSourceASC, EBossArenaTeam InInstigatorTeam, const FAoEShape& InShape,
	const FTransform& Transform, float InDuration, float InTickInterval, float InDamagePerTick, const FLinearColor& InColor)
{
	if (!InInstigator || !InInstigator->HasAuthority())
	{
		return nullptr;
	}

	UWorld* World = InInstigator->GetWorld();
	AAoEPersistentZone* Zone = World->SpawnActorDeferred<AAoEPersistentZone>(AAoEPersistentZone::StaticClass(), Transform, InInstigator, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Zone)
	{
		return nullptr;
	}

	Zone->Shape = InShape;
	Zone->Color = InColor;
	Zone->Duration = InDuration;
	Zone->SourceASC = InSourceASC;
	Zone->InstigatorTeam = InInstigatorTeam;
	Zone->TickInterval = FMath::Max(0.05f, InTickInterval);
	Zone->DamagePerTick = InDamagePerTick;
	Zone->FinishSpawning(Transform);
	return Zone;
}

void AAoEPersistentZone::BeginPlay()
{
	Super::BeginPlay();
	LocalSpawnTime = GetWorld()->GetTimeSeconds();

	if (HasAuthority())
	{
		SetLifeSpan(Duration);
		GetWorldTimerManager().SetTimer(TickTimer, this, &AAoEPersistentZone::ApplyTick, TickInterval, true);
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		ShapeMesh->SetVisibility(false);
	}
}

void AAoEPersistentZone::ApplyTick()
{
	UAbilitySystemComponent* ASC = SourceASC.Get();
	if (!ASC)
	{
		Destroy();
		return;
	}

	TArray<ABossArenaCharacterBase*> Targets;
	UBossArenaAoELibrary::GatherTargetsInShape(this, Shape, GetActorLocation(), GetActorRotation(), InstigatorTeam, false, GetActorLocation(), Targets);
	UBossArenaAoELibrary::ApplyAoEDamage(ASC, Targets, DamagePerTick, false, this);
}

void AAoEPersistentZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Age = float(GetWorld()->GetTimeSeconds() - LocalSpawnTime);
	const float FadeIn = FMath::Clamp(Age / 0.25f, 0.0f, 1.0f);
	const float FadeOut = FMath::Clamp((Duration - Age) / 0.5f, 0.0f, 1.0f);
	const float Pulse = 0.8f + 0.2f * FMath::Sin(Age * 6.0f);
	const float Alpha = FadeIn * FadeOut;

	ShapeMesh->SetShape(Shape, 1.0f);
	ShapeMesh->SetStyle(Color, 0.0f, 0.8f * Alpha, 0.35f * Pulse * Alpha);
}
