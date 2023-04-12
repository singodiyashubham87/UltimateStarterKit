﻿// Created by Henry Jooste

#include "Menu.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "MenuItem.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "USK/Audio/AudioUtils.h"
#include "USK/Logger/Log.h"

/**
 * @brief Overridable native event for when the widget has been constructed
 */
void UMenu::NativeConstruct()
{
	Super::NativeConstruct();
	
	InitializeMenuItems();
	if (CurrentMenuItem != nullptr)
	{
		CurrentMenuItem->SetHighlightedState(true, false, false);
	}

	USK_LOG_TRACE("Adding binding to visibility changed event");
	OnNativeVisibilityChanged.AddUObject(this, &UMenu::OnMenuVisibilityChanged);
}

/**
 * @brief Overridable native event for when the widget has been destroyed
 */
void UMenu::NativeDestruct()
{
	Super::NativeDestruct();
	RemoveInputBindings();
}

/**
 * @brief Overridable native event for when the widget has been initialized
 */
void UMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (AddInputBindingOnLoad)
	{
		AddInputBindings();
	}
}

/**
 * @brief Navigate up or increase the value
 */
void UMenu::OnMenuUp()
{
	USK_LOG_TRACE("Navigating up");
	UpdateHighlightedItemBeforeNavigation(true);
	UpdateHighlightedItem(CurrentMenuItem == nullptr ? nullptr : CurrentMenuItem->MenuItemUp,
		CurrentMenuItem == nullptr ? EMenuNavigation::Disabled : CurrentMenuItem->VerticalNavigation,
		true, false);
}

/**
 * @brief Increase the value while holding the menu up key
 */
void UMenu::OnMenuUpHold()
{
	if (CurrentMenuItem == nullptr || CurrentMenuItem->ValueUpdateMethod != EMenuItemValueUpdateMethod::Hold ||
		CurrentMenuItem->VerticalNavigation != EMenuNavigation::IncreaseDecreaseValue)
	{
		return;
	}
	
	USK_LOG_TRACE("Increasing value (hold)");
	UpdateHighlightedItem(nullptr, EMenuNavigation::IncreaseDecreaseValue, true, true);
}

/**
 * @brief Navigate down or decrease the value
 */
void UMenu::OnMenuDown()
{
	USK_LOG_TRACE("Navigating down");
	UpdateHighlightedItemBeforeNavigation(true);
	UpdateHighlightedItem(CurrentMenuItem == nullptr ? nullptr : CurrentMenuItem->MenuItemDown,
		CurrentMenuItem == nullptr ? EMenuNavigation::Disabled : CurrentMenuItem->VerticalNavigation,
		false, false);
}

/**
 * @brief Decrease the value while holding the menu down key
 */
void UMenu::OnMenuDownHold()
{
	if (CurrentMenuItem == nullptr || CurrentMenuItem->ValueUpdateMethod != EMenuItemValueUpdateMethod::Hold ||
		CurrentMenuItem->VerticalNavigation != EMenuNavigation::IncreaseDecreaseValue)
	{
		return;
	}
	
	USK_LOG_TRACE("Decreasing value (hold)");
	UpdateHighlightedItem(nullptr, EMenuNavigation::IncreaseDecreaseValue, false, true);
}

/**
 * @brief Navigate left or decrease the value
 */
void UMenu::OnMenuLeft()
{
	USK_LOG_TRACE("Navigating left");
	UpdateHighlightedItemBeforeNavigation(false);
	UpdateHighlightedItem(CurrentMenuItem == nullptr ? nullptr : CurrentMenuItem->MenuItemLeft,
		CurrentMenuItem == nullptr ? EMenuNavigation::Disabled : CurrentMenuItem->HorizontalNavigation,
		false, false);
}

/**
 * @brief Decrease the value while holding the menu left key
 */
void UMenu::OnMenuLeftHold()
{
	if (CurrentMenuItem == nullptr || CurrentMenuItem->ValueUpdateMethod != EMenuItemValueUpdateMethod::Hold ||
		CurrentMenuItem->HorizontalNavigation != EMenuNavigation::IncreaseDecreaseValue)
	{
		return;
	}
	
	USK_LOG_TRACE("Decreasing value (hold)");
	UpdateHighlightedItem(nullptr, EMenuNavigation::IncreaseDecreaseValue, false, true);
}

/**
 * @brief Navigate right or increase the value
 */
void UMenu::OnMenuRight()
{
	USK_LOG_TRACE("Navigating right");
	UpdateHighlightedItemBeforeNavigation(false);
	UpdateHighlightedItem(CurrentMenuItem == nullptr ? nullptr : CurrentMenuItem->MenuItemRight,
		CurrentMenuItem == nullptr ? EMenuNavigation::Disabled : CurrentMenuItem->HorizontalNavigation,
		true, false);
}

/**
 * @brief Increase the value while holding the menu right key
 */
