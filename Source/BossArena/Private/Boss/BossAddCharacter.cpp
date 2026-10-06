#include "Boss/BossAddCharacter.h"

#include "AbilitySystemComponent.h"
#include "Attributes/BossAttributeSet.h"
#include "Boss/Abilities/BossKitAbilities.h"
#include "Boss/BossAIController.h"
#include "Boss/ThreatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossAddCharacter)

ABossAddCharacter::ABossAddCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EBossArenaTeam::Enemies;
	CombatName = TEXT("Ember Spawn");

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	GetCapsuleComponent()->InitCapsuleSize(45.0f, 70.0f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -70.0f), FRotator(0.0f, -90.0f, 0.0f));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 420.0f;

	AIControllerClass = ABossArenaAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetStaticMesh(SphereFinder.Object);
	PlaceholderBody->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.3f));
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ThreatComponent = CreateDefaultSubobject<UThreatComponent>(TEXT("Threat"));
	AddAttributes = CreateDefaultSubobject<UBossAttributeSet>(TEXT("AddAttributes"));
	AddAttributes->InitMaxHealth(6000.0f);
	AddAttributes->InitHealth(6000.0f);

	DefaultAbilities = { UGA_Add_ExplodingPuddle::StaticClass(), UGA_Add_ChargeLine::StaticClass() };
}

void ABossAddCharacter::GetAbilityKit(TArray<TSubclassOf<UGameplayAbility>>& OutKit, float& OutGlobalCooldown) const
{
	OutKit = DefaultAbilities;
	OutGlobalCooldown = GlobalCooldown;
}

void ABossAddCharacter::HandleDamageTaken(float Damage, AActor* DamageInstigator, bool bFatal)
{
	ThreatComponent->AddThreat(DamageInstigator, Damage);
	Super::HandleDamageTaken(Damage, DamageInstigator, bFatal);
}

void ABossAddCharacter::Die()
{
	Super::Die();
	SetLifeSpan(3.0f);
}
