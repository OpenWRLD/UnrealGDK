// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SpatialSnapshotTest.h"
#include "SpatialSnapshotTestActor.h"

/**
 * This test handles SpatialOS Snapshots.
 *
 * Here's what you should expect:
 *		- ASpatialSnapshotTest is in a map with testing mode ForceSpatial
 *		- ASpatialSnapshotTest will be expected to run 2 times, the first setting up the data and taking the Snapshot
 *		and the second time checking that the data from the Snapshot is properly loaded and clearing the Snapshot.
 *		- ASpatialSnapshotDummyTest is in another map with testing mode ForceNative.
 *		- ASpatialSnapshotDummyTest will be expected to run 2 times, each time just passing.
 *
 * Caveats that you should be aware to understand the way this needs to be setup:
 *		- (1) Currently passing a Snapshot to a deployment is only allowed at launch time.
 *		- (2) UE Automation Manager only loads maps / launches SpatialOS deployments whenever it wants to run tests in different maps.
 *		- (3) A Snapshot taken from a specific map is only guaranteed to be valid for that same map.
 *		- (4) UE Automation Manager only loads maps if they have a test inside.
 *
 * Because of (1) we need to first launch the test with a clean Snapshot where we setup the data for taking the Snapshot,
 * and the second launch to be able to have a deployment with the Snapshot Taken to verify the data was properly loaded. Because of
 * (2) to have maps / deployments reload we need to have 2 maps. However, then (3) forces us to have the second map be a dummy
 * map which runs in Native to prevent errors of launching with Snapshots from a different map, and in order to be picked up
 * by the Automation Manager (4) it needs to have a dummy test that always passes.
 *
 * So given this setup, Automation Manager will:
 *		- Loads and starts map A with SpatialOS and runs ASpatialSnapshotTest which setups up our data and takes snapshot.
 *		- Stops map A and shuts down SpatialOS deployment.
 *		- Loads and starts map B without SpatialOS and runs ASpatialSnapshotDummyTest which just passes.
 *		- Stops map B.
 *
 * This means that we still are missing a crucial part of the test, we still didn't verify that the Snapshot loading works.
 * Remember how (3) requires us to load a Snapshot with the map it was created, so we make the Automation Manager run
 * these tests an even amount of times (more than 2 if you want to stress test it).
 *
 * The second time will be exact the same way as above, the 2 differences are that (a) map A will be launched with the
 * Snapshot taken in the first run, and (b) ASpatialSnapshotTest will know that it is running from a custom Snapshot
 * and will execute different steps.
 */

ASpatialSnapshotTest::ASpatialSnapshotTest()
	: Super()
{
	Author = "Nuno";
	Description = TEXT(
		"Test SpatialOS Snapshots. This test is expected to run twice, the first time sets up the data and takes a Snapshot and the second "
		"time loads from it and verifies the data is set.");
	SetNumRequiredClients(1);
}

void ASpatialSnapshotTest::BeginPlay()
{
	Super::BeginPlay();

	// First we need to know if we're launching from the default Snapshot or from a taken Snapshot.
	// bool bIsRunningFirstTime = !Was
}