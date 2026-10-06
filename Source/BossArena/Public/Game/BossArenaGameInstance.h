#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BossArenaGameInstance.generated.h"

UCLASS()
class BOSSARENA_API UBossArenaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
