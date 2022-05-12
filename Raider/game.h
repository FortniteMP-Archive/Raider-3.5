#pragma once
#include "ue4.h"

namespace Game
{
    void Start()
    {
		UKismetSystemLibrary* Kismet = reinterpret_cast<UKismetSystemLibrary*>(UKismetSystemLibrary::StaticClass());
		//Kismet->STATIC_ExecuteConsoleCommand(GetWorld(), L"open Athena_Faceoff", GetPlayerController());
        GetPlayerController()->SwitchLevel(L"Athena_Terrain?game=/Game/Athena/Athena_GameMode.Athena_GameMode_C");
        bTraveled = true;
    }

    void OnReadyToStartMatch()
    {
        auto Pawn = SpawnActor<APlayerPawn_Athena_C>({ 0, 0, 10000 }, {});
        Pawn->bCanBeDamaged = false;
        auto PlayerController = (AFortPlayerControllerAthena*)GetEngine()->GameInstance->LocalPlayers[0]->PlayerController;
        PlayerController->Possess(Pawn);

        auto PlayerState = reinterpret_cast<AFortPlayerState*>(Pawn->PlayerState);
        PlayerState->CharacterParts[0] = UObject::FindObject<UCustomCharacterPart>("CustomCharacterPart F_Med_Head1.F_Med_Head1");
        PlayerState->CharacterParts[1] = UObject::FindObject<UCustomCharacterPart>("CustomCharacterPart F_Med_Soldier_01.F_Med_Soldier_01");
        PlayerState->OnRep_CharacterParts();
        PlayerController->CheatManager->God();

        PlayerController->bReadyToStartMatch = true;
        PlayerController->bHasServerFinishedLoading = true;
        PlayerController->bHasClientFinishedLoading = true;
        PlayerController->OnRep_bHasServerFinishedLoading();

        PlayerState->bHasStartedPlaying = true;
        PlayerState->bHasFinishedLoading = true;
        PlayerState->bIsReadyToContinue = true;
        PlayerState->OnRep_bHasStartedPlaying();

        reinterpret_cast<AFortGameStateAthena*>(GetWorld()->GameState)->bGameModeWillSkipAircraft = true;
        reinterpret_cast<AFortGameStateAthena*>(GetWorld()->GameState)->AircraftStartTime = 99999.0f;
        reinterpret_cast<AFortGameStateAthena*>(GetWorld()->GameState)->WarmupCountdownEndTime = 99999.0f;

        reinterpret_cast<AFortGameStateAthena*>(GetWorld()->GameState)->GamePhase = EAthenaGamePhase::Aircraft;
        reinterpret_cast<AFortGameStateAthena*>(GetWorld()->GameState)->OnRep_GamePhase(EAthenaGamePhase::None);

        auto a = reinterpret_cast<UKismetStringLibrary*>(UKismetStringLibrary::StaticClass())->STATIC_Conv_StringToName(L"InProgress");
        reinterpret_cast<AFortGameModeAthena*>(GetWorld()->AuthorityGameMode)->MatchState = a;
        reinterpret_cast<AFortGameModeAthena*>(GetWorld()->AuthorityGameMode)->K2_OnSetMatchState(a);

        reinterpret_cast<AFortGameModeAthena*>(GetWorld()->AuthorityGameMode)->StartPlay();
        ((AAthena_GameState_C*)GetWorld()->GameState)->bReplicatedHasBegunPlay = true;
        ((AAthena_GameState_C*)GetWorld()->GameState)->OnRep_ReplicatedHasBegunPlay();
        reinterpret_cast<AFortGameModeAthena*>(GetWorld()->AuthorityGameMode)->StartMatch();

        if (PlayerController->Pawn)
        {
            if (PlayerController->Pawn->PlayerState)
            {
                auto PlayerState = (AFortPlayerStateAthena*)PlayerController->Pawn->PlayerState;
                PlayerState->TeamIndex = EFortTeam::HumanPvP_Team10;
                PlayerState->OnRep_PlayerTeam();
                PlayerState->SquadId = 1;
                PlayerState->OnRep_SquadId();
            }
        }
    }
}