void UMenu::OnMenuRightHold()
{
	if (CurrentMenuItem == nullptr || CurrentMenuItem->ValueUpdateMethod != EMenuItemValueUpdateMethod::Hold ||
		CurrentMenuItem->HorizontalNavigation != EMenuNavigation::IncreaseDecreaseValue)
	{
		return;
	}
	
	USK_LOG_TRACE("Increasing value (hold)");
	UpdateHighlightedItem(nullptr, EMenuNavigation::IncreaseDecreaseValue, true, true);
}

/**
 * @brief Select the current menu item
 */
void UMenu::OnMenuSelected()
{
	USK_LOG_TRACE("Selecting menu item");	
	if (CurrentMenuItem != nullptr && CurrentMenuItem->AllowSelection)
	{
		UAudioUtils::PlaySound2D(GetWorld(), SelectedSFX);
		CurrentMenuItem->OnSelectedEvent.Broadcast();
	}
}

/**
 * @brief Go back to a previous menu or close the menu
 */
void UMenu::OnMenuBack()
{
	USK_LOG_TRACE("Going back");
	UAudioUtils::PlaySound2D(GetWorld(), BackSFX);
	OnBackEvent.Broadcast();
}

/**
 * @brief Request to highlight a specific menu item
 * @param MenuItem The menu item to highlight
 */
void UMenu::RequestHighlight(UMenuItem* MenuItem)
{
	if (CurrentMenuItem == MenuItem)
	{
		USK_LOG_TRACE("Menu item already highlighted");
		return;
	}

	USK_LOG_TRACE("Handling highlight request");
	UpdateHighlightedItem(MenuItem, EMenuNavigation::HighlightItem, false, false);
}

/**
 * @brief Request to remove the highlighted state from a specific menu item
 * @param MenuItem The menu item to remove the highlighted state from
 */
void UMenu::RemoveHighlight(UMenuItem* MenuItem)
{
	if (CurrentMenuItem != MenuItem)
	{
		USK_LOG_TRACE("Menu item not highlighted");
		return;
	}

	USK_LOG_TRACE("Removing highlight from menu item");
	HighlightedMenuItemBeforeRemoval = CurrentMenuItem;
	UpdateHighlightedItem(nullptr, EMenuNavigation::HighlightItem, false, false);
}

/**
 * @brief Called when the visibility of the menu is changed
 * @param NewVisibility The new visibility of the menu
 */
void UMenu::OnMenuVisibilityChanged(ESlateVisibility NewVisibility)
{
	USK_LOG_TRACE("Visibility changed");
	if (NewVisibility == ESlateVisibility::Visible)
	{
		AddInputBindings();
		return;
	}

	RemoveInputBindings();
}

/**
 * @brief Add the input bindings for the menu
 */
void UMenu::AddInputBindings()
{
	if (PauseGameWhileVisible)
	{
		USK_LOG_INFO("Pausing game");
		UGameplayStatics::SetGamePaused(GetWorld(), true);
	}
	
	if (InputMappingContext == nullptr)
	{
		USK_LOG_WARNING("Unable to add input binding. Input mapping context is nullptr");
		return;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
	{
		USK_LOG_ERROR("Unable to add input binding. Player controller is nullptr");
		return;
	}
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (Subsystem == nullptr)
	{
		USK_LOG_ERROR("Unable to add input binding. Enhanced input subsystem not found");
		return;
	}

	USK_LOG_TRACE("Adding menu input mapping context");
	Subsystem->RemoveMappingContext(InputMappingContext);
	Subsystem->AddMappingContext(InputMappingContext, 0);
	InitializeActionBindings(PlayerController);
}

/**
 * @brief Remove the input bindings for the menu
 */
void UMenu::RemoveInputBindings() const
{
	if (PauseGameWhileVisible)
	{
		USK_LOG_INFO("Resuming game");
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}
	
	if (InputMappingContext == nullptr)
	{
		USK_LOG_WARNING("Unable to remove input binding. Input mapping context is nullptr");
		return;
	}
	
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
	{
		USK_LOG_ERROR("Unable to remove input binding. Player controller is nullptr");
		return;
	}
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (Subsystem == nullptr)
	{
		USK_LOG_ERROR("Unable to remove input binding. Enhanced input subsystem not found");
		return;
	}

	USK_LOG_TRACE("Removing menu input mapping context");
	Subsystem->RemoveMappingContext(InputMappingContext);
}

/**
 * @brief Initialize the action input bindings for the menu
 * @param PlayerController A reference to the current player controller
 */
void UMenu::InitializeActionBindings(const APlayerController* PlayerController)
{
	if (bIsActionBindingsInitialized)
	{
		USK_LOG_TRACE("Action bindings already initialized");
		return;
	}

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerController->GetPawn()->InputComponent);
	if (EnhancedInput == nullptr)
	{
		USK_LOG_ERROR("Unable to add input binding. Enhanced input component not found");
		return;
	}

	USK_LOG_TRACE("Adding input action bindings");
	EnhancedInput->BindAction(MenuUpInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuUp);
	EnhancedInput->BindAction(MenuUpInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuUpHold);
	EnhancedInput->BindAction(MenuDownInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuDown);
	EnhancedInput->BindAction(MenuDownInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuDownHold);
	EnhancedInput->BindAction(MenuLeftInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuLeft);
	EnhancedInput->BindAction(MenuLeftInputAction, ETriggerEvent::Triggered, this, &UMenu::OnMenuLeftHold);
	EnhancedInput->BindAction(MenuRightInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuRight);
	EnhancedInput->BindAction(MenuRightInputAction, ETriggerEvent::Triggered, this, &UMenu::OnMenuRightHold);
	EnhancedInput->BindAction(MenuSelectInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuSelected);
	EnhancedInput->BindAction(MenuBackInputAction, ETriggerEvent::Started, this, &UMenu::OnMenuBack);
	bIsActionBindingsInitialized = true;
}

