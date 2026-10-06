#include "AoE/BossArenaAoELibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Arena/BossArenaBlockout.h"
#include "BossArenaCharacterBase.h"
#include "BossArenaGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GAS/BossArenaGameplayEffects.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaAoELibrary)

bool UBossArenaAoELibrary::IsPointInShape(const FAoEShape& Shape, const FVector& Origin, const FRotator& Rotation, const FVector& Point, float PointRadius)
{
	FVector Delta = Point - Origin;
	Delta.Z = 0.0f;
	const FVector Local = FRotator(0.0f, Rotation.Yaw, 0.0f).UnrotateVector(Delta);
	const float Dist = FVector2D(Local.X, Local.Y).Size();

	switch (Shape.Shape)
	{
	case EBossAoEShape::Circle:
		return Dist <= Shape.Radius + PointRadius;

	case EBossAoEShape::Donut:
		// The safe centre is generous: you are safe as long as your centre is inside the hole.
		return Dist <= Shape.Radius + PointRadius && Dist >= Shape.InnerRadius;

	case EBossAoEShape::Cone:
	{
		if (Dist > Shape.Radius + PointRadius)
		{
			return false;
		}
		if (Dist <= FMath::Max(PointRadius, 1.0f))
		{
			return true;
		}
		const float AngleToPoint = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Local.Y), Local.X));
		const float EdgeSlack = FMath::RadiansToDegrees(FMath::Asin(FMath::Min(1.0f, PointRadius / Dist)));
		return AngleToPoint <= Shape.ConeAngle * 0.5f + EdgeSlack;
	}

	case EBossAoEShape::Line:
	case EBossAoEShape::RotatingSweep:
		return Local.X >= -PointRadius && Local.X <= Shape.Length + PointRadius && FMath::Abs(Local.Y) <= Shape.Width * 0.5f + PointRadius;

	case EBossAoEShape::Rectangle:
		return FMath::Abs(Local.X) <= Shape.Length * 0.5f + PointRadius && FMath::Abs(Local.Y) <= Shape.Width * 0.5f + PointRadius;
	}

	return false;
}

void UBossArenaAoELibrary::GatherTargetsInShape(const UObject* WorldContextObject, const FAoEShape& Shape, const FVector& Origin, const FRotator& Rotation,
	EBossArenaTeam InstigatorTeam, bool bRequireLineOfSight, const FVector& LOSOrigin, TArray<ABossArenaCharacterBase*>& OutTargets)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossArenaAoE), false);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Shape.GetBoundingRadius() + 200.0f), QueryParams);

	TSet<ABossArenaCharacterBase*> Seen;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ABossArenaCharacterBase* Character = Cast<ABossArenaCharacterBase>(Overlap.GetActor());
		if (!Character || Seen.Contains(Character))
		{
			continue;
		}
		Seen.Add(Character);

		if (!Character->IsAlive() || !AreHostile(InstigatorTeam, Character->GetTeam()))
		{
			continue;
		}

		const FVector TargetLocation = Character->GetActorLocation();
		if (FMath::Abs(TargetLocation.Z - Origin.Z) > MaxHeightDifference)
		{
			continue;
		}

		const float CapsuleRadius = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
		if (!IsPointInShape(Shape, Origin, Rotation, TargetLocation, CapsuleRadius))
		{
			continue;
		}

		if (bRequireLineOfSight)
		{
			FCollisionQueryParams LOSParams(SCENE_QUERY_STAT(BossArenaLOS), false);
			LOSParams.AddIgnoredActor(Character);
			const FVector From = LOSOrigin + FVector(0.0f, 0.0f, 50.0f);
			if (World->LineTraceTestByObjectType(From, TargetLocation, FCollisionObjectQueryParams(ECC_WorldStatic), LOSParams))
			{
				continue;
			}
		}

		OutTargets.Add(Character);
	}
}

int32 UBossArenaAoELibrary::ApplyAoEDamage(UAbilitySystemComponent* SourceASC, const TArray<ABossArenaCharacterBase*>& Targets, float Damage, bool bUnblockable, const UObject* SourceObject)
{
	if (!SourceASC || !SourceASC->IsOwnerActorAuthoritative() || Damage <= 0.0f || Targets.Num() == 0)
	{
		return 0;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(SourceObject);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_BossArenaDamage::StaticClass(), 1.0f, Context);
	if (!Spec.IsValid())
	{
		return 0;
	}
	Spec.Data->SetSetByCallerMagnitude(BossArenaTags::Data_Damage, Damage);
	if (bUnblockable)
	{
		Spec.Data->AddDynamicAssetTag(BossArenaTags::Damage_Unblockable);
	}

	int32 Applied = 0;
	for (ABossArenaCharacterBase* Target : Targets)
	{
		UAbilitySystemComponent* TargetASC = Target ? Target->GetAbilitySystemComponent() : nullptr;
		if (TargetASC && Target->IsAlive())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			++Applied;
		}
	}
	return Applied;
}

void UBossArenaAoELibrary::SendInterrupt(AActor* Instigator, const TArray<ABossArenaCharacterBase*>& Targets)
{
	for (ABossArenaCharacterBase* Target : Targets)
	{
		if (Target)
		{
			FGameplayEventData Payload;
			Payload.EventTag = BossArenaTags::Event_Interrupt;
			Payload.Instigator = Instigator;
			Payload.Target = Target;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, BossArenaTags::Event_Interrupt, Payload);
		}
	}
}

double UBossArenaAoELibrary::GetServerTime(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return 0.0;
	}
	if (const AGameStateBase* GameState = World->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}
	return World->GetTimeSeconds();
}

EBossArenaTeam UBossArenaAoELibrary::GetTeam(const AActor* Actor)
{
	const ABossArenaCharacterBase* Character = Cast<ABossArenaCharacterBase>(Actor);
	return Character ? Character->GetTeam() : EBossArenaTeam::Neutral;
}

bool UBossArenaAoELibrary::AreHostile(EBossArenaTeam A, EBossArenaTeam B)
{
	return A != EBossArenaTeam::Neutral && B != EBossArenaTeam::Neutral && A != B;
}

void UBossArenaAoELibrary::GetLivingPlayers(const UObject* WorldContextObject, TArray<ABossArenaCharacterBase*>& OutPlayers)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}
	for (TActorIterator<ABossArenaCharacterBase> It(World); It; ++It)
	{
		if (It->IsAlive() && It->GetTeam() == EBossArenaTeam::Players)
		{
			OutPlayers.Add(*It);
		}
	}
}

FVector UBossArenaAoELibrary::GetArenaCenter(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World)
	{
		for (TActorIterator<ABossArenaBlockout> It(World); It; ++It)
		{
			return It->GetActorLocation();
		}
	}
	return FVector::ZeroVector;
}

FVector UBossArenaAoELibrary::GetGroundLocation(const AActor* Actor)
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}
	FVector Location = Actor->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Location.Z -= Capsule->GetScaledCapsuleHalfHeight();
		}
	}
	return Location;
}
