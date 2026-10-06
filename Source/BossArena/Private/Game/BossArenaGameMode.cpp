#include "Game/BossArenaGameMode.h"

#include "AoE/AoEPersistentZone.h"
#include "AoE/AoETelegraphActor.h"
#include "Arena/BossArenaBlockout.h"
#include "Boss/BossAddCharacter.h"
#include "Boss/BossCharacter.h"
#include "AbilitySystemComponent.h"
#include "BossArenaGameplayTags.h"
#include "BossArenaLog.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/BossArenaGameState.h"
#include "Game/BossArenaPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "UI/BossArenaHUD.h"
#include "Warrior/WarriorCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaGameMode)

ABossArenaGameMode::ABossArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AWarriorCharacter::StaticClass();
	PlayerControllerClass = ABossArenaPlayerController::StaticClass();
	GameStateClass = ABossArenaGameState::StaticClass();
	HUDClass = ABossArenaHUD::StaticClass();
	BossClass = ABossCharacter::StaticClass();
}

ABossArenaGameState* ABossArenaGameMode::GetArenaGameState() const
{
	return GetGameState<ABossArenaGameState>();
}

void ABossArenaGameMode::StartPlay()
{
	EnsureArena();
	EnsureBoss();
	Super::StartPlay();
}

void ABossArenaGameMode::EnsureArena()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!Arena.IsValid())
	{
		for (TActorIterator<ABossArenaBlockout> It(World); It; ++It)
		{
			Arena = *It;
			break;
		}
		if (!Arena.IsValid())
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Arena = World->SpawnActor<ABossArenaBlockout>(ABossArenaBlockout::StaticClass(), FTransform::Identity, Params);
			UE_LOG(LogBossArena, Log, TEXT("No ABossArenaBlockout in level - spawned a default arena."));
		}
	}

	if (PlayerStarts.Num() == 0)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			PlayerStarts.Add(*It);
		}
		if (PlayerStarts.Num() == 0 && Arena.IsValid())
		{
			const float HalfHeight = 96.0f;
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			for (int32 Index = 0; Index < Arena->GetNumPlayerSpawns(); ++Index)
			{
				FTransform Spawn = Arena->GetPlayerSpawnTransform(Index);
				Spawn.AddToTranslation(FVector(0.0f, 0.0f, HalfHeight + 5.0f));
				if (APlayerStart* Start = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Spawn, Params))
				{
					PlayerStarts.Add(Start);
				}
			}
		}
	}
}

void ABossArenaGameMode::EnsureBoss()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!Boss.IsValid())
	{
		for (TActorIterator<ABossCharacter> It(World); It; ++It)
		{
			if (It->IsAlive())
			{
				Boss = *It;
				break;
			}
		}
	}

	if (!Boss.IsValid() && BossClass)
	{
		FTransform Spawn = Arena.IsValid() ? Arena->GetBossSpawnTransform() : FTransform::Identity;
		const ABossCharacter* BossCDO = BossClass->GetDefaultObject<ABossCharacter>();
		Spawn.AddToTranslation(FVector(0.0f, 0.0f, BossCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.0f));
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Boss = World->SpawnActor<ABossCharacter>(BossClass, Spawn, Params);
	}

	if (ABossArenaGameState* ArenaState = GetArenaGameState())
	{
		ArenaState->SetBoss(Boss.Get());
	}
}

AActor* ABossArenaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	EnsureArena();
	PlayerStarts.RemoveAll([](const TObjectPtr<APlayerStart>& Start) { return !IsValid(Start); });
	if (PlayerStarts.Num() > 0)
	{
		return PlayerStarts[NextPlayerStart++ % PlayerStarts.Num()];
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void ABossArenaGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SpawnedAdds.RemoveAll([](const TWeakObjectPtr<ABossAddCharacter>& Add) { return !Add.IsValid(); });

	ABossArenaGameState* ArenaState = GetArenaGameState();
	ABossCharacter* BossPtr = Boss.Get();
	if (!ArenaState || !BossPtr || ArenaState->GetEncounterState() != EBossEncounterState::Waiting)
	{
		return;
	}

	for (TActorIterator<AWarriorCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsAlive() && FVector::Dist2D(It->GetActorLocation(), BossPtr->GetActorLocation()) <= PullRadius)
		{
			StartEncounter();
			return;
		}
	}
}

void ABossArenaGameMode::StartEncounter()
{
	ABossArenaGameState* ArenaState = GetArenaGameState();
	ABossCharacter* BossPtr = Boss.Get();
	if (!ArenaState || !BossPtr || !BossPtr->IsAlive() || ArenaState->GetEncounterState() != EBossEncounterState::Waiting)
	{
		return;
	}

	const int32 NumPlayers = FMath::Max(1, ArenaState->PlayerArray.Num());
	BossPtr->StartEncounter(NumPlayers);
	ArenaState->SetEncounterState(EBossEncounterState::InProgress);
	ArenaState->BroadcastRaidWarning(FString::Printf(TEXT("%s awakens!"), *BossPtr->GetCombatName()), FLinearColor(1.0f, 0.4f, 0.1f), 3.5f);
}