/**
 * @brief Initialize the menu items and highlight the default item
 */
void UMenu::InitializeMenuItems()
{
	USK_LOG_TRACE("Initializing default menu item");
	TArray<UWidget*> Children;
	WidgetTree->GetAllWidgets(Children);
	UMenuItem* FallbackMenuItem = nullptr;

	for (UWidget* Widget : Children)
	{
		if (!Widget->IsA(UMenuItem::StaticClass()))
		{
			continue;
		}
		
		UMenuItem* MenuItem = dynamic_cast<UMenuItem*>(Widget);
		if (MenuItem == nullptr)
		{
			continue;
		}

		MenuItem->Menu = this;
		if (CurrentMenuItem != nullptr)
		{
			continue;
		}
		
		if (FallbackMenuItem == nullptr)
		{
			FallbackMenuItem = MenuItem;
		}
		
		if (!MenuItem->FocusByDefault)
		{
			continue;
		}

		CurrentMenuItem = MenuItem;
	}

	if (CurrentMenuItem == nullptr)
	{
		USK_LOG_WARNING("No default menu item found");
		CurrentMenuItem = FallbackMenuItem;
	}
}

/**
 * @brief Update the highlighted item
 * @param NewItem The new item that should be highlighted
 * @param MenuNavigation The type of navigation used
 * @param IncreaseValue Should the value of the menu item be increased?
 * @param IsHolding Is the key being held down?
 */
void UMenu::UpdateHighlightedItem(UMenuItem* NewItem, const EMenuNavigation MenuNavigation, const bool IncreaseValue,
	const bool IsHolding)
{
	switch (MenuNavigation)
	{
	case EMenuNavigation::Disabled:
		break;
	case EMenuNavigation::HighlightItem:
		if (NewItem != nullptr && !NewItem->IsVisible())
		{
			break;
		}

		if (CurrentMenuItem != nullptr)
		{
			CurrentMenuItem->SetHighlightedState(false, false, false);	
		}
		
		CurrentMenuItem = NewItem;
		if (NewItem != nullptr)
		{
			NewItem->SetHighlightedState(true, true, true);
			if (ScrollContainer != nullptr)
			{
				ScrollContainer->ScrollWidgetIntoView(CurrentMenuItem);
			}
		}
		break;
	case EMenuNavigation::IncreaseDecreaseValue:
		if (CurrentMenuItem != nullptr &&
			(CurrentMenuItem->ValueUpdateMethod != EMenuItemValueUpdateMethod::Hold ||
			(CurrentMenuItem->ValueUpdateMethod == EMenuItemValueUpdateMethod::Hold && IsHolding)))
		{
			const float Value = IsHolding ? CurrentMenuItem->IncrementHold : CurrentMenuItem->IncrementSinglePress;
			CurrentMenuItem->UpdateValue(IncreaseValue ? Value : -Value);
		}
		break;
	}
}

/**
 * @brief Update the highlighted menu item before the navigation event
 * @param IsVerticalNavigation Is this called because of a vertical navigation event?
 */
void UMenu::UpdateHighlightedItemBeforeNavigation(const bool IsVerticalNavigation)
{
	if (CurrentMenuItem != nullptr || HighlightedMenuItemBeforeRemoval == nullptr)
	{
		return;
	}

	const EMenuNavigation ItemNavigation = IsVerticalNavigation
		? HighlightedMenuItemBeforeRemoval->VerticalNavigation
		: HighlightedMenuItemBeforeRemoval->HorizontalNavigation;
	if (ItemNavigation != EMenuNavigation::HighlightItem)
	{
		return;
	}

	CurrentMenuItem = HighlightedMenuItemBeforeRemoval;
	CurrentMenuItem->SetHighlightedState(true, false, false);
	HighlightedMenuItemBeforeRemoval = nullptr;
}
