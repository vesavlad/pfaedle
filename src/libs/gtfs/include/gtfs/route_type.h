#pragma once
namespace pfaedle::gtfs
{
enum class route_type
{
    // GTFS route types
    Tram = 0,         // Tram, Streetcar, Light rail
    Subway = 1,       // Any underground rail system within a metropolitan area
    Rail = 2,         // Intercity or long-distance travel
    Bus = 3,          // Short- and long-distance bus routes
    Ferry = 4,        // Boat service
    CableTram = 5,    // Street-level rail cars where the cable runs beneath the vehicle
    AerialLift = 6,   // Aerial lift, suspended cable car (gondola lift, aerial tramway)
    Funicular = 7,    // Any rail system designed for steep inclines
    Trolleybus = 11,  // Electric buses that draw power from overhead wires using poles
    Monorail = 12,    // Railway in which the track consists of a single rail or a beam

    // Extended route types
    // https://developers.google.com/transit/gtfs/reference/extended-route-types
    RailwayService = 100,
    HighSpeedRailService = 101,
    LongDistanceTrains = 102,
    InterRegionalRailService = 103,
    CarTransportRailService = 104,
    SleeperRailService = 105,
    RegionalRailService = 106,
    TouristRailwayService = 107,
    RailShuttleWithinComplex = 108,
    SuburbanRailway = 109,
    ReplacementRailService = 110,
    SpecialRailService = 111,
    LorryTransportRailService = 112,
    AllRailServices = 113,
    CrossCountryRailService = 114,
    VehicleTransportRailService = 115,
    RackAndPinionRailway = 116,
    AdditionalRailService = 117,

    CoachService = 200,
    InternationalCoachService = 201,
    NationalCoachService = 202,
    ShuttleCoachService = 203,
    RegionalCoachService = 204,
    SpecialCoachService = 205,
    SightseeingCoachService = 206,
    TouristCoachService = 207,
    CommuterCoachService = 208,
    AllCoachServices = 209,

    UrbanRailwayService400 = 400,
    MetroService = 401,
    UndergroundService = 402,
    UrbanRailwayService403 = 403,
    AllUrbanRailwayServices = 404,
    Monorail405 = 405,

    BusService = 700,
    RegionalBusService = 701,
    ExpressBusService = 702,
    StoppingBusService = 703,
    LocalBusService = 704,
    NightBusService = 705,
    PostBusService = 706,
    SpecialNeedsBus = 707,
    MobilityBusService = 708,
    MobilityBusForRegisteredDisabled = 709,
    SightseeingBus = 710,
    ShuttleBus = 711,
    SchoolBus = 712,
    SchoolAndPublicServiceBus = 713,
    RailReplacementBusService = 714,
    DemandAndResponseBusService = 715,
    AllBusServices = 716,

    TrolleybusService = 800,

    TramService = 900,
    CityTramService = 901,
    LocalTramService = 902,
    RegionalTramService = 903,
    SightseeingTramService = 904,
    ShuttleTramService = 905,
    AllTramServices = 906,

    WaterTransportService = 1000,
    AirService = 1100,
    FerryService = 1200,
    AerialLiftService = 1300,
    FunicularService = 1400,
    TaxiService = 1500,
    CommunalTaxiService = 1501,
    WaterTaxiService = 1502,
    RailTaxiService = 1503,
    BikeTaxiService = 1504,
    LicensedTaxiService = 1505,
    PrivateHireServiceVehicle = 1506,
    AllTaxiServices = 1507,
    MiscellaneousService = 1700,
    HorseDrawnCarriage = 1702
};
}
