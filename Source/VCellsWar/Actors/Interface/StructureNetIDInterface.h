// Copyright (c) 2026, Dmitry Tur. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StructureNetIDInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UStructureNetIDInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class VCELLSWAR_API IStructureNetIDInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent)
	int32 GetStructureNetID();
	UFUNCTION(BlueprintNativeEvent)
	void Server_SetStructureNetID(int32 NewID);
};
