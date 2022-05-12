#pragma once
#include "game.h"
#include "replication.h"
#include "ue4.h"

namespace Hooks
{
    void TickFlush(UNetDriver* NetDriver, float DeltaSeconds)
    {
        if (!NetDriver)
            return;

        if (NetDriver->IsA(UIpNetDriver::StaticClass()) && NetDriver->ClientConnections.Num() > 0 && NetDriver->ClientConnections[0]->InternalAck == false)
        {
            Replication::ServerReplicateActors(NetDriver, DeltaSeconds);
        }

        Functions::NetDriver::TickFlush(NetDriver, DeltaSeconds);
    }

    void WelcomePlayer(UWorld* World, UNetConnection* IncomingConnection)
    {
        Functions::World::WelcomePlayer(GetWorld(), IncomingConnection);
    }
   
    char KickPlayer(__int64 a1, __int64 a2, __int64 a3)
    {
        return 0;
    }

    void World_NotifyControlMessage(UWorld* World, UNetConnection* Connection, uint8 MessageType, void* Bunch)
    {
        Functions::World::NotifyControlMessage(GetWorld(), Connection, MessageType, Bunch);
    }

    APlayerController* SpawnPlayActor(UWorld* World, UPlayer* NewPlayer, ENetRole RemoteRole, FURL& URL, void* UniqueId, SDK::FString& Error, uint8 NetPlayerIndex)
    {
        auto PlayerController = (AFortPlayerControllerAthena*)Functions::World::SpawnPlayActor(GetWorld(), NewPlayer, RemoteRole, URL, UniqueId, Error, NetPlayerIndex);
        NewPlayer->PlayerController = PlayerController;

        auto PlayerState = (AFortPlayerState*)NewPlayer->PlayerController->PlayerState;
        PlayerState->SetOwner(PlayerController);

        auto Pawn = (APlayerPawn_Athena_C*)SpawnActor<APlayerPawn_Athena_C>({ 0, 0, 10000 }, PlayerController);
        Pawn->bCanBeDamaged = false;
		
        PlayerController->Pawn = Pawn;
        Pawn->Owner = PlayerController;
        Pawn->OnRep_Owner();
        PlayerController->OnRep_Pawn();
        PlayerController->Possess(Pawn);

        PlayerState = (AFortPlayerState*)PlayerController->Pawn->PlayerState;
        PlayerState->CharacterParts[0] = UObject::FindObject<UCustomCharacterPart>("CustomCharacterPart F_Med_Head1.F_Med_Head1");
        PlayerState->CharacterParts[1] = UObject::FindObject<UCustomCharacterPart>("CustomCharacterPart F_Med_Soldier_01.F_Med_Soldier_01");
        PlayerState->OnRep_CharacterParts();

        //PlayerController->CheatManager = SpawnActor<UCheatManager>({ -230, 430, 3030 }, PlayerController);
        //PlayerController->CheatManager->God();

        PlayerController->bHasClientFinishedLoading = true;
        PlayerController->bHasServerFinishedLoading = true;
        PlayerController->OnRep_bHasServerFinishedLoading();

        PlayerState->bHasFinishedLoading = true;
        PlayerState->bHasStartedPlaying = true;
        PlayerState->OnRep_bHasStartedPlaying();
        PlayerState->OnRep_HeroType();

	    auto QuickBars = SpawnActor<AFortQuickBars>({ -280, 400, 3000 }, PlayerController);
        PlayerController->QuickBars = QuickBars;
        PlayerController->OnRep_QuickBar();

        auto Pickaxe = UObject::FindObject<UFortWeaponMeleeItemDefinition>("FortWeaponMeleeItemDefinition WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
       
        static auto Def = UObject::FindObject<UFortWeaponItemDefinition>("WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
        auto TempItemInstance = Def->CreateTemporaryItemInstanceBP(1, 1);
        TempItemInstance->SetOwningControllerForTemporaryItem(PlayerController);

        ((UFortWorldItem*)TempItemInstance)->ItemEntry.Count = 1;

        auto ItemEntry = ((UFortWorldItem*)TempItemInstance)->ItemEntry;
        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Add(ItemEntry);
        PlayerController->QuickBars->ServerAddItemInternal(ItemEntry.ItemGuid, EFortQuickBars::Primary, 0);

        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        PlayerController->HandleWorldInventoryLocalUpdate();
        PlayerController->OnRep_QuickBar();
        PlayerController->QuickBars->OnRep_PrimaryQuickBar();
        PlayerController->QuickBars->OnRep_SecondaryQuickBar();

        Pawn->EquipWeaponDefinition(Def, ItemEntry.ItemGuid);
        /* auto Inventory = SpawnActor<AFortInventory>({ -281, 401, 3001 }, PlayerController);
        PlayerController->WorldInventory = Inventory;
		
        auto Item = Pickaxe->CreateTemporaryItemInstanceBP(1, 0);
        auto WorldItem = reinterpret_cast<SDK::UFortWorldItem*>(Item);
        WorldItem->ItemEntry.Count = 1;
        WorldItem->SetOwningControllerForTemporaryItem(PlayerController);
        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Add(WorldItem->ItemEntry);
        PlayerController->WorldInventory->Inventory.ItemInstances.Add(WorldItem);
        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        PlayerController->HandleWorldInventoryLocalUpdate();
        
        PlayerController->QuickBars = QuickBars;
        PlayerController->OnRep_QuickBar();
        
        PlayerController->AddItemToQuickBars(Pickaxe, EFortQuickBars::Primary, 0);
        PlayerController->ForceUpdateQuickbar(EFortQuickBars::Primary);
        PlayerController->QuickBars->OnRep_PrimaryQuickBar();
        PlayerController->QuickBars->OnRep_SecondaryQuickBar();*/

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

        return PlayerController;
    }

    bool DestroySwappedPC(UWorld* World, UNetConnection* Connection)
    {
        return Functions::World::DestroySwappedPC(GetWorld(), Connection);
    }

    void Beacon_NotifyControlMessage(AOnlineBeaconHost* Beacon, UNetConnection* Connection, uint8 MessageType, void* Bunch)
    {
        printf("Recieved control message %i\n", MessageType);
        if (MessageType == 15)
            return; // PCSwap no thx

        if (MessageType == 5) // PreLogin isnt really needed so we can just use this
        {
            auto _Bunch = reinterpret_cast<int64*>(Bunch);
            _Bunch[7] += (16 * 1024 * 1024);

            FString OnlinePlatformName = FString(L"");

            Functions::NetConnection::ReceiveFString(Bunch, Connection->ClientResponse);
            Functions::NetConnection::ReceiveFString(Bunch, Connection->RequestURL);
            Functions::NetConnection::ReceiveUniqueIdRepl(Bunch, Connection->PlayerID);
            Functions::NetConnection::ReceiveFString(Bunch, OnlinePlatformName);

            _Bunch[7] -= (16 * 1024 * 1024);

            Functions::World::WelcomePlayer(GetWorld(), Connection);
            return;
        }

        Functions::World::NotifyControlMessage(GetWorld(), Connection, MessageType, Bunch);
    }

    uint8 Beacon_NotifyAcceptingConnection(AOnlineBeacon* Beacon)
    {
        return Functions::World::NotifyAcceptingConnection(GetWorld());
    }

    void* SeamlessTravelHandlerForWorld(UEngine* Engine, UWorld* World)
    {
        return Functions::Engine::SeamlessTravelHandlerForWorld(Engine, GetWorld());
    }

    void* NetDebug(UObject* _this)
    {
        return nullptr;
    }

    void Listen()
    {
        printf("[UWorld::Listen]\n");

        AFortOnlineBeaconHost* HostBeacon = SpawnActor<AFortOnlineBeaconHost>();

        Functions::OnlineBeaconHost::InitHost(HostBeacon);

        HostBeacon->NetDriverName = FName(282); // REGISTER_NAME(282,GameNetDriver)
        HostBeacon->NetDriver->NetDriverName = FName(282); // REGISTER_NAME(282,GameNetDriver)
        HostBeacon->NetDriver->World = GetWorld();

        GetWorld()->NetDriver = HostBeacon->NetDriver;
        GetWorld()->LevelCollections[0].NetDriver = HostBeacon->NetDriver;
        GetWorld()->LevelCollections[1].NetDriver = HostBeacon->NetDriver;

        Functions::OnlineBeacon::PauseBeaconRequest(HostBeacon, false);

        DETOUR_START
        DetourAttachE(Functions::World::WelcomePlayer, WelcomePlayer);
        DetourAttachE(Functions::World::NotifyControlMessage, World_NotifyControlMessage);
        DetourAttachE(Functions::World::SpawnPlayActor, SpawnPlayActor);
        DetourAttachE(Functions::OnlineBeaconHost::NotifyControlMessage, Beacon_NotifyControlMessage);
        DetourAttachE(Functions::OnlineSession::KickPlayer, Hooks::KickPlayer);
        DETOUR_END
        return;
    }

    void ProcessEvent(UObject* Object, UFunction* Function, void* Parameters)
    {
        auto ObjectName = Object->GetFullName();
        auto FunctionName = Function->GetFullName();

		if (Function->FunctionFlags & 0x00200000)
        {
            std::cout << "RPC Called: " << FunctionName << std::endl;
        }

        if (bTraveled && FunctionName.find("ReadyToStartMatch") != -1)
        {
            EXECUTE_ONE_TIME
            {
                printf("[UWorld::ProcessEvent]\n\n\n\n\n\n\n\n");
                Game::OnReadyToStartMatch();
                Listen();
            }
        }

        return PEOriginal(Object, Function, Parameters);
    }

}