void ABossArenaGameMode::NotifyCharacterDied(ABossArenaCharacterBase* Character)
{
	ABossArenaGameState* ArenaState = GetArenaGameState();
	if (!ArenaState || !Character)
	{
		return;
	}

	if (Character == Boss.Get())
	{
		ArenaState->SetEncounterState(EBossEncounterState::Victory);
		ArenaState->BroadcastRaidWarning(TEXT("VICTORY! Kharzul has fallen."), FLinearColor(0.3f, 1.0f, 0.4f), 8.0f);
		for (const TWeakObjectPtr<ABossAddCharacter>& Add : SpawnedAdds)
		{
			if (Add.IsValid() && Add->IsAlive())
			{
				Add->Die();
			}
		}
		GetWorldTimerManager().SetTimer(ResetTimer, this, &ABossArenaGameMode::ResetEncounter, VictoryResetDelay, false);
		return;
	}

	if (Character->GetTeam() == EBossArenaTeam::Players && ArenaState->GetEncounterState() == EBossEncounterState::InProgress)
	{
		for (TActorIterator<AWarriorCharacter> It(GetWorld()); It; ++It)
		{
			if (It->IsAlive())
			{
				return;
			}
		}
		ArenaState->SetEncounterState(EBossEncounterState::Wipe);
		ArenaState->BroadcastRaidWarning(TEXT("WIPE - resetting encounter..."), FLinearColor(1.0f, 0.1f, 0.1f), WipeResetDelay);
		if (ABossCharacter* BossPtr = Boss.Get())
		{
			BossPtr->GetAbilitySystemComponent()->CancelAllAbilities();
		}
		GetWorldTimerManager().SetTimer(ResetTimer, this, &ABossArenaGameMode::ResetEncounter, WipeResetDelay, false);
	}
}

void ABossArenaGameMode::ResetEncounter()
{
	GetWorldTimerManager().ClearTimer(ResetTimer);
	UWorld* World = GetWorld();

	for (const TWeakObjectPtr<ABossAddCharacter>& Add : SpawnedAdds)
	{
		if (Add.IsValid())
		{
			Add->Destroy();
		}
	}
	SpawnedAdds.Reset();

	for (TActorIterator<AAoETelegraphActor> It(World); It; ++It)
	{
		It->Destroy();
	}
	for (TActorIterator<AAoEPersistentZone> It(World); It; ++It)
	{
		It->Destroy();
	}

	if (ABossCharacter* OldBoss = Boss.Get())
	{
		if (AController* BossController = OldBoss->GetController())
		{
			BossController->Destroy();
		}
		OldBoss->Destroy();
	}
	Boss.Reset();

	if (ABossArenaGameState* ArenaState = GetArenaGameState())
	{
		ArenaState->SetEncounterState(EBossEncounterState::Waiting);
	}
	EnsureBoss();

	NextPlayerStart = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->UnPossess();
			Pawn->Destroy();
		}
		RestartPlayer(PC);
	}

	if (ABossArenaGameState* ArenaState = GetArenaGameState())
	{
		ArenaState->BroadcastRaidWarning(TEXT("Encounter reset. Approach the boss to pull."), FLinearColor::White, 4.0f);
	}
}

int32 ABossArenaGameMode::GetLivingAddCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ABossAddCharacter>& Add : SpawnedAdds)
	{
		Count += (Add.IsValid() && Add->IsAlive()) ? 1 : 0;
	}
	return Count;
}

void ABossArenaGameMode::GetAddSpawnTransforms(int32 Count, TArray<FTransform>& OutTransforms) const
{
	if (Arena.IsValid())
	{
		Arena->GetAddSpawnTransforms(Count, OutTransforms);
	}
}

ABossAddCharacter* ABossArenaGameMode::SpawnAdd(TSubclassOf<ABossAddCharacter> AddClass, const FTransform& GroundTransform)
{
	if (!AddClass)
	{
		return nullptr;
	}
	FTransform Spawn = GroundTransform;
	Spawn.SetScale3D(FVector::OneVector);
	Spawn.AddToTranslation(FVector(0.0f, 0.0f, AddClass->GetDefaultObject<ABossAddCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.0f));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ABossAddCharacter* Add = GetWorld()->SpawnActor<ABossAddCharacter>(AddClass, Spawn, Params);
	if (Add)
	{
		SpawnedAdds.Add(Add);
	}
	return Add;
}
