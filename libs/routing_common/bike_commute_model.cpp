#include "routing_common/bike_commute_model.hpp"

// Speed table for utility cycling (commuters, parents with children).
//
// m_weight : routing cost speed — lower means the router avoids that road type
// m_eta    : actual expected travel speed shown to the user
//
// Key differences from BicycleModel:
//   - Cycleway weight raised   (21→25) : stronger preference for bike infrastructure
//   - Residential weight raised (12→16): quiet streets are the backbone of commute routing
//   - Primary weight cut        (10→3) : arterial roads near-impassable for utility cycling
//   - Trunk weight cut          (7→1.5): motorway-style roads essentially blocked
//   - Track/path weight cut     (8→2)  : gravel/dirt not suitable for commuting
//
// Surface penalty (unpaved_bad=0.3) is inherited from BicycleModel unchanged.
// The low weight speeds for track/path already make unpaved routes near-impassable.
// BikeCommuteEstimator applies a stronger climb penalty on top.

namespace routing
{
namespace bike_commute_model
{
// Inline speed constants (mirror values from bicycle_model.cpp kSpeedDismountKMpH etc.)
SpeedKMpH constexpr kDismount  = {2.0, 4.0};
SpeedKMpH constexpr kOnFootway = {8.0, 10.0};

HighwayBasedSpeeds const kCommuteSpeeds = {
    // {highway class : InOutCitySpeedKMpH(in city(weight, eta), out city(weight, eta))}
    // Arterial roads — near-blocked for utility cycling
    {HighwayType::HighwayTrunk,         InOutCitySpeedKMpH(SpeedKMpH(1.5, 17.0), SpeedKMpH(2.0, 19.0))},
    {HighwayType::HighwayTrunkLink,     InOutCitySpeedKMpH(SpeedKMpH(1.5, 17.0), SpeedKMpH(2.0, 19.0))},
    {HighwayType::HighwayPrimary,       InOutCitySpeedKMpH(SpeedKMpH(3.0, 17.0), SpeedKMpH(3.5, 19.0))},
    {HighwayType::HighwayPrimaryLink,   InOutCitySpeedKMpH(SpeedKMpH(3.0, 17.0), SpeedKMpH(3.5, 19.0))},
    // Secondary: penalised but passable when no alternative
    {HighwayType::HighwaySecondary,     InOutCitySpeedKMpH(SpeedKMpH(6.0, 17.0), SpeedKMpH(7.0, 19.0))},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(SpeedKMpH(5.0, 17.0), SpeedKMpH(6.0, 19.0))},
    // Tertiary: acceptable on quiet sections
    {HighwayType::HighwayTertiary,      InOutCitySpeedKMpH(SpeedKMpH(10.0, 17.0), SpeedKMpH(12.0, 19.0))},
    {HighwayType::HighwayTertiaryLink,  InOutCitySpeedKMpH(SpeedKMpH(9.0, 17.0),  SpeedKMpH(11.0, 19.0))},
    {HighwayType::HighwayUnclassified,  InOutCitySpeedKMpH(SpeedKMpH(13.0, 17.0), SpeedKMpH(15.0, 19.0))},
    // Residential: raised weight — backbone of utility commute routing
    {HighwayType::HighwayResidential,   InOutCitySpeedKMpH(SpeedKMpH(16.0, 14.0), SpeedKMpH(18.0, 17.0))},
    {HighwayType::HighwayService,       InOutCitySpeedKMpH(SpeedKMpH(13.0, 15.0), SpeedKMpH(15.0, 17.0))},
    {HighwayType::HighwayRoad,          InOutCitySpeedKMpH(SpeedKMpH(10.0, 15.0), SpeedKMpH(12.0, 17.0))},
    // Off-road surfaces — not appropriate for commuting
    {HighwayType::HighwayTrack,         InOutCitySpeedKMpH(SpeedKMpH(2.0, 10.0), SpeedKMpH(2.5, 12.0))},
    {HighwayType::HighwayPath,          InOutCitySpeedKMpH(SpeedKMpH(1.5, 8.0),  SpeedKMpH(2.0, 10.0))},
    {HighwayType::HighwayBridleway,     InOutCitySpeedKMpH(SpeedKMpH(1.5, 8.0),  SpeedKMpH(2.0, 10.0))},
    // Dedicated cycling infrastructure — preferred above all else
    {HighwayType::HighwayCycleway,      InOutCitySpeedKMpH(SpeedKMpH(25.0, 18.0), SpeedKMpH(27.0, 20.0))},
    // Shared low-traffic streets: very safe for utility cycling
    {HighwayType::HighwayLivingStreet,  InOutCitySpeedKMpH(SpeedKMpH(16.0, 10.0), SpeedKMpH(18.0, 12.0))},
    // Steps: dismount, extreme penalty
    {HighwayType::HighwaySteps,         InOutCitySpeedKMpH(SpeedKMpH(1.0, 1.0))},
    {HighwayType::HighwayPedestrian,    InOutCitySpeedKMpH(kDismount)},
    {HighwayType::HighwayPlatform,      InOutCitySpeedKMpH(kDismount)},
    {HighwayType::HighwayFootway,       InOutCitySpeedKMpH(kDismount)},
    {HighwayType::ManMadePier,          InOutCitySpeedKMpH(kOnFootway)},
    {HighwayType::RouteFerry,           InOutCitySpeedKMpH(SpeedKMpH(9.0, 20.0))},
};

}  // namespace bike_commute_model

