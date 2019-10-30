// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "InertialMeasurementUnit.h"

#include <compiler/disable-ue4-macros.h>
#include "carla/geom/Vector3D.h"
#include <compiler/enable-ue4-macros.h>

#include <cmath>

#include "Carla/Sensor/WorldObserver.h"
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h"

const FVector AInertialMeasurementUnit::CarlaNorthVector =
    FVector(0.0f, -1.0f, 0.0f);

AInertialMeasurementUnit::AInertialMeasurementUnit(
    const FObjectInitializer &ObjectInitializer)
  : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;
}

FActorDefinition AInertialMeasurementUnit::GetSensorDefinition()
{
  return UActorBlueprintFunctionLibrary::MakeGenericSensorDefinition(
      TEXT("other"),
      TEXT("imu"));
}

void AInertialMeasurementUnit::Set(const FActorDescription &ActorDescription)
{

}

void AInertialMeasurementUnit::SetOwner(AActor *Owner)
{
  Super::SetOwner(Owner);
}

// Temporal copy of FWorldObserver_GetAngularVelocity
static carla::geom::Vector3D temp_FWorldObserver_GetAngularVelocity(AActor &Actor)
{
  const auto RootComponent = Cast<UPrimitiveComponent>(Actor.GetRootComponent());
  const FVector AngularVelocity =
      RootComponent != nullptr ?
          RootComponent->GetPhysicsAngularVelocityInDegrees() :
          FVector{0.0f, 0.0f, 0.0f};

  return AngularVelocity;
}

void AInertialMeasurementUnit::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  namespace cg = carla::geom;

  // Accelerometer measures linear acceleration in m/s2
  cg::Vector3D Accelerometer = FVector::OneVector;

  // Gyroscope measures angular velocity in degrees/sec
  cg::Vector3D Gyroscope = temp_FWorldObserver_GetAngularVelocity(*GetOwner());

  // Magnetometer: orientation with respect to the North,
  // that based on OpenDRIVE's lon and lat, is (0.0f, -1.0f, 0.0f)
  auto ForwVect = GetActorForwardVector().GetSafeNormal2D();
  float Compass = FMath::RadiansToDegrees(std::acos(FVector::DotProduct(CarlaNorthVector, ForwVect)));
  if (FVector::CrossProduct(CarlaNorthVector, ForwVect).Z > 0.0f)
  {
    Compass = 360.0f - Compass;
  }

  auto Stream = GetDataStream(*this);
  Stream.Send(
      *this,
      Accelerometer,
      Gyroscope,
      Compass);
}