#pragma once
#include <gtfs/types.h>
#include <gtfs/fare_payment.h>
#include <gtfs/fare_transfers.h>

namespace pfaedle::gtfs
{
// Optional dataset file
struct fare_attributes_item
{
    // Required:
    Id fare_id;
    double price = 0.0;
    CurrencyCode currency_type;
    fare_payment payment_method = fare_payment::BeforeBoarding;
    fare_transfers transfers = fare_transfers::Unlimited;

    // Conditionally required:
    Id agency_id;

    // Optional:
    size_t transfer_duration = 0;  // Length of time in seconds before a transfer expires
};
}