BikeCommuteModel::BikeCommuteModel()
  : BicycleModel(bicycle_model::NoTrunk(), bike_commute_model::kCommuteSpeeds)
{
}

BikeCommuteModel::BikeCommuteModel(VehicleModel::LimitsInitList const & limits)
  : BicycleModel(limits, bike_commute_model::kCommuteSpeeds)
{
}

// static
BikeCommuteModel const & BikeCommuteModel::AllLimitsInstance()
{
  static BikeCommuteModel const instance(bicycle_model::AllAllowed());
  return instance;
}

BikeCommuteModelFactory::BikeCommuteModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  using namespace bicycle_model;
  using std::make_shared;

  // Default: no trunk roads — most commuters in EU/Spain should not be on trunk roads.
  m_models[""] = make_shared<BikeCommuteModel>(NoTrunk());

  // Country overrides mirror BicycleModelFactory jurisdictional access rules.
  m_models["Australia"]                = make_shared<BikeCommuteModel>(AllAllowed());
  m_models["Austria"]                  = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Belarus"]                  = make_shared<BikeCommuteModel>(DefaultOptions());
  m_models["Belgium"]                  = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Brazil"]                   = make_shared<BikeCommuteModel>(AllAllowed());
  m_models["Denmark"]                  = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["France"]                   = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Finland"]                  = make_shared<BikeCommuteModel>(DefaultOptions());
  m_models["Hungary"]                  = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Iceland"]                  = make_shared<BikeCommuteModel>(AllAllowed());
  m_models["Ireland"]                  = make_shared<BikeCommuteModel>(AllAllowed());
  m_models["Italy"]                    = make_shared<BikeCommuteModel>(DefaultOptions());
  m_models["Netherlands"]              = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Norway"]                   = make_shared<BikeCommuteModel>(AllAllowed());
  m_models["Poland"]                   = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Romania"]                  = make_shared<BikeCommuteModel>(AllAllowed());
  m_models["Russian Federation"]       = make_shared<BikeCommuteModel>(DefaultOptions());
  m_models["Slovakia"]                 = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Spain"]                    = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Sweden"]                   = make_shared<BikeCommuteModel>(DefaultOptions());
  m_models["Switzerland"]              = make_shared<BikeCommuteModel>(NoTrunk());
  m_models["Ukraine"]                  = make_shared<BikeCommuteModel>(UkraineOptions());
  m_models["United Kingdom"]           = make_shared<BikeCommuteModel>(AllAllowed());
  m_models["United States of America"] = make_shared<BikeCommuteModel>(AllAllowed());
}

}  // namespace routing
