#pragma once

#include "routing_common/bicycle_model.hpp"

namespace routing
{

/// Utility cycling model tuned for commuters and parents with children.
///
/// Differences from BicycleModel:
///  - Cycleways and living streets are weighted even higher (strongest preference).
///  - Primary/trunk roads are weighted much lower (near-impassable without bike infra).
///  - Off-road surfaces (tracks, paths) are heavily penalised — commuters don't want gravel.
///  - Paired with BikeCommuteEstimator, steeper climb penalty kicks in earlier.
///
/// The model reuses VehicleType::Bicycle for MWM graph loading (no map data regeneration
/// required). Only the speed weight table and estimator differ from the standard bicycle router.
class BikeCommuteModel : public BicycleModel
{
public:
  BikeCommuteModel();
  explicit BikeCommuteModel(VehicleModel::LimitsInitList const & limits);

  static BikeCommuteModel const & AllLimitsInstance();
};

class BikeCommuteModelFactory : public VehicleModelFactory
{
public:
  explicit BikeCommuteModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn = {});
};

}  // namespace routing
