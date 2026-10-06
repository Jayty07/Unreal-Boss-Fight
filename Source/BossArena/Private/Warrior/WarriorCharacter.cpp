#include "Warrior/WarriorCharacter.h"

#include "AbilitySystemComponent.h"
#include "AoE/AoEShapeMeshComponent.h"
#include "Attributes/WarriorAttributeSet.h"
#include "BossArenaGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Warrior/Abilities/GA_Block.h"
#include "Warrior/Abilities/GA_CleaveCombo.h"
#include "Warrior/Abilities/GA_Dodge.h"
#include "Warrior/Abilities/GA_ExecuteSlam.h"
#include "Warrior/Abilities/GA_GroundRend.h"
#include "Warrior/Abilities/GA_LeapSlam.h"
#include "Warrior/Abilities/GA_Shockwave.h"
#include "Warrior/Abilities/GA_TauntingShout.h"
#include "Warrior/Abilities/GA_Whirlwind.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WarriorCharacter)

namespace WarriorCharacterImpl
{
	UInputAction* MakeAction(UObject* Outer, const FName Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name, RF_Transient);
		Action->ValueType = ValueType;
		return Action;
	}

	FWarriorHotbarSlot MakeSlot(EWarriorAbilityInput InputID, TSubclassOf<UWarriorGameplayAbility> AbilityClass, const FKey& Key, const TCHAR* KeyLabel,
		const TCHAR* Label, bool bRepeat = false, bool bShow = true)
	{
		FWarriorHotbarSlot Slot;
		Slot.InputID = InputID;
		Slot.AbilityClass = AbilityClass;
		Slot.Key = Key;
		Slot.KeyLabel = KeyLabel;
		Slot.Label = Label;
		Slot.bRepeatWhileHeld = bRepeat;
		Slot.bShowOnHotbar = bShow;
		return Slot;
	}
}

AWarriorCharacter::AWarriorCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	Team = EBossArenaTeam::Players;
	CombatName = TEXT("Warrior");

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	Movement->MaxWalkSpeed = 600.0f;
	Movement->JumpZVelocity = 600.0f;
	Movement->AirControl = 0.35f;
	Movement->BrakingDecelerationWalking = 2000.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 700.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 160.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -96.0f), FRotator(0.0f, -90.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetStaticMesh(CylinderFinder.Object);
	PlaceholderBody->SetRelativeScale3D(FVector(0.75f, 0.75f, 1.85f));
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Sword = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword"));
	Sword->SetupAttachment(GetMesh(), SwordSocketName);
	Sword->SetStaticMesh(CubeFinder.Object);
	Sword->SetRelativeScale3D(FVector(0.06f, 0.14f, 1.5f));
	Sword->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AimIndicator = CreateDefaultSubobject<UAoEShapeMeshComponent>(TEXT("AimIndicator"));
	AimIndicator->SetupAttachment(RootComponent);
	AimIndicator->SetUsingAbsoluteLocation(true);
	AimIndicator->SetUsingAbsoluteRotation(true);
	AimIndicator->SetVisibility(false);

	WarriorAttributes = CreateDefaultSubobject<UWarriorAttributeSet>(TEXT("WarriorAttributes"));

	using namespace WarriorCharacterImpl;
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::Cleave, UGA_CleaveCombo::StaticClass(), EKeys::LeftMouseButton, TEXT("LMB"), TEXT("Cleave"), true));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::Whirlwind, UGA_Whirlwind::StaticClass(), EKeys::Q, TEXT("Q"), TEXT("Whirlwind")));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::Shockwave, UGA_Shockwave::StaticClass(), EKeys::E, TEXT("E"), TEXT("Shockwave")));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::LeapSlam, UGA_LeapSlam::StaticClass(), EKeys::R, TEXT("R"), TEXT("Leap Slam")));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::GroundRend, UGA_GroundRend::StaticClass(), EKeys::F, TEXT("F"), TEXT("Ground Rend")));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::TauntShout, UGA_TauntingShout::StaticClass(), EKeys::T, TEXT("T"), TEXT("Taunt")));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::Execute, UGA_ExecuteSlam::StaticClass(), EKeys::X, TEXT("X"), TEXT("Execute")));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::Dodge, UGA_Dodge::StaticClass(), EKeys::LeftShift, TEXT("Shift"), TEXT("Dodge")));
	Hotbar.Add(MakeSlot(EWarriorAbilityInput::Block, UGA_Block::StaticClass(), EKeys::RightMouseButton, TEXT("RMB"), TEXT("Block"), false, false));
}

void AWarriorCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWarriorCharacter, bReplicatedHoldToAim);
}

void AWarriorCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdatePlaceholderVisuals();
}

void AWarriorCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdatePlaceholderVisuals();
}

void AWarriorCharacter::UpdatePlaceholderVisuals()
{
	const bool bHasSkeletalMesh = GetMesh() && GetMesh()->GetSkeletalMeshAsset() != nullptr;
	PlaceholderBody->SetVisibility(!bHasSkeletalMesh);

	if (bHasSkeletalMesh)
	{
		if (Sword->GetAttachParent() != GetMesh() || Sword->GetAttachSocketName() != SwordSocketName)
		{
			Sword->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, SwordSocketName);
		}
	}
	else if (Sword->GetAttachParent() != GetCapsuleComponent())
	{
		// Greybox: blade held diagonally across the body so facing and swings read clearly.
		Sword->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Sword->SetRelativeLocationAndRotation(FVector(45.0f, 35.0f, 10.0f), FRotator(-35.0f, 0.0f, 0.0f));
	}
}

void AWarriorCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitAbilitySystem();
}

void AWarriorCharacter::GiveDefaultAbilities()
{
	Super::GiveDefaultAbilities();
	for (const FWarriorHotbarSlot& Slot : Hotbar)
	{
		if (Slot.AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Slot.AbilityClass, 1, static_cast<int32>(Slot.InputID), this));
		}
	}
}

FString AWarriorCharacter::GetCombatName() const
{
	if (const APlayerState* PS = GetPlayerState())
	{
		return PS->GetPlayerName();
	}
	return Super::GetCombatName();
}

float AWarriorCharacter::GetRage() const
{
	return WarriorAttributes ? WarriorAttributes->GetRage() : 0.0f;
}

float AWarriorCharacter::GetMaxRage() const
{
	return WarriorAttributes ? WarriorAttributes->GetMaxRage() : 0.0f;
}

void AWarriorCharacter::EnsureDefaultInput()
{
	using namespace WarriorCharacterImpl;

	if (!MoveAction) { MoveAction = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D); }
	if (!LookAction) { LookAction = MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D); }
	if (!JumpAction) { JumpAction = MakeAction(this, TEXT("IA_Jump"), EInputActionValueType::Boolean); }

	for (const FWarriorHotbarSlot& Slot : Hotbar)
	{
		if (!AbilityActions.Contains(Slot.InputID))
		{
			const FName Name(*FString::Printf(TEXT("IA_%s"), *StaticEnum<EWarriorAbilityInput>()->GetNameStringByValue(static_cast<int64>(Slot.InputID))));
			AbilityActions.Add(Slot.InputID, MakeAction(this, Name, EInputActionValueType::Boolean));
		}
	}

	if (DefaultMappingContext)
	{
		return;
	}

	UInputMappingContext* Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Warrior"), RF_Transient);

	Context->MapKey(MoveAction, EKeys::W).Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
	{
		FEnhancedActionKeyMapping& Back = Context->MapKey(MoveAction, EKeys::S);
		Back.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		Back.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	}
	Context->MapKey(MoveAction, EKeys::D);
	Context->MapKey(MoveAction, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	Context->MapKey(LookAction, EKeys::Mouse2D);
	Context->MapKey(JumpAction, EKeys::SpaceBar);

	for (const FWarriorHotbarSlot& Slot : Hotbar)
	{
		if (Slot.Key.IsValid())
		{
			Context->MapKey(AbilityActions[Slot.InputID], Slot.Key);
		}
	}

	DefaultMappingContext = Context;
}

void AWarriorCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureDefaultInput();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(DefaultMappingContext);
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	ServerSetHoldToAim(bHoldToAim);
}

void AWarriorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureDefaultInput();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogTemp, Error, TEXT("AWarriorCharacter requires EnhancedInputComponent (see Config/DefaultInput.ini)."));
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWarriorCharacter::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWarriorCharacter::HandleLook);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AWarriorCharacter::HandleJump);

	for (const TPair<EWarriorAbilityInput, TObjectPtr<UInputAction>>& Pair : AbilityActions)
	{
		Input->BindAction(Pair.Value, ETriggerEvent::Started, this, &AWarriorCharacter::HandleAbilityPressed, Pair.Key);
		Input->BindAction(Pair.Value, ETriggerEvent::Completed, this, &AWarriorCharacter::HandleAbilityReleased, Pair.Key);
	}
}

void AWarriorCharacter::HandleMove(const FInputActionValue& Value)
{
	if (!Controller || !IsAlive())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AWarriorCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void AWarriorCharacter::HandleJump()
{
	if (IsAlive())
	{
		Jump();
	}
}

void AWarriorCharacter::HandleAbilityPressed(EWarriorAbilityInput InputID)
{
	HeldInputs.Add(InputID);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(InputID));
	}
}

void AWarriorCharacter::HandleAbilityReleased(EWarriorAbilityInput InputID)
{
	HeldInputs.Remove(InputID);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(InputID));
	}
}

void AWarriorCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocallyControlled())
	{
		return;
	}

	// Hold LMB to keep chaining the cleave combo.
	if (AbilitySystemComponent && IsAlive())
	{
		for (const FWarriorHotbarSlot& Slot : Hotbar)
		{
			if (Slot.bRepeatWhileHeld && HeldInputs.Contains(Slot.InputID))
			{
				const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromInputID(static_cast<int32>(Slot.InputID));
				if (Spec && !Spec->IsActive())
				{
					AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(Slot.InputID));
				}
			}
		}
	}

	if (bAimIndicatorVisible && Controller)
	{
		const FRotator AimRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		FVector Feet = GetActorLocation();
		Feet.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		AimIndicator->SetWorldLocationAndRotation(Feet + AimRotation.RotateVector(AimIndicatorOffset), AimRotation);
	}
}

void AWarriorCharacter::ShowAimIndicator(const FAoEShape& Shape, const FVector& LocalOffset, const FLinearColor& Color)
{
	if (!IsLocallyControlled() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	AimIndicatorOffset = LocalOffset;
	AimIndicator->SetShape(Shape, 0.0f);
	AimIndicator->SetStyle(Color, 0.08f, 0.45f, 0.0f);
	AimIndicator->SetVisibility(true);
	bAimIndicatorVisible = true;
}

void AWarriorCharacter::HideAimIndicator()
{
	bAimIndicatorVisible = false;
	if (AimIndicator)
	{
		AimIndicator->SetVisibility(false);
	}
}

void AWarriorCharacter::ToggleHoldToAim()
{
	bHoldToAim = !bHoldToAim;
	ServerSetHoldToAim(bHoldToAim);
	SaveConfig();
}

void AWarriorCharacter::SetMouseSensitivity(float NewSensitivity)
{
	LookSensitivity = FMath::Clamp(NewSensitivity, 0.05f, 5.0f);
	SaveConfig();
}

void AWarriorCharacter::ServerSetHoldToAim_Implementation(bool bEnabled)
{
	bReplicatedHoldToAim = bEnabled;
}

void AWarriorCharacter::OnDeathStateChanged()
{
	Super::OnDeathStateChanged();
	if (!bIsDead)
	{
		return;
	}
	HideAimIndicator();
	HeldInputs.Reset();
	if (PlaceholderBody)
	{
		PlaceholderBody->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -70.0f), FRotator(90.0f, 0.0f, 0.0f));
	}
}
