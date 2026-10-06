#include "Warrior/WarriorAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "BossArenaGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Warrior/WarriorCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WarriorAnimInstance)

void UWarriorAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AWarriorCharacter* Warrior = Cast<AWarriorCharacter>(TryGetPawnOwner());
	if (!Warrior)
	{
		return;
	}

	const FVector Velocity = Warrior->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	bShouldMove = GroundSpeed > 3.0f;

	if (bShouldMove)
	{
		const FRotator Facing = Warrior->GetActorRotation();
		const FVector LocalVelocity = Facing.UnrotateVector(Velocity);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}
	else
	{
		Direction = 0.0f;
	}

	if (const UCharacterMovementComponent* Movement = Warrior->GetCharacterMovement())
	{
		bIsFalling = Movement->IsFalling();
	}

	if (const UAbilitySystemComponent* ASC = Warrior->GetAbilitySystemComponent())
	{
		bIsBlocking = ASC->HasMatchingGameplayTag(BossArenaTags::State_Blocking);
		bIsDodging = ASC->HasMatchingGameplayTag(BossArenaTags::State_Dodging);
		bIsLeaping = ASC->HasMatchingGameplayTag(BossArenaTags::State_Leaping);
	}
	bIsDead = !Warrior->IsAlive();
}
