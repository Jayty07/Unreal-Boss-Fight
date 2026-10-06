#pragma once

#include "CoreMinimal.h"
#include "BossArenaCharacterBase.h"
#include "InputCoreTypes.h"
#include "WarriorCharacter.generated.h"

class UAoEShapeMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
class UWarriorAnimSet;
class UWarriorAttributeSet;
class UWarriorGameplayAbility;
struct FInputActionValue;
struct FAoEShape;

/** InputID used to bind GAS abilities to Enhanced Input. */
UENUM(BlueprintType)
enum class EWarriorAbilityInput : uint8
{
	None = 0,
	Cleave,
	Whirlwind,
	Shockwave,
	LeapSlam,
	GroundRend,
	TauntShout,
	Execute,
	Dodge,
	Block
};

USTRUCT(BlueprintType)
struct BOSSARENA_API FWarriorHotbarSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotbar")
	EWarriorAbilityInput InputID = EWarriorAbilityInput::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotbar")
	TSubclassOf<UWarriorGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotbar")
	FKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotbar")
	FString KeyLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotbar")
	FString Label;

	/** Keep re-activating while held (cleave combo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotbar")
	bool bRepeatWhileHeld = false;

	/** Shown in the HUD hotbar (block is hold-only, so it is hidden). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hotbar")
	bool bShowOnHotbar = true;
};

/**
 * Two-handed sword warrior. Every damaging ability is a projected AoE resolved on the server.
 * ASC lives on the pawn (Mixed replication); abilities are bound to Enhanced Input via InputID.
 */
UCLASS(Config = Game)
class BOSSARENA_API AWarriorCharacter : public ABossArenaCharacterBase
{
	GENERATED_BODY()

public:
	AWarriorCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnRep_Controller() override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual FString GetCombatName() const override;

	UFUNCTION(BlueprintPure, Category = "Warrior")
	float GetRage() const;

	UFUNCTION(BlueprintPure, Category = "Warrior")
	float GetMaxRage() const;

	UFUNCTION(BlueprintPure, Category = "Warrior")
	UWarriorAnimSet* GetAnimSet() const { return AnimSet; }

	const TArray<FWarriorHotbarSlot>& GetHotbar() const { return Hotbar; }

	UFUNCTION(BlueprintPure, Category = "Warrior")
	bool IsHoldToAimEnabled() const { return bReplicatedHoldToAim; }

	/** Local-only faint aim indicator for the warrior's own AoEs. */
	void ShowAimIndicator(const FAoEShape& Shape, const FVector& LocalOffset, const FLinearColor& Color);
	void HideAimIndicator();

	/** Exec: toggle hold-to-aim (press = preview, release = cast). */
	UFUNCTION(Exec)
	void ToggleHoldToAim();

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bHoldToAim = false;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookSensitivity = 1.0f;

protected:
	virtual void GiveDefaultAbilities() override;
	virtual void OnDeathStateChanged() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void EnsureDefaultInput();
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJump();
	void HandleAbilityPressed(EWarriorAbilityInput InputID);
	void HandleAbilityReleased(EWarriorAbilityInput InputID);
	void UpdatePlaceholderVisuals();

	UFUNCTION(Server, Reliable)
	void ServerSetHoldToAim(bool bEnabled);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Two-handed sword, attached to SwordSocketName on the skeletal mesh. Swap the mesh in BP_Warrior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Sword;

	/** Greybox body, only visible while no skeletal mesh is assigned. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAoEShapeMeshComponent> AimIndicator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UWarriorAttributeSet> WarriorAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FName SwordSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UWarriorAnimSet> AnimSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<FWarriorHotbarSlot> Hotbar;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TMap<EWarriorAbilityInput, TObjectPtr<UInputAction>> AbilityActions;

	UPROPERTY(Replicated)
	bool bReplicatedHoldToAim = false;

	TSet<EWarriorAbilityInput> HeldInputs;
	FVector AimIndicatorOffset = FVector::ZeroVector;
	bool bAimIndicatorVisible = false;
};
