// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SpatialTestShutdownPreparationTrigger.h"
#include "SpatialFunctionalTestFlowController.h"
#include "SpatialFunctionalTestLBDelegationInterface.h"
#include "EngineClasses/SpatialNetDriver.h"
#include "SpatialGDKServicesConstants.h"
#include "SpatialGDKServicesModule.h"
#include "TestPrepareShutdownListener.h"

ASpatialTestShutdownPreparationTrigger::ASpatialTestShutdownPreparationTrigger()
{
	Author = "Tilman Schmidt";
	Description = TEXT("Trigger shutdown preparation via worker flags and make sure callbacks get called in C++ and Blueprints");
	StepTimer = 0.0f;
	LocalListener = nullptr;
}

void ASpatialTestShutdownPreparationTrigger::BeginPlay()
{
	Super::BeginPlay();
	{ // Step 1 - Test print on all workers
		AddStep(TEXT("AllWorkers_SetupListener"), FWorkerDefinition::AllWorkers, nullptr, [this](ASpatialFunctionalTest* NetTest) {
			UWorld* World = NetTest->GetWorld();

			// Spawn a non-replicated actor that will listen for the shutdown event.
			// Using a non-replicated actor since this is the easiest way to make sure that every worker has exactly one instance of it.
			LocalListener = World->SpawnActor<ATestPrepareShutdownListener>(PrepareShutdownListenerClass, FVector::ZeroVector, FRotator::ZeroRotator);
			NetTest->AssertTrue(IsValid(LocalListener), TEXT("Listener actor is valid."));
			NetTest->RegisterAutoDestroyActor(LocalListener);

			LocalListener->RegisterCallback();
			if(LocalListener->NativePrepareShutdownEventCount != 0 || LocalListener->BlueprintPrepareShutdownEventCount != 0)
			{
				UE_LOG(LogTemp, Log, TEXT("Failing test due to event counts starting out wrong. native: %d, blueprint: %d"), LocalListener->NativePrepareShutdownEventCount, LocalListener->BlueprintPrepareShutdownEventCount);
				NetTest->FinishTest(EFunctionalTestResult::Failed, TEXT("Number of triggered events should start out at 0"));
				return;
			}

			NetTest->FinishStep();
		});

		AddStep(TEXT("Server1_TriggerShutdownPreparation1"), FWorkerDefinition::Server(1), nullptr, [this](ASpatialFunctionalTest* NetTest) {
			FString WorkerFlagSetArgs = TEXT("local worker-flag set UnrealWorker PrepareShutdown Yes --local_service_grpc_port 9876");

			FString WorkerFlagSetResult;
			FString StdErr;
			int32 ExitCode;
			FPlatformProcess::ExecProcess(*SpatialGDKServicesConstants::SpatialExe, *WorkerFlagSetArgs, &ExitCode, &WorkerFlagSetResult, &StdErr, *SpatialGDKServicesConstants::SpatialOSDirectory);

			if(ExitCode != 0)
			{
				NetTest->FinishTest(EFunctionalTestResult::Error, TEXT("Setting the worker flag failed"));
				return;
			}

			NetTest->FinishStep();
		});

		AddStep(TEXT("AllServers_CheckEventHasTriggered"), FWorkerDefinition::AllServers, nullptr, nullptr, [this](ASpatialFunctionalTest* NetTest, float DeltaTime) {
			// On servers, we expect the event to have been triggered
			if (LocalListener->NativePrepareShutdownEventCount == 1 && LocalListener->BlueprintPrepareShutdownEventCount == 1)
			{
				NetTest->FinishStep();
				return;
			}
			// If the count is 0, we might not have received the event yet. We will keep checking by ticking this function.
		});

		AddStep(TEXT("AllClients_CheckEventHasNotTriggered"), FWorkerDefinition::AllClients, nullptr, nullptr, [this](ASpatialFunctionalTest* NetTest, float DeltaTime) {
			// On clients, the event should not be triggered
			if (LocalListener->NativePrepareShutdownEventCount != 0 || LocalListener->BlueprintPrepareShutdownEventCount != 0)
			{
				NetTest->FinishTest(EFunctionalTestResult::Failed, TEXT("The prepare shutdown event was received on a client"));
				return;
			}

			// The callback may take some time to be called on workers after being triggered. So we should wait a while before claiming that it hasn't been called on a client.
			StepTimer += DeltaTime;
			if (StepTimer > EventWaitTime)
			{
				NetTest->FinishStep();
				StepTimer = 0.0f;
			}
		});

		AddStep(TEXT("Server1_TriggerShutdownPreparation2"), FWorkerDefinition::Server(1), nullptr, [this](ASpatialFunctionalTest* NetTest) {
			FString WorkerFlagSetArgs = TEXT("local worker-flag set UnrealWorker PrepareShutdown Other --local_service_grpc_port 9876");

			FString WorkerFlagSetResult;
			FString StdErr;
			int32 ExitCode;
			FPlatformProcess::ExecProcess(*SpatialGDKServicesConstants::SpatialExe, *WorkerFlagSetArgs, &ExitCode, &WorkerFlagSetResult, &StdErr, *SpatialGDKServicesConstants::SpatialOSDirectory);

			if(ExitCode != 0)
			{
				NetTest->FinishTest(EFunctionalTestResult::Error, TEXT("Setting the worker flag failed"));
				return;
			}

			NetTest->FinishStep();
		});

		AddStep(TEXT("AllServers_CheckEventHasTriggeredOnce"), FWorkerDefinition::AllServers, nullptr, nullptr, [this](ASpatialFunctionalTest* NetTest, float DeltaTime) {
			if(LocalListener->NativePrepareShutdownEventCount != 1 || LocalListener->BlueprintPrepareShutdownEventCount != 1)
			{
				NetTest->FinishTest(EFunctionalTestResult::Failed, TEXT("The prepare shutdown event has been received more than once."));
				return;
			}

			StepTimer += DeltaTime;
			if (StepTimer > EventWaitTime)
			{
				NetTest->FinishStep();
				StepTimer = 0.0f;
			}
		});

		AddStep(TEXT("AllClients_CheckEventStillHasNotTriggered"), FWorkerDefinition::AllClients, nullptr, nullptr, [this](ASpatialFunctionalTest* NetTest, float DeltaTime) {
			// On clients, the event should not be triggered
			if (LocalListener->NativePrepareShutdownEventCount != 0 || LocalListener->BlueprintPrepareShutdownEventCount != 0)
			{
				NetTest->FinishTest(EFunctionalTestResult::Failed, TEXT("The prepare shutdown event was received on a client"));
				return;
			}

			// The callback may take some time to be called on workers after being triggered. So we should wait a while before claiming that it hasn't been called on a client.
			StepTimer += DeltaTime;
			if (StepTimer > EventWaitTime)
			{
				NetTest->FinishStep();
				StepTimer = 0.0f;
			}
		});
	}
}
