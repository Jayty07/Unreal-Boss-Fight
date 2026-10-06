#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BossArenaPlayerController.generated.h"

/** Player controller with server-routed debug commands for testing phases/enrage in PIE. */
UCLASS()
class BOSSARENA_API ABossArenaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	/** Console: start the pull immediately. */
	UFUNCTION(Exec)
	void StartFight();

	/** Console: reset the encounter. */
	UFUNCTION(Exec)
	void ResetFight();

	/** Console: e.g. "SetBossHealth 0.65" to trigger phase 2. */
	UFUNCTION(Exec)
	void SetBossHealth(float Percent);

	/** Console: e.g. "SkipEncounterTime 475" to see soft enrage, 595 for hard enrage. */
	UFUNCTION(Exec)
	void SkipEncounterTime(float Seconds);

protected:
	UFUNCTION(Server, Reliable)
	void ServerDebugCommand(uint8 Command, float Value);

	bool AreDebugCommandsAllowed() const;
};
