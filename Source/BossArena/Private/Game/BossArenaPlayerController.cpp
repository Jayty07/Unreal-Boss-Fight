#include "Game/BossArenaPlayerController.h"

#include "Boss/BossCharacter.h"
#include "Engine/World.h"
#include "Game/BossArenaGameMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaPlayerController)

namespace
{
	enum class EArenaDebugCommand : uint8
	{
		StartFight,
		ResetFight,
		SetBossHealth,
		SkipTime
	};
}

void ABossArenaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

bool ABossArenaPlayerController::AreDebugCommandsAllowed() const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

void ABossArenaPlayerController::StartFight()
{
	ServerDebugCommand(uint8(EArenaDebugCommand::StartFight), 0.0f);
}

void ABossArenaPlayerController::ResetFight()
{
	ServerDebugCommand(uint8(EArenaDebugCommand::ResetFight), 0.0f);
}

void ABossArenaPlayerController::SetBossHealth(float Percent)
{
	ServerDebugCommand(uint8(EArenaDebugCommand::SetBossHealth), Percent);
}

void ABossArenaPlayerController::SkipEncounterTime(float Seconds)
{
	ServerDebugCommand(uint8(EArenaDebugCommand::SkipTime), Seconds);
}

void ABossArenaPlayerController::ServerDebugCommand_Implementation(uint8 Command, float Value)
{
	ABossArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<ABossArenaGameMode>();
	if (!GameMode || !AreDebugCommandsAllowed())
	{
		return;
	}

	ABossCharacter* Boss = GameMode->GetBoss();
	switch (EArenaDebugCommand(Command))
	{
	case EArenaDebugCommand::StartFight:
		GameMode->StartEncounter();
		break;
	case EArenaDebugCommand::ResetFight:
		GameMode->ResetEncounter();
		break;
	case EArenaDebugCommand::SetBossHealth:
		GameMode->StartEncounter();
		if (Boss)
		{
			Boss->DebugSetHealthPercent(Value);
		}
		break;
	case EArenaDebugCommand::SkipTime:
		GameMode->StartEncounter();
		if (Boss)
		{
			Boss->DebugAdvanceEncounter(Value);
		}
		break;
	}
}
